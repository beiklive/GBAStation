#pragma once

#include "core/GameSignal.hpp"
#include "emulator/IEmulatorCore.hpp"

#include <array>
#include <mutex>

namespace beiklive::genesis
{

class GenesisCore final : public beiklive::IEmulatorCore
{
public:
    GenesisCore() = default;
    ~GenesisCore() override;

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
    void NotifyConfigUpdated() override;

    void ApplyCheats(const std::vector<CheatEntry>& cheats) override;
    void SetCheatPath(const std::string& path) override { m_gameEntry.cheatPath = path; }

    bool IsReady() const override { return m_ready; }

    const void* getSramData() const override;
    size_t getSramSize() const override;
    bool saveSram() override;

private:
    static constexpr unsigned kBitmapWidth = 720;
    static constexpr unsigned kBitmapHeight = 576;
    static constexpr double kSampleRate = 48000.0;
    static constexpr size_t kMaxAudioSamples = 32768;
    static constexpr unsigned kInputPorts = 2;
    static constexpr unsigned kInputButtons = 16;

    bool loadSram();
    void updateInput();
    void captureVideoFrame();
    void clearRuntimeState();
    void applyConfig();
    std::string saveFilePath() const;

    beiklive::GameEntry m_gameEntry;
    bool m_ready = false;
    bool m_audioInitialized = false;
    bool m_fastForwarding = false;
    unsigned m_width = 320;
    unsigned m_height = 224;
    double m_fps = 60.0;
    double m_sampleRate = kSampleRate;

    std::vector<uint16_t> m_bitmapStorage;
    mutable std::mutex m_videoMutex;
    LibretroLoader::VideoFrame m_videoFrame;

    mutable std::mutex m_audioMutex;
    std::vector<int16_t> m_audioBuffer;
    std::array<std::array<bool, kInputButtons>, kInputPorts> m_buttons{};
};

} // namespace beiklive::genesis
