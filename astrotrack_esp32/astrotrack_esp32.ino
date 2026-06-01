#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <LiquidCrystal_I2C.h>

// --- Configurações de Hardware ---
const int PIN_PANIC_BTN = 12;
const int PIN_DOOR_SENSOR = 13;
const int PIN_PANIC_LED = 2;
const int PIN_SYSTEM_LED = 4;

// --- Instâncias ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
WebServer server(80);

// --- Variáveis de Estado ---
bool panicActive = false;
bool doorOpen = false;
float latitude = -23.5505;
float longitude = -46.6333;
String lastUpdate = "Never";

// --- WiFi ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ======================================================
// FUNÇÃO LCD (DECLARADA ANTES DE SER UTILIZADA)
// ======================================================

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

// ======================================================
// HANDLERS WEB
// ======================================================

void handleRoot() {
  server.send(200, "text/plain", "AstroTrack IoT Gateway Operational");
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

// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(115200);
  delay(1000);

  Serial.println("--- AstroTrack IoT System Starting ---");

  pinMode(PIN_PANIC_BTN, INPUT_PULLUP);
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);

  pinMode(PIN_PANIC_LED, OUTPUT);
  pinMode(PIN_SYSTEM_LED, OUTPUT);

  Serial.println("GPIOs Initialized");

  // LCD
  Wire.begin(21, 22);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("AstroTrack Init");

  Serial.println("LCD Initialized");

  // WiFi
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

// ======================================================
// LOOP
// ======================================================

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