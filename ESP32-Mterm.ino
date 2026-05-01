#include <Arduino.h>
#include "hardwareLayer.h"
#include "gps_module.h"
#include "ui.h"
#include "config.h"
#pragma GCC optimize("O2")


TaskHandle_t uiTaskHandle;
TaskHandle_t logicTaskHandle;

void uiTask(void* pvParameters) {
  for (;;) {
    HAL::updateButtons();
    MenuManager::handleInput();
    MenuManager::update();
    vTaskDelay(pdMS_TO_TICKS(16));
  }
}

void logicTask(void* pvParameters) {
  for (;;) {
    GPSCalc::process();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void setup() {
  Serial.begin(115200);
  loadConfig();
  HAL::init();
  GPSCalc::init();

  xTaskCreatePinnedToCore(uiTask, "UI", 8192, NULL, 1, &uiTaskHandle, 0);
  xTaskCreatePinnedToCore(logicTask, "Logic", 4096, NULL, 2, &logicTaskHandle, 0);
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}