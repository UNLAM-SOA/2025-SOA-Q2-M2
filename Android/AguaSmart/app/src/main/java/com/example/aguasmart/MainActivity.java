package com.example.aguasmart;

import android.content.Intent;
import android.hardware.SensorManager;
import android.os.Bundle;
import android.widget.Button;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.google.android.material.snackbar.Snackbar;

public class MainActivity extends AppCompatActivity {

    private Button btnVerConsumo;
    private Button btnValvula;
    private boolean valvulaActiva = true;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnVerConsumo = findViewById(R.id.btnVerConsumo);

        btnVerConsumo.setOnClickListener(v -> {
            // Abrir la nueva pantalla ConsumoActivity
            Intent intent = new Intent(MainActivity.this, ConsumoActivity.class);
            startActivity(intent);
        });

        btnValvula = findViewById(R.id.btnValvula);
        actualizarEstadoBoton();

        btnValvula.setOnClickListener(v -> {
            valvulaActiva = !valvulaActiva; // cambia el estado
            actualizarEstadoBoton();

            String mensaje = valvulaActiva ? "Válvula activada" : "Válvula desactivada";
            Snackbar.make(v, mensaje, Snackbar.LENGTH_SHORT).show();
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
}