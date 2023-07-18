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

#ifndef NETWORK_START_H
#define NETWORK_START_H

#include <pthread.h>

#include "esp_event.h"
#include "esp_wifi.h"

#include "Network.h"

class NetworkStart : public Network
{
public:
  NetworkStart();
  ~NetworkStart();

  int start();
  int send_start_race();
  int send_start_timer();
  int send_fault_right();
  int send_fault_left();
  void clear_stats();

  void test_stop()
  {
    if (received_right && received_left)
    {
      timing = false;
      running = false;
    }
  }

  bool can_fault()
  {
    return running && (timing == false);
  }

  bool do_start_race()
  {
    bool value = start_race_signal;
    if (value == false) { return false; }
    start_race_signal = false;
    return value;
  }

private:
  static void wifi_event_handler(
    void *arg,
    esp_event_base_t event_base,
    int32_t event_id,
    void *event_data);

  void httpd_response_index(int socket_id);
  void httpd_response_start(int socket_id);
  void httpd_response_status(int socket_id);
  void httpd_response_404(int socket_id);
  int httpd_get_response(int socket_id);
  void httpd_process(int socket_id);
  void httpd_run();

  float get_score(uint8_t buffer[]);
  void control_process(int socket_id);
  void control_run();

  static void *httpd_thread(void *context);
  static void *control_thread(void *context);

  pthread_t httpd_pid;
  pthread_t control_pid;

  int control_socket_id;

  bool start_race_signal;

  bool received_right;
  bool received_left;

  bool fault_right;
  bool fault_left;

  float right_score;
  float left_score;
  bool timing;
  bool running;
};

#endif

