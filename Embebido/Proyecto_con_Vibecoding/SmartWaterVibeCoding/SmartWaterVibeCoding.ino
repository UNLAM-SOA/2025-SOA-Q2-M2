#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "metrics.h"

// Metrics variables
//cantidad de tiempo que se desea tomar valores de muestreo de cpu y memoria
#define SAMPLING_TIME 10000
unsigned long initSampleTime = 0;
unsigned long actualSampleTime = 0;

// Global objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);
LiquidCrystal_I2C lcd(LCD_I2C_ADDRESS, 16, 2);

// Task handles
TaskHandle_t sensorTaskHandle = NULL;
TaskHandle_t controlTaskHandle = NULL;
TaskHandle_t uiTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;

// Queues
QueueHandle_t eventQueue = NULL;
QueueHandle_t mqttPublishQueue = NULL;

// System configuration
SystemConfig systemConfig = {
    .currentConsumption = 0.0,
    .maxConsumption = MAX_WATER_CONSUMPTION,
    .valveState = true,  // Start with valve open
    .alarmState = false,
    .currentState = STATE_INIT
};

// Flow sensor variables
volatile long pulseCount = 0;
unsigned long oldTime = 0;

// Forward declarations
void IRAM_ATTR pulseCounter();
void sensorTask(void *pvParameters);
void controlTask(void *pvParameters);
void uiTask(void *pvParameters);
void mqttTask(void *pvParameters);
void connectToWiFi();
void connectToMQTT();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void publishSystemStatus();

// Function to test the button
void testButton() {
    Serial.println("\n--- BUTTON TEST ---");
    Serial.print("Reading button pin ");
    Serial.print(BUTTON_PIN);
    Serial.print(": ");
    Serial.println(digitalRead(BUTTON_PIN));
    Serial.println("Press the button...");
    Serial.println("(Should change between 0 and 1 when pressed)");
    Serial.println("-------------------\n");
}

void setup() {
    // Initialize Serial
    Serial.begin(115200);
    delay(1000);  // Give time for serial to initialize
    Serial.println("\n\nStarting system...");
    
    // Test the button
    testButton();
    delay(3000);  // Wait 3 seconds to see the initial state
    
    // Initialize I/O pins
    pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(BUZZER_PIN, OUTPUT);
    
    // Attach interrupt for flow sensor
    attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), pulseCounter, FALLING);
    
    // Initialize LCD
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("SmartWaterVibe");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    
    // Connect to WiFi
    connectToWiFi();
    
    // Setup MQTT
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    connectToMQTT();
    
    // Create queues
    eventQueue = xQueueCreate(QUEUE_LENGTH, ITEM_SIZE);
    mqttPublishQueue = xQueueCreate(5, sizeof(char[100]));
    
    if (eventQueue == NULL || mqttPublishQueue == NULL) {
        Serial.println("Error creating queues!");
        while(1); // Halt if queues can't be created
    }
    
    // Create tasks
    xTaskCreatePinnedToCore(
        sensorTask,      // Task function
        "SensorTask",    // Task name
        TASK_STACK_SIZE, // Stack size
        NULL,            // Task parameters
        TASK_SENSOR_PRIORITY, // Task priority
        &sensorTaskHandle,    // Task handle
        0               // Core (0 or 1)
    );
    
    xTaskCreatePinnedToCore(
        controlTask,
        "ControlTask",
        TASK_STACK_SIZE,
        NULL,
        TASK_CONTROL_PRIORITY,
        &controlTaskHandle,
        1
    );
    
    xTaskCreatePinnedToCore(
        uiTask,
        "UITask",
        TASK_STACK_SIZE,
        NULL,
        TASK_UI_PRIORITY,
        &uiTaskHandle,
        1
    );
    
    xTaskCreatePinnedToCore(
        mqttTask,
        "MQTTTask",
        TASK_STACK_SIZE,
        NULL,
        TASK_MQTT_PRIORITY,
        &mqttTaskHandle,
        1
    );
    
    // Initial system state
    systemConfig.currentState = STATE_IDLE;
    digitalWrite(RELAY_PIN, systemConfig.valveState ? HIGH : LOW);
    
    Serial.println("System initialization complete!");

    // Init metrics
    initStats();
    initSampleTime = millis();
}

void loop() {
    actualSampleTime = millis();
    //cantidad de tiempo que se va a tomar las muestras 
    if(actualSampleTime - initSampleTime > SAMPLING_TIME){
        initSampleTime = actualSampleTime;
    finishStats();
   }
   
    static unsigned long lastButtonCheck = 0;
    static int lastButtonState = HIGH;
    
    // Check button state every 100ms
    if (millis() - lastButtonCheck > 100) {
        lastButtonCheck = millis();
        int currentButtonState = digitalRead(BUTTON_PIN);
        
        if (currentButtonState != lastButtonState) {
            Serial.print("Button state changed to: ");
            Serial.println(currentButtonState);
            lastButtonState = currentButtonState;
        }
    }
    
    // Let other tasks run
    vTaskDelay(10 / portTICK_PERIOD_MS);
}

// Interrupt Service Routine for flow sensor
void IRAM_ATTR pulseCounter() {
    pulseCount++;
}

// WiFi connection function
void connectToWiFi() {
    Serial.print("Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nWiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

// MQTT connection function
void connectToMQTT() {
    while (!mqttClient.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        if (mqttClient.connect(MQTT_CLIENT_ID)) {
            Serial.println("connected");
            mqttClient.subscribe(MQTT_TOPIC_SUB);
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 5 seconds");
            delay(5000);
        }
    }
}

// MQTT message callback
void mqttCallback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    // Process the message
    if (strcmp(message, "TOGGLE_VALVE") == 0) {
        Event event = {.type = EVENT_BUTTON_PRESSED, .data = NULL};
        xQueueSend(eventQueue, &event, portMAX_DELAY);
    } else if (strcmp(message, "RESET_CONSUMPTION") == 0) {
        systemConfig.currentConsumption = 0;
        Event event = {.type = EVENT_SYSTEM_UPDATE, .data = NULL};
        xQueueSend(eventQueue, &event, portMAX_DELAY);
    }
}

// Publish system status to MQTT
void publishSystemStatus() {
    char payload[100];
    snprintf(payload, sizeof(payload), 
             "{\"consumption\":%.2f,\"valve_open\":%s,\"state\":%d}",
             systemConfig.currentConsumption,
             systemConfig.valveState ? "true" : "false",
             systemConfig.currentState);
    
    mqttClient.publish(MQTT_TOPIC_PUB, payload);
}
