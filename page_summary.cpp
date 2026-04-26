#include "pages.h"
#include "ui.h"
#include "gps_module.h"
#include "config.h"

void drawTrackMap(Canvas& cv, int ox, int oy, int lapIdx) {
  int start = 0;
  int end = GPSCalc::trackPointsCount - 1;

  if (lapIdx > 0 && lapIdx <= GPSCalc::laps) {
    start = GPSCalc::lapHistory[lapIdx - 1].trackStartIdx;
    end = GPSCalc::lapHistory[lapIdx - 1].trackEndIdx;
  }

  if (end - start < 1 || start < 0) return;

  float minX = 9999, maxX = -9999, minY = 9999, maxY = -9999;
  for (int i = 0; i < GPSCalc::trackPointsCount; i++) {
    if (GPSCalc::trackX[i] < minX) minX = GPSCalc::trackX[i];
    if (GPSCalc::trackX[i] > maxX) maxX = GPSCalc::trackX[i];
    if (GPSCalc::trackY[i] < minY) minY = GPSCalc::trackY[i];
    if (GPSCalc::trackY[i] > maxY) maxY = GPSCalc::trackY[i];
  }
  float maxDim = max(maxX - minX, maxY - minY);
  float scale = 44.0f / (maxDim > 0.1 ? maxDim : 1.0f);

  for (int i = start + 1; i <= end; i++) {
    int px1 = ox + (GPSCalc::trackX[i - 1] - minX) * scale;
    int py1 = oy + 44 - (GPSCalc::trackY[i - 1] - minY) * scale;
    int px2 = ox + (GPSCalc::trackX[i] - minX) * scale;
    int py2 = oy + 44 - (GPSCalc::trackY[i] - minY) * scale;
    cv.drawLine(px1, py1, px2, py2);
  }
  cv.drawFrame(ox - 2, oy - 2, 48, 48);
}

void drawSummary(Canvas& cv) {
  cv.setFont(u8g2_font_6x10_tf);
  cv.drawStr(2, 10, "Summary");

  float avgPace = 0;
  if (GPSCalc::totalDistance > 10 && GPSCalc::durationSec > 0) {
    float speedKmh = (GPSCalc::totalDistance / GPSCalc::durationSec) * 3.6f;
    if (speedKmh > 1.0f) avgPace = 60.0f / speedKmh;
  }

  cv.setCursor(60, 10);
  if (avgPace > 0) {
    cv.print("Avg:");
    cv.print((int)avgPace);
    cv.print("'");
    int avgSec = (int)((avgPace - (int)avgPace) * 60);
    if (avgSec < 10) cv.print("0");
    cv.print(avgSec);
    cv.print("\"");
  }

  int slowestLapIdx = -1;
  int maxTime = 0;
  for (int i = 0; i < GPSCalc::laps; i++) {
    if (GPSCalc::lapHistory[i].timeSec > maxTime) {
      maxTime = GPSCalc::lapHistory[i].timeSec;
      slowestLapIdx = i;
    }
  }

  for (int i = 0; i < GPSCalc::laps && i < 4; i++) {
    int y = 24 + i * 10;
    if (MenuManager::viewLapIdx == i + 1) cv.drawStr(0, y, ">");

    if (i == slowestLapIdx) {
      cv.drawBox(8, y - 8, 55, 10);
      cv.setDrawColor(0);
    }

    cv.setCursor(10, y);
    cv.print("L");
    cv.print(i + 1);
    cv.print(": ");
    cv.print((int)GPSCalc::lapHistory[i].pace);
    cv.print("'");
    int lapSec = (int)((GPSCalc::lapHistory[i].pace - (int)GPSCalc::lapHistory[i].pace) * 60);
    if (lapSec < 10) cv.print("0");
    cv.print(lapSec);
    cv.print("\"");

    cv.setDrawColor(1);
  }

  if (sysCfg.draw_track) drawTrackMap(cv, 70, 15, MenuManager::viewLapIdx);

  cv.setFont(u8g2_font_4x6_tf);
  cv.drawStr(70, 62, MenuManager::viewLapIdx == 0 ? "VIEW: ALL" : "VIEW: LAP");
}
