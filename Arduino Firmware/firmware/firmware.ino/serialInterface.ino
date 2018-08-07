char serial_command_buffer_[128];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r", " ");

void cmd_unrecognised(SerialCommands* sender, const char* cmd) {
  sender->GetSerial()->println("Command not found - Try \"help\"");
}

void cmd_get_ip(SerialCommands* sender) {
  sender->GetSerial()->println(WiFi.localIP());
}
SerialCommand cmd_get_ip_("ip?", cmd_get_ip);

void cmd_get_mqtt(SerialCommands* sender) {
  sender->GetSerial()->print("MQTT Server: "); 
  sender->GetSerial()->println(String(mqttServer));
  sender->GetSerial()->print("MQTT Port: ");
  sender->GetSerial()->println(mqttPort);
  sender->GetSerial()->print("MQTT Topic Prefix: ");
  sender->GetSerial()->println(mqttTopicPrefix);
  sender->GetSerial()->print("MQTT Device Name: ");
  sender->GetSerial()->println(mqttDeviceName);
}
SerialCommand cmd_get_mqtt_("mqtt?", cmd_get_mqtt);

void cmd_restart(SerialCommands* sender) {
  sender->GetSerial()->println("Restarting...");
  ESP.restart();
  delay(1000);
}
SerialCommand cmd_restart_("restart", cmd_restart);

void cmd_reset_all(SerialCommands* sender) {
  sender->GetSerial()->println("Removing WiFi credentials and restarting device...");
  WiFi.disconnect();
  while (WiFi.status() == WL_CONNECTED) {
    sender->GetSerial()->println("Waiting for WiFi to disconnect...");
    delay(500);
  }
  sender->GetSerial()->println("Restarting...");
  ESP.restart();
  delay(1000);
}
SerialCommand cmd_reset_all_("reset-all", cmd_reset_all);

void cmd_connect_wifi(SerialCommands* sender) {
  char* ssid = sender->Next();
  char* key = sender->Next();
  sender->GetSerial()->printf("Connecting to %s\n", ssid);
  WiFi.begin(ssid, key);

  waitForWiFi();
}
SerialCommand cmd_connect_wifi_("connect-wifi", cmd_connect_wifi);

void cmd_set_mqtt_broker(SerialCommands* sender) {
  char* ip = sender->Next();
  char* port_str = sender->Next();
  
  strncpy(mqttServer, ip, EEPROM_MQTT_SERVER_LEN);
  mqttPort = atoi(port_str);

  sender->GetSerial()->println("Ok");
}
SerialCommand cmd_set_mqtt_broker_("set-mqtt-broker", cmd_set_mqtt_broker);

void cmd_set_mqtt_topic_prefix(SerialCommands* sender) {
  char* prefix = sender->Next();
  
  strncpy(mqttTopicPrefix, prefix, EEPROM_MQTT_TOPIC_PREFIX_LEN);

  sender->GetSerial()->println("Ok");
}
SerialCommand cmd_set_mqtt_topic_prefix_("set-mqtt-topic-prefix", cmd_set_mqtt_topic_prefix);

void cmd_set_mqtt_device_name(SerialCommands* sender) {
  char* name = sender->Next();
  
  strncpy(mqttDeviceName, name, EEPROM_MQTT_DEVICE_NAME_LEN);

  sender->GetSerial()->println("Ok");
}
SerialCommand cmd_set_mqtt_device_name_("set-mqtt-device-name", cmd_set_mqtt_device_name);

void cmd_commit(SerialCommands* sender) {
  saveSettings();

  sender->GetSerial()->println("Ok");
}
SerialCommand cmd_commit_("commit", cmd_commit);

void cmd_help(SerialCommands* sender) {
  sender->GetSerial()->println("Available Commands:");
  sender->GetSerial()->println("  ip?                            - Print current IP address");
  sender->GetSerial()->println("  mqtt?                          - Print MQTT broker connection settings");
  sender->GetSerial()->println("  restart                        - Reboot device");
  sender->GetSerial()->println("  reset-all                      - Remove WiFi credentials and reboot");
  sender->GetSerial()->println("  connect-wifi [SSID] [KEY]      - Set Wifi credentials");
  sender->GetSerial()->println("  set-mqtt-broker [IP] [PORT]    - Set MQTT broker connection settings");
  sender->GetSerial()->println("  set-mqtt-topic-prefix [PREFIX] - Set MQTT topic prefix");
  sender->GetSerial()->println("  set-mqtt-device-name [NAME]    - Set MQTT device name");
  sender->GetSerial()->println("  commit                         - Write all config changes to flash");
}
SerialCommand cmd_help_("help", cmd_help);

void serialSetup() {
  Serial.println("Registering serial command handlers");
  serial_commands_.SetDefaultHandler(cmd_unrecognised);
  serial_commands_.AddCommand(&cmd_get_ip_);
  serial_commands_.AddCommand(&cmd_help_);
  serial_commands_.AddCommand(&cmd_restart_);
  serial_commands_.AddCommand(&cmd_reset_all_);
  serial_commands_.AddCommand(&cmd_connect_wifi_);
  serial_commands_.AddCommand(&cmd_get_mqtt_);
  serial_commands_.AddCommand(&cmd_set_mqtt_broker_);
  serial_commands_.AddCommand(&cmd_set_mqtt_topic_prefix_);
  serial_commands_.AddCommand(&cmd_set_mqtt_device_name_);
  serial_commands_.AddCommand(&cmd_commit_);
}

void serialLoop() {
  serial_commands_.ReadSerial();
}

