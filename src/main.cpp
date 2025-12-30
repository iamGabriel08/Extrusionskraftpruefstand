#include <Arduino.h>
#include <stdint.h>
#include "HotEnd.h"
#include "LoadCell.h"
#include "ExtruderStepper.h"
#include "Rotary_Encoder.h"

// Stepper 
#define EN_PIN     11
#define STEP_PIN    10
#define DIR_PIN     9

// Hot-End
#define NTC_PIN 4
#define HEATER_PIN 6
#define FAN_PIN 7

// Hot-End PI-Regler
#define HEATER_DELAY 100
#define HEATER_SET_POINT 220

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

//========== Tasks ==========//
TaskHandle_t loadCellTaskHandle = NULL;
TaskHandle_t NTCTaskHandle = NULL;
TaskHandle_t stepperTaskHandle =  NULL;
TaskHandle_t hotEndTaskHandle = NULL;
TaskHandle_t serialTaskHandle = NULL;

// Queue für Temperaturwerte
QueueHandle_t tempQueueHandle = NULL; 
#define TEMP_QUEUE_LENGTH 20
#define TEMP_QUEUE_SIZE sizeof(float)

//========== Variablen ==========//
volatile bool measureFlag = false;
volatile bool tempReached = false;

// ====================== Funktionen-Definitionen ======================//
void loadCell_task(void* parameters);
void NTC_task(void* parameters);
void stepper_task(void* parameters);
void hotEnd_task(void* parameters);
void serial_task(void* parameters);

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
  //vTaskSuspend(loadCellTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (NTC_task, "NTC Task", 6144, nullptr, 1, &NTCTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von NTC Task");
  }
  // vTaskSuspend(NTCTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (stepper_task, "Stepper Task", 6144, nullptr, 1, &stepperTaskHandle, 1) != pdPASS) {
    Serial.println("Fehler beim erstellen von Stepper Task");
  }
  vTaskSuspend(stepperTaskHandle); // Task vorerst anhalten

  if (xTaskCreatePinnedToCore (hotEnd_task, "Hot End Task", 6144, nullptr, 1, &hotEndTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Hot End Task");
  }
  //vTaskSuspend(hotEndTaskHandle); // Task vorerst anhalten

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
    float wheight = myLoadCell.getMeanWheight(4);
    Serial.print(">wheight:");
    Serial.println(wheight, 3);      
    float length = myEncoder.get_length();
    Serial.print(">length:");
    Serial.println(length);
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
  // Beispiel: konstanter Filamentvorschub in mm/s
  // (bei Vollschritt ist stepsPerMM ~24.8125, bei 1/16 ~397)
  const float FEED_MM_S = 5.0f;

  extruder.setFilamentSpeedMmS(FEED_MM_S);

  for(;;){
    extruder.runSpeed();
    taskYIELD();  // sehr kurz abgeben, ohne 1ms-Schlaf
  }
}

void hotEnd_task(void* parameters){
  myHotEnd.setFanPwm(180);
  for(;;){

    static float temp = 0;
    if(xQueueReceive(tempQueueHandle, &temp, pdMS_TO_TICKS(10)) != pdPASS) Serial.println("Fehler beim Empfangen der Temperatur");
      Serial.print(">temp:");
      Serial.println(temp, 2);   // println -> Zeilenumbruch \n
      
      if (temp < HEATER_SET_POINT) {
        myHotEnd.setHeaterPwm(255);
        myHotEnd.setFanPwm(180);   
      } 
      else {
        myHotEnd.setHeaterPwm(0);
        myHotEnd.setFanPwm(180);   
      }

      if (!tempReached && temp >= HEATER_SET_POINT) {
        tempReached = true;                 
        if (stepperTaskHandle) {
          vTaskResume(stepperTaskHandle);   // Stepper läuft ab jetzt dauerhaft
        }
    }

    // Regler
    //const float dt_s = HEATER_DELAY / 1000.0f;  
    //myHotEnd.piController(temp, dt_s,HEATER_SET_POINT);

    vTaskDelay(pdMS_TO_TICKS(HEATER_DELAY));
      
    }
}

void serial_task(void* parameters){
  for(;;){

    vTaskDelay(pdMS_TO_TICKS(200));
  }
}