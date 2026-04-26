#include "pages.h"

void drawSplash(Canvas& cv) {
  cv.setFont(u8g2_font_wqy12_t_gb2312);
  cv.setCursor(5, 12);
  cv.print("欢迎使用跑表");
  cv.setCursor(5, 26);
  cv.print("卫星闪烁后起跑");
  cv.setCursor(5, 40);
  cv.print("左右确认取消，上下值");
  cv.setCursor(5, 54);
  cv.print("");
  cv.setFont(u8g2_font_4x6_tf);
  cv.drawStr(70, 63, "Press RIGHT ->");
}
