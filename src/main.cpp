#include <Arduino.h>
#include <stdint.h>
#include "HotEnd.h"
#include "LoadCell.h"
#include "ExtruderStepper.h"

// Stepper 
#define STEP_PIN    10
#define DIR_PIN     11
#define EN_PIN      9

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
ExtruderStepper extruder(STEP_PIN, DIR_PIN, EN_PIN);

// ====================== Funktionen-Definitionen ======================//

void setup(){
  Serial.begin(115200);
  extruder.begin(3000, 3000);
  
}

void loop(){

  extruder.run();

  static uint32_t lastLog = 0;
  uint32_t now = millis();

  if (now - lastLog >= 100) {              // alle 100 ms
    lastLog = now;

    //double temp = myHotEnd.getTemperature();
    //Serial.printf("%0.3f,%lu\n", temp, now);
  }

  static bool once = false;

  if (!extruder.isRunning()) {
    if (!once) {
      Serial.println("Extrudiere 10 mm vorwaerts...");
      extruder.extrudeMM(10.0);
      once = true;
    } else {
      Serial.println("Extrudiere 10 mm rueckwaerts...");
      extruder.extrudeMM(-10.0);
      once = false;
      delay(2000); 
    }
  }
}


// ====================== Funktionen-Implementierungen ======================//

