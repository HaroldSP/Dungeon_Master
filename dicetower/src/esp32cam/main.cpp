#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "esp_camera.h"
#include "dice_detection.h"

// Camera pin definitions for ESP32-CAM (AI-Thinker)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

static WebServer httpServer(80);
static bool cameraInitialized = false;

static DiceDetection lastDetection = {false, 0, 0, 0, 0, 0};
static unsigned long lastDetectionTimestamp = 0;

static inline int clampCoord(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static void drawRectRGB(uint8_t* rgb, int w, int h, int x, int y, int bw, int bh, uint8_t rr, uint8_t gg, uint8_t bb) {
  if (bw <= 0 || bh <= 0) return;
  int x1 = clampCoord(x, 0, w - 1);
  int y1 = clampCoord(y, 0, h - 1);
  int x2 = clampCoord(x + bw - 1, 0, w - 1);
  int y2 = clampCoord(y + bh - 1, 0, h - 1);
  for (int i = x1; i <= x2; ++i) {
    size_t topIdx = ((size_t)y1 * w + i) * 3;
    size_t bottomIdx = ((size_t)y2 * w + i) * 3;
    rgb[topIdx] = rr; rgb[topIdx + 1] = gg; rgb[topIdx + 2] = bb;
    rgb[bottomIdx] = rr; rgb[bottomIdx + 1] = gg; rgb[bottomIdx + 2] = bb;
  }
  for (int j = y1; j <= y2; ++j) {
    size_t leftIdx = ((size_t)j * w + x1) * 3;
    size_t rightIdx = ((size_t)j * w + x2) * 3;
    rgb[leftIdx] = rr; rgb[leftIdx + 1] = gg; rgb[leftIdx + 2] = bb;
    rgb[rightIdx] = rr; rgb[rightIdx + 1] = gg; rgb[rightIdx + 2] = bb;
  }
}

static void drawDigitScaled(uint8_t* rgb, int w, int h, int x, int y, int digit, uint8_t rr, uint8_t gg, uint8_t bb, int scale) {
  if (digit < 0 || digit > 9 || scale <= 0) return;
  for (int row = 0; row < 7; ++row) {
    uint8_t bits = kDigitFont5x7[digit][row];
    for (int col = 0; col < 5; ++col) {
      if (bits & (1 << (4 - col))) {
        for (int sy = 0; sy < scale; ++sy) {
          for (int sx = 0; sx < scale; ++sx) {
            int px = x + col * scale + sx;
            int py = y + row * scale + sy;
            if ((unsigned)px < (unsigned)w && (unsigned)py < (unsigned)h) {
              size_t idx = ((size_t)py * w + px) * 3;
              rgb[idx] = rr; rgb[idx + 1] = gg; rgb[idx + 2] = bb;
            }
          }
        }
      }
    }
  }
}

static void drawValueLabel(uint8_t* rgb, int w, int h, int x, int y, int value, uint8_t rr, uint8_t gg, uint8_t bb, int scale) {
  char buf[6];
  snprintf(buf, sizeof(buf), "%d", value);
  int len = (int)strlen(buf);
  int digitWidth = 5 * scale;
  int spacing = scale;
  int cursorX = x;
  for (int i = 0; i < len; ++i) {
    int d = buf[i] - '0';
    drawDigitScaled(rgb, w, h, cursorX, y, d, rr, gg, bb, scale);
    cursorX += digitWidth + spacing;
    if (cursorX >= w) break;
  }
}

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
  snprintf(ssid, sizeof(ssid), "DiceTower-%02X%02X", mac[4], mac[5]);
  return String(ssid);
}

// --- Camera initialization ---
static bool initCamera() {
  camera_config_t config = {};
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Lower resolution for faster processing and less memory usage
  if(psramFound()){
    config.frame_size = FRAMESIZE_QVGA; // 320x240
    config.jpeg_quality = 12;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_QVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0-No Effect, 1-Negative, 2-Grayscale, 3-Red Tint, 4-Green Tint, 5-Blue Tint, 6-Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
  s->set_vflip(s, 0);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

  Serial.println("Camera initialized successfully");
  return true;
}

// --- HTTP handlers ---
static void handleRoot() {
  addNoCacheAndCors();
  const char* html =
    "<html><head><meta name='viewport' content='width=device-width, initial-scale=1'/>"
    "<style>body{font-family:sans-serif;padding:16px;max-width:640px;margin:auto}"
    "input,button{font-size:16px;padding:8px;margin:6px 0;width:100%}"
    "#mjpeg{display:block;width:320px;height:240px;max-width:100%;object-fit:contain;background:#000;margin:8px 0}</style>"
    "</head><body>"
    "<h3>DiceTower ESP32-CAM</h3>"
    "<div id='msg' style='white-space:pre-wrap;font-family:monospace;background:#111;color:#0f0;padding:8px;min-height:2em'></div>"
    "<h4>Camera Stream</h4>"
    "<img id='mjpeg' alt='Stream will appear here'/>"
    "<div style='display:flex;gap:8px'>"
      "<button onclick=startStream()>Start Stream</button>"
      "<button onclick=stopStream()>Stop Stream</button>"
    "</div>"
    "<h4>Dice Recognition</h4>"
    "<button onclick=doGet('/dice/capture')>Capture & Recognize Dice</button>"
    "<button onclick=doGet('/dice/status')>Get Last Result</button>"
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
    "function startStream(){const img=document.getElementById('mjpeg');img.src='/camera/stream';}"
    "function stopStream(){const img=document.getElementById('mjpeg');img.removeAttribute('src');img.src='';}"
    "</script>"
    "</body></html>";
  httpServer.send(200, "text/html", html);
}

static void handleStatus() {
  String json = String("{\"camera\":") + (cameraInitialized?"true":"false") + 
                ",\"dice\":{\"detected\":" + (lastDetection.detected?"true":"false") +
                ",\"value\":" + lastDetection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"x\":" + lastDetection.x +
                ",\"y\":" + lastDetection.y +
                ",\"w\":" + lastDetection.w +
                ",\"h\":" + lastDetection.h + "}}";
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

// --- MJPEG stream (/camera/stream) ---
static const char* kStreamContentType = "multipart/x-mixed-replace;boundary=frame";
static const char* kStreamBoundary = "--frame\r\n";
static const char* kStreamPartHeader = "Content-Type: image/jpeg\r\nContent-Length: ";

static void handleCameraStream() {
  WiFiClient client = httpServer.client();
  // Manually send HTTP headers to avoid chunked encoding issues
  client.print("HTTP/1.1 200 OK\r\n");
  client.print("Content-Type: "); client.print(kStreamContentType); client.print("\r\n");
  client.print("Cache-Control: no-cache\r\n");
  client.print("Connection: close\r\n\r\n");
  // Write an initial boundary
  client.write((const uint8_t*)kStreamBoundary, strlen(kStreamBoundary));

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      break;
    }

    // Convert to RGB, run detection, draw overlay, then encode to JPEG
    size_t rgbBytes = (size_t)fb->width * (size_t)fb->height * 3;
    uint8_t* rgb = (uint8_t*)malloc(rgbBytes);
    uint8_t* outJpg = nullptr;
    size_t outLen = 0;
    bool ok = false;
    DiceDetection det = {false, 0, 0, 0, 0, 0};
    if (rgb && fmt2rgb888(fb->buf, fb->len, fb->format, rgb)) {
      detectDiceFromRGB(rgb, fb->width, fb->height, det);
      if (det.detected) {
        drawRectRGB(rgb, fb->width, fb->height, det.x, det.y, det.w, det.h, 0, 255, 0);
        int scale = 3;
        int labelY = clampCoord(det.y - scale * 7 - 4, 0, fb->height - scale * 7);
        int labelX = clampCoord(det.x + 4, 0, fb->width - 1);
        drawValueLabel(rgb, fb->width, fb->height, labelX, labelY, det.value, 255, 255, 0, scale);
      }
      if (fmt2jpg(rgb, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 80, &outJpg, &outLen)) {
        ok = true;
      }
    }
    lastDetection = det;
    lastDetectionTimestamp = millis();

    // Write headers
    client.write((const uint8_t*)kStreamPartHeader, strlen(kStreamPartHeader));
    char lenStr[24];
    const uint8_t* payload = ok ? outJpg : fb->buf;
    const size_t payloadLen = ok ? outLen : fb->len;
    snprintf(lenStr, sizeof(lenStr), "%u\r\n\r\n", (unsigned)payloadLen);
    client.write((const uint8_t*)lenStr, strlen(lenStr));

    // Write JPEG payload
    client.write(payload, payloadLen);
    client.write((const uint8_t*)"\r\n", 2);

    esp_camera_fb_return(fb);
    if (rgb) free(rgb);
    if (outJpg) free(outJpg);

    // Small yield to keep Wi‑Fi happy
    delay(1);

    // Next boundary for the following frame
    client.write((const uint8_t*)kStreamBoundary, strlen(kStreamBoundary));
  }
}

static void handleDiceCapture() {
  if (!cameraInitialized) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"camera not initialized\"}");
    return;
  }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"camera capture failed\"}");
    return;
  }

  DiceDetection detection = {false, 0, 0, 0, 0, 0};
  if (fb->format == PIXFORMAT_JPEG) {
    size_t rgbBytes = (size_t)fb->width * (size_t)fb->height * 3;
    uint8_t* rgb = (uint8_t*)malloc(rgbBytes);
    if (rgb && fmt2rgb888(fb->buf, fb->len, fb->format, rgb)) {
      detectDiceFromRGB(rgb, fb->width, fb->height, detection);
    }
    if (rgb) free(rgb);
  }

  esp_camera_fb_return(fb);
  lastDetection = detection;
  lastDetectionTimestamp = millis();

  String json = String("{\"ok\":true,\"detected\":") + (detection.detected?"true":"false") +
                ",\"value\":" + detection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"x\":" + detection.x +
                ",\"y\":" + detection.y +
                ",\"w\":" + detection.w +
                ",\"h\":" + detection.h + "}";
  
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

static void handleDiceStatus() {
  String json = String("{\"ok\":true,\"detected\":") + (lastDetection.detected?"true":"false") +
                ",\"value\":" + lastDetection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"x\":" + lastDetection.x +
                ",\"y\":" + lastDetection.y +
                ",\"w\":" + lastDetection.w +
                ",\"h\":" + lastDetection.h + "}";
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
  Serial.println("Starting ESP32-CAM Dice Tower with Wi-Fi AP + HTTP API + Camera...");

  // FS init
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed, formatting...");
    LittleFS.format();
    if (!LittleFS.begin(true)) {
      Serial.println("LittleFS mount failed after format");
    }
  }

  // Initialize camera
  cameraInitialized = initCamera();

  // Wi-Fi setup
  WiFi.persistent(false);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(100);

  const String apSsid = makeApSsid();
  const char* apPassword = "dungeon123";
  WiFi.mode(WIFI_AP_STA);
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
  httpServer.on("/status", handleStatus);
  httpServer.on("/status", HTTP_OPTIONS, handleOptions);
  httpServer.on("/dice/capture", handleDiceCapture);
  httpServer.on("/dice/capture", HTTP_OPTIONS, handleOptions);
  httpServer.on("/dice/status", handleDiceStatus);
  httpServer.on("/dice/status", HTTP_OPTIONS, handleOptions);
  httpServer.on("/camera/stream", handleCameraStream);
  httpServer.on("/provision", handleProvision);
  httpServer.on("/provision", HTTP_OPTIONS, handleOptions);
  httpServer.on("/wipe", handleWipe);
  httpServer.on("/wipe", HTTP_OPTIONS, handleOptions);
  httpServer.on("/name", handleName);
  httpServer.on("/name", HTTP_OPTIONS, handleOptions);
  httpServer.onNotFound([](){
    addNoCacheAndCors();
    httpServer.send(404, "text/plain", "Not Found");
  });
  httpServer.begin();
  Serial.println("HTTP server started on port 80");
}

void loop() {
  httpServer.handleClient();
  delay(1);
}

