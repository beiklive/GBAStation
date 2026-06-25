#pragma once

#include "core/common.h"
#include "core/Tools.hpp"
#include "core/GameSignal.hpp"
#include "game/retro/LibretroLoader.hpp"
#include "emulator/IEmulatorCore.hpp"

namespace beiklive::snes9x {

class CoreSnes9x : public IEmulatorCore {
public:
    CoreSnes9x() = default;
    CoreSnes9x(beiklive::CoreType coreType, std::string coreName)
        : m_coreType(coreType), m_coreName(std::move(coreName)) {}
    ~CoreSnes9x();

    bool SetupGame(beiklive::GameEntry GameEntry);
    void Cleanup();

    void RunFrame();
    void Reset();

    bool Serialize(std::vector<uint8_t>& outBuf) const;
    bool Unserialize(const std::vector<uint8_t>& buf);

    LibretroLoader::VideoFrame GetVideoFrame() const { return m_core.getVideoFrame(); }
    bool DrainAudio(std::vector<int16_t>& out) { return m_core.drainAudio(out); }

    void SetButtonState(unsigned player, unsigned id, bool pressed) override { m_core.setButtonState(player, id, pressed); }
    void SetButtonsFromSignal(unsigned player) override {
        uint32_t mask = GameSignal::instance().getGameButtonMask(player);
        for (unsigned i = 0; i < 16; ++i)
            m_core.setButtonState(player, i, (mask >> i) & 1u);
    }

    unsigned GameWidth()  const { return m_core.gameWidth();  }
    unsigned GameHeight() const { return m_core.gameHeight(); }
    double   Fps()        const { return m_core.fps();        }
    double   SampleRate() const { return m_core.sampleRate(); }

    void SetFastForwarding(bool ff) { m_core.setFastForwarding(ff); }
    void NotifyConfigUpdated() { m_core.notifyConfigUpdated(); }

    void ApplyCheats(const std::vector<CheatEntry>& cheats) {
        m_cheats = cheats;
        _updateCheats();
    }
    const std::vector<beiklive::CheatEntry>& GetCheats() const { return m_cheats; }
    void UpdateCheats() { _updateCheats(); }
    void ToggleCheat(int idx, bool enabled) {
        if (idx < 0 || idx >= (int)m_cheats.size()) return;
        m_cheats[idx].enabled = enabled;
        _updateCheats();
    }
    void ReloadCheats() { _loadCheats(); }
    void SetCheatPath(const std::string& path) { m_gameEntry.cheatPath = path; }

    bool IsReady() const { return m_ready; }

    const void* getSramData() const { return m_core.getMemoryData(RETRO_MEMORY_SAVE_RAM); }
    size_t      getSramSize() const { return m_core.getMemorySize(RETRO_MEMORY_SAVE_RAM); }
    bool saveSram() { return _saveSram(); }

private:
    beiklive::GameEntry m_gameEntry;
    beiklive::LibretroLoader m_core;
    std::vector<beiklive::CheatEntry> m_cheats;
    beiklive::CoreType m_coreType = beiklive::CoreType::Snes9x;
    std::string m_coreName = "Snes9x";
    bool m_ready = false;

    bool _loadCore();
    bool _loadRom(const std::string &romPath);
    void _initConfig();

    bool _loadSram();
    bool _loadCheats();
    void _updateCheats();

    bool _saveSram();
};

} // namespace beiklive::snes9x
