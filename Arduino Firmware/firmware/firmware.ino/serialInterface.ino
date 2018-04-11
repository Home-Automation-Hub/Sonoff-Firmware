char serial_command_buffer_[32];
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

void cmd_remove_credentials(SerialCommands* sender) {
  sender->GetSerial()->println("Removing WiFi credentials...");
  WiFi.disconnect();
  delay(1000);
  sender->GetSerial()->println("Power cycle device now!");
}
SerialCommand cmd_remove_credentials_("remove-credentials", cmd_remove_credentials);

void cmd_help(SerialCommands* sender) {
  sender->GetSerial()->println("Available Commands:");
  sender->GetSerial()->println("  ip? - Print current IP address");
  sender->GetSerial()->println("  restart - Reboot device");
  sender->GetSerial()->println("  remove-credentials - Remove WiFi credentials");
  sender->GetSerial()->println("");
}
SerialCommand cmd_help_("help", cmd_help);

void serialSetup() {
  Serial.println("Registering serial command handlers");
  serial_commands_.SetDefaultHandler(cmd_unrecognised);
  serial_commands_.AddCommand(&cmd_get_ip_);
  serial_commands_.AddCommand(&cmd_help_);
  serial_commands_.AddCommand(&cmd_restart_);
  serial_commands_.AddCommand(&cmd_remove_credentials_);
}

void serialLoop() {
  serial_commands_.ReadSerial();
}

