// main/boards/xmini-c3-custom/uart1_test.cc
#ifdef ENABLE_UART1_TEST

#include "config.h"
#include <driver/uart.h>
#include <esp_log.h>
#include <cstring>

static const char* TAG = "UART1_TEST";

void TestUart1Loopback() {
    const uart_port_t port = UART_NUM_1;
    const int baud = 115200;
    const int buf_len = 256;

    uart_config_t cfg = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // Install driver (RX/TX buffer), configure and set pins
    ESP_ERROR_CHECK(uart_driver_install(port, buf_len, buf_len, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(port, &cfg));
    ESP_ERROR_CHECK(uart_set_pin(port, U1TXD, U1RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // Send test data
    const char* msg = "UART1 loopback test\r\n";
    ESP_ERROR_CHECK(uart_write_bytes(port, msg, strlen(msg)));

    // Read back (need TX↔RX wired for loopback)
    uint8_t rx[buf_len] = {0};
    int len = uart_read_bytes(port, rx, sizeof(rx), pdMS_TO_TICKS(200));
    if (len > 0) {
        ESP_LOGI(TAG, "Received (%d): %.*s", len, len, (char*)rx);
    } else {
        ESP_LOGW(TAG, "No data received");
    }

    // Cleanup if only testing
    uart_driver_delete(port);
}

#endif  // ENABLE_UART1_TEST
