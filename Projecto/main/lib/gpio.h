#ifndef GPIO_H
#define GPIO_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/**
 *  Configure a GPIO pin for input or output, with optional pull-up/down and open-drain.
 *
 *  io_num      GPIO number (e.g. GPIO_NUM_0 … GPIO_NUM_39)
 *  is_input    true→input mode; false→output mode
 *  pull_up     true→enable pull-up; false→enable pull-down
 *  open_drain  true→open-drain (only when output); false→push-pull
 */
void io_config(gpio_num_t io_num,
                        bool       is_input,
                        bool       pull_up,
                        bool       open_drain);

/**
 *  Same as gpio_lib_io_config(), plus attach an interrupt handler.
 *
 *  io_num      GPIO number
 *  is_input    true→input mode; false→output mode
 *  pull_up     true→enable pull-up; false→enable pull-down
 *  open_drain  true→open-drain (only when output); false→push-pull
 *  isr_type    GPIO_INTR_… type for triggering
 */
void isr_io_config(gpio_num_t      io_num,
                            bool            is_input,
                            bool            pull_up,
                            bool            open_drain,
                            gpio_int_type_t isr_type);

/**
 *  Checks if a debounce time has passed since the last event.
 *
 *  current_time_ticks The current FreeRTOS tick count.
 *  last_event_time_ticks The FreeRTOS tick count of the last event.
 *  debounce_time_ms The debounce time in milliseconds.
 *  true If the debounce time has passed.
 *  false Otherwise.
 */
bool is_debounced(TickType_t current_time_ticks, TickType_t last_event_time_ticks, int debounce_time_ms);


/**
 *  Initialize uart driver, configuring uart basics
 *
 *  port The current UART port.
 *  baud_rate transmission speed between the 2 devices.
 *  queue Used as a good practice, each uart has it's own queue.
 *  buffer_size message RX length
 */
void init_uart(uart_port_t port, uint32_t baud_rate, QueueHandle_t *queue, uint32_t buffer_size);

#endif // GPIO_LIB_H