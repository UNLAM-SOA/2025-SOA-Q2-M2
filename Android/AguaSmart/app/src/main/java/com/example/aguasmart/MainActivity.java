package com.example.aguasmart;

import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import com.google.android.material.snackbar.Snackbar;

public class MainActivity extends AppCompatActivity implements MqttListener  {

    private Button btnVerConsumo;
    private Button btnValvula;
    private Button btnMqttTest;

    private boolean valvulaActiva = true;

    private com.example.aguasmart.mqtt.MqttHandler mqttHelper;

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

        actualizarEstadoBoton();
        btnValvula.setOnClickListener(v -> {
            valvulaActiva = !valvulaActiva;
            actualizarEstadoBoton();
            String mensaje = valvulaActiva ? "Válvula activada" : "Válvula desactivada";
            Snackbar.make(v, mensaje, Snackbar.LENGTH_SHORT).show();

            ///  PRUEBA MQTT //////////////////////////////
            mqttHelper = new com.example.aguasmart.mqtt.MqttHandler(this, this);

            btnMqttTest.setOnClickListener(vm -> {
                mqttHelper.publish("/pruebita", "MENSAJE DESDE ANDROID");
                Toast.makeText(this, "Mensaje publicado", Toast.LENGTH_SHORT).show();
            });

            btnVerConsumo.setOnClickListener(vm -> {
                startActivity(new Intent(MainActivity.this, ConsumoActivity.class));
            });

            actualizarEstadoBoton();
            btnValvula.setOnClickListener(vm -> {
                valvulaActiva = !valvulaActiva;
                actualizarEstadoBoton();
                String mensajeValvula = valvulaActiva ? "Válvula activada" : "Válvula desactivada";
                mqttHelper.publish("/pruebita", mensajeValvula);
                Snackbar.make(v, mensajeValvula, Snackbar.LENGTH_SHORT).show();
            });


        /// ////////////////////////////////////////////
        });
    }


    /// / MQTT PRUEBA ////
    @Override
    public void onConnected() {
        Log.d("MQTT", "Conectado correctamente al broker HiveMQ");
    }

    @Override
    public void onMessageReceived(String topic, String message) {
        Log.d("MQTT", "Mensaje recibido en MainActivity [" + topic + "]: " + message);
    }
    ///
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

}
