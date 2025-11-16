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

#endif