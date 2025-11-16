#ifndef FLOWMETER_H
#define FLOWMETER_H

class FlowMeter {
    private:
        int pinNumber;
        long LAST_MEASUREMENT_TIME = 0;
        long FLOW_RATE_SAMPLE_TIME_MS = 1000;
        float MEASUREMENT_CORRECTION_FACTOR = 3.75;

    public:
        FlowMeter();
        FlowMeter(int pinNumber);
        float GetConsumptionSinceLastMeasurement();
};

#endif