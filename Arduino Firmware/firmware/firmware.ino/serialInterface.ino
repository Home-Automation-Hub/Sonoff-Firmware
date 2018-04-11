char serial_command_buffer_[128];
SerialCommands serial_commands_(&Serial, serial_command_buffer_, sizeof(serial_command_buffer_), "\r", " ");

void cmd_unrecognised(SerialCommands* sender, const char* cmd) {
  sender->GetSerial()->println("Command not found - Try \"help\"");
}

void cmd_get_ip(SerialCommands* sender) {
  sender->GetSerial()->println(WiFi.localIP());
}
SerialCommand cmd_get_ip_("ip?", cmd_get_ip);

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

void cmd_help(SerialCommands* sender) {
  sender->GetSerial()->println("Available Commands:");
  sender->GetSerial()->println("  ip?                       - Print current IP address");
  sender->GetSerial()->println("  restart                   - Reboot device");
  sender->GetSerial()->println("  reset-all                 - Remove WiFi credentials and reboot");
  sender->GetSerial()->println("  connect-wifi [SSID] [KEY] - Set Wifi credentials");
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
}

void serialLoop() {
  serial_commands_.ReadSerial();
}

