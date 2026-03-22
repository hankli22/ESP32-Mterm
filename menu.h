#ifndef MENU_H
#define MENU_H

#include <stdint.h>

enum PageState { PAGE_SPLASH,
                 PAGE_START,
                 PAGE_SETTINGS,
                 PAGE_DEV_MENU,
                 PAGE_DEV,
                 PAGE_SAT_TXT,
                 PAGE_SAT_GUI,
                 PAGE_DEV_STAT,
                 PAGE_SPORT1,
                 PAGE_SPORT2,
                 PAGE_SPORT3,
                 PAGE_SUMMARY };

class MenuManager {
public:
  static void handleInput();
  static void update();
private:
  static PageState currentPage;
  static int cursorIndex;
  static bool isCursorVisible;
  static int setIdx;
  static int setScroll;
  static bool isEditing;
  static int viewLapIdx;

  static uint32_t lastActiveTime;
  static bool isScreenOff;

  static float currentCursorY;
  static float currentPageX;
  static float visualSetCursorY;
  static float visualSetScrollY;

  static void drawSplash(int ox);
  static void drawStartMenu(int ox);
  static void drawSettings(int ox);
  static void drawDevPage(int ox);
  static void drawSport1(int ox);
  static void drawSport2(int ox);
  static void drawSummary(int ox);
  static void drawTrackMap(int ox, int oy, int lapIdx);
  static void drawDevMenu(int ox);
  static void drawSatTxt(int ox);
  static void drawSatGui(int ox);

  static float smoothLerp(float current, float target, float speed = 0.25f);

  static int devMenuIdx;
  static int satTxtScroll;

  static int devScroll;
  static float visualDevCursorY;
  static float visualDevScrollY;
  static void drawDevStat(int ox);
  static float visualSatTxtScrollY;  // 【修复编译报错：补上声明】
  static void drawSport3(int ox);    // 【新增】实时轨迹页面
};

#endif