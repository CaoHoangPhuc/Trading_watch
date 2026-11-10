#include <ArduinoJson.h>
#include <WebSockets.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "WatchModeController.h"
#include "Config.h"

WatchModeController gController;
TaskHandle_t gAppTaskHandle = nullptr;

void appTask(void*) {
  gController.begin();
  for (;;) {
    gController.loop();
    // Let lower-priority system/IDLE tasks run to avoid task_wdt resets.
    vTaskDelay(1);
  }
}

void setup() {
  Serial.begin(921600);
  delay(300);
  BaseType_t ok = xTaskCreatePinnedToCore(appTask, "appTask", Config::APP_TASK_STACK_WORDS, nullptr,
                                          Config::APP_TASK_PRIORITY, &gAppTaskHandle, Config::APP_TASK_CORE);
  if (ok != pdPASS) {
    Serial.println("[rtos] failed to create appTask; running on loopTask fallback");
    gController.begin();
  } else {
    Serial.printf("[rtos] appTask started on core %d\n", static_cast<int>(Config::APP_TASK_CORE));
  }
}

void loop() {
  if (gAppTaskHandle == nullptr) {
    gController.loop();
    vTaskDelay(1);
    return;
  }

  // Keep Arduino loopTask mostly idle to avoid starving IDLE0 on LoopCore=0.
  vTaskDelay(pdMS_TO_TICKS(1000));
}
