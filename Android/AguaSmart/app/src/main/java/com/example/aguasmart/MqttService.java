package com.example.aguasmart;

import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;
import android.os.Handler;

import androidx.annotation.Nullable;
import androidx.core.app.NotificationCompat;

import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.eclipse.paho.client.mqttv3.MqttException;
import org.eclipse.paho.client.mqttv3.MqttMessage;


public class MqttService extends Service {

    private static final String TAG = "AGUA_MQTT";
    public static final String MQTT_MESSAGE_BROADCAST = "com.example.aguasmart.MQTT_MESSAGE";
    private static final String BROKER_URL = "tcp://broker.emqx.io:1883"; //broker alternativo
    //private static final String BROKER_URL = "tcp://broker.hivemq.com:1883";

    // TOPICS ///////////////////////////////////////////
    public static final String TOPIC_TEST = "pruebita";

    public static final String TOPIC_CONSUMO = "aguasmart/consume";
    public static final String TOPIC_VALVULA_CMD = "aguasmart/valve/cmd"; // Android → ESP32
    public static final String TOPIC_VALVULA_STATE = "aguasmart/valve/state"; // ESP32 → Android
    /// ////////////////////////////////////////////////
    ///
    /// NOTIFICACIONES DE CONSUMO
    private static final float MAX_CAPACITY = 10f;
    private static final float THRESHOLD_2 = MAX_CAPACITY * 0.50f;
    private static final float THRESHOLD_3 = MAX_CAPACITY * 0.75f;
    private static final float THRESHOLD_4 = MAX_CAPACITY;
    ///
    private MqttClient mqttClient;
    private final Handler handler = new Handler();
    private boolean isReconnecting = false;
    private static final int RECONNECT_DELAY_MS = 5000; // 5 segundos
    private final IBinder binder = new LocalBinder();


    @Override
    public void onCreate() {
        super.onCreate();
        connect();
    }

    private void connect() {
        new Thread(() -> {
            try{
                String clientId = MqttClient.generateClientId();
                mqttClient = new MqttClient(BROKER_URL, clientId, null);

                mqttClient.setCallback(new MqttCallbackExtended() {
                    @Override
                    public void connectComplete(boolean reconnect, String serverURI) {
                        Log.d(TAG, "Conectado a: " + serverURI);
                    }

                    @Override
                    public void connectionLost(Throwable cause) {
                        Log.w(TAG, "Conexión perdida", cause);
                        scheduleReconnect();
                    }

                    @Override
                    public void messageArrived(String topic, MqttMessage message) {
                        String msg = new String(message.getPayload());
                        Log.d(TAG, "Mensaje recibido → topic: " + topic + " payload: " + msg);
                        if (topic.equals(TOPIC_CONSUMO)) {
                            procesarConsumo(msg);
                        }
                        Intent intent = new Intent(MQTT_MESSAGE_BROADCAST);

                        // Enviar topic + mensaje
                        intent.putExtra("topic", topic);
                        intent.putExtra("payload", msg);

                        intent.setPackage(getPackageName());
                        sendBroadcast(intent);
                    }

                    @Override
                    public void deliveryComplete(IMqttDeliveryToken token) {
                        Log.d(TAG, "Mensaje entregado");
                    }
                });

                MqttConnectOptions options = new MqttConnectOptions();
                options.setCleanSession(true);

                mqttClient.connect(options);
                mqttClient.subscribe(TOPIC_TEST, 0);
                mqttClient.subscribe(TOPIC_VALVULA_CMD, 0);
                mqttClient.subscribe(TOPIC_VALVULA_STATE, 0);
                mqttClient.subscribe(TOPIC_CONSUMO, 0);

                //Log.i(TAG, "Conectado y suscrito al tópico: " + TOPIC_TEST);
                Log.i(TAG, "Conectado y suscrito al tópico: " + TOPIC_CONSUMO);
                Log.i(TAG, "Conectado y suscrito al tópico: " + TOPIC_VALVULA_CMD);
                Log.i(TAG, "Conectado y suscrito al tópico: " + TOPIC_VALVULA_STATE);
                isReconnecting = false;

            } catch (MqttException e) {
                Log.e(TAG, "Error conectando al broker MQTT", e);
                scheduleReconnect();
            } catch (Exception e) {
                Log.e(TAG, "Error inesperado en MQTT", e);
                scheduleReconnect();
            }
        }).start();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {

        if (intent != null && intent.hasExtra("publish")) {
            String msg = intent.getStringExtra("publish");
            String topic = intent.getStringExtra("topic");
            publish(msg, topic);
        }

        return START_STICKY; // Mantiene la conexión MQTT viva aunque la app pase a background
    }

    private void scheduleReconnect() {
        if (isReconnecting) return; // Evitar múltiples reconexiones simultáneas
        isReconnecting = true;

        Log.w(TAG, "Intentando reconectar en " + RECONNECT_DELAY_MS / 1000 + " segundos...");

        handler.postDelayed(() -> {
            Log.i(TAG, "Reintentando conexión MQTT...");
            connect();
        }, RECONNECT_DELAY_MS);
    }

    public void publish(String message, String topic) {
        try {
            if (mqttClient != null && mqttClient.isConnected()) {
                MqttMessage mqttMessage = new MqttMessage(message.getBytes());
                mqttClient.publish(topic, mqttMessage);
                Log.d(TAG, "Mensaje publicado: " + message);
            } else {
                Log.w(TAG, "No se puede publicar, MQTT no conectado");
            }
        } catch (MqttException e) {
            Log.e(TAG, "Error publicando mensaje MQTT", e);
        } catch (Exception e) {
            Log.e(TAG, "Error inesperado publicando mensaje MQTT", e);
        }
    }


    @Override
    public void onDestroy() {
        handler.removeCallbacksAndMessages(null);
        try {
            if (mqttClient != null && mqttClient.isConnected()) {
                mqttClient.disconnect();
                Log.d(TAG, "Desconectado del broker MQTT");
            }
        } catch (MqttException e) {
            Log.e(TAG, "Error al desconectar MQTT", e);
        } catch (Exception e) {
            Log.e(TAG, "Error inesperado al desconectar MQTT", e);
        }
        super.onDestroy();
    }
    public class LocalBinder extends Binder {
        public MqttService getService() {
            return MqttService.this;
        }
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return binder;
    }

    ///  NOTIFICACIONES DE CONSUMO ////
    private void procesarConsumo(String payload) {
        float consumo;
        try {
            consumo = Float.parseFloat(payload);
        } catch (Exception e) {
            return;
        }

        Log.d(TAG, "Consumo procesado en service: " + consumo);

        if (consumo >= THRESHOLD_2 && consumo < THRESHOLD_3) {
            enviarNotificacion("⚠️ Nivel moderado", "El nivel superó el 50%");
        }
        else if (consumo >= THRESHOLD_3 && consumo < THRESHOLD_4) {
            enviarNotificacion("⚠️ Nivel crítico", "El nivel superó el 75%");
        }
        else if (consumo >= THRESHOLD_4) {
            enviarNotificacion("🚨 Nivel máximo", "La válvula se apagará automáticamente");
        }
    }

    private void enviarNotificacion(String titulo, String texto) {
        Intent intent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(
                this, 0, intent, PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, MyApp.CHANNEL_ID_ALERTAS)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentTitle(titulo)
                .setContentText(texto)
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .setAutoCancel(true)
                .setContentIntent(pendingIntent);

        NotificationManager manager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
        manager.notify((int) System.currentTimeMillis(), builder.build());
    }



}
