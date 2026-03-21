#ifndef GPSCALC_H
#define GPSCALC_H

#include <Arduino.h>
#include <TinyGPS++.h>

#define MAX_LAPS 10
#define MAX_TRACK_POINTS 180
#define PACE_WINDOW_SIZE 10

struct SatData {
    uint8_t sys; // 0:GPS, 1:BDS, 2:GLO, 3:SBS
    uint8_t prn;
    uint8_t snr;
    uint8_t ele;
    uint16_t azi;
    uint32_t lastSeen;
};

struct LapInfo {
    int timeSec;
    float pace;
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
    static float slidingPace;   
    static int paceMin;         
    static int paceSec;
    static int laps;
    static int durationSec;     
    
    // 定位调试与质量信息
    static int satellites;
    static int satsInView;   // 捕捉到的卫星数
    static int accuracyPct;  // 精准度百分比
    static double altitude;
    static double course;
    static double homeLat, homeLng, lastLat, lastLng;
    
    static LapInfo lapHistory[MAX_LAPS];
    static float trackX[MAX_TRACK_POINTS];
    static float trackY[MAX_TRACK_POINTS];
    static int trackPointsCount;

// --- 新增卫星详情数组 ---
    static SatData sats[40];
    static int satCount;
    static int sysTracked[4];
    static int sysInView[4];

private:
    static void parseGSV(const char* nmea);
    static void cleanupSats();
    static float paceBuffer[PACE_WINDOW_SIZE];
    static int paceBufIdx;
    static uint32_t lastPaceUpdate;
    static float lastDistForPace;
    static uint32_t runStartTime;
    static uint32_t lastLapTime;
    static uint32_t lastTrackSaveTime;
};
#endif