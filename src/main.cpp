#include <Arduino.h>
#include "HotEnd.h"
#include "LoadCell.h"

//========== Makros ==========//   

// Hot-End
#define NTC_PIN 4
#define HEATER_PIN 5
#define FAN_PIN 6

// Load Cell
#define LOADCELL_DOUT_PIN 8
#define LOADCELL_SCK_PIN 3

//========== Objekte ==========//   
HotEnd myHotEnd(HEATER_PIN, NTC_PIN, FAN_PIN);
LoadCell myLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

bool state = true;

void setup() {
  Serial.begin(115200);

}

void loop() {

  
  unsigned long  time = millis();
  while(state){
    double temp = myHotEnd.getTemperature();
    double res  = myHotEnd.getNtcResistance();
    unsigned long  time = millis();
    // double wheight =  myLoadCell.getMeanWheight(10);

    Serial.printf("%0.3f,%lu\n", temp, time);

    myHotEnd.setFanPwm(180);

      /*
    if (temp < 200.0) {
      Serial.println("ON");
      //myHotEnd.setHeaterPwm(255);
    } else {
      Serial.println("OFF");
      myHotEnd.setHeaterPwm(0);
    }
    */
   if(time > 20000) state = false;
  }
  Serial.println("stop");
  delay(100);
}
