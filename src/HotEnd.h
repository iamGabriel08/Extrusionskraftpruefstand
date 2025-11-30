#ifndef HotEnd_h
#define HotEnd_h
#include <Arduino.h>
#include <stdint.h>

class HotEnd{

    public:
        //========== Konstruktor ==========//   
        HotEnd(const uint8_t gatePin, const uint8_t NTC_Pin);

        //========== Funktions-Prototypen  ==========//

        // Heizer ansteuern (0..255)
        void setHeaterPwm(uint8_t pwmValue);

        // NTC-Widerstand in Ohm (gemittelt)
        double getNtcResistance(void);
        
        // Temperatur in °C (aus Tabelle + Interpolation)
        float getTemperature();

    private:

        //========== Funktions-Prototypen  ==========//

        // aus Widerstand (kΩ) Temperatur interpolieren
        static float temperatureFromResistance(float rKOhm); 

        //========== Variablen  ==========//

        // Heizelement
        const uint8_t _gatePin;
        
        // Thermistor
        const uint8_t _NTCPin;
    
        static constexpr double ADC_VREF   = 3.3;      // V
        static constexpr uint16_t ADC_MAX  = 4095;     // 12-bit Auflösung

        static constexpr double K_CORR = -0.04346;
        static constexpr double D_CORR =  1.1333;
        static constexpr double R_FIXED = 2500.0;      // Ohm, Festwiderstand
        
        static constexpr uint16_t SAMPLE_COUNT = 10;
        static constexpr uint16_t SAMPLE_DELAY_MS = 50;

        struct NtcPoint {
            float tempC;    // °C
            float resKOhm;  // kΩ
        };

        static constexpr size_t NTC_TABLE_SIZE = 61;
        static const NtcPoint _ntcTable[NTC_TABLE_SIZE];

};

#endif