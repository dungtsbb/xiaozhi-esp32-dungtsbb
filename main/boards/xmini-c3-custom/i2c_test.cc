// main/boards/xmini-c3-custom/i2c_test.cc
#ifdef ENABLE_I2C_TEST

#include "freertos/FreeRTOS.h"
#include <driver/i2c_master.h>
#include <esp_log.h>
#include "config.h"

static const char* TAG = "I2C_TEST";

void TestI2cProbe(i2c_master_bus_handle_t bus) {
    const uint8_t probe_addr = 0x2D;  // thay đổi nếu bạn cần địa chỉ khác
    esp_err_t err = i2c_master_probe(bus, probe_addr, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Probe OK at 0x%02X", probe_addr);
    } else {
        ESP_LOGW(TAG, "Probe FAIL at 0x%02X, err=0x%x", probe_addr, err);
    }
}

#endif  // ENABLE_I2C_TEST
