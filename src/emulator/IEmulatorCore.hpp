#pragma once

#include "core/common.h"
#include "game/retro/LibretroLoader.hpp"

namespace beiklive {

struct IEmulatorCore {
    virtual ~IEmulatorCore() = default;

    virtual bool SetupGame(beiklive::GameEntry GameEntry) = 0;
    virtual void Cleanup() = 0;
    virtual void RunFrame() = 0;
    virtual void Reset() = 0;

    virtual bool Serialize(std::vector<uint8_t>& outBuf) const = 0;
    virtual bool Unserialize(const std::vector<uint8_t>& buf) = 0;

    virtual LibretroLoader::VideoFrame GetVideoFrame() const = 0;
    virtual bool DrainAudio(std::vector<int16_t>& out) = 0;

    virtual void SetButtonState(unsigned id, bool pressed) = 0;
    virtual void SetButtonsFromSignal() = 0;

    virtual unsigned GameWidth()  const = 0;
    virtual unsigned GameHeight() const = 0;
    virtual double   Fps()        const = 0;
    virtual double   SampleRate() const = 0;

    virtual void SetFastForwarding(bool ff) = 0;
    virtual void NotifyConfigUpdated() = 0;

    virtual void ApplyCheats(const std::vector<CheatEntry>& cheats) = 0;
    virtual const std::vector<CheatEntry>& GetCheats() const = 0;
    virtual void UpdateCheats() = 0;
    virtual void ToggleCheat(int idx, bool enabled) = 0;
    virtual void ReloadCheats() = 0;
    virtual void SetCheatPath(const std::string& path) = 0;

    virtual bool IsReady() const = 0;

    virtual const void* getSramData() const = 0;
    virtual size_t      getSramSize() const = 0;
    virtual bool saveSram() = 0;
};

IEmulatorCore* CreateEmulatorCore(int platform);

} // namespace beiklive
