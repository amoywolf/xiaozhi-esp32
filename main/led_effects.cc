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
#define SPEED_MIN 1
#define SPEED_MAX 10
#define SPEED_DEFAULT 5

static led_strip_handle_t s_strip = nullptr;
static std::atomic<LedScene> s_scene{LedScene::Off};
static std::atomic<int> s_brightness{MAX_BRIGHTNESS};
static std::atomic<int> s_speed{SPEED_DEFAULT};

static void set_all(uint8_t r, uint8_t g, uint8_t b) {
    int level = s_brightness.load(std::memory_order_relaxed);
    if (level < 0) level = 0;
    if (level > MAX_BRIGHTNESS) level = MAX_BRIGHTNESS;
    if (r > level) r = static_cast<uint8_t>(level);
    if (g > level) g = static_cast<uint8_t>(level);
    if (b > level) b = static_cast<uint8_t>(level);
    for (int i = 0; i < STRIP_LEDS; ++i) {
        led_strip_set_pixel(s_strip, i, r, g, b);
    }
    led_strip_refresh(s_strip);
}

static int scale_delay(int base_ms) {
    int speed = s_speed.load(std::memory_order_relaxed);
    if (speed < SPEED_MIN) {
        speed = SPEED_MIN;
    } else if (speed > SPEED_MAX) {
        speed = SPEED_MAX;
    }
    int scaled = base_ms * SPEED_DEFAULT / speed;
    if (scaled < 10) {
        scaled = 10;
    }
    return scaled;
}

static void do_party() {
    for (int i = 0; i < STRIP_LEDS; ++i) {
        if (s_scene.load(std::memory_order_relaxed) != LedScene::Party) {
            break;
        }
        led_strip_clear(s_strip);
        led_strip_set_pixel(s_strip, i, 6, 2, MAX_BRIGHTNESS);
        led_strip_refresh(s_strip);
        vTaskDelay(pdMS_TO_TICKS(scale_delay(30)));
    }
}

static void do_romantic() {
    set_all(6, 1, 5);
    vTaskDelay(pdMS_TO_TICKS(scale_delay(400)));
    set_all(2, 0, 2);
    vTaskDelay(pdMS_TO_TICKS(scale_delay(400)));
}

static void do_relax() {
    set_all(2, 1, 0);
    vTaskDelay(pdMS_TO_TICKS(scale_delay(500)));
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

LedScene led_effects_get_scene() {
    return s_scene.load(std::memory_order_relaxed);
}

void led_effects_set_brightness(int level) {
    if (level < 0) level = 0;
    if (level > MAX_BRIGHTNESS) level = MAX_BRIGHTNESS;
    s_brightness.store(level, std::memory_order_relaxed);
}

void led_effects_set_speed(int level) {
    if (level < SPEED_MIN) level = SPEED_MIN;
    if (level > SPEED_MAX) level = SPEED_MAX;
    s_speed.store(level, std::memory_order_relaxed);
}

int led_effects_get_brightness() {
    return s_brightness.load(std::memory_order_relaxed);
}

int led_effects_get_speed() {
    return s_speed.load(std::memory_order_relaxed);
}
