package com.example.aguasmart;

import android.content.Context;
import android.content.Intent;
import android.util.Log;

import org.eclipse.paho.android.service.MqttAndroidClient;
import org.eclipse.paho.client.mqttv3.IMqttActionListener;
import org.eclipse.paho.client.mqttv3.IMqttDeliveryToken;
import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttCallbackExtended;
import org.eclipse.paho.client.mqttv3.MqttClient;
import org.eclipse.paho.client.mqttv3.MqttMessage;

public class MqttHandler {

    public static final String ACTION_DATA_RECEIVE = "com.example.aguasmart.DATA_RECEIVE";
    public static final String ACTION_CONNECTION_LOST = "com.example.aguasmart.CONNECTION_LOST";

    private MqttAndroidClient mqttAndroidClient;
    private Context context;

    public MqttHandler(Context context) {
        this.context = context;
    }

    public void connect(String serverUri, String clientId, String username, String password) {
        mqttAndroidClient = new MqttAndroidClient(context, serverUri,
                clientId + System.currentTimeMillis());

        mqttAndroidClient.setCallback(new MqttCallbackExtended() {
            @Override
            public void connectComplete(boolean reconnect, String serverURI) {
                Log.d("MQTT", "Conectado a: " + serverURI);
            }

            @Override
            public void connectionLost(Throwable cause) {
                Log.e("MQTT", "Conexión perdida", cause);
                context.sendBroadcast(new Intent(ACTION_CONNECTION_LOST));
            }

            @Override
            public void messageArrived(String topic, MqttMessage message) {
                String msg = new String(message.getPayload());
                Log.d("MQTT", "Mensaje recibido: " + msg);

                Intent intent = new Intent(ACTION_DATA_RECEIVE);
                intent.putExtra("msgJson", msg);
                context.sendBroadcast(intent);
            }

            @Override
            public void deliveryComplete(IMqttDeliveryToken token) {
                Log.d("MQTT", "Mensaje entregado");
            }
        });

        try {
            IMqttToken token = mqttAndroidClient.connect();
            token.setActionCallback(new IMqttActionListener() {
                @Override
                public void onSuccess(IMqttToken asyncActionToken) {
                    Log.d("MQTT", "Conexión exitosa");
                }

                @Override
                public void onFailure(IMqttToken asyncActionToken, Throwable exception) {
                    Log.e("MQTT", "Error al conectar", exception);
                }
            });
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void publish(String topic, String payload) {
        try {
            MqttMessage message = new MqttMessage();
            message.setPayload(payload.getBytes());
            mqttAndroidClient.publish(topic, message);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void subscribe(String topic) {
        try {
            mqttAndroidClient.subscribe(topic, 0);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void disconnect() {
        try {
            mqttAndroidClient.disconnect();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
}
