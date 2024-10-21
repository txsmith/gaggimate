#ifndef COMMON_UART_H
#define COMMON_UART_H

#include <stddef.h>
#include <stdint.h>

#define UART_BAUD_RATE 115200

// Initialize UART
void uart_init(void);

// Send data over UART
void uart_send(uint8_t *data, size_t length);

// Receive data from UART (blocking call)
int uart_receive(uint8_t *buffer, size_t buffer_size);

#endif // COMMON_UART_H