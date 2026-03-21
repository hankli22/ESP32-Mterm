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
                if (evt == BTN_DOWN_PRESSED) { setIdx++; if(setIdx > 9) setIdx = 9; }
                if (evt == BTN_LEFT_PRESSED) { currentPage = PAGE_START; } 
                if (evt == BTN_RIGHT_PRESSED) {
                    if (setIdx == 6) { saveConfig(); currentPage = PAGE_START; } 
                    else if (setIdx == 7) { HAL::sleepDevice(); }                
                    else if (setIdx == 8) { currentPage = PAGE_DEV; }
                    else if (setIdx == 9) { 
                        Serial1.println("$PCAS04,1*18"); 
                        currentPage = PAGE_START;
                    }
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
                }
                if (evt == BTN_UP_PRESSED) { sysCfg = tempCfg; isEditing = false; } 
                if (evt == BTN_DOWN_PRESSED) { isEditing = false; } 
            }
            break;

        case PAGE_DEV:
            if (evt == BTN_LEFT_PRESSED) currentPage = PAGE_SETTINGS;
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
    else if (currentPage == PAGE_DEV) targetX = -384;
    else if (currentPage == PAGE_SPORT1) targetX = -512;
    else if (currentPage == PAGE_SPORT2) targetX = -640;
    else if (currentPage == PAGE_SUMMARY) targetX = -768;

    currentPageX = smoothLerp(currentPageX, targetX, 0.2f);
    int ox = (int)currentPageX;

    drawSplash(ox);
    drawStartMenu(ox + 128);
    drawSettings(ox + 256);
    drawDevPage(ox + 384);
    drawSport1(ox + 512);
    drawSport2(ox + 640);
    drawSummary(ox + 768);

    u8g2->sendBuffer();
}

void MenuManager::drawSplash(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_wqy12_t_gb2312); 
    u8g2->setCursor(ox + 5, 12); u8g2->print("欢迎使用跑表");
    u8g2->setCursor(ox + 5, 26); u8g2->print("卫星闪烁后起跑");
    u8g2->setCursor(ox + 5, 40); u8g2->print("右键确认,左键返回");
    u8g2->setCursor(ox + 5, 54); u8g2->print("我懒得调bug喵(");
    u8g2->setFont(u8g2_font_4x6_tf);
    u8g2->drawStr(ox + 70, 63, "Press RIGHT ->");
}

void MenuManager::drawStartMenu(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_6x10_tf);
    
    u8g2->drawStr(ox + 0, 10, "Start");
    if (GPSCalc::satellites > 4) u8g2->drawStr(ox + 40, 10, "[SAT:OK]");
    else u8g2->drawStr(ox + 40, 10, "[SAT:--]");
    
    int pct = HAL::getBatteryPercent();
    u8g2->drawFrame(ox + 93, 2, 18, 8); 
    u8g2->drawBox(ox + 111, 4, 2, 4);   
    int fillW = (pct * 14) / 100;       
    if(fillW > 0) u8g2->drawBox(ox + 95, 4, fillW, 4); 
    u8g2->setFont(u8g2_font_5x8_tf);
    u8g2->setCursor(ox + 114, 10);
    u8g2->print(pct); u8g2->print("%");

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
    u8g2->setFont(u8g2_font_6x10_tf);

    const char* titles[] = {"mode:", "freq:", "track:", "scr_off:", "pwr_btn:", "storage:", "[save]", "[pwr_off]", "[dev_page]", "[gps_stdby]"};
    char vFreq[10], vScr[10], vStore[10];
    sprintf(vFreq, sysCfg.record_freq == 0.5 ? "0.5Hz" : "%dHz", (int)sysCfg.record_freq);
    sprintf(vScr, sysCfg.screen_off == 0 ? "never" : "%ds", sysCfg.screen_off);
    sprintf(vStore, sysCfg.storage_track == 0 ? "disable" : "%d", sysCfg.storage_track);
    
    const char* vals[] = {"running", vFreq, sysCfg.draw_track?"yes":"no", vScr, "hold_3s", vStore, "", "", "", ""};

    visualSetScrollY = smoothLerp(visualSetScrollY, setScroll * 12, 0.2f);
    visualSetCursorY = smoothLerp(visualSetCursorY, setIdx * 12, 0.3f);

    for (int i=0; i<10; i++) {  
        float itemY = 20 + i * 12 - visualSetScrollY;
        if (itemY < 5 || itemY > 70) continue; 
        u8g2->drawStr(ox + 10, (int)itemY, titles[i]);
        u8g2->drawStr(ox + 75, (int)itemY, vals[i]);
    }

    float curY = 20 + visualSetCursorY - visualSetScrollY;
    u8g2->drawStr(ox + 0, (int)curY, isEditing ? "*" : ">"); 
}

void MenuManager::drawDevPage(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_5x8_tf);
    
    u8g2->setCursor(ox + 0, 10); u8g2->print("Acc: "); u8g2->print(GPSCalc::accuracyPct); u8g2->print("%");
    u8g2->setCursor(ox + 60, 10); u8g2->print("l:"); u8g2->print(GPSCalc::satellites); u8g2->print(" c:"); u8g2->print(GPSCalc::satsInView);
    
    u8g2->setCursor(ox + 0, 20); u8g2->print("Lat: "); u8g2->print(GPSCalc::homeLat == 0 ? 0 : GPSCalc::lastLat, 6);
    u8g2->setCursor(ox + 0, 30); u8g2->print("Lng: "); u8g2->print(GPSCalc::homeLat == 0 ? 0 : GPSCalc::lastLng, 6);
    u8g2->setCursor(ox + 0, 40); u8g2->print("Height: "); u8g2->print(GPSCalc::altitude); u8g2->print("m");
    u8g2->setCursor(ox + 0, 50); u8g2->print("Speed: "); u8g2->print(GPSCalc::currentSpeed); u8g2->print("kmh");
    u8g2->setCursor(ox + 0, 60); u8g2->print("Angle: "); u8g2->print(GPSCalc::course);
}

void MenuManager::drawSport1(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_6x12_tf);
    
    u8g2->setCursor(ox + 0, 12); 
    u8g2->print("Distance: "); u8g2->print((int)GPSCalc::totalDistance); u8g2->print(" m");
    
    u8g2->setCursor(ox + 0, 24); 
    u8g2->print("Speed: "); u8g2->print(GPSCalc::currentSpeed, 1); u8g2->print(" km/h");
    
    u8g2->setCursor(ox + 0, 36); 
    u8g2->print("Pace(10s): "); 
    if (GPSCalc::paceMin > 0) { u8g2->print(GPSCalc::paceMin); u8g2->print(":"); if(GPSCalc::paceSec<10) u8g2->print("0"); u8g2->print(GPSCalc::paceSec); u8g2->print(" /km"); }
    else u8g2->print("--:-- /km");

    u8g2->setCursor(ox + 0, 48); 
    u8g2->print("time: "); u8g2->print(GPSCalc::durationSec / 60); u8g2->print(":"); 
    if(GPSCalc::durationSec % 60 < 10) u8g2->print("0"); u8g2->print(GPSCalc::durationSec % 60);

    u8g2->setCursor(ox + 0, 60); 
    u8g2->print("turns: "); u8g2->print(GPSCalc::laps);
}

void MenuManager::drawSport2(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_6x12_tf);
    
    float avgPace = 0;
    if (GPSCalc::totalDistance > 10) avgPace = 60.0 / ((GPSCalc::totalDistance / GPSCalc::durationSec) * 3.6);
    
    u8g2->setCursor(ox + 0, 15); u8g2->print("Avg Pace: "); u8g2->print((int)avgPace); u8g2->print("'"); u8g2->print((int)((avgPace - (int)avgPace)*60)); u8g2->print("\"");
    u8g2->setCursor(ox + 0, 30); u8g2->print("Altitude: "); u8g2->print(GPSCalc::altitude); u8g2->print(" m");
    u8g2->setCursor(ox + 0, 45); u8g2->print("Sats Lock: "); u8g2->print(GPSCalc::satellites);
    u8g2->setCursor(ox + 0, 60); u8g2->print("Course: "); u8g2->print(GPSCalc::course); u8g2->print(" deg");
}

void MenuManager::drawSummary(int ox) {
    auto u8g2 = HAL::getDisplay();
    u8g2->setFont(u8g2_font_6x10_tf);
    u8g2->drawStr(ox + 2, 10, "Summary");

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
        u8g2->print("L"); u8g2->print(i + 1); u8g2->print(": ");
        u8g2->print((int)GPSCalc::lapHistory[i].pace); u8g2->print("'");
        u8g2->print((int)((GPSCalc::lapHistory[i].pace - (int)GPSCalc::lapHistory[i].pace) * 60)); u8g2->print("\"");
        
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
    for(int i=0; i<GPSCalc::trackPointsCount; i++) {
        if(GPSCalc::trackX[i] < minX) minX = GPSCalc::trackX[i]; if(GPSCalc::trackX[i] > maxX) maxX = GPSCalc::trackX[i];
        if(GPSCalc::trackY[i] < minY) minY = GPSCalc::trackY[i]; if(GPSCalc::trackY[i] > maxY) maxY = GPSCalc::trackY[i];
    }
    float maxDim = max(maxX - minX, maxY - minY);
    float scale = 44.0f / (maxDim > 0.1 ? maxDim : 1.0f); 

    for(int i = start + 1; i <= end; i++) {
        int px1 = ox + (GPSCalc::trackX[i-1] - minX) * scale; int py1 = oy + 44 - (GPSCalc::trackY[i-1] - minY) * scale; 
        int px2 = ox + (GPSCalc::trackX[i] - minX) * scale;   int py2 = oy + 44 - (GPSCalc::trackY[i] - minY) * scale;
        u8g2->drawLine(px1, py1, px2, py2);
    }
    u8g2->drawFrame(ox-2, oy-2, 48, 48); 
}