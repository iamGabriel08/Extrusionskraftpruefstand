#ifndef GUICommunication_h
#define GUICommunication_h
#include <Arduino.h>


class GUICom{
    
    public:

    //========== Konstruktor ==========// 
    GUICom(); //keine Pins zu übergeben

    //========== Funktions-Prototypen  ==========//

    //überprüft, ob es input von der GUI gibt und schreibt diesen ggf. in die Variablen
    bool get_serial_input(float* temp, float* feedrate, float* feedlength);


    private:

    typedef struct{
        String temp_string;
        String feedrate_string;
        String feedlength_string;
        String abschalten_string;
    }mess_parameter_string;


    //========== Funktions-Prototypen  ==========//

    //konvertiert Zahlen-String zu integer
    float string_to_float(String text);

    //spaltet empfangenen String in Teilstrings aus Zahlen auf
    void split_string(String raw_string, mess_parameter_string* pStruct);

};
#endif