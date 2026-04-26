#include "pages.h"
#include "ui.h"
#include "gps_module.h"
#include "config.h"
#include "trig_lut.h"
#include <stdio.h>

void drawSatTxt(Canvas& cv) {
  cv.setFont(u8g2_font_5x8_tf);

  float scrollY = MenuManager::visualSatTxtScrollY;
  float y = 20 - scrollY;
  const char* sName[] = { "GPS", "BDS", "GLO", "SBS" };

  for (int s = 0; s < 4; s++) {
    if (GPSCalc::sysInView[s] == 0) continue;
    if (y > 10 && y < 70) {
      float dist = abs(y - 36);
      float oxOffset = (dist < 25) ? 4.0f * (1.0f - dist / 25.0f) : 0;
      cv.setCursor(2 + (int)oxOffset, (int)y);
      cv.print("[");
      cv.print(sName[s]);
      cv.print("*");
      cv.print(GPSCalc::sysTracked[s]);
      cv.print("/");
      cv.print(GPSCalc::sysInView[s]);
      cv.print("]");
    }
    y += 10;
    for (int i = 0; i < GPSCalc::satCount; i++) {
      if (GPSCalc::sats[i].sys == s) {
        if (y > 10 && y < 70) {
          float dist = abs(y - 36);
          float oxOffset = (dist < 25) ? 4.0f * (1.0f - dist / 25.0f) : 0;
          char buf[32];
          sprintf(buf, " %02d   %02d   %02d   %03d", GPSCalc::sats[i].prn, GPSCalc::sats[i].snr, GPSCalc::sats[i].ele, GPSCalc::sats[i].azi);
          cv.drawStr(2 + (int)oxOffset, (int)y, buf);
        }
        y += 10;
      }
    }
  }

  int maxScroll = (GPSCalc::satCount * 10) - 20;
  if (maxScroll > 0) {
    int barY = 15 + (int)((scrollY / maxScroll) * 40);
    cv.drawBox(125, barY, 2, 8);
  }

  cv.setDrawColor(0);
  cv.drawBox(0, 0, 126, 12);
  cv.setDrawColor(1);
  cv.drawStr(2, 9, " PRN SNR ELE AZI");
  cv.drawLine(0, 11, 125, 11);
}

void drawSatGui(Canvas& cv) {
  cv.setFont(u8g2_font_4x6_tf);
  const char* sName[] = { "GPS", "BDS", "GLO", "SBS" };
  const uint8_t drawOrder[] = { 1, 0, 2, 3 };

  static const uint8_t bayer[4][4] = {
    { 0,  8,  2, 10},
    {12,  4, 14,  6},
    { 3, 11,  1,  9},
    {15,  7, 13,  5}
  };

  int cx = 30, cy = 35, r = 22;
  int total = 0;
  for (int i = 0; i < 4; i++) total += GPSCalc::sysTracked[i];
  cv.drawCircle(cx, cy, r);

  if (total > 0) {
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

    uint8_t *buf = cv.getBufferPtr();
    int rSq = r * r;
    int py0 = max(cy - r, 0);
    int py1 = min(cy + r, 63);
    int px0 = max(cx - r, 0);
    int px1 = min(cx + r, 127);
    for (int py = py0; py <= py1; py++) {
      uint8_t page = py >> 3;
      uint8_t bit  = 1 << (py & 7);
      int rowOff = page * 128;
      int dy = py - cy;
      for (int px = px0; px <= px1; px++) {
        int dx = px - cx;
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

    for (int i = 0; i < secN; i++) {
      int ad = (int)(startAng[i] * 57.29578f + 0.5f);
      ad = (ad % 360 + 360) % 360;
      float sa_cos = cos_lut[ad], sa_sin = sin_lut[ad];
      cv.drawLine(cx, cy, cx + r * sa_cos, cy + r * sa_sin);

      int md = (int)((startAng[i] + endAng[i]) * 0.5f * 57.29578f + 0.5f);
      md = (md % 360 + 360) % 360;
      float mid_cos = cos_lut[md], mid_sin = sin_lut[md];
      int lx = cx + (r + 8) * mid_cos - 6;
      int ly = cy + (r + 8) * mid_sin + 2;
      cv.setCursor(lx, ly);
      cv.print(sName[secSys[i]]);
    }
  } else {
    cv.drawStr(cx - 10, cy + 2, "NO SAT");
  }

  cv.drawStr(65, 8, "TOP 5 SATS:");
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
    cv.setCursor(65, 18 + i * 9);
    cv.print("[");
    cv.print(sName[GPSCalc::sats[idx].sys]);
    cv.print("] ");
    cv.print(GPSCalc::sats[idx].prn);
    cv.print(" S:");
    cv.print(GPSCalc::sats[idx].snr);
  }
}
