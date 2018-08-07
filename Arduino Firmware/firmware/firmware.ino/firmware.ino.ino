#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include "SerialCommands.h"
#include "DHT.h"
#include <PubSubClient.h>
#include <PubSubClientTools.h>
#include <EEPROM.h>

#define BUTTON_PIN 0
#define CH_PIN 12
#define WIFI_LED 13
#define SENSOR_LED 4
#define DHT_PIN 14
#define DHT_TYPE DHT21
#define DEFAULT_CH_TIMEOUT_SECONDS 30;

#define MQTT_SERVER "10.114.1.136"
#define MQTT_PORT 1883
#define MQTT_TOPIC_PREFIX "flat/heating/hallway/"
#define DEVICE_NAME "hallway"

// Define addresses and lengths of variables to store in EEPROM
#define EEPROM_MQTT_SERVER_ADDR 0
#define EEPROM_MQTT_SERVER_LEN 16
#define EEPROM_MQTT_SERVER_PORT_ADDR 20 // Integer so 4 bytes
#define EEPROM_MQTT_TOPIC_PREFIX_ADDR 25
#define EEPROM_MQTT_TOPIC_PREFIX_LEN 70
#define EEPROM_MQTT_DEVICE_NAME_ADDR 100
#define EEPROM_MQTT_DEVICE_NAME_LEN 25

// Variables to hold settings that will be retrieved from EEPROM
char mqttServer[EEPROM_MQTT_SERVER_LEN] = "";
int mqttPort = 0;
char mqttTopicPrefix[EEPROM_MQTT_TOPIC_PREFIX_LEN] = "";
char mqttDeviceName[EEPROM_MQTT_DEVICE_NAME_LEN] = "";

DHT dht(DHT_PIN, DHT_TYPE);

unsigned long chOnTime;
unsigned long ledToggledTime;
unsigned long sensorPublishedTime;
int ledNextToggleDelay = 0;
bool chIsOn = false;
bool chSetOn = false;
bool defaultSensorLedStatus = LOW;
int chTimeoutSeconds = DEFAULT_CH_TIMEOUT_SECONDS;
bool setupMode = false;

WiFiClient wifiClient;
PubSubClient mqttClient(MQTT_SERVER, MQTT_PORT, wifiClient);
PubSubClientTools mqtt(mqttClient);

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

void loadSettings() {
  EEPROM.get(EEPROM_MQTT_SERVER_ADDR, mqttServer);
  EEPROM.get(EEPROM_MQTT_SERVER_PORT_ADDR, mqttPort);
  EEPROM.get(EEPROM_MQTT_TOPIC_PREFIX_ADDR, mqttTopicPrefix);
  EEPROM.get(EEPROM_MQTT_DEVICE_NAME_ADDR, mqttDeviceName);
}

void saveSettings() {
  EEPROM.put(EEPROM_MQTT_SERVER_ADDR, mqttServer);
  EEPROM.put(EEPROM_MQTT_SERVER_PORT_ADDR, mqttPort);
  EEPROM.put(EEPROM_MQTT_TOPIC_PREFIX_ADDR, mqttTopicPrefix);
  EEPROM.put(EEPROM_MQTT_DEVICE_NAME_ADDR, mqttDeviceName);
  EEPROM.commit();
}

void publishSensors() {
  digitalWrite(WIFI_LED, HIGH);

  Serial.println("Reading");
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

  char temperatureStr[7];
  char humidityStr[7];
  char heatIndexStr[7];

  dtostrf(temperature, 6, 2, temperatureStr);
  dtostrf(humidity, 6, 2, humidityStr);
  dtostrf(heatIndex, 6, 2, heatIndexStr);

  mqtt.publish("temperature", temperatureStr);
  mqtt.publish("humidity", humidityStr);
  mqtt.publish("heat_index", heatIndexStr);

  digitalWrite(WIFI_LED, LOW);
}

void subscriber_sensorLed(String topic, String message) {
  digitalWrite(WIFI_LED, HIGH);

  if (message.equals("on")) {
    defaultSensorLedStatus = HIGH;
  } else if (message.equals("off")) {
    defaultSensorLedStatus = LOW;
  }
}

void subscriber_chState(String topic, String message) {
  digitalWrite(WIFI_LED, HIGH);

  String command = message;
  chTimeoutSeconds = DEFAULT_CH_TIMEOUT_SECONDS;
  int separatorIndex = message.indexOf(",");
  if (separatorIndex != -1) {
    command = message.substring(0, separatorIndex);
    chTimeoutSeconds = message.substring(separatorIndex + 1).toInt();
  }

  if (command.equals("on")) {
    digitalWrite(CH_PIN, HIGH);
    chIsOn = true;
    chSetOn = true;
    chOnTime = millis();
  } else if (command.equals("off")) {
    digitalWrite(CH_PIN, LOW);
    chIsOn = false;
    chSetOn = false;
  }


  digitalWrite(WIFI_LED, LOW);
}

void setup(void) {
  pinMode(WIFI_LED, OUTPUT);
  pinMode(CH_PIN, OUTPUT);
  pinMode(SENSOR_LED, OUTPUT);
  pinMode(DHT_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT);

  digitalWrite(WIFI_LED, LOW);

  Serial.begin(115200);
  EEPROM.begin(4096);
  
  for(size_t i = 0; i < 1000; i++)
  {
    if (digitalRead(BUTTON_PIN) == 0) {
      setupMode = true;
    }
    delay(10);
  }

  if (setupMode) {
    Serial.println("STARTING UP IN SETUP MODE!");
  }

  Serial.println("Reading configuration from EEPROM...");
  loadSettings();

  Serial.print("MQTT Server: ");
  Serial.println(String(mqttServer));
  Serial.print("MQTT Port: ");
  Serial.println(mqttPort);
  Serial.print("MQTT Topic Prefix: ");
  Serial.println(mqttTopicPrefix);
  Serial.print("MQTT Device Name: ");
  Serial.println(mqttDeviceName);

  Serial.println("Loaded!");

  WiFi.begin();
  WiFi.persistent(true);

  boolean wifiConnected = waitForWiFi();
  if (wifiConnected) {
    connectMqtt();
  }

  dht.begin();

  serialSetup();
}

void connectMqtt() {
  Serial.println("Connecting to MQTT Broker...");
  if (mqttClient.connect(DEVICE_NAME)) {
    Serial.println("Connected!");
    mqtt.setSubscribePrefix(MQTT_TOPIC_PREFIX);
    mqtt.setPublishPrefix(MQTT_TOPIC_PREFIX);
    mqtt.subscribe("sensorLed", subscriber_sensorLed);
    mqtt.subscribe("chState", subscriber_chState);
  } else {
    Serial.println("Connection failed!");
    Serial.println(mqttClient.state());
  }
}

void loop(void) {
  serialLoop();

  // Do not run any other logic if we are in setup mode, just allow the
  // serial console to be used
  if (setupMode) {
    return;
  }

  // Only continue with the loop if we are actually connected to the
  // MQTT broker, otherwise periodically retry connecting
  if (mqttClient.state() != 0) {
    Serial.println("Not connected to MQTT broker, retrying in 10 seconds...");
    
    for(size_t i = 0; i < 80; i++)
    {
      digitalWrite(WIFI_LED, !digitalRead(WIFI_LED));
      delay(125);
    }
    digitalWrite(WIFI_LED, HIGH);
    
    connectMqtt();
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(WIFI_LED, LOW);
  } else {
    digitalWrite(WIFI_LED, HIGH);
  }

  if (chIsOn && (millis() - chOnTime) > (chTimeoutSeconds * 1000)) {
    digitalWrite(CH_PIN, LOW);
    chIsOn = false;
  }

  if (chIsOn && chSetOn) {
    if ((millis() - ledToggledTime) > ledNextToggleDelay) {
      digitalWrite(SENSOR_LED, !digitalRead(SENSOR_LED));
      ledToggledTime = millis();
      if (digitalRead(SENSOR_LED)) {
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

  if ((millis() - sensorPublishedTime) > 10000) {
    publishSensors();
    sensorPublishedTime = millis();
  }

  mqttClient.loop();
}
