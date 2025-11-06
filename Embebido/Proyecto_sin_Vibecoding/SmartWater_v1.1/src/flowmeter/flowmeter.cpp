#include "flowmeter.h"
#include "Arduino.h"

// Variables para conteo de pulsos (volatile porque se usan en ISR)
volatile unsigned long pulsos_totales = 0;
volatile unsigned long SAMPLE_PULSES = 0;

void IRAM_ATTR countPulses() {
  pulsos_totales++;
  SAMPLE_PULSES++;
}

FlowMeter :: FlowMeter(){}

FlowMeter :: FlowMeter(int pinNumber){
    this->pinNumber = pinNumber;
    pinMode(pinNumber, INPUT);
    attachInterrupt(digitalPinToInterrupt(pinNumber), countPulses, RISING);
}

float FlowMeter :: GetConsumptionSinceLastMeasurement(){
  long NOW = millis();
  long ELAPSED_TIME = NOW - LAST_MEASUREMENT_TIME;
  
  if (ELAPSED_TIME >= FLOW_RATE_SAMPLE_TIME_MS) {
    // Calculate pulse frequency in Hz
    float frequency = (SAMPLE_PULSES * 1000.0) / ELAPSED_TIME;
    
    // Convert frequency to flow rate in L/min
    float current_flow = frequency / MEASUREMENT_CORRECTION_FACTOR;
    
    // Calculate incremental volume: Q(L/min) * time(min)
    float time_mins = ELAPSED_TIME / 60000.0;
    float incremental_volume = current_flow * time_mins;
    
    // Reset counters for the next sample
    SAMPLE_PULSES = 0;
    LAST_MEASUREMENT_TIME = NOW;

    return incremental_volume;
  }
  
  return 0;
}