#ifndef BUTTON_H
#define BUTTON_H

class Button {
    private:
        int pinNumber;

    public:
        Button();
        Button(int pinNumber);
        bool IsPushed();
};

#endif