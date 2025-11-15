package com.example.aguasmart;

import android.Manifest;
import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.BluetoothLeScanner;
import android.bluetooth.le.ScanCallback;
import android.bluetooth.le.ScanFilter;
import android.bluetooth.le.ScanResult;
import android.bluetooth.le.ScanSettings;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.util.Log;

import androidx.annotation.Nullable;
import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;

import java.util.Collections;
import java.util.List;

public class ProximityService extends Service {

    private static final String TAG = "ProximityService";
    private static final String ESP32_BEACON_NAME = "AguaSmart-ESP32";
    private static final long BEACON_TIMEOUT_MS = 15000; // 15 segundos

    private BluetoothLeScanner bleScanner;
    private final Handler handler = new Handler(Looper.getMainLooper());
    private boolean isDeviceNearby = false;

    // Runnable que se ejecuta cuando se pierde el beacon
    private final Runnable beaconLostRunnable = () -> {
        Log.w(TAG, "¡Faro perdido! Se superó el tiempo de espera.");
        if (isDeviceNearby) {
            isDeviceNearby = false;
            controlarValvula(false); // Cerrar válvula
        }
    };

    // Callback que recibe señales BLE
    private final ScanCallback leScanCallback = new ScanCallback() {
        @Override
        public void onScanResult(int callbackType, ScanResult result) {
            super.onScanResult(callbackType, result);

            // En API >= 31 puede requerirse BLUETOOTH_CONNECT para acceder a device info
            if (ActivityCompat.checkSelfPermission(getApplicationContext(), Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                Log.w(TAG, "Falta permiso BLUETOOTH_CONNECT para leer dispositivo.");
                return;
            }

            String deviceName = result.getDevice() != null ? result.getDevice().getName() : null;

            if (deviceName != null && deviceName.equals(ESP32_BEACON_NAME)) {
                Log.d(TAG, "Faro " + ESP32_BEACON_NAME + " encontrado.");

                // Reiniciamos el temporizador de pérdida
                handler.removeCallbacks(beaconLostRunnable);
                handler.postDelayed(beaconLostRunnable, BEACON_TIMEOUT_MS);

                if (!isDeviceNearby) {
                    isDeviceNearby = true;
                    controlarValvula(true); // Abrir válvula
                }
            }
        }

        @Override
        public void onScanFailed(int errorCode) {
            super.onScanFailed(errorCode);
            Log.e(TAG, "Fallo en el escaneo BLE, código: " + errorCode);
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        BluetoothManager bluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);

        if (bluetoothManager == null) {
            Log.e(TAG, "BluetoothManager es null. No se puede obtener BluetoothAdapter.");
            stopSelf();
            return;
        }

        BluetoothAdapter bluetoothAdapter = bluetoothManager.getAdapter();

        if (bluetoothAdapter == null) {
            Log.e(TAG, "BluetoothAdapter es null. Dispositivo no soporta Bluetooth.");
            stopSelf();
            return;
        }

        if (!bluetoothAdapter.isEnabled()) {
            Log.e(TAG, "Bluetooth está apagado. Pide al usuario que lo encienda.");
            // No matamos inmediatamente para que la UI pueda pedir al usuario que lo active.
            // Pero sin adapter habilitado no hay scanner.
            bleScanner = null;
            return;
        }

        bleScanner = bluetoothAdapter.getBluetoothLeScanner();
        if (bleScanner == null) {
            Log.e(TAG, "BluetoothLeScanner es null. No se puede escanear.");
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // Intent para abrir la app desde la notificación
        Intent notificationIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, notificationIntent, PendingIntent.FLAG_IMMUTABLE);

        Notification notification = new NotificationCompat.Builder(this, MyApp.CHANNEL_ID_PROXIMITY)
                .setContentTitle("AguaSmart Conectado")
                .setContentText("Protegiendo la válvula por proximidad.")
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .build();

        startForeground(1, notification);

        // Iniciar el escaneo BLE (verifica permisos)
        startBleScan();

        return START_STICKY;
    }

    private void startBleScan() {
        // Validaciones
        if (ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
            Log.e(TAG, "No se puede escanear: falta permiso BLUETOOTH_SCAN.");
            return;
        }

        if (bleScanner == null) {
            Log.e(TAG, "bleScanner es null — no hay scanner disponible.");
            return;
        }

        // Filtro para nuestro dispositivo
        ScanFilter scanFilter = new ScanFilter.Builder()
                .setDeviceName(ESP32_BEACON_NAME)
                .build();
        List<ScanFilter> filters = Collections.singletonList(scanFilter);

        ScanSettings settings = new ScanSettings.Builder()
                .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
                .build();

        try {
            bleScanner.startScan(filters, settings, leScanCallback);
            Log.i(TAG, "Escaneo BLE iniciado.");
        } catch (SecurityException se) {
            Log.e(TAG, "SecurityException al iniciar escaneo BLE: " + se.getMessage());
        } catch (Exception e) {
            Log.e(TAG, "Excepción al iniciar escaneo BLE: " + e.getMessage());
        }
    }

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
        if (bleScanner != null && ActivityCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) == PackageManager.PERMISSION_GRANTED) {
            try {
                bleScanner.stopScan(leScanCallback);
            } catch (Exception e) {
                Log.w(TAG, "Error al detener escaneo: " + e.getMessage());
            }
        }
        handler.removeCallbacks(beaconLostRunnable);
        Log.i(TAG, "Servicio de Proximidad destruido.");
    }

    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
}
