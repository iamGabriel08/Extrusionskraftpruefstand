#ifndef Rotary_Encoder_h
#define Rotary_Encoder_h
#include <Arduino.h>
#include <stdint.h>
#include "driver/pcnt.h"

#define PCNT_UNIT    PCNT_UNIT_0         // Use PCNT unit 0
#define PCNT_CHANNEL PCNT_CHANNEL_0      // Use channel 0 of the unit
#define Counter_limit 32767
#define Pulses_per_mm 6.421

class Encoder{

    public:
    //========== Konstruktor ==========//
    Encoder(const uint8_t dataPin);

    //========== Funktions-Prototypen  ==========//
    uint32_t get_length ();


    private:
    //========== Funktions-Prototypen  ==========//
    uint32_t calc_length(uint32_t totalPulses);
    void pcnt_init();
    static void IRAM_ATTR pcnt_intr_handler(void *arg);

    //========== Variablen  ==========//
    const uint8_t _dataPin;
    static int32_t _overflowCount;

};




#endif