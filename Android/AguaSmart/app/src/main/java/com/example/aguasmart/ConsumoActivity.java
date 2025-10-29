package com.example.aguasmart;

import androidx.appcompat.app.AppCompatActivity;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import java.util.Random;

public class ConsumoActivity extends AppCompatActivity {

    private TextView tvConsumo;
    private Button btnActualizar;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_consumo);

        // Botón para volver al menú :)
        Button btnVolver = findViewById(R.id.btnVolverMenu);
        btnVolver.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                Intent intent = new Intent(ConsumoActivity.this, MainActivity.class);
                startActivity(intent);
                finish();
            }
        });

        tvConsumo = findViewById(R.id.tvConsumo);
        btnActualizar = findViewById(R.id.btnActualizar);

        // Valor inicial hardcodeado
        tvConsumo.setText(" 15.3 L");

        btnActualizar.setOnClickListener(v -> {
            // Simula actualización de datos desde el embebido
            double nuevoConsumo = generarConsumoAleatorio();
            tvConsumo.setText(String.format(" %.2f L", nuevoConsumo));
            Toast.makeText(this, "Consumo actualizado", Toast.LENGTH_SHORT).show();
        });
    }

    // Genera un valor aleatorio entre 10 y 30 litros
    private double generarConsumoAleatorio() {
        Random random = new Random();
        return 10 + (20 * random.nextDouble());
    }
}
