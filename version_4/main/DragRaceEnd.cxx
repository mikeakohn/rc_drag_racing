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
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/spi_common.h"
#include "soc/gpio_reg.h"
#include "rom/ets_sys.h"
#include "esp_log.h"

#include "defines.h"
#include "DragRaceEnd.h"

DragRaceEnd::DragRaceEnd() :
  spi_handle { 0 },
  need_update_display { true },
  is_connected { false }
{
  network = new NetworkEnd();
}

DragRaceEnd::~DragRaceEnd()
{
  delete network;
}

void DragRaceEnd::run()
{
  network->start();

  ESP_LOGI(
    "run",
    "drag_race=%p",
    this);

  init_gpio();
  init_laser_sensor();
  init_spi();

  // Interrupt every 1/100th of a second.
  init_timer(10000, timer_callback);
  ESP_ERROR_CHECK(gptimer_start(gptimer));

  while (true)
  {
    if (need_update_display)
    {
      if (network->is_connected() && is_connected == false)
      {
        is_connected = true;
        clear_displays();
      }

      need_update_display = false;
      update_display();
    }

    int value = detect_laser();

    if (right.running)
    {
      if ((value & 1) != 0)
      {
        right.running = false;
        network->set_right_value(right.value);
      }
    }

    if (left.running)
    {
      if ((value & 2) != 0)
      {
        left.running = false;
        network->set_left_value(left.value);
      }
    }
  }
}

void DragRaceEnd::init_gpio()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Setup SPI chip select.
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_OUTPUT_SPI_CS_RIGHT) |
    (1ULL << GPIO_OUTPUT_SPI_CS_LEFT);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  gpio_set_level(GPIO_OUTPUT_SPI_CS_RIGHT, 1);
  gpio_set_level(GPIO_OUTPUT_SPI_CS_LEFT,  1);
}

void DragRaceEnd::init_spi_sw()
{
  // Zero-initialize the config structure.
  gpio_config_t io_conf = { };

  // Setup software SPI.
  io_conf.intr_type = GPIO_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pin_bit_mask =
    (1ULL << GPIO_OUTPUT_SPI_SCLK) |
    (1ULL << GPIO_OUTPUT_SPI_MOSI);
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);

  gpio_set_level(GPIO_OUTPUT_SPI_SCLK, 0);
  gpio_set_level(GPIO_OUTPUT_SPI_MOSI,  0);
}

void DragRaceEnd::init_spi_hw()
{
  spi_bus_config_t spi_bus_config = { };
  spi_bus_config.sclk_io_num     = GPIO_OUTPUT_SPI_SCLK;
  spi_bus_config.mosi_io_num     = GPIO_OUTPUT_SPI_MOSI;
  spi_bus_config.miso_io_num     = -1;
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
  spi_dev_config.clock_speed_hz = 250000;

  spi_bus_add_device(SPI2_HOST, &spi_dev_config, &spi_handle);
}

void DragRaceEnd::send_spi_sw(char ch)
{
  int i;
  int data = ch & 0xff;

  for (i = 0; i < 8; i++)
  {
    if ((data & 0x80) != 0)
    {
      gpio_set_level(GPIO_OUTPUT_SPI_MOSI,  1);
    }

    data = data << 1;

    gpio_set_level(GPIO_OUTPUT_SPI_SCLK, 1);
    ets_delay_us(10);
    gpio_set_level(GPIO_OUTPUT_SPI_SCLK, 0);

    gpio_set_level(GPIO_OUTPUT_SPI_MOSI, 0);

  }
}

void DragRaceEnd::send_spi_hw(char ch)
{
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

void DragRaceEnd::clear_displays()
{
  gpio_set_level(GPIO_OUTPUT_SPI_CS_RIGHT, 0);
  gpio_set_level(GPIO_OUTPUT_SPI_CS_LEFT,  0);

  // 0x76: Clear.
  send_spi(0x76);

  gpio_set_level(GPIO_OUTPUT_SPI_CS_RIGHT, 1);
  gpio_set_level(GPIO_OUTPUT_SPI_CS_LEFT,  1);
}

void DragRaceEnd::display_digits(int value)
{
  char digits[4];

  value = value / 10;

  for (int n = 3; n >= 0; n--)
  {
    if (value == 0)
    {
      digits[n] = n >= 2 ? 0 : ' ';
    }
      else
    {
      digits[n] = value % 10;
    }

    value = value / 10;
  }

  for (int n = 0; n < 4; n++)
  {
    send_spi(digits[n]);
  }
}

void DragRaceEnd::display_time_right()
{
  gpio_set_level(GPIO_OUTPUT_SPI_CS_RIGHT, 0);

  if (right.enabled)
  {
    if (right.value != right.last_value)
    {
      // 0x76: Clear.
      send_spi(0x76);

      // 0x77 0x04: Turn on dot at 10th's of a second.
      send_spi(0x77);
      send_spi(4);

      display_digits(right.value);
      right.last_value = right.value;
    }
  }
    else
  {
    if (right.last_value != -1)
    {
      for (int n = 0; n < 4; n++) { send_spi(' '); }
    }
  }

  gpio_set_level(GPIO_OUTPUT_SPI_CS_RIGHT, 1);
}

void DragRaceEnd::display_time_left()
{
  gpio_set_level(GPIO_OUTPUT_SPI_CS_LEFT,  0);

  if (left.enabled)
  {
    if (left.value != left.last_value)
    {
      // 0x76: Clear.
      send_spi(0x76);

      // 0x77 0x04: Turn on dot at 10th's of a second.
      send_spi(0x77);
      send_spi(4);

      display_digits(left.value);
      left.last_value = left.value;
    }
  }
    else
  {
    if (left.last_value != -1)
    {
      for (int n = 0; n < 4; n++) { send_spi(' '); }
    }
  }

  gpio_set_level(GPIO_OUTPUT_SPI_CS_LEFT,  1);
}

void DragRaceEnd::display_not_connected()
{
  gpio_set_level(GPIO_OUTPUT_SPI_CS_RIGHT,  0);
  // 0x76: Clear.
  send_spi(0x76);

  for (int n = 0; n < 4; n++) { send_spi('9'); }

  gpio_set_level(GPIO_OUTPUT_SPI_CS_RIGHT,  1);

  gpio_set_level(GPIO_OUTPUT_SPI_CS_LEFT,  0);
  // 0x76: Clear.
  send_spi(0x76);

  for (int n = 0; n < 4; n++) { send_spi('9'); }

  gpio_set_level(GPIO_OUTPUT_SPI_CS_LEFT,  1);
}

void DragRaceEnd::update_display()
{
  if (network->is_connected())
  {
    if (network->is_right_fault())
    {
      right.fault();
    }

    if (right.running) { right.value++; }
    display_time_right();

    if (network->is_left_fault())
    {
      left.fault();
    }

    if (left.running) { left.value++; }
    display_time_left();
  }
    else
  {
    display_not_connected();
    right.value = 0;
    right.last_value = -1;
    left.value = 0;
    left.last_value = -1;
  }
}

bool IRAM_ATTR DragRaceEnd::timer_callback(
  gptimer_handle_t timer,
  const gptimer_alarm_event_data_t *edata,
  void *args)
{
  TimerUserData *timer_user_data = (TimerUserData *)args;
  timer_user_data->interrupt_count++;
  timer_user_data->count_value = edata->count_value;

  DragRaceEnd *drag_race = (DragRaceEnd *)timer_user_data->drag_race;

  if (drag_race->network->do_start_race())
  {
    drag_race->right.prepare();
    drag_race->left.prepare();
  }

  if (drag_race->network->do_start_timer())
  {
    drag_race->right.start();
    drag_race->left.start();
  }

  drag_race->need_update_display = true;

  return pdFALSE;
}

