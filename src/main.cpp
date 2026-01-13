#include <Arduino.h>
#include <stdint.h>
#include "HotEnd.h"
#include "LoadCell.h"
#include "ExtruderStepper.h"
#include "Rotary_Encoder.h"
#include "GUICommunication.h"

//========== Stepper ==========//

#define EN_PIN     11
#define STEP_PIN    10
#define DIR_PIN     9

// Parameter kommen vom GUI
float feed_rate_per_s_in_mm;
float feed_length_in_mm;

// Messzeit in ms (wird durch GUI Parameter berechnet)
unsigned long measureTimeMS = 0;

//========== Hot-End ==========//

#define NTC_PIN 4
#define HEATER_PIN 6
#define FAN_PIN 7
#define HEATER_DELAY 100

// Parameter kommen vom GUI
float heater_temp_target = 180.; 
uint8_t hot_end_abschalten=0; 

//==========  Load Cell ==========//

#define LOADCELL_DOUT_PIN 8
#define LOADCELL_SCK_PIN 3
uint8_t tare=0; //1, falls genullt werden soll


//==========  Rotary Encoder ==========//

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

//========== Queues ==========//

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

// Queue für Slip (RotEncoder Task -> Telemetry Task)
QueueHandle_t SlipTelemetryQueueHandle = NULL; 
#define SLIP_QUEUE_LENGTH 10
#define SLIP_QUEUE_SIZE sizeof(float)
// Queue für Slip (RotEncoder Task -> Controller Task) --> nutz die Selben Makros wie SlipTelemetryQueue
QueueHandle_t SlipControllerQueueHandle = NULL; 

//========== Controller Task ==========//

// Zeitstempel für Controller Taks
unsigned long timeStamp = 0;

// Flag falls definierte Temperatur noch nicht erreicht
bool tempReached = false; 

// Flag das die Messung läuft
bool isMeasuring = false;

// States für Messmodi
enum MeasureMode: uint8_t{
  MODE_TIME = 0,
  MODE_MAX_FORCE = 1
};
volatile MeasureMode gMode = MODE_TIME;

// Stepper-Skip/Slip-Erkennung
static constexpr float SLIP_TRIGGER_PERCENT = 5.0f;     // Schlupf in % beim welchem abgeschalten werden soll
static constexpr uint32_t SLIP_HOLD_MS = 2000;            // Schlupf muss so lange stattfinden
static constexpr uint32_t MAX_FORCE_TIMEOUT_MS = 120000; // Safety: 2min max-force Mode

bool heaterLockedOff = false;                   // im Max-Force Mode nach T_soll = true

// ====================== Funktionen-Definitionen ======================//
// Task Funktionen
void loadCell_task(void* parameters);
void NTC_task(void* parameters);
void stepper_task(void* parameters);
void hotEnd_task(void* parameters);
void serial_task(void* parameters);
void rotEncoder_task(void* parameters);
void controller_task(void* parameters);
void Telemetry_Task(void* parameters);

// Helfer Funktionen 
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

  SlipTelemetryQueueHandle = xQueueCreate(SLIP_QUEUE_LENGTH, SLIP_QUEUE_SIZE);
  if(SlipTelemetryQueueHandle == NULL) Serial.println("Fehler beim erstellen der Slip Queue für die Telemetry");

  SlipControllerQueueHandle = xQueueCreate(SLIP_QUEUE_LENGTH, SLIP_QUEUE_SIZE);
  if(SlipControllerQueueHandle == NULL) Serial.println("Fehler beim erstellen der Slip Queue für den Controller");

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
        
        if(xQueueSend(SlipTelemetryQueueHandle, (void*)&schlupf, 0) != pdPASS){
          xQueueReceive(SlipTelemetryQueueHandle, (void*)&dummy,0);
          xQueueSend(SlipTelemetryQueueHandle, (void*)&schlupf, 0);
        }
        if(xQueueSend(SlipControllerQueueHandle, (void*)&schlupf, 0) != pdPASS){
          xQueueReceive(SlipControllerQueueHandle, (void*)&dummy,0);
          xQueueSend(SlipControllerQueueHandle, (void*)&schlupf, 0);
        }
      } 
      else {

        float schlupf_0 = 0;
        if(xQueueSend(SlipTelemetryQueueHandle, (void*)&schlupf_0, 0) != pdPASS){
        xQueueReceive(SlipTelemetryQueueHandle, (void*)&dummy,0);
        xQueueSend(SlipTelemetryQueueHandle, (void*)&schlupf_0, 0);
        }
        if(xQueueSend(SlipControllerQueueHandle, (void*)&schlupf_0, 0) != pdPASS){
        xQueueReceive(SlipControllerQueueHandle, (void*)&dummy,0);
        xQueueSend(SlipControllerQueueHandle, (void*)&schlupf_0, 0);
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
    
    if (heaterLockedOff) {
      myHotEnd.setHeaterPwm(0);
      myHotEnd.setFanPwm(180);
      } 
    else{
      // normale Heizlogik
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
    }

    if (!tempReached && temp >= heater_temp_target) {
      timeStamp = millis(); // Zeitstempel setzten wenn Messung beginnt
      tempReached = true;                 
      isMeasuring = true;

      if (gMode == MODE_MAX_FORCE) {
        heaterLockedOff = true;  // <- ab jetzt bleibt der Heater aus!
        myHotEnd.setHeaterPwm(0);
      }

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
      Serial.println("begin"); //meldet der GUI, dass Aufheizen zuende und Messung beginnt
    }
    // PI-Regler --> P und I Wert sind NICHT richtig eingestellt
    //const float dt_s = HEATER_DELAY / 1000.0f;  
    //myHotEnd.piController(temp, dt_s,HEATER_SET_POINT);
    vTaskDelay(pdMS_TO_TICKS(HEATER_DELAY));
  }
}

void serial_task(void* parameters){
  for(;;){

    bool gotCmd = my_GUI.get_serial_input(&heater_temp_target, &feed_rate_per_s_in_mm, &feed_length_in_mm, &hot_end_abschalten, &tare);

    // Tare immer priorisieren, sobald Flag gesetzt ist
    if (tare == 1) {
      tareLoadCell();     
      tare = 0;           // WICHTIG: sonst wird endlos getared
    }

    if(gotCmd){

      //printe Daten 
      Serial.println("Empfangene Daten:");
      Serial.println("Temperatur");
      Serial.println(heater_temp_target);
      Serial.println("FeedRate");
      Serial.println(feed_rate_per_s_in_mm);
      Serial.println("FeedLength");
      Serial.println(feed_length_in_mm);
      Serial.println("Abschalten?");
      Serial.println(hot_end_abschalten);

      // Modus anhand GUI-Flag wählen
      gMode = (hot_end_abschalten == 1) ? MODE_MAX_FORCE : MODE_TIME;


      // Messung vorbereiten
      tempReached = false;
      isMeasuring = false;
      heaterLockedOff = false;
      timeStamp = 0;              // wichtig: damit Controller nicht sofort stoppt

      xQueueReset(tempTelemetryQueueHandle);
      xQueueReset(ForceQueueHandle);
      xQueueReset(SlipTelemetryQueueHandle);
      xQueueReset(tempHotEndQueueHandle);        
      xQueueReset(SlipControllerQueueHandle);

      // Zeit nur für MODE_TIME berechnen
      if (gMode == MODE_TIME) {
        if (!computeMeasureTime()) {
          Serial.println("Fehler beim Berechnen der Messzeit");
          continue;
        }
      } 
      else {
        measureTimeMS = 0; // im Max-Force Mode wird nicht nach Zeit gestoppt
      }
      // Heizen starten
      vTaskResume(TelemetryTaskHandle);
      vTaskResume(NTCTaskHandle);
      vTaskResume(hotEndTaskHandle);
      vTaskSuspend(NULL);   // Serial Task anhalten
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void controller_task(void* parameters){
  static uint32_t slipAboveSince = 0; // Zeit seit überschreiten des SLIP_TRIGGER_PERCENT
  static uint32_t maxForceStart = 0; // Max Force Mode start Zeit in ms

  for (;;) {

    // Safety-Startzeit für Max-Force Mode
    if (isMeasuring && gMode == MODE_MAX_FORCE && maxForceStart == 0) {
      maxForceStart = millis();
      slipAboveSince = 0;
    }

    bool stopNow = false; // Flag zu beenden der Messung bei Zeitablauf oder zu viel Slip

    if (isMeasuring && timeStamp != 0) {

      if (gMode == MODE_TIME) {
        if (measureTimeMS != 0 && (millis() - timeStamp >= measureTimeMS)) {
          stopNow = true;
        }
      } 
      else { // MODE_MAX_FORCE

        // Slip “neueste” Werte holen (Queue leeren bis zuletzt)
        float slip = NAN;
        bool gotSlip = false;
        while (xQueueReceive(SlipControllerQueueHandle, &slip, 0) == pdPASS) {
          gotSlip = true;
        }

        if (gotSlip) {
          if (slip >= SLIP_TRIGGER_PERCENT) {
            if (slipAboveSince == 0) slipAboveSince = millis();
            if (millis() - slipAboveSince >= SLIP_HOLD_MS) stopNow = true;
          } 
          else {
            slipAboveSince = 0;
          }
        }

        // Safety Timeout
        if (!stopNow && maxForceStart != 0 && (millis() - maxForceStart >= MAX_FORCE_TIMEOUT_MS)) {
          Serial.println("Max-Force Timeout -> Stop");
          stopNow = true;
        }
      }
    }

    if (stopNow) {
      timeStamp = 0;
      measureTimeMS = 0;
      tempReached = false;
      isMeasuring = false;
      heaterLockedOff = false;
      maxForceStart = 0;
      slipAboveSince = 0;

      stopAllActuators();
      myEncoder.reset();
      Serial.println("end");

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
    while(xQueueReceive(SlipTelemetryQueueHandle, &slip, 0) == pdPASS){
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
  
  eTaskState st = eTaskGetState(loadCellTaskHandle);
  bool wasRunning = (st != eSuspended);
  if (wasRunning) vTaskSuspend(loadCellTaskHandle);

  if (!myLoadCell.tare(20)) {
    Serial.println("Tare fehlgeschlagen (HX711 nicht ready?)");
  } else {
    Serial.println("Tare OK -> Force/Weight jetzt relativ zu Nullpunkt");
  }

  if (wasRunning) vTaskResume(loadCellTaskHandle);

  tare = 0; 

}