#include <Arduino.h>
// --- Wi-Fi + HTTP server to control blinking ---
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "NodeMCU/setup/led_strip.h"

static ESP8266WebServer httpServer(80);
static bool blinkEnabled = false; // Not blinking by default
static bool stripEnabled = true;
static unsigned long lastToggleMs = 0;
static const unsigned long blinkIntervalMs = 500;
static bool ledOn = false; // logical state

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

static bool tryConnectSta(const WifiCredentials& creds, unsigned long timeoutMs = 15000) {
  // Keep AP up while attempting STA connect to avoid breaking active clients
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(creds.ssid.c_str(), creds.pass.c_str());
  Serial.print("Connecting to WiFi SSID=\""); Serial.print(creds.ssid); Serial.println("\"...");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("Connected, IP: "); Serial.println(WiFi.localIP());
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
  String json = String("{\"blinking\":") + (blinkEnabled?"true":"false") + 
                ",\"ledOn\":" + (ledOn?"true":"false") +
                ",\"strip\":" + (stripEnabled?"true":"false") + "}";
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
    connected = tryConnectSta(creds);
    // Auto-disable AP after successful STA connection to reduce interference
    if (connected) {
      WiFi.mode(WIFI_STA);
      Serial.println("AP disabled after successful STA connection");
    }
  }
  String json = String("{\"ok\":") + (saved?"true":"false") + 
                ",\"connected\":" + (connected?"true":"false") + 
                ",\"ip\":\"" + (connected ? WiFi.localIP().toString() : "") + 
                "\",\"name\":\"" + creds.name + "\"}";
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

static void handleApToggle() {
  String action = httpServer.hasArg("action") ? httpServer.arg("action") : "";
  bool apEnabled = WiFi.getMode() & WIFI_AP;
  
  if (action == "off" && apEnabled) {
    WiFi.mode(WIFI_STA);
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"ap\":false}");
  } else if (action == "on" && !apEnabled) {
    WiFi.mode(WIFI_AP_STA);
    const String apSsid = makeApSsid();
    WiFi.softAP(apSsid.c_str(), "dungeon123");
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true,\"ap\":true}");
  } else {
    addNoCacheAndCors();
    httpServer.send(200, "application/json", String("{\"ok\":true,\"ap\":") + (apEnabled?"true":"false") + "}");
  }
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
  delay(100);
  WiFi.setSleep(false);

  // FS init
  if (!LittleFS.begin()) {
    Serial.println("LittleFS mount failed");
  }

  // Always bring up AP for provisioning, and try STA if creds exist
  const String apSsid = makeApSsid();
  const char* apPassword = "dungeon123"; // simple default
  WiFi.mode(WIFI_AP_STA);
  // WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
  bool apOk = WiFi.softAP(apSsid.c_str(), apPassword);
  delay(100);
  IPAddress apIp = WiFi.softAPIP();
  Serial.print("AP SSID: "); Serial.println(apSsid);
  Serial.print("AP PASS: "); Serial.println(apPassword);
  Serial.print("AP IP:   "); Serial.println(apIp);
  if (!apOk) {
    Serial.println("AP start failed; retrying once...");
    WiFi.mode(WIFI_OFF); delay(100); WiFi.mode(WIFI_AP_STA);
    apOk = WiFi.softAP(apSsid.c_str(), apPassword);
    Serial.println(apOk ? "AP started on retry" : "AP failed again");
  }

  WifiCredentials creds;
  bool haveCreds = loadCredentials(creds);
  if (haveCreds) {
    (void)tryConnectSta(creds, 12000);
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
  httpServer.on("/ap/toggle", handleApToggle);
  httpServer.on("/ap/toggle", HTTP_OPTIONS, handleOptions);
  httpServer.on("/name", handleName);
  httpServer.on("/name", HTTP_OPTIONS, handleOptions);
  httpServer.onNotFound([](){
    addNoCacheAndCors();
    httpServer.send(404, "text/plain", "Not Found");
  });
  httpServer.begin();
  Serial.println("HTTP server started on port 80");

  // LED strip (WS2812 on D2) — set your actual pixel count here
  ledStripSetup(144);
}

void loop() {
  httpServer.handleClient();

  // Run LED strip effect only when enabled
  if (stripEnabled) {
    ledStripLoop();
  }

  if (!blinkEnabled) {
    return;
  }

  const unsigned long now = millis();
  if (now - lastToggleMs >= blinkIntervalMs) {
    lastToggleMs = now;
    ledOn = !ledOn;
    // Active-low LED on ESP8266
    digitalWrite(LED_BUILTIN, ledOn ? LOW : HIGH);
  }
}


