#include "ui.h"
#include "hardwareLayer.h"
#include "gps_module.h"
#include "config.h"
#include "trig_lut.h"

PageState MenuManager::currentPage = PAGE_SPLASH;
int MenuManager::cursorIndex = 0;
bool MenuManager::isCursorVisible = true;
int MenuManager::setIdx = 0;
int MenuManager::setScroll = 0;
bool MenuManager::isEditing = false;
int MenuManager::viewLapIdx = 0;

uint32_t MenuManager::lastActiveTime = 0;
bool MenuManager::isScreenOff = false;

SystemConfig tempCfg;

float MenuManager::currentPageX = 0;
float MenuManager::currentCursorY = 45;
float MenuManager::visualSetCursorY = 0;
float MenuManager::visualSetScrollY = 0;
int MenuManager::devMenuIdx = 0;
int MenuManager::satTxtScroll = 0;
int MenuManager::devScroll = 0;
float MenuManager::visualDevCursorY = 0;
float MenuManager::visualDevScrollY = 0;
float MenuManager::visualSatTxtScrollY = 0;

float MenuManager::smoothLerp(float current, float target, float speed) {
  if (abs(target - current) < 0.5f) return target;
  return current + (target - current) * speed;
}

void MenuManager::handleInput() {
  BtnEvent evt = HAL::getEvent();

  if (evt != BTN_NONE) {
    if (isScreenOff) {
      isScreenOff = false;
      HAL::getDisplay()->setPowerSave(0);
      lastActiveTime = millis();
      return;
    }
    lastActiveTime = millis();
  }

  if (evt == BTN_NONE) return;

  switch (currentPage) {
    case PAGE_SPLASH:
      currentPage = PAGE_START;
      break;

    case PAGE_START:
      if (evt == BTN_UP_PRESSED) {
        cursorIndex = 0;
        isCursorVisible = true;
      }
      if (evt == BTN_DOWN_PRESSED) {
        cursorIndex = 1;
        isCursorVisible = true;
      }
      if (evt == BTN_LEFT_PRESSED) {
        isCursorVisible = false;
        cursorIndex = 0;
      }
      if (evt == BTN_RIGHT_PRESSED) {
        if (!isCursorVisible) {
          isCursorVisible = true;
          return;
        }
        if (cursorIndex == 0) {
          GPSCalc::startRun();
          currentPage = PAGE_SPORT1;
        }
        if (cursorIndex == 1) {
          currentPage = PAGE_SETTINGS;
          setIdx = 0;
          setScroll = 0;
          isEditing = false;
        }
      }
      break;

    case PAGE_SETTINGS:
      if (!isEditing) {
        if (evt == BTN_UP_PRESSED) {
          setIdx--;
          if (setIdx < 0) setIdx = 0;
        }
        if (evt == BTN_DOWN_PRESSED) {
          setIdx++;
          if (setIdx > 13) setIdx = 13;
        }

        if (evt == BTN_LEFT_PRESSED) { currentPage = PAGE_START; }
        if (evt == BTN_RIGHT_PRESSED) {
          if (setIdx == 9) {
            saveConfig();
            currentPage = PAGE_START;
          }
          else if (setIdx == 10) { HAL::sleepDevice(); }
          else if (setIdx == 11) {
            currentPage = PAGE_DEV_MENU;
            devMenuIdx = 0;
          }
          else if (setIdx == 12) {
            Serial1.println("$PCAS04,1*18");
            currentPage = PAGE_START;
          }
          else if (setIdx == 13) {
          } else {
            isEditing = true;
            tempCfg = sysCfg;
          }
        }
        if (setIdx < setScroll) setScroll = setIdx;
        if (setIdx > setScroll + 4) setScroll = setIdx - 4;
      } else {
        if (evt == BTN_UP_PRESSED || evt == BTN_DOWN_PRESSED) {
          int dir = (evt == BTN_DOWN_PRESSED) ? 1 : -1;
          if (setIdx == 1) {
            if (dir > 0) {
              if (sysCfg.record_freq == 5.0) sysCfg.record_freq = 2.0;
              else if (sysCfg.record_freq == 2.0) sysCfg.record_freq = 1.0;
              else if (sysCfg.record_freq == 1.0) sysCfg.record_freq = 0.5;
              else sysCfg.record_freq = 5.0;
            } else {
              if (sysCfg.record_freq == 0.5) sysCfg.record_freq = 1.0;
              else if (sysCfg.record_freq == 1.0) sysCfg.record_freq = 2.0;
              else if (sysCfg.record_freq == 2.0) sysCfg.record_freq = 5.0;
              else sysCfg.record_freq = 0.5;
            }
          } else if (setIdx == 2) sysCfg.draw_track = !sysCfg.draw_track;
          else if (setIdx == 3) {
            if (dir > 0) {
              if (sysCfg.screen_off == 30) sysCfg.screen_off = 60;
              else if (sysCfg.screen_off == 60) sysCfg.screen_off = 300;
              else if (sysCfg.screen_off == 300) sysCfg.screen_off = 0;
              else sysCfg.screen_off = 30;
            } else {
              if (sysCfg.screen_off == 0) sysCfg.screen_off = 300;
              else if (sysCfg.screen_off == 300) sysCfg.screen_off = 60;
              else if (sysCfg.screen_off == 60) sysCfg.screen_off = 30;
              else sysCfg.screen_off = 0;
            }
          } else if (setIdx == 5) {
            if (dir > 0) sysCfg.storage_track = (sysCfg.storage_track + 1) % 4;
            else sysCfg.storage_track = (sysCfg.storage_track + 3) % 4;
          } else if (setIdx == 6) sysCfg.en_multycol = !sysCfg.en_multycol;
          else if (setIdx == 7) {
            if (dir > 0) {
              sysCfg.contrast++;
              if (sysCfg.contrast > 5) sysCfg.contrast = 1;
            } else {
              sysCfg.contrast--;
              if (sysCfg.contrast < 1) sysCfg.contrast = 5;
            }
            HAL::getDisplay()->setContrast(sysCfg.contrast * 51);
          } else if (setIdx == 8) sysCfg.auto_sleep = !sysCfg.auto_sleep;
        }

        if (evt == BTN_LEFT_PRESSED) {
          sysCfg = tempCfg;
          HAL::getDisplay()->setContrast(sysCfg.contrast * 51);
          isEditing = false;
        }
        if (evt == BTN_RIGHT_PRESSED) { isEditing = false; }
      }
      break;

    case PAGE_DEV_MENU:
      if (evt == BTN_UP_PRESSED) {
        devMenuIdx--;
        if (devMenuIdx < 0) devMenuIdx = 0;
      }
      if (evt == BTN_DOWN_PRESSED) {
        devMenuIdx++;
        if (devMenuIdx > 4) devMenuIdx = 4;
      }
      if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_SETTINGS;
      if (evt == BTN_RIGHT_PRESSED) {
        if (devMenuIdx == 0) currentPage = PAGE_DEV;
        else if (devMenuIdx == 1) {
          currentPage = PAGE_SAT_TXT;
          satTxtScroll = 0;
        } else if (devMenuIdx == 2) currentPage = PAGE_SAT_GUI;
        else if (devMenuIdx == 3) currentPage = PAGE_DEV_STAT;
        else if (devMenuIdx == 4) {}
      }
      if (devMenuIdx < devScroll) devScroll = devMenuIdx;
      if (devMenuIdx > devScroll + 2) devScroll = devMenuIdx - 2;
      break;

    case PAGE_DEV_STAT:
    case PAGE_DEV:
      if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_DEV_MENU;
      break;

    case PAGE_SAT_TXT:
      if (evt == BTN_UP_PRESSED) {
        satTxtScroll -= 10;
        if (satTxtScroll < 0) satTxtScroll = 0;
      }
      if (evt == BTN_DOWN_PRESSED) {
        satTxtScroll += 10;
        int maxScroll = (GPSCalc::satCount * 10) - 20;
        if (maxScroll < 0) maxScroll = 0;
        if (satTxtScroll > maxScroll) satTxtScroll = maxScroll;
      }
      if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_DEV_MENU;
      break;

    case PAGE_SAT_GUI:
      if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_DEV_MENU;
      break;

    case PAGE_SPORT1:
      if (evt == BTN_LEFT_PRESSED) {
        GPSCalc::stopRun();
        currentPage = PAGE_SUMMARY;
        viewLapIdx = 0;
      }
      if (evt == BTN_DOWN_PRESSED) currentPage = PAGE_SPORT2;
      if (evt == BTN_UP_PRESSED) currentPage = PAGE_SPORT3;
      break;

    case PAGE_SPORT2:
      if (evt == BTN_LEFT_PRESSED) {
        GPSCalc::stopRun();
        currentPage = PAGE_SUMMARY;
        viewLapIdx = 0;
      }
      if (evt == BTN_DOWN_PRESSED) currentPage = PAGE_SPORT3;
      if (evt == BTN_UP_PRESSED) currentPage = PAGE_SPORT1;
      break;

    case PAGE_SPORT3:
      if (evt == BTN_LEFT_PRESSED) {
        GPSCalc::stopRun();
        currentPage = PAGE_SUMMARY;
        viewLapIdx = 0;
      }
      if (evt == BTN_DOWN_PRESSED) currentPage = PAGE_SPORT1;
      if (evt == BTN_UP_PRESSED) currentPage = PAGE_SPORT2;
      break;

    case PAGE_SUMMARY:
      if (evt == BTN_LEFT_PRESSED) {
        if (sysCfg.screen_off == 0) currentPage = PAGE_START;
        else HAL::sleepDevice();
      }
      if (evt == BTN_UP_PRESSED) {
        viewLapIdx--;
        if (viewLapIdx < 0) viewLapIdx = 0;
      }
      if (evt == BTN_DOWN_PRESSED) {
        viewLapIdx++;
        if (viewLapIdx > GPSCalc::laps) viewLapIdx = GPSCalc::laps;
      }
      break;
  }
}

void MenuManager::update() {
  static bool contrastInit = false;
  if (!contrastInit) {
    HAL::getDisplay()->setContrast(sysCfg.contrast * 51);
    contrastInit = true;
  }

  if (sysCfg.screen_off > 0 && !isScreenOff) {
    if (millis() - lastActiveTime > sysCfg.screen_off * 1000UL) {
      isScreenOff = true;
      HAL::getDisplay()->setPowerSave(1);
    }
  }
  if (isScreenOff) return;

  GPSCalc::lock();
  auto u8g2 = HAL::getDisplay();
  u8g2->clearBuffer();

  float targetX = 0;
  if (currentPage == PAGE_SPLASH) targetX = 0;
  else if (currentPage == PAGE_START) targetX = -128;
  else if (currentPage == PAGE_SETTINGS) targetX = -256;
  else if (currentPage == PAGE_DEV_MENU) targetX = -384;
  else if (currentPage == PAGE_DEV) targetX = -512;
  else if (currentPage == PAGE_SAT_TXT) targetX = -640;
  else if (currentPage == PAGE_SAT_GUI) targetX = -768;
  else if (currentPage == PAGE_DEV_STAT) targetX = -896;
  else if (currentPage == PAGE_SPORT1) targetX = -1024;
  else if (currentPage == PAGE_SPORT2) targetX = -1152;
  else if (currentPage == PAGE_SPORT3) targetX = -1280;
  else if (currentPage == PAGE_SUMMARY) targetX = -1408;

  currentPageX = smoothLerp(currentPageX, targetX, 0.2f);
  int ox = (int)currentPageX;

  if (ox > -128 && ox < 128) drawSplash(ox);
  if (ox + 128 > -128 && ox + 128 < 128) drawStartMenu(ox + 128);
  if (ox + 256 > -128 && ox + 256 < 128) drawSettings(ox + 256);
  if (ox + 384 > -128 && ox + 384 < 128) drawDevMenu(ox + 384);
  if (ox + 512 > -128 && ox + 512 < 128) drawDevPage(ox + 512);
  if (ox + 640 > -128 && ox + 640 < 128) drawSatTxt(ox + 640);
  if (ox + 768 > -128 && ox + 768 < 128) drawSatGui(ox + 768);
  if (ox + 896 > -128 && ox + 896 < 128) drawDevStat(ox + 896);
  if (ox + 1024 > -128 && ox + 1024 < 128) drawSport1(ox + 1024);
  if (ox + 1152 > -128 && ox + 1152 < 128) drawSport2(ox + 1152);
  if (ox + 1280 > -128 && ox + 1280 < 128) drawSport3(ox + 1280);
  if (ox + 1408 > -128 && ox + 1408 < 128) drawSummary(ox + 1408);

  GPSCalc::unlock();
  u8g2->sendBuffer();
}

void MenuManager::drawSplash(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_wqy12_t_gb2312);
  u8g2->setCursor(ox + 5, 12);
  u8g2->print("欢迎使用跑表");
  u8g2->setCursor(ox + 5, 26);
  u8g2->print("卫星闪烁后起跑");
  u8g2->setCursor(ox + 5, 40);
  u8g2->print("左右确认取消，上下值");
  u8g2->setCursor(ox + 5, 54);
  u8g2->print("");
  u8g2->setFont(u8g2_font_4x6_tf);
  u8g2->drawStr(ox + 70, 63, "Press RIGHT ->");
}

void MenuManager::drawStartMenu(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);

  u8g2->drawStr(ox + 0, 10, "Start");
  if (GPSCalc::satellites > 4) {
    u8g2->drawStr(ox + 40, 10, "[SAT:OK]");
  } else {
    const char* dots[] = { "[SAT:   ]", "[SAT:.  ]", "[SAT:.. ]", "[SAT:...]", "[SAT: ..]", "[SAT:  .]" };
    int frame = (millis() / 300) % 6;
    u8g2->drawStr(ox + 40, 10, dots[frame]);
  }

  int pct = HAL::getBatteryPercent();
  u8g2->drawFrame(ox + 93, 2, 18, 8);
  u8g2->drawBox(ox + 111, 4, 2, 4);
  int fillW = (pct * 14) / 100;
  if (fillW > 0) u8g2->drawBox(ox + 95, 4, fillW, 4);
  u8g2->setFont(u8g2_font_5x8_tf);
  u8g2->setCursor(ox + 114, 10);
  u8g2->print(pct);
  u8g2->print("%");

  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->setCursor(ox + 0, 25);
  u8g2->print(GPSCalc::getDateTime().c_str());

  float targetY = 45 + cursorIndex * 15;
  currentCursorY = smoothLerp(currentCursorY, targetY, 0.3f);

  if (isCursorVisible) u8g2->drawStr(ox + 0, (int)currentCursorY, ">");
  else {
    u8g2->drawStr(ox + 0, 45, "-");
    u8g2->drawStr(ox + 0, 60, "-");
  }

  // 【新增】：主页面的横向排斥弹性动画
  float dist1 = abs(45 - currentCursorY);
  float dist2 = abs(60 - currentCursorY);
  float ox1 = (dist1 < 15.0f && isCursorVisible) ? 6.0f * (1.0f - (dist1 / 15.0f)) : 0;
  float ox2 = (dist2 < 15.0f && isCursorVisible) ? 6.0f * (1.0f - (dist2 / 15.0f)) : 0;

  u8g2->drawStr(ox + 10 + (int)ox1, 45, "mode: running");
  u8g2->drawStr(ox + 10 + (int)ox2, 60, "settings");
}


void MenuManager::drawSettings(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);

  visualSetScrollY = smoothLerp(visualSetScrollY, setScroll * 15, 0.2f);
  visualSetCursorY = smoothLerp(visualSetCursorY, setIdx * 15, 0.3f);

  float titleY = 12 - visualSetScrollY;
  if (titleY > -10) u8g2->drawStr(ox + 2, (int)titleY, "--- SETTINGS ---");

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

  float curY = 27 + visualSetCursorY - visualSetScrollY;

  for (int i = 0; i < 14; i++) {
    float itemY = 27 + i * 15 - visualSetScrollY;
    if (itemY < 0 || itemY > 75) continue;

    float dist = abs(itemY - curY);
    float offsetX = (dist < 15.0f) ? 6.0f * (1.0f - (dist / 15.0f)) : 0;

    u8g2->drawStr(ox + 12 + (int)offsetX, (int)itemY, titles[i]);
    u8g2->drawStr(ox + 80 + (int)offsetX, (int)itemY, vals[i]);
  }

  if (setIdx != 13) u8g2->drawStr(ox + 2, (int)curY, isEditing ? "*" : ">");
}

void MenuManager::drawDevMenu(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);

  visualDevScrollY = smoothLerp(visualDevScrollY, devScroll * 15, 0.2f);
  visualDevCursorY = smoothLerp(visualDevCursorY, devMenuIdx * 15, 0.3f);

  float titleY = 12 - visualDevScrollY;
  if (titleY > -10) u8g2->drawStr(ox + 2, (int)titleY, "--- DEV MENU ---");

  const char* items[] = { "1. Basic Info", "2. [sat_view_txt]", "3.[sat_view_gui]", "4. [dev_stat]", "" };
  float curY = 27 + visualDevCursorY - visualDevScrollY;

  for (int i = 0; i < 5; i++) {
    float itemY = 27 + i * 15 - visualDevScrollY;
    if (itemY < 0 || itemY > 70) continue;

    float dist = abs(itemY - curY);
    float offsetX = 0;
    if (dist < 15.0f) offsetX = 6.0f * (1.0f - (dist / 15.0f));

    u8g2->drawStr(ox + 12 + (int)offsetX, (int)itemY, items[i]);
  }
  if (devMenuIdx != 4) u8g2->drawStr(ox + 2, (int)curY, ">");
}

void MenuManager::drawDevPage(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_5x8_tf);
  u8g2->setCursor(ox + 0, 10);
  u8g2->print("Acc: ");
  u8g2->print(GPSCalc::accuracyPct);
  u8g2->print("%");
  u8g2->setCursor(ox + 60, 10);
  u8g2->print("l:");
  u8g2->print(GPSCalc::satellites);
  u8g2->print(" c:");
  u8g2->print(GPSCalc::satsInView);
  u8g2->setCursor(ox + 0, 20);
  u8g2->print("Lat: ");
  u8g2->print(GPSCalc::rawLat, 6);
  u8g2->setCursor(ox + 0, 30);
  u8g2->print("Lng: ");
  u8g2->print(GPSCalc::rawLng, 6);
  u8g2->setCursor(ox + 0, 40);
  u8g2->print("Height: ");
  u8g2->print(GPSCalc::altitude);
  u8g2->print("m");
  u8g2->setCursor(ox + 0, 50);
  u8g2->print("Speed: ");
  u8g2->print(GPSCalc::currentSpeed);
  u8g2->print("kmh");
  u8g2->setCursor(ox + 0, 60);
  u8g2->print("Angle: ");
  u8g2->print(GPSCalc::course);
}

void MenuManager::drawDevStat(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_5x8_tf);
  u8g2->setCursor(ox + 2, 12);
  u8g2->print("--- DEV STAT ---");
  u8g2->setCursor(ox + 2, 28);
  u8g2->print("Heap: ");
  u8g2->print(ESP.getFreeHeap() / 1024);
  u8g2->print(" KB");
  u8g2->setCursor(ox + 2, 40);
  u8g2->print("Min Heap: ");
  u8g2->print(ESP.getMinFreeHeap() / 1024);
  u8g2->print(" KB");
  u8g2->setCursor(ox + 2, 52);
  u8g2->print("GPS Rdy: ");
  if (GPSCalc::isGpsReady()) u8g2->print("YES (UART OK)");
  else u8g2->print("NO (NO DATA)");
}

void MenuManager::drawSatTxt(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_5x8_tf);

  visualSatTxtScrollY = smoothLerp(visualSatTxtScrollY, satTxtScroll, 0.2f);
  float y = 20 - visualSatTxtScrollY;
  const char* sName[] = { "GPS", "BDS", "GLO", "SBS" };

  for (int s = 0; s < 4; s++) {
    if (GPSCalc::sysInView[s] == 0) continue;
    if (y > 10 && y < 70) {
      float dist = abs(y - 36);
      float oxOffset = (dist < 25) ? 4.0f * (1.0f - dist / 25.0f) : 0;
      u8g2->setCursor(ox + 2 + (int)oxOffset, (int)y);
      u8g2->print("[");
      u8g2->print(sName[s]);
      u8g2->print("*");
      u8g2->print(GPSCalc::sysTracked[s]);
      u8g2->print("/");
      u8g2->print(GPSCalc::sysInView[s]);
      u8g2->print("]");
    }
    y += 10;
    for (int i = 0; i < GPSCalc::satCount; i++) {
      if (GPSCalc::sats[i].sys == s) {
        if (y > 10 && y < 70) {
          float dist = abs(y - 36);
          float oxOffset = (dist < 25) ? 4.0f * (1.0f - dist / 25.0f) : 0;
          char buf[32];
          sprintf(buf, " %02d   %02d   %02d   %03d", GPSCalc::sats[i].prn, GPSCalc::sats[i].snr, GPSCalc::sats[i].ele, GPSCalc::sats[i].azi);
          u8g2->drawStr(ox + 2 + (int)oxOffset, (int)y, buf);
        }
        y += 10;
      }
    }
  }

  int maxScroll = (GPSCalc::satCount * 10) - 20;
  if (maxScroll > 0) {
    int barY = 15 + (int)((visualSatTxtScrollY / maxScroll) * 40);
    u8g2->drawBox(ox + 125, barY, 2, 8);
  }

  u8g2->setDrawColor(0);
  u8g2->drawBox(ox, 0, 126, 12);
  u8g2->setDrawColor(1);
  u8g2->drawStr(ox + 2, 9, " PRN SNR ELE AZI");
  u8g2->drawLine(ox, 11, ox + 125, 11);
}

void MenuManager::drawSatGui(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_4x6_tf);
  const char* sName[] = { "GPS", "BDS", "GLO", "SBS" };
  const uint8_t drawOrder[] = { 1, 0, 2, 3 };  // BDS 优先

  static const uint8_t bayer[4][4] = {
    { 0,  8,  2, 10},
    {12,  4, 14,  6},
    { 3, 11,  1,  9},
    {15,  7, 13,  5}
  };

  int cx = ox + 30, cy = 35, r = 22;
  int total = 0;
  for (int i = 0; i < 4; i++) total += GPSCalc::sysTracked[i];
  u8g2->drawCircle(cx, cy, r);

  if (total > 0) {
    // 预计算扇区边界（12 点方向顺时针）
    float startAng[4], endAng[4];
    uint8_t secSys[4];
    int secN = 0;
    float cur = -PI / 2;
    for (int di = 0; di < 4; di++) {
      uint8_t s = drawOrder[di];
      if (GPSCalc::sysTracked[s] == 0) continue;
      float sweep = (GPSCalc::sysTracked[s] / (float)total) * 2 * PI;
      startAng[secN] = cur;
      endAng[secN]   = cur + sweep;
      secSys[secN]   = s;
      secN++;
      cur += sweep;
    }

    // 预计算扇区边界法向量（用 LUT 替代 cosf/sinf）
    float sn[4], cs[4], en[4], ce[4];
    bool wide[4];
    for (int i = 0; i < secN; i++) {
      int sd = (int)(startAng[i] * 57.29578f + 0.5f);
      int ed = (int)(endAng[i]   * 57.29578f + 0.5f);
      sd = (sd % 360 + 360) % 360;
      ed = (ed % 360 + 360) % 360;
      cs[i] = cos_lut[sd]; sn[i] = sin_lut[sd];
      ce[i] = cos_lut[ed]; en[i] = sin_lut[ed];
      wide[i] = (endAng[i] - startAng[i]) > PI;
    }
    const uint8_t density[4] = { 12, 8, 4, 16 };

    // 直接写 U8g2 帧缓冲，避免 ~760 次 drawPixel 调用开销
    uint8_t *buf = u8g2->getBufferPtr();
    int rSq = r * r;
    for (int py = cy - r; py <= cy + r; py++) {
      uint8_t page = py >> 3;
      uint8_t bit  = 1 << (py & 7);
      int rowOff = page * 128;
      for (int px = cx - r; px <= cx + r; px++) {
        int dx = px - cx, dy = py - cy;
        if (dx * dx + dy * dy > rSq) continue;

        int sec = -1;
        for (int s = 0; s < secN; s++) {
          bool cw  = cs[s] * dx + sn[s] * dy >= 0;
          bool ccw = ce[s] * dx + en[s] * dy <= 0;
          if (wide[s] ? (cw || ccw) : (cw && ccw)) {
            sec = secSys[s]; break;
          }
        }
        if (sec < 0) continue;

        uint8_t th = sysCfg.en_multycol ? density[sec] : 16;
        if (bayer[(py - cy) & 3][(px - cx) & 3] < th) {
          buf[rowOff + px] |= bit;
        }
      }
    }

    // 扇区分隔线 + 标签（用 LUT）
    for (int i = 0; i < secN; i++) {
      int ad = (int)(startAng[i] * 57.29578f + 0.5f);
      ad = (ad % 360 + 360) % 360;
      float sa_cos = cos_lut[ad], sa_sin = sin_lut[ad];
      u8g2->drawLine(cx, cy, cx + r * sa_cos, cy + r * sa_sin);

      int md = (int)((startAng[i] + endAng[i]) * 0.5f * 57.29578f + 0.5f);
      md = (md % 360 + 360) % 360;
      float mid_cos = cos_lut[md], mid_sin = sin_lut[md];
      int lx = cx + (r + 8) * mid_cos - 6;
      int ly = cy + (r + 8) * mid_sin + 2;
      u8g2->setCursor(lx, ly);
      u8g2->print(sName[secSys[i]]);
    }
  } else {
    u8g2->drawStr(cx - 10, cy + 2, "NO SAT");
  }

  u8g2->drawStr(ox + 65, 8, "TOP 5 SATS:");
  int top[40];
  for (int i = 0; i < GPSCalc::satCount; i++) top[i] = i;
  for (int i = 0; i < GPSCalc::satCount - 1; i++) {
    for (int j = i + 1; j < GPSCalc::satCount; j++) {
      if (GPSCalc::sats[top[j]].snr > GPSCalc::sats[top[i]].snr) {
        int tmp = top[i];
        top[i] = top[j];
        top[j] = tmp;
      }
    }
  }
  for (int i = 0; i < 5 && i < GPSCalc::satCount; i++) {
    int idx = top[i];
    if (GPSCalc::sats[idx].snr == 0) break;
    u8g2->setCursor(ox + 65, 18 + i * 9);
    u8g2->print("[");
    u8g2->print(sName[GPSCalc::sats[idx].sys]);
    u8g2->print("] ");
    u8g2->print(GPSCalc::sats[idx].prn);
    u8g2->print(" S:");
    u8g2->print(GPSCalc::sats[idx].snr);
  }
}

void MenuManager::drawSport1(int ox) {
  auto u8g2 = HAL::getDisplay();

  if (GPSCalc::satellites < 4) {
    u8g2->setFont(u8g2_font_6x12_tr);
    u8g2->drawStr(ox + 20, 35, "WAITING GPS");
    int dotCount = (millis() / 500) % 4;
    const char* d[] = { "", ".", "..", "..." };
    u8g2->drawStr(ox + 92, 35, d[dotCount]);
    u8g2->setCursor(ox + 45, 50);
    u8g2->print("SATS: ");
    u8g2->print(GPSCalc::satellites);
    return;
  }

  u8g2->setFont(u8g2_font_6x12_tf);
  u8g2->setCursor(ox + 0, 12);
  u8g2->print("Distance: ");
  u8g2->print((int)GPSCalc::totalDistance);
  u8g2->print(" m");

  u8g2->setCursor(ox + 0, 24);
  u8g2->print("Speed: ");
  u8g2->print(GPSCalc::currentSpeed, 1);
  u8g2->print(" km/h");

  u8g2->setCursor(ox + 0, 36);
  u8g2->print("Pace(10s): ");
  if (GPSCalc::paceMin > 0) {
    u8g2->print(GPSCalc::paceMin);
    u8g2->print(":");
    if (GPSCalc::paceSec < 10) u8g2->print("0");
    u8g2->print(GPSCalc::paceSec);
    u8g2->print(" /km");
  } else u8g2->print("--:-- /km");

  u8g2->setCursor(ox + 0, 48);
  u8g2->print("time: ");
  u8g2->print(GPSCalc::durationSec / 60);
  u8g2->print(":");
  if (GPSCalc::durationSec % 60 < 10) u8g2->print("0");
  u8g2->print(GPSCalc::durationSec % 60);

  u8g2->setCursor(ox + 0, 60);
  u8g2->print("turns: ");
  u8g2->print(GPSCalc::laps);
}

void MenuManager::drawSport2(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x12_tf);

  float avgPace = 0;
  if (GPSCalc::totalDistance > 10 && GPSCalc::durationSec > 0) avgPace = 60.0 / ((GPSCalc::totalDistance / GPSCalc::durationSec) * 3.6);

  u8g2->setCursor(ox + 0, 12);
  u8g2->print("Avg Pace: ");
  u8g2->print((int)avgPace);
  u8g2->print("'");
  int avgSec = (int)((avgPace - (int)avgPace) * 60);
  if (avgSec < 10) u8g2->print("0");
  u8g2->print(avgSec);
  u8g2->print("\"");

  u8g2->setCursor(ox + 0, 24);
  u8g2->print("Max Spd:  ");
  u8g2->print(GPSCalc::maxSpeed, 1);
  u8g2->print(" km/h");
  u8g2->setCursor(ox + 0, 36);
  u8g2->print("Calories: ");
  u8g2->print(GPSCalc::calories);
  u8g2->print(" kcal");
  u8g2->setCursor(ox + 0, 48);
  u8g2->print("Altitude: ");
  u8g2->print(GPSCalc::altitude);
  u8g2->print(" m");
  u8g2->setCursor(ox + 0, 60);
  u8g2->print("Sats Lck: ");
  u8g2->print(GPSCalc::satellites);
}

void MenuManager::drawSport3(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->drawStr(ox + 2, 10, "Live Track");

  u8g2->setCursor(ox + 2, 25);
  u8g2->print("D: ");
  u8g2->print((int)GPSCalc::totalDistance);
  u8g2->setCursor(ox + 2, 40);
  u8g2->print("P: ");
  u8g2->print(GPSCalc::paceMin);
  u8g2->print("'");
  u8g2->print(GPSCalc::paceSec);
  u8g2->setCursor(ox + 2, 55);
  u8g2->print("C: ");
  u8g2->print(GPSCalc::calories);
  drawTrackMap(ox + 70, 12, 0);
}

void MenuManager::drawSummary(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->drawStr(ox + 2, 10, "Summary");

  float avgPace = 0;
  if (GPSCalc::totalDistance > 10 && GPSCalc::durationSec > 0) {
    float speedKmh = (GPSCalc::totalDistance / GPSCalc::durationSec) * 3.6f;
    if (speedKmh > 1.0f) avgPace = 60.0f / speedKmh;
  }

  u8g2->setCursor(ox + 60, 10);
  if (avgPace > 0) {
    u8g2->print("Avg:");
    u8g2->print((int)avgPace);
    u8g2->print("'");
    int avgSec = (int)((avgPace - (int)avgPace) * 60);
    if (avgSec < 10) u8g2->print("0");
    u8g2->print(avgSec);
    u8g2->print("\"");
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
    if (viewLapIdx == i + 1) u8g2->drawStr(ox + 0, y, ">");

    if (i == slowestLapIdx) {
      u8g2->drawBox(ox + 8, y - 8, 55, 10);
      u8g2->setDrawColor(0);
    }

    u8g2->setCursor(ox + 10, y);
    u8g2->print("L");
    u8g2->print(i + 1);
    u8g2->print(": ");
    u8g2->print((int)GPSCalc::lapHistory[i].pace);
    u8g2->print("'");
    int lapSec = (int)((GPSCalc::lapHistory[i].pace - (int)GPSCalc::lapHistory[i].pace) * 60);
    if (lapSec < 10) u8g2->print("0");
    u8g2->print(lapSec);
    u8g2->print("\"");

    u8g2->setDrawColor(1);
  }

  if (sysCfg.draw_track) drawTrackMap(ox + 70, 15, viewLapIdx);

  u8g2->setFont(u8g2_font_4x6_tf);
  u8g2->drawStr(ox + 70, 62, viewLapIdx == 0 ? "VIEW: ALL" : "VIEW: LAP");
}

void MenuManager::drawTrackMap(int ox, int oy, int lapIdx) {
  auto u8g2 = HAL::getDisplay();
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
    u8g2->drawLine(px1, py1, px2, py2);
  }
  u8g2->drawFrame(ox - 2, oy - 2, 48, 48);
}