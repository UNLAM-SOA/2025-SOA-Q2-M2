#ifndef LCDDISPLAY_H
#define LCDDISPLAY_H

class LCDDisplay {
    private:
        long REFRESH_RATE_MS = 1000; // Default refresh is 1 second
        long LAST_WRITE_TIME;

    public:
        LCDDisplay();
        LCDDisplay(long REFRESH_RATE);
        void Write(const char* msg, const char* msg2);
};

#endif