#include "gps_module.h"
#include "config.h"

bool GPSCalc::isRunning = false;
float GPSCalc::totalDistance = 0;
float GPSCalc::currentSpeed = 0;
float GPSCalc::slidingPace = 0;
int GPSCalc::paceMin = 0;
int GPSCalc::paceSec = 0;
int GPSCalc::laps = 0;
int GPSCalc::durationSec = 0;

int GPSCalc::satellites = 0;
int GPSCalc::satsInView = 0;
int GPSCalc::accuracyPct = 0;

double GPSCalc::altitude = 0;
double GPSCalc::course = 0;
bool GPSCalc::homeSet = false;
double GPSCalc::homeLat = 0, GPSCalc::homeLng = 0;
double GPSCalc::lastLat = 0, GPSCalc::lastLng = 0;

LapInfo GPSCalc::lapHistory[MAX_LAPS];
float GPSCalc::trackX[MAX_TRACK_POINTS];
float GPSCalc::trackY[MAX_TRACK_POINTS];
int GPSCalc::trackPointsCount = 0;

float GPSCalc::paceBuffer[PACE_WINDOW_SIZE] = { 0 };
int GPSCalc::paceBufIdx = 0;
uint32_t GPSCalc::lastPaceUpdate = 0;
float GPSCalc::lastDistForPace = 0;
uint32_t GPSCalc::runStartTime = 0;
uint32_t GPSCalc::lastLapTime = 0;
uint32_t GPSCalc::lastTrackSaveTime = 0;

SatData GPSCalc::sats[40];
int GPSCalc::satCount = 0;
int GPSCalc::sysTracked[4] = { 0 };
int GPSCalc::sysInView[4] = { 0 };

double GPSCalc::rawLat = 0;
double GPSCalc::rawLng = 0;
float GPSCalc::maxSpeed = 0;
int GPSCalc::calories = 0;

static TinyGPSPlus gps;
static TinyGPSCustom satsInViewCustom(gps, "GPGSV", 3);

SemaphoreHandle_t GPSCalc::mutex = nullptr;

void GPSCalc::lock() {
  if (mutex) xSemaphoreTake(mutex, portMAX_DELAY);
}

void GPSCalc::unlock() {
  if (mutex) xSemaphoreGive(mutex);
}

void GPSCalc::init() {
  mutex = xSemaphoreCreateMutex();
  Serial1.begin(9600, SERIAL_8N1, 17, 18);
  if (sysCfg.record_freq >= 5.0) Serial1.println("$PCAS02,200*1D");
  else if (sysCfg.record_freq >= 2.0) Serial1.println("$PCAS02,500*1A");
  else Serial1.println("$PCAS02,1000*2E");
}

void GPSCalc::startRun() {
  isRunning = true;
  totalDistance = 0;
  laps = 0;
  trackPointsCount = 0;
  homeSet = false;
  homeLat = 0;
  durationSec = 0;
  maxSpeed = 0;
  calories = 0;
  runStartTime = millis();
  lastLapTime = millis();
  for (int i = 0; i < PACE_WINDOW_SIZE; i++) paceBuffer[i] = 0;
  lastDistForPace = 0;
  lastPaceUpdate = millis();
  lapHistory[0].trackStartIdx = 0;
}

void GPSCalc::stopRun() {
  isRunning = false;
  if (laps < MAX_LAPS) lapHistory[laps].trackEndIdx = trackPointsCount - 1;
}

static float calcDist(double lat1, double lng1, double lat2, double lng2) {
  return (float)TinyGPSPlus::distanceBetween(lat1, lng1, lat2, lng2);
}

String GPSCalc::getDateTime() {
  if (!gps.time.isValid() || gps.date.year() < 2020) {
    if (gps.time.isValid()) {
      char tBuf[32];
      int h = (gps.time.hour() + 8) % 24;
      sprintf(tBuf, "TIME: %02d:%02d:%02d", h, (int)gps.time.minute(), (int)gps.time.second());
      return String(tBuf);
    }
    return "WAITING SATELLITES";
  }
  char timeBuf[32];
  int localHour = gps.time.hour() + 8;
  int localDay = gps.date.day();
  if (localHour >= 24) {
    localHour -= 24;
    localDay += 1;
  }
  sprintf(timeBuf, "%02d/%02d/%02d %02d:%02d:%02d",
          (int)(gps.date.year() % 100), (int)gps.date.month(), (int)localDay,
          (int)localHour, (int)gps.time.minute(), (int)gps.time.second());
  return String(timeBuf);
}


void GPSCalc::process() {
  lock();
  static char nmeaBuf[85];
  static int nmeaPos = 0;

  while (Serial1.available() > 0) {
    char c = Serial1.read();
    gps.encode(c);

    if (c == '$') {
      nmeaPos = 0;
      nmeaBuf[nmeaPos++] = c;
    } else if (c == '\n' || c == '\r') {
      nmeaBuf[nmeaPos] = 0;
      if (nmeaPos > 10) parseGSV(nmeaBuf);
      nmeaPos = 0;
    } else if (nmeaPos < 84) {
      nmeaBuf[nmeaPos++] = c;
    }
  }

  cleanupSats();

  satellites = gps.satellites.value();
  if (satsInViewCustom.isUpdated() && satsInViewCustom.isValid()) {
    satsInView = atoi(satsInViewCustom.value());
  }

  double h = gps.hdop.hdop();
  if (h > 0) {
    int acc = 100 - (int)((h - 1.0) * 15);
    accuracyPct = constrain(acc, 0, 100);
  } else accuracyPct = 0;

  if (gps.location.isValid()) {
    rawLat = gps.location.lat();
    rawLng = gps.location.lng();
    altitude = gps.altitude.meters();
    course = gps.course.deg();
    currentSpeed = gps.speed.kmph();
  }

  if (!isRunning || !gps.location.isValid() || gps.location.age() > 2000) { unlock(); return; }

  uint32_t now = millis();
  durationSec = (now - runStartTime) / 1000;
  double lat = gps.location.lat();
  double lng = gps.location.lng();

  if (now - lastPaceUpdate >= 1000) {
    float distThisSec = totalDistance - lastDistForPace;
    lastDistForPace = totalDistance;

    paceBuffer[paceBufIdx] = distThisSec;
    paceBufIdx = (paceBufIdx + 1) % PACE_WINDOW_SIZE;
    lastPaceUpdate = now;

    float distInWindow = 0;
    for (int i = 0; i < PACE_WINDOW_SIZE; i++) distInWindow += paceBuffer[i];

    if (distInWindow > 0.5f) {
      float speedMps = distInWindow / (float)PACE_WINDOW_SIZE;
      slidingPace = 16.6667f / speedMps;
      paceMin = (int)slidingPace;
      paceSec = (int)((slidingPace - paceMin) * 60);
    } else {
      slidingPace = 0;
      paceMin = 0;
      paceSec = 0;
    }

    if (currentSpeed > maxSpeed) maxSpeed = currentSpeed;
    calories = (int)((totalDistance / 1000.0f) * 60.0f * 1.036f);
  }

  if (!homeSet) {
    if (satellites > 4 && gps.hdop.hdop() < 2.5) {
      homeSet = true;
      homeLat = lat;
      homeLng = lng;
      lastLat = lat;
      lastLng = lng;
      lastLapTime = now;
      if (laps < MAX_LAPS) lapHistory[laps].trackStartIdx = trackPointsCount;
    }
    unlock(); return;
  }

  float d = calcDist(lat, lng, lastLat, lastLng);
  if (d >= 2.0f && d < 35.0f) {
    totalDistance += d;
    lastLat = lat;
    lastLng = lng;

    if (now - lastTrackSaveTime > 2000 && trackPointsCount < MAX_TRACK_POINTS) {
      trackX[trackPointsCount] = (float)((lng - homeLng) * 111320.0 * cos(homeLat * 0.017453));
      trackY[trackPointsCount] = (float)((lat - homeLat) * 111320.0);
      trackPointsCount++;
      lastTrackSaveTime = now;
    }
  }

  float distToHome = calcDist(lat, lng, homeLat, homeLng);
  //回到起点 15 米内，且本圈已经跑了至少 200 米
  if (distToHome < 15.0f && totalDistance > (laps * 250.0f + 200.0f)) {
    if (laps < MAX_LAPS) {
      lapHistory[laps].timeSec = (now - lastLapTime) / 1000;
      if (lapHistory[laps].timeSec > 0) {
        lapHistory[laps].pace = (lapHistory[laps].timeSec / 0.25f) / 60.0f;
      }
      lapHistory[laps].trackEndIdx = (trackPointsCount > 0) ? trackPointsCount - 1 : 0;
      laps++;
      if (laps < MAX_LAPS) {
        lapHistory[laps].trackStartIdx = trackPointsCount;
      }
    }
    lastLapTime = now;
  }
  unlock();
}

void GPSCalc::parseGSV(const char* nmea) {
  if (strncmp(nmea + 3, "GSV", 3) != 0) return;

  uint8_t sys = 3;                                     // 默认 SBS/其他
  if (nmea[1] == 'G' && nmea[2] == 'P') sys = 0;       // GPS
  else if (nmea[1] == 'B' && nmea[2] == 'D') sys = 1;  // 北斗 BDS
  else if (nmea[1] == 'G' && nmea[2] == 'B') sys = 1;  // 北斗兼容名
  else if (nmea[1] == 'G' && nmea[2] == 'L') sys = 2;  // GLONASS

  int toks[20];
  int tCnt = 0;
  toks[tCnt++] = 0;
  for (int i = 0; nmea[i]; i++) {
    if (nmea[i] == ',') toks[tCnt++] = i + 1;
    else if (nmea[i] == '*') {
      toks[tCnt++] = i + 1;
      break;
    }
  }
  if (tCnt < 4) return;
  sysInView[sys] = atoi(nmea + toks[3]);

  // 解析每颗卫星 PRN, ELE, AZI, SNR
  for (int i = 4; i + 3 < tCnt; i += 4) {
    int prn = atoi(nmea + toks[i]);
    if (prn == 0) continue;
    int ele = atoi(nmea + toks[i + 1]);
    int azi = atoi(nmea + toks[i + 2]);
    int snr = atoi(nmea + toks[i + 3]);

    int idx = -1;
    for (int j = 0; j < satCount; j++) {
      if (sats[j].sys == sys && sats[j].prn == prn) {
        idx = j;
        break;
      }
    }
    if (idx == -1 && satCount < 40) idx = satCount++;
    if (idx != -1) {
      sats[idx].sys = sys;
      sats[idx].prn = prn;
      sats[idx].ele = ele;
      sats[idx].azi = azi;
      sats[idx].snr = snr;
      sats[idx].lastSeen = millis();
    }
  }
}

void GPSCalc::cleanupSats() {
  uint32_t now = millis();
  for (int i = 0; i < satCount;) {
    if (now - sats[i].lastSeen > 5000) {
      sats[i] = sats[satCount - 1];
      satCount--;
    } else i++;
  }
  for (int i = 0; i < 4; i++) sysTracked[i] = 0;
  for (int i = 0; i < satCount; i++) {
    if (sats[i].snr > 15) sysTracked[sats[i].sys]++;
  }
}


bool GPSCalc::isGpsReady() {
  return gps.charsProcessed() > 0;
}