#include "hal.h"
#include <SPI.h>

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/ U8X8_PIN_NONE, /* dc=*/ 21, /* reset=*/ U8X8_PIN_NONE);

BtnEvent HAL::lastEvent = BTN_NONE;
bool lastBtnState[4] = {true, true, true, true};

void HAL::init() {
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);
    pinMode(BTN_LEFT, INPUT_PULLUP);
    pinMode(BTN_RIGHT, INPUT_PULLUP);
    
    pinMode(GPS_PWR_MOSFET, OUTPUT); 
    digitalWrite(GPS_PWR_MOSFET, LOW); 
    
    pinMode(OLED_PWR, OUTPUT);
    pinMode(OLED_RST, OUTPUT);
    
    digitalWrite(OLED_PWR, LOW);
    digitalWrite(OLED_RST, LOW);
    delay(50);
    digitalWrite(OLED_PWR, HIGH); 
    delay(50);
    digitalWrite(OLED_RST, HIGH); 
    delay(50);

    SPI.begin(23, -1, 22, -1); 
    u8g2.begin();
    u8g2.setBusClock(20000000); 

    // 【新增】：开启 UTF8 中文支持，并设置 ADC 精度
    u8g2.enableUTF8Print(); 
    analogReadResolution(12);
}

// 【新增】：电池电量测量与计算
int HAL::getBatteryPercent() {
    int analogVolts = analogReadMilliVolts(0);
    int batMv = analogVolts * 2; // 根据你的电路分压系数 * 2
    
    // 3.7V 锂电池满电约 4200mV，关机电压约 3300mV
    int pct = (batMv - 3300) / 9; // (batMv - 3300) / (4200 - 3300) * 100
    if (pct > 100) pct = 100;
    if (pct < 0) pct = 0;
    return pct;
}

void HAL::updateButtons() {
    bool current[4] = {
        (digitalRead(BTN_UP) == HIGH), 
        (digitalRead(BTN_DOWN) == HIGH), 
        (digitalRead(BTN_LEFT) == HIGH), 
        (digitalRead(BTN_RIGHT) == HIGH)
    };
    lastEvent = BTN_NONE;
    if (lastBtnState[0] && !current[0]) lastEvent = BTN_UP_PRESSED;
    if (lastBtnState[1] && !current[1]) lastEvent = BTN_DOWN_PRESSED;
    if (lastBtnState[2] && !current[2]) lastEvent = BTN_LEFT_PRESSED;
    if (lastBtnState[3] && !current[3]) lastEvent = BTN_RIGHT_PRESSED;
    for(int i=0; i<4; i++) lastBtnState[i] = current[i];
}

BtnEvent HAL::getEvent() {
    BtnEvent evt = lastEvent;
    lastEvent = BTN_NONE;
    return evt;
}

U8G2* HAL::getDisplay() { return &u8g2; }

void HAL::sleepDevice() { 
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x12_tr);
    u8g2.drawStr(15, 30, "Release Button");
    u8g2.drawStr(15, 45, "to Power Off...");
    u8g2.sendBuffer();

    while(digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW || 
          digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
        delay(50);
    }
    delay(300); 

    u8g2.clearBuffer();
    u8g2.sendBuffer();

    digitalWrite(OLED_PWR, LOW);
    digitalWrite(OLED_RST, LOW);
    digitalWrite(GPS_PWR_MOSFET, HIGH); 

    esp_deep_sleep_enable_gpio_wakeup((1ULL<<4)|(1ULL<<5)|(1ULL<<6)|(1ULL<<7), ESP_GPIO_WAKEUP_GPIO_LOW);
    esp_deep_sleep_start();
}