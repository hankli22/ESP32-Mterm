#include "pages.h"
#include "ui.h"
#include "gps_module.h"
#include <Arduino.h>

void drawDevMenu(Canvas& cv) {
  cv.setFont(u8g2_font_6x10_tf);

  float vy = MenuManager::visualDevScrollY;
  float titleY = 12 - vy;
  if (titleY > -10) cv.drawStr(2, (int)titleY, "--- DEV MENU ---");

  const char* items[] = { "1. Basic Info", "2. [sat_view_txt]", "3.[sat_view_gui]", "4. [dev_stat]", "" };
  float curY = 27 + MenuManager::visualDevCursorY - vy;

  for (int i = 0; i < 5; i++) {
    float itemY = 27 + i * 15 - vy;
    if (itemY < 0 || itemY > 70) continue;

    float dist = abs(itemY - curY);
    float offsetX = 0;
    if (dist < 15.0f) offsetX = 6.0f * (1.0f - (dist / 15.0f));

    cv.drawStr(12 + (int)offsetX, (int)itemY, items[i]);
  }
  if (MenuManager::devMenuIdx != 4) cv.drawStr(2, (int)curY, ">");
}

void drawDevPage(Canvas& cv) {
  cv.setFont(u8g2_font_5x8_tf);
  cv.setCursor(0, 10);
  cv.print("Acc: ");
  cv.print(GPSCalc::accuracyPct);
  cv.print("%");
  cv.setCursor(60, 10);
  cv.print("l:");
  cv.print(GPSCalc::satellites);
  cv.print(" c:");
  cv.print(GPSCalc::satsInView);
  cv.setCursor(0, 20);
  cv.print("Lat: ");
  cv.print(GPSCalc::rawLat, 6);
  cv.setCursor(0, 30);
  cv.print("Lng: ");
  cv.print(GPSCalc::rawLng, 6);
  cv.setCursor(0, 40);
  cv.print("Height: ");
  cv.print(GPSCalc::altitude);
  cv.print("m");
  cv.setCursor(0, 50);
  cv.print("Speed: ");
  cv.print(GPSCalc::currentSpeed);
  cv.print("kmh");
  cv.setCursor(0, 60);
  cv.print("Angle: ");
  cv.print(GPSCalc::course);
}

void drawDevStat(Canvas& cv) {
  cv.setFont(u8g2_font_5x8_tf);
  cv.setCursor(2, 12);
  cv.print("--- DEV STAT ---");
  cv.setCursor(2, 28);
  cv.print("Heap: ");
  cv.print(ESP.getFreeHeap() / 1024);
  cv.print(" KB");
  cv.setCursor(2, 40);
  cv.print("Min Heap: ");
  cv.print(ESP.getMinFreeHeap() / 1024);
  cv.print(" KB");
  cv.setCursor(2, 52);
  cv.print("GPS Rdy: ");
  if (GPSCalc::isGpsReady()) cv.print("YES (UART OK)");
  else cv.print("NO (NO DATA)");
}
