#include "LoadCell.h"


//========== Konstruktor ==========//   
LoadCell::LoadCell(const uint8_t dataPin, const uint8_t clockPin): _dataPin(dataPin), _clockPin(clockPin) {
    _scale.begin(_dataPin, _clockPin);
}

//========== Öffentliche Funktions-Implementierungen  ==========//
double LoadCell::getMeanWheight(const uint8_t NUM_SAMPLES){
    static int64_t sumAdc = 0;
    static uint8_t count  = 0;

    if (_scale.is_ready()) {
        long raw = _scale.read();
        sumAdc += raw;
        count++;

        if (count >= NUM_SAMPLES) {
            uint32_t meanAdc = (uint32_t)sumAdc / count;
            double weight = calcWeight(meanAdc);
            sumAdc = 0;
            count  = 0;
            return weight;
        }
    } else {
        Serial.println("HX711 not found.");
    }

    // Wenn kein neuer Mittelwert fertig ist, gib den letzten Wert oder 0 zurück
}


double LoadCell::getRawWheight(){

    static double lastWeight = 0.0;  // letzter gültiger Gewichtswert
    static bool hasRaw = false;      // gibt es schon einen gültigen Rohwert?

    if (_scale.is_ready()) { 
        long raw = _scale.read();    // HX711-Lib liefert long (signed)
        double weight = calcWeight(raw);

        lastWeight = weight;         
        hasRaw = true;

        return weight;               // neuer gültiger Messwert
    }
    else {
        Serial.println("HX711 not found.");
    }

    // Hier landen wir, wenn gerade kein neuer Wert gelesen werden konnte
    if (hasRaw) {
        // letzten gültigen Wert zurückgeben
        return lastWeight;
    } else {
        // Ganz am Anfang: noch nie ein gültiger Wert -> Default
        return 0.0;
    }
}


//========== Private Funktions-Implementierungen  ==========//

double LoadCell::calcWeight(long analogVal){
  return 0.010008  * analogVal - 1838.82;
}