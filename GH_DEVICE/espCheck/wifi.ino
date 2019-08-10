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

  if(wss.connectedClients(false)){
    wss.broadcastTXT("{\""+part+"\":"+s+"}");
    Serial.printf("[broadcast client] %s: %s", part, s.c_str());
  }
}
void broadcastInfoServer(String part){
  String s = "";
  serializeJson(config[part], s);

  wsc.sendTXT("{\""+part+"\":"+s+"}");

  Serial.printf("[broadcast server] %s: %s", part, s.c_str());
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