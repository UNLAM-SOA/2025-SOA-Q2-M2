#ifndef BUTTON_H
#define BUTTON_H

class Button {
    private:
        int pinNumber;
        bool estado_interruptor = false;
        bool ultimo_estado_pulsador = false;
    
    public:
        Button();
        Button(int pinNumber);
        bool IsPushed();
};

#endif