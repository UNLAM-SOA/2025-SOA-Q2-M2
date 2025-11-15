package com.example.aguasmart;
import android.Manifest;
import android.annotation.SuppressLint;
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
    private Button btnMqttTest;
    private Button btnFijarUbicacion; // Botón nuevo para GPS

    private FusedLocationProviderClient locationClient; // Cliente GPS para el botón

    private boolean valvulaActiva = true;
    private static final String TAG = "MAIN ACTIVITY";

    private BroadcastReceiver mqttReceiver;

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
        btnMqttTest = findViewById(R.id.btnMqttTest);
        btnFijarUbicacion = findViewById(R.id.btnFijarUbicacion); // Nuevo

        // Inicializar el cliente de GPS para el botón
        locationClient = LocationServices.getFusedLocationProviderClient(this);

        // --- OnClickListeners ---
        btnVerConsumo.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, ConsumoActivity.class);
            startActivity(intent);
        });

        btnValvula.setOnClickListener(v -> {
            String comando = valvulaActiva ? "OFF" : "ON"; // querés cambiar el estado
            enviarMensajeMqtt(this, comando, MqttService.TOPIC_VALVULA_CMD);
            Snackbar.make(v, "⏳ Esperando confirmación del ESP32...", Snackbar.LENGTH_SHORT).show();
        });

        // --- NUEVO ONCLICKLISTENER para GPS ---
        btnFijarUbicacion.setOnClickListener(v -> {
            fijarUbicacionActualComoCero();
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

        // REGISTRAR EL RECEIVER
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

        ///  PRUEBA MQTT //////////////////////////////
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
        if (mqttReceiver != null) {
            unregisterReceiver(mqttReceiver);
        }

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

        // 2. Iniciar servicio de GPS
        Intent gpsServiceIntent = new Intent(this, GpsService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(gpsServiceIntent);
        } else {
            startService(gpsServiceIntent);
        }
    }

    /**
     * Verifica si tiene los permisos de GPS. Si no, los pide.
     * @return true si ya tiene permisos, false si los tuvo que solicitar.
     */
    private boolean checkAndRequestPermissions() {
        List<String> permissionsNeeded = new ArrayList<>();

        // Solo pedimos permisos de Ubicación (GPS)
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        }

        // Si la lista no está vacía, pedimos los permisos
        if (!permissionsNeeded.isEmpty()) {
            ActivityCompat.requestPermissions(this, permissionsNeeded.toArray(new String[0]), PERMISSIONS_REQUEST_CODE);
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
                // ¡Permisos concedidos! Arrancamos los servicios.
                Log.d(TAG, "¡Permisos de GPS CONCEDIDOS por el usuario!");
                startServices();
            } else {
                // Permisos denegados. La función no andará.
                Log.w(TAG, "¡Permisos de GPS DENEGADOS por el usuario!");
                Toast.makeText(this, "Se requieren permisos de Ubicación para esta función", Toast.LENGTH_LONG).show();
            }
        }
    }
}