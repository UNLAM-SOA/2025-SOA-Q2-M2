package com.example.aguasmart;

import android.Manifest;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.location.Location;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;

import com.google.android.gms.location.FusedLocationProviderClient;
import com.google.android.gms.location.LocationCallback;
import com.google.android.gms.location.LocationRequest;
import com.google.android.gms.location.LocationResult;
import com.google.android.gms.location.LocationServices;
import com.google.android.gms.location.Priority;

public class GpsService extends Service {

    private static final String TAG = "GpsService";
    private static final float RADIO_LIMITE_METROS = 8.0f; // El radio de 10m
    private static final long INTERVALO_ACTUALIZACION_MS = 2000; // 10 segundos

    private FusedLocationProviderClient fusedLocationClient;
    private LocationCallback locationCallback;

    private Location ubicacionEsp32 = null; // El "Punto Cero"
    private boolean estaDentroDelRadio = false;

    // Claves para guardar/leer la ubicación
    public static final String PREFS_NAME = "AguaSmartPrefs";
    public static final String PREF_KEY_LAT = "esp32_lat";
    public static final String PREF_KEY_LON = "esp32_lon";

    @Override
    public void onCreate() {
        super.onCreate();
        fusedLocationClient = LocationServices.getFusedLocationProviderClient(this);
        cargarUbicacionEsp32(); // Intentamos cargar el "Punto Cero" guardado

        locationCallback = new LocationCallback() {
            @Override
            public void onLocationResult(@NonNull LocationResult locationResult) {
                if (locationResult.getLastLocation() == null) {
                    Log.w(TAG, "Ubicación recibida es null");
                    return;
                }
                Location ubicacionActual = locationResult.getLastLocation();

                if (ubicacionEsp32 == null) {
                    Log.w(TAG, "No se ha fijado la ubicación 'Cero' del ESP32.");
                    return;
                }

                // --- El Cálculo de Distancia ---
                float distancia = ubicacionActual.distanceTo(ubicacionEsp32);
                Log.d(TAG, "Distancia actual al 'Punto Cero': " + distancia + " metros.");

                // --- Lógica de la Válvula ---
                if (distancia <= RADIO_LIMITE_METROS && !estaDentroDelRadio) {
                    // Acabamos de ENTRAR al radio
                    Log.i(TAG, "Entrando al radio. Enviando ON.");
                    estaDentroDelRadio = true;
                    controlarValvula(true); // Abrir
                } else if (distancia > RADIO_LIMITE_METROS && estaDentroDelRadio) {
                    // Acabamos de SALIR del radio
                    Log.i(TAG, "Saliendo del radio. Enviando OFF.");
                    estaDentroDelRadio = false;
                    controlarValvula(false); // Cerrar
                }
            }
        };
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // --- Iniciar el Servicio en Primer Plano ---
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, PendingIntent.FLAG_IMMUTABLE);

        Notification notification = new NotificationCompat.Builder(this, MyApp.CHANNEL_ID_SERVICE) // Usa el ID de MyApp
                .setContentTitle("AguaSmart Conectado (GPS)")
                .setContentText("Protegiendo la válvula por ubicación GPS.")
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .build();

        startForeground(2, notification); // ID 2 (para no confundir con el de BLE)

        // --- Iniciar la escucha de GPS ---
        iniciarActualizacionesDeUbicacion();

        return START_STICKY;
    }

    private void iniciarActualizacionesDeUbicacion() {
        // Pedimos la máxima precisión posible
        LocationRequest locationRequest = new LocationRequest.Builder(
                Priority.PRIORITY_HIGH_ACCURACY,
                INTERVALO_ACTUALIZACION_MS
        ).build();

        // Chequeo de permisos (crucial)
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "No se puede iniciar GPS. Faltan permisos.");
            return;
        }

        fusedLocationClient.requestLocationUpdates(locationRequest,
                locationCallback,
                Looper.getMainLooper());
        Log.i(TAG, "Servicio de ubicación GPS iniciado.");
    }

    // Carga la ubicación "Cero" guardada en el teléfono
    private void cargarUbicacionEsp32() {
        SharedPreferences prefs = getSharedPreferences(PREFS_NAME, MODE_PRIVATE);
        double lat = Double.longBitsToDouble(prefs.getLong(PREF_KEY_LAT, 0));
        double lon = Double.longBitsToDouble(prefs.getLong(PREF_KEY_LON, 0));

        if (lat != 0 && lon != 0) {
            ubicacionEsp32 = new Location("ESP32_Punto_Cero");
            ubicacionEsp32.setLatitude(lat);
            ubicacionEsp32.setLongitude(lon);
            Log.i(TAG, "Ubicación 'Cero' cargada: " + lat + ", " + lon);
        } else {
            Log.w(TAG, "No hay ubicación 'Cero' guardada.");
        }
    }

    // --- Métodos de MQTT (copiados) ---
    private void controlarValvula(boolean activar) {
        String comando = activar ? "ON" : "OFF";
        Log.i(TAG, "Enviando comando de válvula: " + comando);
        enviarMensajeMqtt(this, comando, MqttService.TOPIC_VALVULA_CMD);
    }

    private void enviarMensajeMqtt(Context context, String message, String topic) {
        Intent intent = new Intent(context, MqttService.class);
        intent.putExtra("publish", message);
        intent.putExtra("topic", topic);
        context.startService(intent);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        // Detener la escucha de GPS (muy importante para ahorrar batería)
        fusedLocationClient.removeLocationUpdates(locationCallback);
        Log.i(TAG, "Servicio GPS destruido.");
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null; // No usamos "binding"
    }
}