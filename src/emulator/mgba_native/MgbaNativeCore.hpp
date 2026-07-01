#pragma once

#include "core/common.h"
#include "core/GameSignal.hpp"
#include "emulator/IEmulatorAudioOutput.hpp"
#include "emulator/IEmulatorCore.hpp"

#include <mgba/core/interface.h>
#include <mgba/gba/interface.h>

#include <array>
#include <ctime>

#ifdef __SWITCH__
#include <switch.h>
#endif

struct mCore;

namespace beiklive::mgba_native
{

class MgbaNativeCore : public beiklive::IEmulatorCore,
                       public beiklive::IEmulatorAudioOutput
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
    double SampleRate() const override { return m_sampleRate; }

    void SetFastForwarding(bool ff) override { m_fastForwarding = ff; }
    void NotifyConfigUpdated() override { applyConfig(); }

    void ApplyCheats(const std::vector<CheatEntry>& cheats) override;
    void ReloadCheats();
    void SetCheatPath(const std::string& path) override;

    bool IsReady() const override { return m_ready; }

    const void* getSramData() const override;
    size_t getSramSize() const override;
    bool saveSram() override;
    bool HandlesAudioOutput() const override;
    void SetAudioOutputEnabled(bool enabled) override;
    void FlushAudioOutput() override;

private:
    static constexpr double kDefaultSampleRate = 32768.0;
    static constexpr size_t kSwitchAudioSamples = 0x200;
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
    void applyAudioLowPassBuffer(int16_t* samples, size_t frames);
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
    void configureAudioStream();
    bool initNativeAudioOutput();
    void shutdownNativeAudioOutput();
    int waitNativeAudioOutput(uint64_t timeoutNs);

    static void sampleLux(GBALuminanceSource* source);
    static uint8_t readLux(GBALuminanceSource* source);
    static void sampleRtc(mRTCSource* source);
    static time_t readRtcUnixTime(mRTCSource* source);
    static void postAudioBuffer(mAVStream* stream, blip_t* left, blip_t* right);

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
    double m_sampleRate = kDefaultSampleRate;

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
    unsigned m_audioProbeFrames = 0;
    unsigned m_audioSilentProbeFrames = 0;

    struct NativeAudioStream
    {
        mAVStream d{};
        MgbaNativeCore* owner = nullptr;
    };
    NativeAudioStream m_audioStream{};
    bool m_audioStreamEnabled = false;
    bool m_audioOutputEnabled = true;

#ifdef __SWITCH__
    static constexpr size_t kSwitchAudioBufferBytes = 0x1000;
    static constexpr int kSwitchAudioBufferCount = 4;
    std::array<int16_t*, kSwitchAudioBufferCount> m_switchAudioBuffers{};
    std::array<AudioOutBuffer, kSwitchAudioBufferCount> m_switchAudioOutBuffers{};
    int m_switchAudioActive = 0;
    uint32_t m_switchAudioEnqueued = 0;
    bool m_switchAudioInitialized = false;
    bool m_loggedFirstSwitchAppend = false;
    bool m_loggedFirstNonZeroSwitchAudio = false;
    unsigned m_switchSilentProbeBuffers = 0;
    uint32_t m_switchAudioCallbackCount = 0;
#endif

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
