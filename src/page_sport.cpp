#include "pages.h"
#include "gps_module.h"
#include "config.h"

void drawSport1(Canvas& cv) {
  if (GPSCalc::satellites < 4) {
    cv.setFont(u8g2_font_6x12_tr);
    cv.drawStr(20, 35, "WAITING GPS");
    int dotCount = (millis() / 500) % 4;
    const char* d[] = { "", ".", "..", "..." };
    cv.drawStr(92, 35, d[dotCount]);
    cv.setCursor(45, 50);
    cv.print("SATS: ");
    cv.print(GPSCalc::satellites);
    return;
  }

  cv.setFont(u8g2_font_6x12_tf);
  cv.setCursor(0, 12);
  cv.print("Distance: ");
  cv.print((int)GPSCalc::totalDistance);
  cv.print(" m");

  cv.setCursor(0, 24);
  cv.print("Speed: ");
  cv.print(GPSCalc::currentSpeed, 1);
  cv.print(" km/h");

  cv.setCursor(0, 36);
  cv.print("Pace(10s): ");
  if (GPSCalc::paceMin > 0) {
    cv.print(GPSCalc::paceMin);
    cv.print(":");
    if (GPSCalc::paceSec < 10) cv.print("0");
    cv.print(GPSCalc::paceSec);
    cv.print(" /km");
  } else cv.print("--:-- /km");

  cv.setCursor(0, 48);
  cv.print("time: ");
  cv.print(GPSCalc::durationSec / 60);
  cv.print(":");
  if (GPSCalc::durationSec % 60 < 10) cv.print("0");
  cv.print(GPSCalc::durationSec % 60);

  cv.setCursor(0, 60);
  cv.print("turns: ");
  cv.print(GPSCalc::laps);
}

void drawSport2(Canvas& cv) {
  cv.setFont(u8g2_font_6x12_tf);

  float avgPace = 0;
  if (GPSCalc::totalDistance > 10 && GPSCalc::durationSec > 0) avgPace = 60.0 / ((GPSCalc::totalDistance / GPSCalc::durationSec) * 3.6);

  cv.setCursor(0, 12);
  cv.print("Avg Pace: ");
  cv.print((int)avgPace);
  cv.print("'");
  int avgSec = (int)((avgPace - (int)avgPace) * 60);
  if (avgSec < 10) cv.print("0");
  cv.print(avgSec);
  cv.print("\"");

  cv.setCursor(0, 24);
  cv.print("Max Spd:  ");
  cv.print(GPSCalc::maxSpeed, 1);
  cv.print(" km/h");
  cv.setCursor(0, 36);
  cv.print("Calories: ");
  cv.print(GPSCalc::calories);
  cv.print(" kcal");
  cv.setCursor(0, 48);
  cv.print("Altitude: ");
  cv.print(GPSCalc::altitude);
  cv.print(" m");
  cv.setCursor(0, 60);
  cv.print("Sats Lck: ");
  cv.print(GPSCalc::satellites);
}

void drawSport3(Canvas& cv) {
  cv.setFont(u8g2_font_6x10_tf);
  cv.drawStr(2, 10, "Live Track");

  cv.setCursor(2, 25);
  cv.print("D: ");
  cv.print((int)GPSCalc::totalDistance);
  cv.setCursor(2, 40);
  cv.print("P: ");
  cv.print(GPSCalc::paceMin);
  cv.print("'");
  cv.print(GPSCalc::paceSec);
  cv.setCursor(2, 55);
  cv.print("C: ");
  cv.print(GPSCalc::calories);
  drawTrackMap(cv, 70, 12, 0);
}
