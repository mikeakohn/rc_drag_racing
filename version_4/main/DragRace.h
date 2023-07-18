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

#ifndef DRAG_RACE_H
#define DRAG_RACE_H

class DragRace
{
public:
  DragRace();
  ~DragRace();

protected:
  struct TimerUserData
  {
    int interrupt_count;
    int count_value;
    DragRace *drag_race;
  };

  void init_laser_sensor();
  void init_timer(int resolution, gptimer_alarm_cb_t timer_callback);
  int detect_laser();

  gptimer_handle_t gptimer;
  TimerUserData timer_user_data;
  bool is_right_blocked;
  bool is_left_blocked;
};

#endif

