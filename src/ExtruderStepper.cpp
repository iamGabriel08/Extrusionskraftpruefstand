#include "ExtruderStepper.h"
#include <math.h> // lroundf

ExtruderStepper::ExtruderStepper(uint8_t stepPin, uint8_t dirPin, uint8_t enPin,
                                 int stepsPerRev, int microstepping,
                                 float gearRatio, float mmPerRev)
: _stepPin(stepPin),
  _dirPin(dirPin),
  _enPin(enPin),
  _stepsPerRev(stepsPerRev),
  _microstepping(microstepping),
  _gearRatio(gearRatio),
  _mmPerRev(mmPerRev),
  _stepsPerMM(0.0f),
  _stepper(AccelStepper::DRIVER, stepPin, dirPin)
{
  _stepsPerMM = (_stepsPerRev * _microstepping * _gearRatio) / _mmPerRev;
}

void ExtruderStepper::begin(uint32_t serialBaud) {
  Serial.begin(serialBaud);
  delay(500);

  pinMode(_stepPin, OUTPUT);
  pinMode(_dirPin,  OUTPUT);
  pinMode(_enPin,   OUTPUT);

  // BTT/TMC: EN=HIGH -> aus, EN=LOW -> an
  disable();
  enable();

  Serial.println("Extruder initialisiert.");
}

void ExtruderStepper::enable() {
  digitalWrite(_enPin, LOW);
}

void ExtruderStepper::disable() {
  digitalWrite(_enPin, HIGH);
}

void ExtruderStepper::setMaxSpeed(float stepsPerSecond) {
  _stepper.setMaxSpeed(stepsPerSecond);
}

void ExtruderStepper::setAcceleration(float stepsPerSecond2) {
  _stepper.setAcceleration(stepsPerSecond2);
}

void ExtruderStepper::extrudeMM(float mm) {
  long steps = (long)lroundf(mm * _stepsPerMM);
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
