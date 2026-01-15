#pragma once

#include <stdint.h>

enum class LedScene : uint8_t {
    Off = 0,
    Party,
    Romantic,
    Relax
};

void led_effects_init();
void led_effects_set_scene(LedScene scene);
LedScene led_effects_get_scene();
void led_effects_set_brightness(int level);
void led_effects_set_speed(int level);
int led_effects_get_brightness();
int led_effects_get_speed();
