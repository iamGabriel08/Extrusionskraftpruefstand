#ifndef LoadCell_h
#define LoadCell_h
#include <Arduino.h>
#include "HX711.h"
#include <stdint.h>

class LoadCell{

    public:
        LoadCell(const uint8_t dataPin, const uint8_t clockPin);


    private:
        const uint8_t _dataPin;
        const uint8_t _clockPin;
        HX711 _scale;
};

#endif