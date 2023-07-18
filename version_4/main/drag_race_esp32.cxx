/**
 *  Drag Race Tree 4.0
 *  Author: Michael Kohn
 *   Email: mike@mikekohn.net
 *     Web: https://www.mikekohn.net/
 * License: BSD
 *
 * Copyright 2023 by Michael Kohn
 *
 * https://www.mikekohn.net/micro/drag_racing_tree.php
 *
 * This is source code for firmware for the ESP32-C3-DevKitM-1
 * for both the start track circuit and end track circuit. A jumper
 * between GPIO19 and GND (pins 14/15 on J3) will select between
 * the start / end circuit.
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
#include "esp_spiffs.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"

#include "defines.h"
#include "DragRaceStart.h"
#include "DragRaceEnd.h"

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

static void init_select_gpio()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Check pin 19 to see if this is the start or end track circuit.
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_INPUT_TRACK_SELECT);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
}

static void init_nvs_flash()
{
  // Setting up NVS (Non Volatile Storage) appears to be needed for WiFi.
  esp_err_t ret = nvs_flash_init();

  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }

  ESP_ERROR_CHECK(ret);
}

static void init_spiffs()
{
  esp_vfs_spiffs_conf_t spiffs_config = { };

  spiffs_config.base_path = "/spiffs";
  spiffs_config.partition_label = NULL;
  spiffs_config.max_files = 5;
  spiffs_config.format_if_mount_failed = true;

  esp_err_t ret = esp_vfs_spiffs_register(&spiffs_config);
  ESP_ERROR_CHECK(ret);
}

#if 0
static void stop_timer(gptimer_handle_t gptimer)
{
  ESP_ERROR_CHECK(gptimer_stop(gptimer));
}
#endif

static void run()
{
  init_select_gpio();

  if (gpio_get_level(GPIO_INPUT_IO_19) != 0)
  {
    DragRaceStart drag_race_start;
    drag_race_start.run();
  }
    else
  {
    DragRaceEnd drag_race_end;
    drag_race_end.run();
  }

#if 0
  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    //gpio_set_level(GPIO_OUTPUT_IO_19, count % 2);
    count++;

    //if (gpio_get_level(GPIO_INPUT_IO_1) == 0)
    if (gpio_get_level(GPIO_INPUT_IO_4) == 1 && count > 5)
    {
      if (! user_data.is_playing)
      {
        //user_data.sound_ptr = 0;
        user_data.is_playing = true;
        gpio_set_level(GPIO_OUTPUT_IO_1, 1);
      }

      count = 0;
    }
  }
#endif
}

#ifdef __cplusplus
extern "C"
{
#endif

void app_main()
{
  show_chip_info();

  for (int i = 10; i >= 0; i--)
  {
    printf("Restarting in %d seconds...\n", i);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  init_nvs_flash();
  init_spiffs();

  // Turn off watchdog.
  esp_task_wdt_deinit();

  run();

  printf("Restarting now.\n");
  fflush(stdout);

  esp_restart();
}

#ifdef __cplusplus
}
#endif

