#ifndef MENU_H
#define MENU_H

// 加入 PAGE_SPLASH 启动页面
enum PageState { PAGE_SPLASH, PAGE_START, PAGE_SETTINGS, PAGE_DEV, PAGE_SPORT1, PAGE_SPORT2, PAGE_SUMMARY };

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
    
    // 全局横向与纵向平滑动画变量
    static float currentPageX;
    static float currentCursorY;     // Start菜单纵向光标
    static float visualSetCursorY;   // Settings菜单纵向光标
    static float visualSetScrollY;   // Settings菜单全局纵向滚动
    
    static void drawSplash(int ox);
    static void drawStartMenu(int ox);
    static void drawSettings(int ox);
    static void drawDevPage(int ox);
    static void drawSport1(int ox);
    static void drawSport2(int ox);
    static void drawSummary(int ox);
    static void drawTrackMap(int ox, int oy, int lapIdx);
    
    static float smoothLerp(float current, float target, float speed = 0.25f);
};

#endif