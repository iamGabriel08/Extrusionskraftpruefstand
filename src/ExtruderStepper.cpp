#include "ExtruderStepper.h"
#include <math.h>

ExtruderStepper::ExtruderStepper(uint8_t stepPin, uint8_t dirPin, uint8_t enPin)
: _stepPin(stepPin),
  _dirPin(dirPin),
  _enPin(enPin),
  _stepsPerMM(_STEPS_PER_MM_X16 * ((float)_MICROSTEPPING / 16.0f)),
  _stepper(AccelStepper::DRIVER, stepPin, dirPin)
{}

void ExtruderStepper::begin(float maxSpeedStepsPerS, float accelStepsPerS2) {
  pinMode(_stepPin, OUTPUT);
  pinMode(_dirPin,  OUTPUT);
  pinMode(_enPin,   OUTPUT);
  enable(true);

  // A4988 mag saubere STEP-Pulse
  _stepper.setMinPulseWidth(2);
  _stepper.setPinsInverted(true);
  _stepper.setMaxSpeed(maxSpeedStepsPerS);
  _stepper.setAcceleration(accelStepsPerS2);

  Serial.print("Extruder microstepping = 1/");
  Serial.println(_MICROSTEPPING);

  Serial.print("Extruder stepsPerMM = ");
  Serial.println(_stepsPerMM, 4);

  _lastStepPos = _stepper.currentPosition();
  _extrudedSteps = 0;
  _timeStamp = millis();
}

void ExtruderStepper::enable(bool on) {
  if (_ENABLE_ACTIVE_LOW) digitalWrite(_enPin, on ? LOW : HIGH);
  else                   digitalWrite(_enPin, on ? HIGH : LOW);
}

// -------- Konstantgeschwindigkeit ----------
void ExtruderStepper::setFilamentSpeedMmS(float mm_s) {
  float steps_s = mm_s * _stepsPerMM;              // mm/s -> steps/s
  float maxS    = _stepper.maxSpeed();             // gesetztes Limit

  if (fabsf(steps_s) > maxS) {                     // clamp
    steps_s = (steps_s > 0) ? maxS : -maxS;
  }
  _stepper.setSpeed(steps_s);
}

void ExtruderStepper::stop() {
  _stepper.setSpeed(0);
}

// -------- Positionsmodus (falls du weiterhin “Menge” willst) ----------
void ExtruderStepper::extrudeMM(float mm) {
  long steps = lroundf(mm * _stepsPerMM);
  _stepper.move(steps);
}

bool ExtruderStepper::isRunning() {
  return _stepper.distanceToGo() != 0;
}

long ExtruderStepper::distanceToGo() {
  return _stepper.distanceToGo();
}

void ExtruderStepper::run() {
  _stepper.run();

  int32_t now = _stepper.currentPosition();
  _extrudedSteps += (int64_t)(now - _lastStepPos);
  _lastStepPos = now;
}

void ExtruderStepper::runSpeed() {
  _stepper.runSpeed();

  int32_t now = _stepper.currentPosition();
  _extrudedSteps += (int64_t)(now - _lastStepPos);
  _lastStepPos = now;
}

float ExtruderStepper::getExtrudedMmSinceStart() const {
  return (float)_extrudedSteps / _stepsPerMM;
}

long ExtruderStepper::getTimeMsSinceStart() const {
  return millis() - _timeStamp;
}

void ExtruderStepper::resetExtrudedMm() {
  _extrudedSteps = 0;
  _lastStepPos = _stepper.currentPosition();
  _timeStamp = millis();              // <<< WICHTIG
}
