package com.example.aguasmart;

import android.Manifest;
import android.app.ActivityManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.location.Location;
import android.os.Build;
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
    private static final float RADIO_LIMITE_METROS = 20.0f; // El radio de 10m
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
        cargarUbicacionEsp32(); // cargar el "Punto Cero" guardado

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


                // Si aún no sabemos si está dentro o fuera, inicializar
                if (!estaDentroDelRadio && distancia <= RADIO_LIMITE_METROS) {
                    estaDentroDelRadio = true;
                }

                // --- Lógica de la Válvula ---
                if (distancia > RADIO_LIMITE_METROS && estaDentroDelRadio) {
                    // Acabamos de SALIR del radio
                    Log.i(TAG, "Saliendo del radio. Enviando comando para desactivar válvula.");
                    estaDentroDelRadio = false;

                    controlarValvula(false); // Cerrar

                    Intent intent = new Intent("GPS_Rango_Alerta");
                    sendBroadcast(intent);

                    enviarNotificacionDesactivacion();
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
                .setContentTitle("AguaSmart Conectado")
                .setContentText("Protegiendo la válvula por ubicación GPS.")
                .setPriority(NotificationCompat.PRIORITY_LOW)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .build();

        startForeground(2, notification); // ID 2 (para no confundir con el de BLE)

        // --- Iniciar la escucha de GPS ---
        iniciarActualizacionesDeUbicacion();

        return START_STICKY;
    }

    public static boolean isServiceRunning(Context context) {
        ActivityManager manager = (ActivityManager) context.getSystemService(Context.ACTIVITY_SERVICE);
        for (ActivityManager.RunningServiceInfo service : manager.getRunningServices(Integer.MAX_VALUE)) {
            if (GpsService.class.getName().equals(service.service.getClassName())) {
                return true;
            }
        }
        return false;
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
        String comando = "deactivate_valve";
        Log.i(TAG, "Enviando comando de válvula: " + comando);
        enviarMensajeMqtt(this, comando, MqttService.TOPIC_VALVULA_CMD);
    }

    private void enviarMensajeMqtt(Context context, String message, String topic) {
        Intent intent = new Intent(context, MqttService.class);
        intent.putExtra("publish", message);
        intent.putExtra("topic", topic);
        context.startService(intent);
    }
    private void enviarNotificacionDesactivacion() {
        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(
                this, 0, intent, PendingIntent.FLAG_IMMUTABLE
        );

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, MyApp.CHANNEL_ID_ALERTAS)
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentTitle("Válvula apagada automáticamente")
                .setContentText("Saliste del rango permitido. La válvula fue desactivada.")
                .setPriority(NotificationCompat.PRIORITY_HIGH)
                .setAutoCancel(true)
                .setContentIntent(pendingIntent);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU &&
                ActivityCompat.checkSelfPermission(this, Manifest.permission.POST_NOTIFICATIONS) != PackageManager.PERMISSION_GRANTED) {
            Log.w(TAG, "No se puede enviar notificación: permiso denegado");
            return;
        }

        NotificationManager manager = (NotificationManager) getSystemService(Context.NOTIFICATION_SERVICE);
        if (manager != null) {
            manager.notify(1001, builder.build());
        } else {
            Log.w(TAG, "No se pudo obtener NotificationManager");
        }
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