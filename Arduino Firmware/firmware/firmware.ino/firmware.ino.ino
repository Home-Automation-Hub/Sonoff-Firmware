#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include "SerialCommands.h"
#include <WiFiManager.h>

ESP8266WebServer server(80);

const int led = 13;

void handleRoot() {
  digitalWrite(led, 1);
  server.send(200, "text/plain", "hello from esp8266!");
  digitalWrite(led, 0);
}

void handleNotFound(){
  digitalWrite(led, 1);
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
  digitalWrite(led, 0);
}

void setup(void){;
  Serial.begin(115200);

  Serial.println("Starting WiFi Manager");
  WiFiManager wifiManager;
  wifiManager.autoConnect("SETUP");
  WiFi.persistent(true);

  Serial.println("Starting HTTP Server");
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();

  serialSetup();
}

void loop(void){
  server.handleClient();
  serialLoop();
}
