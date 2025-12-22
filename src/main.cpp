#include <Arduino.h>
#include <stdint.h>
#include "HotEnd.h"
#include "LoadCell.h"
#include <AccelStepper.h>

//========== Makros und Parameter ==========//

// Stepper
#define STEP_PIN    10
#define DIR_PIN     11
#define EN_PIN      9

// TMC2209-Parameter
#define R_SENSE        0.11f         // Rsense des BTT TMC2209 V1.3
#define DRIVER_ADDRESS 0b00          // Adresse, falls CFG-Pins nicht geändert wurden

// Extruder-Parameter (anpassen!)
const uint8_t   stepsPerRev   = 200;   // Vollschritte Motor
const uint8_t   microstepping = 16;    // wie oben im Treiber gesetzt
const float gearRatio     = 3.0;   // falls Getriebe, sonst 1.0
const float mmPerRev      = 7.0;   // Förderraten-Angabe für dein Extruder
const float stepsPerMM = (stepsPerRev * microstepping * gearRatio) / mmPerRev;

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
AccelStepper extruder(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);

// ====================== Funktionen-Definitionen ======================//
void extrudeMM(float mm);
void initExtruder();

void setup(){
  Serial.begin(115200);
  initExtruder();
  
}

void loop(){
   // Stepper IMMER zuerst und so oft wie möglich bedienen
  extruder.run();

  // -------- Hot End / Logging nur periodisch --------
  static uint32_t lastLog = 0;
  uint32_t now = millis();

  if (now - lastLog >= 100) {              // alle 100 ms
    lastLog = now;

    double temp = myHotEnd.getTemperature();
    Serial.printf("%0.3f,%lu\n", temp, now);
  }

  static bool once = false;

  if (!extruder.isRunning()) {
    if (!once) {
      Serial.println("Extrudiere 10 mm vorwaerts...");
      extrudeMM(10.0);
      once = true;
    } else {
      Serial.println("Extrudiere 10 mm rueckwaerts...");
      extrudeMM(-10.0);
      once = false;
      delay(2000); 
    }
  }
}


// ====================== Funktionen-Implementierungen ======================//

void extrudeMM(float mm) {
  long steps = (long)(mm * stepsPerMM);
  extruder.move(steps);
}

void initExtruder(){
  pinMode(STEP_PIN, OUTPUT);
  pinMode(DIR_PIN,  OUTPUT);
  pinMode(EN_PIN,   OUTPUT);

  // Treiber erstmal deaktivieren (EN = HIGH bei BTT-Modulen = off)
  digitalWrite(EN_PIN, HIGH);

  // Jetzt Treiber aktivieren (EN = LOW)
  digitalWrite(EN_PIN, LOW);

  // AccelStepper konfigurieren
  extruder.setMaxSpeed(3000);       // Schritte pro Sekunde
  extruder.setAcceleration(3000);   // Schritte pro Sekunde^2

  Serial.println("Extruder initialisiert.");
}