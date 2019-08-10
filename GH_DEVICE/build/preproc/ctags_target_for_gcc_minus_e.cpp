# 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
# 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
# 2 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2

# 4 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2
# 5 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2
# 6 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2

# 8 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2

# 10 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2
# 11 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2

# 13 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2
# 14 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino" 2
# 39 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
ESP8266WebServer http(80);
WebSocketsServer wss = WebSocketsServer(81);

WebSocketsClient wsc;

OneWire oneWire(D7);
DallasTemperature sensors(&oneWire);

DynamicJsonDocument config(1024);

String msg;

void setup(){

  Serial.begin(115200);
  Serial.println();

  pinMode(16, 0x01);
  digitalWrite(16, !0x0);

  pinMode(D1, 0x01);
  pinMode(D2, 0x01);
  motor(0);

  pinMode(D5, 0x02);
  pinMode(D6, 0x02);

  initService();
}

void initService(){
  loadConfig();

  initTempSensor();

  wss.begin();
  wss.onEvent(webSocketEvent1);

  //beginSSL not working with heroku & repl.it
  wsc.begin(config["settings"]["server"].as<char*>(), 80);
  wsc.onEvent(webSocketEvent2);
  wsc.setReconnectInterval(5000);
  wsc.enableHeartbeat(5000, 3000, 2);

  http.on("/", indexFunction);
  http.onNotFound(errorFunction);
  http.begin();

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.hostname(config["settings"]["name"].as<char*>());

}

void loop(){
  checkDoorState();
  checkTemp();

  if(establisheWiFiConnection()){
    wss.loop();
    wsc.loop();
    http.handleClient();
  }
}
# 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\config.ino"
String defaultSettings = "{  \"settings\": {\"name\": \"door\", \"wifi\": {\"ssid\": \"Home\", \"pass\": \"1qazsedcft6\"}, \"temp\": {\"open_bound\": 30, \"close_bound\": 20 }, \"server\": \"greenhouse--qcp.repl.co\"} }";

void loadConfig(){
  if (!SPIFFS.begin()) {
    log("Config load", "warning", "Failed to mount file system, load default config");
    deserializeJson(config, defaultSettings);
    return;
  }

  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    if(!SPIFFS.exists("/config.json")){
      log("Config load", "warning", "Config file does not exists, create default config");
      deserializeJson(config, defaultSettings);
      updateSettings();
    }
    else{
      log("Config load", "warning", "Failed to open config file, load default config");
      deserializeJson(config, defaultSettings);
    }
    return;
  }

  size_t size = configFile.size();
  if (size > 1024) {
    log("Config load", "warning", "Config file size is too large, load default config");
    deserializeJson(config, defaultSettings);
    return;
  }

  config.clear();

  auto error = deserializeJson(config, configFile.readString());
  configFile.close();

  if (error) {
    log("Config load", "warning", "Failed to parse config file, load default config");
    deserializeJson(config, defaultSettings);
    return;
  }

  config["device"]["kind"] = "mainDoor";
  config["device"]["mac"] = WiFi.macAddress();

  config["states"]["door"] = 4;
  config["states"]["average_temp"] = -127;

  log("Config load", "info", "Config file successfully loaded");
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
    log("Config save", "error", "Failed to mount file system");
    return;
  }

  SPIFFS.remove("/config.json");
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    log("Config save", "error", "Failed to open config file for writing");
    return;
  }
  //configFile.print(defaultSettings);
  serializeJson(tempSettings, configFile);
  configFile.close();
  tempSettings.clear();

  log("Config save", "info", "Config file successfully saved");

  initService();
}
# 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\door.ino"
enum {
  GH_OPEN = 0,
  GH_OPENING = 1,
  GH_CLOSE = 2,
  GH_CLOSING = 3,
  GH_UNKNOW = 4,
  GH_ERROR = 5
};

void updateDoorState(uint8 newDoorState){
    uint8 _ds = config["states"]["door"].as<uint8>();
    if(newDoorState == _ds)
      return;
    if(newDoorState == GH_OPENING && _ds == GH_OPEN)
      return;
    if(newDoorState == GH_CLOSING && _ds == GH_CLOSE)
      return;

    _ds = newDoorState;
    config["states"]["door"] = _ds;
    broadcastInfo("states");

    // if(_ds == GH_OPEN)
    //     log(DOOR_SOURCE, INFO_KIND, "state - open");
    // else if(_ds == GH_OPENING)
    //     log(DOOR_SOURCE, INFO_KIND, "state - opening");
    // else if(_ds == GH_CLOSE)
    //     log(DOOR_SOURCE, INFO_KIND, "state - close");
    // else if(_ds == GH_CLOSING)
    //     log(DOOR_SOURCE, INFO_KIND, "state - closing");
    // else if(_ds == GH_UNKNOW)
    //     log(DOOR_SOURCE, WARNING_KIND, "state - unknow");
    // else if(_ds == GH_ERROR)
    //     log(DOOR_SOURCE, ERROR_KIND, "state - error");

}

void checkDoorState(){
  bool openLS = !digitalRead(D5);
  bool closeLS = !digitalRead(D6);

  uint8 _ds = config["states"]["door"].as<uint8>();

  switch(_ds){
      case GH_OPEN:
      {
          motor(0);

          if(openLS && !closeLS){
              //всё ок, как и должно быть            
          }
          else if(!openLS && closeLS){
              //дверь оказалась резко закрыта
              updateDoorState(GH_CLOSE);
          }
          else if(openLS && closeLS){
              //оба концевика замкнуты - ошибка
              updateDoorState(GH_ERROR);
          }
          else{
              //оба концевика разомкнуты - ктото дёргает дверь
              updateDoorState(GH_UNKNOW);
          }
      }
      break;
      case GH_OPENING:
      {
          motor(1);

          if(openLS && !closeLS){
              //дверь открылась нами
              updateDoorState(GH_OPEN);
          }
          else if(!openLS && closeLS){
              //дверь закрылась(!?), мотор крутился нетуда
              //updateDoorState(GH_CLOSE);
              //или концевик ещё не отпустило, таймаут?
          }
          else if(openLS && closeLS){
              //оба концевика замкнуты - ошибка
              updateDoorState(GH_ERROR);
          }
          else{
              //дверь в процессе открытия            
          }
      }
      break;
      case GH_CLOSE:
      {
          motor(0);

          if(openLS && !closeLS){
              //дверь оказалась резко открыта
              updateDoorState(GH_OPEN);
          }
          else if(!openLS && closeLS){
              //всё ок, так и должно быть                      
          }
          else if(openLS && closeLS){
              //оба концевика замкнуты - ошибка
              updateDoorState(GH_ERROR);
          }
          else{
              //оба концевика разомкнуты - ктото дёргает дверь
              updateDoorState(GH_UNKNOW);
          }
      }
      break;
      case GH_CLOSING:
      {
          motor(-1);

          if(openLS && !closeLS){
              //дверь открылась(!?), мотор крутился нетуда
              //updateDoorState(GH_OPEN);
              //или концевик ещё не отпустило, таймаут?
          }
          else if(!openLS && closeLS){
              //дверь закрылась нами
              updateDoorState(GH_CLOSE);
              motor(0);
          }
          else if(openLS && closeLS){
              //оба концевика замкнуты - ошибка
              updateDoorState(GH_ERROR);
          }
          else{
              //дверь в процессе закрытия            
          }
      }
      break;
      case GH_UNKNOW:
      {
          motor(0);

          if(openLS && !closeLS){
              //дверь открылась
              updateDoorState(GH_OPEN);
          }
          else if(!openLS && closeLS){
              //дверь закрылась
              updateDoorState(GH_CLOSE);
          }
          else if(openLS && closeLS){
              //оба концевика замкнуты - ошибка
              updateDoorState(GH_ERROR);
          }
          else{
              //всё ещё кто-то дёргает дверь           
          }
      }
      break;
      case GH_ERROR:
      {
          motor(0);

          if(openLS && !closeLS){
              //дверь открылась
              updateDoorState(GH_OPEN);
          }
          else if(!openLS && closeLS){
              //дверь закрылась
              updateDoorState(GH_CLOSE);
          }
          else if(openLS && closeLS){
              //оба концевика замкнуты - ошибка                                  
          }
          else{
              //оба концевика разомкнуты - ктото дёргает дверь
              updateDoorState(GH_UNKNOW);
          }
      }
      break;
  }
}

void motor(int8 dir){
  if(dir == 1){
    digitalWrite(D1, 0x0);
    delay(200);
    digitalWrite(D2, 0x1);
  }
  else if(dir == -1){
    digitalWrite(D2, 0x0);
    delay(200);
    digitalWrite(D1, 0x1);
  }
  else{
    digitalWrite(D1, 0x0);
    digitalWrite(D1, 0x0);
  }
}
# 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\temp.ino"
uint8 numberOfDevices;
DeviceAddress tempDeviceAddress;
float lastAverageTemp = 127;

void initTempSensor(){
  sensors.begin();
  numberOfDevices = sensors.getDeviceCount();

  msg = "Found ";
  msg += numberOfDevices;
  msg += " devices, parasite power is: ";
  msg += sensors.isParasitePowerMode() ? "ON" : "OFF";
  log("Temp", "info", msg);

  for(uint8 i=0; i<numberOfDevices; i++)
  {
    if(sensors.getAddress(tempDeviceAddress, i))
    {
      sensors.setResolution(tempDeviceAddress, 9);

      msg = "Found device ";
      msg += i;
      msg += " with address: ";
      for (uint8_t i = 0; i < 8; i++) {
        msg += tempDeviceAddress[i] < 16 ? "0" : "";
        msg += String(tempDeviceAddress[i], 16);
      }
      msg += " resolution ";
      msg += sensors.getResolution(tempDeviceAddress);
      log("Temp", "info", msg);
    }
    else{
      msg = "Found ghost device at ";
      msg += i;
      msg += " but could not detect address. Check power and cabling";
      log("Temp", "error", msg);
    }
  }

  sensors.requestTemperatures();
}

void updateAverageTemp(){
  uint8 counts = 0;
  float sum = 0;
  for(int i=0;i<numberOfDevices; i++){
    if(sensors.getAddress(tempDeviceAddress, i)){
      //msg = "Temperature for device: ";
      //msg += i;

      float tempC = sensors.getTempC(tempDeviceAddress);
      if(tempC == -127){
        //msg += " could not read temperature data";
      }
      else{
        //msg += tempC;

        counts++;
        sum += tempC;
      }
      //log(TEMP_SOURCE, INFO_KIND, msg);
    }
    else{
      // msg = "Could not connect to device: ";
      // msg += i;
      // log(TEMP_SOURCE, WARNING_KIND, msg);
    }
  }

  if(counts > 0){
    config["states"]["average_temp"] = sum / counts;
    // msg = "Average temperature ";
    // msg += config["states"]["average_temp"].as<String>();
    // log(TEMP_SOURCE, INFO_KIND, msg);
  }
  else{
    // config["states"]["average_temp"] = DEVICE_DISCONNECTED_C;
    // log(TEMP_SOURCE, ERROR_KIND, "No temperature information");
  }

  sensors.requestTemperatures();
}

void checkTemp(){
  updateAverageTemp();

  float _at = config["states"]["average_temp"].as<float>();

  if(_at != lastAverageTemp){

    if(_at == -127)
      broadcastInfo("states");//??    
    else if(_at >= config["settings"]["temp"]["open_bound"].as<float>())
      updateDoorState(GH_OPENING);
    else if (_at <= config["settings"]["temp"]["close_bound"].as<float>())
      updateDoorState(GH_CLOSING);
    else
      broadcastInfo("states");

    lastAverageTemp = _at;
  }
}
# 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
String indexHtml = "<!DOCTYPE HTML> <html> <head> <title id=\"i-name\"></title> <meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width, minimum-scale=1\"> <meta http-equiv=\"Cache-Control\" content=\"no-cache\"> <link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Roboto:300,300italic,700,700italic\"> <link rel=\"stylesheet\" href=\"https://cdn.rawgit.com/necolas/normalize.css/master/normalize.css\"> <link rel=\"stylesheet\" href=\"https://cdn.rawgit.com/milligram/milligram/master/dist/milligram.min.css\"> <style> body{ margin: 0; background: #f3f3f3; } section{ height: 100vh; min-height: 800px; width: 80%; max-width: 600px; margin: auto; display: flex; flex-direction: column; justify-content: center; } </style> </head> <body> <section id=\"ui-error\"> Нет подключения </section> <section id=\"ui-control\"> <h2>Управление</h2> <p>MAC: <span id=\"i-mac\"></span></p> <p>Дверь: <span id=\"i-door\"></span></p> <p>Температура: <span id=\"i-average_temp\"></span></p> <button id=\"openBtn\" class=\"button\" onclick=\"btnOpen()\">Открыть</button> <button id=\"closeBtn\" class=\"button\" onclick=\"btnClose()\">Закрыть</button> </section> <section id=\"ui-settings\"> <h2>Настройки</h2> <p>Название устройства: <input id=\"s-name\"></p> <p>Сервер: <input id=\"s-server\"></p> <p>Название WiFi: <input id=\"s-ssid\"></p> <p>Пароль WiFi: <input id=\"s-pass\"></p> <p>Температура открытия: <input id=\"s-open_bound\"></p> <p>Температура закрытия: <input id=\"s-close_bound\"></p> <button id=\"saveBtn\" class=\"button\" onclick=\"btnSave()\">Сохранить</button> </section> <script type = \"text/javascript\"> function hide(){ document.getElementById(\"ui-error\").style.display = \"flex\"; document.getElementById(\"ui-control\").style.display = \"none\"; document.getElementById(\"ui-settings\").style.display = \"none\"; } hide(); if (!(\"WebSocket\" in window)) document.getElementById(\"ui-error\").textContent = \"Не поддерживаются веб сокеты\"; var ws = new WebSocket('ws://' + window.location.hostname + ':81'); ws.onopen = () => { document.getElementById(\"ui-error\").style.display = \"none\"; document.getElementById(\"ui-control\").style.display = \"flex\"; document.getElementById(\"ui-settings\").style.display = \"flex\"; }; ws.onmessage = (event) => { console.log(event.data); var obj = JSON.parse(event.data); if(obj.device){ document.getElementById(\"i-mac\").textContent = obj.device.mac; document.getElementById(\"i-name\").textContent = obj.device.name; } if(obj.settings){ document.getElementById(\"s-name\").value = obj.settings.name; document.getElementById(\"s-server\").value = obj.settings.server; document.getElementById(\"s-ssid\").value = obj.settings.wifi.ssid; document.getElementById(\"s-pass\").value = obj.settings.wifi.pass; document.getElementById(\"s-open_bound\").value = obj.settings.temp.open_bound; document.getElementById(\"s-close_bound\").value = obj.settings.temp.close_bound; } if(obj.states){ var s = { 0:\"Открыта\", 1:\"Открывается\", 2:\"Закрыта\", 3:\"Закрывается\", 4:\"Неизвестно\", 5:\"Ошибка\" }; document.getElementById(\"i-door\").textContent = s[obj.states.door]; document.getElementById(\"i-average_temp\").textContent = obj.states.average_temp == -127 ? \"Ошибка\" : obj.states.average_temp; } }; ws.onclose = (event) => { document.getElementById(\"ui-error\").textContent = event.reason; hide(); }; ws.onerror = (error) => { document.getElementById(\"ui-error\").textContent = error.message; hide(); }; function btnOpen(){ var o={ \"action\": { \"command\":\"open\" } }; ws.send(JSON.stringify(o)); } function btnClose(){ var o={ \"action\": { \"command\":\"close\" } }; ws.send(JSON.stringify(o)); } function btnSave(){ var o={ \"settings\": { \"name\": document.getElementById(\"s-name\").value, \"wifi\": { \"ssid\": document.getElementById(\"s-ssid\").value, \"pass\": document.getElementById(\"s-pass\").value }, \"temp\": { \"open_bound\": parseInt(document.getElementById(\"s-open_bound\").value), \"close_bound\": parseInt(document.getElementById(\"s-close_bound\").value) }, \"server\": document.getElementById(\"s-server\").value } }; ws.send(JSON.stringify(o)); hide(); document.location.reload(true); } </script> </body> </html>";
String errorHtml = "<!DOCTYPE HTML> <html> <head> <meta http-equiv=\"refresh\" content=\"1;URL=/\" /> </head> <body> <center>Redirecting...</center> </body> </html>";

void webSocketEvent2(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
        log("WS Client", "info", "Disconnected");
        break;
    case WStype_CONNECTED:
        {
          log("WS Client", "info", "Connected");
          broadcastInfoServer("device");
          //broadcastInfoServer("settings");
          //broadcastInfoServer("states");
        }
        break;
    case WStype_TEXT:
        {
          msg = "Get text: ";
          msg += String((char*)payload);
          log("WS Client", "info", msg);
        }
        break;
  }
}
void webSocketEvent1(unsigned short num, WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_ERROR:
      {
        msg = "Error ";
        msg += num;
        log("WS Server", "info", msg);
      }
      break;
    case WStype_DISCONNECTED:
      {
        msg = "Disconnected ";
        msg += num;
        log("WS Server", "info", msg);
      }
      break;
    case WStype_CONNECTED: {
      IPAddress ip = wss.remoteIP(num);

      msg = "Connected ";
      msg += num;
      msg += " from ";
      msg += ip.toString();
      log("WS Server", "info", msg);

      broadcastInfoClient("device");
      broadcastInfoClient("settings");
      broadcastInfoClient("states");
    }
    break;
    case WStype_TEXT:{
      StaticJsonDocument<512> tempAction;

      auto error = deserializeJson(tempAction, payload);
      if (error) {
        msg = "Get ";
        msg += num;
        msg += " unknown command: ";
        msg += String((char*)payload);
        log("WS Server", "info", msg);
        return;
      }
      if(tempAction.containsKey("action")){
        if(tempAction["action"]["command"] == "open")
          updateDoorState(GH_OPENING);
        else if(tempAction["action"]["command"] == "close")
          updateDoorState(GH_CLOSING);
      }
      else if(tempAction.containsKey("settings")){
        config["settings"] = tempAction["settings"];
        updateSettings();
      }
      tempAction.clear();
    }
    break;
  }
}

void indexFunction(){
    http.send(200, "text/html", indexHtml);
}

void errorFunction(){
    http.send(200, "text/html", errorHtml);
}


void printConnectionInfo(){
  switch (WiFi.status())
  {
  case WL_CONNECTED:
    log("WiFi", "info", String("Connected, IP - ")+WiFi.localIP().toString());
    break;
  case WL_IDLE_STATUS:
    log("WiFi", "info", "Connecting");
    break;
  case WL_NO_SSID_AVAIL:
    log("WiFi", "error", "WiFi unavailable");
    break;
  case WL_CONNECT_FAILED:
    log("WiFi", "warning", "Connecting failed");
    break;
  case WL_CONNECTION_LOST:
    log("WiFi", "warning", "Connecting loose");
    break;
  case WL_DISCONNECTED:
    log("WiFi", "warning", "Disconnected");
    break;
  default:
    log("WiFi", "error", String("Unknown state №")+String(WiFi.status()));
    break;
  }
}

bool establisheWiFiConnection(){
  if(WiFi.status() == WL_CONNECTED){
    digitalWrite(16, !0x1);
    return true;
  }
  else{
    digitalWrite(16, !0x0);
    printConnectionInfo();
    WiFi.begin(config["settings"]["wifi"]["ssid"].as<char*>(), config["settings"]["wifi"]["pass"].as<char*>());

    bool result = WiFi.waitForConnectResult() == WL_CONNECTED;
    printConnectionInfo();
    return result;
  }
}
void broadcastInfo(String part){
  broadcastInfoClient(part);
  broadcastInfoServer(part);
}
void broadcastInfoClient(String part){
  String s = "";
  serializeJson(config[part], s);

  if(wss.connectedClients(false))
    wss.broadcastTXT("{\""+part+"\":"+s+"}");

  log(part, "broadcast", s);
}
void broadcastInfoServer(String part){
  String s = "";
  serializeJson(config[part], s);

  wsc.sendTXT("{\""+part+"\":"+s+"}");

  log(part, "broadcast", s);
}
void log(String source, String kind, String message){
  StaticJsonDocument<512> tempLog;
  tempLog["log"]["kind"] = kind;
  tempLog["log"]["source"] = source;
  tempLog["log"]["message"] = message;

  String s = "";
  serializeJson(tempLog, s);
  wsc.sendTXT(s);
  Serial.printf("[%s] %s: %s \r\n", kind.c_str(), source.c_str(), message.c_str());
}
