package com.example.aguasmart;

import static com.example.aguasmart.MqttService.TOPIC_CONSUMO;

import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import com.github.anastr.speedviewlib.SpeedView;
import com.github.anastr.speedviewlib.components.Section;


public class ConsumoActivity extends AppCompatActivity {

    private TextView tvConsumo;
    private BroadcastReceiver consumoReceiver;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_consumo);

        tvConsumo = findViewById(R.id.tvConsumo);

        SpeedView gauge = findViewById(R.id.gaugeConsumo);

        // Limpiar secciones por si vienen por defecto

        gauge.setSpeedometerWidth(100);
        gauge.setWithTremble(false);

        float maxCapacity = 10f;
        gauge.setMaxSpeed(maxCapacity); // maxima capacidad del medidor

        // Zonas del medidor
        gauge.clearSections();
        gauge.addSections(
                new Section(0f, 0.25f, Color.parseColor("#4CAF50")),   // Verde 0–25%
                new Section(0.25f, 0.50f, Color.parseColor("#FFEB3B")), // Amarillo 25–50%
                new Section(0.50f, 0.75f, Color.parseColor("#FF9800")), // Naranja 50–75%
                new Section(0.75f, 1f, Color.parseColor("#F44336"))     // Rojo 75–100%
        );

        // Botón volver
        findViewById(R.id.btnVolverMenu).setOnClickListener(v -> {
            startActivity(new Intent(this, MainActivity.class));
            finish();
        });

        // Receiver de consumo
        consumoReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {

                String topic = intent.getStringExtra("topic");
                String payload = intent.getStringExtra("payload");

                if (topic == null || payload == null || !topic.equals(TOPIC_CONSUMO)) return;


                float consumoFloat = 0f;
                try {
                    consumoFloat = Float.parseFloat(payload);
                } catch (Exception e) {
                    return; // ignorar mensajes que no sean un número
                }

                // Actualizar el medidor
                gauge.speedTo(consumoFloat, 0);

                // Actualiza UI
                tvConsumo.setText(payload + " L");

            }
        };
    }

    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    @Override
    protected void onResume() {
        super.onResume();
        IntentFilter filter = new IntentFilter(MqttService.MQTT_MESSAGE_BROADCAST);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(consumoReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(consumoReceiver, filter);
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(consumoReceiver);
    }

}
