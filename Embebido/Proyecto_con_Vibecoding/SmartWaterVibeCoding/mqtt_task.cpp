#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"

extern WiFiClient espClient;
extern PubSubClient mqttClient;
extern SystemConfig systemConfig;

extern void connectToMQTT();
extern void publishSystemStatus();

void mqttTask(void *pvParameters) {
    unsigned long lastPublishTime = 0;
    bool lastMqttConnected = false;
    
    // Task loop
    for (;;) {
        unsigned long currentTime = millis();
        
        // Check MQTT connection
        if (!mqttClient.connected()) {
            if (lastMqttConnected) {
                Serial.println("MQTT disconnected, attempting to reconnect...");
                lastMqttConnected = false;
            }
            connectToMQTT();
        } else if (!lastMqttConnected) {
            Serial.println("MQTT connected");
            lastMqttConnected = true;
        }
        
        // Process MQTT messages
        mqttClient.loop();
        
        // Publish system status at regular intervals
        if (currentTime - lastPublishTime >= MQTT_UPDATE_INTERVAL) {
            if (mqttClient.connected()) {
                publishSystemStatus();
                lastPublishTime = currentTime;
            }
        }
        
        // Small delay to prevent task from hogging CPU
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
