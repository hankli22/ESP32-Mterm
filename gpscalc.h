#ifndef GPSCALC_H
#define GPSCALC_H

#include <Arduino.h>
#include <TinyGPS++.h>

#define MAX_LAPS 10
#define MAX_TRACK_POINTS 180
#define PACE_WINDOW_SIZE 10 // 10秒滑动窗口

struct LapInfo {
    int timeSec;
    float pace; // 本圈平均配速 (min/km)
    int trackStartIdx;
    int trackEndIdx;
};

class GPSCalc {
public:
    static void init();
    static void process();
    static void startRun();
    static void stopRun();
    static String getDateTime();

    static bool isRunning;
    static float totalDistance; 
    static float currentSpeed;  
    static float slidingPace;   // 10s滑动窗口配速
    static int paceMin;         
    static int paceSec;
    static int laps;
    static int durationSec;     
    static int satellites;
    
    // 定位调试信息
    static double altitude;
    static double course;
    static double homeLat, homeLng, lastLat, lastLng;
    
    static LapInfo lapHistory[MAX_LAPS];
    static float trackX[MAX_TRACK_POINTS];
    static float trackY[MAX_TRACK_POINTS];
    static int trackPointsCount;

private:
    static float paceBuffer[PACE_WINDOW_SIZE];
    static int paceBufIdx;
    static uint32_t lastPaceUpdate;
    static float lastDistForPace;
    static uint32_t runStartTime;
    static uint32_t lastLapTime;
    static uint32_t lastTrackSaveTime;
};
#endif