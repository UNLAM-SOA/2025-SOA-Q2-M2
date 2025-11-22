#include "config.h"
#include <Arduino.h>

extern "C" {
    #include <freertos/FreeRTOS.h>
    #include <freertos/task.h>
    #include <freertos/queue.h>
}

// Forward declarations
void IRAM_ATTR pulseCounter();

// Global variables
extern volatile long pulseCount;
extern SystemConfig systemConfig;
extern QueueHandle_t eventQueue;

// Button debouncing variables
static unsigned long lastDebounceTime = 0;
static int lastButtonState = HIGH;
static int buttonState = HIGH;
static bool buttonPressed = false;
static unsigned long lastButtonPressTime = 0;

void sensorTask(void *pvParameters) {
    // Local variables for flow calculation
    float flowRate = 0.0;
    float totalMilliLitres = 0;
    float totalLitres = 0;
    unsigned long oldTime = millis();
    
    // Task loop
    for (;;) {
        unsigned long currentTime = millis();
        
        // Calculate flow rate every second
        if (currentTime - oldTime > 1000) {
            // Disable interrupts while reading pulse count
            noInterrupts();
            long currentPulseCount = pulseCount;
            pulseCount = 0;
            interrupts();
            
            // Calculate flow rate in L/s
            flowRate = ((1000.0 / (currentTime - oldTime)) * currentPulseCount) / PULSES_PER_LITER;
            
            // Calculate volume in liters
            totalLitres += flowRate * ((currentTime - oldTime) / 1000.0);
            systemConfig.currentConsumption = totalLitres;
            
            // Debug output for consumption
            static unsigned long lastPrintTime = 0;
            if (currentTime - lastPrintTime > 1000) {  // Print every second
                Serial.print("Current Consumption: ");
                Serial.print(systemConfig.currentConsumption, 3);
                Serial.println(" L");
                lastPrintTime = currentTime;
            }
            
            // Update old time
            oldTime = currentTime;
            
            // Create flow detected event
            Event flowEvent;
            flowEvent.type = EVENT_FLOW_DETECTED;
            flowEvent.data.ptrValue = nullptr;
            xQueueSend(eventQueue, &flowEvent, 0);
            
            // Check consumption thresholds
            if (totalLitres >= systemConfig.maxConsumption * WARNING_THRESHOLD_1 && 
                totalLitres < systemConfig.maxConsumption * WARNING_THRESHOLD_2 &&
                systemConfig.currentState < STATE_WARNING_1) {
                Event thresholdEvent;
                thresholdEvent.type = EVENT_THRESHOLD_REACHED;
                thresholdEvent.data.floatValue = WARNING_THRESHOLD_1;
                xQueueSend(eventQueue, &thresholdEvent, 0);
            } else if (totalLitres >= systemConfig.maxConsumption * WARNING_THRESHOLD_2 && 
                      totalLitres < systemConfig.maxConsumption * WARNING_THRESHOLD_3 &&
                      systemConfig.currentState < STATE_WARNING_2) {
                Event thresholdEvent;
                thresholdEvent.type = EVENT_THRESHOLD_REACHED;
                thresholdEvent.data.floatValue = WARNING_THRESHOLD_2;
                xQueueSend(eventQueue, &thresholdEvent, 0);
            } else if (totalLitres >= systemConfig.maxConsumption * WARNING_THRESHOLD_3 && 
                      totalLitres < systemConfig.maxConsumption &&
                      systemConfig.currentState < STATE_WARNING_3) {
                Event thresholdEvent;
                thresholdEvent.type = EVENT_THRESHOLD_REACHED;
                thresholdEvent.data.floatValue = WARNING_THRESHOLD_3;
                xQueueSend(eventQueue, &thresholdEvent, 0);
            } else if (totalLitres >= systemConfig.maxConsumption && 
                      systemConfig.currentState < STATE_LIMIT_REACHED) {
                Event limitEvent;
                limitEvent.type = EVENT_THRESHOLD_REACHED;
                limitEvent.data.floatValue = 1.0f;
                xQueueSend(eventQueue, &limitEvent, 0);
            }
        }
        
        // Button debouncing with edge detection and state tracking
        int reading = digitalRead(BUTTON_PIN);
        
        // Check for button state change
        if (reading != lastButtonState) {
            lastDebounceTime = currentTime;
        }
        
        // If the button state has been stable for the debounce delay
        if ((currentTime - lastDebounceTime) > DEBOUNCE_DELAY) {
            // If the button state has changed
            if (reading != buttonState) {
                buttonState = reading;
                
                // Button is pressed (LOW because of pull-up)
                if (buttonState == LOW) {
                    // Only register a new press if it's been a while since the last one
                    if ((currentTime - lastButtonPressTime) > 500) { // 500ms debounce
                        lastButtonPressTime = currentTime;
                        
                        // If we're closing the valve, reset consumption
                        systemConfig.currentConsumption = 0;
                        totalLitres = 0;  // Reset the local totalLitres counter
                        pulseCount = 0;   // Reset the pulse counter
                        Serial.println("Consumption reset to 0");
                        
                        // Create and send button press event
                        Event buttonEvent;
                        buttonEvent.type = EVENT_BUTTON_PRESSED;
                        buttonEvent.data.ptrValue = nullptr;
                        
                        // Try to send the event to the queue
                        if (xQueueSend(eventQueue, &buttonEvent, 0) != pdPASS) {
                            Serial.println("Queue full - button press not sent");
                        } else {
                            Serial.println("Button press event sent to queue");
                        }
                    }
                }
            }
        }
        
        // Update last button state for next iteration
        lastButtonState = reading;
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
