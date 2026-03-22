#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

struct SystemConfig {
  float record_freq = 5.0;
  bool draw_track = true;
  int screen_off = 30;
  int pwr_off_btn = 0;
  int storage_track = 0;
  bool en_multycol = true;
  int contrast = 5;  // 【新增】亮度/对比度 (1~5档)
  bool auto_sleep = false; // 【新增】跑完步退出时是否自动关机
};

extern SystemConfig sysCfg;

void loadConfig();
void saveConfig();

#endif