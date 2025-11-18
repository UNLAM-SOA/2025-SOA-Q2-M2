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
    private static final int REQ_LOCATION = 100;
    private static final int REQ_NOTIFICATIONS = 101;

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
    private Boolean lastValveState = null;

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
        pedirPermisosUbicacion();
        registrarReceiverMqtt();

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

        // Si el estado NO CAMBIÓ → no notificar
        if (lastValveState != null && lastValveState == nueva) {
            Log.d("VALVULA", "Estado repetido, no se notifica");
            valvulaActiva = nueva;
            runOnUiThread(this::actualizarEstadoBoton);
            return;
        }

        // Estado cambió → actualizar estado
        lastValveState = nueva;
        valvulaActiva = nueva;

        // Guardar en SharedPreferences el último estado registrado
        getSharedPreferences("VALVE_PREFS", MODE_PRIVATE)
                .edit()
                .putBoolean("valvulaActiva", nueva)
                .apply();

        // Actualizar UI + notificar
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

    private void pedirPermisosUbicacion() {
        String[] permisosUbicacion = new String[]{
                Manifest.permission.ACCESS_FINE_LOCATION,
                Manifest.permission.ACCESS_COARSE_LOCATION,
                Manifest.permission.FOREGROUND_SERVICE_LOCATION
        };

        boolean ok = true;
        for (String p : permisosUbicacion) {
            if (ContextCompat.checkSelfPermission(this, p) != PackageManager.PERMISSION_GRANTED) {
                ok = false;
            }
        }

        if (!ok) {
            ActivityCompat.requestPermissions(this, permisosUbicacion, REQ_LOCATION);
        } else {
            pedirPermisoNotificaciones();
        }
    }


    private void pedirPermisoNotificaciones() {
        if (Build.VERSION.SDK_INT >= 33) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {

                requestPermissions(new String[]{Manifest.permission.POST_NOTIFICATIONS}, REQ_NOTIFICATIONS);
                return;
            }
        }

        // Si ya está concedido, arrancar servicios
        iniciarServicios();
    }

    private void iniciarServicios() {
        Log.d("MAIN", "✔ Permisos completos → iniciando servicios");

        startService(new Intent(this, MqttService.class));
        startForegroundService(new Intent(this, GpsService.class));

        registrarReceiverMqtt();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           @NonNull String[] permissions,
                                           @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == REQ_LOCATION) {
            boolean allGranted = true;
            if (grantResults.length == 0) allGranted = false;
            for (int res : grantResults) {
                if (res != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }

            if (allGranted) {
                Log.d(TAG, "Permisos de ubicación concedidos → pedir permiso de notificaciones");
                pedirPermisoNotificaciones();
            } else {
                Toast.makeText(this, "La app necesita permisos de ubicación para funcionar", Toast.LENGTH_LONG).show();
            }
            return;
        }

        if (requestCode == REQ_NOTIFICATIONS) {
            boolean granted = (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED);

            if (granted) {
                Log.d(TAG, "Permiso de notificaciones concedido → iniciando servicios");
                iniciarServicios();
            } else {
                Log.w(TAG, "Permiso de notificaciones DENEGADO. No se iniciarán los servicios que muestran notificaciones.");
                Toast.makeText(this, "Sin permiso de notificaciones no se puede iniciar el servicio en primer plano", Toast.LENGTH_LONG).show();
            }
            return;
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

