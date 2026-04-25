#ifndef HARDWARELAYER_H
#define HARDWARELAYER_H

#include <U8g2lib.h>
#include <driver/spi_master.h>

#define BTN_UP 4
#define BTN_DOWN 5
#define BTN_LEFT 6
#define BTN_RIGHT 7

#define OLED_PWR 19
#define GPS_PWR_MOSFET 20  // 接 A1SHB 的 Gate
#define OLED_RST 15        // 解决屏幕不复位问题

#define BAT_ADC 0

enum BtnEvent { BTN_NONE,
                BTN_UP_PRESSED,
                BTN_DOWN_PRESSED,
                BTN_LEFT_PRESSED,
                BTN_RIGHT_PRESSED };

class HAL {
public:
  static void init();
  static void updateButtons();
  static BtnEvent getEvent();
  static U8G2* getDisplay();
  static void sleepDevice();
  static int getBatteryPercent();
  static void InitDMA();  // 在 setup 初始化时调用
  static void Flush();    // 替代 sendBuffer()
  static spi_device_handle_t getSpiHandle() { return spi_handle; }
private:
  static BtnEvent lastEvent;
  static spi_device_handle_t spi_handle;
};

#endif
