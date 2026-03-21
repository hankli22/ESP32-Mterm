#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

struct SystemConfig {
    float record_freq = 5.0; // 5.0, 2.0, 1.0, 0.5
    bool draw_track = true;  // true(yes), false(no)
    int screen_off = 30;     // 30, 60(1m), 300(5m), 0(never)
    int pwr_off_btn = 0;     // 0: hold_3s, 1: w,a,s,d (UI展示用)
    int storage_track = 0;   // 0: disable, 1, 2, 4
};

extern SystemConfig sysCfg;

void loadConfig();
void saveConfig();

#endif