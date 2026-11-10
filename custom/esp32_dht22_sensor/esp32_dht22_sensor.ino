// ESP32 + DHT22 chamber sensor for the BambuStudio ChamberSensor module.
// Serves:  GET /sensor  ->  {"temperature": 24.5, "humidity": 41.2, "uptime_ms": 12345}
//
// Libraries (Arduino Library Manager): "DHT sensor library" by Adafruit,
// "Adafruit Unified Sensor".
// Wiring: DHT22 VCC -> 3V3, GND -> GND, DATA -> GPIO 4 (10k pull-up to 3V3
// if your board is a bare sensor, not a breakout module).

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <DHT.h>

const char *WIFI_SSID = "YOUR_WIFI_SSID";
const char *WIFI_PASS = "YOUR_WIFI_PASSWORD";

#define DHT_PIN  4
#define DHT_TYPE DHT22

DHT       dht(DHT_PIN, DHT_TYPE);
WebServer server(80);

float         lastTemp = NAN, lastHum = NAN;
unsigned long lastRead = 0;
const unsigned long READ_INTERVAL_MS = 2500; // DHT22 max rate is ~0.5 Hz

void readSensor() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  if (!isnan(h) && !isnan(t)) { lastHum = h; lastTemp = t; }
  lastRead = millis();
}

void handleSensor() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store");
  if (isnan(lastTemp) || isnan(lastHum)) {
    server.send(503, "application/json", "{\"error\":\"sensor not ready\"}");
    return;
  }
  char buf[128];
  snprintf(buf, sizeof(buf), "{\"temperature\":%.1f,\"humidity\":%.1f,\"uptime_ms\":%lu}",
           lastTemp, lastHum, millis());
  server.send(200, "application/json", buf);
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
  Serial.printf("\nIP: %s  ->  http://%s/sensor\n", WiFi.localIP().toString().c_str(),
                WiFi.localIP().toString().c_str());
  if (MDNS.begin("chamber-sensor")) Serial.println("Also: http://chamber-sensor.local/sensor");

  server.on("/sensor", HTTP_GET, handleSensor);
  server.onNotFound([]() { server.send(404, "text/plain", "use GET /sensor"); });
  server.begin();
  readSensor();
}

void loop() {
  server.handleClient();
  if (millis() - lastRead >= READ_INTERVAL_MS) readSensor();
  if (WiFi.status() != WL_CONNECTED) WiFi.reconnect();
}
