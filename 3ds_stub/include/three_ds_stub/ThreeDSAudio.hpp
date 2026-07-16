#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "audio_core/sink.h"
#include "three_ds_stub/ThreeDSSwitch.hpp"

namespace beiklive::three_ds_stub {

class ThreeDSSwitchAudioSink final : public AudioCore::Sink {
public:
    explicit ThreeDSSwitchAudioSink(std::string_view device_id);
    ~ThreeDSSwitchAudioSink() override;

    unsigned int GetNativeSampleRate() const override;
    void SetCallback(std::function<void(s16*, std::size_t)> callback) override;
    bool ImmediateSubmission() override;
    void PushSamples(const void* data, std::size_t num_samples) override;

    [[nodiscard]] bool Ready() const;

private:
    static constexpr std::size_t kBufferCount = 4;
    static constexpr std::size_t kFramesPerBuffer = 960;
    static constexpr std::size_t kChannels = 2;
    static constexpr std::size_t kBufferBytes = kFramesPerBuffer * kChannels * sizeof(s16);

    void CollectReleased(bool wait);
    bool SubmitPending();

    std::array<AudioOutBuffer, kBufferCount> out_buffers_{};
    std::array<s16*, kBufferCount> samples_{};
    std::array<bool, kBufferCount> busy_{};
    std::vector<s16> pending_;
    double resample_position_ = 0.0;
    bool initialized_ = false;
    bool first_push_logged_ = false;
    bool first_submit_logged_ = false;
};

std::unique_ptr<AudioCore::Sink> CreateThreeDSSwitchAudioSink(std::string_view device_id);
std::vector<std::string> ListThreeDSSwitchAudioDevices();

} // namespace beiklive::three_ds_stub
