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
  ESTADO_ALARMA1,
  ESTADO_ALARMA2,
  ESTADO_ALARMA3,
  ESTADO_LIMITE,
  ESTADO_FINALIZADO
};

struct stEvento {
  tipo_evento_t tipo;
};

class SmartWater {
    private:
        estado_t ESTADO_ACTUAL = ESTADO_IDLE;
        Button button;
        Buzzer buzzer;
        FlowMeter flowmeter;
        Relay valve;
        LCDDisplay display;

        float CONSUMO_TOTAL = 0.0;
        long TIEMPO_INICIO_ALARMA = 0;
        long TIEMPO_ULTIMO_EVENTO = 0;
        long TIEMPO_ULTIMO_LOG = 0;
        long INTERVALO_DETECCION_MS;
        long INTERVALO_LOG_MS = 1000; 
        long INTERVALO_DURACION_ALARMA = 0;

        float UMBRAL_CONSUMO_1;
        float UMBRAL_CONSUMO_2;
        float UMBRAL_CONSUMO_3;
        float UMBRAL_LIMITE;

        bool ALARMA_1_USADA = false;
        bool ALARMA_2_USADA = false;
        bool ALARMA_3_USADA = false;
        bool ALARMA_LIMITE_USADA = false;

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
            const float UMBRAL_LIMITE,
            const char* MQTT_TOPIC_NAME,
            const char* MQTT_SERVER,
            const int MQTT_PORT,
            const long INTERVALO_DETECCION_MS,
            const long INTERVALO_DURACION_ALARMA
        );
        void FSM(stEvento event);
        stEvento GetEvent();
        const char* GetLog();
        void SendLog(const char* msg);

        bool detectar_button_on(stEvento* e);
        bool detectar_consumo_umbral1(stEvento* e);
        bool detectar_consumo_umbral2(stEvento* e);
        bool detectar_consumo_umbral3(stEvento* e);
        bool detectar_consumo_limite(stEvento* e);
        bool detectar_timeout_alarma(stEvento* e);
};

typedef bool (*func_evento_t)(SmartWater*, stEvento*);

bool detectar_button_on(SmartWater* s, stEvento* e);
bool detectar_consumo_umbral1(SmartWater* s, stEvento* e);
bool detectar_consumo_umbral2(SmartWater* s, stEvento* e);
bool detectar_consumo_umbral3(SmartWater* s, stEvento* e);
bool detectar_consumo_limite(SmartWater* s, stEvento* e);
bool detectar_timeout_alarma(SmartWater* s, stEvento* e);

#endif