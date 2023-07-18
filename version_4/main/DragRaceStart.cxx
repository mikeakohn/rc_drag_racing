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
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "soc/gpio_reg.h"
#include "esp_chip_info.h"
#include "esp_flash.h"

#include "defines.h"
#include "DragRaceStart.h"

DragRaceStart::DragRaceStart() :
  state            { 0 },
  start_race_flag  { false },
  start_timer_flag { false },
  fault_right      { false },
  fault_left       { false }
{
  network = new NetworkStart();
}

DragRaceStart::~DragRaceStart()
{
  delete network;
}

void DragRaceStart::run()
{
  network->start();

  init_gpio();
  init_laser_sensor();

  set_state(0);

  // Interrupt every 1/2 second.
  init_timer(500000, timer_callback);
  ESP_ERROR_CHECK(gptimer_start(gptimer));

  while (true)
  {
    if (gpio_get_level(GPIO_INPUT_BUTTON) == 0 || network->do_start_race())
    {
      set_state(1);
      continue;
    }

    int value = detect_laser();

    if (network->can_fault())
    {
      if ((value & 1) != 0)
      {
        if (! fault_right)
        {
          send_fault_right();
          fault_right = true;
        }
      }

      if ((value & 2) != 0)
      {
        if (! fault_left)
        {
          send_fault_left();
          fault_left = true;
        }
      }
    }

    if (start_race_flag)
    {
      network->clear_stats();

      send_start_race();
      start_race_flag = false;
      fault_right = false;
      fault_left = false;
    }

    if (start_timer_flag)
    {
      send_start_timer();
      start_timer_flag = false;
    }
  }
}

void DragRaceStart::init_gpio()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Setup LEDs.
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_OUTPUT_RED_LEFT) |
    (1ULL << GPIO_OUTPUT_GREEN) |
    (1ULL << GPIO_OUTPUT_YELLOW_BOTTOM) |
    (1ULL << GPIO_OUTPUT_RED_RIGHT) |
    (1ULL << GPIO_OUTPUT_YELLOW_MIDDLE) |
    (1ULL << GPIO_OUTPUT_YELLOW_TOP);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  // Set button as input with a pullup.
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.pin_bit_mask = (1ULL << GPIO_INPUT_BUTTON);
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);
}

void DragRaceStart::set_tone(int frequency)
{
  if (frequency == 0)
  {
    // Turn off sound.
    gpio_set_level(GPIO_OUTPUT_AUDIO, 0);
    ledc_stop(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
    return;
  }

  ledc_timer_config_t ledc_timer = { };
  ledc_timer.speed_mode      = LEDC_LOW_SPEED_MODE;
  ledc_timer.timer_num       = LEDC_TIMER_0;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;
  ledc_timer.freq_hz         = frequency;
  ledc_timer.clk_cfg         = LEDC_AUTO_CLK;

  ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = { };
  ledc_channel.speed_mode = LEDC_LOW_SPEED_MODE;
  ledc_channel.channel    = LEDC_CHANNEL_0;
  ledc_channel.timer_sel  = LEDC_TIMER_0;
  ledc_channel.intr_type  = LEDC_INTR_DISABLE;
  ledc_channel.gpio_num   = GPIO_OUTPUT_AUDIO;
  ledc_channel.duty       = frequency / 2;
  ledc_channel.hpoint     = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  gpio_set_level(GPIO_OUTPUT_AUDIO, 1);
}

void DragRaceStart::set_state(int value)
{
  state = value;

  switch (value)
  {
    case 0:
      gpio_set_level(GPIO_OUTPUT_YELLOW_TOP,    0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_MIDDLE, 0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_BOTTOM, 0);
      gpio_set_level(GPIO_OUTPUT_GREEN,         0);
      gpio_set_level(GPIO_OUTPUT_RED_LEFT,      1);
      gpio_set_level(GPIO_OUTPUT_RED_RIGHT,     1);
      break;
    case 1:
      gpio_set_level(GPIO_OUTPUT_YELLOW_TOP,    0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_MIDDLE, 0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_BOTTOM, 0);
      gpio_set_level(GPIO_OUTPUT_GREEN,         0);
      gpio_set_level(GPIO_OUTPUT_RED_LEFT,      0);
      gpio_set_level(GPIO_OUTPUT_RED_RIGHT,     0);
      start_race_flag = true;
      break;
    case 8:
      gpio_set_level(GPIO_OUTPUT_YELLOW_TOP,    1);
      gpio_set_level(GPIO_OUTPUT_YELLOW_MIDDLE, 0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_BOTTOM, 0);
      gpio_set_level(GPIO_OUTPUT_GREEN,         0);
      set_tone(440);
      break;
    case 9:
      set_tone(0);
      break;
    case 10:
      gpio_set_level(GPIO_OUTPUT_YELLOW_TOP,    0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_MIDDLE, 1);
      gpio_set_level(GPIO_OUTPUT_YELLOW_BOTTOM, 0);
      gpio_set_level(GPIO_OUTPUT_GREEN,         0);
      set_tone(440);
      break;
    case 11:
      set_tone(0);
      break;
    case 12:
      gpio_set_level(GPIO_OUTPUT_YELLOW_TOP,    0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_MIDDLE, 0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_BOTTOM, 1);
      gpio_set_level(GPIO_OUTPUT_GREEN,         0);
      set_tone(440);
      break;
    case 13:
      set_tone(0);
      break;
    case 14:
      gpio_set_level(GPIO_OUTPUT_YELLOW_TOP,    0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_MIDDLE, 0);
      gpio_set_level(GPIO_OUTPUT_YELLOW_BOTTOM, 0);
      gpio_set_level(GPIO_OUTPUT_GREEN,         1);
      start_timer_flag = true;
      set_tone(880);
      break;
    case 16:
      set_tone(0);
      break;
    case 22:
      set_state(0);
      break;
    default:
      break;
  }
}

bool IRAM_ATTR DragRaceStart::timer_callback(
  gptimer_handle_t timer,
  const gptimer_alarm_event_data_t *edata,
  void *args)
{
  TimerUserData *timer_user_data = (TimerUserData *)args;
  timer_user_data->interrupt_count++;
  timer_user_data->count_value = edata->count_value;

  DragRaceStart *drag_race_start = (DragRaceStart *)timer_user_data->drag_race;

  do
  {
    if (drag_race_start->state == 0) { break; }

    drag_race_start->update_state();

  } while (false);

  return pdFALSE;
}

