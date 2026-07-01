#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "emulator/IEmulatorCore.hpp"

#include <mgba/core/interface.h>
#include <mgba/gba/interface.h>

#include <array>
#include <ctime>

struct mCore;

namespace beiklive::mgba_native
{

class MgbaNativeCore : public beiklive::IEmulatorCore
{
public:
    MgbaNativeCore() = default;
    ~MgbaNativeCore() override;

    bool SetupGame(beiklive::GameEntry gameEntry) override;
    void Cleanup() override;
    void RunFrame() override;
    void Reset() override;

    bool Serialize(std::vector<uint8_t>& outBuf) const override;
    bool Unserialize(const std::vector<uint8_t>& buf) override;

    LibretroLoader::VideoFrame GetVideoFrame() const override;
    bool DrainAudio(std::vector<int16_t>& out) override;

    void SetButtonState(unsigned player, unsigned id, bool pressed) override;
    void SetButtonsFromSignal(unsigned player) override;

    unsigned GameWidth() const override { return m_width; }
    unsigned GameHeight() const override { return m_height; }
    double Fps() const override { return m_fps; }
    double SampleRate() const override { return kSampleRate; }

    void SetFastForwarding(bool ff) override { m_fastForwarding = ff; }
    void NotifyConfigUpdated() override { applyConfig(); }

    void ApplyCheats(const std::vector<CheatEntry>& cheats) override;
    const std::vector<CheatEntry>& GetCheats() const override { return m_cheats; }
    void UpdateCheats() override { updateCheats(); }
    void ToggleCheat(int idx, bool enabled) override;
    void ReloadCheats() override;
    void SetCheatPath(const std::string& path) override;

    bool IsReady() const override { return m_ready; }

    const void* getSramData() const override;
    size_t getSramSize() const override;
    bool saveSram() override;

private:
    static constexpr double kSampleRate = 32768.0;
    static constexpr unsigned kMaxVideoWidth = 256;
    static constexpr unsigned kMaxVideoHeight = 224;
    static constexpr size_t kAudioBufferCapacity = 32768;
    static constexpr unsigned kMaxInputPorts = 2;
    static constexpr unsigned kMaxButtons = RETRO_DEVICE_ID_JOYPAD_R3 + 1;

    bool loadRom(const std::string& romPath);
    void initSettingsDefaults();
    void initConfigDefaults();
    void applyConfig();
    void applyAudioLowPassSettings();
    void applyAudioLowPass(std::vector<int16_t>& samples);
    void installPeripherals();
    void updateLuxLevel();
    bool loadSram();
    bool loadCheats();
    void updateCheats();
    void drainMgbaAudio();
    void captureVideoFrame();
    void updateKeys();
    void releaseCore();
    std::string saveFilePath() const;

    static void sampleLux(GBALuminanceSource* source);
    static uint8_t readLux(GBALuminanceSource* source);
    static void sampleRtc(mRTCSource* source);
    static time_t readRtcUnixTime(mRTCSource* source);

    struct NativeLuminanceSource
    {
        GBALuminanceSource d{};
        MgbaNativeCore* owner = nullptr;
    };

    struct NativeRtcSource
    {
        mRTCSource d{};
        MgbaNativeCore* owner = nullptr;
    };

    beiklive::GameEntry m_gameEntry;
    mCore* m_core = nullptr;
    bool m_coreInitialized = false;
    bool m_configInitialized = false;
    bool m_ready = false;
    bool m_fastForwarding = false;

    unsigned m_width = 0;
    unsigned m_height = 0;
    double m_fps = 60.0;

    unsigned m_bufferWidth = 0;
    unsigned m_bufferHeight = 0;
    std::vector<color_t> m_videoBuffer;
    mutable std::mutex m_videoMutex;
    LibretroLoader::VideoFrame m_videoFrame;

    mutable std::mutex m_audioMutex;
    std::vector<int16_t> m_audioBuffer;
    bool m_audioLowPassEnabled = false;
    int32_t m_audioLowPassRange = (60 * 0x10000) / 100;
    int32_t m_audioLowPassLeftPrev = 0;
    int32_t m_audioLowPassRightPrev = 0;
    bool m_loggedFirstAudio = false;

    std::array<std::array<bool, kMaxButtons>, kMaxInputPorts> m_buttons{};
    uint32_t m_keyMask = 0;

    std::vector<beiklive::CheatEntry> m_cheats;

    NativeLuminanceSource m_luminanceSource{};
    uint8_t m_luxLevel = 0x16;
    int m_luxLevelIndex = 5;

    NativeRtcSource m_rtcSource{};
    bool m_useSystemRtc = false;

    mutable std::vector<uint8_t> m_sramSnapshot;
};

} // namespace beiklive::mgba_native
