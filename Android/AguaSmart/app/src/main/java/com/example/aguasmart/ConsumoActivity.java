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

     static final float MAX_CAPACITY = 10f;
    private static final float THRESHOLD_3 = MAX_CAPACITY * 0.75f; // 7.5 L
    private static final float THRESHOLD_4 = MAX_CAPACITY;          // 10 L


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
        gauge.addSections(new Section(0f, 2.5f, Color.parseColor("#4CAF50")));   // Verde
        gauge.addSections(new Section(2.5f, 5f, Color.parseColor("#FFEB3B")));  // Amarillo
        gauge.addSections(new Section(5f, 7.5f, Color.parseColor("#FF9800")));  // Naranja
        gauge.addSections(new Section(7.5f, maxCapacity, Color.parseColor("#F44336")));  // Rojo



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

                // --- Alertas por umbrales ---
                if (consumoFloat >= THRESHOLD_3 && consumoFloat < THRESHOLD_4) {
                    enviarNotificacionConsumo("⚠️ Nivel crítico", "Nivel de agua alcanzó el umbral 3 (75%)");
                } else if (consumoFloat >= THRESHOLD_4) {
                    enviarNotificacionConsumo("🚨 Nivel máximo alcanzado", "La válvula se desactivará automáticamente");
                }
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

    private void enviarNotificacionConsumo(String titulo, String texto) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
                ActivityCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            Log.w("ConsumoActivity", "No se puede enviar notificación: permiso denegado");
            return;
        }

        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, MyApp.CHANNEL_ID_ALERTAS)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentTitle(titulo)
                .setContentText(texto)
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .setAutoCancel(true)
                .setContentIntent(pendingIntent);

        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        manager.notify((int) System.currentTimeMillis(), builder.build()); // ID único
    }

}
