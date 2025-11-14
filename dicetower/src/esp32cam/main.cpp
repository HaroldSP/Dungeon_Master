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

static constexpr bool kOverlayEnabled = true;

static WebServer httpServer(80);
static bool cameraInitialized = false;

static DiceDetection lastDetection = {false, 0, 0, 0, 0, 0, 0.0f};
static unsigned long lastDetectionTimestamp = 0;

// Camera settings (runtime adjustable)
static int streamDelayMs = 1;  // Latency control

static inline int clampCoord(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static inline int clampi(int v, int lo, int hi) {
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
  s->set_brightness(s, 2);     // -2 to 2
  s->set_contrast(s, 1);       // -2 to 2
  s->set_saturation(s, 1);     // -2 to 2
  s->set_special_effect(s, 2); // 2 = Grayscale output
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 2);       // -2 to 2
  s->set_aec_value(s, 600);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)2);  // 0 to 6
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
      "<button onclick=startStream() title='GET /camera/stream - Starts MJPEG video stream'>Start Stream</button>"
      "<button onclick=stopStream() title='Stops the video stream'>Stop Stream</button>"
    "</div>"
    "<h4>Dice Recognition</h4>"
    "<button onclick=doGet('/dice/capture') title='GET /dice/capture - Captures a frame and runs dice detection'>Capture & Recognize Dice</button>"
    "<button onclick=doGet('/dice/status') title='GET /dice/status - Returns last detection result'>Get Last Result</button>"
    "<div id='detectionInfo' style='margin-top:10px;font-family:monospace;color:#0cf;'>Waiting for detections...</div>"
    "<h4>Camera Settings</h4>"
    "<div style='margin-bottom:8px'><label>Brightness: <span id='brightnessVal'>2</span></label><br><input type='range' id='brightness' min='-2' max='2' value='2' oninput='updateBrightness(this.value)'/></div>"
    "<div style='margin-bottom:8px'><label>Contrast: <span id='contrastVal'>1</span></label><br><input type='range' id='contrast' min='-2' max='2' value='1' oninput='updateContrast(this.value)'/></div>"
    "<div style='margin-bottom:8px'><label>Saturation: <span id='saturationVal'>1</span></label><br><input type='range' id='saturation' min='-2' max='2' value='1' oninput='updateSaturation(this.value)'/></div>"
    "<div style='margin-bottom:8px'><label>Exposure Level: <span id='aeLevelVal'>2</span></label><br><input type='range' id='aeLevel' min='-2' max='2' value='2' oninput='updateAeLevel(this.value)'/></div>"
    "<div style='margin-bottom:8px'><label>Exposure Value: <span id='aecValueVal'>600</span></label><br><input type='range' id='aecValue' min='0' max='1200' step='10' value='600' oninput='updateAecValue(this.value)'/></div>"
    "<div style='margin-bottom:8px'><label>Stream Latency (ms): <span id='delayVal'>1</span></label><br><input type='range' id='delay' min='0' max='100' value='1' oninput='updateDelay(this.value)'/></div>"
    "<button onclick=loadCameraSettings()>Load Current Settings</button>"
    "<h4>Provision Wi‑Fi</h4>"
    "<input id='ssid' placeholder='SSID'/>"
    "<input id='pass' type='password' placeholder='Password (optional)'/>"
    "<button onclick=provision() title='GET /provision?ssid=...&pass=... - Saves Wi-Fi credentials and connects'>Save & Connect</button>"
    "<h4>API Endpoints</h4>"
    "<div style='background:#fff;border:1px solid #ccc;padding:12px;border-radius:4px;font-family:monospace;font-size:12px;line-height:1.6;color:#222'>"
    "<strong>GET /</strong> - This page<br>"
    "<strong>GET /status</strong> - Camera and dice detection status (JSON)<br>"
    "<strong>GET /camera/stream</strong> - MJPEG video stream<br>"
    "<strong>GET /dice/capture</strong> - Capture frame and detect dice (JSON)<br>"
    "<strong>GET /dice/status</strong> - Last detection result (JSON)<br>"
    "<strong>POST /external/detection</strong> - Accept external detection (JSON body)<br>"
    "<strong>GET /provision?ssid=...&pass=...</strong> - Save Wi-Fi credentials<br>"
    "<strong>GET /name?action=get</strong> - Get device name<br>"
    "<strong>GET /name?action=set&name=...</strong> - Set device name<br>"
    "<strong>GET /camera/settings</strong> - Get camera settings (JSON)<br>"
    "<strong>POST /camera/settings</strong> - Update camera settings (JSON body)<br>"
    "<strong>GET /wipe</strong> - Wipe credentials and reboot<br>"
    "</div>"
    "<h4>Danger Zone</h4>"
    "<button onclick=doGet('/wipe') style='background:#b00;color:#fff' title='GET /wipe - Wipes saved Wi-Fi credentials and reboots'>Wipe Credentials & Reboot</button>"
    "<script>"
    "function show(o){document.getElementById('msg').textContent=(typeof o==='string')?o:JSON.stringify(o,null,2);}"
    "async function doGet(path){try{const r=await fetch(path,{cache:'no-store'});const t=await r.text();show(t);}catch(e){show('ERR '+e);}}"
    "async function provision(){const ssid=document.getElementById('ssid').value;const pass=document.getElementById('pass').value;"
    "if(!ssid){show('Missing SSID');return;}"
    "const url='/provision?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);"
    "try{const r=await fetch(url,{cache:'no-store'});const t=await r.text();show(t);}catch(e){show('ERR '+e);}}"
    "let statusPolling=true;"
    "function startStream(){const img=document.getElementById('mjpeg');img.src='/camera/stream';statusPolling=true;}"
    "function stopStream(){const img=document.getElementById('mjpeg');img.removeAttribute('src');img.src='';statusPolling=false;}"
    "async function pollStatus(){"
    "  try{"
    "    if(!statusPolling){setTimeout(pollStatus,2000);return;}"
    "    const res=await fetch('/status',{cache:'no-store'});"
    "    const data=await res.json();"
    "    const info=document.getElementById('detectionInfo');"
    "    if(data && data.dice && data.dice.detected){"
    "      const conf=(data.dice.confidence||0).toFixed(2);"
    "      info.textContent=`Die=${data.dice.value}  conf=${conf}  device_ms=${data.dice.timestamp}`;"
    "    }else{"
    "      info.textContent='No detection';"
    "    }"
    "  }catch(e){"
    "    document.getElementById('detectionInfo').textContent='Waiting for device...';"
    "  }finally{"
    "    setTimeout(pollStatus,2000);"
    "  }"
    "}"
    "pollStatus();"
    "let camSettings={brightness:2,contrast:1,saturation:1,ae_level:2,aec_value:600,stream_delay_ms:1};"
    "async function loadCameraSettings(){try{const r=await fetch('/camera/settings',{cache:'no-store'});const d=await r.json();if(d.ok){camSettings=d;document.getElementById('brightness').value=d.brightness||2;document.getElementById('brightnessVal').textContent=d.brightness||2;document.getElementById('contrast').value=d.contrast||1;document.getElementById('contrastVal').textContent=d.contrast||1;document.getElementById('saturation').value=d.saturation||1;document.getElementById('saturationVal').textContent=d.saturation||1;document.getElementById('aeLevel').value=d.ae_level||2;document.getElementById('aeLevelVal').textContent=d.ae_level||2;document.getElementById('aecValue').value=d.aec_value||600;document.getElementById('aecValueVal').textContent=d.aec_value||600;document.getElementById('delay').value=d.stream_delay_ms||1;document.getElementById('delayVal').textContent=d.stream_delay_ms||1;show('Settings loaded');}}catch(e){show('ERR loading settings: '+e);}}"
    "async function updateCameraSetting(key,val){camSettings[key]=parseInt(val);try{const r=await fetch('/camera/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(camSettings),cache:'no-store'});if(r.ok){show(key+' updated');}else{show('Failed to update '+key);}}catch(e){show('ERR updating '+key+': '+e);}}"
    "function updateBrightness(v){document.getElementById('brightnessVal').textContent=v;updateCameraSetting('brightness',v);}"
    "function updateContrast(v){document.getElementById('contrastVal').textContent=v;updateCameraSetting('contrast',v);}"
    "function updateSaturation(v){document.getElementById('saturationVal').textContent=v;updateCameraSetting('saturation',v);}"
    "function updateAeLevel(v){document.getElementById('aeLevelVal').textContent=v;updateCameraSetting('ae_level',v);}"
    "function updateAecValue(v){document.getElementById('aecValueVal').textContent=v;updateCameraSetting('aec_value',v);}"
    "function updateDelay(v){document.getElementById('delayVal').textContent=v;updateCameraSetting('stream_delay_ms',v);}"
    "</script>"
    "</body></html>";
  httpServer.send(200, "text/html", html);
}

static void handleStatus() {
  String json = String("{\"camera\":") + (cameraInitialized?"true":"false") + 
                ",\"dice\":{\"detected\":" + (lastDetection.detected?"true":"false") +
                ",\"value\":" + lastDetection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"confidence\":" + String(lastDetection.confidence, 3) +
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
    DiceDetection det = {false, 0, 0, 0, 0, 0, 0.0f};
    if (rgb && fmt2rgb888(fb->buf, fb->len, fb->format, rgb)) {
      if (kOverlayEnabled) {
        detectDiceFromRGB(rgb, fb->width, fb->height, det);
        if (det.detected) {
          drawRectRGB(rgb, fb->width, fb->height, det.x, det.y, det.w, det.h, 0, 255, 0);
          int scale = 3;
          int labelY = clampCoord(det.y - scale * 7 - 4, 0, fb->height - scale * 7);
          int labelX = clampCoord(det.x + 4, 0, fb->width - 1);
          drawValueLabel(rgb, fb->width, fb->height, labelX, labelY, det.value, 255, 255, 0, scale);
        }
        lastDetection = det;
        lastDetectionTimestamp = millis();
      }
      if (fmt2jpg(rgb, fb->width*fb->height*3, fb->width, fb->height, PIXFORMAT_RGB888, 80, &outJpg, &outLen)) {
        ok = true;
      }
    }

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

    // Configurable delay to control latency
    delay(streamDelayMs);

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

  DiceDetection detection = {false, 0, 0, 0, 0, 0, 0.0f};
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
                ",\"confidence\":" + String(lastDetection.confidence, 3) +
                ",\"x\":" + lastDetection.x +
                ",\"y\":" + lastDetection.y +
                ",\"w\":" + lastDetection.w +
                ",\"h\":" + lastDetection.h + "}";
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

static void handleExternalDetection() {
  if (httpServer.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }
  if (httpServer.method() != HTTP_POST) {
    addNoCacheAndCors();
    httpServer.send(405, "application/json", "{\"ok\":false,\"error\":\"method not allowed\"}");
    return;
  }
  String body = httpServer.arg("plain");
  if (!body.length()) {
    addNoCacheAndCors();
    httpServer.send(400, "application/json", "{\"ok\":false,\"error\":\"empty body\"}");
    return;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    addNoCacheAndCors();
    httpServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid json\"}");
    return;
  }

  DiceDetection incoming = lastDetection;
  incoming.detected = doc["detected"] | true;
  incoming.value = doc["value"] | incoming.value;
  incoming.confidence = doc["confidence"] | incoming.confidence;
  incoming.x = doc["x"] | 0;
  incoming.y = doc["y"] | 0;
  incoming.w = doc["w"] | 0;
  incoming.h = doc["h"] | 0;

  lastDetection = incoming;
  lastDetectionTimestamp = millis();

  addNoCacheAndCors();
  httpServer.send(200, "application/json", "{\"ok\":true}");
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

static void handleCameraSettings() {
  if (httpServer.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }
  
  if (!cameraInitialized) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"camera not initialized\"}");
    return;
  }
  
  sensor_t * s = esp_camera_sensor_get();
  if (!s) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"sensor not available\"}");
    return;
  }
  
  if (httpServer.method() == HTTP_GET) {
    // Return current settings
    JsonDocument doc;
    doc["ok"] = true;
    doc["brightness"] = 2;  // Default, sensor doesn't expose getter
    doc["contrast"] = 1;
    doc["saturation"] = 1;
    doc["special_effect"] = 2;
    doc["ae_level"] = 2;
    doc["aec_value"] = 600;
    doc["agc_gain"] = 0;
    doc["gainceiling"] = 2;
    doc["stream_delay_ms"] = streamDelayMs;
    
    String json;
    serializeJson(doc, json);
    addNoCacheAndCors();
    httpServer.send(200, "application/json", json);
    return;
  }
  
  if (httpServer.method() == HTTP_POST) {
    String body = httpServer.arg("plain");
    if (!body.length()) {
      addNoCacheAndCors();
      httpServer.send(400, "application/json", "{\"ok\":false,\"error\":\"empty body\"}");
      return;
    }
    
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, body);
    if (err) {
      addNoCacheAndCors();
      httpServer.send(400, "application/json", "{\"ok\":false,\"error\":\"invalid json\"}");
      return;
    }
    
    // Apply settings
    if (doc["brightness"].is<int>()) {
      int val = doc["brightness"];
      s->set_brightness(s, clampi(val, -2, 2));
    }
    if (doc["contrast"].is<int>()) {
      int val = doc["contrast"];
      s->set_contrast(s, clampi(val, -2, 2));
    }
    if (doc["saturation"].is<int>()) {
      int val = doc["saturation"];
      s->set_saturation(s, clampi(val, -2, 2));
    }
    if (doc["special_effect"].is<int>()) {
      int val = doc["special_effect"];
      s->set_special_effect(s, clampi(val, 0, 6));
    }
    if (doc["ae_level"].is<int>()) {
      int val = doc["ae_level"];
      s->set_ae_level(s, clampi(val, -2, 2));
    }
    if (doc["aec_value"].is<int>()) {
      int val = doc["aec_value"];
      s->set_aec_value(s, clampi(val, 0, 1200));
    }
    if (doc["agc_gain"].is<int>()) {
      int val = doc["agc_gain"];
      s->set_agc_gain(s, clampi(val, 0, 30));
    }
    if (doc["gainceiling"].is<int>()) {
      int val = doc["gainceiling"];
      s->set_gainceiling(s, (gainceiling_t)clampi(val, 0, 6));
    }
    if (doc["stream_delay_ms"].is<int>()) {
      int val = doc["stream_delay_ms"];
      streamDelayMs = clampi(val, 0, 100);
    }
    
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  
  addNoCacheAndCors();
  httpServer.send(405, "application/json", "{\"ok\":false,\"error\":\"method not allowed\"}");
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
  httpServer.on("/external/detection", HTTP_POST, handleExternalDetection);
  httpServer.on("/external/detection", HTTP_OPTIONS, handleOptions);
  httpServer.on("/provision", handleProvision);
  httpServer.on("/provision", HTTP_OPTIONS, handleOptions);
  httpServer.on("/wipe", handleWipe);
  httpServer.on("/wipe", HTTP_OPTIONS, handleOptions);
  httpServer.on("/name", handleName);
  httpServer.on("/name", HTTP_OPTIONS, handleOptions);
  httpServer.on("/camera/settings", HTTP_GET, handleCameraSettings);
  httpServer.on("/camera/settings", HTTP_POST, handleCameraSettings);
  httpServer.on("/camera/settings", HTTP_OPTIONS, handleOptions);
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

