#include "board.h"
#include "display.h"
#include "led/circular_strip.h"

#include <esp_log.h>
#include <led_strip.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "hw_smoke_test"
#define STRIP_GPIO GPIO_NUM_18
#define STRIP_LEDS 60

static void hw_smoke_test_task(void* arg) {
    ESP_LOGI(TAG, "HW smoke test start");

    auto display = Board::GetInstance().GetDisplay();
    if (display) {
        display->ShowNotification("LED TEST");
        display->SetChatMessage("system", "GPIO18 WS2812B");
    }

    ESP_LOGI(TAG, "Init strip on GPIO18, leds=60");
    led_strip_handle_t strip = nullptr;
    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = STRIP_GPIO;
    strip_config.max_leds = STRIP_LEDS;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
    strip_config.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000;

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &strip);
    if (err != ESP_OK || strip == nullptr) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %s", esp_err_to_name(err));
        vTaskDelete(nullptr);
        return;
    }
    // Clear entire strip so no stale pixels remain lit.
    led_strip_clear(strip);

    ESP_LOGI(TAG, "Set RED");
    for (int i = 0; i < STRIP_LEDS; ++i) {
        led_strip_set_pixel(strip, i, 4, 0, 0);
    }
    led_strip_refresh(strip);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Set GREEN");
    for (int i = 0; i < STRIP_LEDS; ++i) {
        led_strip_set_pixel(strip, i, 0, 4, 0);
    }
    led_strip_refresh(strip);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Set BLUE");
    for (int i = 0; i < STRIP_LEDS; ++i) {
        led_strip_set_pixel(strip, i, 0, 0, 4);
    }
    led_strip_refresh(strip);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Chase WHITE");
    for (int i = 0; i < STRIP_LEDS; ++i) {
        led_strip_clear(strip);
        led_strip_set_pixel(strip, i, 2, 2, 2);
        led_strip_refresh(strip);
        vTaskDelay(pdMS_TO_TICKS(40));
    }

    ESP_LOGI(TAG, "Set warm dim");
    for (int i = 0; i < STRIP_LEDS; ++i) {
        led_strip_set_pixel(strip, i, 3, 2, 1);
    }
    led_strip_refresh(strip);
    vTaskDelay(pdMS_TO_TICKS(1000));

    ESP_LOGI(TAG, "Turn off");
    led_strip_clear(strip);

    if (display) {
        display->ShowNotification("LED TEST OK");
        display->SetChatMessage("system", "LED TEST OK");
    }

    led_strip_del(strip);
    ESP_LOGI(TAG, "HW smoke test done");
    vTaskDelete(nullptr);
}

void hw_smoke_test_start() {
    xTaskCreate(hw_smoke_test_task, "hw_smoke_test", 4096, nullptr, 2, nullptr);
}
