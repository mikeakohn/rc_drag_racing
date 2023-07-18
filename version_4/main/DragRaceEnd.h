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

#ifndef DRAG_RACE_END_H
#define DRAG_RACE_END_H

#include "driver/spi_master.h"
#include "driver/spi_common.h"

#include "DragRace.h"
#include "NetworkEnd.h"

class DragRaceEnd : public DragRace
{
public:
  DragRaceEnd();
  ~DragRaceEnd();

  void run();

private:
  void init_gpio();
  void init_spi_sw();
  void init_spi_hw();
  void init_spi() { init_spi_hw(); }
  void send_spi_sw(char ch);
  void send_spi_hw(char ch);
  void send_spi(char ch) { send_spi_hw(ch); }
  void clear_displays();
  void display_digits(int value);
  void display_time_right();
  void display_time_left();
  void display_not_connected();
  void update_display();
  void set_right_value(int value) { right.value = value; };
  void set_left_value(int value)  { left.value  = value; };

  static bool IRAM_ATTR timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *args);

  spi_device_handle_t spi_handle;

  NetworkEnd *network;
  bool need_update_display;
  bool is_connected;

  struct Timer
  {
    Timer() :
      running    { false },
      enabled    { false },
      value      { 0 },
      last_value { -1 }
    {
    }

    void prepare()
    {
      running = false;
      enabled = true;
      value = 0;
      last_value = -1;
    }

    void fault()
    {
      running = false;
      enabled = false;
      last_value = -2;
    }

    void start()
    {
      if (enabled) { running = true; }
    }

    bool running;
    bool enabled;
    int value;
    int last_value;
  };

  Timer right;
  Timer left;
};

#endif

