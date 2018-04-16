#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "SerialCommands.h"
#include "DHT.h"

#define CH_PIN 12
#define WIFI_LED 13
//#define SENSOR_LED 4
#define SENSOR_LED 13
#define DHT_PIN 14
#define DHT_TYPE DHT21

ESP8266WebServer server(80);
DHT dht(DHT_PIN, DHT_TYPE);

unsigned long chOnTime;
unsigned long ledToggledTime;
bool chIsOn = false;
bool chSetOn = false;

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

void handleTemp() {
  digitalWrite(WIFI_LED, HIGH);

  float temperature = dht.readTemperature();
  char buf[6];

  dtostrf(temperature, 6, 2, buf);
  
  server.send(200, "text/plain", buf);
  digitalWrite(WIFI_LED, LOW);
}

void handleHumidity() {
  digitalWrite(WIFI_LED, HIGH);

  float humidity = dht.readHumidity();
  char buf[6];

  dtostrf(humidity, 6, 2, buf);
  
  server.send(200, "text/plain", buf);
  digitalWrite(WIFI_LED, LOW);
}

void handleHeatIndex() {
  digitalWrite(WIFI_LED, HIGH);

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);
  char buf[6];

  dtostrf(heatIndex, 6, 2, buf);
  
  server.send(200, "text/plain", buf);
  digitalWrite(WIFI_LED, LOW);
}

void handleChOn() {
  digitalWrite(WIFI_LED, HIGH);
  digitalWrite(CH_PIN, HIGH);
  chIsOn = true;
  chSetOn = true;
  chOnTime = millis();

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
  server.on("/temperature", handleTemp);
  server.on("/humidity", handleHumidity);
  server.on("/heat_index", handleHeatIndex);
  server.on("/chon", handleChOn);
  server.on("/choff", handleChOff);
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

  if (chIsOn && (millis() - chOnTime) > 10000) {
    digitalWrite(CH_PIN, LOW);
    chIsOn = false;
  }

  if (chIsOn && chSetOn) {
    digitalWrite(SENSOR_LED, LOW);
  } else if (!chIsOn && chSetOn) {
    if ((millis() - ledToggledTime) > 250) {
      digitalWrite(SENSOR_LED, !digitalRead(SENSOR_LED));
      ledToggledTime = millis();
    }
  } else {
    digitalWrite(SENSOR_LED, HIGH);
  }
  
  server.handleClient();
  serialLoop();
}
