#ifndef RELAY_H
#define RELAY_H

class Relay {
    private:
        int pinNumber;
        bool isActive;

    public:
        Relay();
        Relay(int pinNumber);
        void activate();
        void deactivate();
        bool IsActive();
};

#endif