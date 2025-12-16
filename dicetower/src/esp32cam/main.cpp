#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include "esp_camera.h"
#include "esp_http_server.h"
#include "dice_detection.h"

// Suppress deprecation warning for DynamicJsonDocument - we need fixed-size allocation
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

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

// 0 = QVGA 320x240, 1 = VGA 640x480 (runtime switchable via UI)
static int frameSizeMode = 0;

// Simple 5x7 bitmap font for drawing numeric overlays
static const uint8_t kDigitFont5x7[10][7] = {
  {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110}, // 0
  {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110}, // 1
  {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111}, // 2
  {0b01110, 0b10001, 0b00001, 0b00110, 0b00001, 0b10001, 0b01110}, // 3
  {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010}, // 4
  {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110}, // 5
  {0b00111, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110}, // 6
  {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000}, // 7
  {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110}, // 8
  {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b11100}  // 9
};

static WebServer httpServer(80);
static bool cameraInitialized = false;

static DiceDetection lastDetection = {false, 0, 0, 0, 0, 0, 0.0f, 0};
static unsigned long lastDetectionTimestamp = 0;

// Wi-Fi state tracking
static String currentApSsid;
static IPAddress currentApIp;
static bool staConnected = false;
static IPAddress staIp;

// Dedicated HTTP server for MJPEG stream
static httpd_handle_t streamHttpd = nullptr;
static volatile bool streamStopRequested = false;  // Flag to stop active stream

// Camera settings (runtime adjustable)
static int streamDelayMs = 1;  // Latency control

// External detection server (optional - set via /detection/server endpoint)
static String externalDetectionServer = "";  // e.g., "http://192.168.1.100:5000/detect"

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
  char buf[16];
  if (value == -1) {
    // Special case: draw "d20-" prefix
    snprintf(buf, sizeof(buf), "d20-");
  } else {
    snprintf(buf, sizeof(buf), "%d", value);
  }
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
    staConnected = true;
    staIp = WiFi.localIP();
    Serial.print("Connected, IP: "); Serial.println(staIp);
    return true;
  }
  staConnected = false;
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

static void tryStartMdns(const char* hostname) {
  if (!hostname || !*hostname) return;
  if (WiFi.status() != WL_CONNECTED) return;
  if (MDNS.begin(hostname)) {
    Serial.print("mDNS responder started: ");
    Serial.print(hostname);
    Serial.println(".local");
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("http", "tcp", 81);
  } else {
    Serial.println("mDNS start failed");
  }
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
  
  // Default resolution: QVGA for speed; can be switched to VGA at runtime
  if (psramFound()) {
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
    "<div style='display:flex;gap:8px;align-items:flex-start'>"
      "<div>"
        "<img id='mjpeg' alt='Stream will appear here'/>"
        "<div style='display:flex;gap:8px;margin-top:8px'>"
          "<button onclick=startStream() title='GET /camera/stream - Starts MJPEG video stream'>Start Stream</button>"
          "<button onclick=stopStream() title='Stops the video stream'>Stop Stream</button>"
        "</div>"
      "</div>"
      "<div style='display:flex;flex-direction:column;gap:8px'>"
        "<button id='mostLikelyBtn' style='min-width:80px;padding:12px;font-size:18px;font-weight:bold;background:#4CAF50;color:#fff' disabled>--</button>"
        "<button id='secondLikelyBtn' style='min-width:80px;padding:12px;font-size:18px;font-weight:bold;background:#FF9800;color:#fff' disabled>--</button>"
      "</div>"
    "</div>"
    "<h4>Dice Recognition</h4>"
    "<div style='display:flex;flex-wrap:wrap;gap:8px'>"
      "<button onclick='doGetAndUpdate(\"/dice/capture\")' title='GET /dice/capture - Captures a frame and runs local/ML detection'>Capture & Recognize Dice</button>"
      "<button onclick='doGetAndUpdate(\"/dice/capture_test\")' title='Send to test server (multipart form-data)'>Test Server</button>"
      "<button onclick='doGetAndUpdate(\"/dice/capture_gpt\")' title='GET /dice/capture_gpt - Captures a frame and asks ChatGPT to guess the value'>ChatGPT Guess</button>"
      "<button onclick=doGet('/dice/status') title='GET /dice/status - Returns last detection result'>Get Last Result</button>"
    "</div>"
    "<div id='detectionInfo' style='margin-top:10px;font-family:monospace;color:#0cf;'>Waiting for detections...</div>"
    "<button id='statusToggle' onclick=toggleStatusPolling() style='margin-top:8px'>Start Status Updates</button>"
    "<h4>External Detection Server</h4>"
    "<p style='line-height:1.4'>1) Run <code>dice_detection_server.py</code> on your PC.<br>"
    "2) Enter the full URL (e.g. <code>http://192.168.0.102:5000/detect</code>) below.<br>"
    "3) Click Save, then use Capture, Test Server, or ChatGPT to send frames through the server.</p>"
    "<input id='serverUrl' placeholder='http://192.168.0.102:5000/detect' style='width:100%;padding:8px;margin-bottom:6px'/>"
    "<div style='display:flex;flex-wrap:wrap;gap:8px;margin-bottom:6px'>"
      "<button onclick=saveDetectionServer()>Save Server URL</button>"
      "<button onclick=clearDetectionServer() style='background:#555;color:#fff'>Disable External Detection</button>"
      "<button onclick=loadDetectionServer()>Refresh Status</button>"
    "</div>"
    "<div id='serverStatus' style='font-family:monospace;background:#111;color:#0ff;padding:8px;border-radius:4px'>Detection server not configured.</div>"
    "<h4>Camera Settings</h4>"
    "<div style='margin-bottom:8px'><label>Brightness (overall lightness): <span id='brightnessVal'>2</span></label><br><input type='range' id='brightness' min='-2' max='2' step='1' value='2' oninput='updateBrightness(this.value)'/><small>Negative = darker, positive = brighter (5 steps)</small></div>"
    "<div style='margin-bottom:8px'><label>Contrast (difference between dark/light): <span id='contrastVal'>1</span></label><br><input type='range' id='contrast' min='-2' max='2' step='1' value='1' oninput='updateContrast(this.value)'/><small>Lower values flatten the image, higher values increase punch</small></div>"
    "<div style='margin-bottom:8px'><label>Saturation (color intensity): <span id='saturationVal'>1</span></label><br><input type='range' id='saturation' min='-2' max='2' step='1' value='1' oninput='updateSaturation(this.value)'/><small>-2 = grayscale, +2 = vivid colors</small></div>"
    "<div style='margin-bottom:8px'><label>Exposure Level (auto bias): <span id='aeLevelVal'>2</span></label><br><input type='range' id='aeLevel' min='-2' max='2' step='1' value='2' oninput='updateAeLevel(this.value)'/><small>Tells auto exposure to favor darker (-) or brighter (+) scenes</small></div>"
    "<div style='margin-bottom:8px'><label>Exposure Value (manual shutter): <span id='aecValueVal'>600</span></label><br><input type='range' id='aecValue' min='0' max='1200' step='1' value='600' oninput='previewAecValue(this.value)' onchange='commitAecValue(this.value)'/><small>0 = darkest, 1200 = brightest (use when auto exposure struggles)</small></div>"
    "<div style='margin-bottom:8px'><label>Stream Latency (ms): <span id='delayVal'>1</span></label><br><input type='range' id='delay' min='0' max='100' step='1' value='1' oninput='updateDelay(this.value)'/><small>Increase if Wi-Fi is noisy; 0 = lowest latency</small></div>"
    "<div style='margin-bottom:8px'><label>Resolution: <span id='resLabel'>320x240</span></label><br>"
      "<div style='display:flex;gap:8px;margin-top:4px'>"
        "<button onclick='setResolution(0)' title='Use 320x240 (faster, less detail)'>320x240</button>"
        "<button onclick='setResolution(1)' title='Use 640x480 (slower, more detail)'>640x480</button>"
      "</div>"
      "<small>Higher resolution gives sharper input for the model but uses more memory and bandwidth.</small>"
    "</div>"
    "<button onclick=loadCameraSettings()>Load Current Settings</button>"
    "<h4>Provision Wi‑Fi</h4>"
    "<input id='ssid' placeholder='SSID'/>"
    "<input id='pass' type='password' placeholder='Password (optional)'/>"
    "<button onclick=provision() title='GET /provision?ssid=...&pass=... - Saves Wi-Fi credentials and connects'>Save & Connect</button>"
    "<div id='wifiStatus' style='white-space:pre-line;font-family:monospace;border:1px solid #ccc;padding:8px;margin-top:8px;background:#f4f8ff;color:#123'>Loading Wi-Fi status...</div>"
    "<h4>API Endpoints</h4>"
    "<div style='background:#fff;border:1px solid #ccc;padding:12px;border-radius:4px;font-family:monospace;font-size:12px;line-height:1.6;color:#222'>"
    "<strong>GET /</strong> - This page<br>"
    "<strong>GET /status</strong> - Camera and dice detection status (JSON)<br>"
    "<strong>GET /camera/stream</strong> - MJPEG video stream<br>"
    "<strong>GET /dice/capture</strong> - Capture frame and detect dice (JSON)<br>"
    "<strong>GET /dice/capture_test</strong> - Capture frame and send to test server (multipart)<br>"
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
    "async function doGetAndUpdate(path){try{const r=await fetch(path,{cache:'no-store'});const t=await r.text();show(t);try{const data=JSON.parse(t);if(data.ok!==undefined && data.detected!==undefined){updateDetectionButtons({detected:!!data.detected,value:parseInt(data.value)||0,confidence:parseFloat(data.confidence)||0,second_most_likely:parseInt(data.second_most_likely)||0});}else{updateDetectionButtons(data);}try{const st=await fetch('/dice/status?ts='+Date.now(),{cache:'no-store'});const stText=await st.text();const stData=JSON.parse(stText);updateDetectionButtons(stData);}catch(err2){/* best effort */}}catch(e){console.error('Parse error:',e,t.substring(0,100));}}catch(e){show('ERR '+e);}}"
    "function updateDetectionButtons(data){const mostBtn=document.getElementById('mostLikelyBtn');const secondBtn=document.getElementById('secondLikelyBtn');const info=document.getElementById('detectionInfo');if(!mostBtn || !secondBtn){console.error('Buttons not found');return;}if(data && data.detected && data.value>0){mostBtn.textContent=data.value;mostBtn.disabled=false;if(data.second_most_likely && data.second_most_likely>0){secondBtn.textContent=data.second_most_likely;secondBtn.disabled=false;}else{secondBtn.textContent='--';secondBtn.disabled=true;}if(info){const conf=(data.confidence||0).toFixed(2);info.textContent=`Die=${data.value}  conf=${conf}`;}}else if(data && data.dice && data.dice.detected && data.dice.value>0){mostBtn.textContent=data.dice.value;mostBtn.disabled=false;if(data.dice.second_most_likely && data.dice.second_most_likely>0){secondBtn.textContent=data.dice.second_most_likely;secondBtn.disabled=false;}else{secondBtn.textContent='--';secondBtn.disabled=true;}if(info){const conf=(data.dice.confidence||0).toFixed(2);info.textContent=`Die=${data.dice.value}  conf=${conf}`;}}else{mostBtn.textContent='--';mostBtn.disabled=true;secondBtn.textContent='--';secondBtn.disabled=true;}}"
    "async function provision(){const ssid=document.getElementById('ssid').value;const pass=document.getElementById('pass').value;"
    "if(!ssid){show('Missing SSID');return;}"
    "const url='/provision?ssid='+encodeURIComponent(ssid)+'&pass='+encodeURIComponent(pass);"
    "try{const r=await fetch(url,{cache:'no-store'});const t=await r.text();show(t);}catch(e){show('ERR '+e);}}"
    "const streamHostPort=window.location.hostname+':81';"
    "const streamUrl=window.location.protocol+'//'+streamHostPort+'/camera/stream';"
    "let statusPolling=false;"
    "let statusTimer=null;"
    "function updateStatusToggleText(){const btn=document.getElementById('statusToggle');if(btn){btn.textContent=statusPolling?'Stop Status Updates':'Start Status Updates';}}"
    "function setStatusPolling(enabled){statusPolling=!!enabled;updateStatusToggleText();if(statusPolling){if(!statusTimer){pollStatus();}}else if(statusTimer){clearTimeout(statusTimer);statusTimer=null;}}"
    "function toggleStatusPolling(){setStatusPolling(!statusPolling);}"
"function startStream(){const img=document.getElementById('mjpeg');img.src=streamUrl+'?t='+Date.now();}"
"function stopStream(){const img=document.getElementById('mjpeg');img.removeAttribute('src');img.src='';setStatusPolling(false);}"
    "async function loadDetectionServer(){try{const res=await fetch('/detection/server',{cache:'no-store'});if(!res.ok)throw new Error('HTTP '+res.status);const data=await res.json();const input=document.getElementById('serverUrl');if(input && data.server_url){input.value=data.server_url;}const box=document.getElementById('serverStatus');if(box){box.textContent=(data.enabled&&data.server_url)?'Using '+data.server_url:'Detection server disabled.';}}catch(err){const box=document.getElementById('serverStatus');if(box){box.textContent='Unable to load detection server info.';}show('ERR loading server: '+err);}}"
    "async function saveDetectionServer(){const input=document.getElementById('serverUrl');const url=input?input.value.trim():'';if(!url){show('Enter server URL first');return;}try{const res=await fetch('/detection/server',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({server_url:url}),cache:'no-store'});if(!res.ok){throw new Error('HTTP '+res.status);}show('Detection server saved');loadDetectionServer();}catch(err){show('ERR saving detection server: '+err);}}"
    "async function clearDetectionServer(){try{const res=await fetch('/detection/server',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:false}),cache:'no-store'});if(!res.ok){throw new Error('HTTP '+res.status);}show('Detection server disabled');const input=document.getElementById('serverUrl');if(input){input.value='';}loadDetectionServer();}catch(err){show('ERR disabling detection server: '+err);}}"
    "async function pollStatus(){"
    "  if(!statusPolling){statusTimer=null;return;}"
    "  try{"
    "    const res=await fetch('/status',{cache:'no-store'});"
    "    const data=await res.json();"
    "    const info=document.getElementById('detectionInfo');"
    "    const mostBtn=document.getElementById('mostLikelyBtn');"
    "    const secondBtn=document.getElementById('secondLikelyBtn');"
    "    updateDetectionButtons(data);"
    "    const wifiBox=document.getElementById('wifiStatus');"
    "    if(wifiBox && data && data.wifi){"
    "      const ap=data.wifi.ap||{};"
    "      const sta=data.wifi.sta||{};"
    "      const apLine=`AP: ${ap.ssid||'n/a'} (${ap.ip||'-'})`;"
    "      const staLine=sta.connected?`STA: connected (${sta.ip||'-'})`:'STA: not connected';"
    "      wifiBox.textContent=apLine+'\\n'+staLine;"
    "    }"
    "  }catch(e){"
    "    document.getElementById('detectionInfo').textContent='Waiting for device...';"
    "  }finally{"
    "    if(statusPolling){statusTimer=setTimeout(pollStatus,2000);}else{statusTimer=null;}"
    "  }"
    "}"
    "let camSettings={brightness:2,contrast:1,saturation:1,ae_level:2,aec_value:600,stream_delay_ms:1};"
    "async function loadCameraSettings(){try{const r=await fetch('/camera/settings',{cache:'no-store'});const d=await r.json();if(d.ok){camSettings=d;document.getElementById('brightness').value=d.brightness||2;document.getElementById('brightnessVal').textContent=d.brightness||2;document.getElementById('contrast').value=d.contrast||1;document.getElementById('contrastVal').textContent=d.contrast||1;document.getElementById('saturation').value=d.saturation||1;document.getElementById('saturationVal').textContent=d.saturation||1;document.getElementById('aeLevel').value=d.ae_level||2;document.getElementById('aeLevelVal').textContent=d.ae_level||2;document.getElementById('aecValue').value=d.aec_value||600;document.getElementById('aecValueVal').textContent=d.aec_value||600;document.getElementById('delay').value=d.stream_delay_ms||1;document.getElementById('delayVal').textContent=d.stream_delay_ms||1;const mode=(typeof d.frame_size_mode==='number')?d.frame_size_mode:0;document.getElementById('resLabel').textContent=mode? '640x480':'320x240';show('Settings loaded');}}catch(e){show('ERR loading settings: '+e);}}"
    "async function updateCameraSetting(key,val){camSettings[key]=parseInt(val);try{const r=await fetch('/camera/settings',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(camSettings),cache:'no-store'});if(r.ok){show(key+' updated');}else{show('Failed to update '+key);}}catch(e){show('ERR updating '+key+': '+e);}}"
    "function updateBrightness(v){document.getElementById('brightnessVal').textContent=v;updateCameraSetting('brightness',v);}"
    "function updateContrast(v){document.getElementById('contrastVal').textContent=v;updateCameraSetting('contrast',v);}"
    "function updateSaturation(v){document.getElementById('saturationVal').textContent=v;updateCameraSetting('saturation',v);}"
"function updateAeLevel(v){document.getElementById('aeLevelVal').textContent=v;updateCameraSetting('ae_level',v);}"
"function previewAecValue(v){document.getElementById('aecValueVal').textContent=v;}"
"function commitAecValue(v){previewAecValue(v);updateCameraSetting('aec_value',v);}"
    "function updateDelay(v){document.getElementById('delayVal').textContent=v;updateCameraSetting('stream_delay_ms',v);}"
    "function setResolution(mode){mode=parseInt(mode);camSettings.frame_size_mode=mode;document.getElementById('resLabel').textContent=mode? '640x480':'320x240';updateCameraSetting('frame_size_mode',mode);}"
    "document.addEventListener('DOMContentLoaded',()=>{loadDetectionServer();});"
    "</script>"
    "</body></html>";
  httpServer.send(200, "text/html", html);
}

static void handleStatus() {
  // Use DynamicJsonDocument with fixed size to avoid heap fragmentation on ESP32
  DynamicJsonDocument doc(512);
  doc["camera"] = cameraInitialized;
  JsonObject dice = doc["dice"].to<JsonObject>();
  dice["detected"] = lastDetection.detected;
  dice["value"] = lastDetection.value;
  dice["timestamp"] = lastDetectionTimestamp;
  dice["confidence"] = lastDetection.confidence;
  dice["second_most_likely"] = lastDetection.second_most_likely;

  JsonObject wifi = doc["wifi"].to<JsonObject>();
  JsonObject ap = wifi["ap"].to<JsonObject>();
  ap["ssid"] = currentApSsid;
  ap["ip"] = currentApIp.toString();
  JsonObject sta = wifi["sta"].to<JsonObject>();
  sta["connected"] = staConnected;
  sta["ip"] = staConnected ? staIp.toString() : "";

  String json;
  serializeJson(doc, json);
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

// --- MJPEG stream (/camera/stream) ---
static const char* kStreamBoundary = "--frame";

static esp_err_t streamHttpdHandler(httpd_req_t* req) {
  if (!cameraInitialized) {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Camera not initialized");
    return ESP_FAIL;
  }
  
  // Reset stop flag when new stream starts
  streamStopRequested = false;
  
  httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=frame");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache");

  while (true) {
    // Check if stop was requested
    if (streamStopRequested) {
      break;
    }
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      return ESP_FAIL;
    }

    uint8_t* rgb = nullptr;
    uint8_t* outJpg = nullptr;
    size_t outLen = 0;
    const uint8_t* payload = fb->buf;
    size_t payloadLen = fb->len;

    size_t rgbBytes = (size_t)fb->width * (size_t)fb->height * 3;
    rgb = (uint8_t*)malloc(rgbBytes);
    if (rgb && fmt2rgb888(fb->buf, fb->len, fb->format, rgb)) {
      // Box drawing removed - using frontend buttons instead
      if (fmt2jpg(rgb, fb->width * fb->height * 3, fb->width, fb->height, PIXFORMAT_RGB888, 80, &outJpg, &outLen)) {
        payload = outJpg;
        payloadLen = outLen;
      }
    }

    char partHeader[128];
    int headerLen = snprintf(partHeader, sizeof(partHeader),
                             "%s\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
                             kStreamBoundary, (unsigned)payloadLen);
    if (httpd_resp_send_chunk(req, partHeader, headerLen) != ESP_OK ||
        httpd_resp_send_chunk(req, (const char*)payload, payloadLen) != ESP_OK ||
        httpd_resp_send_chunk(req, "\r\n", 2) != ESP_OK) {
      if (rgb) free(rgb);
      if (outJpg) free(outJpg);
      esp_camera_fb_return(fb);
      break;
    }

    esp_camera_fb_return(fb);
    if (rgb) free(rgb);
    if (outJpg) free(outJpg);

    if (streamDelayMs > 0) {
      vTaskDelay(streamDelayMs / portTICK_PERIOD_MS);
    } else {
      taskYIELD();
    }
  }

  httpd_resp_send_chunk(req, nullptr, 0);
  return ESP_OK;
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

  DiceDetection detection = {false, 0, 0, 0, 0, 0, 0.0f, 0};
  if (fb->format == PIXFORMAT_JPEG) {
    // Try external server first if configured
    if (externalDetectionServer.length() > 0 && WiFi.status() == WL_CONNECTED) {
      detectDiceFromJPEG(fb->buf, fb->len, externalDetectionServer, detection);
    }
  }

  esp_camera_fb_return(fb);
  lastDetection = detection;
  lastDetectionTimestamp = millis();

  String json = String("{\"ok\":true,\"detected\":") + (detection.detected?"true":"false") +
                ",\"value\":" + detection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"confidence\":" + String(detection.confidence, 3) +
                ",\"second_most_likely\":" + detection.second_most_likely +
                ",\"x\":" + detection.x +
                ",\"y\":" + detection.y +
                ",\"w\":" + detection.w +
                ",\"h\":" + detection.h + "}";
  
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

// Capture frame and send to GPT-backed detector on the external server.
// Uses the same externalDetectionServer URL but swaps `/detect` for `/gpt_detect`.
static void handleDiceCaptureGpt() {
  if (!cameraInitialized) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"camera not initialized\"}");
    return;
  }

  if (externalDetectionServer.length() == 0 || WiFi.status() != WL_CONNECTED) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"detection server not configured or STA not connected\"}");
    return;
  }

  // Build GPT endpoint URL from configured /detect URL
  String gptUrl = externalDetectionServer;
  int slash = gptUrl.lastIndexOf('/');
  if (slash > 0) {
    gptUrl = gptUrl.substring(0, slash) + "/gpt_detect";
  } else {
    gptUrl += "/gpt_detect";
  }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"camera capture failed\"}");
    return;
  }

  DiceDetection detection = {false, 0, 0, 0, 0, 0, 0.0f, 0};
  if (fb->format == PIXFORMAT_JPEG) {
    detectDiceFromJPEG(fb->buf, fb->len, gptUrl, detection);
  }

  esp_camera_fb_return(fb);
  lastDetection = detection;
  lastDetectionTimestamp = millis();

  String json = String("{\"ok\":true,\"detected\":") + (detection.detected?"true":"false") +
                ",\"value\":" + detection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"confidence\":" + String(detection.confidence, 3) +
                ",\"second_most_likely\":" + detection.second_most_likely + "}";
  
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

// Capture frame and send to fixed test server via multipart/form-data.
static void handleDiceCaptureTest() {
  if (!cameraInitialized) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"camera not initialized\"}");
    return;
  }

  if (WiFi.status() != WL_CONNECTED) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"STA not connected\"}");
    return;
  }

  const String testUrl = "http://192.168.0.102:8003/detect";

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    addNoCacheAndCors();
    httpServer.send(500, "application/json", "{\"ok\":false,\"error\":\"camera capture failed\"}");
    return;
  }

  DiceDetection detection = {false, 0, 0, 0, 0, 0, 0.0f, 0};
  if (fb->format == PIXFORMAT_JPEG) {
    detectDiceToTestServer(fb->buf, fb->len, testUrl, detection);
  }
  
  esp_camera_fb_return(fb);
  lastDetection = detection;
  lastDetectionTimestamp = millis();

  String json = String("{\"ok\":true,\"detected\":") + (detection.detected?"true":"false") +
                ",\"value\":" + detection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"confidence\":" + String(detection.confidence, 3) +
                ",\"second_most_likely\":" + detection.second_most_likely + "}";
  
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

static void handleDiceStatus() {
  String json = String("{\"ok\":true,\"detected\":") + (lastDetection.detected?"true":"false") +
                ",\"value\":" + lastDetection.value +
                ",\"timestamp\":" + lastDetectionTimestamp +
                ",\"confidence\":" + String(lastDetection.confidence, 3) +
                ",\"second_most_likely\":" + lastDetection.second_most_likely + "}";
  addNoCacheAndCors();
  httpServer.send(200, "application/json", json);
}

// Return a single JPEG snapshot for external processing.
static void handleCameraSnapshot() {
  if (!cameraInitialized) {
    addNoCacheAndCors();
    httpServer.send(500, "text/plain", "camera not initialized");
    return;
  }
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    addNoCacheAndCors();
    httpServer.send(500, "text/plain", "capture failed");
    return;
  }

  addNoCacheAndCors();
  httpServer.sendHeader("Content-Type", "image/jpeg");
  httpServer.sendHeader("Content-Disposition", "inline; filename=\"snapshot.jpg\"");
  httpServer.setContentLength(fb->len);
  httpServer.send(200);
  WiFiClient client = httpServer.client();
  client.write((const uint8_t*)fb->buf, fb->len);
  esp_camera_fb_return(fb);
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
  incoming.x = 0;
  incoming.y = 0;
  incoming.w = 0;
  incoming.h = 0;
  incoming.second_most_likely = doc["second_most_likely"] | 0;

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
      // Keep AP enabled so web interface is always accessible
      // WiFi.mode(WIFI_STA);  // Commented out to keep AP+STA mode
      Serial.println("STA connected, AP remains enabled for web access");
      staConnected = true;
      staIp = WiFi.localIP();
    }
  }
  if (!connected) {
    staConnected = false;
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
    doc["frame_size_mode"] = frameSizeMode;
    
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
    if (doc["frame_size_mode"].is<int>()) {
      int val = clampi((int)doc["frame_size_mode"], 0, 1);
      frameSizeMode = val;
      sensor_t * sensor = esp_camera_sensor_get();
      if (sensor) {
        sensor->set_framesize(sensor, frameSizeMode ? FRAMESIZE_VGA : FRAMESIZE_QVGA);
      }
    }
    
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  
  addNoCacheAndCors();
  httpServer.send(405, "application/json", "{\"ok\":false,\"error\":\"method not allowed\"}");
}

static void handleDetectionServer() {
  if (httpServer.method() == HTTP_OPTIONS) {
    handleOptions();
    return;
  }
  
  if (httpServer.method() == HTTP_GET) {
    JsonDocument doc;
    doc["ok"] = true;
    doc["server_url"] = externalDetectionServer;
    doc["enabled"] = (externalDetectionServer.length() > 0);
    
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
    
    if (doc["server_url"].is<String>()) {
      externalDetectionServer = doc["server_url"].as<String>();
      // Save to LittleFS for persistence
      const char* serverPath = "/detection_server.txt";
      File f = LittleFS.open(serverPath, "w");
      if (f) {
        f.print(externalDetectionServer);
        f.close();
      }
    } else if (doc["enabled"].is<bool>() && !doc["enabled"].as<bool>()) {
      externalDetectionServer = "";
      const char* serverPath = "/detection_server.txt";
      if (LittleFS.exists(serverPath)) {
        LittleFS.remove(serverPath);
      }
    }
    
    addNoCacheAndCors();
    httpServer.send(200, "application/json", "{\"ok\":true}");
    return;
  }
  
  addNoCacheAndCors();
  httpServer.send(405, "application/json", "{\"ok\":false,\"error\":\"method not allowed\"}");
}

// Stop the active MJPEG stream by setting a flag
static void handleStreamStop() {
  streamStopRequested = true;
  addNoCacheAndCors();
  httpServer.send(200, "application/json", "{\"ok\":true,\"message\":\"stream stop requested\"}");
}

static void startStreamServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 81;
  config.ctrl_port = 32817;
  config.max_resp_headers = 16;
  config.lru_purge_enable = true;
  if (httpd_start(&streamHttpd, &config) == ESP_OK) {
    httpd_uri_t streamUri = {
      .uri = "/camera/stream",
      .method = HTTP_GET,
      .handler = streamHttpdHandler,
      .user_ctx = nullptr
    };
    httpd_register_uri_handler(streamHttpd, &streamUri);
    Serial.println("Stream server started on port 81");
  } else {
    Serial.println("Failed to start stream server");
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

  // Load saved detection server URL
  const char* serverPath = "/detection_server.txt";
  if (LittleFS.exists(serverPath)) {
    File f = LittleFS.open(serverPath, "r");
    if (f) {
      externalDetectionServer = f.readString();
      f.close();
      Serial.print("Loaded detection server: ");
      Serial.println(externalDetectionServer);
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
  currentApSsid = apSsid;
  currentApIp = apIp;
  Serial.print("AP SSID: "); Serial.println(apSsid);
  Serial.print("AP PASS: "); Serial.println(apPassword);
  Serial.print("AP IP:   "); Serial.println(apIp);
  if (!apOk) {
    Serial.println("AP start failed; retrying once...");
    WiFi.mode(WIFI_OFF); delay(100); WiFi.mode(WIFI_AP_STA);
    apOk = WiFi.softAP(apSsid.c_str(), apPassword);
    apIp = WiFi.softAPIP();
    currentApIp = apIp;
    Serial.println(apOk ? "AP started on retry" : "AP failed again");
  }

  WifiCredentials creds;
  bool haveCreds = loadCredentials(creds);
  
  // If no saved credentials, try using build-time .env credentials if available
  #ifdef WIFI_SSID_ENV
  if (!haveCreds && strlen(WIFI_SSID_ENV) > 0) {
    Serial.println("Using build-time WiFi credentials from .env file");
    creds.ssid = String(WIFI_SSID_ENV);
    #ifdef WIFI_PASS_ENV
    creds.pass = String(WIFI_PASS_ENV);
    #else
    creds.pass = "";
    #endif
    creds.name = "";
    haveCreds = true;
    // Optionally save to LittleFS for persistence
    saveCredentials(creds);
  }
  #endif
  
  if (haveCreds) {
    (void)tryConnectSta(creds, 12000);
    if (WiFi.status() == WL_CONNECTED) {
      tryStartMdns("dicetower");
    }
  }

  // HTTP routes
  httpServer.on("/", handleRoot);
  httpServer.on("/", HTTP_OPTIONS, handleOptions);
  httpServer.on("/status", handleStatus);
  httpServer.on("/status", HTTP_OPTIONS, handleOptions);
  httpServer.on("/dice/capture", handleDiceCapture);
  httpServer.on("/dice/capture", HTTP_OPTIONS, handleOptions);
  httpServer.on("/dice/capture_gpt", handleDiceCaptureGpt);
  httpServer.on("/dice/capture_gpt", HTTP_OPTIONS, handleOptions);
  httpServer.on("/dice/status", handleDiceStatus);
  httpServer.on("/dice/status", HTTP_OPTIONS, handleOptions);
  httpServer.on("/external/detection", HTTP_POST, handleExternalDetection);
  httpServer.on("/external/detection", HTTP_OPTIONS, handleOptions);
  httpServer.on("/camera/snapshot", HTTP_GET, handleCameraSnapshot);
  httpServer.on("/camera/snapshot", HTTP_OPTIONS, handleOptions);
  httpServer.on("/camera/stream/stop", HTTP_POST, handleStreamStop);
  httpServer.on("/camera/stream/stop", HTTP_OPTIONS, handleOptions);
  httpServer.on("/provision", handleProvision);
  httpServer.on("/provision", HTTP_OPTIONS, handleOptions);
  httpServer.on("/wipe", handleWipe);
  httpServer.on("/wipe", HTTP_OPTIONS, handleOptions);
  httpServer.on("/name", handleName);
  httpServer.on("/name", HTTP_OPTIONS, handleOptions);
  httpServer.on("/camera/settings", HTTP_GET, handleCameraSettings);
  httpServer.on("/camera/settings", HTTP_POST, handleCameraSettings);
  httpServer.on("/camera/settings", HTTP_OPTIONS, handleOptions);
  httpServer.on("/detection/server", HTTP_GET, handleDetectionServer);
  httpServer.on("/detection/server", HTTP_POST, handleDetectionServer);
  httpServer.on("/detection/server", HTTP_OPTIONS, handleOptions);
  httpServer.on("/dice/capture_test", handleDiceCaptureTest);
  httpServer.on("/dice/capture_test", HTTP_OPTIONS, handleOptions);
  httpServer.on("/whoami", HTTP_GET, []() {
    JsonDocument doc;
    doc["ok"] = true;
    doc["sta_connected"] = (WiFi.status() == WL_CONNECTED);
    doc["sta_ip"] = WiFi.localIP().toString();
    doc["ap_ip"] = WiFi.softAPIP().toString();
    doc["ap_ssid"] = currentApSsid;
    String json;
    serializeJson(doc, json);
    addNoCacheAndCors();
    httpServer.send(200, "application/json", json);
  });
  httpServer.onNotFound([](){
    addNoCacheAndCors();
    httpServer.send(404, "text/plain", "Not Found");
  });
  httpServer.begin();
  Serial.println("HTTP server started on port 80");

  if (cameraInitialized) {
    startStreamServer();
  }
}
#pragma GCC diagnostic pop

void loop() {
  httpServer.handleClient();
  delay(1);
}

