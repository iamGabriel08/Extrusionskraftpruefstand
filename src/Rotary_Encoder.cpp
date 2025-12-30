#include "Rotary_Encoder.h"

volatile int32_t Encoder::_overflowCount = 0;

Encoder::Encoder(const uint8_t dataPin):_dataPin(dataPin){
    pcnt_init(); 
}


// PCNT interrupt handler: called when the hardware counter hits the high limit.
void IRAM_ATTR Encoder::pcnt_intr_handler(void *arg) {
  
  uint32_t status = 0;
  pcnt_get_event_status(PCNT_UNIT, &status);     // Read and clear event status bits

  if (status & PCNT_EVT_H_LIM) {                 // High-limit reached (0..32767)
    _overflowCount++;                              // Extend count by tracking overflows
  pcnt_counter_clear(PCNT_UNIT);  // reset HW count to 0 so next block can accumulate

  }
  // Underflow not used in this project
}

// Configure and start PCNT to count rising edges on PULSE_PIN.
void Encoder::pcnt_init() {
  pcnt_config_t pcnt_config = {};
  pcnt_config.pulse_gpio_num  = _dataPin;        // Count pulses arriving on this GPIO
  pcnt_config.ctrl_gpio_num   = PCNT_PIN_NOT_USED;// No control pin
  pcnt_config.unit            = PCNT_UNIT;
  pcnt_config.channel         = PCNT_CHANNEL;
  pcnt_config.counter_h_lim   = Counter_limit;            // High limit → overflow event
  pcnt_config.counter_l_lim   = 0;                // Low limit (not used)
  pcnt_config.pos_mode        = PCNT_COUNT_INC;   // +1 on rising edges
  pcnt_config.neg_mode        = PCNT_COUNT_DIS;   // Ignore falling edges
  pcnt_config.lctrl_mode      = PCNT_MODE_KEEP;   // No control action
  pcnt_config.hctrl_mode      = PCNT_MODE_KEEP;

  pcnt_unit_config(&pcnt_config);                 // Apply configuration

  pcnt_event_enable(PCNT_UNIT, PCNT_EVT_H_LIM);   // Enable high-limit event interrupt

  pcnt_isr_service_install(0);                    // Install common ISR service (default flags)
  pcnt_isr_handler_add(PCNT_UNIT, pcnt_intr_handler, NULL); // Attach our handler

  pcnt_counter_pause(PCNT_UNIT);                  // Ensure stopped before clearing
  pcnt_counter_clear(PCNT_UNIT);                  // Reset hardware counter to 0
  pcnt_counter_resume(PCNT_UNIT);                 // Start counting
}


float Encoder::calc_length(uint32_t totalPulses) {
  return (float)totalPulses / (float)Pulses_per_mm;
}
float Encoder::get_length (){
    int16_t count = 0;
    pcnt_get_counter_value(PCNT_UNIT, &count);    // Read current 0..32767 hardware count

    // Each overflow represents 32768 counts (0..32767 inclusive)
    uint32_t totalPulses = (_overflowCount * 32768u) + (uint16_t)count;
    float length = calc_length(totalPulses);
    return length;
  
    //pcnt_counter_clear(PCNT_UNIT);                // Clear hardware counter for next window
   // overflowCount = 0;                            // Clear software overflow extension
}


