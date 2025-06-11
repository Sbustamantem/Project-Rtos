#ifndef UART_LIB_H
#define UART_LIB_H
#include "driver/gpio.h"

void uart_init(void);
int uart_read_string(char *buffer, size_t bufsize, int timeout_ms);
#endif // UART_LIB_H