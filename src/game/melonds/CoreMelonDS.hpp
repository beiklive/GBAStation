#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "core/Tools.hpp"

#include "core/IEmulatorCore.hpp"

#include "config/MelonDSConfig.hpp"
#include "MelonDSInstance.hpp"
#include "video/MelonDSVideo.hpp"
#include "video/FrameComposer.hpp"
#include "audio/MelonDSAudio.hpp"
#include "input/MelonDSInput.hpp"
#include "save/MelonDSSave.hpp"

#include "types.h"

namespace beiklive::melonds
{

class CoreMelonDS : public beiklive::core::IEmulatorCore
{
public:
    CoreMelonDS();
    ~CoreMelonDS() override;

    bool SetupGame(beiklive::GameEntry entry);
    void Cleanup();

    // ---- IEmulatorCore 接口 -----------------------------------------------

    bool LoadRom(const std::string& path) override;
    void Reset() override;
    void RunFrame() override;
    void Stop() override;
    bool IsRunning() const override;

    const uint32_t* GetTopScreen() override;
    const uint32_t* GetBottomScreen() override;
    int GetScreenWidth() const override;
    int GetScreenHeight() const override;
    int GetTopScreenHeight() const override;
    int GetBottomScreenHeight() const override;

    void PushInput(int key, bool pressed) override;
    void SaveState(const std::string& path) override;
    void LoadState(const std::string& path) override;

    // ---- 扩展接口 ---------------------------------------------------------

    const uint32_t* GetTopFramebuffer() const;
    const uint32_t* GetBottomFramebuffer() const;

    int ReadAudio(int16_t* data, int samples);

    void SetButtonState(unsigned id, bool pressed);
    void SetButtonsFromSignal();
    void TouchScreen(uint16_t x, uint16_t y);
    void ReleaseScreen();

    unsigned GameWidth()  const { return 256; }
    unsigned GameHeight() const { return 192 * 2; }
    double   Fps()        const { return 60.0; }

    void SetFastForwarding(bool ff) { m_fastForward = ff; }

    void SaveNDSSave();
    void LoadNDSSave();

    void ApplyCheats(const std::vector<CheatEntry>& cheats);
    const std::vector<CheatEntry>& GetCheats() const { return m_cheats; }

    bool IsReady() const { return m_ready; }

    MelonDSInstance& instance() { return m_instance; }

private:
    beiklive::GameEntry m_gameEntry;
    std::vector<CheatEntry> m_cheats;

    MelonDSInstance m_instance;
    std::unique_ptr<MelonDSVideo> m_video;
    std::unique_ptr<FrameComposer> m_composer;
    std::unique_ptr<MelonDSAudio> m_audio;
    std::unique_ptr<MelonDSInput> m_input;
    std::unique_ptr<MelonDSSave> m_save;

    MelonDSConfig m_config;

    std::vector<uint8_t> m_romData;

    bool m_ready = false;
    bool m_fastForward = false;

    bool _loadROMFile(const std::string& romPath);
};

} // namespace beiklive::melonds
