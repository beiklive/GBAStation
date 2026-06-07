#pragma once

#include <string>
#include <cstdint>

enum class PlatformBadgeColor {
    GBA,
    GBC,
    GB,
    NES,
    SNES,
    GENESIS,
    NONE,
};

struct GridDrawItem {
    uint64_t gameId = 0;

    bool empty = true;

    std::string title;
    std::string subText;
    std::string playTime;

    PlatformBadgeColor badgeColor = PlatformBadgeColor::NONE;
    std::string badgeText;

    std::string imagePath;
    std::string imageLayerPath;
    bool imageLayerVisible = false;

    bool favorite = false;

    float marqueeOffset = 0.f;
    float marqueeMaxOffset = 0.f;
    float focusScale = 1.f;
    float focusGlow = 0.f;

    int textureHandle = -1;
    int imageLayerHandle = -1;
    bool textureLoading = false;
    bool textureReady = false;

    bool selected = false;

    void reset() {
        empty = true;
        title.clear();
        subText.clear();
        playTime.clear();
        badgeColor = PlatformBadgeColor::NONE;
        badgeText.clear();
        imagePath.clear();
        imageLayerPath.clear();
        imageLayerVisible = false;
        favorite = false;
        marqueeOffset = 0.f;
        marqueeMaxOffset = 0.f;
        focusScale = 1.f;
        focusGlow = 0.f;
        textureLoading = false;
        textureReady = false;
        selected = false;
    }
};
