package com.example.aguasmart;

import static com.example.aguasmart.GpsService.isServiceRunning;

import android.Manifest;
import android.annotation.SuppressLint;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;
import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.content.ContextCompat;

import com.google.android.gms.tasks.CancellationToken;
import com.google.android.gms.tasks.OnTokenCanceledListener;
import com.google.android.material.snackbar.Snackbar;
import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.Priority;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    // -----------------------------------------
    //              CONSTANTES
    // -----------------------------------------

    private static final String TAG = "MAIN ACTIVITY";
    private static final int NOTIFICATION_PERMISSION_REQUEST_CODE = 456;
    private static final int PERMISSIONS_REQUEST_CODE = 123;

    // -----------------------------------------
    //               UI ELEMENTOS
    // -----------------------------------------

    private Button btnVerConsumo;
    private Button btnValvula;
    private Button btnFijarUbicacion;

    // -----------------------------------------
    //               VARIABLES
    // -----------------------------------------

    private boolean valvulaActiva = false;
    private BroadcastReceiver mqttReceiver;
    private FusedLocationProviderClient locationClient;

    // -----------------------------------------
    //                 CICLO DE VIDA
    // -----------------------------------------

    @SuppressLint({"UnspecifiedRegisterReceiverFlag", "MissingInflatedId"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.d(TAG, "¡onCreate() iniciado!");

        locationClient = LocationServices.getFusedLocationProviderClient(this);

        inicializarUI();
        registrarReceiverGps();
        configurarListeners();
        cargarEstadoValvula();

        Log.d(TAG, "Chequeando permisos...");
        if (checkAndRequestPermissions()) {
            Log.d(TAG, "Permisos OK → iniciando servicios y registrando MQTT Receiver");
            startServices();
            registrarReceiverMqtt();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();

        if (mqttReceiver != null) unregisterReceiver(mqttReceiver);
        unregisterReceiver(gpsRangeReceiver);
    }

    // -----------------------------------------
    //      INICIALIZACIÓN Y CONFIGURACIONES
    // -----------------------------------------

    private void inicializarUI() {
        btnVerConsumo = findViewById(R.id.btnVerConsumo);
        btnValvula = findViewById(R.id.btnValvula);
        btnFijarUbicacion = findViewById(R.id.btnFijarUbicacion);
    }

    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    private void registrarReceiverGps() {
        IntentFilter gpsFilter = new IntentFilter("GPS_Rango_Alerta");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(gpsRangeReceiver, gpsFilter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(gpsRangeReceiver, gpsFilter);
        }
    }

    private void configurarListeners() {

        btnVerConsumo.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, ConsumoActivity.class);
            startActivity(intent);
        });

        btnValvula.setOnClickListener(v -> {
            enviarMensajeMqtt(this, "button_push", MqttService.TOPIC_VALVULA_CMD);

            btnValvula.setText("Esperando...");
            btnValvula.setBackgroundTintList(getColorStateList(android.R.color.darker_gray));

            Snackbar.make(v, "⏳ Esperando confirmación del ESP32...", Snackbar.LENGTH_SHORT).show();
        });

        btnFijarUbicacion.setOnClickListener(v -> fijarUbicacionActualComoCero());
    }

    private void cargarEstadoValvula() {
        SharedPreferences prefs = getSharedPreferences("VALVE_PREFS", MODE_PRIVATE);
        valvulaActiva = prefs.getBoolean("valvulaActiva", false);
        actualizarEstadoBoton();
    }

    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    private void registrarReceiverMqtt() {
        mqttReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {

                String topic = intent.getStringExtra("topic");
                String payload = intent.getStringExtra("payload");
                if (topic == null || payload == null) return;

                if (topic.trim().equals(MqttService.TOPIC_VALVULA_STATE)) {
                    manejarEstadoValvula(payload.trim());
                    Log.d("MAIN", "Topic recibido: [" + topic + "]");
                    return;
                }

                if (topic.trim().equals(MqttService.TOPIC_CONSUMO)) {
                    Log.d("MAIN", "Nuevo consumo: " + payload);
                }
            }
        };

        IntentFilter filter = new IntentFilter(MqttService.MQTT_MESSAGE_BROADCAST);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(mqttReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(mqttReceiver, filter);
        }
    }

    // -----------------------------------------
    //       LÓGICA DE ESTADO DE VÁLVULA
    // -----------------------------------------

    private void manejarEstadoValvula(String estado) {

        boolean nueva = estado.equals("active");
        valvulaActiva = nueva;

        getSharedPreferences("VALVE_PREFS", MODE_PRIVATE)
                .edit()
                .putBoolean("valvulaActiva", nueva)
                .apply();

        runOnUiThread(() -> {
            actualizarEstadoBoton();
            enviarNotificacionValvula(nueva);
        });
    }

    private void actualizarEstadoBoton() {
        if (valvulaActiva) {
            btnValvula.setText("Desactivar válvula");
            btnValvula.setBackgroundTintList(getColorStateList(android.R.color.holo_red_dark));
        } else {
            btnValvula.setText("Activar válvula");
            btnValvula.setBackgroundTintList(getColorStateList(android.R.color.holo_green_dark));
        }

        btnValvula.setTextColor(getColor(android.R.color.white));
    }

    // -----------------------------------------
    //                 MQTT
    // -----------------------------------------

    private void enviarMensajeMqtt(Context context, String message, String topic) {
        Intent intent = new Intent(context, MqttService.class);
        intent.putExtra("publish", message);
        intent.putExtra("topic", topic);
        context.startService(intent);
    }

    // -----------------------------------------
    //                 GPS
    // -----------------------------------------

    @SuppressLint("MissingPermission")
    private void fijarUbicacionActualComoCero() {

        Log.d(TAG, "Intentando fijar ubicación 'Cero'...");

        Snackbar snackbar = Snackbar.make(findViewById(android.R.id.content),
                "Obteniendo ubicación GPS...", Snackbar.LENGTH_INDEFINITE);
        snackbar.show();

        if (locationClient == null) {
            locationClient = LocationServices.getFusedLocationProviderClient(this);
        }
        locationClient.getCurrentLocation(Priority.PRIORITY_HIGH_ACCURACY,
                        new CancellationToken() {
                            @Override
                            public boolean isCancellationRequested() { return false; }
                            @Override
                            public CancellationToken onCanceledRequested(OnTokenCanceledListener listener)
                            { return this; }
                        })
                .addOnSuccessListener(this, location -> {

                    snackbar.dismiss();

                    if (location != null) {

                        SharedPreferences prefs = getSharedPreferences(GpsService.PREFS_NAME, MODE_PRIVATE);
                        prefs.edit()
                                .putLong(GpsService.PREF_KEY_LAT, Double.doubleToRawLongBits(location.getLatitude()))
                                .putLong(GpsService.PREF_KEY_LON, Double.doubleToRawLongBits(location.getLongitude()))
                                .apply();

                        Snackbar.make(findViewById(android.R.id.content),
                                "Ubicación del ESP32 fijada.", Snackbar.LENGTH_SHORT).show();

                        stopService(new Intent(this, GpsService.class));
                        startService(new Intent(this, GpsService.class));

                    } else {
                        Toast.makeText(this,
                                "Error al fijar ubicación. ¿GPS encendido?",
                                Toast.LENGTH_SHORT).show();
                    }
                })
                .addOnFailureListener(this, e -> {
                    snackbar.dismiss();
                    Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                });
    }

    // -----------------------------------------
    //              PERMISOS
    // -----------------------------------------

    private boolean checkAndRequestPermissions() {

        List<String> permissionsNeeded = new ArrayList<>();

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED)
            permissionsNeeded.add(Manifest.permission.ACCESS_FINE_LOCATION);

        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION)
                != PackageManager.PERMISSION_GRANTED)
            permissionsNeeded.add(Manifest.permission.ACCESS_COARSE_LOCATION);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ActivityCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, 101);
            }
        }

        if (!permissionsNeeded.isEmpty()) {
            ActivityCompat.requestPermissions(this,
                    permissionsNeeded.toArray(new String[0]),
                    PERMISSIONS_REQUEST_CODE);
            return false;
        }

        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {

        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == PERMISSIONS_REQUEST_CODE) {

            boolean allGranted = true;
            for (int res : grantResults)
                if (res != PackageManager.PERMISSION_GRANTED)
                    allGranted = false;

            if (allGranted) {
                Log.d(TAG, "¡Permisos de GPS CONCEDIDOS por el usuario!");
                startServices();

            } else {
                Toast.makeText(this,
                        "Se requieren permisos de Ubicación para esta función",
                        Toast.LENGTH_LONG).show();
            }
        }

        if (requestCode == NOTIFICATION_PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 &&
                    grantResults[0] == PackageManager.PERMISSION_GRANTED)
                Log.d(TAG, "Permiso de notificaciones concedido");
            else
                Log.w(TAG, "Permiso de notificaciones denegado");
        }
    }

    // -----------------------------------------
    //       RECEIVER - ALERTA DE RANGO
    // -----------------------------------------

    private final BroadcastReceiver gpsRangeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Snackbar.make(findViewById(android.R.id.content),
                    "🚨 Saliste del rango. Apagando válvula...",
                    Snackbar.LENGTH_LONG).show();
        }
    };

    // -----------------------------------------
    //        NOTIFICACIONES
    // -----------------------------------------

    private void enviarNotificacionValvula(boolean activa) {

        String titulo = activa ? "Válvula activada" : "Válvula desactivada";
        String texto = activa ? "La válvula se activó correctamente."
                : "La válvula se desactivó correctamente.";

        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);

        PendingIntent pendingIntent =
                PendingIntent.getActivity(this, 0, intent, PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder builder =
                new NotificationCompat.Builder(this, MyApp.CHANNEL_ID_ALERTAS)
                        .setSmallIcon(R.mipmap.ic_launcher)
                        .setContentTitle(titulo)
                        .setContentText(texto)
                        .setPriority(NotificationCompat.PRIORITY_HIGH)
                        .setAutoCancel(true)
                        .setContentIntent(pendingIntent);

        NotificationManager manager =
                (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);

        manager.notify(1002, builder.build());
    }

    // -----------------------------------------
    //      SERVICIOS (MQTT + GPS)
    // -----------------------------------------

    private void startServices() {

        Log.d(TAG, "¡Función startServices() EJECUTADA!");

        startService(new Intent(this, MqttService.class));

        if (!isServiceRunning(this))
            startForegroundService(new Intent(this, GpsService.class));
    }
}

