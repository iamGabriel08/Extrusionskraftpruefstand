#include <Arduino.h>
#include <stdint.h>
#include "HotEnd.h"
#include "LoadCell.h"
#include "ExtruderStepper.h"
#include "Rotary_Encoder.h"
#include "GUICommunication.h"

// Stepper 
#define EN_PIN     11
#define STEP_PIN    10
#define DIR_PIN     9

float feed_rate_per_s_in_mm = 5;
float feed_length_in_mm = 50;

// Messzeit in ms (wird durch GUI Parameter berechnet)
unsigned long measureTimeMS = 0;

// Hot-End
#define NTC_PIN 4
#define HEATER_PIN 6
#define FAN_PIN 7

// Hot-End PI-Regler
#define HEATER_DELAY 100

float heater_temp_target = 180.; //ersetzt HEATER_SET_POINT
uint8_t hot_end_abschalten=0; 

// Load Cell
#define LOADCELL_DOUT_PIN 8
#define LOADCELL_SCK_PIN 3
uint8_t tare=0; //1, falls genullt werden soll

// Rotary Encoder
#define ROTARY_ENCODER_PIN 5

//========== Objekte ==========//
HotEnd myHotEnd(HEATER_PIN, NTC_PIN, FAN_PIN);
LoadCell myLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
ExtruderStepper extruder(STEP_PIN, DIR_PIN, EN_PIN);
Encoder myEncoder(ROTARY_ENCODER_PIN);
GUICom my_GUI;

//========== Tasks ==========//
TaskHandle_t loadCellTaskHandle = NULL;
TaskHandle_t NTCTaskHandle = NULL;
TaskHandle_t stepperTaskHandle =  NULL;
TaskHandle_t hotEndTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;
TaskHandle_t RotEncoderTaskHandle = NULL;
TaskHandle_t ControllerTaskHandle = NULL;
TaskHandle_t TelemetryTaskHandle = NULL;

// Queue für Temperaturwerte (NTC Task -> Hot End Task)
QueueHandle_t tempHotEndQueueHandle = NULL; 
#define TEMP_QUEUE_LENGTH 10
#define TEMP_QUEUE_SIZE sizeof(float)
// Queue für Temperaturwerte (NTC Task -> Telemetry Task) --> nutz die Selben Makros wie tempHotEndQueue
QueueHandle_t tempTelemetryQueueHandle = NULL; 


// Queue für Kraftwerte (LoadCell Task -> Telemetry Task)
QueueHandle_t ForceQueueHandle = NULL; 
#define FORCE_QUEUE_LENGTH 10
#define FORCE_QUEUE_SIZE sizeof(float)

// Queue für Kraftwerte (RotEncoder Task -> Telemetry Task)
QueueHandle_t SlipQueueHandle = NULL; 
#define SLIP_QUEUE_LENGTH 10
#define SLIP_QUEUE_SIZE sizeof(float)

// Zeitstempel für Controller Taks
unsigned long timeStamp = 0;

// Flag falls definierte Temperatur noch nicht erreicht
volatile bool tempReached = false; 

// Flag das die Messung läuft
volatile bool isMeasuring = false;

// ====================== Funktionen-Definitionen ======================//
void loadCell_task(void* parameters);
void NTC_task(void* parameters);
void stepper_task(void* parameters);
void hotEnd_task(void* parameters);
void serial_task(void* parameters);
void rotEncoder_task(void* parameters);
void controller_task(void* parameters);
void Telemetry_Task(void* parameters);

void stopAllActuators(void);
bool computeMeasureTime();
void tareLoadCell();

void setup(){
  Serial.begin(115200);
  extruder.begin(3000, 3000);

  // Queues
  tempHotEndQueueHandle = xQueueCreate(TEMP_QUEUE_LENGTH, TEMP_QUEUE_SIZE);
  if(tempHotEndQueueHandle == NULL) Serial.println("Fehler beim erstellen der Temperatur Queue fürs Hotend");

  tempTelemetryQueueHandle = xQueueCreate(TEMP_QUEUE_LENGTH, TEMP_QUEUE_SIZE);
  if(tempTelemetryQueueHandle == NULL) Serial.println("Fehler beim erstellen der Temperatur Queue für die Telemetry");

  ForceQueueHandle = xQueueCreate(FORCE_QUEUE_LENGTH, FORCE_QUEUE_SIZE);
  if(ForceQueueHandle == NULL) Serial.println("Fehler beim erstellen der Kraft Queue für die Telemetry");

  SlipQueueHandle = xQueueCreate(SLIP_QUEUE_LENGTH, SLIP_QUEUE_SIZE);
  if(SlipQueueHandle == NULL) Serial.println("Fehler beim erstellen der Slip Queue für die Telemetry");

  // Tasks
  if (xTaskCreatePinnedToCore (loadCell_task, "Load Cell Task", 6144, nullptr, 1, &loadCellTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Load Cell Task");
  }
  vTaskSuspend(loadCellTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (NTC_task, "NTC Task", 6144, nullptr, 1, &NTCTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von NTC Task");
  }
  vTaskSuspend(NTCTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (stepper_task, "Stepper Task", 6144, nullptr, 1, &stepperTaskHandle, 1) != pdPASS) {
    Serial.println("Fehler beim erstellen von Stepper Task");
  }
  vTaskSuspend(stepperTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (hotEnd_task, "Hot End Task", 6144, nullptr, 1, &hotEndTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Hot End Task");
  }
  vTaskSuspend(hotEndTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (rotEncoder_task, "Rotary Encoder Task", 6144, nullptr, 1, &RotEncoderTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Rotary Encoder Task");
  }
  vTaskSuspend(RotEncoderTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (controller_task, "Controller Task", 6144, nullptr, 1, &ControllerTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Controller Task");
  }
  vTaskSuspend(ControllerTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (Telemetry_Task, "Telemetry Task", 6144, nullptr, 1, &TelemetryTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Controller Task");
  }
  vTaskSuspend(TelemetryTaskHandle);

  if (xTaskCreatePinnedToCore (serial_task, "Serial Task", 6144, nullptr, 1, &serialTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Serial Task");
  }
  //vTaskSuspend(serialTaskHandle); // Task vorerst anhalten
  
}

void loop(){

  
  vTaskDelay(pdMS_TO_TICKS(50));
}

// ====================== Funktionen-Implementierungen ======================//

// Task Funktionen
void loadCell_task(void* parameters){
  for(;;){
    float force = myLoadCell.getForce();
    if(isMeasuring){
      float dummy;
      if(xQueueSend(ForceQueueHandle, (void*)&force, 0) != pdPASS){
        xQueueReceive(ForceQueueHandle, (void*)&dummy,0);
        xQueueSend(ForceQueueHandle, (void*)&force, 0);
      }
      
    }
       
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void NTC_task(void* parameters){
  for(;;){
    float temp = myHotEnd.getTemperature(); 
    float dummy;
    if(xQueueSend(tempHotEndQueueHandle, (void*)&temp, 0) != pdPASS){
        xQueueReceive(tempHotEndQueueHandle, (void*)&dummy,0);
        xQueueSend(tempHotEndQueueHandle, (void*)&temp, 0);
      }

      if(xQueueSend(tempTelemetryQueueHandle, (void*)&temp, 0) != pdPASS){
        xQueueReceive(tempTelemetryQueueHandle, (void*)&dummy,0);
        xQueueSend(tempTelemetryQueueHandle, (void*)&temp, 0);
      }    
    vTaskDelay(pdMS_TO_TICKS(100));           
  }
}

void stepper_task(void*) {
  for(;;){
    extruder.runSpeed();
    taskYIELD();  // sehr kurz abgeben, ohne 1ms-Schlaf
  }
}

void rotEncoder_task(void* parameters){
  for(;;){
    
    float ist = myEncoder.get_length();
    float soll = extruder.getExtrudedMmSinceStart();
    float dummy;
    if(isMeasuring){
       if (soll > 0.001f) {
      float schlupf = (1.0f - (ist / soll)) * 100.0f; //schlupf in %
      if(xQueueSend(SlipQueueHandle, (void*)&schlupf, 0) != pdPASS){
        xQueueReceive(SlipQueueHandle, (void*)&dummy,0);
        xQueueSend(SlipQueueHandle, (void*)&schlupf, 0);
      }
      } 
      else {
        float schlupf_0 = 0;
        if(xQueueSend(SlipQueueHandle, (void*)&schlupf_0, 0) != pdPASS){
        xQueueReceive(SlipQueueHandle, (void*)&dummy,0);
        xQueueSend(SlipQueueHandle, (void*)&schlupf_0, 0);
      }
      }
    } 
    vTaskDelay(pdMS_TO_TICKS(100));
  }
   
}

void hotEnd_task(void* parameters){
  myHotEnd.setFanPwm(180);
  for(;;){

    static float temp = 0;
    while(xQueueReceive(tempHotEndQueueHandle, &temp, 0) == pdPASS){
      //Queue leeren bis zum neuesten element
    }
    
      if (temp >= heater_temp_target) {
          // über oder am Soll: Heizer aus
          myHotEnd.setHeaterPwm(0);
          myHotEnd.setFanPwm(180);
      }
      else if (temp >= (heater_temp_target - 20.0f)) {
          // im Band: 20°C unter Soll bis Soll -> halbe Leistung
          myHotEnd.setHeaterPwm(128);
          myHotEnd.setFanPwm(180);
      }
      else {
          // weiter als 20°C unter Soll: Vollgas
          myHotEnd.setHeaterPwm(255);
          myHotEnd.setFanPwm(180);
      }
      
      if (!tempReached && temp >= heater_temp_target) {
        timeStamp = millis(); // Zeitstempel setzten wenn Messung beginnt
        tempReached = true;                 
        isMeasuring = true;
        // Stepper
        extruder.resetExtrudedMm();
        extruder.enable(true);
        extruder.setFilamentSpeedMmS(feed_rate_per_s_in_mm);
        vTaskResume(stepperTaskHandle);

        // Encoder
        if(!myEncoder.reset()) Serial.println("Fehler beim reseten des Encoders");
        if(!myEncoder.start_counter()) Serial.print("Fehler beim Encoderstart");

        vTaskResume(RotEncoderTaskHandle);
        vTaskResume(loadCellTaskHandle);
        vTaskResume(ControllerTaskHandle);
        vTaskSuspend(serialTaskHandle);
        Serial.println("begin"); //meldet der GUI, dass Aufheizen zuende und Messung beginnt
    }
    // Regler
    //const float dt_s = HEATER_DELAY / 1000.0f;  
    //myHotEnd.piController(temp, dt_s,HEATER_SET_POINT);
    vTaskDelay(pdMS_TO_TICKS(HEATER_DELAY));
    }
}

void serial_task(void* parameters){
  for(;;){
    if(my_GUI.get_serial_input(&heater_temp_target, &feed_rate_per_s_in_mm, &feed_length_in_mm, &hot_end_abschalten, &tare)==true){

      //printe Daten (zum Debuggen)
      Serial.println("Empfangene Daten:");
      Serial.println("Temperatur");
      Serial.println(heater_temp_target);
      Serial.println("FeedRate");
      Serial.println(feed_rate_per_s_in_mm);
      Serial.println("FeedLength");
      Serial.println(feed_length_in_mm);
      Serial.println("Abschalten?");
      Serial.println(hot_end_abschalten);
      // Vorbereitung für neue Messung
      tempReached = false;                 // WICHTIG!

      xQueueReset(tempTelemetryQueueHandle);
      xQueueReset(ForceQueueHandle);
      xQueueReset(SlipQueueHandle);
      xQueueReset(tempHotEndQueueHandle);        // optional, aber sauber

      if (!computeMeasureTime()) {
        Serial.println("Fehler beim Berechnen der Messzeit");
        continue;
      }
      Serial.print("Measure Zeit:");
      Serial.println(measureTimeMS);
      // Heizen starten
      vTaskResume(TelemetryTaskHandle);
      vTaskResume(NTCTaskHandle);
      vTaskResume(hotEndTaskHandle);
    }else{
      if(tare == 1){
       tareLoadCell();
      }
      //Serial.print("Tare=");
      //Serial.println(tare);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void controller_task(void* parameters){
  for(;;){
    if (timeStamp != 0 && measureTimeMS != 0 && (millis() - timeStamp >= measureTimeMS)) {
    timeStamp = 0;
    measureTimeMS = 0;
    tempReached = false;
    isMeasuring = false;
    stopAllActuators();
    Serial.println("end"); //teilt der GUI mit, dass die Messung zu Ende ist

    if(!myEncoder.reset()) Serial.println("Fehler beim reseten des Encoders");

    vTaskSuspend(stepperTaskHandle);
    vTaskSuspend(hotEndTaskHandle);
    vTaskSuspend(loadCellTaskHandle);
    vTaskSuspend(NTCTaskHandle);
    vTaskSuspend(RotEncoderTaskHandle);
    vTaskSuspend(TelemetryTaskHandle);
    vTaskResume(serialTaskHandle);
    vTaskSuspend(NULL);
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}
  
void Telemetry_Task(void* parameters){
  for(;;){
    float temp = NAN, force = NAN, slip = NAN;
    unsigned long time = millis();
    while(xQueueReceive(tempTelemetryQueueHandle, &temp, 0) == pdPASS){
      //Queue leeren bis zum neuesten element
    }
    while(xQueueReceive(ForceQueueHandle, &force, 0) == pdPASS){
      //Queue leeren bis zum neuesten element
    }
    while(xQueueReceive(SlipQueueHandle, &slip, 0) == pdPASS){
      //Queue leeren bis zum neuesten element
    }

    Serial.print("f");
    Serial.println(force, 3);
    Serial.print("s");
    Serial.println(slip, 2);
    Serial.print("c");
    Serial.println(temp, 2);  
    Serial.print("t");
    Serial.println(time);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// Helfer Funktionen
void stopAllActuators(void){
  myHotEnd.setFanPwm(0);
  myHotEnd.setHeaterPwm(0);
  extruder.stop();
  extruder.enable(false);
}

bool computeMeasureTime(){ 
  float t_s = feed_length_in_mm / feed_rate_per_s_in_mm;  // Sekunden
  measureTimeMS = (unsigned long)(t_s * 1000.0f);    // Millisekunden

  Serial.print("MeasureTimeMS=");
  Serial.println(measureTimeMS);
  return true;
}

void tareLoadCell(){
  
  vTaskSuspend(loadCellTaskHandle);

  if (!myLoadCell.tare(20)) {
    Serial.println("Tare fehlgeschlagen (HX711 nicht ready?)");
  } else {
    Serial.println("Tare OK -> Force/Weight jetzt relativ zu Nullpunkt");
  }

  vTaskResume(loadCellTaskHandle);

  tare = 0; // optional: verhindert, dass du bei gleichem Flag dauernd neu tarest

}