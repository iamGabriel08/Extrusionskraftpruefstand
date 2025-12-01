#include <Arduino.h>
#include "HotEnd.h"
#include "LoadCell.h"

//========== Makros ==========//   

// Hot-End
#define NTC_PIN 0
#define HEATER_PIN 2

// Load Cell
#define LOADCELL_DOUT_PIN 4
#define LOADCELL_SCK_PIN 5

//========== Objekte ==========//   
HotEnd myHotEnd(HEATER_PIN, NTC_PIN);
LoadCell myLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

void setup() {
  Serial.begin(115200);
}

void loop() {
  double temp = myHotEnd.getTemperature();
  Serial.println(temp);
  delay(1000);
}

