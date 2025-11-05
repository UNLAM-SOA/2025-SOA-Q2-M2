package com.example.aguasmart;
import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.snackbar.Snackbar;

public class MainActivity extends AppCompatActivity {

    // Botones
    private Button btnVerConsumo;
    private Button btnValvula;
    private Button btnMqttTest;
    private boolean valvulaActiva = true;
    private static final String TAG = "MAIN ACTIVITY";

    private BroadcastReceiver mqttReceiver;


    /// //////
    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnVerConsumo = findViewById(R.id.btnVerConsumo);
        btnValvula = findViewById(R.id.btnValvula);
        btnMqttTest = findViewById(R.id.btnMqttTest);

        btnVerConsumo.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, ConsumoActivity.class);
            startActivity(intent);
        });

        btnValvula.setOnClickListener(v -> {
            String comando = valvulaActiva ? "OFF" : "ON"; // querés cambiar el estado
            enviarMensajeMqtt(this, comando, MqttService.TOPIC_VALVULA_CMD);

            Snackbar.make(v, "⏳ Esperando confirmación del ESP32...", Snackbar.LENGTH_SHORT).show();
        });

        actualizarEstadoBoton();

        // --- CREAR EL BROADCAST RECEIVER ---
        mqttReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String message = intent.getStringExtra(MqttService.MQTT_MESSAGE_KEY);

                if (message == null) return;

                if (message.startsWith("VALVULA:")) {
                    String estado = message.replace("VALVULA:", "").trim();

                    switch (estado) {
                        case "ON_OK":
                            valvulaActiva = true;
                            actualizarEstadoBoton();
                            Snackbar.make(findViewById(android.R.id.content),
                                    "✅ Válvula ACTIVADA",
                                    Snackbar.LENGTH_SHORT).show();
                            break;

                        case "OFF_OK":
                            valvulaActiva = false;
                            actualizarEstadoBoton();
                            Snackbar.make(findViewById(android.R.id.content),
                                    "✅ Válvula DESACTIVADA",
                                    Snackbar.LENGTH_SHORT).show();
                            break;

                        default:
                            // Si llegara algo raro, lo logueamos
                            Log.w("MAIN", "⚠ Estado de válvula desconocido: " + estado);
                            break;
                    }

                    return;
                }

                if (message.startsWith("CONSUMO:")) {
                    String consumo = message.replace("CONSUMO:", "").trim();
                    Log.d("MAIN", "Nuevo consumo: " + consumo);
                    // Acá se actualiza la pantalla de consumo

                    return;
                }
            }
        };

        // REGISTRAR EL RECEIVER (esto es lo que te faltaba ubicar)
        IntentFilter filter = new IntentFilter(MqttService.MQTT_MESSAGE_BROADCAST);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(mqttReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(mqttReceiver, filter);
        }

        // Iniciar el servicio MQTT
        Intent mqttServiceIntent = new Intent(this, MqttService.class);
        startService(mqttServiceIntent);

        ///  PRUEBA MQTT //////////////////////////////
        // Botón publicar MQTT
        btnMqttTest.setOnClickListener(v -> {
            Log.d("BTN", "Presionaste el botón MQTT");
            enviarMensajeMqtt(this, "Hola desde Android!", MqttService.TOPIC_TEST);
        });
        //////////////////////////////////////////

    }

    private void actualizarEstadoBoton() {
        if (valvulaActiva) {
            btnValvula.setText("Desactivar válvula");
            btnValvula.setBackgroundTintList(getColorStateList(android.R.color.holo_red_dark));
            btnValvula.setTextColor(getColor(android.R.color.white));
        } else {
            btnValvula.setText("Activar válvula");
            btnValvula.setBackgroundTintList(getColorStateList(android.R.color.holo_green_dark));
            btnValvula.setTextColor(getColor(android.R.color.white));
        }
    }


    private void enviarMensajeMqtt(Context context, String message, String topic) {
        Intent intent = new Intent(context, MqttService.class);
        intent.putExtra("publish", message);
        intent.putExtra("topic", topic);
        context.startService(intent);
    }
    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mqttReceiver);

        // Para que el MQTT NO siga corriendo en background cuando la app se cierre:
        stopService(new Intent(this, MqttService.class));
        // Si SÍ querés que siga — simplemente elimina la línea de arriba.
    }
}
