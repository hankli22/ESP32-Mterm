#include "pages.h"
#include "ui.h"
#include "hardwareLayer.h"
#include "gps_module.h"
#include "config.h"

void drawStartMenu(Canvas& cv) {
  cv.setFont(u8g2_font_6x10_tf);

  cv.drawStr(0, 10, "Start");
  if (GPSCalc::satellites > 4) {
    cv.drawStr(40, 10, "[SAT:OK]");
  } else {
    const char* dots[] = { "[SAT:   ]", "[SAT:.  ]", "[SAT:.. ]", "[SAT:...]", "[SAT: ..]", "[SAT:  .]" };
    int frame = (millis() / 300) % 6;
    cv.drawStr(40, 10, dots[frame]);
  }

  int pct = HAL::getBatteryPercent();
  cv.drawFrame(93, 2, 18, 8);
  cv.drawBox(111, 4, 2, 4);
  int fillW = (pct * 14) / 100;
  if (fillW > 0) cv.drawBox(95, 4, fillW, 4);
  cv.setFont(u8g2_font_5x8_tf);
  cv.setCursor(114, 10);
  cv.print(pct);
  cv.print("%");

  cv.setFont(u8g2_font_6x10_tf);
  cv.setCursor(0, 25);
  cv.print(GPSCalc::getDateTime().c_str());

  if (MenuManager::isCursorVisible) cv.drawStr(0, (int)MenuManager::currentCursorY, ">");
  else {
    cv.drawStr(0, 45, "-");
    cv.drawStr(0, 60, "-");
  }

  float dist1 = abs(45 - MenuManager::currentCursorY);
  float dist2 = abs(60 - MenuManager::currentCursorY);
  float ox1 = (dist1 < 15.0f && MenuManager::isCursorVisible) ? 6.0f * (1.0f - (dist1 / 15.0f)) : 0;
  float ox2 = (dist2 < 15.0f && MenuManager::isCursorVisible) ? 6.0f * (1.0f - (dist2 / 15.0f)) : 0;

  cv.drawStr(10 + (int)ox1, 45, "mode: running");
  cv.drawStr(10 + (int)ox2, 60, "settings");
}
