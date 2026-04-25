#ifndef GPSMODULE_H
#define GPSMODULE_H

#include <Arduino.h>
#include <TinyGPS++.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#define MAX_LAPS 10
#define MAX_TRACK_POINTS 180
#define PACE_WINDOW_SIZE 10

struct SatData {
  uint8_t sys;  // 0:GPS, 1:BDS, 2:GLO, 3:SBS
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

  static int satellites;
  static int satsInView;
  static int accuracyPct;
  static double altitude;
  static double course;
  static bool homeSet;
  static double homeLat, homeLng, lastLat, lastLng;

  static LapInfo lapHistory[MAX_LAPS];
  static float trackX[MAX_TRACK_POINTS];
  static float trackY[MAX_TRACK_POINTS];
  static int trackPointsCount;

  static SatData sats[40];
  static int satCount;
  static int sysTracked[4];
  static int sysInView[4];

  static bool isGpsReady();

  static void lock();
  static void unlock();

  static double rawLat, rawLng;
  static float maxSpeed;
  static int calories;
private:
  static SemaphoreHandle_t mutex;
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