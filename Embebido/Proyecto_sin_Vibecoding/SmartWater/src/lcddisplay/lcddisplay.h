#ifndef LCDDISPLAY_H
#define LCDDISPLAY_H

class LCDDisplay {
    public:
        LCDDisplay();
        void Write(const char* msg, const char* msg2);
};

#endif