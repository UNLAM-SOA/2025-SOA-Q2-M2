package com.example.aguasmart;
import android.content.Intent;
import android.os.Bundle;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.snackbar.Snackbar;

public class MainActivity extends AppCompatActivity {

    // Botones
    private Button btnVerConsumo;
    private Button btnValvula;
    private Button btnMqttTest;
    private boolean valvulaActiva = true;


    ///  MQTT
    private MqttHandler mqttHandler;
    private static final String BROKER_URI = "tcp://broker.hivemq.com:1883";
    private static final String TOPIC = "/pruebita";

    /// //////
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

        ///  PRUEBA MQTT //////////////////////////////

        // Este botón publica el mensaje MQTT
        btnMqttTest.setOnClickListener(v -> {
            try {
                mqttHandler.publish(TOPIC, "Hola desde Android!");
                Toast.makeText(this, "Mensaje enviado a " + TOPIC, Toast.LENGTH_SHORT).show();
            } catch (Exception e) {
                Toast.makeText(this, "Error enviando mensaje: " + e.getMessage(), Toast.LENGTH_SHORT).show();
            }
        });

        actualizarEstadoBoton();
        /// ////////////////////////////////////////////
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
    private void connectMqtt() {
        mqttHandler.connect(BROKER_URI, "AndroidClient", null, null);

        // Esperar conexión antes de publicar
        new Thread(() -> {
            try {
                // Esperamos unos segundos para asegurar conexión
                Thread.sleep(2000);
                runOnUiThread(() -> {
                    Toast.makeText(this, "Conectado a MQTT", Toast.LENGTH_SHORT).show();
                    configurarBoton();
                });
            } catch (InterruptedException e) {
                e.printStackTrace();
            }
        }).start();
    }

    private void configurarBoton() {
        btnMqttTest.setOnClickListener(v -> {
            try {
                mqttHandler.publish(TOPIC, "Hola desde Android!");
                Toast.makeText(this, "Mensaje enviado a " + TOPIC, Toast.LENGTH_SHORT).show();
            } catch (Exception e) {
                Toast.makeText(this, "Error enviando mensaje: " + e.getMessage(), Toast.LENGTH_SHORT).show();
            }
        });
    }
}
