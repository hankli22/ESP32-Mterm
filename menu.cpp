#include "menu.h"
#include "hal.h"
#include "gpscalc.h"
#include "config.h"

PageState MenuManager::currentPage = PAGE_SPLASH;
int MenuManager::cursorIndex = 0;
bool MenuManager::isCursorVisible = true;
int MenuManager::setIdx = 0;
int MenuManager::setScroll = 0;
bool MenuManager::isEditing = false;
int MenuManager::viewLapIdx = 0;
int MenuManager::devMenuIdx = 0;
int MenuManager::satTxtScroll = 0;

uint32_t MenuManager::lastActiveTime = 0;
bool MenuManager::isScreenOff = false;

SystemConfig tempCfg;

float MenuManager::currentPageX = 0;
float MenuManager::currentCursorY = 45;
float MenuManager::visualSetCursorY = 0;
float MenuManager::visualSetScrollY = 0;

float MenuManager::smoothLerp(float current, float target, float speed) {
  if (abs(target - current) < 0.5f) return target;
  return current + (target - current) * speed;
}

void MenuManager::handleInput() {
    BtnEvent evt = HAL::getEvent();

    // 1. 【核心修复】：找回息屏唤醒拦截逻辑！
    // 必须放在 switch 之前，一旦息屏，第一下按键绝对不触发任何菜单操作
    if (evt != BTN_NONE) {
        if (isScreenOff) {
            isScreenOff = false;
            HAL::getDisplay()->setPowerSave(0); // 唤醒 OLED 硬件
            lastActiveTime = millis();          // 重置倒计时
            return; 
        }
        lastActiveTime = millis(); // 只要有按键操作，重置息屏倒计时
    }

    if (evt == BTN_NONE) return;

    switch (currentPage) {
        case PAGE_SPLASH:
            currentPage = PAGE_START;
            break;

        case PAGE_START:
            if (evt == BTN_UP_PRESSED) { cursorIndex = 0; isCursorVisible = true; }
            if (evt == BTN_DOWN_PRESSED) { cursorIndex = 1; isCursorVisible = true; }
            if (evt == BTN_LEFT_PRESSED) { isCursorVisible = false; cursorIndex = 0; }
            if (evt == BTN_RIGHT_PRESSED) {
                if (!isCursorVisible) { isCursorVisible = true; return; }
                if (cursorIndex == 0) { GPSCalc::startRun(); currentPage = PAGE_SPORT1; }
                if (cursorIndex == 1) { currentPage = PAGE_SETTINGS; setIdx = 0; setScroll = 0; isEditing = false; }
            }
            break;

        case PAGE_SETTINGS:
            if (!isEditing) {
                if (evt == BTN_UP_PRESSED) { setIdx--; if(setIdx < 0) setIdx = 0; }
                // 【修复】：共有 12 项，最大索引是 11
                if (evt == BTN_DOWN_PRESSED) { setIdx++; if(setIdx > 11) setIdx = 11; }
                if (evt == BTN_LEFT_PRESSED) { currentPage = PAGE_START; } 
                if (evt == BTN_RIGHT_PRESSED) {
                    // 2. 【核心修复】：严格对齐新版数组的索引
                    // 0:mode, 1:freq, 2:track, 3:scr_off, 4:pwr_btn, 5:storage, 6:multi_col
                    if (setIdx == 7) { saveConfig(); currentPage = PAGE_START; }           // [save]
                    else if (setIdx == 8) { HAL::sleepDevice(); }                          // [pwr_off]
                    else if (setIdx == 9) { currentPage = PAGE_DEV_MENU; devMenuIdx = 0; } // [dev_page]
                    else if (setIdx == 10) { Serial1.println("$PCAS04,1*18"); currentPage = PAGE_START; } // [gps_stdby]
                    else if (setIdx == 11) { } // 空白项，不操作
                    else { isEditing = true; tempCfg = sysCfg; }                 
                }
                if (setIdx < setScroll) setScroll = setIdx;
                if (setIdx > setScroll + 4) setScroll = setIdx - 4;
            } else {
                if (evt == BTN_LEFT_PRESSED || evt == BTN_RIGHT_PRESSED) {
                    if(setIdx == 1) {
                        if (sysCfg.record_freq == 5.0) sysCfg.record_freq = 2.0;
                        else if (sysCfg.record_freq == 2.0) sysCfg.record_freq = 1.0;
                        else if (sysCfg.record_freq == 1.0) sysCfg.record_freq = 0.5;
                        else sysCfg.record_freq = 5.0;
                    }
                    if(setIdx == 2) sysCfg.draw_track = !sysCfg.draw_track;
                    if(setIdx == 3) { 
                        if(sysCfg.screen_off == 30) sysCfg.screen_off = 60;
                        else if(sysCfg.screen_off == 60) sysCfg.screen_off = 300;
                        else if(sysCfg.screen_off == 300) sysCfg.screen_off = 0;
                        else sysCfg.screen_off = 30;
                    }
                    if(setIdx == 5) sysCfg.storage_track = (sysCfg.storage_track + 1) % 4; 
                    if(setIdx == 6) sysCfg.en_multycol = !sysCfg.en_multycol; // 【修复】：找回被误删的多颜色开关！
                }
                if (evt == BTN_UP_PRESSED) { sysCfg = tempCfg; isEditing = false; } 
                if (evt == BTN_DOWN_PRESSED) { isEditing = false; } 
            }
            break;

        case PAGE_DEV_MENU:
            if (evt == BTN_UP_PRESSED) { devMenuIdx--; if(devMenuIdx < 0) devMenuIdx = 0; }
            if (evt == BTN_DOWN_PRESSED) { devMenuIdx++; if(devMenuIdx > 2) devMenuIdx = 2; }
            if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_SETTINGS;
            if (evt == BTN_RIGHT_PRESSED) {
                if (devMenuIdx == 0) currentPage = PAGE_DEV;
                else if (devMenuIdx == 1) { currentPage = PAGE_SAT_TXT; satTxtScroll = 0; }
                else if (devMenuIdx == 2) currentPage = PAGE_SAT_GUI;
            }
            break;

        case PAGE_DEV:
            if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_DEV_MENU;
            break;

        case PAGE_SAT_TXT:
            if (evt == BTN_UP_PRESSED) { satTxtScroll -= 10; if(satTxtScroll < 0) satTxtScroll = 0; }
            if (evt == BTN_DOWN_PRESSED) { 
                satTxtScroll += 10; 
                // 防止一直按下键导致文字滚出屏幕
                int maxScroll = (GPSCalc::satCount * 10) - 20;
                if (maxScroll < 0) maxScroll = 0;
                if (satTxtScroll > maxScroll) satTxtScroll = maxScroll;
            }
            if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_DEV_MENU;
            break;

        case PAGE_SAT_GUI:
            // 3. 【防串台保护】：除了左键退出，其他按键坚决不响应
            if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_DEV_MENU;
            break;

        case PAGE_SPORT1:
            if (evt == BTN_LEFT_PRESSED) { GPSCalc::stopRun(); currentPage = PAGE_SUMMARY; viewLapIdx = 0; }
            if (evt == BTN_DOWN_PRESSED) currentPage = PAGE_SPORT2;
            break;

        case PAGE_SPORT2:
            if (evt == BTN_LEFT_PRESSED) { GPSCalc::stopRun(); currentPage = PAGE_SUMMARY; viewLapIdx = 0; }
            if (evt == BTN_UP_PRESSED) currentPage = PAGE_SPORT1;
            break;

        case PAGE_SUMMARY:
            if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_START;
            if (evt == BTN_UP_PRESSED) { viewLapIdx--; if(viewLapIdx < 0) viewLapIdx = 0; }
            if (evt == BTN_DOWN_PRESSED) { viewLapIdx++; if(viewLapIdx > GPSCalc::laps) viewLapIdx = GPSCalc::laps; }
            break;
    }
}

void MenuManager::update() {
  if (sysCfg.screen_off > 0 && !isScreenOff) {
    if (millis() - lastActiveTime > sysCfg.screen_off * 1000UL) {
      isScreenOff = true;
      HAL::getDisplay()->setPowerSave(1);
    }
  }
  if (isScreenOff) return;

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
  else if (currentPage == PAGE_SPORT1) targetX = -896;
  else if (currentPage == PAGE_SPORT2) targetX = -1024;
  else if (currentPage == PAGE_SUMMARY) targetX = -1152;

  currentPageX = smoothLerp(currentPageX, targetX, 0.2f);
  int ox = (int)currentPageX;

  // 【核心性能优化：视口裁剪】
  // 只有页面进入了屏幕的横向显示范围 (-128 到 128)，才执行繁重的绘制函数！
  if (ox > -128 && ox < 128) drawSplash(ox);
  if (ox + 128 > -128 && ox + 128 < 128) drawStartMenu(ox + 128);
  if (ox + 256 > -128 && ox + 256 < 128) drawSettings(ox + 256);
  if (ox + 384 > -128 && ox + 384 < 128) drawDevMenu(ox + 384);
  if (ox + 512 > -128 && ox + 512 < 128) drawDevPage(ox + 512);
  if (ox + 640 > -128 && ox + 640 < 128) drawSatTxt(ox + 640);
  if (ox + 768 > -128 && ox + 768 < 128) drawSatGui(ox + 768);
  if (ox + 896 > -128 && ox + 896 < 128) drawSport1(ox + 896);
  if (ox + 1024 > -128 && ox + 1024 < 128) drawSport2(ox + 1024);
  if (ox + 1152 > -128 && ox + 1152 < 128) drawSummary(ox + 1152);

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
  u8g2->print("右键确认,左键返回");
  u8g2->setCursor(ox + 5, 54);
  u8g2->print("我懒得调bug喵(");
  u8g2->setFont(u8g2_font_4x6_tf);
  u8g2->drawStr(ox + 70, 63, "Press RIGHT ->");
}

void MenuManager::drawStartMenu(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);

  u8g2->drawStr(ox + 0, 10, "Start");

  // 【动画】：如果没有定位，显示闪烁的省略号
  if (GPSCalc::satellites > 4) {
    u8g2->drawStr(ox + 40, 10, "[SAT:OK]");
  } else {
    const char* dots[] = { "[SAT:   ]", "[SAT:.  ]", "[SAT:.. ]", "[SAT:...]", "[SAT: ..]", "[SAT:  .]" };
    int frame = (millis() / 300) % 6;  // 每300毫秒换一帧
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

  u8g2->drawStr(ox + 10, 45, "mode: running");
  u8g2->drawStr(ox + 10, 60, "settings");
}

void MenuManager::drawSettings(int ox) {
  auto u8g2 = HAL::getDisplay();
  // 数组增加到 12 项，加入了 multi_col
  const char* titles[] = { "mode:", "freq:", "track:", "scr_off:", "pwr_btn:", "storage:", "multi_col:", "[save]", "[pwr_off]", "[dev_page]", "[gps_stdby]", "" };
  char vFreq[10], vScr[10], vStore[10];
  sprintf(vFreq, sysCfg.record_freq == 0.5 ? "0.5Hz" : "%dHz", (int)sysCfg.record_freq);
  sprintf(vScr, sysCfg.screen_off == 0 ? "never" : "%ds", sysCfg.screen_off);
  sprintf(vStore, sysCfg.storage_track == 0 ? "disable" : "%d", sysCfg.storage_track);

  // 对应加入 multi_col 的值
  const char* vals[] = { "running", vFreq, sysCfg.draw_track ? "yes" : "no", vScr, "hold_3s", vStore, sysCfg.en_multycol ? "yes" : "no", "", "", "", "", "" };

  visualSetScrollY = smoothLerp(visualSetScrollY, setScroll * 12, 0.2f);
  visualSetCursorY = smoothLerp(visualSetCursorY, setIdx * 12, 0.3f);

  for (int i = 0; i < 12; i++) {  // 循环改为 12
    float itemY = 20 + i * 12 - visualSetScrollY;
    if (itemY < 5 || itemY > 70) continue;
    u8g2->drawStr(ox + 10, (int)itemY, titles[i]);
    u8g2->drawStr(ox + 75, (int)itemY, vals[i]);
  }

  float curY = 20 + visualSetCursorY - visualSetScrollY;
  if (setIdx != 11) u8g2->drawStr(ox + 0, (int)curY, isEditing ? "*" : ">");  // 第12项(索引11)是空白项
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
  u8g2->print(GPSCalc::homeLat == 0 ? 0 : GPSCalc::lastLat, 6);
  u8g2->setCursor(ox + 0, 30);
  u8g2->print("Lng: ");
  u8g2->print(GPSCalc::homeLat == 0 ? 0 : GPSCalc::lastLng, 6);
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

void MenuManager::drawSport1(int ox) {
  auto u8g2 = HAL::getDisplay();

  if (GPSCalc::satellites < 4) {
    u8g2->setFont(u8g2_font_6x12_tr);
    u8g2->drawStr(ox + 20, 35, "WAITING GPS");

    // 【动画】：三个小点来回跳动
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

  u8g2->setCursor(ox + 0, 15);
  u8g2->print("Avg Pace: ");
  u8g2->print((int)avgPace);
  u8g2->print("'");
  u8g2->print((int)((avgPace - (int)avgPace) * 60));
  u8g2->print("\"");
  u8g2->setCursor(ox + 0, 30);
  u8g2->print("Altitude: ");
  u8g2->print(GPSCalc::altitude);
  u8g2->print(" m");
  u8g2->setCursor(ox + 0, 45);
  u8g2->print("Sats Lock: ");
  u8g2->print(GPSCalc::satellites);
  u8g2->setCursor(ox + 0, 60);
  u8g2->print("Course: ");
  u8g2->print(GPSCalc::course);
  u8g2->print(" deg");
}

void MenuManager::drawSummary(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->drawStr(ox + 2, 10, "Summary");

  // 【新增与修复】：安全计算总平均配速，加入 durationSec > 0 的除0保护
  float avgPace = 0;
  if (GPSCalc::totalDistance > 10 && GPSCalc::durationSec > 0) {
    float speedKmh = (GPSCalc::totalDistance / GPSCalc::durationSec) * 3.6f;
    if (speedKmh > 1.0f) avgPace = 60.0f / speedKmh;
  }

  // 在右上角显示全局平均配速
  u8g2->setCursor(ox + 60, 10);
  if (avgPace > 0) {
    u8g2->print("Avg:");
    u8g2->print((int)avgPace);
    u8g2->print("'");
    int avgSec = (int)((avgPace - (int)avgPace) * 60);
    if (avgSec < 10) u8g2->print("0");  // 补零逻辑
    u8g2->print(avgSec);
    u8g2->print("\"");
  }

  // 寻找最慢的圈 (耗时最大的圈)
  int slowestLapIdx = -1;
  int maxTime = 0;
  for (int i = 0; i < GPSCalc::laps; i++) {
    if (GPSCalc::lapHistory[i].timeSec > maxTime) {
      maxTime = GPSCalc::lapHistory[i].timeSec;
      slowestLapIdx = i;
    }
  }

  // 绘制前4圈的圈速列表
  for (int i = 0; i < GPSCalc::laps && i < 4; i++) {
    int y = 24 + i * 10;

    // 绘制光标 (如果在按键选择当前圈)
    if (viewLapIdx == i + 1) u8g2->drawStr(ox + 0, y, ">");

    // 最慢圈高亮反色背景
    if (i == slowestLapIdx) {
      u8g2->drawBox(ox + 8, y - 8, 55, 10);
      u8g2->setDrawColor(0);  // 把画笔变成黑色，这样字就是镂空的
    }

    u8g2->setCursor(ox + 10, y);
    u8g2->print("L");
    u8g2->print(i + 1);
    u8g2->print(": ");
    u8g2->print((int)GPSCalc::lapHistory[i].pace);
    u8g2->print("'");

    // 【UI细节修复】：秒数小于10时补零
    int lapSec = (int)((GPSCalc::lapHistory[i].pace - (int)GPSCalc::lapHistory[i].pace) * 60);
    if (lapSec < 10) u8g2->print("0");
    u8g2->print(lapSec);
    u8g2->print("\"");

    u8g2->setDrawColor(1);  // 必须恢复画笔颜色，否则后面的UI全黑了
  }

  // 绘制轨迹微缩地图
  if (sysCfg.draw_track) drawTrackMap(ox + 70, 15, viewLapIdx);

  // 底部视图状态提示
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


void MenuManager::drawDevMenu(int ox) {
  auto u8g2 = HAL::getDisplay();
  u8g2->setFont(u8g2_font_6x10_tf);
  u8g2->drawStr(ox + 2, 10, "--- DEV MENU ---");
  const char* items[] = { "1. Basic GPS Info", "2. [sat_view_text]", "3. [sat_view_gui]" };
  for (int i = 0; i < 3; i++) {
    if (devMenuIdx == i) u8g2->drawStr(ox + 2, 25 + i * 15, ">");
    u8g2->drawStr(ox + 12, 25 + i * 15, items[i]);
  }
}

// 1. 卫星文本信息页
void MenuManager::drawSatTxt(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_5x8_tf);
    int y = 20 - satTxtScroll; 
    const char* sName[] = {"GPS", "BDS", "GLO", "SBS"};
    
    for (int s = 0; s < 4; s++) {
        if (GPSCalc::sysInView[s] == 0) continue; 
        if (y > 10 && y < 70) {
            u8g2->setCursor(ox + 2, y);
            u8g2->print("["); u8g2->print(sName[s]); u8g2->print("*");
            u8g2->print(GPSCalc::sysTracked[s]); u8g2->print("/");
            u8g2->print(GPSCalc::sysInView[s]); u8g2->print("]");
        }
        y += 10;
        for (int i = 0; i < GPSCalc::satCount; i++) {
            if (GPSCalc::sats[i].sys == s) {
                if (y > 10 && y < 70) {
                    char buf[32];
                    sprintf(buf, " %02d   %02d   %02d   %03d", GPSCalc::sats[i].prn, GPSCalc::sats[i].snr, GPSCalc::sats[i].ele, GPSCalc::sats[i].azi);
                    u8g2->drawStr(ox + 2, y, buf);
                }
                y += 10;
            }
        }
    }
    u8g2->setDrawColor(0); u8g2->drawBox(ox, 0, 128, 12); u8g2->setDrawColor(1);
    u8g2->drawStr(ox + 2, 9, " PRN SNR ELE AZI");
    u8g2->drawLine(ox, 11, ox+128, 11);
} // <--- 这里一定要闭合这个函数！

// 2. 卫星图形化GUI页
void MenuManager::drawSatGui(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_4x6_tf);
    const char* sName[] = { "GPS", "BDS", "GLO", "SBS" };

    int cx = ox + 30, cy = 35, r = 22;
    int total = 0;
    for (int i = 0; i < 4; i++) total += GPSCalc::sysTracked[i];
    u8g2->drawCircle(cx, cy, r);

    if (total > 0) {
        float angle = -PI / 2;
        for (int i = 0; i < 4; i++) {
            if (GPSCalc::sysTracked[i] == 0) continue;
            float sweep = (GPSCalc::sysTracked[i] / (float)total) * 2 * PI;

        // 【优化】：放弃基于半径的步长，改为基于固定角度的步长
        if (sysCfg.en_multycol) {
            float angleStep = 0;
            // 密度越低，线条越少，CPU 负担越小，运行越流畅！
            if (i == 0)      angleStep = PI / 48.0f; // GPS:  (极密)
            else if (i == 1) angleStep = PI / 16.0f;  // BDS:  (中密)
            else if (i == 2) angleStep = PI / 9.0f;  // GLO: (稀疏)
            // SBS(i==3) 保持 0，全白

            if (angleStep > 0) {
                // 仅绘制从中心向边缘的线段
                for (float a = angle; a < angle + sweep; a += angleStep) {
                    // 【关键优化】：为了性能，只画圆周半径的 80%
                    // 不必从中心画到边缘，画一条短线效果是一样的
                    u8g2->drawLine(cx + (r*0.2f)*cos(a), cy + (r*0.2f)*sin(a), 
                                   cx + r*cos(a), cy + r*sin(a));
                }
            }
        }
            u8g2->drawLine(cx, cy, cx + r * cos(angle), cy + r * sin(angle));
            int lx = cx + (r + 8) * cos(angle + sweep / 2) - 6;
            int ly = cy + (r + 8) * sin(angle + sweep / 2) + 2;
            u8g2->setCursor(lx, ly);
            u8g2->print(sName[i]);
            angle += sweep;
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
                int tmp = top[i]; top[i] = top[j]; top[j] = tmp;
            }
        }
    }
    for (int i = 0; i < 5 && i < GPSCalc::satCount; i++) {
        int idx = top[i];
        if (GPSCalc::sats[idx].snr == 0) break;
        u8g2->setCursor(ox + 65, 18 + i * 9);
        u8g2->print("["); u8g2->print(sName[GPSCalc::sats[idx].sys]); u8g2->print("] ");
        u8g2->print(GPSCalc::sats[idx].prn); u8g2->print(" S:");
        u8g2->print(GPSCalc::sats[idx].snr);
    }
}