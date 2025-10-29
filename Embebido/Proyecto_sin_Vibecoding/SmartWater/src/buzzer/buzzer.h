#ifndef BUZZER_H
#define BUZZER_H

class Buzzer {
    private:
        int pinNumber;
    
    public:
        Buzzer();
        Buzzer(int pinNumber);
        void PlayAlarm(int level);
        void StopAlarm();
};

const int ALARM_LEVELS[] = {
  0,
  5000,   // Nivel 1: Frecuencia baja
  7000,  // Nivel 2: Frecuencia media
  9000,  // Nivel 3: Frecuencia alta
  12000   // Nivel 4: Frecuencia máxima
};

#endif