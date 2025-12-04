package com.example.aguasmart;


import android.annotation.SuppressLint;
import android.content.Context;
import android.content.Intent;
import android.graphics.Color;
import android.hardware.Sensor;
import android.hardware.SensorEvent;
import android.hardware.SensorEventListener;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.os.VibrationEffect;
import android.os.Vibrator;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import android.os.CountDownTimer;


import androidx.appcompat.app.AppCompatActivity;

public class ShakeActivity extends AppCompatActivity implements SensorEventListener {

    private SensorManager sensorManager;
    private Sensor accelerometer;
    private TextView tvEstadoShake;
    private CountDownTimer timerVisual;

    // Variables para controlar el "Shake"
    private static final String TAG = "SHAKE";
    private static final float SHAKE_THRESHOLD_GRAVITY = 3F; // Sensibilidad (3G)
    private static final int MIN_TIME_BETWEEN_SHAKES_MS = 5000; // Evitar enviar muchos mensajes por segundo
    private long lastShakeTime = 0;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_shake);

        tvEstadoShake = findViewById(R.id.tvEstadoShake);
        Button btnVolver = findViewById(R.id.btnVolver);

        // Inicializar Sensor Manager
        sensorManager = (SensorManager) getSystemService(Context.SENSOR_SERVICE);
        if (sensorManager != null) {
            accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER);
        }

        // Botón para volver atrás
        // Recordar la pila de platos... Como en mainActivity ya hice un item
        //significa que la pila queda mainActivity -> shakeActivity.
        //Si hago un intent aca para volver atras, estaria poniendo un nuevo plato arriba de shake, no sería lo correcto.
        //Uso finish para sacar el plato de shake y que quede unicamente mainActivity.
        btnVolver.setOnClickListener(v -> finish());
    }

    @Override
    protected void onResume() {
        super.onResume();
        // Registrar el listener cuando la actividad está visible
        if (sensorManager != null && accelerometer != null) {
            sensorManager.registerListener(this, accelerometer, SensorManager.SENSOR_DELAY_UI);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        // Importante: Des-registrar para ahorrar batería cuando salimos
        if (sensorManager != null) {
            sensorManager.unregisterListener(this);
        }
    }

    // --- NUEVO: Evitamos crash si el usuario se va mientras cuenta ---
    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (timerVisual != null) {
            timerVisual.cancel();
        }
    }

    @Override
    public void onSensorChanged(SensorEvent event) {
        //Utilizamos onSensorChanged para verificar cambios en los parametros del sensor
        if (event.sensor.getType() == Sensor.TYPE_ACCELEROMETER) {

            float x = event.values[0];
            float y = event.values[1];
            float z = event.values[2];

            // Cálculo de Fuerza G: sqrt(x² + y² + z²) / Gravedad
            float gX = x / SensorManager.GRAVITY_EARTH;
            float gY = y / SensorManager.GRAVITY_EARTH;
            float gZ = z / SensorManager.GRAVITY_EARTH;

            // Fuerza total (Vectorial)
            float gForce = (float) Math.sqrt(gX * gX + gY * gY + gZ * gZ);

            // Si la fuerza supera el umbral (agitación fuerte)
            if (gForce > SHAKE_THRESHOLD_GRAVITY) {
                long currentTime = System.currentTimeMillis();

                // Verificar que haya pasado tiempo desde el último shake
                if ((currentTime - lastShakeTime) > MIN_TIME_BETWEEN_SHAKES_MS) {
                    lastShakeTime = currentTime;

                    ejecutarAccionShake();
                }
            }
        }
    }

    @Override
    public void onAccuracyChanged(Sensor sensor, int accuracy) {
        // No es necesario implementar esto para este caso
    }

    @SuppressLint("SetTextI18n")
    private void ejecutarAccionShake() {
        tvEstadoShake.setText("¡SHAKE DETECTADO! Enviando msj...");
        tvEstadoShake.setTextColor(Color.RED);
        Toast.makeText(this, "Enviando mensaje al Broker", Toast.LENGTH_SHORT).show();

        //En este punto le decimos que al dispositivo que se manifieste con una vibración física si recibio el evento.
        Vibrator v = (Vibrator) getSystemService(Context.VIBRATOR_SERVICE);
        if (v != null) {
            v.vibrate(VibrationEffect.createOneShot(500, VibrationEffect.DEFAULT_AMPLITUDE));
        }

        // Usamos la misma lógica que tenemos en MainActivity para usar MqttService
        //Aca llamamos a MqttService y TOPIC_MSJ_SHAKE es la constante que contiene la ruta de comunicación con MQTT.
        //Lo almacenamos en topic
        String topic = MqttService.TOPIC_MSJ_SHAKE;
        //Aca almacenamos en msg el string que viaja al topic.
        String msg = "ALERTA_MOVIMIENTO_DETECTADO";

        //Invocamos a la función que nos permite enviar el mensaje al topico de MQTT.
        //Le pasamos el contexto, el mensaje y el topico.
        enviarMensajeMqtt(this, msg, topic);

        //Este mensaje es un aviso ni bien se produce el Shake, que es un aviso indicando que el mensaje
        //se esta enviando al Broker ni bien se produce el shake.
        //No come ciclos de CPU
        //Es como poner una alarma en tu celular para dentro de 1 segundo y en el medio hace otra cosa.
        timerVisual = new CountDownTimer(MIN_TIME_BETWEEN_SHAKES_MS, 1000) {
          public void onTick(long millisUntilFinished) {
              //millisUntilFinished se ejecuta automáticamente cada vez que pasa el intervalo que definiste
              //devuelve el tiempo qu falta para terminar
              long segundos = millisUntilFinished / 1000;
              tvEstadoShake.setText("Proximo envío posible en: " + segundos + " segundos.");

          }
          public void onFinish() {
              //Esto se ejecuta cuando el tiemp llega a 0...
              tvEstadoShake.setText("Esperando Shake...");
              tvEstadoShake.setTextColor(Color.parseColor("#4CAF50")); // Verde
          }
        }.start();
    }

    // -----------------------------------------
    //                 MQTT
    // -----------------------------------------
    //función que me permite unificar el envío de mensaje a MQTT.
    private void enviarMensajeMqtt(Context context, String message, String topic) {
        Intent intent = new Intent(context, MqttService.class);
        intent.putExtra("publish", message);
        intent.putExtra("topic", topic);
        context.startService(intent);
    }
}

