#include "ExtruderStepper.h"
#include <math.h> 

//========== Konstruktor ==========//   
ExtruderStepper::ExtruderStepper(uint8_t stepPin, uint8_t dirPin, uint8_t enPin)
: _stepPin(stepPin),
  _dirPin(dirPin),
  _enPin(enPin),
  _stepsPerMM( (_STEPS_PER_REV * (float)_MICROSTEPPING * _GEAR_RATIO) / _MM_PER_REV ),
  _stepper(AccelStepper::DRIVER, stepPin, dirPin)
{}

//========== Öffentliche Funktions-Implementierungen  ==========//
void ExtruderStepper::begin(float maxSpeedStepsPerS, float accelStepsPerS2) {
  pinMode(_stepPin, OUTPUT);
  pinMode(_dirPin,  OUTPUT);
  pinMode(_enPin,   OUTPUT);
  enable(true);

  _stepper.setMaxSpeed(maxSpeedStepsPerS);
  _stepper.setAcceleration(accelStepsPerS2);

  // Debug: zeigt dir sofort, ob stepsPerMM plausibel ist
  Serial.print("Extruder stepsPerMM = ");
  Serial.println(_stepsPerMM, 4);
}

void ExtruderStepper::enable(bool on) {
  if (_ENABLE_ACTIVE_LOW) {
    digitalWrite(_enPin, on ? LOW : HIGH);
  } else {
    digitalWrite(_enPin, on ? HIGH : LOW);
  }
}

void ExtruderStepper::extrudeMM(float mm) {
  long steps = lroundf(mm * _stepsPerMM);
  _stepper.move(steps);
}

void ExtruderStepper::run() {
  _stepper.run();
}

bool ExtruderStepper::isRunning() {
  return _stepper.distanceToGo() != 0;
}

long ExtruderStepper::distanceToGo() {
  return _stepper.distanceToGo();
}
