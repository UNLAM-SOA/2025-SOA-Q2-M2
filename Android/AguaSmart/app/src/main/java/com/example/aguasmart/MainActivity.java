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
import android.content.SharedPreferences; // Importado para GPS
import android.content.pm.PackageManager; // Importado para Permisos
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;
import androidx.annotation.NonNull; // Importado para Permisos
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat; // Importado para Permisos
import androidx.core.app.NotificationCompat;
import androidx.core.content.ContextCompat; // Importado para Permisos
import com.google.android.material.snackbar.Snackbar;

// Imports de GPS (Nuevos)
import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.Priority;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    // Botones
    private Button btnVerConsumo;
    private Button btnValvula;
    private Button btnFijarUbicacion; // Botón nuevo para GPS

    private FusedLocationProviderClient locationClient; // Cliente GPS para el botón
    private boolean valvulaActiva = false; // valor neutral hasta recibir estado real
    private static final String TAG = "MAIN ACTIVITY";

    private BroadcastReceiver mqttReceiver;

    // Permissions
    private static final int NOTIFICATION_PERMISSION_REQUEST_CODE = 456;
    private static final int PERMISSIONS_REQUEST_CODE = 123; // ID para la solicitud de permisos

    /// //////
    @SuppressLint({"UnspecifiedRegisterReceiverFlag", "MissingInflatedId"})
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        Log.d(TAG, "¡onCreate() iniciado!");

        // --- FindViewById ---
        btnVerConsumo = findViewById(R.id.btnVerConsumo);
        btnValvula = findViewById(R.id.btnValvula);
        btnFijarUbicacion = findViewById(R.id.btnFijarUbicacion); // Nuevo


        // Inicializar el cliente de GPS para el botón
        // --- REGISTRAR gpsRangeReceiver ---
        IntentFilter gpsFilter = new IntentFilter("GPS_Rango_Alerta");

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(gpsRangeReceiver, gpsFilter, null, null, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(gpsRangeReceiver, gpsFilter);
        }

        // --- OnClickListeners ---
        btnVerConsumo.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, ConsumoActivity.class);
            startActivity(intent);
        });

        SharedPreferences prefs = getSharedPreferences("VALVE_PREFS", MODE_PRIVATE);
        valvulaActiva = prefs.getBoolean("valvulaActiva", false);

        actualizarEstadoBoton();

        btnValvula.setOnClickListener(v -> {
            String comando = "button_push";
            enviarMensajeMqtt(this, comando, MqttService.TOPIC_VALVULA_CMD);

            // Actualiza visual mientras espera la respuesta del ESP32
            runOnUiThread(() -> {
                btnValvula.setText("Esperando...");
                btnValvula.setBackgroundTintList(getColorStateList(android.R.color.darker_gray));
            });

            Snackbar.make(v, "⏳ Esperando confirmación del ESP32...", Snackbar.LENGTH_SHORT).show();
        });

        // --- NUEVO ONCLICKLISTENER para GPS ---
        btnFijarUbicacion.setOnClickListener(v -> {
            fijarUbicacionActualComoCero();
        });

        // --- CREAR EL BROADCAST RECEIVER ---
        mqttReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {

                String topic = intent.getStringExtra("topic");
                String payload = intent.getStringExtra("payload");

                if (topic == null || payload == null) return;

                if (topic.equals(MqttService.TOPIC_VALVULA_STATE)) {
                    switch (payload.trim()) {
                        case "active":
                            valvulaActiva = true;
                            getSharedPreferences("VALVE_PREFS", MODE_PRIVATE)
                                    .edit()
                                    .putBoolean("valvulaActiva", true)
                                    .apply();

                            runOnUiThread(() -> {
                                actualizarEstadoBoton();
                                enviarNotificacionValvula(true);
                            });
                            break;

                        case "inactive":
                            valvulaActiva = false;
                            getSharedPreferences("VALVE_PREFS", MODE_PRIVATE)
                                    .edit()
                                    .putBoolean("valvulaActiva", false)
                                    .apply();

                            runOnUiThread(() -> {
                                actualizarEstadoBoton();
                                enviarNotificacionValvula(false);
                            });
                            break;
                    }

                return;
                }

                if (topic.equals(MqttService.TOPIC_CONSUMO)) {
                    Log.d("MAIN", "Nuevo consumo: " + payload);
                }
            }
        };


        // REGISTRAR EL RECEIVER del MQTT
        IntentFilter filter = new IntentFilter(MqttService.MQTT_MESSAGE_BROADCAST);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(mqttReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(mqttReceiver, filter);
        }

        // --- LÓGICA DE ARRANQUE CON PERMISOS ---
        // (Reemplaza el simple "startService" que tenías)
        Log.d(TAG, "Chequeando permisos de GPS...");
        if (checkAndRequestPermissions()) {
            // Si ya tenemos permisos, arrancamos los servicios
            Log.d(TAG, "Ya teníamos permisos. Iniciando servicios.");
            startServices();
        } else {
            // Si no, esperamos a que el usuario responda
            Log.d(TAG, "No teníamos permisos. Solicitando...");
        }

        locationClient = LocationServices.getFusedLocationProviderClient(this);
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
        if (mqttReceiver != null) {
            unregisterReceiver(mqttReceiver);
        }

        unregisterReceiver(gpsRangeReceiver);

        // Detenemos AMBOS servicios
        stopService(new Intent(this, MqttService.class));
        stopService(new Intent(this, GpsService.class)); // Detenemos el servicio GPS
    }

    //
    // --- MÉTODOS NUEVOS PARA GPS Y PERMISOS ---
    //

    /**
     * Obtiene la ubicación GPS actual y la guarda como el "Punto Cero"
     * para que el GpsService la use.
     */
    @SuppressLint("MissingPermission") // Los permisos ya se piden en checkAndRequestPermissions()
    private void fijarUbicacionActualComoCero() {
        Log.d(TAG, "Intentando fijar ubicación 'Cero'...");

        // 1. Creamos el Snackbar Y LO GUARDAMOS EN UNA VARIABLE
        final Snackbar snackbar = Snackbar.make(findViewById(android.R.id.content), "Obteniendo ubicación GPS...", Snackbar.LENGTH_INDEFINITE);
        snackbar.show(); // Lo mostramos

        // Pedimos la ubicación actual UNA SOLA VEZ con alta precisión
        locationClient.getCurrentLocation(Priority.PRIORITY_HIGH_ACCURACY, null) // <-- ARREGLO 1 (quitamos "cancellationToken:")
                .addOnSuccessListener(this, location -> {

                    // 2. Usamos la variable para ocultar el snackbar
                    snackbar.dismiss();

                    if (location != null) {
                        double lat = location.getLatitude();
                        double lon = location.getLongitude();

                        // Guardamos la ubicación
                        SharedPreferences prefs = getSharedPreferences(GpsService.PREFS_NAME, MODE_PRIVATE);
                        SharedPreferences.Editor editor = prefs.edit();
                        editor.putLong(GpsService.PREF_KEY_LAT, Double.doubleToRawLongBits(lat));
                        editor.putLong(GpsService.PREF_KEY_LON, Double.doubleToRawLongBits(lon));
                        editor.apply();

                        Log.i(TAG, "¡Nueva ubicación 'Cero' guardada! " + lat + ", " + lon);
                        Snackbar.make(findViewById(android.R.id.content), "Ubicación del ESP32 fijada.", Snackbar.LENGTH_SHORT).show();

                        // Reiniciamos el GpsService para que lea la nueva ubicación
                        stopService(new Intent(this, GpsService.class));
                        startService(new Intent(this, GpsService.class));

                    } else {
                        Log.e(TAG, "No se pudo obtener la ubicación (es null). ¿GPS encendido?");
                        Toast.makeText(this, "Error al fijar ubicación. ¿GPS encendido?", Toast.LENGTH_SHORT).show();
                    }
                })
                .addOnFailureListener(this, e -> {

                    // 3. También lo ocultamos si falla
                    snackbar.dismiss();

                    Log.e(TAG, "Error al obtener ubicación", e);
                    Toast.makeText(this, "Error: " + e.getMessage(), Toast.LENGTH_SHORT).show();
                });
    }

    /**
     * Inicia los servicios de MQTT y GPS.
     */
    private void startServices() {
        Log.d(TAG, "¡Función startServices() EJECUTADA!");

        // 1. Iniciar servicio MQTT
        Intent mqttServiceIntent = new Intent(this, MqttService.class);
        startService(mqttServiceIntent);

        // 2. Iniciar servicio de GPS (si no está ya corriendo)
        if (!isServiceRunning(this)) {
            startForegroundService(new Intent(this, GpsService.class));
        }

    }

    /**
     * Verifica si tiene los permisos de GPS. Si no, los pide.
     * @return true si ya tiene permisos, false si los tuvo que solicitar.
     */
    private boolean checkAndRequestPermissions() {
        List<String> permissionsNeeded = new ArrayList<>();

        // Permisos de Ubicación (GPS)
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION)
                != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        }

        // Permiso FOREGROUND_SERVICE_LOCATION en Android 12+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) { // S = 31
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.FOREGROUND_SERVICE_LOCATION)
                    != PackageManager.PERMISSION_GRANTED) {
                permissionsNeeded.add(Manifest.permission.FOREGROUND_SERVICE_LOCATION);
            }
        }

        // Permiso de Notificaciones en Android 13+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS)
                    != PackageManager.PERMISSION_GRANTED) {
                ActivityCompat.requestPermissions(this,
                        new String[]{Manifest.permission.POST_NOTIFICATIONS},
                        NOTIFICATION_PERMISSION_REQUEST_CODE);
            }
        }

        // Pedir permisos que falten
        if (!permissionsNeeded.isEmpty()) {
            ActivityCompat.requestPermissions(this,
                    permissionsNeeded.toArray(new String[0]),
                    PERMISSIONS_REQUEST_CODE);
            return false;
        }

        return true; // Ya teníamos todos los permisos
    }


    /**
     * Callback que se ejecuta después de que el usuario acepta/rechaza los permisos.
     */

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);

        if (requestCode == PERMISSIONS_REQUEST_CODE) {
            boolean allGranted = true;
            for (int grantResult : grantResults) {
                if (grantResult != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }

            if (allGranted) {
                Log.d(TAG, "¡Permisos de GPS CONCEDIDOS por el usuario!");
                startServices();
            } else {
                Log.w(TAG, "¡Permisos de GPS DENEGADOS por el usuario!");
                Toast.makeText(this, "Se requieren permisos de Ubicación para esta función", Toast.LENGTH_LONG).show();
            }
        }

        if (requestCode == NOTIFICATION_PERMISSION_REQUEST_CODE) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                Log.d(TAG, "Permiso de notificaciones concedido");
            } else {
                Log.w(TAG, "Permiso de notificaciones denegado");
            }
        }
    }

    private final BroadcastReceiver gpsRangeReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {

            Snackbar.make(
                    findViewById(android.R.id.content),
                    "🚨 Saliste del rango. Apagando válvula...",
                    Snackbar.LENGTH_LONG
            ).show();
        }
    };

    private void enviarNotificacionValvula(boolean activa) {
        // Verifica permiso en Android 13+
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
                ActivityCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            Log.w(TAG, "No se puede enviar notificación: permiso denegado");
            return;
        }

        String titulo = activa ? "Válvula activada" : "Válvula desactivada";
        String texto = activa ? "La válvula se activó correctamente." : "La válvula se desactivó correctamente.";

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
        manager.notify(1002, builder.build()); // ID único para la notificación
    }

}