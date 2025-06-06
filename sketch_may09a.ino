#include <WiFi.h>
#include <WebServer.h>
#include "DHT.h"

// Sensor and pin setup
#define DHTTYPE DHT11
const uint8_t DHTPin = 4;
const uint8_t MoisturePin = 34;
const uint8_t RelayPin = 26;

// Moisture threshold in %
const int moistureThreshold = 40;

// WiFi credentials
const char* ssid = "Redmi";
const char* password = "sakshamsavant123";

// Web server
WebServer server(80);
DHT dht(DHTPin, DHTTYPE);

// Globals
float Temperature = 0.0;
float Humidity = 0.0;
int soilMoisturePercent = 0;
unsigned long lastRead = 0;
const unsigned long readInterval = 1000;

void setup() {
  Serial.begin(115200);
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, HIGH); // Pump initially OFF

  dht.begin();

  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", handle_OnConnect);
  server.on("/readings", handle_UpdateReadings);
  server.onNotFound(handle_NotFound);
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  unsigned long currentMillis = millis();
  if (currentMillis - lastRead >= readInterval) {
    lastRead = currentMillis;

    Temperature = dht.readTemperature();
    Humidity = dht.readHumidity();

    int rawMoisture = analogRead(MoisturePin);
    soilMoisturePercent = map(rawMoisture, 4095, 1000, 0, 100);
    soilMoisturePercent = constrain(soilMoisturePercent, 0, 100);

    if (soilMoisturePercent < moistureThreshold) {
      digitalWrite(RelayPin, HIGH); // Pump ON
    } else {
      digitalWrite(RelayPin, LOW);  // Pump OFF
    }

    Serial.printf("Temp: %.1f °C | Hum: %.1f %% | Moisture: %d %%\n", Temperature, Humidity, soilMoisturePercent);
  }
}

void handle_OnConnect() {
  server.send(200, "text/html", SendHTML());
}

void handle_UpdateReadings() {
  if (isnan(Temperature) || isnan(Humidity)) {
    server.send(200, "application/json", "{\"error\":\"Sensor read failed\"}");
    return;
  }

  String data = "{";
  data += "\"temperature\":" + String(Temperature, 1) + ",";
  data += "\"humidity\":" + String(Humidity, 1) + ",";
  data += "\"moisture\":" + String(soilMoisturePercent);
  data += "}";
  server.send(200, "application/json", data);
}

void handle_NotFound() {
  server.send(404, "text/plain", "Not found");
}

String SendHTML() {
  String ptr = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  ptr += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  ptr += "<title>ESP32 Smart Farming</title>";
  ptr += "<script src='https://cdn.jsdelivr.net/npm/gaugeJS/dist/gauge.min.js'></script>";
  ptr += "<style>";
  ptr += "body { font-family: 'Segoe UI', sans-serif; background: #f4f6f8; text-align: center; }";
  ptr += "h1 { background: #4CAF50; color: white; padding: 20px; border-radius: 0 0 15px 15px; }";
  ptr += ".gauge-container { display: flex; flex-wrap: wrap; justify-content: center; gap: 40px; margin: 30px; }";
  ptr += ".gauge-box { background: white; border-radius: 15px; padding: 20px; box-shadow: 0 8px 16px rgba(0,0,0,0.2); width: 250px; }";
  ptr += "canvas { width: 200px; height: 200px; }";
  ptr += ".value { font-size: 24px; margin-top: 10px; color: #333; font-weight: bold; }";
  ptr += "</style></head><body>";

  ptr += "<h1>🌱 ESP32 Smart Farming Monitor</h1>";
  ptr += "<div class='gauge-container'>";

  ptr += "<div class='gauge-box'><h3>🌡️ Temperature</h3><canvas id='tempGauge'></canvas><div class='value' id='tempValue'></div></div>";
  ptr += "<div class='gauge-box'><h3>💧 Humidity</h3><canvas id='humGauge'></canvas><div class='value' id='humValue'></div></div>";
  ptr += "<div class='gauge-box'><h3>🌾 Soil Moisture</h3><canvas id='moistGauge'></canvas><div class='value' id='moistValue'></div></div>";

  ptr += "</div><script>";

  ptr += "var temp = new Gauge(document.getElementById('tempGauge')).setOptions({angle:0,lineWidth:0.3,pointer:{length:0.6,strokeWidth:0.035,color:'#000'},colorStart:'#6FADCF',colorStop:'#8FC0DA',strokeColor:'#E0E0E0',highDpiSupport:true});";
  ptr += "temp.maxValue = 50; temp.setMinValue(0); temp.animationSpeed = 32; temp.set(0); temp.setTextField(document.getElementById('tempValue'));";

  ptr += "var hum = new Gauge(document.getElementById('humGauge')).setOptions({angle:0,lineWidth:0.3,pointer:{length:0.6,strokeWidth:0.035,color:'#000'},colorStart:'#FFD54F',colorStop:'#FFCA28',strokeColor:'#E0E0E0',highDpiSupport:true});";
  ptr += "hum.maxValue = 100; hum.setMinValue(0); hum.animationSpeed = 32; hum.set(0); hum.setTextField(document.getElementById('humValue'));";

  ptr += "var moist = new Gauge(document.getElementById('moistGauge')).setOptions({angle:0,lineWidth:0.3,pointer:{length:0.6,strokeWidth:0.035,color:'#000'},colorStart:'#A5D6A7',colorStop:'#81C784',strokeColor:'#E0E0E0',highDpiSupport:true});";
  ptr += "moist.maxValue = 100; moist.setMinValue(0); moist.animationSpeed = 32; moist.set(0); moist.setTextField(document.getElementById('moistValue'));";

  ptr += "function updateReadings() {";
  ptr += "fetch('/readings').then(res => res.json()).then(data => {";
  ptr += "if (!data.error) {";
  ptr += "temp.set(data.temperature); document.getElementById('tempValue').innerHTML = data.temperature + ' °C';";
  ptr += "hum.set(data.humidity); document.getElementById('humValue').innerHTML = data.humidity + ' %';";
  ptr += "moist.set(data.moisture); document.getElementById('moistValue').innerHTML = data.moisture + ' %';";
  ptr += "}}).catch(err => console.error('Fetch error:', err));";
  ptr += "}";
  ptr += "updateReadings(); setInterval(updateReadings, 1000);";

  ptr += "</script></body></html>";
  return ptr;
}
