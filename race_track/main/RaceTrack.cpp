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

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "driver/spi_common.h"
#include "soc/gpio_reg.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "rom/ets_sys.h"

#include "defines.h"
#include "RaceTrack.h"

RaceTrack::RaceTrack() :
  lap_number { -1 },
  lap_final  { 0 },
  running    { false }
{
  memset(lap_times, 0, sizeof(lap_times));
}

RaceTrack::~RaceTrack()
{
}

void RaceTrack::run()
{
  //NetworkServer server;
  //server.start();

  gpio_init();

  spi_init();
  display_clear();
  display_set_color(29, 29, 29);
  display_update();

  // Interrupt every 1/100th of a second.
  timer_init(10000, timer_callback);
  ESP_ERROR_CHECK(gptimer_start(gptimer));

  while (true)
  {
    vTaskDelay(10 / portTICK_PERIOD_MS);

    if (gpio_get_level(GPIO_BUTTON) == 0)
    {
      run_game();
    }

    //gpio_set_level(GPIO_OUTPUT_IO_19, count % 2);
    //count++;
  }
}

void RaceTrack::gpio_init()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Set GPIO_NUM_4 as input with pullup.
  memset(&io_conf, 0, sizeof(io_conf));
  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_BUTTON);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
  gpio_config(&io_conf);

  // Setup SPI chip select.
  memset(&io_conf, 0, sizeof(io_conf));
  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_SPEAKER) |
    (1ULL << GPIO_TREE)    |
    (1ULL << GPIO_SPI_CS)  |
    (1ULL << GPIO_SPI_SCK) |
    (1ULL << GPIO_SPI_DO);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;

  gpio_config(&io_conf);

  // GPIO_NUM_0: Speaker.
  // GPIO_NUM_4: /CS
  // GPIO_NUM_5: DI
  // GPIO_NUM_6: SCK
  // GPIO_NUM_7: DO
  gpio_set_level(GPIO_SPEAKER, 0);
  gpio_set_level(GPIO_TREE,    0);
  gpio_set_level(GPIO_SPI_CS,  1);
  gpio_set_level(GPIO_SPI_DI,  0);
  gpio_set_level(GPIO_SPI_SCK, 0);
  gpio_set_level(GPIO_SPI_DO,  0);

  memset(&io_conf, 0, sizeof(io_conf));

  // Set GPIO_LASER as input.
  io_conf.intr_type    = GPIO_INTR_DISABLE;
  io_conf.mode         = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = (1ULL << GPIO_LASER);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en   = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
}

void RaceTrack::timer_init(int resolution, gptimer_alarm_cb_t timer_callback)
{
  gptimer_config_t timer_config = { };
  timer_config.clk_src       = GPTIMER_CLK_SRC_DEFAULT;
  timer_config.direction     = GPTIMER_COUNT_UP;
  timer_config.resolution_hz = TIMER_RESOLUTION_HZ;

  ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));

  timer_user_data.interrupt_count = 0;
  timer_user_data.count_value     = 0;
  timer_user_data.race_track      = this;

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

void RaceTrack::spi_init()
{
  spi_bus_config_t spi_bus_config = { };
  spi_bus_config.sclk_io_num     = GPIO_SPI_SCK;
  spi_bus_config.mosi_io_num     = GPIO_SPI_DO;
  spi_bus_config.miso_io_num     = GPIO_SPI_DI;
#if 0
  spi_bus_config.data0_io_num    = -1;
  spi_bus_config.data1_io_num    = -1;
  spi_bus_config.data2_io_num    = -1;
  spi_bus_config.data3_io_num    = -1;
  spi_bus_config.data4_io_num    = -1;
  spi_bus_config.data5_io_num    = -1;
  spi_bus_config.data6_io_num    = -1;
  spi_bus_config.data7_io_num    = -1;
#endif
  spi_bus_config.quadwp_io_num   = -1;
  spi_bus_config.quadhd_io_num   = -1;
  spi_bus_config.max_transfer_sz = 1;

  spi_bus_initialize(SPI2_HOST, &spi_bus_config, SPI_DMA_CH_AUTO);

  spi_device_interface_config_t spi_dev_config = { };
  spi_dev_config.spics_io_num   = -1;
  spi_dev_config.command_bits   = 0;
  spi_dev_config.address_bits   = 0;
  spi_dev_config.mode           = 0;
  spi_dev_config.queue_size     = 7;
  //spi_dev_config.clock_source   = SPI_CLK_SRC_DEFAULT;
  spi_dev_config.clock_speed_hz = 100000;

  spi_bus_add_device(SPI2_HOST, &spi_dev_config, &spi_handle);
}

void RaceTrack::spi_send(uint8_t ch)
{
  //spi_send_sw(ch);
  //return;

  spi_transaction_t trans_desc = { };
  trans_desc.cmd = 0;
  trans_desc.length = 8;
  trans_desc.tx_buffer = &ch;
  //trans_desc.tx_data[0] = ch;
  //trans_desc.tx_data[1] = ch;
  //trans_desc.tx_data[2] = ch;
  //trans_desc.tx_data[3] = ch;

  spi_device_transmit(spi_handle, &trans_desc);
}

void RaceTrack::spi_send_sw(uint8_t ch)
{
  int i;
  int data = ch & 0xff;

  for (i = 0; i < 8; i++)
  {
    if ((data & 0x80) != 0)
    {
      gpio_set_level(GPIO_SPI_DO,  1);
    }

    data = data << 1;

    gpio_set_level(GPIO_SPI_SCK, 1);
    ets_delay_us(10);
    gpio_set_level(GPIO_SPI_SCK, 0);

    gpio_set_level(GPIO_SPI_DO, 0);
  }
}

void RaceTrack::display_clear()
{
  gpio_set_level(GPIO_SPI_CS, 0);

  // 0x7c, 0x2d: Clear.
  spi_send('|');
  spi_send(0x2d);

  gpio_set_level(GPIO_SPI_CS, 1);
}

void RaceTrack::display_show(const char *text)
{
  gpio_set_level(GPIO_SPI_CS, 0);

  while (*text != 0)
  {
    spi_send(*text);
    text++;
  }

  gpio_set_level(GPIO_SPI_CS, 1);
}

void RaceTrack::display_show(int value)
{
  gpio_set_level(GPIO_SPI_CS, 0);

  char digits[8];
  int ptr = 0;

  while (value > 0)
  {

    digits[ptr++] = value % 10;

    value = value / 10;
  }

  if (ptr == 0) { digits[ptr++] = 0; }

  while (ptr > 0)
  {
    spi_send(digits[--ptr] + '0');
  }

  gpio_set_level(GPIO_SPI_CS, 1);
}

void RaceTrack::display_update()
{
  char temp[16];

  display_clear();

  for (int n = 0; n < 3; n++)
  {
    sprintf(temp, "Lap %d: ", n);
    display_show(temp);

    if (lap_number > n || lap_number == -1)
    {
      if (lap_times[n] != 0)
      {
        sprintf(temp, "%.2f", (float)lap_times[n] / 100);
        display_show(temp);
      }
    }

    display_show("\r");
  }

  if (lap_number != -1)
  {
    display_show("--");
  }
    else
  {
    if (lap_final != 0)
    {
      display_show("Final: ");
      sprintf(temp, "%.2f", (float)lap_final / 100);
      display_show(temp);
    }
  }
}

void RaceTrack::display_set_color(int r, int g, int b)
{
  if (r > 29) { r = 29; }
  if (g > 29) { g = 29; }
  if (b > 29) { b = 29; }

  gpio_set_level(GPIO_SPI_CS, 0);

  spi_send('|');
  spi_send(0x80 + r);
  spi_send('|');
  spi_send(0x9e + g);
  spi_send('|');
  spi_send(0xbc + b);

#if 0
  spi_send('|');
  spi_send('+');
  spi_send(r);
  spi_send(g);
  spi_send(b);
  spi_send(0);
#endif

  gpio_set_level(GPIO_SPI_CS, 1);
}

void RaceTrack::set_tone(int frequency)
{
  if (frequency == 0)
  {
    // Turn off sound.
    gpio_set_level(GPIO_SPEAKER, 0);
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
  ledc_channel.gpio_num   = GPIO_SPEAKER;
  ledc_channel.duty       = frequency / 2;
  ledc_channel.hpoint     = 0;

  ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));

  gpio_set_level(GPIO_SPEAKER, 1);
}

void RaceTrack::play_song_finish()
{
  // G4 B4 D5 B4 G5
  set_tone(392);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(494);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(587);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(494);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(784);
  vTaskDelay(100 / portTICK_PERIOD_MS);
  set_tone(0);
}

void RaceTrack::tree_pulse(int value)
{
  ticks = 0;
  //value = value / 10;

  gpio_set_level(GPIO_TREE, 1);
  //while (ticks < value);
  vTaskDelay(value / portTICK_PERIOD_MS);
  gpio_set_level(GPIO_TREE, 0);
}

void RaceTrack::tree_set_start()
{
  // 100ms: Top light (start).
  tree_pulse(100);
}

void RaceTrack::tree_set_drop()
{
  // 50ms: Drop by 1 light, ignore if already red.
  tree_pulse(60);
}

void RaceTrack::tree_set_red()
{
  // 150ms: Red light.
  tree_pulse(150);
}

void RaceTrack::tree_set_blank()
{
  // 200ms: All off.
  tree_pulse(200);
}

bool RaceTrack::start_game()
{
  tree_set_blank();

  for (int i = 0; i < 3; i++)
  {
    set_tone(440);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    set_tone(0);

    switch (i)
    {
      case 0:
        vTaskDelay(400 / portTICK_PERIOD_MS);
        tree_set_start();
        break;
      //case 1:
      //  vTaskDelay(400 / portTICK_PERIOD_MS);
      //  break;
      default:
        vTaskDelay(450 / portTICK_PERIOD_MS);
        tree_set_drop();
        break;
    }
  }

  running = true;
  lap_final = 0;
  ticks = 0;

  if (gpio_get_level(GPIO_LASER) == 1)
  {
    set_tone(880);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    set_tone(0);
  }
    else
  {
    tree_set_red();
    return false;
  }

  vTaskDelay(3000 / portTICK_PERIOD_MS);

  return true;
}

void RaceTrack::run_game()
{
  ESP_LOGI(TAG, "run_game()");

  memset(lap_times, 0, sizeof(lap_times));

  display_update();
  lap_number = 0;

  bool is_okay = start_game();

  while (is_okay)
  {
    if (gpio_get_level(GPIO_LASER) == 0)
    {
      ESP_LOGI(TAG, "No laser");
      lap_times[lap_number] = ticks;
      ticks = 0;
      lap_number++;

      if (lap_number == 3) { break; }

      display_update();
      vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
  }

  lap_number = -1;
  running = false;

  display_update();

  play_song_finish();
  tree_set_red();
}

bool IRAM_ATTR RaceTrack::timer_callback(
  gptimer_handle_t timer,
  const gptimer_alarm_event_data_t *edata,
  void *args)
{
  TimerUserData *timer_user_data = (TimerUserData *)args;
  timer_user_data->interrupt_count++;
  timer_user_data->count_value = edata->count_value;

  RaceTrack *race_track = (RaceTrack *)timer_user_data->race_track;

  race_track->ticks += 1;
  if (race_track->running) { race_track->lap_final++; }

  return pdFALSE;
}

const char *RaceTrack::TAG = "TRACK";

