#include "LoadCell.h"


//========== Konstruktor ==========//   
LoadCell::LoadCell(const uint8_t dataPin, const uint8_t clockPin): _dataPin(dataPin), _clockPin(clockPin) {
    _scale.begin(_dataPin, _clockPin);
}

//========== Öffentliche Funktions-Implementierungen  ==========//
double LoadCell::getMeanWheight(const uint8_t NUM_SAMPLES){
    static long sumAdc  = 0;
    static uint8_t count = 0;
    static long meanAdc = 0;      // letzter gültiger Mittelwert (ADC)
    static bool hasMean = false;  // gibt es schon einen gültigen Mittelwert?

    if (_scale.is_ready()) { 
        long raw = _scale.read();   // HX711-Lib liefert long (signed)
        sumAdc += raw;
        count++;

        if (count >= NUM_SAMPLES) {
            meanAdc = sumAdc / count;   // neuer Mittelwert

            double weight = calcWeight(meanAdc);

            Serial.print("ADC mean: ");
            Serial.println(meanAdc);

            Serial.print("Gewicht = ");
            Serial.print(weight);
            Serial.println(" g");

            // Zurücksetzen für nächste Mittelungsrunde
            sumAdc = 0;
            count  = 0;
            hasMean = true;

            return weight;  // neuer gültiger Messwert
        }
    } 
    else {
        Serial.println("HX711 not found.");
    }

    // Hier landen wir, wenn noch kein neues Mittel fertig ist
    if (hasMean) {
        // letzten gültigen Mittelwert in g zurückgeben
        return calcWeight(meanAdc);
    } else {
        // Ganz am Anfang: noch nie ein Mittelwert → irgendein Default
        return 0.0;
    }
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