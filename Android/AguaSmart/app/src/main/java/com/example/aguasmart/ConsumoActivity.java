package com.example.aguasmart;

import androidx.appcompat.app.AppCompatActivity;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Random;

public class ConsumoActivity extends AppCompatActivity {

    private TextView tvConsumo;
    private Button btnActualizar;
    private BroadcastReceiver consumoReceiver;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_consumo);

        tvConsumo = findViewById(R.id.tvConsumo);

        // Botón volver
        findViewById(R.id.btnVolverMenu).setOnClickListener(v -> {
            startActivity(new Intent(this, MainActivity.class));
            finish();
        });

        // Botón actualizar manual (opcional)
        findViewById(R.id.btnActualizar).setOnClickListener(v -> {
            Toast.makeText(this, "Esperando datos del ESP32...", Toast.LENGTH_SHORT).show();
        });

        // Receiver de consumo
        consumoReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {

                String message = intent.getStringExtra(MqttService.MQTT_MESSAGE_KEY);
                if (message == null || !message.startsWith("CONSUMO:")) return;

                String valor = message.replace("CONSUMO:", "").trim();

                // Actualiza UI
                tvConsumo.setText(String.format(" %s L", valor));
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
