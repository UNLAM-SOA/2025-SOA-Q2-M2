#include "lcddisplay.h"
#include <LiquidCrystal_I2C.h>

int lcdColumns = 16;
int lcdRows = 2;

LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows); 

LCDDisplay :: LCDDisplay(){
    // initialize LCD
    lcd.init();
    // turn on LCD backlight                      
    lcd.backlight();
}

void LCDDisplay :: Write(const char* msg, const char* msg2){
    lcd.clear(); 
     // set cursor to first column, first row
    lcd.setCursor(0, 0);
    lcd.print(msg);
    // set cursor to first column, second row
    lcd.setCursor(0,1);
    lcd.print(msg2);
}