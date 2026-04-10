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

void GPSCalc::init() {
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
      char tBuf[32];  // 【修复】：扩大缓冲区到32
      int h = (gps.time.hour() + 8) % 24;
      sprintf(tBuf, "TIME: %02d:%02d:%02d", h, (int)gps.time.minute(), (int)gps.time.second());
      return String(tBuf);
    }
    return "WAITING SATELLITES";
  }
  char timeBuf[32];  // 【修复】：扩大缓冲区到32防溢出警告
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
  // 1. 底层 NMEA 拦截器：用于解析 GSV 报文获取详细卫星信息
  static char nmeaBuf[85];
  static int nmeaPos = 0;

  while (Serial1.available() > 0) {
    char c = Serial1.read();
    gps.encode(c);  // 同步喂给 TinyGPS++ 库进行标准解析

    // 拦截 NMEA 语句
    if (c == '$') {
      nmeaPos = 0;
      nmeaBuf[nmeaPos++] = c;
    } else if (c == '\n' || c == '\r') {
      nmeaBuf[nmeaPos] = 0;
      if (nmeaPos > 10) parseGSV(nmeaBuf);  // 调用 GSV 解析函数
      nmeaPos = 0;
    } else if (nmeaPos < 84) {
      nmeaBuf[nmeaPos++] = c;
    }
  }

  // 定期清理丢失信号的卫星，更新 sysTracked/sysInView 数组
  cleanupSats();

  // 2. 更新基础状态数据（无论是否点击 Start 都会更新，供 Dev 页面查看）
  satellites = gps.satellites.value();
  if (satsInViewCustom.isUpdated() && satsInViewCustom.isValid()) {
    satsInView = atoi(satsInViewCustom.value());
  }

  // 更新精准度百分比 (基于 HDOP)
  double h = gps.hdop.hdop();
  if (h > 0) {
    int acc = 100 - (int)((h - 1.0) * 15);
    accuracyPct = constrain(acc, 0, 100);
  } else accuracyPct = 0;

  // 核心修复：更新原始坐标 (rawLat/rawLng)，无视过滤条件供 Dev 页面调试
  if (gps.location.isValid()) {
    rawLat = gps.location.lat();
    rawLng = gps.location.lng();
    altitude = gps.altitude.meters();
    course = gps.course.deg();
    currentSpeed = gps.speed.kmph();
  }

  // 如果没有开始运动，或者定位已失效超过 2 秒，则停止后续计算
  if (!isRunning || !gps.location.isValid() || gps.location.age() > 2000) return;

  uint32_t now = millis();
  durationSec = (now - runStartTime) / 1000;
  double lat = gps.location.lat();
  double lng = gps.location.lng();

  // 3. 10秒滑动窗口配速计算 (每秒执行一次)
  if (now - lastPaceUpdate >= 1000) {
    float distThisSec = totalDistance - lastDistForPace;
    lastDistForPace = totalDistance;

    paceBuffer[paceBufIdx] = distThisSec;
    paceBufIdx = (paceBufIdx + 1) % PACE_WINDOW_SIZE;
    lastPaceUpdate = now;

    float distInWindow = 0;
    for (int i = 0; i < PACE_WINDOW_SIZE; i++) distInWindow += paceBuffer[i];

    if (distInWindow > 0.5f) {  // 窗口内移动超过 0.5 米才计算配速
      float speedMps = distInWindow / (float)PACE_WINDOW_SIZE;
      slidingPace = 16.6667f / speedMps;  // 换算为 min/km
      paceMin = (int)slidingPace;
      paceSec = (int)((slidingPace - paceMin) * 60);
    } else {
      slidingPace = 0;
      paceMin = 0;
      paceSec = 0;
    }

    // 运动分析：更新最大速度
    if (currentSpeed > maxSpeed) maxSpeed = currentSpeed;
    // 运动分析：简易卡路里 (假设 60kg 体重)
    calories = (int)((totalDistance / 1000.0f) * 60.0f * 1.036f);
  }

  // 4. 起点锚定 (Home Point 初始化)
  if (homeLat == 0) {
    // 只有信号质量达标才设置起点，确保计圈精准
    if (satellites > 4 && gps.hdop.hdop() < 2.5) {
      homeLat = lat;
      homeLng = lng;
      lastLat = lat;
      lastLng = lng;
      lastLapTime = now;
      if (laps < MAX_LAPS) lapHistory[laps].trackStartIdx = trackPointsCount;
    }
    return;
  }

  // 5. 核心：锚点法计算距离 (解决慢走被吞、原地漂移问题)
  float d = calcDist(lat, lng, lastLat, lastLng);
  if (d >= 2.0f && d < 35.0f) {  // 真实移动超过 2 米且小于 35 米(防跳点)
    totalDistance += d;
    lastLat = lat;  // 更新锚点
    lastLng = lng;

    // 记录轨迹点 (约每 2 秒存一个点，或每移动一段距离存一个点)
    if (now - lastTrackSaveTime > 2000 && trackPointsCount < MAX_TRACK_POINTS) {
      // 投影计算相对坐标
      trackX[trackPointsCount] = (float)((lng - homeLng) * 111320.0 * cos(homeLat * 0.017453));
      trackY[trackPointsCount] = (float)((lat - homeLat) * 111320.0);
      trackPointsCount++;
      lastTrackSaveTime = now;
    }
  }

  // 6. 250m 智能分圈逻辑
  float distToHome = calcDist(lat, lng, homeLat, homeLng);
  // 判断条件：回到起点 15 米内，且本圈已经跑了至少 200 米
  if (distToHome < 15.0f && totalDistance > (laps * 250.0f + 200.0f)) {
    if (laps < MAX_LAPS) {
      lapHistory[laps].timeSec = (now - lastLapTime) / 1000;
      // 计算本圈平均配速
      if (lapHistory[laps].timeSec > 0) {
        lapHistory[laps].pace = (lapHistory[laps].timeSec / 0.25f) / 60.0f;
      }
      lapHistory[laps].trackEndIdx = (trackPointsCount > 0) ? trackPointsCount - 1 : 0;

      laps++;
      // 设置下一圈的轨迹起始索引
      if (laps < MAX_LAPS) {
        lapHistory[laps].trackStartIdx = trackPointsCount;
      }
    }
    lastLapTime = now;  // 重置本圈计时器
  }
}

void GPSCalc::parseGSV(const char* nmea) {
  if (strncmp(nmea + 3, "GSV", 3) != 0) return;  // 只解析 GSV 报文

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
  sysInView[sys] = atoi(nmea + toks[3]);  // 该星系视野中总数

  // 解析每颗卫星 (PRN, ELE, AZI, SNR)
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
    if (now - sats[i].lastSeen > 5000) {  // 超过5秒没见到的卫星踢出列表
      sats[i] = sats[satCount - 1];
      satCount--;
    } else i++;
  }
  for (int i = 0; i < 4; i++) sysTracked[i] = 0;
  for (int i = 0; i < satCount; i++) {
    if (sats[i].snr > 15) sysTracked[sats[i].sys]++;  // SNR > 15 视为已锁定/Tracked
  }
}


bool GPSCalc::isGpsReady() {
  return gps.charsProcessed() > 0;  // 只要解析到底层数据就说明通信正常
}