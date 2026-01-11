#include "scene_mapping.h"

static bool contains(const std::string& text, const char* key) {
    return text.find(key) != std::string::npos;
}

LedScene map_scene_from_text(const std::string& text) {
    if (contains(text, "关灯") || contains(text, "关闭灯光")) {
        return LedScene::Off;
    }
    if (contains(text, "恢复正常") || contains(text, "默认")) {
        return LedScene::Relax;
    }
    if (contains(text, "派对") || contains(text, "party")) {
        return LedScene::Party;
    }
    if (contains(text, "浪漫")) {
        return LedScene::Romantic;
    }
    if (contains(text, "睡前") || contains(text, "放松") || contains(text, "relax")) {
        return LedScene::Relax;
    }
    return LedScene::Relax;
}

const char* scene_name(LedScene scene) {
    switch (scene) {
        case LedScene::Off:
            return "Scene: Off";
        case LedScene::Party:
            return "Scene: Party";
        case LedScene::Romantic:
            return "Scene: Romantic";
        case LedScene::Relax:
            return "Scene: Relax";
        default:
            return "Scene: Unknown";
    }
}
