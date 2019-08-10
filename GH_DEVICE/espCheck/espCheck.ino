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





ESP8266WebServer http(80);
WebSocketsServer wss = WebSocketsServer(81);

WebSocketsClient wsc;

OneWire oneWire(TEPM_PIN);
DallasTemperature sensors(&oneWire);

DynamicJsonDocument config(1024);

String msg;

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