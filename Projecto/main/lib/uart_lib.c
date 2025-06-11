#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include <stdlib.h>
#include <stdio.h>
#include "freertos/FreeRTOSConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"
#include "uart_lib.h"
#include <portmacro.h>


#define UART_PORT_NUM      UART_NUM_1
#define UART_BAUD_RATE     115200
#define UART_TX_PIN        GPIO_NUM_17
#define UART_RX_PIN        GPIO_NUM_16
#define UART_BUF_SIZE      1024

void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uint8_t *data = (uint8_t *) malloc(UART_BUF_SIZE);
    // Configure UART parameters
    uart_param_config(UART_PORT_NUM, &uart_config);
    // Set UART pins
    uart_set_pin(UART_PORT_NUM, UART_TX_PIN, UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    // Install UART driver
    uart_driver_install(UART_PORT_NUM, UART_BUF_SIZE * 2, 0, 0, NULL, 0);
}  
int uart_read_string(char *buffer, size_t bufsize, int timeout_ms)
{
    int len = uart_read_bytes(UART_PORT_NUM, (uint8_t *)buffer, bufsize - 1, timeout_ms / portTICK_PERIOD_MS);
    if (len > 0) {
        buffer[len] = '\0'; // Null-terminate for string use
    }
    return len;
}
//     uart_set_pin(port, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);