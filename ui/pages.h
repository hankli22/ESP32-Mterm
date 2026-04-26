#ifndef PAGES_H
#define PAGES_H

#include "canvas.h"

void drawSplash(Canvas& cv);
void drawStartMenu(Canvas& cv);
void drawSettings(Canvas& cv);
void drawDevMenu(Canvas& cv);
void drawDevPage(Canvas& cv);
void drawDevStat(Canvas& cv);
void drawSatTxt(Canvas& cv);
void drawSatGui(Canvas& cv);
void drawSport1(Canvas& cv);
void drawSport2(Canvas& cv);
void drawSport3(Canvas& cv);
void drawSummary(Canvas& cv);
void drawTrackMap(Canvas& cv, int ox, int oy, int lapIdx);

#endif
