#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "SerialCommands.h"
#include "DHT.h"

#define CH_PIN 12
#define WIFI_LED 13
// When changing the sensor LED pin to the correct
// GPIO one, also change the digitalWrite and digitalRead
// calls to reverse HIGH/LOW (Pin 13 needs pulled low for
// the LED to light!)
//#define SENSOR_LED 4
#define SENSOR_LED 13
#define DHT_PIN 14
#define DHT_TYPE DHT21
#define DEFAULT_CH_TIMEOUT_SECONDS 30;

ESP8266WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long chOnTime;
unsigned long ledToggledTime;
int ledNextToggleDelay = 0;
bool chIsOn = false;
bool chSetOn = false;
bool defaultSensorLedStatus = HIGH;
int chTimeoutSeconds = DEFAULT_CH_TIMEOUT_SECONDS;

bool waitForWiFi() {
  int connectAttemptCount = 0;
  while (WiFi.status() != WL_CONNECTED && connectAttemptCount < 30) {
    digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));
    Serial.println("Waiting for WiFi to connect...");
    delay(500);
    connectAttemptCount++;
  }

  if (connectAttemptCount < 30) {
    Serial.println("Connected!");
    return true;
  } else {
    Serial.println("Connection failed!");
    return false;
  }
}

void handleRoot() {
  digitalWrite(WIFI_LED, HIGH);
  server.send(200, "text/plain", "hello from esp8266!");
  delay(100);
  digitalWrite(WIFI_LED, LOW);
}

void handleReadSensors() {
  digitalWrite(WIFI_LED, HIGH);

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

  char temperatureStr[7];
  char humidityStr[7];
  char heatIndexStr[7];
  
  dtostrf(temperature, 6, 2, temperatureStr);
  dtostrf(humidity, 6, 2, humidityStr);
  dtostrf(heatIndex, 6, 2, heatIndexStr);

  String response = "{";
  response += "\n  temperature: ";
  response += temperatureStr;
  response += ",\n  humidity: ";
  response += humidityStr;
  response += ",\n  heatIndex: ";
  response += heatIndexStr;
  response += "\n}\n";
  
  server.send(200, "application", response);
  digitalWrite(WIFI_LED, LOW);
}

void handleChOn() {
  digitalWrite(WIFI_LED, HIGH);
  digitalWrite(CH_PIN, HIGH);
  chIsOn = true;
  chSetOn = true;
  chOnTime = millis();

  chTimeoutSeconds = DEFAULT_CH_TIMEOUT_SECONDS;
  for (int i = 0; i < server.args(); i++) {
    if (strcmp(server.argName(i).c_str(), "timeout") == 0) {
      chTimeoutSeconds = atoi(server.arg(i).c_str());
    }
  } 

  server.send(200, "text/plain", "ok");
  digitalWrite(WIFI_LED, LOW);
}

void handleChOff() {
  digitalWrite(WIFI_LED, HIGH);
  digitalWrite(CH_PIN, LOW);
  chIsOn = false;
  chSetOn = false;

  server.send(200, "text/plain", "ok");
  digitalWrite(WIFI_LED, LOW);
}

void handleSensorLedOn() {
  digitalWrite(WIFI_LED, HIGH);
  defaultSensorLedStatus = LOW;

  server.send(200, "text/plain", "ok");
  digitalWrite(WIFI_LED, LOW);
}

void handleSensorLedOff() {
  digitalWrite(WIFI_LED, HIGH);
  defaultSensorLedStatus = HIGH;

  server.send(200, "text/plain", "ok");
  digitalWrite(WIFI_LED, LOW);
}

void handleNotFound(){
  digitalWrite(WIFI_LED, HIGH);
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  delay(100);
  digitalWrite(WIFI_LED, LOW);
}

void setup(void){;
  pinMode(WIFI_LED, OUTPUT);
  pinMode(CH_PIN, OUTPUT);

  Serial.begin(115200);

  WiFi.begin();
  WiFi.persistent(true);

  waitForWiFi();

  Serial.println("Starting HTTP Server");
  server.on("/", handleRoot);
  server.on("/read_sensors", handleReadSensors);
  server.on("/chon", handleChOn);
  server.on("/choff", handleChOff);
  server.on("/sensorLedOn", handleSensorLedOn);
  server.on("/sensorLedOff", handleSensorLedOff);
  server.onNotFound(handleNotFound);
  server.begin();

  dht.begin();

  serialSetup();
}

void loop(void){
//  if (WiFi.status() == WL_CONNECTED) {
//    digitalWrite(WIFI_LED, LOW);
//  } else {
//    digitalWrite(WIFI_LED, HIGH);
//  }

  if (chIsOn && (millis() - chOnTime) > (chTimeoutSeconds * 1000)) {
    digitalWrite(CH_PIN, LOW);
    chIsOn = false;
  }

  if (chIsOn && chSetOn) {
    if ((millis() - ledToggledTime) > ledNextToggleDelay) {
      digitalWrite(SENSOR_LED, !digitalRead(SENSOR_LED));
      ledToggledTime = millis();
      if (!digitalRead(SENSOR_LED)) {
        ledNextToggleDelay = 1000;
      } else {
        ledNextToggleDelay = 100;
      }
    }
  } else if (!chIsOn && chSetOn) {
    if ((millis() - ledToggledTime) > 250) {
      digitalWrite(SENSOR_LED, !digitalRead(SENSOR_LED));
      ledToggledTime = millis();
    }
  } else {
    digitalWrite(SENSOR_LED, defaultSensorLedStatus);
  }
  
  server.handleClient();
  serialLoop();
}
