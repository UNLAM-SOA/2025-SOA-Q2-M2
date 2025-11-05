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
            valvulaActiva = !valvulaActiva;
            actualizarEstadoBoton();
            String mensaje = valvulaActiva ? "Válvula activada" : "Válvula desactivada";
            Snackbar.make(v, mensaje, Snackbar.LENGTH_SHORT).show();
        });
        actualizarEstadoBoton();

        ///  PRUEBA MQTT //////////////////////////////
        // --- CREAR EL BROADCAST RECEIVER ---
        mqttReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String message = intent.getStringExtra(MqttService.MQTT_MESSAGE_KEY);
                Log.d(TAG, "Mensaje recibido");
                if (message != null) {
                    Toast.makeText(context, "Mensaje MQTT: " + message, Toast.LENGTH_LONG).show();
                }
            }
        };

        // ✅ REGISTRAR EL RECEIVER (esto es lo que te faltaba ubicar)
        IntentFilter filter = new IntentFilter(MqttService.MQTT_MESSAGE_BROADCAST);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(mqttReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(mqttReceiver, filter);
        }

        // ✅ Iniciar el servicio MQTT
        Intent mqttServiceIntent = new Intent(this, MqttService.class);
        startService(mqttServiceIntent);

        // ✅ Botón publicar MQTT
        btnMqttTest.setOnClickListener(v -> {
            Log.d("BTN", "Presionaste el botón MQTT");
            enviarMensajeMqtt(this, "Hola desde Android!");
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

    /// / MQTT PRUEBA ////
    private void enviarMensajeMqtt(Context context, String message) {
        Intent intent = new Intent(context, MqttService.class);
        intent.putExtra("publish", message);
        context.startService(intent);
    }
    /// ////


    @Override
    protected void onDestroy() {
        super.onDestroy();
        unregisterReceiver(mqttReceiver);

        // Para que el MQTT NO siga corriendo en background cuando la app se cierre:
        stopService(new Intent(this, MqttService.class));
        // Si SÍ querés que siga — simplemente elimina la línea de arriba.
    }
}
