#ifdef ARDUINO_ARCH_STM32

#include "common_uart.h"
#include "stm32f4xx_hal.h"

UART_HandleTypeDef huart2;

void MX_USART2_UART_Init(void)
{
    huart2.Instance = USART2;
    huart2.Init.BaudRate = UART_BAUD_RATE;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

void uart_init(void) {
    MX_USART2_UART_Init();
}

void uart_send(uint8_t *data, size_t length) {
    HAL_UART_Transmit(&huart2, data, length, HAL_MAX_DELAY);
}

int uart_receive(uint8_t *buffer, size_t buffer_size) {
    // Receive data and check how many bytes were received
    HAL_UART_Receive(&huart2, buffer, buffer_size, HAL_MAX_DELAY);

    // Check if any data was received
    if (buffer[0] != 0) { // Simple check assuming non-zero means data is present
        return 1; // Indicate that data was received
    }
    return 0; // Indicate no data received
}

#endif

#ifdef ARDUINO_ARCH_ESP32

#include "common_uart.h"
#include "driver/uart.h"

#define UART_NUM UART_NUM_1

void uart_init(void) {
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM, &uart_config);
    uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0);
}

void uart_send(uint8_t *data, size_t length) {
    uart_write_bytes(UART_NUM, (const char*)data, length);
}

int uart_receive(uint8_t *buffer, size_t buffer_size) {
    // Read data from UART and return the number of bytes received
    int len = uart_read_bytes(UART_NUM, buffer, buffer_size, portMAX_DELAY);

    // Check if any data was received
    if (len > 0) {
        return 1; // Indicate that data was received
    }
    return 0; // Indicate no data received
}
#endif