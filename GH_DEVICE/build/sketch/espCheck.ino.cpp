#line 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
#line 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <WebSocketsServer.h>
#include <ESP8266WebServer.h>

#include <WebSocketsClient.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ArduinoJson.h>
#include <FS.h>

#define RELAY_PIN_1 D1
#define RELAY_PIN_2 D2
#define LIMITSWITCH_PIN_1 D5
#define LIMITSWITCH_PIN_2 D6
#define TEPM_PIN D7

#define TEMP_SOURCE "Temp"
#define WIFI_SOURCE "WiFi"
#define DOOR_SOURCE "Door"
#define CONFIG_LOAD_SOURCE "Config load"
#define CONFIG_SAVE_SOURCE "Config save"
#define WS_SERVER_SOURCE "WS Server"
#define WS_CLIENT_SOURCE "WS Client"


#define INFO_KIND "info"
#define WARNING_KIND "warning"
#define ERROR_KIND "error"
#define BROADCAST_KIND "broadcast"





ESP8266WebServer http(80);
WebSocketsServer wss = WebSocketsServer(81);

WebSocketsClient wsc;

OneWire oneWire(TEPM_PIN);
DallasTemperature sensors(&oneWire);

DynamicJsonDocument config(1024);

String msg;

#line 51 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
void setup();
#line 69 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
void initService();
#line 93 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
void loop();
#line 3 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\config.ino"
void loadConfig();
#line 57 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\config.ino"
void updateSettings();
#line 10 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\door.ino"
void updateDoorState(uint8 newDoorState);
#line 38 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\door.ino"
void checkDoorState();
#line 177 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\door.ino"
void motor(int8 dir);
#line 5 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\temp.ino"
void initTempSensor();
#line 43 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\temp.ino"
void updateAverageTemp();
#line 84 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\temp.ino"
void checkTemp();
#line 4 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void webSocketEvent2(WStype_t type, uint8_t * payload, size_t length);
#line 26 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void webSocketEvent1(unsigned short num, WStype_t type, uint8_t * payload, size_t length);
#line 84 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void indexFunction();
#line 88 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void errorFunction();
#line 93 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void printConnectionInfo();
#line 120 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
bool establisheWiFiConnection();
#line 135 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void broadcastInfo(String part);
#line 139 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void broadcastInfoClient(String part);
#line 148 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void broadcastInfoServer(String part);
#line 156 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
void log(String source, String kind, String message);
#line 51 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\espCheck.ino"
void setup(){
  
  Serial.begin(115200);
  Serial.println();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !LOW);

  pinMode(RELAY_PIN_1, OUTPUT);
  pinMode(RELAY_PIN_2, OUTPUT);
  motor(0);

  pinMode(LIMITSWITCH_PIN_1, INPUT_PULLUP);
  pinMode(LIMITSWITCH_PIN_2, INPUT_PULLUP); 

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
#line 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\config.ino"
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

#line 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\door.ino"
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
  bool openLS = !digitalRead(LIMITSWITCH_PIN_1);
  bool closeLS = !digitalRead(LIMITSWITCH_PIN_2);

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
    digitalWrite(RELAY_PIN_1, LOW);
    delay(200);
    digitalWrite(RELAY_PIN_2, HIGH);
  }
  else if(dir == -1){
    digitalWrite(RELAY_PIN_2, LOW);
    delay(200);
    digitalWrite(RELAY_PIN_1, HIGH);
  }
  else{
    digitalWrite(RELAY_PIN_1, LOW);
    digitalWrite(RELAY_PIN_1, LOW);
  }
}
#line 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\temp.ino"
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
  log(TEMP_SOURCE, INFO_KIND, msg);
  
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
        msg += String(tempDeviceAddress[i], HEX);
      }           
      msg += " resolution ";
      msg += sensors.getResolution(tempDeviceAddress); 
      log(TEMP_SOURCE, INFO_KIND, msg);
    }
    else{
      msg = "Found ghost device at ";
      msg += i;
      msg += " but could not detect address. Check power and cabling";
      log(TEMP_SOURCE, ERROR_KIND, msg);
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
      if(tempC == DEVICE_DISCONNECTED_C){
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
    
    if(_at == DEVICE_DISCONNECTED_C)
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
#line 1 "e:\\GitHub\\gh\\GH_DEVICE\\espCheck\\wifi.ino"
String indexHtml = "<!DOCTYPE HTML> <html> <head> <title id=\"i-name\"></title> <meta charset=\"utf-8\"> <meta name=\"viewport\" content=\"width=device-width, minimum-scale=1\"> <meta http-equiv=\"Cache-Control\" content=\"no-cache\"> <link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css?family=Roboto:300,300italic,700,700italic\"> <link rel=\"stylesheet\" href=\"https://cdn.rawgit.com/necolas/normalize.css/master/normalize.css\"> <link rel=\"stylesheet\" href=\"https://cdn.rawgit.com/milligram/milligram/master/dist/milligram.min.css\"> <style> body{ margin: 0; background: #f3f3f3; } section{ height: 100vh; min-height: 800px; width: 80%; max-width: 600px; margin: auto; display: flex; flex-direction: column; justify-content: center; } </style> </head> <body> <section id=\"ui-error\"> Нет подключения </section> <section id=\"ui-control\"> <h2>Управление</h2> <p>MAC: <span id=\"i-mac\"></span></p> <p>Дверь: <span id=\"i-door\"></span></p> <p>Температура: <span id=\"i-average_temp\"></span></p> <button id=\"openBtn\" class=\"button\" onclick=\"btnOpen()\">Открыть</button> <button id=\"closeBtn\" class=\"button\" onclick=\"btnClose()\">Закрыть</button> </section> <section id=\"ui-settings\"> <h2>Настройки</h2> <p>Название устройства: <input id=\"s-name\"></p> <p>Сервер: <input id=\"s-server\"></p> <p>Название WiFi: <input id=\"s-ssid\"></p> <p>Пароль WiFi: <input id=\"s-pass\"></p> <p>Температура открытия: <input id=\"s-open_bound\"></p> <p>Температура закрытия: <input id=\"s-close_bound\"></p> <button id=\"saveBtn\" class=\"button\" onclick=\"btnSave()\">Сохранить</button> </section> <script type = \"text/javascript\"> function hide(){ document.getElementById(\"ui-error\").style.display = \"flex\"; document.getElementById(\"ui-control\").style.display = \"none\"; document.getElementById(\"ui-settings\").style.display = \"none\"; } hide(); if (!(\"WebSocket\" in window)) document.getElementById(\"ui-error\").textContent = \"Не поддерживаются веб сокеты\"; var ws = new WebSocket('ws://' + window.location.hostname + ':81'); ws.onopen = () => { document.getElementById(\"ui-error\").style.display = \"none\"; document.getElementById(\"ui-control\").style.display = \"flex\"; document.getElementById(\"ui-settings\").style.display = \"flex\"; }; ws.onmessage = (event) => { console.log(event.data); var obj = JSON.parse(event.data); if(obj.device){ document.getElementById(\"i-mac\").textContent = obj.device.mac; document.getElementById(\"i-name\").textContent = obj.device.name; } if(obj.settings){ document.getElementById(\"s-name\").value = obj.settings.name; document.getElementById(\"s-server\").value = obj.settings.server; document.getElementById(\"s-ssid\").value = obj.settings.wifi.ssid; document.getElementById(\"s-pass\").value = obj.settings.wifi.pass; document.getElementById(\"s-open_bound\").value = obj.settings.temp.open_bound; document.getElementById(\"s-close_bound\").value = obj.settings.temp.close_bound; } if(obj.states){ var s = { 0:\"Открыта\", 1:\"Открывается\", 2:\"Закрыта\", 3:\"Закрывается\", 4:\"Неизвестно\", 5:\"Ошибка\" }; document.getElementById(\"i-door\").textContent = s[obj.states.door]; document.getElementById(\"i-average_temp\").textContent = obj.states.average_temp == -127 ? \"Ошибка\" : obj.states.average_temp; } }; ws.onclose = (event) => { document.getElementById(\"ui-error\").textContent = event.reason; hide(); }; ws.onerror = (error) => { document.getElementById(\"ui-error\").textContent = error.message; hide(); }; function btnOpen(){ var o={ \"action\": { \"command\":\"open\" } }; ws.send(JSON.stringify(o)); } function btnClose(){ var o={ \"action\": { \"command\":\"close\" } }; ws.send(JSON.stringify(o)); } function btnSave(){ var o={ \"settings\": { \"name\": document.getElementById(\"s-name\").value, \"wifi\": { \"ssid\": document.getElementById(\"s-ssid\").value, \"pass\": document.getElementById(\"s-pass\").value }, \"temp\": { \"open_bound\": parseInt(document.getElementById(\"s-open_bound\").value), \"close_bound\": parseInt(document.getElementById(\"s-close_bound\").value) }, \"server\": document.getElementById(\"s-server\").value } }; ws.send(JSON.stringify(o)); hide(); document.location.reload(true); } </script> </body> </html>";
String errorHtml = "<!DOCTYPE HTML> <html> <head> <meta http-equiv=\"refresh\" content=\"1;URL=/\" /> </head> <body> <center>Redirecting...</center> </body> </html>";
  
void webSocketEvent2(WStype_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case WStype_DISCONNECTED:
        log(WS_CLIENT_SOURCE, INFO_KIND, "Disconnected");
        break;
    case WStype_CONNECTED:
        {
          log(WS_CLIENT_SOURCE, INFO_KIND, "Connected");
          broadcastInfoServer("device");
          //broadcastInfoServer("settings");
          //broadcastInfoServer("states");
        }
        break;
    case WStype_TEXT:
        {
          msg = "Get text: ";
          msg += String((char*)payload);
          log(WS_CLIENT_SOURCE, INFO_KIND, msg);
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
        log(WS_SERVER_SOURCE, INFO_KIND, msg);
      }
      break;
    case WStype_DISCONNECTED:
      {
        msg = "Disconnected ";
        msg += num;
        log(WS_SERVER_SOURCE, INFO_KIND, msg);
      }
      break;
    case WStype_CONNECTED: {
      IPAddress ip = wss.remoteIP(num);

      msg = "Connected ";
      msg += num;
      msg += " from ";
      msg += ip.toString();
      log(WS_SERVER_SOURCE, INFO_KIND, msg);

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
        log(WS_SERVER_SOURCE, INFO_KIND, msg); 
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
    log(WIFI_SOURCE, INFO_KIND, String("Connected, IP - ")+WiFi.localIP().toString());
    break;
  case WL_IDLE_STATUS:
    log(WIFI_SOURCE, INFO_KIND, "Connecting");
    break;
  case WL_NO_SSID_AVAIL:
    log(WIFI_SOURCE, ERROR_KIND, "WiFi unavailable");
    break;
  case WL_CONNECT_FAILED:
    log(WIFI_SOURCE, WARNING_KIND, "Connecting failed");
    break;
  case WL_CONNECTION_LOST:
    log(WIFI_SOURCE, WARNING_KIND, "Connecting loose");
    break;  
  case WL_DISCONNECTED:
    log(WIFI_SOURCE, WARNING_KIND, "Disconnected");
    break;
  default:
    log(WIFI_SOURCE, ERROR_KIND, String("Unknown state №")+String(WiFi.status()));
    break;
  }
}

bool establisheWiFiConnection(){
  if(WiFi.status() == WL_CONNECTED){
    digitalWrite(LED_BUILTIN, !HIGH); 
    return true;   
  }
  else{
    digitalWrite(LED_BUILTIN, !LOW);
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

  log(part, BROADCAST_KIND, s);
}
void broadcastInfoServer(String part){
  String s = "";
  serializeJson(config[part], s);

  wsc.sendTXT("{\""+part+"\":"+s+"}");

  log(part, BROADCAST_KIND, s);
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
