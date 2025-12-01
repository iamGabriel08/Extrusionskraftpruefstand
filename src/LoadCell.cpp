#include "LoadCell.h"


//========== Konstruktor ==========//   
LoadCell::LoadCell(const uint8_t dataPin, const uint8_t clockPin): _dataPin(dataPin), _clockPin(clockPin) {
    _scale.begin(_dataPin, _clockPin);
}

//========== Öffentliche Funktions-Implementierungen  ==========//




//========== Private Funktions-Implementierungen  ==========//