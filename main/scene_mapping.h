#pragma once

#include <string>
#include "led_effects.h"

LedScene map_scene_from_text(const std::string& text);
const char* scene_name(LedScene scene);
