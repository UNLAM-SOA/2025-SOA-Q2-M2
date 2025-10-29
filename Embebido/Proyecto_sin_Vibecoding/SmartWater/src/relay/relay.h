#ifndef RELAY_H
#define RELAY_H

class Relay {
    private:
        int pinNumber;

    public:
        Relay();
        Relay(int pinNumber);
        void activate();
        void deactivate();
};

#endif