#include <Arduino.h>
#include <stdint.h>
#include "HotEnd.h"
#include "LoadCell.h"
#include "ExtruderStepper.h"

// Stepper 
#define STEP_PIN    10
#define DIR_PIN     11
#define EN_PIN      9

// Hot-End
#define NTC_PIN 4
#define HEATER_PIN 5
#define FAN_PIN 6

// Load Cell
#define LOADCELL_DOUT_PIN 8
#define LOADCELL_SCK_PIN 3

//========== Objekte ==========//
HotEnd myHotEnd(HEATER_PIN, NTC_PIN, FAN_PIN);
LoadCell myLoadCell(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
ExtruderStepper extruder(STEP_PIN, DIR_PIN, EN_PIN);

//========== Tasks ==========//
TaskHandle_t loadCellTaskHandle = NULL;
TaskHandle_t NTCTaskHandle = NULL;
TaskHandle_t stepperTaskHandle =  NULL;
TaskHandle_t hotEndTaskHandle = NULL;

QueueHandle_t tempQueueHandle = NULL;
#define TEMP_QUEUE_LENGTH 10
#define TEMP_QUEUE_SIZE sizeof(float)

// ====================== Funktionen-Definitionen ======================//
void loadCell_task(void* parameters);
void NTC_task(void* parameters);
void stepper_task(void* parameters);
void hotEnd_task(void* parameters);

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
  if (xTaskCreatePinnedToCore (NTC_task, "NTC Task", 6144, nullptr, 1, &NTCTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von NTC Task");
  }
  if (xTaskCreatePinnedToCore (stepper_task, "Stepper Task", 6144, nullptr, 1, &stepperTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Stepper Task");
  }
  if (xTaskCreatePinnedToCore (hotEnd_task, "Hot end Task", 6144, nullptr, 1, &hotEndTaskHandle, 0) != pdPASS) {
    Serial.println("Fehler beim erstellen von Stepper Task");
  }
}

void loop(){

  vTaskDelay(pdMS_TO_TICKS(50));
}

// ====================== Funktionen-Implementierungen ======================//
void loadCell_task(void* parameters){
  for(;;){
    float wheight = myLoadCell.getMeanWheight(10);
    //Serial.println(wheight, 3);      
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

void stepper_task(void*){
  for(;;){
    extruder.run(); // so oft wie möglich!

    if (!extruder.isRunning()) {
      extruder.extrudeMM(10.0f);     // erst neue Bewegung planen, wenn fertig
    }
    vTaskDelay(1); // 1 Tick “Luft”, damit andere Tasks laufen können
  }
}

void hotEnd_task(void* parameters){
  myHotEnd.setFanPwm(180);
  for(;;){

    static float temp = 0;
    if(xQueueReceive(tempQueueHandle, &temp, pdMS_TO_TICKS(10)) != pdPASS) Serial.println("Fehler beim Empfangen der Temperatur");
      Serial.print(">temp:");
      Serial.println(temp, 2);   // println -> Zeilenumbruch \n

      if (temp < 180.0f) {
        myHotEnd.setHeaterPwm(255);
        myHotEnd.setFanPwm(180);   
      } 
      else {
        myHotEnd.setHeaterPwm(0);
        myHotEnd.setFanPwm(180);   
      }
      vTaskDelay(pdMS_TO_TICKS(100));
    }
}