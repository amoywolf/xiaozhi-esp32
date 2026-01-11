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
