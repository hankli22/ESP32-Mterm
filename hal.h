#ifndef HAL_H
#define HAL_H

#include <U8g2lib.h>

#define BTN_UP 4
#define BTN_DOWN 5
#define BTN_LEFT 6
#define BTN_RIGHT 7

#define OLED_PWR 19
#define GPS_PWR_MOSFET 20 
#define OLED_RST 15       

enum BtnEvent { BTN_NONE, BTN_UP_PRESSED, BTN_DOWN_PRESSED, BTN_LEFT_PRESSED, BTN_RIGHT_PRESSED };

class HAL {
public:
    static void init();
    static void updateButtons();
    static BtnEvent getEvent();
    static U8G2* getDisplay();
    static void sleepDevice();
    static int getBatteryPercent(); // 获取真实电量百分比
private:
    static BtnEvent lastEvent;
};

#endif