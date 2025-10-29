#ifndef SMARTWATER_H
#define SMARTWATER_H

#include "wifi/wifi.h"
#include "button/button.h"
#include "buzzer/buzzer.h"
#include "flowmeter/flowmeter.h"
#include "relay/relay.h"
#include "lcddisplay/lcddisplay.h"

// === Tipos y estructuras ===
enum tipo_evento_t {
  TIPO_EVENTO_CONTINUE,
  TIPO_EVENTO_BUTTON_ON,
  TIPO_EVENTO_BUTTON_OFF,
  TIPO_EVENTO_TIMEOUT_VALVULA,
  TIPO_EVENTO_TIMEOUT_ALARMA,
  TIPO_EVENTO_CONSUMO_UMBRAL1,
  TIPO_EVENTO_CONSUMO_UMBRAL2,
  TIPO_EVENTO_CONSUMO_UMBRAL3,
  TIPO_EVENTO_CONSUMO_LIMITE
};

enum estado_t {
  ESTADO_IDLE,
  ESTADO_APERTURA_ELECTROVALVULA,
  ESTADO_NORMAL,
  ESTADO_ALARMA1,
  ESTADO_ALARMA2,
  ESTADO_ALARMA3,
  ESTADO_LIMITE
};

struct stEvento {
  tipo_evento_t tipo;
};

class SmartWater {
    private:
        estado_t ESTADO_ACTUAL = ESTADO_IDLE;
        // WiFiClient wifiClient;
        // MQTTClient mqttClient;
        Button button;
        Buzzer buzzer;
        FlowMeter flowmeter;
        Relay valve;
        LCDDisplay display;

        float CONSUMO_TOTAL = 0.0;
        long TIEMPO_ULTIMO_EVENTO = 0;
        long TIEMPO_ULTIMO_LOG = 0;
        long INTERVALO_DETECCION_MS;
        long INTERVALO_LOG_MS = 500; 

        float UMBRAL_CONSUMO_1;
        float UMBRAL_CONSUMO_2;
        float UMBRAL_CONSUMO_3;

        short indice_evento = 0; 

        char buffer[32];

    public:
        SmartWater();
        SmartWater(
            const int PIN_DIGITAL_PULSADOR,
            const int PIN_DIGITAL_CAUDALIMETRO,
            const int PIN_PWM_BUZZER,
            const int PIN_DIGITAL_RELAY_ELECTROVALVULA,
            const float UMBRAL_CONSUMO_1,
            const float UMBRAL_CONSUMO_2,
            const float UMBRAL_CONSUMO_3,
            const char* WIFI_SSID,
            const char* WIFI_PASSWORD,
            const char* MQTT_TOPIC_NAME,
            const char* MQTT_SERVER,
            const int MQTT_PORT,
            const long INTERVALO_DETECCION_MS
        );
        void FSM(stEvento event);
        stEvento GetEvent();
        const char* GetLog();
        void SendLog(const char* msg);

        bool detectar_button_on(stEvento* e);
        bool detectar_button_off(stEvento* e);
        bool detectar_consumo_umbral1(stEvento* e);
        bool detectar_consumo_umbral2(stEvento* e);
        bool detectar_consumo_umbral3(stEvento* e);
        bool detectar_consumo_limite(stEvento* e);
};

typedef bool (*func_evento_t)(SmartWater*, stEvento*);

bool detectar_button_on(SmartWater* s, stEvento* e);
bool detectar_button_off(SmartWater* s, stEvento* e);
bool detectar_consumo_umbral1(SmartWater* s, stEvento* e);
bool detectar_consumo_umbral2(SmartWater* s, stEvento* e);
bool detectar_consumo_umbral3(SmartWater* s, stEvento* e);
bool detectar_consumo_limite(SmartWater* s, stEvento* e);

#endif