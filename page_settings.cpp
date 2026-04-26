#include "pages.h"
#include "ui.h"
#include "config.h"
#include <stdio.h>

void drawSettings(Canvas& cv) {
  cv.setFont(u8g2_font_6x10_tf);

  float vy = MenuManager::visualSetScrollY;
  float titleY = 12 - vy;
  if (titleY > -10) cv.drawStr(2, (int)titleY, "--- SETTINGS ---");

  const char* titles[] = {
    "mode:", "freq:", "track:", "scr_off:", "pwr_btn:",
    "storage:", "multi_col:", "contrast:", "auto_slp:",
    "[save]", "[pwr_off]", "[dev_page]", "[gps_stdby]", ""
  };

  char vFreq[10], vScr[10], vStore[10], vCont[10];
  sprintf(vFreq, sysCfg.record_freq == 0.5 ? "0.5Hz" : "%gHz", sysCfg.record_freq);
  sprintf(vScr, sysCfg.screen_off == 0 ? "never" : "%ds", sysCfg.screen_off);
  sprintf(vStore, sysCfg.storage_track == 0 ? "disable" : "%d", sysCfg.storage_track);
  sprintf(vCont, "Lv.%d", sysCfg.contrast);

  const char* vals[] = {
    "running", vFreq, sysCfg.draw_track ? "yes" : "no", vScr, "hold_3s",
    vStore, sysCfg.en_multycol ? "yes" : "no", vCont, sysCfg.auto_sleep ? "ON" : "OFF",
    "", "", "", "", ""
  };

  float curY = 27 + MenuManager::visualSetCursorY - vy;

  for (int i = 0; i < 14; i++) {
    float itemY = 27 + i * 15 - vy;
    if (itemY < 0 || itemY > 75) continue;

    float dist = abs(itemY - curY);
    float offsetX = (dist < 15.0f) ? 6.0f * (1.0f - (dist / 15.0f)) : 0;

    cv.drawStr(12 + (int)offsetX, (int)itemY, titles[i]);
    cv.drawStr(80 + (int)offsetX, (int)itemY, vals[i]);
  }

  if (MenuManager::setIdx != 13) cv.drawStr(2, (int)curY, MenuManager::isEditing ? "*" : ">");
}
