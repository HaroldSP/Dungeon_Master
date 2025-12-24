#include <Arduino.h>
// --- Wi-Fi + HTTP server to control blinking ---
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "setup/led_strip.h"

static ESP8266WebServer httpServer(80);
static bool blinkEnabled = false; // Not blinking by default (controlled via API)
static bool stripEnabled = true;
static bool ledOn = false; // logical state for built-in LED

// --- HTTP helpers ---
static void addNoCacheAndCors() {
  httpServer.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, proxy-revalidate, max-age=0");
  httpServer.sendHeader("Pragma", "no-cache");
  httpServer.sendHeader("Expires", "0");
  httpServer.sendHeader("Access-Control-Allow-Origin", "*");
  httpServer.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  httpServer.sendHeader("Access-Control-Allow-Headers", "Content-Type, Accept");
}

static void handleOptions() {
  addNoCacheAndCors();
  httpServer.send(204);
}

// --- Provisioning storage ---
struct WifiCredentials {
  String ssid;
  String pass;
  String name;
};

static const char* kCredsPath = "/creds.json";

static bool saveCredentials(const WifiCredentials& creds) {
  JsonDocument doc;
  doc["ssid"] = creds.ssid;
  doc["pass"] = creds.pass;
  doc["name"] = creds.name;
  File f = LittleFS.open(kCredsPath, "w");
  if (!f) return false;
  bool ok = (serializeJson(doc, f) > 0);
  f.close();
  return ok;
}

static bool loadCredentials(WifiCredentials& out) {
  if (!LittleFS.exists(kCredsPath)) return false;
  File f = LittleFS.open(kCredsPath, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  out.ssid = String((const char*)doc["ssid"]);
  out.pass = String((const char*)doc["pass"]);
  out.name = String((const char*)doc["name"]);
  return out.ssid.length() > 0;
}

static bool tryConnectSta(const WifiCredentials& creds, unsigned long timeoutMs = 15000, bool keepApEnabled = false) {
  // Use AP+STA mode only if we need to keep AP enabled (e.g., during provisioning from AP)
  // Otherwise use STA-only mode
  if (keepApEnabled) {
    WiFi.mode(WIFI_AP_STA);
  } else {
    WiFi.mode(WIFI_STA);
  }
  WiFi.begin(creds.ssid.c_str(), creds.pass.c_str());
  Serial.print("Connecting to WiFi SSID=\""); Serial.print(creds.ssid); Serial.println("\"...");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
    Serial.print('.');
    yield(); // Feed watchdog
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected, IP: "); Serial.println(WiFi.localIP());
    // If we were in AP+STA mode and now connected, explicitly disable AP
    if (keepApEnabled) {
      WiFi.mode(WIFI_STA);
      Serial.println("AP disabled - now in STA-only mode");
    }
    return true;
  }
  Serial.println("STA connect failed, falling back to AP");
  return false;
}

static String makeApSsid() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ssid[32];
  // Use a more descriptive AP name, e.g. "DiceTower-nodeMCU-D659"
  snprintf(ssid, sizeof(ssid), "DiceTower-nodeMCU-%02X%02X", mac[4], mac[5]);
  return String(ssid);
}

static void handleRoot() {
  addNoCacheAndCors();
  const char* html =
    "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'/>"
    "<style>body{font-family:sans-serif;padding:16px;max-width:560px;margin:auto}input,button{font-size:16px;padding:8px;margin:6px 0;width:100%}code{background:#eee;padding:2px 4px}</style>"
    "</head><body>"
    "<h3>DiceTower</h3>"
    "<div id='msg' style='white-space:pre-wrap;font-family:monospace;background:#111;color:#0f0;padding:8px;min-height:2em'></div>"
    "<h4>LED Control</h4>"
    "<button onclick=doGet('/blink/on')>Blink ON</button>"
    "<button onclick=doGet('/blink/off')>Blink OFF</button>"
    "<button onclick=doGet('/blink/toggle')>Blink TOGGLE</button>"
    "<button onclick=doGet('/status')>Get STATUS</button>"
    "<h4>Provision Wi‑Fi</h4>"
    "<input id='ssid' placeholder='SSID'/>"
    "<input id='pass' type='password' placeholder='Password (optional)'/>"
    "<button onclick=provision()>Save & Connect</button>"
    "<h4>Danger Zone</h4>"
    "<button onclick=doGet('/wipe') style='background:#b00;color:#fff'>Wipe Credentials & Reboot</button>"
    "<script>"
    "function show(o){document.getElementById('msg').textContent=(typeof o==='string')?o:JSON.stringify(o,null,2);}"
    "async function doGet(path){try{const r=await fetch(path,{cache:'no-store'});const t=await r.text();show(t);}catch(e){show('ERR '+e);}}"
    "async function provision(){const ssid=document.getElementById('ssid').value;const pass=document.getElementById('pass').value;"
    "if(!ssid){show('Missing SSID');return;}"
    "const url='/provision?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);"
    "try{const r=await fetch(url,{cache:'no-store'});const t=await r.text();show(t);}catch(e){show('ERR '+e);}}"
    "</script>"
    "</body></html>";
  httpServer.send(200, "text/html", html);
}

static void handleBlinkOn() {
  blinkEnabled = true;
  addNoCacheAndCors();
  httpServer.send(200, "application/json", "{\"ok\":true,\"blinking\":true}");
}

static void handleBlinkOff() {
  blinkEnabled = false;
  // Ensure LED off when blinking disabled
  digitalWrite(LED_BUILTIN, HIGH);
  ledOn = false;
  addNoCacheAndCors();
  httpServer.send(200, "application/json", "{\"ok\":true,\"blinking\":false}");
}

static void handleBlinkToggle() {
  blinkEnabled = !blinkEnabled;
  if (!blinkEnabled) {
    digitalWrite(LED_BUILTIN, HIGH);
    ledOn = false;
  }
  addNoCacheAndCors();
  httpServer.send(200, "application/json", String("{\"ok\":true,\"blinking\":" ) + (blinkEnabled?"true":"false") + "}");
}

static void handleStatus() {
  bool staConnected = (WiFi.status() == WL_CONNECTED);
  bool apEnabled = WiFi.getMode() & WIFI_AP;
  String mode = "unknown";
  if (apEnabled && staConnected) {
    mode = "AP+STA";
  } else if (apEnabled) {
    mode = "AP";
  } else if (staConnected) {
    mode = "STA";
  } else {
    mode = "OFF";
  }
  
  String json = String("{\"blinking\":") + (blinkEnabled?"true":"false") + 
                ",\"ledOn\":" + (ledOn?"true":"false") +
                ",\"strip\":" + (stripEnabled?"true":"false") +
                ",\"mode\":\"" + mode + "\"" +
                ",\"staConnected\":" + (staConnected?"true":"false") +
                ",\"staIp\":\"" + (staConnected ? WiFi.localIP().toString() : "") + "\"" +
                ",\"apIp\":\"" + WiFi.softAPIP().toString() + "\"" +
                "}";
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

static void handleProvision() {
  String ssid = httpServer.hasArg("ssid") ? httpServer.arg("ssid") : "";
  String pass = httpServer.hasArg("pass") ? httpServer.arg("pass") : "";
  String name = httpServer.hasArg("name") ? httpServer.arg("name") : "";
  if (ssid.length() == 0) {
    addNoCacheAndCors();
    httpServer.send(400, "application/json", "{\"ok\":false,\"error\":\"missing ssid\"}");
    return;
  }
  WifiCredentials creds{ssid, pass, name};
  bool saved = saveCredentials(creds);
  bool connected = false;
  if (saved) {
    // We're currently in AP mode, so keep AP enabled during connection attempt
    // to allow the provisioning client to stay connected
    connected = tryConnectSta(creds, 15000, true); // keepApEnabled=true
    if (connected) {
      // AP is already disabled by tryConnectSta() after successful connection
      // Explicitly ensure we're in STA-only mode
      WiFi.mode(WIFI_STA);
      Serial.println("STA connected - AP turned off, staying in STA-only mode");
      Serial.print("STA IP: "); Serial.println(WiFi.localIP());
    } else {
      // If connection failed, ensure AP is still running for retry
      WiFi.mode(WIFI_AP);
      const String apSsid = makeApSsid();
      WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
      WiFi.softAP(apSsid.c_str(), "dungeon123");
      Serial.println("STA connection failed - staying in AP mode for retry");
      Serial.print("AP SSID: "); Serial.println(apSsid);
    }
  }
  String json = String("{\"ok\":") + (saved?"true":"false") + 
                ",\"connected\":" + (connected?"true":"false") + 
                ",\"ip\":\"" + (connected ? WiFi.localIP().toString() : "") + "\"" +
                ",\"apOff\":" + (connected?"true":"false") +
                ",\"name\":\"" + creds.name + "\"}";
  addNoCacheAndCors();
  httpServer.send(saved ? 200 : 500, "application/json", json);
}

static void handleWipe() {
  bool removed = true;
  if (LittleFS.exists(kCredsPath)) {
    removed = LittleFS.remove(kCredsPath);
  }
  addNoCacheAndCors();
  httpServer.send(200, "application/json", String("{\"ok\":" ) + (removed?"true":"false") + "}\nRebooting...");
  delay(200);
  ESP.restart();
}

static void handleForceAp() {
  // Force AP mode for re-provisioning (turns off STA, starts AP)
  const String apSsid = makeApSsid();
  const char* apPassword = "dungeon123";
  
  WiFi.disconnect();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  bool apOk = WiFi.softAP(apSsid.c_str(), apPassword);
  delay(200);
  
  Serial.println("AP mode forced for re-provisioning");
  Serial.print("AP SSID: "); Serial.println(apSsid);
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
  
  String json = String("{\"ok\":") + (apOk?"true":"false") +
                ",\"ap\":true" +
                ",\"ssid\":\"" + apSsid + "\"" +
                ",\"ip\":\"" + WiFi.softAPIP().toString() + "\"}";
  addNoCacheAndCors();
  httpServer.send(apOk ? 200 : 500, "application/json", json);
}

static void handleName() {
  String action = httpServer.hasArg("action") ? httpServer.arg("action") : "";
  
  if (action == "get") {
    WifiCredentials creds;
    if (loadCredentials(creds)) {
      addNoCacheAndCors();
      httpServer.send(200, "application/json", String("{\"ok\":true,\"name\":\"") + creds.name + "\"}");
    } else {
      addNoCacheAndCors();
      httpServer.send(200, "application/json", "{\"ok\":true,\"name\":\"\"}");
    }
  } else if (action == "set") {
    String newName = httpServer.hasArg("name") ? httpServer.arg("name") : "";
    WifiCredentials creds;
    if (loadCredentials(creds)) {
      creds.name = newName;
      bool saved = saveCredentials(creds);
      addNoCacheAndCors();
      httpServer.send(saved ? 200 : 500, "application/json", String("{\"ok\":") + (saved?"true":"false") + ",\"name\":\"" + newName + "\"}");
    } else {
      addNoCacheAndCors();
      httpServer.send(404, "application/json", "{\"ok\":false,\"error\":\"no credentials found\"}");
    }
  } else {
    addNoCacheAndCors();
    httpServer.send(400, "application/json", "{\"ok\":false,\"error\":\"missing action parameter\"}");
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("Starting onboard LED blink with Wi-Fi AP + HTTP API + Provisioning...");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // off (active-low)

  // Ensure a clean Wi-Fi state on first boot after flashing
  WiFi.persistent(false);        // do not write SDK NVRAM unless we ask
  WiFi.disconnect(true);         // clear any saved STA credentials
  WiFi.mode(WIFI_OFF);
  delay(300);  // Give more time for WiFi to fully reset
  WiFi.setSleep(false);
  
  // FS init (try once, don't fail if it doesn't work)
  Serial.println("Initializing filesystem...");
  bool haveCreds = false;
  WifiCredentials savedCreds;
  if (LittleFS.begin()) {
    Serial.println("Filesystem OK");
    haveCreds = loadCredentials(savedCreds);
    if (haveCreds) {
      Serial.println("Found saved credentials - connecting to WiFi...");
    } else {
      Serial.println("No saved credentials - starting in AP mode for provisioning");
    }
  } else {
    Serial.println("LittleFS mount failed (non-critical)");
  }

  if (haveCreds) {
    // Connect to saved WiFi (STA mode only, no AP)
    bool connected = tryConnectSta(savedCreds, 15000, false); // keepApEnabled=false
    if (connected) {
      Serial.print("Connected to WiFi, IP: "); Serial.println(WiFi.localIP());
      Serial.println("Running in STA mode only (AP disabled)");
    } else {
      Serial.println("WiFi connection failed - starting AP mode for re-provisioning");
      // Fall back to AP mode if connection fails
      haveCreds = false;
    }
  }
  
  if (!haveCreds || WiFi.status() != WL_CONNECTED) {
    // No credentials or connection failed - start AP mode for provisioning
    const String apSsid = makeApSsid();
    const char* apPassword = "dungeon123";
    
    WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    WiFi.mode(WIFI_AP);
    bool apOk = WiFi.softAP(apSsid.c_str(), apPassword);
    delay(200);
    IPAddress apIp = WiFi.softAPIP();
    Serial.print("AP SSID: "); Serial.println(apSsid);
    Serial.print("AP PASS: "); Serial.println(apPassword);
    Serial.print("AP IP:   "); Serial.println(apIp);
    Serial.print("AP Status: "); Serial.println(apOk ? "OK" : "FAILED");
    
    if (!apOk) {
      Serial.println("AP start failed; retrying...");
      WiFi.mode(WIFI_OFF); 
      delay(200); 
      WiFi.mode(WIFI_AP);
      WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
      apOk = WiFi.softAP(apSsid.c_str(), apPassword);
      delay(200);
      if (apOk) {
        Serial.println("AP started on retry");
        Serial.print("AP IP: "); Serial.println(WiFi.softAPIP());
      } else {
        Serial.println("AP failed again - check hardware");
      }
    }
    Serial.println("AP mode - connect to provision Wi-Fi credentials");
  }

  // HTTP routes
  httpServer.on("/", handleRoot);
  httpServer.on("/", HTTP_OPTIONS, handleOptions);
  httpServer.on("/blink/on", handleBlinkOn);
  httpServer.on("/blink/on", HTTP_OPTIONS, handleOptions);
  httpServer.on("/blink/off", handleBlinkOff);
  httpServer.on("/blink/off", HTTP_OPTIONS, handleOptions);
  httpServer.on("/blink/toggle", handleBlinkToggle);
  httpServer.on("/blink/toggle", HTTP_OPTIONS, handleOptions);
  httpServer.on("/status", handleStatus);
  httpServer.on("/status", HTTP_OPTIONS, handleOptions);
  // LED strip control endpoints
  httpServer.on("/strip/on", [](){
    stripEnabled = true;
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"strip\":true}");
  });
  httpServer.on("/strip/on", HTTP_OPTIONS, handleOptions);
  httpServer.on("/strip/off", [](){
    stripEnabled = false;
    ledStripAllOff();
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"strip\":false}");
  });
  httpServer.on("/strip/off", HTTP_OPTIONS, handleOptions);
  httpServer.on("/strip/toggle", [](){
    stripEnabled = !stripEnabled;
    if (!stripEnabled) { ledStripAllOff(); }
    addNoCacheAndCors();
    httpServer.send(200, "application/json", String("{\"ok\":true,\"strip\":" ) + (stripEnabled?"true":"false") + "}");
  });
  httpServer.on("/strip/toggle", HTTP_OPTIONS, handleOptions);
  httpServer.on("/strip/mode/toggle", [](){
    LedStripMode cur = ledStripGetMode();
    LedStripMode next = (cur == LED_MODE_GLOW) ? LED_MODE_CHASE : LED_MODE_GLOW;
    ledStripSetMode(next);
    addNoCacheAndCors();
    httpServer.send(200, "application/json", String("{\"ok\":true,\"mode\":\"") + (next==LED_MODE_GLOW?"glow":"chase") + "\"}");
  });
  httpServer.on("/strip/mode/toggle", HTTP_OPTIONS, handleOptions);
  httpServer.on("/strip/mode/glow", [](){
    ledStripSetMode(LED_MODE_GLOW);
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"mode\":\"glow\"}");
  });
  httpServer.on("/strip/mode/glow", HTTP_OPTIONS, handleOptions);
  httpServer.on("/strip/blink", [](){
    ledStripBlink3();
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true}");
  });
  httpServer.on("/strip/blink", HTTP_OPTIONS, handleOptions);
  httpServer.on("/strip/solid", [](){
    stripEnabled = true;
    ledStripSolidOn();
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"strip\":true}");
  });
  httpServer.on("/strip/solid", HTTP_OPTIONS, handleOptions);
  // Built-in LED direct control (not blinking)
  httpServer.on("/led/on", [](){
    blinkEnabled = false; // Stop blinking
    digitalWrite(LED_BUILTIN, LOW); // On (active-low)
    ledOn = true;
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"led\":true}");
  });
  httpServer.on("/led/on", HTTP_OPTIONS, handleOptions);
  httpServer.on("/led/off", [](){
    blinkEnabled = false; // Stop blinking
    digitalWrite(LED_BUILTIN, HIGH); // Off (active-low)
    ledOn = false;
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"led\":false}");
  });
  httpServer.on("/led/off", HTTP_OPTIONS, handleOptions);
  httpServer.on("/provision", handleProvision);
  httpServer.on("/provision", HTTP_OPTIONS, handleOptions);
  httpServer.on("/wipe", handleWipe);
  httpServer.on("/wipe", HTTP_OPTIONS, handleOptions);
  httpServer.on("/force-ap", handleForceAp);
  httpServer.on("/force-ap", HTTP_OPTIONS, handleOptions);
  httpServer.on("/name", handleName);
  httpServer.on("/name", HTTP_OPTIONS, handleOptions);
  httpServer.onNotFound([](){
    addNoCacheAndCors();
    httpServer.send(404, "text/plain", "Not Found");
  });
  httpServer.begin();
  Serial.println("HTTP server started on port 80");

  // LED strip (WS2812 on D5/GPIO14) — set your actual pixel count here
  // Initialize LED strip - wrap in try/catch equivalent to prevent crashes
  Serial.println("Initializing LED strip...");
  ledStripSetup(144);
  Serial.println("LED strip initialized");
}

void loop() {
  // Feed the watchdog timer - critical for ESP8266 stability
  yield();
  
  // Handle HTTP requests
  httpServer.handleClient();
  yield();

  // Run LED strip effect only when enabled
  if (stripEnabled) {
    ledStripLoop();
    yield();  // Feed watchdog after LED operations
  }

  // Small delay to prevent tight loop and feed watchdog
  delay(10);
}


