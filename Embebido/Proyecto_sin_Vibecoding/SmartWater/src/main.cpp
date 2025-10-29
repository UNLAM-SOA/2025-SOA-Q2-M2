#include "smartwater/smartwater.h"
#include "configurations.h"
#include "utils/utils.h"
// #include "metrics/metrics.h"

// #include "mqtt/mqttclient.h"
#include <WiFi.h>
#include <PubSubClient.h>

QueueHandle_t queueEvents;
QueueHandle_t queueLogs;
TaskHandle_t loopTaskHandler;
TaskHandle_t loopNewEventHandler;
TaskHandle_t loopLogHandler;


// MQTT
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Metrics stats
unsigned long initTime=0;
unsigned long  actualTime=0;


SmartWater* smartWater;

void tarea1(void *pvParameters) {
  while (1) {
    //Serial.println("tarea 1");
    vTaskDelay(1); // se cede CPU
  }
}

void tarea2(void *pvParameters) {
  while (1) {
    //Serial.println("tarea 2");
    vTaskDelay(1);
  }
}


void connectWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("[MQTT] Connected!");
    } else {
      Serial.print("[MQTT] Failed, rc=");
      Serial.println(mqttClient.state());
      vTaskDelay(pdMS_TO_TICKS(2000));
    }
  }
}


// === FreeRTOS - Loop ===
void vLoopTask(void *pvParameters) {
  while (1) {
    stEvento event;
    xQueueReceive(queueEvents, &event, portMAX_DELAY);
    smartWater->FSM(event);

    //cantidad de tiempo que se va a tomar las muestras 
    // if(actualTime-initTime>SAMPLING_TIME){
    //   initTime=actualTime;
    //   finishStats();
    // }

  }
}

// === FreeRTOS - Event Handler ===
void vGetNewEventTask(void *pvParameters) {
  while (1) {
    stEvento event = smartWater->GetEvent();
    const char* log = smartWater->GetLog();
    xQueueSend(queueEvents, &event, portMAX_DELAY);
    // xQueueSend(queueLogs, &log, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(TASKS_INTERVAL));
  }
}

// === FreeRTOS - Event Handler ===
// void vLogTask(void *pvParameters) {
//   while (1) {
//     // char* log;
//     // xQueueReceive(queueLogs, &log, portMAX_DELAY);
//     // smartWater->SendLog(log);
//   }
// }

void mqttTask(void* pvParameters) {
  connectWiFi(WIFI_SSID, WIFI_PASSWORD);
  mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
  connectMQTT();

  const TickType_t waitTime = pdMS_TO_TICKS(100);

  while (true) {
    mqttClient.loop();  // Mantiene viva la conexión

    // Revisa si hay mensajes en la cola
    char logMsg[128];
    if (xQueueReceive(queueLogs, &logMsg, waitTime) == pdTRUE) {
      debug_print("Enviando msj a topic");
      debug_print(MQTT_TOPIC_NAME);
      debug_print(logMsg);
      mqttClient.publish(MQTT_TOPIC_NAME, logMsg);
    }

    vTaskDelay(pdMS_TO_TICKS(50)); // Pequeña espera no bloqueante
  }
}


void logTask(void* pvParameters) {
  while (true) {
    char msg[64];
    snprintf(msg, sizeof(msg), smartWater->GetLog());

    // Enviar a la cola
    if (xQueueSend(queueLogs, &msg, 0) != pdTRUE) {
      Serial.println("[WARN] Cola MQTT llena, descartando log");
    }

    vTaskDelay(pdMS_TO_TICKS(5000));  // Cada 5 segundos
  }
}


void setup() {
  Serial.begin(SERIAL_MONITOR_BAUD_RATE);
  
  // System Main class init
  smartWater = new SmartWater(
    PIN_DIGITAL_PULSADOR,
    PIN_DIGITAL_CAUDALIMETRO,
    PIN_PWM_BUZZER,
    PIN_DIGITAL_RELAY_ELECTROVALVULA,
    UMBRAL_CONSUMO_1,
    UMBRAL_CONSUMO_2,
    UMBRAL_CONSUMO_3,
    WIFI_SSID,
    WIFI_PASSWORD,
    MQTT_TOPIC_NAME,
    MQTT_SERVER,
    MQTT_PORT,
    INTERVALO_DETECCION_MS
  );
  
  // FreeRTOS - Init Event Queue
  queueEvents = xQueueCreate(QUEUE_SIZE, sizeof(stEvento));
  queueLogs = xQueueCreate(QUEUE_SIZE, sizeof(char[32]));
  
  // FreeRTOS - Loop
  xTaskCreate(
    vLoopTask,
    "LoopFSM",
    STACK_SIZE,
    NULL,
    TASKS_PRIORITY,
    &loopTaskHandler
  );
  
  // FreeRTOS - Event handler
  xTaskCreate(
    vGetNewEventTask,
    "EventTask",
    STACK_SIZE,
    NULL,
    TASKS_PRIORITY,
    &loopNewEventHandler
  );

  // FreeRTOS - Log handler
  // xTaskCreate(
  //   vLogTask,
  //   "LogTask",
  //   STACK_SIZE,
  //   NULL,
  //   TASKS_PRIORITY,
  //   &loopLogHandler
  // );

  xTaskCreatePinnedToCore(mqttTask, "MQTT Task", 4096, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(logTask, "Log Task", 2048, NULL, 1, NULL, 1);

  // initStats();
  // initTime=millis();

}

void loop(){
  // The system loop is handled by FreeRTOS in vLoopTask
}