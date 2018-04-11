#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "SerialCommands.h"

#define WIFI_LED 13

ESP8266WebServer server(80);

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

  Serial.begin(115200);

  WiFi.begin();
  WiFi.persistent(true);

  waitForWiFi();

  Serial.println("Starting HTTP Server");
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  serialSetup();
}

void loop(void){
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFI_LED, LOW);
  } else {
    digitalWrite(WIFI_LED, HIGH);
  }
  
  server.handleClient();
  serialLoop();
}
