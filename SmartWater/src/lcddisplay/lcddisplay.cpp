#include "lcddisplay.h"
#include <LiquidCrystal_I2C.h>

int lcdColumns = 16;
int lcdRows = 2;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

LCDDisplay :: LCDDisplay(){}

LCDDisplay :: LCDDisplay(long REFRESH_RATE){
    this->REFRESH_RATE_MS = REFRESH_RATE;
    lcd.init();
    lcd.backlight();
}

void LCDDisplay :: Write(const char* msgFirstRow, const char* msgSecondRow){
    long NOW = millis();
    if (NOW - LAST_WRITE_TIME > REFRESH_RATE_MS) {
        lcd.clear(); 
         // set cursor to first column, first row
        lcd.setCursor(0, 0);
        lcd.print(msgFirstRow);
        // set cursor to first column, second row
        lcd.setCursor(0,1);
        lcd.print(msgSecondRow);
        LAST_WRITE_TIME = NOW;
    }
}