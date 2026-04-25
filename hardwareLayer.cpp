#include "hardwareLayer.h"
#include <SPI.h>


U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/U8X8_PIN_NONE, /* dc=*/21, /* reset=*/U8X8_PIN_NONE);


BtnEvent HAL::lastEvent = BTN_NONE;
bool lastBtnState[4] = { true, true, true, true };
static uint32_t btnNextRepeat[2] = { 0, 0 };

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
  u8g2.setBusClock(40000000);

  u8g2.enableUTF8Print();
  analogReadResolution(12);
}


int HAL::getBatteryPercent() {
  int analogVolts = analogReadMilliVolts(BAT_ADC);
  int batMv = analogVolts * 2;
  int pct = (batMv - 3300) / 9;
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
  uint32_t now = millis();

  for (int i = 0; i < 4; i++) {
    if (lastBtnState[i] == true && current[i] == false) {
      if (i == 0) {
        lastEvent = BTN_UP_PRESSED;
        btnNextRepeat[0] = now + 400;
      }
      if (i == 1) {
        lastEvent = BTN_DOWN_PRESSED;
        btnNextRepeat[1] = now + 400;
      }
      if (i == 2) { lastEvent = BTN_LEFT_PRESSED; }
      if (i == 3) { lastEvent = BTN_RIGHT_PRESSED; }
    } else if (lastBtnState[i] == false && current[i] == false) {
      if (i == 0 && now >= btnNextRepeat[0]) {
        lastEvent = BTN_UP_PRESSED;
        btnNextRepeat[0] = now + 80;
      }
      if (i == 1 && now >= btnNextRepeat[1]) {
        lastEvent = BTN_DOWN_PRESSED;
        btnNextRepeat[1] = now + 80;
      }
    }
    lastBtnState[i] = current[i];
  }
}

BtnEvent HAL::getEvent() {
  BtnEvent evt = lastEvent;
  lastEvent = BTN_NONE;
  return evt;
}

U8G2* HAL::getDisplay() {
  return &u8g2;
}

void HAL::sleepDevice() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(15, 30, "Release Button");
  u8g2.drawStr(15, 45, "to Power Off...");
  u8g2.sendBuffer();

  while (digitalRead(BTN_UP) == LOW || digitalRead(BTN_DOWN) == LOW || digitalRead(BTN_LEFT) == LOW || digitalRead(BTN_RIGHT) == LOW) {
    delay(50);
  }
  delay(300);

  u8g2.clearBuffer();
  u8g2.sendBuffer();

  digitalWrite(OLED_PWR, LOW);
  digitalWrite(OLED_RST, LOW);
  digitalWrite(GPS_PWR_MOSFET, HIGH);

  esp_sleep_enable_ext1_wakeup((1ULL << 4) | (1ULL << 5) | (1ULL << 6) | (1ULL << 7), ESP_EXT1_WAKEUP_ANY_LOW);
  esp_deep_sleep_start();
}
