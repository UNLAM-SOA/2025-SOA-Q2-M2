package com.example.aguasmart;

import android.annotation.SuppressLint;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;
import androidx.appcompat.app.AppCompatActivity;
import com.google.android.material.snackbar.Snackbar;

import android.Manifest;
import android.content.pm.PackageManager;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import java.util.ArrayList;
import java.util.List;

public class MainActivity extends AppCompatActivity {

    private Button btnVerConsumo;
    private Button btnValvula;
    private Button btnMqttTest;
    private boolean valvulaActiva = true;
    private static final String TAG = "MAIN ACTIVITY";

    private BroadcastReceiver mqttReceiver;

    private static final int PERMISSIONS_REQUEST_CODE = 123;

    @SuppressLint("UnspecifiedRegisterReceiverFlag")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnVerConsumo = findViewById(R.id.btnVerConsumo);
        btnValvula = findViewById(R.id.btnValvula);
        btnMqttTest = findViewById(R.id.btnMqttTest);

        btnVerConsumo.setOnClickListener(v -> {
            Intent intent = new Intent(MainActivity.this, ConsumoActivity.class);
            startActivity(intent);
        });

        btnValvula.setOnClickListener(v -> {
            String comando = valvulaActiva ? "OFF" : "ON";
            enviarMensajeMqtt(this, comando, MqttService.TOPIC_VALVULA_CMD);
            Snackbar.make(v, "⏳ Esperando confirmación del ESP32.", Snackbar.LENGTH_SHORT).show();
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
                    return;
                }
            }
        };

        IntentFilter filter = new IntentFilter(MqttService.MQTT_MESSAGE_BROADCAST);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
            registerReceiver(mqttReceiver, filter, Context.RECEIVER_NOT_EXPORTED);
        } else {
            registerReceiver(mqttReceiver, filter);
        }

        // --- CHEQUEAR Y PEDIR PERMISOS ---
        if (checkAndRequestPermissions()) {
            startServices();
        }

        btnMqttTest.setOnClickListener(v -> {
            Log.d("BTN", "Presionaste el botón MQTT");
            enviarMensajeMqtt(this, "Hola desde Android!", MqttService.TOPIC_TEST);
        });
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
        stopService(new Intent(this, MqttService.class));
        stopService(new Intent(this, ProximityService.class));
    }

    private void startServices() {
        Intent mqttServiceIntent = new Intent(this, MqttService.class);
        startService(mqttServiceIntent);

        Intent proximityServiceIntent = new Intent(this, ProximityService.class);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            startForegroundService(proximityServiceIntent);
        } else {
            startService(proximityServiceIntent);
        }
    }

    private boolean checkAndRequestPermissions() {
        List<String> permissionsNeeded = new ArrayList<>();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.S) {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_SCAN) != PackageManager.PERMISSION_GRANTED) {
                permissionsNeeded.add(Manifest.permission.BLUETOOTH_SCAN);
            }
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_CONNECT) != PackageManager.PERMISSION_GRANTED) {
                permissionsNeeded.add(Manifest.permission.BLUETOOTH_CONNECT);
            }
        } else {
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH) != PackageManager.PERMISSION_GRANTED) {
                permissionsNeeded.add(Manifest.permission.BLUETOOTH);
            }
            if (ContextCompat.checkSelfPermission(this, Manifest.permission.BLUETOOTH_ADMIN) != PackageManager.PERMISSION_GRANTED) {
                permissionsNeeded.add(Manifest.permission.BLUETOOTH_ADMIN);
            }
        }

        // Ubicación: pedimos BOTH Fine y Coarse (coincide con el Manifest)
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.ACCESS_FINE_LOCATION);
        }
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) != PackageManager.PERMISSION_GRANTED) {
            permissionsNeeded.add(Manifest.permission.ACCESS_COARSE_LOCATION);
        }

        if (!permissionsNeeded.isEmpty()) {
            ActivityCompat.requestPermissions(this, permissionsNeeded.toArray(new String[0]), PERMISSIONS_REQUEST_CODE);
            return false;
        }

        return true;
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
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
                startServices();
            } else {
                Toast.makeText(this, "Se requieren permisos de Bluetooth y Ubicación para esta función", Toast.LENGTH_LONG).show();
            }
        }
    }
}
