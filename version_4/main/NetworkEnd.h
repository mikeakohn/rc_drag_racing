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

#ifndef NETWORK_END_H
#define NETWORK_END_H

#include <pthread.h>

#include "esp_event.h"
#include "esp_wifi.h"

#include "Network.h"

class NetworkEnd : public Network
{
public:
  NetworkEnd();
  ~NetworkEnd();

  int start();
  int start_network_thread();
  int start_wifi();
  int send_right_value();
  int send_left_value();

  bool is_connected() { return socket_id > 0; }

  bool do_start_race()
  {
    bool value = start_race_signal;
    if (value == false) { return false; }
    start_race_signal = false;
    return value;
  }

  bool do_start_timer()
  {
    bool value = start_timer_signal;
    if (value == false) { return false; }
    start_timer_signal = false;
    return value;
  }

  void set_right_value(int value)
  {
    right_value = value;
    send_right_value();
  }

  void set_left_value(int value)
  {
    left_value  = value;
    send_left_value();
  }

  bool is_right_fault() { return right_fault; }
  bool is_left_fault()  { return left_fault; }

private:
  static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data);

  void reset();
  int net_connect();
  void control_run();

  static void *control_thread(void *context);

  pthread_t control_pid;
  int socket_id;

  bool right_fault;
  bool left_fault;
  bool start_race_signal;
  bool start_timer_signal;
  int right_value;
  int left_value;
};

#endif

