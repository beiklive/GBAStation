#pragma once

#include "emulator/IEmulatorCore.hpp"
#include "emulator/IEmulatorStopRequest.hpp"
#include "emulator/IEmulatorTouchInput.hpp"
#include "emulator/IEmulatorVideoTexture.hpp"
#include "emulator/melonds/MelonDSAudio.h"
#include "emulator/melonds/MelonDSInput.h"
#include "emulator/melonds/MelonDSPlatform.h"
#include "emulator/melonds/MelonDSVideo.h"

#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <vector>

namespace melonDS {
class Renderer3D;
class NDS;
struct NDSArgs;
namespace NDSCart {
struct NDSCartArgs;
}
}

namespace beiklive::melonds {

struct MelonDSGLContext;

class MelonDSCore : public IEmulatorCore, public IEmulatorTouchInput, public IEmulatorStopRequest, public IEmulatorVideoTexture {
public:
    MelonDSCore();
    ~MelonDSCore() override;

    bool SetupGame(beiklive::GameEntry GameEntry) override;
    void Cleanup() override;
    void RunFrame() override;
    void Reset() override;

    bool Serialize(std::vector<uint8_t>& outBuf) const override;
    bool Unserialize(const std::vector<uint8_t>& buf) override;

    LibretroLoader::VideoFrame GetVideoFrame() const override { return m_video.GetFrame(); }
    bool DrainAudio(std::vector<int16_t>& out) override { return m_audio.Drain(out); }

    void SetButtonState(unsigned id, bool pressed) override { SetButton(static_cast<int>(id), pressed); }
    void SetButtonsFromSignal() override;

    unsigned GameWidth() const override { return MelonDSVideo::kWidth; }
    unsigned GameHeight() const override { return MelonDSVideo::kHeight; }
    double Fps() const override { return 59.8261; }
    double SampleRate() const override { return 48000.0; }

    void SetFastForwarding(bool ff) override { m_fastForwarding = ff; }
    void NotifyConfigUpdated() override;

    void ApplyCheats(const std::vector<CheatEntry>& cheats) override { m_cheats = cheats; }
    const std::vector<beiklive::CheatEntry>& GetCheats() const override { return m_cheats; }
    void UpdateCheats() override {}
    void ToggleCheat(int idx, bool enabled) override;
    void ReloadCheats() override {}
    void SetCheatPath(const std::string& path) override { m_gameEntry.cheatPath = path; }

    bool IsReady() const override { return m_ready.load(std::memory_order_acquire); }

    const void* getSramData() const override;
    size_t getSramSize() const override;
    bool saveSram() override;

    bool Initialize();
    bool LoadGame(const std::string& path);
    void Pause(bool paused);
    void Stop();
    void RequestStop() override;
    bool SaveState(const std::string& path);
    bool LoadState(const std::string& path);
    void SetButton(int key, bool pressed);
    void SetTouch(int x, int y, bool down) override;
    const uint32_t* GetFrameBuffer() const { return m_video.GetFrameBuffer(); }
    bool GetVideoTexture(beiklive::EmulatorVideoTexture& out) override;

private:
    beiklive::GameEntry m_gameEntry;
    std::unique_ptr<melonDS::NDS> m_nds;
    PlatformUserData m_platformData;
    MelonDSAudio m_audio;
    MelonDSInput m_input;
    MelonDSVideo m_video;
    std::vector<beiklive::CheatEntry> m_cheats;
    std::vector<uint8_t> m_romData;
    std::string m_saveFile;
    std::string m_stateFile;
    std::string m_biosDir;
    std::atomic<bool> m_ready{false};
    std::atomic<bool> m_initialized{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_fastForwarding{false};
    std::atomic<bool> m_stopRequested{false};
    int m_internalResolution = 1;
    bool m_usingAcceleratedRenderer = false;
    bool m_usingComputeRenderer = false;
    bool m_acceleratedReadbackFailed = false;
    std::unique_ptr<MelonDSGLContext> m_glContext;
    std::vector<uint32_t> m_acceleratedReadback;
    int m_skipAcceleratedReadbackFrames = 0;
    mutable std::mutex m_ndsMutex;

    bool loadBiosFiles(melonDS::NDSArgs& args);
    bool loadBatterySave(melonDS::NDSCart::NDSCartArgs& args) const;
    std::unique_ptr<melonDS::Renderer3D> createRenderer3D();
    bool captureAcceleratedFrame();
    std::string defaultSaveDir() const;
    std::string defaultBiosDir() const;
};

} // namespace beiklive::melonds
