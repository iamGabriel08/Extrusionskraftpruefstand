#include "HotEnd.h"

//========== Konstruktor ==========//   

HotEnd::HotEnd(const uint8_t gatePin, const uint8_t NTC_Pin): _gatePin(gatePin), _NTCPin(NTC_Pin){
    pinMode(_gatePin,OUTPUT);
    pinMode(_NTCPin,INPUT);
}

//========== Öffentliche Funktions-Implementierungen  ==========//

void HotEnd::setHeaterPwm(uint8_t pwmValue) {
    analogWrite(_gatePin, pwmValue);   
}
   
double HotEnd::getNtcResistance() {
    double sum_res = 0.0;

    for (uint16_t i = 0; i < SAMPLE_COUNT; ++i) {
        uint16_t val_raw = analogRead(_NTCPin);

        // ADC-Spannung laut ESP
        double U_out_esp = (static_cast<double>(val_raw) * ADC_VREF) / ADC_MAX;

        //  Korrektur
        double U_out = U_out_esp * (K_CORR * U_out_esp + D_CORR); 

        // Spannungsteiler-Formel: U_out = U_b * R_ntc / (R_fixed + R_ntc)
        // => R_ntc = R_fixed * U_out / (U_b - U_out)
        double res_ntc = (R_FIXED * U_out) / (ADC_VREF - U_out);
        sum_res += res_ntc;
        delay(SAMPLE_DELAY_MS);
    }
    return sum_res / SAMPLE_COUNT; // Mittelwert in Ohm
}

float HotEnd::getTemperature() {
    double resOhm = getNtcResistance();
    float rKOhm = static_cast<float>(resOhm / 1000.0); // Ohm -> kΩ
    return temperatureFromResistance(rKOhm);
}

//========== Private Funktions-Implementierungen  ==========//

// NTC 104NT-4-R025H42G
// Tabelle: { temperature [°C], resistance [kΩ] }
const HotEnd::NtcPoint HotEnd::_ntcTable[HotEnd::NTC_TABLE_SIZE] = {
    {   0.0f, 354.6f  },
    {   5.0f, 270.8f  },
    {  10.0f, 208.8f  },
    {  15.0f, 162.1f  },
    {  20.0f, 126.9f  },
    {  25.0f, 100.0f  },
    {  30.0f,  79.33f },
    {  35.0f,  63.32f },
    {  40.0f,  50.90f },
    {  45.0f,  41.13f },
    {  50.0f,  33.45f },
    {  55.0f,  27.34f },
    {  60.0f,  22.48f },
    {  65.0f,  18.57f },
    {  70.0f,  15.43f },
    {  75.0f,  12.88f },
    {  80.0f,  10.80f },
    {  85.0f,   9.094f },
    {  90.0f,   7.690f },
    {  95.0f,   6.530f },
    { 100.0f,   5.569f },
    { 105.0f,   4.767f },
    { 110.0f,   4.097f },
    { 115.0f,   3.533f },
    { 120.0f,   3.058f },
    { 125.0f,   2.655f },
    { 130.0f,   2.313f },
    { 135.0f,   2.020f },
    { 140.0f,   1.770f },
    { 145.0f,   1.556f },
    { 150.0f,   1.371f },
    { 155.0f,   1.212f },
    { 160.0f,   1.074f },
    { 165.0f,   0.9544f },
    { 170.0f,   0.8501f },
    { 175.0f,   0.7590f },
    { 180.0f,   0.6793f },
    { 185.0f,   0.6092f },
    { 190.0f,   0.5476f },
    { 195.0f,   0.4932f },
    { 200.0f,   0.4452f },
    { 205.0f,   0.4027f },
    { 210.0f,   0.3650f },
    { 215.0f,   0.3314f },
    { 220.0f,   0.3016f },
    { 225.0f,   0.2749f },
    { 230.0f,   0.2510f },
    { 235.0f,   0.2296f },
    { 240.0f,   0.2104f },
    { 245.0f,   0.1931f },
    { 250.0f,   0.1775f },
    { 255.0f,   0.1634f },
    { 260.0f,   0.1507f },
    { 265.0f,   0.1391f },
    { 270.0f,   0.1287f },
    { 275.0f,   0.1191f },
    { 280.0f,   0.1105f },
    { 285.0f,   0.1026f },
    { 290.0f,   0.09539f },
    { 295.0f,   0.08881f },
    { 300.0f,   0.08278f }
};

// Interpolation der Temperatur aus Tabelle
float HotEnd::temperatureFromResistance(float rKOhm) {
    // Unterhalb/oberhalb des Tabellenbereichs clampen
    if (rKOhm >= _ntcTable[0].resKOhm) {
        return _ntcTable[0].tempC;     // kälter als 0°C
    }
    if (rKOhm <= _ntcTable[NTC_TABLE_SIZE - 1].resKOhm) {
        return _ntcTable[NTC_TABLE_SIZE - 1].tempC; // heißer als 300°C
    }
    // Widerstand nimmt mit Temperatur ab -> wir suchen das Intervall
    for (size_t i = 0; i < NTC_TABLE_SIZE - 1; ++i) {
        float r1 = _ntcTable[i].resKOhm;
        float r2 = _ntcTable[i + 1].resKOhm;

        if (rKOhm <= r1 && rKOhm >= r2) {
            float t1 = _ntcTable[i].tempC;
            float t2 = _ntcTable[i + 1].tempC;

            // lineare Interpolation in R
            float alpha = (rKOhm - r1) / (r2 - r1); // r2 < r1 -> Nenner negativ, passt
            return t1 + alpha * (t2 - t1);
        }
    }
    // sollte nie erreicht werden
    return _ntcTable[NTC_TABLE_SIZE - 1].tempC;
}