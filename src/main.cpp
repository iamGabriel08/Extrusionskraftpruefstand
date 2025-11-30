#include <Arduino.h>
#include "HotEnd.h"

#define NTC_PIN 0
#define HEATER_PIN 2
#define HIGH_PIN 4

HotEnd myHotEnd(HEATER_PIN, NTC_PIN);

void setup() {
  Serial.begin(115400);
  pinMode(HIGH_PIN, OUTPUT);
}

void loop() {
  digitalWrite(HIGH_PIN,HIGH);
  double temp = myHotEnd.getTemperature();
  Serial.println(temp);
  delay(200);
  Serial.print("dsljkhdlf");
  delay(1000);
}

