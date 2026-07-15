#include "three_ds_stub/ThreeDSAudio.hpp"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "audio_core/audio_types.h"
#include "three_ds_stub/ThreeDSLog.hpp"

namespace beiklive::three_ds_stub {
namespace {

constexpr double kSourceRate = static_cast<double>(AudioCore::native_sample_rate);
constexpr double kOutputRate = 48000.0;

} // namespace

ThreeDSSwitchAudioSink::ThreeDSSwitchAudioSink(std::string_view) {
    libnx_Result rc = audoutInitialize();
    if (R_FAILED(rc)) {
        appendLog("GBAStation3DSStub: audoutInitialize failed rc=%#x", rc);
        return;
    }

    rc = audoutStartAudioOut();
    if (R_FAILED(rc)) {
        appendLog("GBAStation3DSStub: audoutStartAudioOut failed rc=%#x", rc);
        audoutExit();
        return;
    }

    for (std::size_t i = 0; i < kBufferCount; ++i) {
        samples_[i] = static_cast<s16*>(aligned_alloc(0x1000, 0x1000));
        if (!samples_[i]) {
            appendLog("GBAStation3DSStub: audio buffer allocation failed index=%zu", i);
            return;
        }
        out_buffers_[i].buffer = samples_[i];
        out_buffers_[i].buffer_size = 0x1000;
        out_buffers_[i].data_offset = 0;
        out_buffers_[i].data_size = kBufferBytes;
    }

    pending_.reserve(kFramesPerBuffer * kChannels * 2);
    initialized_ = true;
}

ThreeDSSwitchAudioSink::~ThreeDSSwitchAudioSink() {
    if (initialized_) {
        while (std::any_of(busy_.begin(), busy_.end(), [](bool value) { return value; })) {
            CollectReleased(true);
        }
        audoutStopAudioOut();
        audoutExit();
    }
    for (s16* samples : samples_) {
        std::free(samples);
    }
}

unsigned int ThreeDSSwitchAudioSink::GetNativeSampleRate() const {
    return AudioCore::native_sample_rate;
}

void ThreeDSSwitchAudioSink::SetCallback(std::function<void(s16*, std::size_t)>) {}

bool ThreeDSSwitchAudioSink::ImmediateSubmission() {
    return true;
}

bool ThreeDSSwitchAudioSink::Ready() const {
    return initialized_;
}

void ThreeDSSwitchAudioSink::CollectReleased(bool wait) {
    if (!initialized_) {
        return;
    }

    AudioOutBuffer* released = nullptr;
    u32 count = 0;
    const libnx_Result rc = wait ? audoutWaitPlayFinish(&released, &count, 20'000'000)
                                 : audoutGetReleasedAudioOutBuffer(&released, &count);
    if (R_FAILED(rc)) {
        return;
    }
    for (AudioOutBuffer* item = released; item; item = item->next) {
        for (std::size_t i = 0; i < kBufferCount; ++i) {
            if (item == &out_buffers_[i]) {
                busy_[i] = false;
                break;
            }
        }
    }
}

bool ThreeDSSwitchAudioSink::SubmitPending() {
    if (pending_.size() < kFramesPerBuffer * kChannels) {
        return true;
    }

    CollectReleased(false);
    auto free_it = std::find(busy_.begin(), busy_.end(), false);
    while (free_it == busy_.end()) {
        CollectReleased(true);
        free_it = std::find(busy_.begin(), busy_.end(), false);
    }

    const std::size_t index = static_cast<std::size_t>(free_it - busy_.begin());
    std::memcpy(samples_[index], pending_.data(), kBufferBytes);
    armDCacheFlush(samples_[index], 0x1000);
    out_buffers_[index].data_size = kBufferBytes;
    const libnx_Result rc = audoutAppendAudioOutBuffer(&out_buffers_[index]);
    if (R_FAILED(rc)) {
        appendLog("GBAStation3DSStub: audoutAppendAudioOutBuffer failed rc=%#x", rc);
        return false;
    }
    busy_[index] = true;
    pending_.erase(pending_.begin(), pending_.begin() + kFramesPerBuffer * kChannels);
    return true;
}

void ThreeDSSwitchAudioSink::PushSamples(const void* data, std::size_t num_samples) {
    if (!initialized_ || !data || num_samples == 0) {
        return;
    }

    const auto* source = static_cast<const s16*>(data);
    const double step = kSourceRate / kOutputRate;
    while (resample_position_ < static_cast<double>(num_samples)) {
        const std::size_t index = std::min<std::size_t>(
            static_cast<std::size_t>(resample_position_), num_samples - 1);
        pending_.push_back(source[index * 2]);
        pending_.push_back(source[index * 2 + 1]);
        resample_position_ += step;
    }
    resample_position_ -= static_cast<double>(num_samples);

    while (pending_.size() >= kFramesPerBuffer * kChannels) {
        if (!SubmitPending()) {
            pending_.clear();
            break;
        }
    }
}

std::unique_ptr<AudioCore::Sink> CreateThreeDSSwitchAudioSink(std::string_view device_id) {
    return std::make_unique<ThreeDSSwitchAudioSink>(device_id);
}

std::vector<std::string> ListThreeDSSwitchAudioDevices() {
    return {"Nintendo Switch audout"};
}

} // namespace beiklive::three_ds_stub
