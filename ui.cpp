#include "ui.h"
#include "hardwareLayer.h"
#include "gps_module.h"
#include "config.h"
#include "canvas.h"
#include "src/pages.h"

PageState MenuManager::currentPage = PAGE_SPLASH;
int MenuManager::cursorIndex = 0;
bool MenuManager::isCursorVisible = true;
int MenuManager::setIdx = 0;
int MenuManager::setScroll = 0;
bool MenuManager::isEditing = false;
int MenuManager::viewLapIdx = 0;
int MenuManager::devMenuIdx = 0;
int MenuManager::satTxtScroll = 0;
int MenuManager::devScroll = 0;
int MenuManager::usbBridgeBaudIdx = 0;
unsigned long MenuManager::usbBridgeBytesRx = 0;
unsigned long MenuManager::usbBridgeBytesTx = 0;

uint32_t MenuManager::lastActiveTime = 0;
bool MenuManager::isScreenOff = false;

float MenuManager::currentPageX = 0;
float MenuManager::currentCursorY = 45;
float MenuManager::visualSetCursorY = 0;
float MenuManager::visualSetScrollY = 0;
float MenuManager::visualDevCursorY = 0;
float MenuManager::visualDevScrollY = 0;
float MenuManager::visualSatTxtScrollY = 0;

SystemConfig tempCfg;

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
        else if (devMenuIdx == 4) {
          currentPage = PAGE_USB_BRIDGE;
          usbBridgeBaudIdx = 0;
          usbBridgeBytesRx = 0;
          usbBridgeBytesTx = 0;
          Serial1.updateBaudRate(9600);
        }
      }
      if (devMenuIdx < devScroll) devScroll = devMenuIdx;
      if (devMenuIdx > devScroll + 2) devScroll = devMenuIdx - 2;
      break;

    case PAGE_DEV_STAT:
    case PAGE_DEV:
    case PAGE_USB_BRIDGE:
      if (evt == BTN_LEFT_PRESSED) {
        if (currentPage == PAGE_USB_BRIDGE) {
          Serial1.updateBaudRate(9600);
        }
        currentPage = PAGE_DEV_MENU;
      }
      if (currentPage == PAGE_USB_BRIDGE) {
        if (evt == BTN_UP_PRESSED) {
          usbBridgeBaudIdx++;
          if (usbBridgeBaudIdx > 4) usbBridgeBaudIdx = 0;
          const int bauds[] = { 9600, 19200, 38400, 57600, 115200 };
          Serial1.updateBaudRate(bauds[usbBridgeBaudIdx]);
        }
        if (evt == BTN_DOWN_PRESSED) {
          usbBridgeBaudIdx--;
          if (usbBridgeBaudIdx < 0) usbBridgeBaudIdx = 4;
          const int bauds[] = { 9600, 19200, 38400, 57600, 115200 };
          Serial1.updateBaudRate(bauds[usbBridgeBaudIdx]);
        }
      }
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

  // Update sliding animations
  currentCursorY = smoothLerp(currentCursorY, 45 + cursorIndex * 15, 0.3f);
  visualSetScrollY = smoothLerp(visualSetScrollY, setScroll * 15, 0.2f);
  visualSetCursorY = smoothLerp(visualSetCursorY, setIdx * 15, 0.3f);
  visualDevScrollY = smoothLerp(visualDevScrollY, devScroll * 15, 0.2f);
  visualDevCursorY = smoothLerp(visualDevCursorY, devMenuIdx * 15, 0.3f);
  visualSatTxtScrollY = smoothLerp(visualSatTxtScrollY, satTxtScroll, 0.2f);

  // Page slide animation
  float targetX = 0;
  if (currentPage == PAGE_SPLASH) targetX = 0;
  else if (currentPage == PAGE_START) targetX = -128;
  else if (currentPage == PAGE_SETTINGS) targetX = -256;
  else if (currentPage == PAGE_DEV_MENU) targetX = -384;
  else if (currentPage == PAGE_DEV) targetX = -512;
  else if (currentPage == PAGE_SAT_TXT) targetX = -640;
  else if (currentPage == PAGE_SAT_GUI) targetX = -768;
  else if (currentPage == PAGE_DEV_STAT) targetX = -896;
  else if (currentPage == PAGE_USB_BRIDGE) targetX = -1024;
  else if (currentPage == PAGE_SPORT1) targetX = -1152;
  else if (currentPage == PAGE_SPORT2) targetX = -1280;
  else if (currentPage == PAGE_SPORT3) targetX = -1408;
  else if (currentPage == PAGE_SUMMARY) targetX = -1536;

  currentPageX = smoothLerp(currentPageX, targetX, 0.2f);
  int ox = (int)currentPageX;

  // Render visible pages to canvases, blit to U8g2
  Canvas cv(u8g2);

  static const int pageOffsets[] = { 0, 128, 256, 384, 512, 640, 768, 896, 1024, 1152, 1280, 1408, 1536 };
  typedef void (*PageDrawFn)(Canvas&);
  static const PageDrawFn drawers[] = {
    drawSplash, drawStartMenu, drawSettings, drawDevMenu,
    drawDevPage, drawSatTxt, drawSatGui, drawDevStat,
    drawUsbBridge, drawSport1, drawSport2, drawSport3, drawSummary
  };

  for (int i = 0; i < 13; i++) {
    int pox = ox + pageOffsets[i];
    if (pox > -128 && pox < 128) {
      cv.clear();
      drawers[i](cv);
      cv.blitTo(u8g2, pox, 0);
    }
  }

  // USB Bridge: pump data between USB CDC (Serial) and hardware UART (Serial1)
  if (currentPage == PAGE_USB_BRIDGE) {
    int n = 256;
    while (Serial1.available() && --n > 0) {
      Serial.write(Serial1.read());
      usbBridgeBytesRx++;
    }
    n = 256;
    while (Serial.available() && --n > 0) {
      Serial1.write(Serial.read());
      usbBridgeBytesTx++;
    }
  }

  GPSCalc::unlock();
  u8g2->sendBuffer();
}
