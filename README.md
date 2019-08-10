## tags
nodemcu esp8266 websockets heroku repl nodejs ws


## about
Проект предназначен для управления открыванием\закрыванием двери по границам температуры
 - управление направлением мотора двери - два пина платы, обвязка на транзисторах или реле гуглится
 - два концевика - крайние состояния двери - два пина
 - датчик(и) температуры DS18B20 на одном пине (до 250 вроде как, опасайтесь палёных датчиков)
Всё это с реал-тайм управлением и красивым отображением из интернета


## important
Проект свёрнут(, на гит выложил всё то что было готово, смотри

Сделано:
 - автономная работа двери по температуре
 - настройка, хранение
 - реалтайм интерфейс в локальной сети, скрины [1](https://raw.githubusercontent.com/qcp/gh/master/some%20pics/_E__GitHub_gh_GH_DEVICE_html_index.html(iPhone%20X)%20(1).png), [2](https://raw.githubusercontent.com/qcp/gh/master/some%20pics/_E__GitHub_gh_GH_DEVICE_html_index.html(iPhone%20X)%20(2).png)
 
Не сделано:
 - внешний сервер, планировался на nodejs и крутиться на heroku

Код не причёсан, сорре


## intresting
Из интересного что ты тут можешь найти
 - На июль 2019 не работает функция beginSSL библиотеки WebSockets для heroku и repl.it, чекай этот [вопрос](https://github.com/Links2004/arduinoWebSockets/issues/440)
 - для подключения к сокету юзай 80 порт, сам редиректнет на нужный
 - заметки в ходе поиска подходящих пинов, [скриншот финалочки](https://raw.githubusercontent.com/qcp/gh/master/some%20pics/wifi2.png)
 - настройки visual code и расширения arduino от microsoft в папке **GH_DEVICE\espCheck\.vscode**
 - ну и возможно логику работы с дверью и её состояния в **door.ino**


## code here
 - тут [болванка сервера](https://github.com/qcp/gh/blob/master/GH_NODE)
 - [заметка по пинам](https://github.com/qcp/gh/blob/master/GH_DEVICE/pinout.txt)
 - [код платы](https://github.com/qcp/gh/tree/master/GH_DEVICE/espCheck)
 - [код страницы настроек в локальной сети](https://github.com/qcp/gh/tree/master/GH_DEVICE/html), при встраивании внимательней на адрес подключения коммент\ункомент
 - [предпологаемый формат состояний](https://github.com/qcp/gh/blob/master/GH_DEVICE/door.json)

### board - nodeMCU 1.0
 - esp8266 by ESP8266 Community v. 2.5.2

### libs
 - DallasTemperature v. 3.8.0
 - OneWire v. 2.3.4
 - ArduinoJson by Benoit Blanchon v.6.11.13
 - WebSockets by Markus Sattler v. 2.1.4

> библиотеки ставь сам