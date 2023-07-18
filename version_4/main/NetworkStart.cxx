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

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "NetworkStart.h"

#define COMPARE(line, a) strncmp(line, a, sizeof(a) - 1)

NetworkStart::NetworkStart() :
  httpd_pid         { 0 },
  control_pid       { 0 },
  control_socket_id { -1 },
  start_race_signal { false },
  received_right    { false },
  received_left     { false },
  fault_right       { false },
  fault_left        { false },
  right_score       { 0 },
  left_score        { 0 },
  timing            { false },
  running           { false }
{
}

NetworkStart::~NetworkStart()
{
}

int NetworkStart::start()
{
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());

  esp_netif_create_default_wifi_ap();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_event_handler_instance_register(
    WIFI_EVENT,
    ESP_EVENT_ANY_ID,
    &wifi_event_handler,
    NULL,
    NULL));

  wifi_config_t wifi_config = { };

  strcpy((char *)wifi_config.ap.ssid, SSID);
  wifi_config.ap.ssid_len = sizeof(SSID) - 1;
  wifi_config.ap.channel = CHANNEL;
  strcpy((char *)wifi_config.ap.password, PASSWORD);
  wifi_config.ap.max_connection = 8;
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
  wifi_config.ap.authmode = WIFI_AUTH_WPA3_PSK;
  wifi_config.ap.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
#else
  wifi_config.ap.authmode = WIFI_AUTH_WPA2_PSK;
#endif
  wifi_config.ap.pmf_cfg.required = true;

  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());

  ESP_LOGI(
    "wifi",
    "wifi_init_softap finished. SSID:%s password:%s channel:%d",
    SSID, PASSWORD, CHANNEL);

  pthread_create(&httpd_pid,   NULL, httpd_thread,   this);
  pthread_create(&control_pid, NULL, control_thread, this);

  return 0;
}

int NetworkStart::send_start_race()
{
  const char *text = "S";

  if (control_socket_id != -1)
  {
    if (Network::net_send(control_socket_id, (const uint8_t *)text, strlen(text)) > 0)
    {
      running = true;
      timing = false;
      received_right = false;
      received_left = false;
      return 0;
    }
  }

  return -1;
}

int NetworkStart::send_start_timer()
{
  const char *text = "T";

  if (control_socket_id != -1)
  {
    if (Network::net_send(control_socket_id, (const uint8_t *)text, strlen(text)) > 0)
    {
      timing = true;
      return 0;
    }
  }

  return -1;
}

int NetworkStart::send_fault_right()
{
  const char *text = "R";

  received_right = true;
  fault_right = true;

  test_stop();

  if (control_socket_id != -1)
  {
    Network::net_send(control_socket_id, (const uint8_t *)text, strlen(text));
    return 0;
  }

  return -1;
}

int NetworkStart::send_fault_left()
{
  const char *text = "L";

  received_left = true;
  fault_left = true;

  test_stop();

  if (control_socket_id != -1)
  {
    Network::net_send(control_socket_id, (const uint8_t *)text, strlen(text));
    return 0;
  }

  return -1;
}

void NetworkStart::clear_stats()
{
  received_right = false;
  received_left = false;
  fault_right = false;
  fault_left = false;
  right_score = 0;
  left_score = 0;
  timing = false;
  running = false;
}

void NetworkStart::wifi_event_handler(
  void *arg,
  esp_event_base_t event_base,
  int32_t event_id,
  void *event_data)
{
  if (event_id == WIFI_EVENT_AP_STACONNECTED)
  {
    wifi_event_ap_staconnected_t *event =
      (wifi_event_ap_staconnected_t *)event_data;

    ESP_LOGI(
      "wifi",
      "station " MACSTR " join, AID=%d",
      MAC2STR(event->mac),
      event->aid);
  }
    else
  if (event_id == WIFI_EVENT_AP_STADISCONNECTED)
  {
    wifi_event_ap_stadisconnected_t *event =
      (wifi_event_ap_stadisconnected_t *)event_data;

    ESP_LOGI(
      "wifi",
      "station " MACSTR " leave, AID=%d",
      MAC2STR(event->mac),
      event->aid);
  }
}

void NetworkStart::httpd_response_index(int socket_id)
{
  FILE *in;
  char text[1024];
  int length = 0;

  in = fopen("/spiffs/index.html", "r");

  if (in != NULL)
  {
    fseek(in, 0, SEEK_END);
    length = ftell(in);
    fseek(in, 0, SEEK_SET);
  }

  sprintf(
    text,
    "HTTP/1.1 200 OK\n"
    "Cache-Control: no-cache, must-revalidate\n"
    "Pragma: no-cache\n"
    "Content-Type: text/html\n"
    "Content-Length: %d\n\n",
    length);

  Network::net_send(socket_id, (const uint8_t *)text, strlen(text));

  if (in == NULL) { return; }

  while (true)
  {
    length = fread(text, 1, sizeof(text), in);
    if (length == 0) { break; }

    Network::net_send(socket_id, (const uint8_t *)text, length);
  }

  fclose(in);
}

void NetworkStart::httpd_response_start(int socket_id)
{
  const char *text =
    "HTTP/1.1 200 OK\n"
    "Cache-Control: no-cache, must-revalidate\n"
    "Pragma: no-cache\n"
    "Content-Type: text/html\n"
    "Content-Length: 18\n\n"
    "{ \"status\": \"ok\" }";

  Network::net_send(socket_id, (const uint8_t *)text, strlen(text));

  start_race_signal = true;
  running = false;
  timing = false;
}

void NetworkStart::httpd_response_status(int socket_id)
{
  char text[512];
  char header[256];
  char right[16];
  char left[16];

  right[0] = 0;
  left[0] = 0;

  if (right_score != 0 && fault_right == false)
  {
    snprintf(right, sizeof(right), "%.2f", right_score);
  }

  if (left_score != 0 && fault_left == false)
  {
    snprintf(left, sizeof(left), "%.2f", left_score);
  }

  sprintf(
    text,
    "{ "
    " \"right_score\": \"%s\","
    " \"left_score\": \"%s\","
    " \"right_fault\": %s,"
    " \"left_fault\": %s,"
    " \"timing\": %s,"
    " \"running\": %s"
    "}",
    right,
    left,
    fault_right ? "true" : "false",
    fault_left ? "true" : "false",
    timing ? "true" : "false",
    running ? "true" : "false");

  int length = strlen(text);

  sprintf(
    header,
    "HTTP/1.1 200 OK\n"
    "Cache-Control: no-cache, must-revalidate\n"
    "Pragma: no-cache\n"
    "Content-Type: text/html\n"
    "Content-Length: %d\n\n",
    length);

  Network::net_send(socket_id, (const uint8_t *)header, strlen(header));
  Network::net_send(socket_id, (const uint8_t *)text, length);
}

void NetworkStart::httpd_response_404(int socket_id)
{
  const char *text =
    "HTTP/1.1 200 OK\n"
    "Cache-Control: no-cache, must-revalidate\n"
    "Pragma: no-cache\n"
    "Content-Type: text/html\n"
    "Content-Length: 44\n\n"
    "<html><body><h1>Not Found</h1></body></html>";

  Network::net_send(socket_id, (const uint8_t *)text, strlen(text));
}

int NetworkStart::httpd_get_response(int socket_id)
{
  char buffer[1024];
  char line[256];
  //int filled = 0;
  int ptr = 0;
  bool done = false;
  int response = 0;

  while (! done)
  {
    int length = net_recv(socket_id, (uint8_t *)buffer, sizeof(buffer));
    if (length <= 0) { return -1; }

    //filled += lenght;

    for (int n = 0; n < length; n++)
    {
      if (buffer[n] == '\r') { continue; }

      if (buffer[n] == '\n')
      {
        line[ptr++] = 0;

        if (line[0] == 0)
        {
          done = true;
          break;
        }
          else
        if (COMPARE(line, "GET / ") == 0 ||
            COMPARE(line, "GET /index.html") == 0)
        {
          response = 1;
        }
          else
        if (COMPARE(line, "GET /start.json") == 0)
        {
          response = 2;
        }
          else
        if (COMPARE(line, "GET /status.json") == 0)
        {
          response = 3;
        }

        ptr = 0;
      }
        else
      {
        if (ptr == sizeof(line)) { ptr = 0; }
        line[ptr++] = buffer[n];
      }
    }
  }

  return response;
}

void NetworkStart::httpd_process(int socket_id)
{
  int response = httpd_get_response(socket_id);

  if (response < 0) { return; }

  switch (response)
  {
    case 1: httpd_response_index(socket_id); break;
    case 2: httpd_response_start(socket_id); break;
    case 3: httpd_response_status(socket_id); break;
    default: httpd_response_404(socket_id); break;
  }
}

void NetworkStart::httpd_run()
{
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;

  int socket_id = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_id < 0)
  {
    printf("Can't open socket.\n");
    return;
  }

  memset((char*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(HTTP_PORT);

  if (bind(socket_id, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Server can't bind.\n");
    return;
  }

  if (listen(socket_id, 1) != 0)
  {
    printf("Listen failed.\n");
    return;
  }

  while (true)
  {
    socklen_t n = sizeof(client_addr);
    int client = accept(socket_id, (struct sockaddr *)&client_addr, &n);

    if (client == -1) { continue; }
    fcntl(client, F_SETFL, O_NONBLOCK);

    httpd_process(client);
    Network::net_close(client);
  }
}

float NetworkStart::get_score(uint8_t buffer[])
{
  int value =
     buffer[1] |
    (buffer[2] << 8)  |
    (buffer[3] << 16) |
    (buffer[4] << 24);

  return (float)value / 100;
}

void NetworkStart::control_process(int socket_id)
{
  int length, n;
  uint8_t buffer[5];
  struct timeval tv;
  fd_set readset;

  control_socket_id = socket_id;

  while (true)
  {
    length = 0;

    while (length < 5)
    {
      FD_ZERO(&readset);
      FD_SET(socket_id, &readset);

      tv.tv_sec = 2;
      tv.tv_usec = 0;

      n = select(socket_id + 1, &readset, NULL, NULL, &tv);

      if (n == -1)
      {
        if (errno == EINTR) { continue; }
        return;
      }

      // Timeout on select().
      //if (n == 0) { continue; }

      n = recv(socket_id, buffer + length, sizeof(buffer) - length, 0);

      if (n == 0) { continue; }

      if (n < 0)
      {
        if (errno == ENOTCONN) { return; }
        continue;
      }

      length += n;
    }

    switch (buffer[0])
    {
      case 'R':
        right_score = get_score(buffer);
        received_right = true;
        break;
      case 'L':
        left_score = get_score(buffer);
        received_left = true;
        break;
      default:
        return;
    }

    test_stop();
  }

  control_socket_id = -1;
}

void NetworkStart::control_run()
{
  struct sockaddr_in server_addr;
  struct sockaddr_in client_addr;

  int socket_id = socket(AF_INET, SOCK_STREAM, 0);

  if (socket_id < 0)
  {
    printf("Can't open socket.\n");
    return;
  }

  memset((char*)&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(CONTROL_PORT);

  if (bind(socket_id, (const sockaddr *)&server_addr, sizeof(server_addr)) < 0)
  {
    printf("Server can't bind.\n");
    return;
  }

  if (listen(socket_id, 1) != 0)
  {
    printf("Listen failed.\n");
    return;
  }

  while (true)
  {
    socklen_t n = sizeof(client_addr);
    int client = accept(socket_id, (struct sockaddr *)&client_addr, &n);

    if (client == -1) { continue; }
    fcntl(client, F_SETFL, O_NONBLOCK);

    ESP_LOGI("control_run", "New connection socket_id=%d\n", client);
    control_process(client);
    ESP_LOGI("control_run", "Disconnect.\n");

    Network::net_close(client);
  }
}

void *NetworkStart::httpd_thread(void *context)
{
  NetworkStart *network_start = (NetworkStart *)context;
  network_start->httpd_run();

  return NULL;
}

void *NetworkStart::control_thread(void *context)
{
  NetworkStart *network_start = (NetworkStart *)context;
  network_start->control_run();

  return NULL;
}

