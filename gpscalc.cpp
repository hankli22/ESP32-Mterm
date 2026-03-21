#include "gpscalc.h"
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
double GPSCalc::altitude = 0;
double GPSCalc::course = 0;
double GPSCalc::homeLat = 0, GPSCalc::homeLng = 0;
double GPSCalc::lastLat = 0, GPSCalc::lastLng = 0;

LapInfo GPSCalc::lapHistory[MAX_LAPS];
float GPSCalc::trackX[MAX_TRACK_POINTS];
float GPSCalc::trackY[MAX_TRACK_POINTS];
int GPSCalc::trackPointsCount = 0;

float GPSCalc::paceBuffer[PACE_WINDOW_SIZE] = {0};
int GPSCalc::paceBufIdx = 0;
uint32_t GPSCalc::lastPaceUpdate = 0;
float GPSCalc::lastDistForPace = 0;
uint32_t GPSCalc::runStartTime = 0;
uint32_t GPSCalc::lastLapTime = 0;
uint32_t GPSCalc::lastTrackSaveTime = 0;

static TinyGPSPlus gps;

void GPSCalc::init() {
    Serial1.begin(9600, SERIAL_8N1, 17, 18);
    if (sysCfg.record_freq >= 5.0) Serial1.println("$PCAS02,200*1D");
    else if (sysCfg.record_freq >= 2.0) Serial1.println("$PCAS02,500*1A");
    else Serial1.println("$PCAS02,1000*2E");
}

void GPSCalc::startRun() {
    isRunning = true;
    totalDistance = 0; laps = 0; trackPointsCount = 0;
    homeLat = 0; durationSec = 0;
    runStartTime = millis(); lastLapTime = millis();
    
    for(int i=0; i<PACE_WINDOW_SIZE; i++) paceBuffer[i] = 0;
    lastDistForPace = 0;
    lastPaceUpdate = millis();
    
    lapHistory[0].trackStartIdx = 0;
}

void GPSCalc::stopRun() { 
    isRunning = false; 
    if (laps < MAX_LAPS) {
        lapHistory[laps].trackEndIdx = trackPointsCount - 1;
    }
}

static float calcDist(double lat1, double lng1, double lat2, double lng2) {
    return (float)TinyGPSPlus::distanceBetween(lat1, lng1, lat2, lng2);
}

String GPSCalc::getDateTime() {
    if (!gps.time.isValid() || gps.date.year() < 2020) {
        if (gps.time.isValid()) {
            char tBuf[20];
            int h = (gps.time.hour() + 8) % 24;
            sprintf(tBuf, "TIME: %02d:%02d:%02d", h, (int)gps.time.minute(), (int)gps.time.second());
            return String(tBuf);
        }
        return "WAITING SATELLITES";
    }
    char timeBuf[20];
    int localHour = gps.time.hour() + 8;
    int localDay = gps.date.day();
    if (localHour >= 24) { localHour -= 24; localDay += 1; }
    sprintf(timeBuf, "%02d/%02d/%02d %02d:%02d:%02d", 
            (int)(gps.date.year() % 100), (int)gps.date.month(), (int)localDay, 
            (int)localHour, (int)gps.time.minute(), (int)gps.time.second());
    return String(timeBuf);
}

void GPSCalc::process() {
    while (Serial1.available() > 0) gps.encode(Serial1.read());
    satellites = gps.satellites.value();

    if (!gps.location.isValid() || gps.location.age() > 2000) return;

    double lat = gps.location.lat();
    double lng = gps.location.lng();
    currentSpeed = gps.speed.kmph();
    altitude = gps.altitude.meters();
    course = gps.course.deg();

    if (!isRunning) return;
    uint32_t now = millis();
    durationSec = (now - runStartTime) / 1000;

    // --- 10秒滑动窗口实时配速计算 ---
    if (now - lastPaceUpdate >= 1000) {
        float distThisSec = totalDistance - lastDistForPace;
        lastDistForPace = totalDistance;
        paceBuffer[paceBufIdx] = distThisSec;
        paceBufIdx = (paceBufIdx + 1) % PACE_WINDOW_SIZE;
        lastPaceUpdate = now;

        float distInWindow = 0;
        for(int i=0; i<PACE_WINDOW_SIZE; i++) distInWindow += paceBuffer[i];
        
        if (distInWindow > 0.5) {
            float speedMps = distInWindow / (float)PACE_WINDOW_SIZE;
            slidingPace = 16.6667f / speedMps; // min/km
            paceMin = (int)slidingPace;
            paceSec = (int)((slidingPace - paceMin) * 60);
        } else {
            slidingPace = 0; paceMin = 0; paceSec = 0;
        }
    }

    if (homeLat == 0) {
        if (satellites > 4 && gps.hdop.hdop() < 2.5) {
            homeLat = lat; homeLng = lng; lastLat = lat; lastLng = lng; lastLapTime = now;
        }
        return;
    }

    // --- 锚点法：完美解决慢走距离被吞的问题 ---
    float d = calcDist(lat, lng, lastLat, lastLng);
    if (d >= 2.0 && d < 30.0) { // 真实移动超过 2.0 米才记录
        totalDistance += d; 
        lastLat = lat; // 更新锚点
        lastLng = lng;
        
        if (now - lastTrackSaveTime > 2000 && trackPointsCount < MAX_TRACK_POINTS) {
            trackX[trackPointsCount] = (float)((lng - homeLng) * 111320.0 * cos(homeLat * 0.01745));
            trackY[trackPointsCount] = (float)((lat - homeLat) * 111320.0);
            trackPointsCount++; lastTrackSaveTime = now;
        }
    }

    // --- 250m 智能分圈 ---
    float distToHome = calcDist(lat, lng, homeLat, homeLng);
    if (distToHome < 15.0 && totalDistance > (laps * 250 + 200)) {
        lapHistory[laps].timeSec = (now - lastLapTime) / 1000;
        lapHistory[laps].pace = lapHistory[laps].timeSec / 0.25f / 60.0f; // min/km
        lapHistory[laps].trackEndIdx = trackPointsCount > 0 ? trackPointsCount - 1 : 0;
        
        laps++;
        if (laps < MAX_LAPS) lapHistory[laps].trackStartIdx = trackPointsCount;
        lastLapTime = now;
    }
}