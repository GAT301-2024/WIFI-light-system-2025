#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>

// WiFi credentials
const char* ssid = "BARASA";
const char* password = "123456789";

// Pin definitions
#define DHTPIN 23       
#define FAN_PIN 19      
#define DHTTYPE DHT11   

// Temperature threshold for fan (in °C)
#define TEMP_THRESHOLD 20.0

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

// Variables to store sensor readings and fan state
float temperature = 0;
float humidity = 0;
bool fanState = false;
unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize DHT sensor
  dht.begin();
  
  // Initialize fan control pin
  pinMode(FAN_PIN, OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi. IP address: ");
  Serial.println(WiFi.localIP());
  
  // Route handlers
  server.on("/", handle_root);
  server.on("/refresh", handle_refresh);
  server.on("/fan_on", handle_fan_on);
  server.on("/fan_off", handle_fan_off);
  
  // Start server
  server.begin();
  Serial.println("HTTP server started");
}


void handle_root() {
  readSensorData();
  
  String html = "<!DOCTYPE html><html>";
  html += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>body { font-family: Arial; text-align: center; margin: 0 auto; padding: 20px; }";
  html += ".data { font-size: 24px; margin: 20px; }";
  html += ".button { background-color: #4CAF50; border: none; color: white; padding: 15px 32px;";
  html += "text-align: center; text-decoration: none; display: inline-block; font-size: 16px;";
  html += "margin: 10px 2px; cursor: pointer; border-radius: 8px; }";
  html += ".fan-on { background-color: #4CAF50; }";
  html += ".fan-off { background-color: #f44336; }";
  html += ".status { font-size: 18px; margin: 15px; padding: 10px; border-radius: 5px; }";
  html += ".fan-status { background-color: " + String(fanState ? "#e7f3fe" : "#f8f8f8") + "; }";
  html += "</style></head>";
  html += "<body><h1>ESP32 Temp & Humid Control</h1>";
  html += "<div class=\"data\">Temperature: " + String(temperature, 1) + "°C</div>";
  html += "<div class=\"data\">Humidity: " + String(humidity, 1) + "%</div>";
  
  // Fan control section
  html += "<div class=\"status fan-status\">Fan is currently: " + String(fanState ? "ON" : "OFF") + "</div>";
  html += "<div>";
  html += "<a href=\"/fan_on\"><button class=\"button fan-on\">Turn Fan ON</button></a>";
  html += "<a href=\"/fan_off\"><button class=\"button fan-off\">Turn Fan OFF</button></a>";
  html += "</div>";
  
  // Refresh button
  html += "<div style=\"margin-top: 30px;\">";
  html += "<a href=\"/refresh\"><button class=\"button\">Refresh Data</button></a>";
  html += "</div>";
  
  // Footer
  html += "<p>Last update: " + getTimeString() + "</p>";
  html += "<p>Fan turns on automatically at >" + String(TEMP_THRESHOLD, 1) + "°C</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handle_refresh() {
  handle_root();
}

void handle_fan_on() {
  setFan(true);
  handle_root();
}

void handle_fan_off() {
  setFan(false);
  handle_root();
}

void setFan(bool state) {
  fanState = state;
  digitalWrite(FAN_PIN, fanState ? HIGH : LOW);
  Serial.println("Fan set to: " + String(fanState ? "ON" : "OFF"));
}

void readSensorData() {
  float newTemp = dht.readTemperature();
  float newHum = dht.readHumidity();
  
  if (isnan(newTemp) || isnan(newHum)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  
  temperature = newTemp;
  humidity = newHum;
  lastUpdate = millis();
  
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Humidity: ");
  Serial.print(humidity);
  Serial.println("%");
}

String getTimeString() {
  unsigned long currentMillis = millis();
  unsigned long seconds = currentMillis / 1000;
  unsigned long minutes = seconds / 60;
  unsigned long hours = minutes / 60;
  
  seconds %= 60;
  minutes %= 60;
  hours %= 24;
  
  char timeString[12];
  sprintf(timeString, "%02d:%02d:%02d", hours, minutes, seconds);
  
  return String(timeString);
}

void loop() {
  long unsigned previousTime = 0;
  server.handleClient();
  if (millis() - previousTime > 3000){
    handle_root();
    previousTime = millis();
  }
  
  // Automatic fan control
  if (temperature > TEMP_THRESHOLD && !fanState) {
    setFan(true);
  } else if (temperature <= TEMP_THRESHOLD - 2.0 && fanState) { // Add hysteresis
    setFan(false);
  }
}