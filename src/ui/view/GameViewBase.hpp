#pragma once

#include "core/common.h"
#include "game/render/GLSLPParser.hpp"

#include <borealis.hpp>

#include <string>
#include <vector>

namespace beiklive
{
    class GameMenuView;
    class RewindSelectorView;

    struct RewindFrame {
        std::vector<uint8_t> state;
        std::vector<uint16_t> thumb;

        static constexpr unsigned THUMB_W = 120;
        static constexpr unsigned THUMB_H = 80;
    };

    struct RewindThumbSnapshot {
        int bufferIdx = 0;
        int secondsAgo = 0;
        std::vector<uint16_t> thumb;
    };

    class GameViewBase : public brls::Box
    {
    public:
        ~GameViewBase() override = default;

        virtual void prepareExitCleanup() = 0;
        virtual void setGameMenuView(GameMenuView* menuView) = 0;
        virtual void setRewindSelectorView(RewindSelectorView* view) = 0;

        virtual std::string getStatePath(int slot) const = 0;
        virtual std::string getStateThumbPath(int slot) const = 0;
        virtual bool stateExists(int slot) const = 0;
        virtual std::vector<RewindThumbSnapshot> snapshotRewindThumbs() const = 0;
        virtual void requestRestoreRewindFrame(int frameIndex) = 0;

        virtual void requestCheatPathUpdate(const std::string& path) = 0;
        virtual void applyCheatsUpdate(const std::vector<CheatEntry>& cheats) = 0;

        virtual void _onShaderToggle(bool on) = 0;
        virtual void _onShaderPathChange(const std::string& path) = 0;
        virtual void _onDisplayModeChange(const std::string& mode) = 0;
        virtual void _onIntegerScaleChange(float scale) = 0;
        virtual void _onCustomValuesChanged(float x, float y, float scale) = 0;
        virtual void _onOverlayToggle(bool enabled) = 0;
        virtual void _onOverlayPathChange(const std::string& path) = 0;
        virtual void _onNdsLayoutChange(const std::string& layout) = 0;
        virtual void _onNdsScreenOrientationChange(const std::string& orientation) = 0;
        virtual void _onNdsScreenValuesChanged(bool topScreen, float x, float y, float scale) = 0;
        virtual void _onNdsIntegerScaleChange(bool enabled) = 0;
        virtual void _onNdsInternalResolutionChange(int scale) = 0;
        virtual void _onFilterChange(const std::string& filter) = 0;
        virtual void _onConfigUpdated() = 0;
        virtual std::vector<ShaderParamInfo> _getShaderParams() const = 0;
        virtual void _setShaderParam(const std::string& name, float val) = 0;
    };
}
