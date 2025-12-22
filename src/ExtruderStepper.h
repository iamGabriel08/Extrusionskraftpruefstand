#pragma once
#include <Arduino.h>
#include <AccelStepper.h>

class ExtruderStepper {
  public:
    //========== Konstruktor ==========//
    ExtruderStepper(uint8_t stepPin, uint8_t dirPin, uint8_t enPin);

    //========== Funktions-Prototypen  ==========//

    // Initialisiert Pins + aktiviert Treiber + setzt Speed/Accel
    void begin(float maxSpeedStepsPerS = 3000.0f, float accelStepsPerS2 = 3000.0f);

    void extrudeMM(float mm);   // Bewegung planen
    void run();                 // MUSS oft aufgerufen werden
    bool isRunning();           // ohne const (AccelStepper API)
    long distanceToGo();        // Debug/Info
    void enable(bool on = true);
    float stepsPerMM() const { return _stepsPerMM; }

  private:

    //========== Variablen  ==========//
    static constexpr int   _STEPS_PER_REV   = 200;
    static constexpr int   _MICROSTEPPING   = 16;
    static constexpr float _GEAR_RATIO      = 3.0f;
    static constexpr float _MM_PER_REV      = 7.0f;

    static constexpr bool _ENABLE_ACTIVE_LOW = true; // BTT/TMC: meist LOW=enable

    uint8_t _stepPin, _dirPin, _enPin;
    float   _stepsPerMM;

    AccelStepper _stepper;
};
