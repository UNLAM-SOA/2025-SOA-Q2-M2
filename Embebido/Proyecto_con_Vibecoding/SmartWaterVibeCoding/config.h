#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <stdint.h>
#include <cstddef>

// WiFi Configuration
#define WIFI_SSID "TeleCentro-b0f8"
#define WIFI_PASSWORD "GTZFTYZMKNMM"

// MQTT Configuration
#define MQTT_SERVER "broker.hivemq.com"
#define MQTT_PORT 1883
#define MQTT_TOPIC_PUB "smartwatervibe/status"
#define MQTT_TOPIC_SUB "smartwatervibe/control"
#define MQTT_CLIENT_ID "ESP32_Water_Controller"

// Hardware Pin Definitions
#define FLOW_SENSOR_PIN 5     // YF-S201 Flow Sensor
#define RELAY_PIN 16          // KY-019 Relay Module
#define BUTTON_PIN 19         // Push Button
#define BUZZER_PIN 4          // Passive Buzzer

// LCD I2C Address (typically 0x27 or 0x3F)
#define LCD_I2C_ADDRESS 0x27

// Water Consumption Thresholds (in liters)
#define MAX_WATER_CONSUMPTION 3.0  // Maximum allowed water consumption before shutoff
#define WARNING_THRESHOLD_1 0.25f     // 25% of max
#define WARNING_THRESHOLD_2 0.50f     // 50% of max
#define WARNING_THRESHOLD_3 0.75f     // 75% of max

// Flow Sensor Configuration (pulses per liter)
#define PULSES_PER_LITER 450  // YF-S201 typically has 450 pulses per liter

// System Parameters
#define DEBOUNCE_DELAY 50     // Button debounce time in ms
#define MQTT_UPDATE_INTERVAL 10000  // MQTT update interval in ms
#define BUZZER_FREQ 500      // Buzzer frequency in Hz

// Task Priorities (higher number = higher priority)
#define TASK_MQTT_PRIORITY 3
#define TASK_SENSOR_PRIORITY 2
#define TASK_UI_PRIORITY 2
#define TASK_CONTROL_PRIORITY 4  // Highest priority for control task

// Task Stack Sizes (in words)
#define TASK_STACK_SIZE 4096  // Increased from 2048

// Queue Sizes
#define QUEUE_LENGTH 10
#define ITEM_SIZE sizeof(Event)

// Event Types
typedef enum {
    EVENT_NONE,
    EVENT_BUTTON_PRESSED,
    EVENT_FLOW_DETECTED,
    EVENT_THRESHOLD_REACHED,
    EVENT_MQTT_MESSAGE,
    EVENT_SYSTEM_UPDATE
} EventType;

// System States
typedef enum {
    STATE_INIT,
    STATE_IDLE,
    STATE_WARNING_1,
    STATE_WARNING_2,
    STATE_WARNING_3,
    STATE_LIMIT_REACHED,
    STATE_MANUAL_OVERRIDE
} SystemState;

// Event Structure
typedef struct {
    EventType type;
    union {
        float floatValue;
        void* ptrValue;
    } data;
} Event;

// System Configuration Structure
typedef struct {
    float currentConsumption;  // in liters
    float maxConsumption;      // in liters
    bool valveState;           // true = open, false = closed
    bool alarmState;           // true = on, false = off
    SystemState currentState;
} SystemConfig;

#endif // CONFIG_H
