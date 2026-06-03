#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

const int PIN_PANIC_BTN = 12;
const int PIN_DOOR_SENSOR = 13;
const int PIN_PANIC_LED = 2;
const int PIN_SYSTEM_LED = 4;

LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

bool panicActive = false;
bool doorOpen = false;
float latitude = -23.5505;
float longitude = -46.6333;
String lastUpdate = "Never";

const char* ssid = "Wokwi-GUEST";
const char* password = "";

void updateLCD() {
  lcd.clear();

  lcd.setCursor(0, 0);

  if (panicActive) {
    lcd.print("PANICO ATIVO");
  } else {
    lcd.print("SISTEMA OK");
  }

  lcd.setCursor(0, 1);

  if (doorOpen) {
    lcd.print("PORTA ABERTA");
  } else {
    lcd.print("PORTA FECHADA");
  }
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>AstroTrack - Dashboard</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    h2 {font-size: 2.0rem;}
    p {font-size: 1.2rem;}
    body {margin:0; background-color: #f4f7f6;}
    .topnav {overflow: hidden; background-color: #0c1c4c; color: white; font-size: 1.5rem; padding: 15px;}
    .content {padding: 20px;}
    .card {background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); padding: 20px; border-radius: 10px; margin: 10px;}
    .cards {max-width: 800px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));}
    .status {font-weight: bold; padding: 5px 10px; border-radius: 5px;}
    .active {background-color: #ff4d4d; color: white;}
    .inactive {background-color: #4CAF50; color: white;}
    .btn {background-color: #0c1c4c; border: none; color: white; padding: 10px 20px; text-decoration: none; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 5px;}
    .btn-panic {background-color: #d32f2f;}
  </style>
</head>
<body>
  <div class="topnav">
    <i class="fas fa-satellite"></i> AstroTrack IoT Dashboard
  </div>
  <div class="content">
    <div class="cards">
      <div class="card">
        <i class="fas fa-exclamation-triangle" style="color:#d32f2f; font-size: 2rem;"></i>
        <h3>Status de Pânico</h3>
        <p><span id="panic-status" class="status">CARREGANDO...</span></p>
        <button class="btn btn-panic" onclick="togglePanic()">ALTERNAR PÂNICO</button>
      </div>
      <div class="card">
        <i class="fas fa-truck" style="color:#0c1c4c; font-size: 2rem;"></i>
        <h3>Integridade da Carga</h3>
        <p><span id="door-status" class="status">CARREGANDO...</span></p>
      </div>
      <div class="card">
        <i class="fas fa-map-marker-alt" style="color:#2196F3; font-size: 2rem;"></i>
        <h3>Localização GPS</h3>
        <p>Lat: <span id="lat">0.00</span></p>
        <p>Lng: <span id="lng">0.00</span></p>
      </div>
    </div>
  </div>
<script>
function updateStatus() {
  fetch('/status')
    .then(response => response.json())
    .then(data => {
      const panicEl = document.getElementById('panic-status');
      if(data.panic) {
        panicEl.innerText = "ALERTA ATIVO";
        panicEl.className = "status active";
      } else {
        panicEl.innerText = "SISTEMA SEGURO";
        panicEl.className = "status inactive";
      }
      
      const doorEl = document.getElementById('door-status');
      if(data.door_open) {
        doorEl.innerText = "PORTA ABERTA";
        doorEl.className = "status active";
      } else {
        doorEl.innerText = "FECHADA/SEGURA";
        doorEl.className = "status inactive";
      }
    });
}

function updateGPS() {
  fetch('/gps')
    .then(response => response.json())
    .then(data => {
      document.getElementById('lat').innerText = data.lat.toFixed(5);
      document.getElementById('lng').innerText = data.lng.toFixed(5);
    });
}

function togglePanic() {
  fetch('/panic/toggle', { method: 'POST' })
    .then(() => updateStatus());
}

setInterval(updateStatus, 2000);
setInterval(updateGPS, 3000);
updateStatus();
updateGPS();
</script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", index_html);
}

void handleGetStatus() {
  StaticJsonDocument<200> doc;

  doc["panic"] = panicActive;
  doc["door_open"] = doorOpen;
  doc["system_led"] = digitalRead(PIN_SYSTEM_LED) == HIGH;
  doc["last_sync"] = lastUpdate;

  String json;
  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

void handleGetGPS() {
  StaticJsonDocument<200> doc;

  doc["lat"] = latitude;
  doc["lng"] = longitude;
  doc["satellites"] = 8;

  String json;
  serializeJson(doc, json);

  server.send(200, "application/json", json);
}

void handleTogglePanic() {
  panicActive = !panicActive;

  digitalWrite(
    PIN_PANIC_LED,
    panicActive ? HIGH : LOW
  );

  StaticJsonDocument<100> doc;

  doc["status"] = "success";
  doc["new_panic_state"] = panicActive;

  String json;
  serializeJson(doc, json);

  server.send(200, "application/json", json);

  updateLCD();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("--- AstroTrack IoT System Starting ---");

  pinMode(PIN_PANIC_BTN, INPUT_PULLUP);
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);

  pinMode(PIN_PANIC_LED, OUTPUT);
  pinMode(PIN_SYSTEM_LED, OUTPUT);

  Serial.println("GPIOs Initialized");

  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("AstroTrack Init");

  Serial.println("LCD Initialized");
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);

    Serial.print(".");

    digitalWrite(
      PIN_SYSTEM_LED,
      !digitalRead(PIN_SYSTEM_LED)
    );
  }

  digitalWrite(PIN_SYSTEM_LED, HIGH);

  Serial.println();
  Serial.println("WiFi Connected");
  Serial.println(WiFi.localIP());

  // Endpoints REST
  server.on("/", HTTP_GET, handleRoot);
  server.on("/status", HTTP_GET, handleGetStatus);
  server.on("/gps", HTTP_GET, handleGetGPS);
  server.on("/panic/toggle", HTTP_POST, handleTogglePanic);

  server.begin();

  Serial.println("HTTP Server Started");

  updateLCD();
}
void loop() {

  server.handleClient();

  // Botão de pânico
  if (digitalRead(PIN_PANIC_BTN) == LOW) {

    delay(50);

    if (digitalRead(PIN_PANIC_BTN) == LOW) {

      panicActive = !panicActive;

      digitalWrite(
        PIN_PANIC_LED,
        panicActive ? HIGH : LOW
      );

      updateLCD();

      while (digitalRead(PIN_PANIC_BTN) == LOW) {
        delay(10);
      }
    }
  }

  // Sensor de porta
  bool currentDoorState =
    (digitalRead(PIN_DOOR_SENSOR) == LOW);

  if (currentDoorState != doorOpen) {

    doorOpen = currentDoorState;

    updateLCD();

    Serial.print("Porta: ");

    if (doorOpen) {
      Serial.println("Aberta");
    } else {
      Serial.println("Fechada");
    }
  }

  // GPS simulado
  latitude += (random(-10, 10) / 100000.0);
  longitude += (random(-10, 10) / 100000.0);

  delay(100);
}
