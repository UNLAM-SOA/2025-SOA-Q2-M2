#include "config.h"
#include <Arduino.h>

extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/queue.h>
}

// Forward declarations
void controlTask(void *pvParameters);

// Global variables
extern SystemConfig systemConfig;
extern QueueHandle_t eventQueue;

// Buzzer variables
static bool buzzerOn = false;
static unsigned long lastBuzzerToggle = 0;
static unsigned long buzzerStartTime = 0;
static const unsigned long BUZZER_DURATION = 1000; // 3 seconds
static unsigned long buzzerInterval = 500; // Buzzer beep interval in ms
static bool buzzerActive = false;

void controlTask(void *pvParameters) {
    Event event;
    
    // Task loop
    for (;;) {
        // Wait for an event
        if (xQueueReceive(eventQueue, &event, portMAX_DELAY) == pdPASS) {
            // Handle the event based on current state
            switch (systemConfig.currentState) {
                case STATE_INIT:
                    // Initialization state
                    systemConfig.currentState = STATE_IDLE;
                    systemConfig.valveState = true; // Start with valve open
                    digitalWrite(RELAY_PIN, HIGH);
                    break;
                    
                case STATE_IDLE:
                    if (event.type == EVENT_BUTTON_PRESSED) {
                        // Toggle valve state
                        systemConfig.valveState = !systemConfig.valveState;
                        digitalWrite(RELAY_PIN, systemConfig.valveState ? HIGH : LOW);
                        
                        Serial.print("Button pressed - Toggling valve to: ");
                        Serial.println(systemConfig.valveState ? "ON" : "OFF");
                        
                        // If we're closing the valve, reset consumption
                        if (!systemConfig.valveState) {
                            systemConfig.currentConsumption = 0;
                            Serial.println("Valve closed - Consumption reset to 0");
                        }
                    }
                    break;
                    
                case STATE_WARNING_1:
                case STATE_WARNING_2:
                case STATE_WARNING_3:
                    if (event.type == EVENT_BUTTON_PRESSED) {
                        // Toggle valve state on button press
                        systemConfig.valveState = !systemConfig.valveState;
                        digitalWrite(RELAY_PIN, systemConfig.valveState ? HIGH : LOW);
                        
                        // If we're closing the valve, reset consumption
                        if (!systemConfig.valveState) {
                            systemConfig.currentConsumption = 0;
                        }
                        // Always return to IDLE state after button press in warning states
                        systemConfig.currentState = STATE_IDLE;
                    }
                    break;
                    
                case STATE_LIMIT_REACHED:
                    if (event.type == EVENT_BUTTON_PRESSED) {
                        // Reset consumption
                        systemConfig.currentConsumption = 0;
                        
                        // Keep the valve closed (safety measure)
                        systemConfig.valveState = false;
                        digitalWrite(RELAY_PIN, LOW);
                        
                        // Return to IDLE state
                        systemConfig.currentState = STATE_IDLE;
                        
                        // Turn off buzzer and alarm
                        noTone(BUZZER_PIN);
                        buzzerActive = false;
                        systemConfig.alarmState = false;
                        
                        Serial.println("Consumption reset to 0 - System ready for new cycle");
                    }
                    break;
                    
                case STATE_MANUAL_OVERRIDE:  // This case should no longer be reachable, but kept for safety
                    systemConfig.currentState = STATE_IDLE;  // Immediately return to IDLE state
                    break;
            }
            
            // Handle threshold events from any state
            if (event.type == EVENT_THRESHOLD_REACHED) {
                float threshold = event.data.floatValue;
                
                if (threshold == WARNING_THRESHOLD_1) {
                    systemConfig.currentState = STATE_WARNING_1;
                    buzzerStartTime = millis();
                    buzzerActive = true;
                    systemConfig.alarmState = true;
                    Serial.println("Warning 1 - Buzzer activated for 3 seconds");
                } 
                else if (threshold == WARNING_THRESHOLD_2) {
                    systemConfig.currentState = STATE_WARNING_2;
                    buzzerStartTime = millis();
                    buzzerActive = true;
                    systemConfig.alarmState = true;
                    Serial.println("Warning 2 - Buzzer activated for 3 seconds");
                }
                else if (threshold == WARNING_THRESHOLD_3) {
                    systemConfig.currentState = STATE_WARNING_3;
                    buzzerStartTime = millis();
                    buzzerActive = true;
                    systemConfig.alarmState = true;
                    Serial.println("Warning 3 - Buzzer activated for 3 seconds");
                }
                else if (threshold >= 1.0f) {
                    // Max consumption reached
                    systemConfig.valveState = false;
                    digitalWrite(RELAY_PIN, LOW);
                    systemConfig.currentState = STATE_LIMIT_REACHED;
                    buzzerStartTime = millis();
                    buzzerActive = true;
                    systemConfig.alarmState = true;
                    Serial.println("Limit reached - Buzzer activated for 3 seconds");
                }
            }
        }
        
        // Handle buzzer based on alarm state and timing
        unsigned long currentTime = millis();
        
        // Check if buzzer should be active
        if (buzzerActive) {
            // Turn off buzzer after 3 seconds
            if (currentTime - buzzerStartTime >= BUZZER_DURATION) {
                noTone(BUZZER_PIN);
                buzzerActive = false;
                systemConfig.alarmState = false;
                buzzerOn = false;
                Serial.println("Buzzer turned off after 3 seconds");
            } 
            // Toggle buzzer for beeping effect
            else if (currentTime - lastBuzzerToggle > buzzerInterval) {
                buzzerOn = !buzzerOn;
                if (buzzerOn) {
                    tone(BUZZER_PIN, BUZZER_FREQ);
                } else {
                    noTone(BUZZER_PIN);
                }
                lastBuzzerToggle = currentTime;
            }
        } 
        // Ensure buzzer is off if not active
        else if (buzzerOn) {
            noTone(BUZZER_PIN);
            buzzerOn = false;
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
