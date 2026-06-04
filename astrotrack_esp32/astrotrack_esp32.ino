#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LiquidCrystal_I2C.h>

const int PIN_PANIC_BTN = 12;
const int PIN_DOOR_SENSOR = 13;
const int PIN_PANIC_LED = 2;
const int PIN_SYSTEM_LED = 4;

LiquidCrystal_I2C lcd(0x27, 16, 2);

bool panicActive = false;
bool doorOpen = false;
float latitude = -23.5505;
float longitude = -46.6333;
unsigned long lastSendTime = 0;
const unsigned long sendInterval = 5000;

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* serverUrl = "http://192.168.10.4:3000/telemetry"; 
const String deviceId = "ASTRO_001";

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

void sendDataToApi() {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    String json = "{";
    json += "\"deviceId\":\"" + deviceId + "\",";
    json += "\"panic\":" + String(panicActive ? "true" : "false") + ",";
    json += "\"door_open\":" + String(doorOpen ? "true" : "false") + ",";
    json += "\"latitude\":" + String(latitude, 6) + ",";
    json += "\"longitude\":" + String(longitude, 6) + ",";
    json += "\"timestamp\":" + String(millis());
    json += "}";

    int httpResponseCode = http.POST(json); 

    if (httpResponseCode > 0) {
      Serial.print("Dados enviados! Codigo: ");
      Serial.println(httpResponseCode);
    } else {
      Serial.print("Erro no envio: ");
      Serial.println(http.errorToString(httpResponseCode).c_str());
    }
    http.end();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000); 
  Serial.println("--- INICIANDO SIMULACAO ASTROTRACK ---");

  pinMode(PIN_PANIC_BTN, INPUT_PULLUP);
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);
  pinMode(PIN_PANIC_LED, OUTPUT);
  pinMode(PIN_SYSTEM_LED, OUTPUT);
  
  Serial.println("GPIOs configurados.");

  Serial.println("Tentando inicializar LCD...");
  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();
  lcd.print("AstroTrack Init");
  Serial.println("LCD Inicializado.");

  Serial.print("Conectando ao WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(PIN_SYSTEM_LED, !digitalRead(PIN_SYSTEM_LED));
  }
  digitalWrite(PIN_SYSTEM_LED, HIGH);
  Serial.println("\nWiFi Conectado!");

  updateLCD();
}

void loop() {
  if (digitalRead(PIN_PANIC_BTN) == LOW) {
    delay(50);
    if (digitalRead(PIN_PANIC_BTN) == LOW) {
      panicActive = !panicActive;
      digitalWrite(PIN_PANIC_LED, panicActive ? HIGH : LOW);
      updateLCD();
      sendDataToApi();
      while (digitalRead(PIN_PANIC_BTN) == LOW) delay(10);
    }
  }

  bool currentDoorState = (digitalRead(PIN_DOOR_SENSOR) == LOW);
  if (currentDoorState != doorOpen) {
    doorOpen = currentDoorState;
    updateLCD();
    sendDataToApi();
  }

  latitude += (random(-10, 10) / 100000.0);
  longitude += (random(-10, 10) / 100000.0);

  if (millis() - lastSendTime > sendInterval) {
    sendDataToApi();
    lastSendTime = millis();
  }

  delay(100);
}
