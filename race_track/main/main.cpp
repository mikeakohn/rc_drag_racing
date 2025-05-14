/**
 *  Race Tarck
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: https://www.mikekohn.net/
 * License: BSD
 *
 * Copyright 2025 by Michael Kohn 
 *
 * https://www.mikekohn.net/
 *
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "soc/gpio_reg.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

#include "RaceTrack.h"

static void show_chip_info()
{
  // Print chip information.
  esp_chip_info_t chip_info;
  uint32_t flash_size;

  esp_chip_info(&chip_info);

  printf(
    "This is %s chip with %d CPU core(s), WiFi%s%s, ",
    CONFIG_IDF_TARGET,
    chip_info.cores,
   (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
   (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");

  uint32_t major_rev = chip_info.revision / 100;
  uint32_t minor_rev = chip_info.revision % 100;
  printf("silicon revision v%d.%d, ", major_rev, minor_rev);

  if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
  {
    printf("Get flash size failed");
    return;
  }

  printf(
    "%uMB %s flash\n", flash_size / (1024 * 1024),
    (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

  printf("Minimum free heap size: %d bytes\n",
    esp_get_minimum_free_heap_size());
}

#ifdef __cplusplus
extern "C"
{
#endif

void app_main()
{
  show_chip_info();

  for (int i = 4; i >= 0; i--)
  {
    printf("Restarting in %d seconds...\n", i);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // Turn off watchdog.
  esp_task_wdt_deinit();

  ESP_ERROR_CHECK(nvs_flash_init());

  RaceTrack race_track;

  race_track.run();

  printf("Restarting now.\n");
  fflush(stdout);

  esp_restart();
}

#ifdef __cplusplus
}
#endif

