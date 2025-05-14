/**
 *  MiniGolf
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

#ifndef RACE_TRACK_H
#define RACE_TRACK_H

#include <string.h>

#include "driver/spi_master.h"
#include "driver/spi_common.h"

class RaceTrack
{
public:
  RaceTrack();
  ~RaceTrack();

  void run();

private:
  void gpio_init();
  void timer_init(int resolution, gptimer_alarm_cb_t timer_callback);
  void spi_init();

  void spi_send(uint8_t ch);
  void spi_send_sw(uint8_t ch);

  void display_clear();
  void display_show(const char *text);
  void display_show(int value);
  void display_update();
  void display_set_color(int r, int g, int b);

  void set_tone(int frequency);
  void play_song_finish();

  void tree_pulse(int value);
  void tree_set_start();
  void tree_set_drop();
  void tree_set_red();
  void tree_set_blank();

  bool start_game();
  void run_game();

  static bool IRAM_ATTR timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *args);

  gptimer_handle_t gptimer;
  spi_device_handle_t spi_handle;

  int lap_times[3];
  int lap_number;
  int lap_final;
  bool running;

  struct TimerUserData
  {
    int interrupt_count;
    int count_value;
    RaceTrack *race_track;
  } timer_user_data;

  volatile int ticks;

  static const char *TAG;
};

#endif

