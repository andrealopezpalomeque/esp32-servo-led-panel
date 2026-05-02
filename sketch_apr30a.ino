#include "WiFi.h"
#include "WebServer.h"
#include <ESP32Servo.h>

#include "secrets.h"
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

WebServer server(80);
Servo myServo;

// === Servo state ===
int sweepSpeed = 50;
int minAngle = 0;
int maxAngle = 180;
int currentAngle = 90;
int direction = 1;
bool sweepMode = true;
unsigned long lastMove = 0;

// === LED state ===
const int LED_PIN = 2;
int blinkRate = 500;
bool blinkOn = true;
bool ledState = false;
unsigned long lastBlink = 0;

// === Notify pattern state ===
bool notifyMode = false;
int notifyStep = 0;
unsigned long lastNotify = 0;
const int NOTIFY_STEPS = 6;
const int notifyTiming[NOTIFY_STEPS] = {80, 80, 80, 80, 80, 1500};
const bool notifyLed[NOTIFY_STEPS]   = {true, false, true, false, true, false};

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>ESP32 Control</title>";
  html += "<style>";
  html += "*{box-sizing:border-box;margin:0;padding:0}";
  html += "body{font-family:-apple-system,BlinkMacSystemFont,'SF Pro Display',sans-serif;background:linear-gradient(135deg,#0f0f1e 0%,#1a1a2e 100%);color:#fff;min-height:100vh;padding:20px;display:flex;flex-direction:column;align-items:center}";
  html += ".header{text-align:center;margin:20px 0 30px;letter-spacing:3px;font-size:12px;opacity:0.5;text-transform:uppercase}";
  html += ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(320px,1fr));gap:20px;max-width:900px;width:100%}";
  html += ".card{background:rgba(255,255,255,0.04);backdrop-filter:blur(10px);border:1px solid rgba(255,255,255,0.08);border-radius:20px;padding:28px;transition:transform 0.2s}";
  html += ".card:hover{transform:translateY(-2px);border-color:rgba(255,255,255,0.15)}";
  html += ".card-title{font-size:11px;letter-spacing:2px;opacity:0.5;text-transform:uppercase;margin-bottom:20px;display:flex;align-items:center;gap:8px}";
  html += ".dot{width:8px;height:8px;border-radius:50%;background:#4ade80;box-shadow:0 0 12px #4ade80;animation:pulse 2s ease-in-out infinite}";
  html += "@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}";
  html += ".control{margin-bottom:22px}";
  html += ".control:last-child{margin-bottom:0}";
  html += ".row{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:10px}";
  html += ".lbl{font-size:13px;opacity:0.6}";
  html += ".val{font-size:22px;font-weight:600;font-variant-numeric:tabular-nums}";
  html += ".unit{font-size:13px;opacity:0.5;margin-left:4px}";
  html += "input[type=range]{width:100%;-webkit-appearance:none;appearance:none;height:6px;background:rgba(255,255,255,0.08);border-radius:3px;outline:none}";
  html += "input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:22px;height:22px;border-radius:50%;background:#fff;cursor:pointer;box-shadow:0 2px 8px rgba(0,0,0,0.3)}";
  html += "input[type=range]::-moz-range-thumb{width:22px;height:22px;border-radius:50%;background:#fff;cursor:pointer;border:none}";
  html += "button{font-family:inherit;font-size:14px;font-weight:500;padding:12px 20px;border-radius:12px;border:1px solid rgba(255,255,255,0.1);background:rgba(255,255,255,0.05);color:#fff;cursor:pointer;width:100%;transition:all 0.15s}";
  html += "button:hover{background:rgba(255,255,255,0.1);border-color:rgba(255,255,255,0.2)}";
  html += "button:active{transform:scale(0.98)}";
  html += "button.active{background:#4ade80;color:#0f0f1e;border-color:#4ade80}";
  html += ".tinylabel{font-size:11px;opacity:0.4;display:flex;justify-content:space-between;margin-top:6px}";
  html += "</style></head><body>";

  html += "<div class='header'>ESP32 Control Panel</div>";
  html += "<div class='grid'>";

  // SERVO CARD
  html += "<div class='card'>";
  html += "<div class='card-title'><span class='dot'></span>Servo</div>";
  html += "<div class='control'>";
  html += "<button id='modeBtn' onclick='toggleMode()' class='" + String(sweepMode ? "active" : "") + "'>" + String(sweepMode ? "Sweep Mode" : "Manual Mode") + "</button>";
  html += "</div>";
  html += "<div class='control'>";
  html += "<div class='row'><span class='lbl'>Sweep Speed</span><span class='val' id='speedVal'>" + String(sweepSpeed) + "<span class='unit'>ms</span></span></div>";
  html += "<input type='range' min='5' max='200' value='" + String(sweepSpeed) + "' oninput='setSpeed(205-this.value)'>";
  html += "<div class='tinylabel'><span>slower</span><span>faster</span></div>";
  html += "</div>";
  html += "<div class='control'>";
  html += "<div class='row'><span class='lbl'>Sweep Range</span><span class='val' id='rangeVal'>" + String(minAngle) + "&deg; &ndash; " + String(maxAngle) + "&deg;</span></div>";
  html += "<input type='range' id='minSlider' min='0' max='180' value='" + String(minAngle) + "' oninput='setMin(this.value)'>";
  html += "<div class='tinylabel'><span>min</span></div>";
  html += "<input type='range' id='maxSlider' min='0' max='180' value='" + String(maxAngle) + "' oninput='setMax(this.value)' style='margin-top:10px'>";
  html += "<div class='tinylabel'><span>max</span></div>";
  html += "</div>";
  html += "<div class='control'>";
  html += "<div class='row'><span class='lbl'>Manual Angle</span><span class='val' id='angleVal'>" + String(currentAngle) + "<span class='unit'>&deg;</span></span></div>";
  html += "<input type='range' min='0' max='180' value='" + String(currentAngle) + "' oninput='setAngle(this.value)'>";
  html += "<div class='tinylabel'><span>0&deg;</span><span>180&deg;</span></div>";
  html += "</div>";
  html += "</div>";

  // LED CARD
  html += "<div class='card'>";
  html += "<div class='card-title'><span class='dot'></span>LED</div>";
  html += "<div class='control'>";
  html += "<button id='ledBtn' onclick='toggleLed()' class='" + String(blinkOn ? "active" : "") + "'>Blink: " + String(blinkOn ? "ON" : "OFF") + "</button>";
  html += "</div>";
  html += "<div class='control'>";
  html += "<button id='notifyBtn' onclick='toggleNotify()' class='" + String(notifyMode ? "active" : "") + "'>Notify: " + String(notifyMode ? "ON" : "OFF") + "</button>";
  html += "</div>";
  html += "<div class='control'>";
  html += "<div class='row'><span class='lbl'>Blink Rate</span><span class='val' id='blinkVal'>" + String(blinkRate) + "<span class='unit'>ms</span></span></div>";
  html += "<input type='range' min='50' max='2000' value='" + String(blinkRate) + "' oninput='setBlink(this.value)'>";
  html += "<div class='tinylabel'><span>strobe</span><span>slow</span></div>";
  html += "</div>";
  html += "</div>";

  html += "</div>"; // grid

  html += "<script>";
  html += "function setSpeed(v){fetch('/speed?v='+v);document.getElementById('speedVal').innerHTML=v+'<span class=\"unit\">ms</span>';}";
  html += "function setMin(v){fetch('/min?v='+v);updateRange();}";
  html += "function setMax(v){fetch('/max?v='+v);updateRange();}";
  html += "function updateRange(){document.getElementById('rangeVal').innerHTML=document.getElementById('minSlider').value+'\\u00B0 \\u2013 '+document.getElementById('maxSlider').value+'\\u00B0';}";
  html += "function setAngle(v){fetch('/angle?v='+v);document.getElementById('angleVal').innerHTML=v+'<span class=\"unit\">\\u00B0</span>';var b=document.getElementById('modeBtn');b.innerHTML='Manual Mode';b.classList.remove('active');}";
  html += "function setBlink(v){fetch('/blink?v='+v);document.getElementById('blinkVal').innerHTML=v+'<span class=\"unit\">ms</span>';}";
  html += "function toggleLed(){fetch('/ledtoggle').then(r=>r.text()).then(t=>{var b=document.getElementById('ledBtn');b.innerHTML='Blink: '+t;b.classList.toggle('active',t==='ON');});}";
  html += "function toggleMode(){fetch('/modetoggle').then(r=>r.text()).then(t=>{var b=document.getElementById('modeBtn');b.innerHTML=t+' Mode';b.classList.toggle('active',t==='Sweep');});}";
  html += "function toggleNotify(){fetch('/notifytoggle').then(r=>r.text()).then(t=>{var b=document.getElementById('notifyBtn');b.innerHTML='Notify: '+t;b.classList.toggle('active',t==='ON');});}";
  html += "</script>";

  html += "</body></html>";
  server.send(200, "text/html", html);
}

void handleSpeed() { if (server.hasArg("v")) sweepSpeed = server.arg("v").toInt(); server.send(200, "text/plain", "OK"); }
void handleMin()   { if (server.hasArg("v")) minAngle   = server.arg("v").toInt(); server.send(200, "text/plain", "OK"); }
void handleMax()   { if (server.hasArg("v")) maxAngle   = server.arg("v").toInt(); server.send(200, "text/plain", "OK"); }
void handleBlink() { if (server.hasArg("v")) blinkRate  = server.arg("v").toInt(); server.send(200, "text/plain", "OK"); }

void handleAngle() {
  if (server.hasArg("v")) {
    currentAngle = server.arg("v").toInt();
    sweepMode = false;
    myServo.write(currentAngle);
  }
  server.send(200, "text/plain", "OK");
}

void handleLedToggle() {
  blinkOn = !blinkOn;
  if (!blinkOn) digitalWrite(LED_PIN, LOW);
  server.send(200, "text/plain", blinkOn ? "ON" : "OFF");
}

void handleModeToggle() {
  sweepMode = !sweepMode;
  server.send(200, "text/plain", sweepMode ? "Sweep" : "Manual");
}

void handleNotifyToggle() {
  notifyMode = !notifyMode;
  if (notifyMode) {
    notifyStep = 0;
    lastNotify = millis();
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  server.send(200, "text/plain", notifyMode ? "ON" : "OFF");
}

void setup() {
  Serial.begin(115200);
  myServo.attach(13);
  myServo.write(currentAngle);
  pinMode(LED_PIN, OUTPUT);

  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nConnected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/speed", handleSpeed);
  server.on("/min", handleMin);
  server.on("/max", handleMax);
  server.on("/angle", handleAngle);
  server.on("/blink", handleBlink);
  server.on("/ledtoggle", handleLedToggle);
  server.on("/modetoggle", handleModeToggle);
  server.on("/notifytoggle", handleNotifyToggle);
  server.begin();
}

void loop() {
  server.handleClient();

  if (Serial.available()) {
    char cmd = Serial.read();
    if      (cmd == 'W') { notifyMode = false; blinkOn = false; digitalWrite(LED_PIN, HIGH); }
    else if (cmd == 'N') { notifyMode = true; notifyStep = 0; lastNotify = millis(); blinkOn = false; }
    else if (cmd == 'O') { notifyMode = false; blinkOn = false; digitalWrite(LED_PIN, LOW); }
  }

  if (sweepMode && millis() - lastMove > sweepSpeed) {
    currentAngle += direction;
    if (currentAngle >= maxAngle) direction = -1;
    if (currentAngle <= minAngle) direction = 1;
    myServo.write(currentAngle);
    lastMove = millis();
  }

  if (notifyMode && millis() - lastNotify > (unsigned long)notifyTiming[notifyStep]) {
    notifyStep = (notifyStep + 1) % NOTIFY_STEPS;
    digitalWrite(LED_PIN, notifyLed[notifyStep]);
    lastNotify = millis();
  } else if (!notifyMode && blinkOn && millis() - lastBlink > blinkRate) {
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    lastBlink = millis();
  }
}