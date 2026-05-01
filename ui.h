#ifndef UI_H
#define UI_H

#include <stdint.h>

enum PageState { PAGE_SPLASH,
                 PAGE_START,
                 PAGE_SETTINGS,
                 PAGE_DEV_MENU,
                 PAGE_DEV,
                 PAGE_SAT_TXT,
                 PAGE_SAT_GUI,
                 PAGE_DEV_STAT,
                 PAGE_USB_BRIDGE,
                 PAGE_SPORT1,
                 PAGE_SPORT2,
                 PAGE_SPORT3,
                 PAGE_SUMMARY };

class MenuManager {
public:
  static void handleInput();
  static void update();

  // Public state — accessed by page renderers
  static PageState currentPage;
  static int cursorIndex;
  static bool isCursorVisible;
  static int setIdx;
  static int setScroll;
  static bool isEditing;
  static int viewLapIdx;
  static int devMenuIdx;
  static int satTxtScroll;
  static int devScroll;
  static int usbBridgeBaudIdx;
  static unsigned long usbBridgeBytesRx;
  static unsigned long usbBridgeBytesTx;

  // Animation state — updated each frame by update()
  static float currentCursorY;
  static float currentPageX;
  static float visualSetCursorY;
  static float visualSetScrollY;
  static float visualDevCursorY;
  static float visualDevScrollY;
  static float visualSatTxtScrollY;

private:
  static uint32_t lastActiveTime;
  static bool isScreenOff;
  static float smoothLerp(float current, float target, float speed = 0.25f);
};

#endif
