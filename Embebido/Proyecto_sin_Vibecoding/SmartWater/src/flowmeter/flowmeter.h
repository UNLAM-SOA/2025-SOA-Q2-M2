#ifndef FLOWMETER_H
#define FLOWMETER_H

class FlowMeter {
    private:
        int pinNumber;


    public:
        FlowMeter();
        FlowMeter(int pinNumber);
        float GetConsumptionSinceLastMeasurement();
};

#endif