package com.example.aguasmart;

import android.app.Application;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.os.Build;
import androidx.appcompat.app.AppCompatDelegate;

public class MyApp extends Application {

    // Canal del servicio en primer plano
    public static final String CHANNEL_ID_SERVICE = "ServiceChannel";

    // Canal para alertas de umbral de afua
    public static final String CHANNEL_ID_ALERTAS = "AlertasChannel";

    @Override
    public void onCreate() {
        super.onCreate();
        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_FOLLOW_SYSTEM);
        createNotificationChannel();
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {

            // Canal servicio primer plano
            NotificationChannel serviceChannel = new NotificationChannel(
                    CHANNEL_ID_SERVICE,
                    "Servicio de Proximidad",
                    NotificationManager.IMPORTANCE_DEFAULT
            );

            // Canal para alertas de válvula / GPS
            NotificationChannel alertChannel = new NotificationChannel(
                    CHANNEL_ID_ALERTAS,
                    "Alertas de AguaSmart",
                    NotificationManager.IMPORTANCE_HIGH
            );


            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager != null) {
                manager.createNotificationChannel(serviceChannel);
                manager.createNotificationChannel(alertChannel);
            }
        }
    }
}