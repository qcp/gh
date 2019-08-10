String defaultSettings = "{  \"settings\": {\"name\": \"door\", \"wifi\": {\"ssid\": \"Home\", \"pass\": \"1qazsedcft6\"}, \"temp\": {\"open_bound\": 30, \"close_bound\": 20 }, \"server\": \"greenhouse--qcp.repl.co\"} }";

void loadConfig(){
  if (!SPIFFS.begin()) {
    log(CONFIG_LOAD_SOURCE, WARNING_KIND, "Failed to mount file system, load default config");
    deserializeJson(config, defaultSettings);
    return;
  }
  
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    if(!SPIFFS.exists("/config.json")){
      log(CONFIG_LOAD_SOURCE, WARNING_KIND, "Config file does not exists, create default config");
      deserializeJson(config, defaultSettings);
      updateSettings();
    }
    else{
      log(CONFIG_LOAD_SOURCE, WARNING_KIND, "Failed to open config file, load default config");
      deserializeJson(config, defaultSettings);
    }
    return;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    log(CONFIG_LOAD_SOURCE, WARNING_KIND, "Config file size is too large, load default config");
    deserializeJson(config, defaultSettings);
    return;
  }

  config.clear();

  auto error = deserializeJson(config, configFile.readString());
  configFile.close();  

  if (error) {
    log(CONFIG_LOAD_SOURCE, WARNING_KIND, "Failed to parse config file, load default config");
    deserializeJson(config, defaultSettings);
    return;
  } 

  config["device"]["kind"] = "mainDoor";
  config["device"]["mac"] = WiFi.macAddress();

  config["states"]["door"] = 4;
  config["states"]["average_temp"] = DEVICE_DISCONNECTED_C;

  log(CONFIG_LOAD_SOURCE, INFO_KIND, "Config file successfully loaded");
  // Serial.printf("         ssid: %s \n", SSID);
  // Serial.printf("         pass: %s \n", PASS);
  // Serial.printf("         open_bound: %f \n", OPEN_BOUND);
  // Serial.printf("         close_bound: %f \n", CLOSE_BOUND);
  // Serial.printf("         name: %s \n", DEVICE_NAME);
  // Serial.printf("         server: %s \n", SERVER_ADDR);
}

void updateSettings(){
  DynamicJsonDocument tempSettings(512);

  tempSettings["settings"] = config["settings"];

  if (!SPIFFS.begin()) {
    log(CONFIG_SAVE_SOURCE, ERROR_KIND, "Failed to mount file system");
    return;
  }

  SPIFFS.remove("/config.json");
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    log(CONFIG_SAVE_SOURCE, ERROR_KIND, "Failed to open config file for writing");
    return;
  }
  //configFile.print(defaultSettings);
  serializeJson(tempSettings, configFile);
  configFile.close();
  tempSettings.clear();

  log(CONFIG_SAVE_SOURCE, INFO_KIND, "Config file successfully saved");  

  initService();
}
