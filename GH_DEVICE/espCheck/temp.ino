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