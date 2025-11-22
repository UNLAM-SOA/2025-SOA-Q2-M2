#include <LiquidCrystal_I2C.h>
#include "config.h"

extern SystemConfig systemConfig;
extern LiquidCrystal_I2C lcd;

// UI update interval in milliseconds
#define UI_UPDATE_INTERVAL 500

// Function to format consumption for display
String formatConsumption(float liters) {
    if (liters < 1.0) {
        return String(liters * 1000, 0) + " ml";
    } else if (liters < 1000.0) {
        return String(liters, 2) + " L";
    } else {
        return String(liters / 1000.0, 2) + " m³";
    }
}

void uiTask(void *pvParameters) {
    unsigned long lastUpdateTime = 0;
    
    // Task loop
    for (;;) {
        unsigned long currentTime = millis();
        
        // Update display at regular intervals
        if (currentTime - lastUpdateTime >= UI_UPDATE_INTERVAL) {
            // Clear the display
            lcd.clear();
            
            // First line: Current consumption and valve state
            lcd.setCursor(0, 0);
            lcd.print("Cons: ");
            lcd.print(formatConsumption(systemConfig.currentConsumption));
            
            // Right-align valve status
            String valveStatus = systemConfig.valveState ? "OPEN " : "CLOSED";
            lcd.setCursor(16 - valveStatus.length(), 0);
            lcd.print(valveStatus);
            
            // Second line: System status and warnings
            lcd.setCursor(0, 1);
            
            switch (systemConfig.currentState) {
                case STATE_INIT:
                    lcd.print("Initializing...");
                    break;
                    
                case STATE_IDLE:
                    lcd.print("System Ready");
                    break;
                    
                case STATE_WARNING_1:
                    lcd.print("WARNING: 25% Usage");
                    break;
                    
                case STATE_WARNING_2:
                    lcd.print("WARNING: 50% Usage");
                    break;
                    
                case STATE_WARNING_3:
                    lcd.print("WARNING: 75% Usage");
                    break;
                    
                case STATE_LIMIT_REACHED:
                    lcd.print("LIMIT REACHED!");
                    break;
                    
                case STATE_MANUAL_OVERRIDE:
                    lcd.print("Manual Mode");
                    break;
                    
                default:
                    lcd.print("Unknown State");
                    break;
            }
            
            lastUpdateTime = currentTime;
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
