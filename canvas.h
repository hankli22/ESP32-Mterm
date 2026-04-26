#ifndef CANVAS_H
#define CANVAS_H

#include <U8g2lib.h>
#include <string.h>
#include <stdio.h>

class Canvas {
  uint8_t buf[1024];   // 128x64 / 8
  U8G2* u8g2;
  int cursorX, cursorY;

public:
  Canvas(U8G2* u) : u8g2(u), cursorX(0), cursorY(0) { clear(); }

  void clear() { memset(buf, 0, 1024); }
  uint8_t* getBufferPtr() { return buf; }

  void setDrawColor(uint8_t c) { u8g2->setDrawColor(c); }
  void setFont(const uint8_t* f) { u8g2->setFont(f); u8g2->enableUTF8Print(); }
  void setCursor(int x, int y) { cursorX = x; cursorY = y; u8g2->setCursor(x, y); }

  // --- Pixel ops ---
  void setPixel(int x, int y) {
    if ((unsigned)x >= 128 || (unsigned)y >= 64) return;
    buf[(y >> 3) * 128 + x] |= (1 << (y & 7));
  }
  void clearPixel(int x, int y) {
    if ((unsigned)x >= 128 || (unsigned)y >= 64) return;
    buf[(y >> 3) * 128 + x] &= ~(1 << (y & 7));
  }
  void drawPixel(int x, int y) {
    if (u8g2->getDrawColor() == 1) setPixel(x, y);
    else clearPixel(x, y);
  }

  // --- Primitives ---
  void drawLine(int x0, int y0, int x1, int y1) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
      drawPixel(x0, y0);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  void drawBox(int x, int y, int w, int h) {
    for (int py = y; py < y + h; py++)
      for (int px = x; px < x + w; px++)
        drawPixel(px, py);
  }

  void drawFrame(int x, int y, int w, int h) {
    drawLine(x, y, x + w - 1, y);
    drawLine(x + w - 1, y, x + w - 1, y + h - 1);
    drawLine(x + w - 1, y + h - 1, x, y + h - 1);
    drawLine(x, y + h - 1, x, y);
  }

  void drawCircle(int cx, int cy, int r) {
    int x = 0, y = r, d = 3 - 2 * r;
    while (y >= x) {
      drawPixel(cx + x, cy - y);
      drawPixel(cx + y, cy - x);
      drawPixel(cx + y, cy + x);
      drawPixel(cx + x, cy + y);
      drawPixel(cx - x, cy + y);
      drawPixel(cx - y, cy + x);
      drawPixel(cx - y, cy - x);
      drawPixel(cx - x, cy - y);
      if (d > 0) { y--; d += 4 * (x - y) + 10; }
      else d += 4 * x + 6;
      x++;
    }
  }

  // --- Text (delegated to U8g2 with buffer swap) ---
  int drawStr(int x, int y, const char* s) {
    u8g2_t* u = u8g2->getU8g2();
    uint8_t* save = u->tile_buf_ptr;
    u->tile_buf_ptr = buf;
    int w = u8g2->drawUTF8(x, y, s);
    u->tile_buf_ptr = save;
    return w;
  }

  void print(const char* s)       { cursorX += drawStr(cursorX, cursorY, s); }
  void print(int n)               { char b[12]; snprintf(b, sizeof(b), "%d", n); print(b); }
  void print(unsigned int n)      { char b[12]; snprintf(b, sizeof(b), "%u", n); print(b); }
  void print(long n)              { char b[12]; snprintf(b, sizeof(b), "%ld", n); print(b); }
  void print(unsigned long n)     { char b[12]; snprintf(b, sizeof(b), "%lu", n); print(b); }
  void print(float f)             { print(f, 2); }
  void print(double f)            { print((float)f, 2); }
  void print(float f, int d)      { char b[16]; dtostrf(f, 1, d, b); print(b); }
  void print(double f, int d)     { char b[16]; dtostrf((float)f, 1, d, b); print(b); }

  // UTF-8 versions (redirect to drawStr)
  int drawUTF8(int x, int y, const char* s) { return drawStr(x, y, s); }

  // --- Copy to U8g2 framebuffer ---
  void blitTo(U8G2* dst, int dx, int dy) {
    uint8_t* dstBuf = dst->getBufferPtr();
    for (int y = 0; y < 64; y++) {
      int dy2 = y + dy;
      if ((unsigned)dy2 >= 64) continue;
      uint8_t dstBit = 1 << (dy2 & 7);
      int dstRow = (dy2 >> 3) * 128;
      int srcRow = (y >> 3) * 128;
      uint8_t srcBit = 1 << (y & 7);
      for (int x = 0; x < 128; x++) {
        int dx2 = x + dx;
        if ((unsigned)dx2 >= 128) continue;
        if (buf[srcRow + x] & srcBit)
          dstBuf[dstRow + dx2] |= dstBit;
      }
    }
  }

  // Full copy (when no offset)
  void blitTo(U8G2* dst) {
    memcpy(dst->getBufferPtr(), buf, 1024);
  }
};

#endif
