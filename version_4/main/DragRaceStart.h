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

#ifndef DRAG_RACE_START_H
#define DRAG_RACE_START_H

#include "DragRace.h"
#include "NetworkStart.h"

class DragRaceStart : public DragRace
{
public:
  DragRaceStart();
  ~DragRaceStart();

  void run();

private:
  void init_gpio();
  void set_tone(int frequency);
  void set_state(int value);
  void update_state()     { set_state(state + 1);        }
  void send_start_race()  { network->send_start_race();  }
  void send_start_timer() { network->send_start_timer(); }
  void send_fault_right() { network->send_fault_right(); }
  void send_fault_left()  { network->send_fault_left();  }

  static bool IRAM_ATTR timer_callback(
    gptimer_handle_t timer,
    const gptimer_alarm_event_data_t *edata,
    void *args);

  NetworkStart *network;
  int state;
  bool start_race_flag;
  bool start_timer_flag;
  bool fault_right;
  bool fault_left;
};

#endif

