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

void setup() {
  Serial.begin(115200);
}

void loop() {

  double temp = myHotEnd.getTemperature();
  double res  = myHotEnd.getNtcResistance();
  double wheight =  myLoadCell.getMeanWheight(10);
  /*
  // Temperatur über Zeit
  Serial.print(">temp:");
  Serial.println(temp);

  // ==== (optional) normale Debug-Ausgaben ====
  // Diese werden nur im Teleplot-Log angezeigt, nicht geplottet
  Serial.print("Widerstand: ");
  Serial.println(res);
  Serial.print("Temperatur: ");
  Serial.println(temp);

  Serial.print("Gewicht: ");
  Serial.println(wheight);

  */
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

  delay(100);
}
