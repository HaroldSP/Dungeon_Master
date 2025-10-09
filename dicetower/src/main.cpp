#include <Arduino.h>
// --- Wi-Fi + HTTP server to control blinking ---
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

static ESP8266WebServer httpServer(80);
static bool blinkEnabled = true;
static unsigned long lastToggleMs = 0;
static const unsigned long blinkIntervalMs = 500;
static bool ledOn = false; // logical state

static String makeApSsid() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "DiceTower-%02X%02X", mac[4], mac[5]);
  return String(ssid);
}

static void handleRoot() {
  httpServer.send(200, "text/plain", "DiceTower online. Use /blink/on or /blink/off or /status");
}

static void handleBlinkOn() {
  blinkEnabled = true;
  httpServer.send(200, "application/json", "{\"ok\":true,\"blinking\":true}");
}

static void handleBlinkOff() {
  blinkEnabled = false;
  // Ensure LED off when blinking disabled
  digitalWrite(LED_BUILTIN, HIGH);
  ledOn = false;
  httpServer.send(200, "application/json", "{\"ok\":true,\"blinking\":false}");
}

static void handleBlinkToggle() {
  blinkEnabled = !blinkEnabled;
  if (!blinkEnabled) {
    digitalWrite(LED_BUILTIN, HIGH);
    ledOn = false;
  }
  httpServer.send(200, "application/json", String("{\"ok\":true,\"blinking\":" ) + (blinkEnabled?"true":"false") + "}");
}

static void handleStatus() {
  String json = String("{\"blinking\":") + (blinkEnabled?"true":"false") + 
                ",\"ledOn\":" + (ledOn?"true":"false") + "}";
  httpServer.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(50);
  Serial.println("Starting onboard LED blink with Wi-Fi AP + HTTP API...");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); // off (active-low)

  // Start Wi-Fi AP for simple first-run control
  const String apSsid = makeApSsid();
  const char* apPassword = "dungeon123"; // simple default
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid.c_str(), apPassword);
  IPAddress ip = WiFi.softAPIP();
  Serial.print("AP SSID: "); Serial.println(apSsid);
  Serial.print("AP PASS: "); Serial.println(apPassword);
  Serial.print("AP IP:   "); Serial.println(ip);

  // HTTP routes
  httpServer.on("/", handleRoot);
  httpServer.on("/blink/on", handleBlinkOn);
  httpServer.on("/blink/off", handleBlinkOff);
  httpServer.on("/blink/toggle", handleBlinkToggle);
  httpServer.on("/status", handleStatus);
  httpServer.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  httpServer.handleClient();

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


