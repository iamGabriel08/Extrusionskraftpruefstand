#include "GUICommunication.h"

//========== Konstruktor ==========//

GUICom::GUICom(){

};

//========== Öffentliche Funktions-Implementierungen  ==========//

bool GUICom::get_serial_input(float* temp_data, float* feedrate, float* feedlength){
    if(Serial.available()>0){ //falls input von GUI vorhanden
        String receivedData = Serial.readStringUntil('\n');

        mess_parameter_string meine_parameter;
        split_string(receivedData, &meine_parameter);
        
        *temp_data=string_to_float(meine_parameter.temp_string);
        *feedrate=string_to_float(meine_parameter.feedrate_string);
        *feedlength=string_to_float(meine_parameter.feedlength_string);
        return true;
    }else{
        return false;
    }
};

//========== Private Funktions-Implementierungen  ==========//

float GUICom::string_to_float(String text){
  int length=text.length();
  int point_pos=1;
  while(point_pos<length && text[point_pos]!='.'){
    point_pos++;
  }
  String vorKomma=text.substring(0,point_pos);
  int factor=1;
  float val=0.;
  for(int i=0; i<vorKomma.length(); i++){
    val+=(vorKomma[vorKomma.length()-i-1]-'0')*factor;
    factor*=10;
  }

  if(point_pos<length){
    String nachKomma=text.substring(point_pos+1);
    float factor=10.;
    for(int i=0; i<nachKomma.length(); i++){
      val+=(nachKomma[i]-'0')/factor;
      factor*=10.;
    }
  }
  return val;
}


void GUICom::split_string(String raw_string, mess_parameter_string* pStruct){
  int index=raw_string.indexOf(' ');
  pStruct->temp_string=raw_string.substring(0,index);
  raw_string=raw_string.substring(index+1);

  index=raw_string.indexOf(' ');
  pStruct->feedrate_string=raw_string.substring(0,index);
  raw_string=raw_string.substring(index+1);

  index=raw_string.indexOf(' ');
  pStruct->feedlength_string=raw_string.substring(0,index);
  /*
  raw_string=raw_string.substring(index+1);

  index=raw_string.indexOf(' ');
  pStruct->abschalten_string=raw_string.substring(0,index);
  */
}