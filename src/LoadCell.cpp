#include "LoadCell.h"


//========== Konstruktor ==========//   
LoadCell::LoadCell(const uint8_t dataPin, const uint8_t clockPin): _dataPin(dataPin), _clockPin(clockPin) {
    _scale.begin(_dataPin, _clockPin);
}

//========== Öffentliche Funktions-Implementierungen  ==========//
double LoadCell::getMeanWheight(const uint8_t NUM_SAMPLES){
    static int64_t sumAdc = 0;
    static uint8_t count  = 0;

    static double lastWeight = 0.0;
    static bool hasMean = false;

    if (_scale.is_ready()) {
        long raw = _scale.read();
        sumAdc += raw;
        count++;

        if (count >= NUM_SAMPLES) {
            int64_t meanAdc = sumAdc / count;          // kein uint32_t-cast!
            lastWeight = calcWeight((long)meanAdc);
            hasMean = true;

            sumAdc = 0;
            count  = 0;
            return lastWeight;
        }
    }
    // Falls noch kein neuer Mittelwert fertig ist:
    return hasMean ? lastWeight : 0.0;
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