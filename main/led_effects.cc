#include "led_effects.h"

#include <atomic>

#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <led_strip.h>

#define TAG "led_effects"
#define STRIP_GPIO GPIO_NUM_18
#define STRIP_LEDS 60
#define MAX_BRIGHTNESS 8

static led_strip_handle_t s_strip = nullptr;
static std::atomic<LedScene> s_scene{LedScene::Off};

static void set_all(uint8_t r, uint8_t g, uint8_t b) {
    if (r > MAX_BRIGHTNESS) r = MAX_BRIGHTNESS;
    if (g > MAX_BRIGHTNESS) g = MAX_BRIGHTNESS;
    if (b > MAX_BRIGHTNESS) b = MAX_BRIGHTNESS;
    for (int i = 0; i < STRIP_LEDS; ++i) {
        led_strip_set_pixel(s_strip, i, r, g, b);
    }
    led_strip_refresh(s_strip);
}

static void do_party() {
    for (int i = 0; i < STRIP_LEDS; ++i) {
        if (s_scene.load(std::memory_order_relaxed) != LedScene::Party) {
            break;
        }
        led_strip_clear(s_strip);
        led_strip_set_pixel(s_strip, i, 6, 2, MAX_BRIGHTNESS);
        led_strip_refresh(s_strip);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}

static void do_romantic() {
    set_all(6, 1, 5);
    vTaskDelay(pdMS_TO_TICKS(400));
    set_all(2, 0, 2);
    vTaskDelay(pdMS_TO_TICKS(400));
}

static void do_relax() {
    set_all(2, 1, 0);
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void led_task(void* arg) {
    while (true) {
        switch (s_scene.load(std::memory_order_relaxed)) {
            case LedScene::Off:
                led_strip_clear(s_strip);
                vTaskDelay(pdMS_TO_TICKS(200));
                break;
            case LedScene::Party:
                do_party();
                break;
            case LedScene::Romantic:
                do_romantic();
                break;
            case LedScene::Relax:
                do_relax();
                break;
        }
    }
}

void led_effects_init() {
    if (s_strip) {
        return;
    }

    led_strip_config_t strip_config = {};
    strip_config.strip_gpio_num = STRIP_GPIO;
    strip_config.max_leds = STRIP_LEDS;
    strip_config.color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB;
    strip_config.led_model = LED_MODEL_WS2812;

    led_strip_rmt_config_t rmt_config = {};
    rmt_config.resolution_hz = 10 * 1000 * 1000;

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip);
    if (err != ESP_OK || s_strip == nullptr) {
        ESP_LOGE(TAG, "led_strip_new_rmt_device failed: %s", esp_err_to_name(err));
        return;
    }

    led_strip_clear(s_strip);
    s_scene.store(LedScene::Relax, std::memory_order_relaxed);
    xTaskCreate(led_task, "led_effects", 4096, nullptr, 2, nullptr);
}

void led_effects_set_scene(LedScene scene) {
    s_scene.store(scene, std::memory_order_relaxed);
}
