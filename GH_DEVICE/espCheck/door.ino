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