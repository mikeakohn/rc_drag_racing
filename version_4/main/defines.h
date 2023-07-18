#ifndef DEFINES_H
#define DEFINES_H

// Start Track:
// GPIO2  (J1  4): Laser sensor (right).
// GPIO3  (J1  5): Laser sensor (left).
// GPIO0  (J1  9): LED - debug sensor (right).
// GPIO1  (J1 10): LED - debug sensor (left).

// GPIO21 (J3  2): Tree RED left.
// GPIO20 (J3  3): Tree GREEN.
// GPIO9  (J3  5): Tree YELLOW bottom.
// GPIO7  (J3  8): Tree RED right.
// GPIO6  (J3  9): Tree YELLOW middle.
// GPIO5  (J3 10): Tree YELLOW top.
// GPIO4  (J3 11): Button (connect to GND to start the race).
// GPIO18 (J3 13): Audio output (for beeping sound).
// GPIO19 (J3 14): Leave open to configure for the start track.

// End Track:
// GPIO2  (J1  4): Laser sensor (right).
// GPIO3  (J1  5): Laser sensor (left).
// GPIO0  (J1  9): LED - debug sensor (right).
// GPIO1  (J1 10): LED - debug sensor (left).

// GPIO19 (J3 14): Connect to GND to configure for end track.

#define GPIO_OUTPUT_IO_0   GPIO_NUM_0
#define GPIO_OUTPUT_IO_1   GPIO_NUM_1
#define GPIO_OUTPUT_IO_18  GPIO_NUM_18
#define GPIO_OUTPUT_IO_21  GPIO_NUM_21
#define GPIO_OUTPUT_IO_20  GPIO_NUM_20
#define GPIO_OUTPUT_IO_10  GPIO_NUM_10
#define GPIO_OUTPUT_IO_9   GPIO_NUM_9
#define GPIO_OUTPUT_IO_7   GPIO_NUM_7
#define GPIO_OUTPUT_IO_6   GPIO_NUM_6
#define GPIO_OUTPUT_IO_5   GPIO_NUM_5

#define GPIO_OUTPUT_RED_LEFT       GPIO_NUM_21
#define GPIO_OUTPUT_GREEN          GPIO_NUM_10
#define GPIO_OUTPUT_YELLOW_BOTTOM  GPIO_NUM_9
#define GPIO_OUTPUT_RED_RIGHT      GPIO_NUM_7
#define GPIO_OUTPUT_YELLOW_MIDDLE  GPIO_NUM_6
#define GPIO_OUTPUT_YELLOW_TOP     GPIO_NUM_5

#define GPIO_OUTPUT_AUDIO          GPIO_NUM_18

#define GPIO_OUTPUT_DEBUG_RIGHT    GPIO_NUM_0
#define GPIO_OUTPUT_DEBUG_LEFT     GPIO_NUM_1

#define GPIO_OUTPUT_SPI_MOSI       GPIO_NUM_7
#define GPIO_OUTPUT_SPI_SCLK       GPIO_NUM_6
#define GPIO_OUTPUT_SPI_CS_RIGHT   GPIO_NUM_5
#define GPIO_OUTPUT_SPI_CS_LEFT    GPIO_NUM_4

#define GPIO_INPUT_IO_2            GPIO_NUM_2
#define GPIO_INPUT_IO_3            GPIO_NUM_3
#define GPIO_INPUT_IO_4            GPIO_NUM_4
#define GPIO_INPUT_IO_19           GPIO_NUM_19

#define GPIO_INPUT_SENSOR_RIGHT    GPIO_NUM_2
#define GPIO_INPUT_SENSOR_LEFT     GPIO_NUM_3
#define GPIO_INPUT_BUTTON          GPIO_NUM_4

#define GPIO_INPUT_TRACK_SELECT    GPIO_NUM_19

// Default settings for both start and end track circuits.
#if 0
#define GPIO_OUTPUT_PIN_SEL \
  (1ULL << GPIO_OUTPUT_IO_0) | \
  (1ULL << GPIO_OUTPUT_IO_1)

#define GPIO_INPUT_PIN_SEL \
  (1ULL << GPIO_INPUT_IO_2) | \
  (1ULL << GPIO_INPUT_IO_3) | \
  (1ULL << GPIO_INPUT_IO_19)
#endif

#define ESP_INTR_FLAG_DEFAULT 0

// Define the output GPIO
#define LEDC_OUTPUT_IO (0)

// Set duty to 50%. ((2 ** 13) - 1) * 50% = 4095
//#define LEDC_DUTY (4095)
#define LEDC_DUTY (0)

// Frequency in Hertz. Set frequency at 40 kHz.
#define LEDC_FREQUENCY (40000)

// 1MHz resolution.
#define TIMER_RESOLUTION_HZ   1000000

// Alarm period 0.5s.
#define TIMER_ALARM_PERIOD_S  0.5

#endif

