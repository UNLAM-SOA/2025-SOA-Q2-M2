#include <WiFi.h>
#include <PubSubClient.h>

// ======== CONFIGURACIÓN WiFi ========
const char* ssid = "nombre-wifi";
const char* password = "contraseña-wifi";

// ======== CONFIGURACIÓN MQTT ========
const char* mqtt_server = "broker.hivemq.com";  // Broker público
const int mqtt_port = 1883;
const char* topic = "unlam/soa/m2/test"; // Topico

WiFiClient espClient;
PubSubClient client(espClient);

// ================================================
// CONFIGURACIÓN DE PINES
// ================================================
// Pines de sensores
#define PIN_D_PULSADOR          19  // Pulsador de emergencia (digital)
#define PIN_A_CAUDALIMETRO      5  // Sensor de flujo (analógico)

// Pines de actuadores
#define PIN_P_BUZZER            4   // Buzzer con PWM
#define PIN_D_RELAY_ELECTROVALVULA  16  // Relé para electroválvula

// ================================================
// CONFIGURACIÓN DE UMBRALES DE CONSUMO
// ================================================
const float CAPACIDAD_RECIPIENTE = 100.0;  // Litros - Ajustar según recipienterecipiente
const float UMBRAL1 = CAPACIDAD_RECIPIENTE * 0.25;  // 25% del recipiente
const float UMBRAL2 = CAPACIDAD_RECIPIENTE * 0.50;  // 50% del recipiente
const float UMBRAL3 = CAPACIDAD_RECIPIENTE * 0.75;  // 75% del recipiente
const float LIMITE = CAPACIDAD_RECIPIENTE * 0.90;   // 90% del recipiente

// ================================================
// CONFIGURACIÓN DE SENSORES
// ================================================
// Factor de conversión del caudalímetro a litros
#define FACTOR_AJUSTE_SENSOR_A_CONSUMO_AGUA 3.75

// Resolución del ADC del ESP32
#define ADC_MAX_VALUE 4095

// ================================================
// CONFIGURACIÓN DE TIEMPOS
// ================================================
#define TIEMPO_ESPERA_VALVULA_MS  3000  // Tiempo de apertura de válvula
#define TIEMPO_BUZZER_MS          3000  // Duración de cada alarma
#define INTERVALO_TAREAS          10    // Intervalo entre tareas (ms)
#define INTERVALO_DETECCION_MS    50    // Intervalo detección eventos (ms)
#define TIEMPO_MUESTRA_CAUDAL_MS  1000  // Tiempo de muestreo para calcular caudal


// ================================================
// CONFIGURACIÓN DE ALARMAS
// ================================================
// Frecuencias para diferentes niveles de alarma (Hz)
const int FREC_APAGADO = 0;
const int FREC_UMBRAL1 = 1;
const int FREC_UMBRAL2 = 2;
const int FREC_UMBRAL3 = 3;
const int FREC_LIMITE = 4;

const int FRECUENCIA_ALARMA[5] = {
  0,
  5000,   // Nivel 1: Frecuencia baja
  7000,  // Nivel 2: Frecuencia media
  9000,  // Nivel 3: Frecuencia alta
  12000   // Nivel 4: Frecuencia máxima
};
// FRECUENCIA_APAGADO = 0 ??

// ================================================
// CONFIGURACIÓN DE FREERTOS
// ================================================
#define QUEUE_SIZE        10     // Tamaño de la cola de eventos
#define STACK_SIZE        4096   // Tamaño del stack para las tareas
#define PRIORIDAD_TAREAS  1      // Prioridad de las tareas

// ================================================
// CONFIGURACIÓN DE EVENTOS
// ================================================

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

// === Variables globales ===
estado_t ESTADO_GLOBAL = ESTADO_IDLE;
stEvento evento;
QueueHandle_t queueEvents;
TaskHandle_t loopTaskHandler;
TaskHandle_t loopNewEventHandler;

float CONSUMO_GLOBAL = 0.0;
float CONSUMO_ACTUAL_CICLO = 0.0;  // Consumo leído una vez por ciclo de detección
float consumo_previo = 0.0;
unsigned long tiempo_inicio_valvula = 0;
unsigned long tiempo_inicio_alarma = 0;
float caudal_actual = 0.0;   // Caudal instantáneo en L/min
unsigned long tiempo_ultima_muestra = 0;

bool timeout_valvula_habilitado = false;
bool timeout_alarma_habilitado = false;
int NIVEL_ALARMA_ACTUAL = FREC_APAGADO;
bool ultimo_estado_pulsador = false;
bool estado_interruptor = false;

// Índice para round-robin de eventos
short indice_evento = 0;
long tiempo_ultimo_evento = 0;

// === Funciones auxiliares ===
void debug_print(const char* msg) {
  Serial.println(msg);
}

#define FACTOR_K 3.75  // Calibrar según tu sensor (7.5 para YF-S201 1/2")

// Variables para conteo de pulsos (volatile porque se usan en ISR)
volatile unsigned long pulsos_totales = 0;
volatile unsigned long pulsos_muestra = 0;

// ================================================
// RUTINA DE INTERRUPCIÓN (ISR)
// ================================================
void IRAM_ATTR contarPulsos() {
  pulsos_totales++;
  pulsos_muestra++;
}

// float leer_consumo() {
//   int valor_sensor = analogRead(PIN_A_CAUDALIMETRO);
//   float consumo_actual = valor_sensor * FACTOR_AJUSTE_SENSOR_A_CONSUMO_AGUA;
//   CONSUMO_GLOBAL += consumo_actual;
//   // consumo_previo = consumo_actual;
//   String msg = "Consumo Actual: " + String(consumo_actual);
//   debug_print(msg.c_str());
//   msg = "[SENSOR] Consumo Global: " + String(CONSUMO_GLOBAL);
//   debug_print(msg.c_str());
//   return CONSUMO_GLOBAL;
// }

// Función para calcular el caudal actual y actualizar el volumen
void actualizar_mediciones() {
  unsigned long tiempo_actual = millis();
  unsigned long tiempo_transcurrido = tiempo_actual - tiempo_ultima_muestra;
  
  // Actualizar cada segundo (o según TIEMPO_MUESTRA_CAUDAL_MS)
  if (tiempo_transcurrido >= TIEMPO_MUESTRA_CAUDAL_MS) {
    // Calcular frecuencia de pulsos en Hz
    float frecuencia = (pulsos_muestra * 1000.0) / tiempo_transcurrido;
    
    // Convertir frecuencia a caudal en L/min
    caudal_actual = frecuencia / FACTOR_K;
    
    // Calcular volumen incremental: Q(L/min) * tiempo(min)
    float tiempo_min = tiempo_transcurrido / 60000.0;
    float volumen_incremental = caudal_actual * tiempo_min;
    
    // Acumular el volumen
    CONSUMO_GLOBAL += volumen_incremental;
    // Debug info
    if (caudal_actual > 0.01) {  // Solo mostrar si hay flujo significativo
      String msg = "[SENSOR] Frecuencia: " + String(frecuencia, 2) + " Hz";
      debug_print(msg.c_str());
      msg = "[SENSOR] Caudal: " + String(caudal_actual, 3) + " L/min";
      debug_print(msg.c_str());
      msg = "[SENSOR] Volumen incremental: " + String(volumen_incremental, 4) + " L";
      debug_print(msg.c_str());
      msg = "[SENSOR] Consumo total: " + String(CONSUMO_GLOBAL, 3) + " L";
      debug_print(msg.c_str());
    }
    
    // Resetear contadores para la próxima muestra
    pulsos_muestra = 0;
    tiempo_ultima_muestra = tiempo_actual;
  }
}

float leer_consumo() {
  actualizar_mediciones();
  return CONSUMO_GLOBAL;
}

bool boton_presionado() {
  bool lectura_sensor = digitalRead(PIN_D_PULSADOR);
  if (ultimo_estado_pulsador == LOW && lectura_sensor == HIGH) {
    delay(50); // Debounce simple
    lectura_sensor = digitalRead(PIN_D_PULSADOR); // Verificar nuevamente
    if (lectura_sensor == HIGH) {
      estado_interruptor = !estado_interruptor;
    }
  }
  ultimo_estado_pulsador = lectura_sensor;
  return estado_interruptor;
}

void activar_electrovalvula() {
  digitalWrite(PIN_D_RELAY_ELECTROVALVULA, LOW); // Relay activo en LOW
  debug_print("[ACCION] Electroválvula activada");
}

void desactivar_electrovalvula() {
  digitalWrite(PIN_D_RELAY_ELECTROVALVULA, HIGH); // Relay inactivo en HIGH
  debug_print("[ACCION] Electroválvula desactivada");
}

void reproducir_alarma(int nivel) {
  if (nivel >= FREC_UMBRAL1 && nivel <= FREC_LIMITE) {
    tone(PIN_P_BUZZER, FRECUENCIA_ALARMA[nivel]);
    debug_print("[BUZZER] Alarma nivel " + nivel);
  }
}

void detener_alarma() {
  noTone(PIN_P_BUZZER);
  debug_print("[BUZZER] Alarma detenida");
}

// === Detección de eventos ===
typedef bool (*func_evento_t)(stEvento*);

bool detectar_button_on(stEvento* e) {
  if (ESTADO_GLOBAL == ESTADO_IDLE && boton_presionado()) { 
    debug_print("Botón presionado");
    // CHECKEAR: MANEJO DE ESTADO ACA O SOLO SWITCH?
    // PROBAR SI BOTON ASI FUNCIONA O AÑADIR LOGICA DE TOGGLE
    
    e->tipo = TIPO_EVENTO_BUTTON_ON;
    return true;
  }
  return false;
}

bool detectar_button_off(stEvento* e) {
  if ((ESTADO_GLOBAL == ESTADO_APERTURA_ELECTROVALVULA || ESTADO_GLOBAL == ESTADO_NORMAL) 
  && !boton_presionado()) {
        debug_print("Botón presionado off");
    e->tipo = TIPO_EVENTO_BUTTON_OFF;
    return true;
  }
  return false;
}

bool detectar_timeout_valvula(stEvento* e) { //CHECKEAR SI NECESARIO
  if (timeout_valvula_habilitado && 
      (millis() - tiempo_inicio_valvula > TIEMPO_ESPERA_VALVULA_MS)) {
    e->tipo = TIPO_EVENTO_TIMEOUT_VALVULA;
    timeout_valvula_habilitado = false;
    return true;
  }
  return false;
}

bool detectar_timeout_alarma(stEvento* e) {
  if (timeout_alarma_habilitado && 
      (millis() - tiempo_inicio_alarma > TIEMPO_BUZZER_MS)) {
    e->tipo = TIPO_EVENTO_TIMEOUT_ALARMA;
    timeout_alarma_habilitado = false;
    return true;
  }
  return false;
}

bool detectar_consumo_umbral1(stEvento* e) {
  if (ESTADO_GLOBAL == ESTADO_NORMAL) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_ACTUAL_CICLO >= UMBRAL1 && CONSUMO_ACTUAL_CICLO < UMBRAL2) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL1;
      return true;
    }
  }
  return false;
}

bool detectar_consumo_umbral2(stEvento* e) {
  if (ESTADO_GLOBAL == ESTADO_NORMAL || ESTADO_GLOBAL == ESTADO_ALARMA1) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_ACTUAL_CICLO >= UMBRAL2 && CONSUMO_ACTUAL_CICLO < UMBRAL3) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL2;
      return true;
    }
  }
  return false;
}

bool detectar_consumo_umbral3(stEvento* e) {
  if (ESTADO_GLOBAL == ESTADO_NORMAL || ESTADO_GLOBAL == ESTADO_ALARMA1 || ESTADO_GLOBAL == ESTADO_ALARMA2) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_ACTUAL_CICLO >= UMBRAL3 && CONSUMO_ACTUAL_CICLO < LIMITE) {
      e->tipo = TIPO_EVENTO_CONSUMO_UMBRAL3;
      return true;
    }
  }
  return false;
}

bool detectar_consumo_limite(stEvento* e) {
  if (ESTADO_GLOBAL == ESTADO_NORMAL || ESTADO_GLOBAL == ESTADO_ALARMA1 || 
      ESTADO_GLOBAL == ESTADO_ALARMA2 || ESTADO_GLOBAL == ESTADO_ALARMA3) {
    // Usar el consumo del ciclo actual (leído una sola vez)
    if (CONSUMO_ACTUAL_CICLO >= LIMITE) {
      e->tipo = TIPO_EVENTO_CONSUMO_LIMITE;
      return true;
    }
  }
  return false;
}

#define MAX_EVENTOS 8  // Número máximo de tipos de eventos
// Array de funciones detectoras de eventos
func_evento_t EVENTOS_POSIBLES[MAX_EVENTOS] = {
  detectar_button_on,
  detectar_button_off,
  detectar_timeout_valvula,
  detectar_timeout_alarma,
  detectar_consumo_umbral1,
  detectar_consumo_umbral2,
  detectar_consumo_umbral3,
  detectar_consumo_limite
};

void get_event() {
  long ahora = millis();
  
  // Actualizar mediciones UNA SOLA VEZ al inicio del ciclo
  // Así todas las funciones detectoras usan el mismo valor de consumo
  actualizar_mediciones();
  CONSUMO_ACTUAL_CICLO = CONSUMO_GLOBAL;
  
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "Consumo total: %.2f", CONSUMO_ACTUAL_CICLO);
  const char* mensaje = buffer;
  client.publish(topic, mensaje);
  
  // Round-robin para chequear eventos
  if ((ahora - tiempo_ultimo_evento) > INTERVALO_DETECCION_MS) {
    tiempo_ultimo_evento = ahora;

    // Intentar detectar el evento en el índice actual
    if (EVENTOS_POSIBLES[indice_evento](&evento)) {
      debug_print("[EVENTO] Evento detectado: " + evento.tipo);
      // Si se detectó un evento, enviarlo a la cola
      xQueueSend(queueEvents, &evento, portMAX_DELAY);
      indice_evento = (indice_evento + 1) % MAX_EVENTOS;
      return;
    }
    
    // Avanzar al siguiente evento
    indice_evento = (indice_evento + 1) % MAX_EVENTOS;
  }
  
  // Enviar evento CONTINUE si no hay otros eventos
  evento.tipo = TIPO_EVENTO_CONTINUE;
  xQueueSend(queueEvents, &evento, portMAX_DELAY);
}

// === Máquina de estados ===
void fsm() {
  stEvento ev;
  xQueueReceive(queueEvents, &ev, portMAX_DELAY);
  
  switch (ESTADO_GLOBAL) {
    case ESTADO_IDLE:
      if (ev.tipo == TIPO_EVENTO_BUTTON_ON) {
        debug_print("[FSM] IDLE -> APERTURA_ELECTROVALVULA");
        activar_electrovalvula();
        // reproducir_alarma(FREC_UMBRAL1); // ESTO NO VA
        tiempo_inicio_valvula = millis();
        timeout_valvula_habilitado = true;
        ESTADO_GLOBAL = ESTADO_APERTURA_ELECTROVALVULA;
      }
      break;
      
    case ESTADO_APERTURA_ELECTROVALVULA:
      switch (ev.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] APERTURA_ELECTROVALVULA -> IDLE");
          desactivar_electrovalvula();
          // detener_alarma(); ESTO NO VA
          // debug_print("[FSM] DETENIENDO ALARMA"); ESTO NO VA

          timeout_valvula_habilitado = false;
          ESTADO_GLOBAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_VALVULA: // CHECKEAR
          debug_print("[FSM] APERTURA_ELECTROVALVULA -> NORMAL");
          ESTADO_GLOBAL = ESTADO_NORMAL;
          break;
      }
      break;
      
    case ESTADO_NORMAL:
      switch (ev.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          desactivar_electrovalvula();
          ESTADO_GLOBAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL1:
          debug_print("[FSM] NORMAL -> ALARMA1");
          reproducir_alarma(FREC_UMBRAL1);
          tiempo_inicio_alarma = millis();
          timeout_alarma_habilitado = true;
          NIVEL_ALARMA_ACTUAL = FREC_UMBRAL1;
          ESTADO_GLOBAL = ESTADO_ALARMA1;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL2:
          debug_print("[FSM] NORMAL -> ALARMA2");
          reproducir_alarma(FREC_UMBRAL2);
          tiempo_inicio_alarma = millis();
          timeout_alarma_habilitado = true;
          NIVEL_ALARMA_ACTUAL = FREC_UMBRAL2;
          ESTADO_GLOBAL = ESTADO_ALARMA2;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL3:
          debug_print("[FSM] NORMAL -> ALARMA3");
          reproducir_alarma(FREC_UMBRAL3);
          tiempo_inicio_alarma = millis();
          timeout_alarma_habilitado = true;
          NIVEL_ALARMA_ACTUAL = FREC_UMBRAL3;
          ESTADO_GLOBAL = ESTADO_ALARMA3;
          break;
        case TIPO_EVENTO_CONSUMO_LIMITE:
          debug_print("[FSM] NORMAL -> ALARMA4");
          reproducir_alarma(FREC_LIMITE);
          tiempo_inicio_alarma = millis();
          timeout_alarma_habilitado = true;
          NIVEL_ALARMA_ACTUAL = FREC_LIMITE;
          ESTADO_GLOBAL = ESTADO_LIMITE;
          break;
      }
      break;
      
    case ESTADO_ALARMA1:
      switch (ev.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          desactivar_electrovalvula();
          ESTADO_GLOBAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA1 -> NORMAL");
          detener_alarma();
          ESTADO_GLOBAL = ESTADO_NORMAL;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL2:
          debug_print("[FSM] ALARMA1 -> ALARMA2");
          detener_alarma();
          reproducir_alarma(FREC_UMBRAL2);
          tiempo_inicio_alarma = millis();
          timeout_alarma_habilitado = true;
          NIVEL_ALARMA_ACTUAL = FREC_UMBRAL2;
          ESTADO_GLOBAL = ESTADO_ALARMA2;
          break;
      }
      break;
      
    case ESTADO_ALARMA2:
      switch (ev.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          desactivar_electrovalvula();
          ESTADO_GLOBAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA2 -> NORMAL");
          detener_alarma();
          ESTADO_GLOBAL = ESTADO_NORMAL;
          break;
        case TIPO_EVENTO_CONSUMO_UMBRAL3:
          debug_print("[FSM] ALARMA2 -> ALARMA3");
          detener_alarma();
          reproducir_alarma(FREC_UMBRAL3);
          tiempo_inicio_alarma = millis();
          timeout_alarma_habilitado = true;
          NIVEL_ALARMA_ACTUAL = FREC_UMBRAL3;
          ESTADO_GLOBAL = ESTADO_ALARMA3;
          break;
      }
      break;
      
    case ESTADO_ALARMA3:
      switch (ev.tipo) {
        case TIPO_EVENTO_BUTTON_OFF:
          debug_print("[FSM] NORMAL -> IDLE");
          desactivar_electrovalvula();
          ESTADO_GLOBAL = ESTADO_IDLE;
          break;
        case TIPO_EVENTO_TIMEOUT_ALARMA:
          debug_print("[FSM] ALARMA3 -> NORMAL");
          detener_alarma();
          ESTADO_GLOBAL = ESTADO_NORMAL;
          break;
        case TIPO_EVENTO_CONSUMO_LIMITE:
          debug_print("[FSM] ALARMA3 -> ALARMA4");
          detener_alarma();
          reproducir_alarma(FREC_LIMITE);
          tiempo_inicio_alarma = millis();
          timeout_alarma_habilitado = true;
          NIVEL_ALARMA_ACTUAL = FREC_LIMITE;
          ESTADO_GLOBAL = ESTADO_LIMITE;
          break;
      }
      break;
      
    case ESTADO_LIMITE:
      if (ev.tipo == TIPO_EVENTO_TIMEOUT_ALARMA) {
        debug_print("[FSM] ALARMA4 -> IDLE (LÍMITE ALCANZADO)");
        detener_alarma();
        desactivar_electrovalvula();
        ESTADO_GLOBAL = ESTADO_IDLE;
      }
      break;
  }
}

// === Tareas FreeRTOS ===
void vLoopTask(void *pvParameters) {
  while (1) {
    if (!client.connected()) {
      conectarMQTT();
    }
    client.loop();
    fsm();
  }
}

void vGetNewEventTask(void *pvParameters) {
  while (1) {
    get_event();
    vTaskDelay(pdMS_TO_TICKS(INTERVALO_TAREAS));
  }
}


void conectarWiFi() {
  Serial.print("Conectando a WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void conectarMQTT() {
  while (!client.connected()) {
    Serial.print("Conectando al broker MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println(" conectado!");
    } else {
      Serial.print(" fallo, rc=");
      Serial.print(client.state());
      Serial.println(" intentando de nuevo en 5 segundos");
      delay(5000);
    }
  }
}

// === Setup y Loop ===
void setup() {
  Serial.begin(115200);

  conectarWiFi();
  client.setServer(mqtt_server, mqtt_port);
  conectarMQTT();
  
    // Configurar pines
  pinMode(PIN_D_PULSADOR, INPUT_PULLDOWN);
  pinMode(PIN_D_RELAY_ELECTROVALVULA, OUTPUT);
  pinMode(PIN_P_BUZZER, OUTPUT);
  pinMode(PIN_A_CAUDALIMETRO, INPUT);
    
  // Estado inicial de actuadores
  digitalWrite(PIN_D_RELAY_ELECTROVALVULA, HIGH); // Relay recipiente
  // Configurar pines
  pinMode(PIN_D_PULSADOR, INPUT_PULLDOWN);
  pinMode(PIN_D_RELAY_ELECTROVALVULA, OUTPUT);
  pinMode(PIN_P_BUZZER, OUTPUT);
  pinMode(PIN_A_CAUDALIMETRO, INPUT);
  
  // Estado inicial de actuadores
  digitalWrite(PIN_D_RELAY_ELECTROVALVULA, HIGH); // Relay desactivado en HIGH
  noTone(PIN_P_BUZZER);

  debug_print("==============================================");
  debug_print("[SISTEMA] Medidor de Agua con Caudalímetro YF-S201");
  debug_print("==============================================");
  // debug_print("[CONFIG] Capacidad: " + String(CAPACIDAD_RECIPIENTE) + " L");
  // debug_print("[CONFIG] Umbral 1: " + String(UMBRAL1) + " L");
  // debug_print("[CONFIG] Umbral 2: " + String(UMBRAL2) + " L");
  // debug_print("[CONFIG] Umbral 3: " + String(UMBRAL3) + " L");
  // debug_print("[CONFIG] Límite: " + String(LIMITE) + " L");
  // debug_print("[CONFIG] Factor K: " + String(FACTOR_K));
  debug_print("==============================================");
  // Configurar interrupción para el caudalímetro
  // ESP32: usar attachInterrupt con el número de pin directamente
  attachInterrupt(digitalPinToInterrupt(PIN_A_CAUDALIMETRO), contarPulsos, RISING);
  
  // Crear cola de eventos
  queueEvents = xQueueCreate(QUEUE_SIZE, sizeof(stEvento));
  
   // Inicializar tiempo de última muestra
  tiempo_ultima_muestra = millis();
  // Crear tareas FreeRTOS
  xTaskCreate(
    vLoopTask,          // Función de la tarea
    "LoopFSM",          // Nombre de la tarea
    STACK_SIZE,         // Tamaño del stack
    NULL,               // Parámetros
    PRIORIDAD_TAREAS,   // Prioridad
    &loopTaskHandler    // Handle
  );
  
  xTaskCreate(
    vGetNewEventTask,
    "EventTask",
    STACK_SIZE,
    NULL,
    PRIORIDAD_TAREAS,
    &loopNewEventHandler
  );
  
  debug_print("[SISTEMA] Iniciando Medidor de Agua...");
  debug_print("[SISTEMA] Sistema iniciado correctamente");
  
}

void loop() {
  // Loop vacío - todo se maneja con FreeRTOS
}
