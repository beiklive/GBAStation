#pragma once

#include <string>
#include <cstdint>

enum class PlatformBadgeColor {
    GBA,
    GBC,
    GB,
    NES,
    SNES,
    NDS,
    THREEDS,
    GENESIS,
    ARCADE,
    DREAMCAST,
    NONE,
};

struct GridDrawItem {
    uint64_t gameId = 0;

    bool populated = false;
    bool empty = true;

    std::string title;
    std::string subText;
    std::string playTime;

    PlatformBadgeColor badgeColor = PlatformBadgeColor::NONE;
    std::string badgeText;

    std::string imagePath;
    std::string imageLayerPath;
    std::string platformImagePath;
    std::string platformImageSourcePath;
    bool imageLayerVisible = false;
    float coverAspect = 1.f;

    bool favorite = false;

    float marqueeOffset = 0.f;
    float marqueeMaxOffset = 0.f;
    float focusScale = 1.f;
    float focusGlow = 0.f;

    int textureHandle = -1;
    int platformTextureHandle = -1;
    int imageLayerHandle = -1;
    bool textureLoading = false;
    bool textureReady = false;
    bool textureFailed = false;
    bool platformTextureReady = false;
    bool platformTextureFailed = false;
    bool imageLayerFailed = false;

    bool selected = false;

    void reset() {
        populated = false;
        empty = true;
        title.clear();
        subText.clear();
        playTime.clear();
        badgeColor = PlatformBadgeColor::NONE;
        badgeText.clear();
        imagePath.clear();
        imageLayerPath.clear();
        platformImagePath.clear();
        platformImageSourcePath.clear();
        imageLayerVisible = false;
        coverAspect = 1.f;
        favorite = false;
        marqueeOffset = 0.f;
        marqueeMaxOffset = 0.f;
        focusScale = 1.f;
        focusGlow = 0.f;
        textureHandle = -1;
        platformTextureHandle = -1;
        imageLayerHandle = -1;
        textureLoading = false;
        textureReady = false;
        textureFailed = false;
        platformTextureReady = false;
        platformTextureFailed = false;
        imageLayerFailed = false;
        selected = false;
    }
};
