#ifndef CONFIGURATIONS_H
#define CONFIGURATIONS_H

// ======== WiFi ========
#define WIFI_SSID "TeleCentro-b0f8"
#define WIFI_PASSWORD "GTZFTYZMKNMM"

// ======== MQTT ========
#define MQTT_SERVER "broker.hivemq.com"        // Public Broker
#define MQTT_PORT 1883
#define MQTT_TOPIC_NAME "unlam/soa/m2/test/mensaje"  // Publish Topic

// ======== SYSTEM ========
#define SERIAL_MONITOR_BAUD_RATE 115200
#define PIN_DIGITAL_PULSADOR 19
#define PIN_DIGITAL_CAUDALIMETRO 5
#define PIN_PWM_BUZZER 4
#define PIN_DIGITAL_RELAY_ELECTROVALVULA 16
#define CAPACIDAD_MAXIMA 100.0
#define UMBRAL_CONSUMO_1 CAPACIDAD_MAXIMA * 0.25
#define UMBRAL_CONSUMO_2 CAPACIDAD_MAXIMA * 0.50
#define UMBRAL_CONSUMO_3 CAPACIDAD_MAXIMA * 0.75
#define INTERVALO_DETECCION_MS 50
//cantidad de tiempo que se desea tomar valores de muestreo de cpu y memoria
#define SAMPLING_TIME 10000


// ======== FREERTOS ========
#define QUEUE_SIZE        10     // Events Queue Size
#define STACK_SIZE        8192   // Tasks Stack Size
#define TASKS_PRIORITY    1      // Tasks priority
#define TASKS_INTERVAL    10     // Tasks interval (ms)

#endif