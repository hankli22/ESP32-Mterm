#include "config.h"
#include <Preferences.h>

SystemConfig sysCfg;
Preferences prefs;

void loadConfig() {
    prefs.begin("runner", true);
    sysCfg.record_freq = prefs.getFloat("freq", 5.0);
    sysCfg.draw_track = prefs.getBool("track", true);
    sysCfg.screen_off = prefs.getInt("soff", 30);
    sysCfg.storage_track = prefs.getInt("stor", 0);
    prefs.end();
}

void saveConfig() {
    prefs.begin("runner", false);
    prefs.putFloat("freq", sysCfg.record_freq);
    prefs.putBool("track", sysCfg.draw_track);
    prefs.putInt("soff", sysCfg.screen_off);
    prefs.putInt("stor", sysCfg.storage_track);
    prefs.end();
}