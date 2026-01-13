#include "HotEnd.h"

//========== Konstruktor ==========//   

HotEnd::HotEnd(const uint8_t gatePin, const uint8_t NTC_Pin, const uint8_t fanPin): _gatePin(gatePin), _NTCPin(NTC_Pin), _fanPin(fanPin){
    pinMode(_gatePin,OUTPUT);
    pinMode(_fanPin,OUTPUT);
    pinMode(_NTCPin,INPUT);
}

//========== Öffentliche Funktions-Implementierungen  ==========//

void HotEnd::setHeaterPwm(uint8_t pwmValue) {
    analogWrite(_gatePin, pwmValue);   
}
   
void HotEnd::setFanPwm(uint8_t pwmValue){
    analogWrite(_fanPin, pwmValue); 
}

float HotEnd::getTemperature() {
    double esp_voltage = getNtcVoltage();
    float v1; //unterer Stützwert, Spannung
    float v2; //oberer Stützwert, Spannung
    
    float t1; //unterer Stützwert, Temperatur
    float t2; //oberer Stützwert, Temperatur

    // Unterhalb/oberhalb des Tabellenbereichs clampen
    if (esp_voltage >= _ntcTable[0].voltageV) {
        //interpoliere mit Steigung zwischen 0ten und 1ten Element
        v1=_ntcTable[0].voltageV;
        v2=_ntcTable[1].voltageV;
        t1=_ntcTable[0].tempC;
        t2=_ntcTable[1].tempC;
    }
    else if (esp_voltage <= _ntcTable[_NTC_TABLE_SIZE - 1].voltageV) {
        //interpoliere mit Steigung zwischen vorletzten und letzten Element
        v1=_ntcTable[_NTC_TABLE_SIZE - 2].voltageV;
        v2=_ntcTable[_NTC_TABLE_SIZE - 1].voltageV;
        t1=_ntcTable[_NTC_TABLE_SIZE - 2].tempC;
        t2=_ntcTable[_NTC_TABLE_SIZE - 1].tempC;
    }
    else{
        bool noch_nicht_gefunden=true;
        for (size_t i = 0; i < _NTC_TABLE_SIZE - 1 &&noch_nicht_gefunden; ++i) {
            v1 = _ntcTable[i].voltageV;
            v2 = _ntcTable[i + 1].voltageV;

            if (esp_voltage <= v1 && esp_voltage >= v2) {
                t1 = _ntcTable[i].tempC;
                t2 = _ntcTable[i + 1].tempC;
                noch_nicht_gefunden=false;
            }
        }
    }

    //lineare Interpolation
    float k=(t1-t2)/(v1-v2);
    float d=t2;
    return k*(esp_voltage-v2)+d;
}

float HotEnd::getPower(uint8_t pwmVal){
    float D = static_cast<float>(pwmVal) / 255.0f;  // oder pwmVal / 255.0f;
    float power = D * _powerHeater;                 // z.B. _powerHeater = 40.0f;
    return power;
}

void HotEnd::piController(float temp, float dt, const float setPoint){
    if (temp > _T_MAX_SAFE) {
        Serial.println("Maximale Temperatur von 290°C überschritten! Heizer AUS.");
        _integralTerm   = 0.0f;
        _controlOutput  = 0.0f;
        setHeaterPwm(0);       
        return;
    }

    if(temp < 0){
        Serial.println("Minimale Temperatur von 0°C darf nicht unterschritten werden! Heizer AUS.");
        _integralTerm   = 0.0f;
        _controlOutput  = 0.0f;
        setHeaterPwm(0);
        return;
    }

    float error =  setPoint - temp;
    // I-Anteil integrieren: Fehler [°C] * dt [s]
    _integralTerm += error * dt;

    // Integral in Ausgangsgrenzen beschneiden
    if (_integralTerm >  _OUT_MAX) _integralTerm =  _OUT_MAX;
    if (_integralTerm < -_OUT_MAX) _integralTerm = -_OUT_MAX;

    // PI-Ausgang
    float u = _Kp * error + _Ki * _integralTerm;

    // Stellgröße begrenzen
    if (u > _OUT_MAX) u = _OUT_MAX;
    if (u < _OUT_MIN) u = _OUT_MIN;
    _controlOutput = u;

    // PWM-Wert (0..255) schreiben
    uint8_t pwmValue = static_cast<uint8_t>(_controlOutput);
    setHeaterPwm(pwmValue);
    Serial.print(">Power:");
    Serial.println(getPower(pwmValue));
}


//========== Private Funktions-Implementierungen  ==========//

//Tabelle mit Stützwerten, direkt {voltage [V], temperature[°C]}
const HotEnd::_NtcPoint HotEnd::_ntcTable[HotEnd::_NTC_TABLE_SIZE] = {
    {2.046000, 102.923943},
    {1.653000, 120.719604},
    {1.240000, 140.233643},
    {0.838000, 163.093643},
    {0.525000, 188.993500},
    {0.414000, 201.788239},
    {0.370000, 208.010605},
    {0.293000, 220.299622},
    {0.245000, 230.233643},
    {0.193000, 243.005783},
    {0.173000, 249.198715},
    {0.139000, 260.301727},
    {0.119000, 269.375000},
    {0.095000, 279.709290},
    {0.084000, 286.803040}
};

double HotEnd::getNtcVoltage() {
    double sum_voltage = 0.0;

    for (uint16_t i = 0; i < _SAMPLE_COUNT; ++i) {
        uint16_t val_raw = analogRead(_NTCPin);

        // ADC-Spannung laut ESP
        double U_out_esp = (static_cast<double>(val_raw) * _ADC_VREF) / _ADC_MAX;

        sum_voltage+=U_out_esp;
    }
    return sum_voltage / _SAMPLE_COUNT; // Mittelwert in Volt
}