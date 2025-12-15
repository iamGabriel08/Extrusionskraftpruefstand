#pragma once
#include <Arduino.h>
#include <AccelStepper.h>

class ExtruderStepper {
public:
  ExtruderStepper(uint8_t stepPin, uint8_t dirPin, uint8_t enPin,
                  int stepsPerRev, int microstepping,
                  float gearRatio, float mmPerRev);

  void begin(uint32_t serialBaud = 115200);
  void enable();
  void disable();
  void setMaxSpeed(float stepsPerSecond);
  void setAcceleration(float stepsPerSecond2);
  
  long distanceToGo();   // neu

  void extrudeMM(float mm);
  void run();          // nicht const
  bool isRunning();    // WICHTIG: nicht const (wegen AccelStepper API)

  float getStepsPerMM() const { return _stepsPerMM; }

private:

  uint8_t _stepPin, _dirPin, _enPin;

  int   _stepsPerRev;
  int   _microstepping;
  float _gearRatio;
  float _mmPerRev;
  float _stepsPerMM;

  AccelStepper _stepper; // nicht const
};
