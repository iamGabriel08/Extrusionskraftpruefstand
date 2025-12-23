#pragma once
#include <Arduino.h>
#include <AccelStepper.h>

class ExtruderStepper {
public:
  ExtruderStepper(uint8_t stepPin, uint8_t dirPin, uint8_t enPin);

  // maxSpeedStepsPerS = maximale Schrittfrequenz (Steps/s)
  void begin(float maxSpeedStepsPerS = 3000.0f, float accelStepsPerS2 = 3000.0f);

  // ====== Konstantgeschwindigkeit (empfohlen für “läuft dauerhaft”) ======
  void setFilamentSpeedMmS(float mm_s);   // Vorschub in mm/s (Filament)
  void runSpeed();                        // muss sehr oft aufgerufen werden
  void stop();                            // Geschwindigkeit auf 0

  // ====== Positionsmodus (für definierte Menge) ======
  void extrudeMM(float mm);
  void run();
  bool isRunning();
  long distanceToGo();

  void enable(bool on = true);

  float stepsPerMM() const { return _stepsPerMM; }
  float mmPerStep() const { return 1.0f / _stepsPerMM; }   // “mm/step” ist abgeleitet

    // Gesamt extrudierte Strecke seit Start (mm)
  float getExtrudedMmSinceStart() const;
  long getTimeMsSinceStart() const;
  void  resetExtrudedMm();

private:
  // ===== Mechanik / Kalibrierung =====
  // Hemera Datenblatt: nominal steps/mm bei 1/16 = 397
  static constexpr float _STEPS_PER_MM_X16 = 397.0f;

  // A4988 OHNE Jumper auf MS1..MS3 -> Vollschritt (1)
  // Wenn du wieder auf TMC2209 (typisch 1/16) wechselst -> 16
  static constexpr int   _MICROSTEPPING = 16;

  static constexpr bool  _ENABLE_ACTIVE_LOW = true; // EN low = enable (A4988 meist so)

  int32_t _lastStepPos = 0;     // letzter gemessener Stepper-Positionswert (Steps)
  int64_t _extrudedSteps = 0;   // aufsummierte Steps seit Start
  long _timeStamp = 0;

  uint8_t _stepPin, _dirPin, _enPin;
  float   _stepsPerMM;
  AccelStepper _stepper;
};
