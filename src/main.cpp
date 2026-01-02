#include <Arduino.h>
#include <stdint.h>
#include "HotEnd.h"
#include "LoadCell.h"
#include "ExtruderStepper.h"
#include "Rotary_Encoder.h"
#include "GUICommunication.h"

// Messung 
#define MEASURE_TIMER_MIN 1.5 // Zeit einer Messung in Minuten

// Stepper 
#define EN_PIN     11
#define STEP_PIN    10
#define DIR_PIN     9

//float extrusion_per_s_in_mm = 0.0f;
//uint32_t extrusion_per_min_in_mm = 300.;
float feed_rate_per_s_in_mm;
float feed_length_in_mm;

// Hot-End
#define NTC_PIN 4
#define HEATER_PIN 6
#define FAN_PIN 7

// Hot-End PI-Regler
#define HEATER_DELAY 100

float heater_temp_target = 200.; //ersetzt HEATER_SET_POINT
float hot_end_abschalten; //in der aktuellen GUI nicht beinhaltet

// Load Cell
#define LOADCELL_DOUT_PIN 8
#define LOADCELL_SCK_PIN 3

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

// Queue für Temperaturwerte (NTC Task -> Hot End Task)
QueueHandle_t tempQueueHandle = NULL; 
#define TEMP_QUEUE_LENGTH 10
#define TEMP_QUEUE_SIZE sizeof(float)

// Zeitstempel für Controller Taks
unsigned long timeStamp = 0;

// Flag falls definierte Temperatur noch nicht erreicht
bool tempReached = false; 

// ====================== Funktionen-Definitionen ======================//
void loadCell_task(void* parameters);
void NTC_task(void* parameters);
void stepper_task(void* parameters);
void hotEnd_task(void* parameters);
void serial_task(void* parameters);
void rotEncoder_task(void* parameters);
void controller_task(void* parameters);
void stopAllActuators(void);



void setup(){
  Serial.begin(115200);
  extruder.begin(3000, 3000);

  // Queues
  tempQueueHandle = xQueueCreate(TEMP_QUEUE_LENGTH, TEMP_QUEUE_SIZE);
  if(tempQueueHandle == NULL) Serial.println("Fehler beim erstellen der Temperatur Queue");

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

  if (xTaskCreatePinnedToCore (serial_task, "Serial Task", 6144, nullptr, 1, &serialTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Serial Task");
  }
  //vTaskSuspend(serialTaskHandle); // Task vorerst anhalten
}

void loop(){

  
  vTaskDelay(pdMS_TO_TICKS(50));
}


// ====================== Funktionen-Implementierungen ======================//

void loadCell_task(void* parameters){
  for(;;){
    float force = myLoadCell.getForce();
    Serial.print(">Force:");
    Serial.println(force, 3);      
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void NTC_task(void* parameters){
  for(;;){
    float temp = myHotEnd.getTemperature(); 
    if(xQueueSend(tempQueueHandle, (void*)&temp, pdMS_TO_TICKS(10)) != pdPASS) Serial.println("Fehler beim Senden der Temperatur");               
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
    float schlupf = (1-(ist/soll))*100;   //schlupf in %
    Serial.print(">Schlupf %:");
    Serial.println(schlupf);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void hotEnd_task(void* parameters){
  myHotEnd.setFanPwm(180);
  for(;;){

    static float temp = 0;
    if(xQueueReceive(tempQueueHandle, &temp, pdMS_TO_TICKS(10)) != pdPASS) Serial.println("Fehler beim Empfangen der Temperatur");
      Serial.print(">temp:");
      Serial.println(temp, 2);   
      
      if (temp >= heater_temp_target) {
          // über oder am Soll: Heizer aus
          myHotEnd.setHeaterPwm(0);
          float power = myHotEnd.getPower(0);
          myHotEnd.setFanPwm(180);
          Serial.print(">Power:");
          Serial.println(power);
      }
      else if (temp >= (heater_temp_target - 20.0f)) {
          // im Band: 20°C unter Soll bis Soll -> halbe Leistung
          myHotEnd.setHeaterPwm(128);
          float power = myHotEnd.getPower(128);
          myHotEnd.setFanPwm(180);
          Serial.print(">Power:");
          Serial.println(power);
      }
      else {
          // weiter als 20°C unter Soll: Vollgas
          myHotEnd.setHeaterPwm(255);
          float power = myHotEnd.getPower(255);
          myHotEnd.setFanPwm(180);
          Serial.print(">Power:");
          Serial.println(power);
      }
      
      if (!tempReached && temp >= heater_temp_target) {
        timeStamp = millis(); // Zeitstempel setzten wenn Messung beginnt
        tempReached = true;                 
        
        // Stepper
        vTaskResume(stepperTaskHandle);
        //extrusion_per_s_in_mm = (float)extrusion_per_min_in_mm / 60.0f;
        extruder.setFilamentSpeedMmS(feed_rate_per_s_in_mm);
        extruder.enable(true);               // WICHTIG!
        extruder.resetExtrudedMm(); // muss aufgerufen werden damit Filamentlängenzählung und Zeitzählung neu beginnt

        // Encoder
        if(!myEncoder.start_counter()) Serial.print("Fehler beim Encoderstart");

        vTaskResume(RotEncoderTaskHandle);
        vTaskResume(loadCellTaskHandle);
        vTaskResume(ControllerTaskHandle);
        vTaskSuspend(serialTaskHandle);
    }

    // Regler
    //const float dt_s = HEATER_DELAY / 1000.0f;  
    //myHotEnd.piController(temp, dt_s,HEATER_SET_POINT);
    vTaskDelay(pdMS_TO_TICKS(HEATER_DELAY));
    }
}

void serial_task(void* parameters){
  for(;;){
    if(my_GUI.get_serial_input(&heater_temp_target, &feed_rate_per_s_in_mm, &feed_length_in_mm)==true){

      //printe Daten (zum Debuggen)
      Serial.println("Empfangene Daten:");
      Serial.println("Temperatur");
      Serial.println(heater_temp_target);
      Serial.println("FeedRate");
      Serial.println(feed_rate_per_s_in_mm);
      Serial.println("FeedLength");
      Serial.println(feed_length_in_mm);
      // Vorbereitung für neue Messung
      tempReached = false;                 // WICHTIG!
      xQueueReset(tempQueueHandle);        // optional, aber sauber

      // Heizen starten
      vTaskResume(NTCTaskHandle);
      vTaskResume(hotEndTaskHandle);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void controller_task(void* parameters){
  for(;;){
    static unsigned long lastPrint = 0;
    if (millis() - lastPrint >= 1000) {
      lastPrint = millis();
      Serial.print("Time:");
      Serial.println(millis() / 1000);
    }

    // Timestamp wird im Hot End Task gesetzt sobald Soll Temperatur erreicht 
    if (timeStamp != 0 && (millis() - timeStamp >= MEASURE_TIMER_MIN * 60UL * 1000UL)) {
      timeStamp = 0;
      tempReached = false;
      stopAllActuators();
      Serial.println("Die Messung ist abgeschlossen");

      // Encoder reseten
      if(!myEncoder.reset()) Serial.println("Fehler beim reseten des Encoders");

      // Alle Sensoren und Aktoren abschalten
      vTaskSuspend(stepperTaskHandle);
      vTaskSuspend(hotEndTaskHandle);
      vTaskSuspend(loadCellTaskHandle);
      vTaskSuspend(NTCTaskHandle);
      vTaskSuspend(RotEncoderTaskHandle);

      // Serial Taks wieder wecken
      vTaskResume(serialTaskHandle);

      // Supsendet Controller Task
      vTaskSuspend(NULL); 
    }
    vTaskDelay(pdMS_TO_TICKS(40));
  }
}

void stopAllActuators(void){
  myHotEnd.setFanPwm(0);
  myHotEnd.setHeaterPwm(0);
  extruder.stop();
  extruder.enable(false);
}
