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
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "soc/gpio_reg.h"

#include "defines.h"
#include "DragRace.h"

DragRace::DragRace() : gptimer { NULL }
{
}

DragRace::~DragRace()
{
}

void DragRace::init_laser_sensor()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Setup debug sensor LEDs.
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_OUTPUT_DEBUG_RIGHT) |
    (1ULL << GPIO_OUTPUT_DEBUG_LEFT);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  // Set laser sensors as input.
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_INPUT_SENSOR_RIGHT) |
    (1ULL << GPIO_INPUT_SENSOR_LEFT);
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  gpio_set_level(GPIO_OUTPUT_DEBUG_RIGHT, 0);
  gpio_set_level(GPIO_OUTPUT_DEBUG_LEFT,  0);
}

void DragRace::init_timer(int resolution, gptimer_alarm_cb_t timer_callback)
{
  gptimer_config_t timer_config = { };
  timer_config.clk_src = GPTIMER_CLK_SRC_DEFAULT;
  timer_config.direction = GPTIMER_COUNT_UP;
  timer_config.resolution_hz = TIMER_RESOLUTION_HZ;

  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  timer_user_data.interrupt_count = 0;
  timer_user_data.count_value = 0;
  timer_user_data.drag_race = this;

  gptimer_event_callbacks_t cbs =
  {
    .on_alarm = timer_callback,
  };

  gptimer_register_event_callbacks(gptimer, &cbs, &timer_user_data);
  gptimer_enable(gptimer);

  gptimer_alarm_config_t alarm_config = { };
  //.alarm_count = 1000000, // period = 1s
  alarm_config.alarm_count = resolution;
  alarm_config.flags.auto_reload_on_alarm = true;

  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
}

int DragRace::detect_laser()
{ 
  int value = 0;

  is_right_blocked = gpio_get_level(GPIO_INPUT_SENSOR_RIGHT) == 0;
  is_left_blocked  = gpio_get_level(GPIO_INPUT_SENSOR_LEFT)  == 0;

  gpio_set_level(GPIO_OUTPUT_DEBUG_RIGHT, is_right_blocked ? 1 : 0);
  gpio_set_level(GPIO_OUTPUT_DEBUG_LEFT,  is_left_blocked  ? 1 : 0);

  value = (is_left_blocked  ? 2 : 0) | (is_right_blocked ? 1 : 0);

  return value;
} 

