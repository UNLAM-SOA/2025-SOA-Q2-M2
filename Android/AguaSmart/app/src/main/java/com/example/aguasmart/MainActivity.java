package com.example.aguasmart;

import android.os.Bundle;
import android.widget.Button;
import android.widget.Toast;

import androidx.activity.EdgeToEdge;
import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;
import androidx.core.graphics.Insets;
import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

public class MainActivity extends AppCompatActivity {

    private Button btnVerConsumo;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        btnVerConsumo = findViewById(R.id.btnVerConsumo);

        btnVerConsumo.setOnClickListener(v -> {
            Toast.makeText(MainActivity.this, "Ir a consumo de agua...", Toast.LENGTH_SHORT).show();
            // Acá se puede abrir otra Activity con los detalles del consumo
            // startActivity(new Intent(MenuActivity.this, ConsumoActivity.class));
        });
    }
}