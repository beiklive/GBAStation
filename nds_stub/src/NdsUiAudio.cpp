#include "nds_stub/NdsUiAudio.hpp"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <switch.h>

#include "nds_stub/StubLog.hpp"

namespace beiklive::nds_stub {
namespace {

constexpr int kSwitchOutRate = 48000;
constexpr const char* kSoundRoot = "romfs:/sounds/switch/";

std::uint16_t readU16LE(const std::uint8_t* p)
{
    return static_cast<std::uint16_t>(p[0]) |
           (static_cast<std::uint16_t>(p[1]) << 8);
}

std::uint32_t readU32LE(const std::uint8_t* p)
{
    return static_cast<std::uint32_t>(p[0]) |
           (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) |
           (static_cast<std::uint32_t>(p[3]) << 24);
}

} // namespace

NdsUiAudioPlayer::NdsUiAudioPlayer()
{
    Result rc = audoutInitialize();
    if (R_FAILED(rc))
    {
        appendStubLog("GBAStationNDSStub: ui wav audio audoutInitialize failed rc=%#x", rc);
        return;
    }

    rc = audoutStartAudioOut();
    if (R_FAILED(rc))
    {
        appendStubLog("GBAStationNDSStub: ui wav audio audoutStartAudioOut rc=%#x", rc);
    }

    m_init = true;
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&NdsUiAudioPlayer::playbackThread, this);
}

NdsUiAudioPlayer::~NdsUiAudioPlayer()
{
    stop();
}

void NdsUiAudioPlayer::stop()
{
    m_running.store(false, std::memory_order_release);
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
    if (m_init)
    {
        audoutStopAudioOut();
        audoutExit();
        m_init = false;
    }
}

NdsUiAudioPlayer::SoundSlot NdsUiAudioPlayer::slotForMenuSound(NdsMenuSound sound)
{
    switch (sound)
    {
    case NdsMenuSound::Focus: return SoundSlot::Focus;
    case NdsMenuSound::Click: return SoundSlot::Click;
    case NdsMenuSound::Back: return SoundSlot::Back;
    case NdsMenuSound::Error: return SoundSlot::Error;
    case NdsMenuSound::Slider: return SoundSlot::Slider;
    default: return SoundSlot::Error;
    }
}

const char* NdsUiAudioPlayer::soundFileName(SoundSlot slot)
{
    switch (slot)
    {
    case SoundSlot::Focus: return "SeNaviFocus.wav";
    case SoundSlot::Click: return "SeBtnDecide.wav";
    case SoundSlot::Back: return "SeFooterDecideFinish.wav";
    case SoundSlot::Error: return "SeKeyErrorCursor.wav";
    case SoundSlot::Slider: return "SeSliderTickOver.wav";
    default: return "";
    }
}

bool NdsUiAudioPlayer::loadWav(const char* path, WavData& out)
{
    FILE* file = std::fopen(path, "rb");
    if (!file)
        return false;

    if (std::fseek(file, 0, SEEK_END) != 0)
    {
        std::fclose(file);
        return false;
    }
    const long fileSize = std::ftell(file);
    if (fileSize < 44)
    {
        std::fclose(file);
        return false;
    }
    std::rewind(file);

    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(fileSize));
    const std::size_t read = std::fread(bytes.data(), 1, bytes.size(), file);
    std::fclose(file);
    if (read != bytes.size())
        return false;

    if (std::memcmp(bytes.data(), "RIFF", 4) != 0 ||
        std::memcmp(bytes.data() + 8, "WAVE", 4) != 0)
    {
        return false;
    }

    std::uint16_t fmtTag = 0;
    std::uint16_t channels = 0;
    std::uint16_t bitsPerSample = 0;
    std::uint32_t sampleRate = 0;
    const std::uint8_t* pcmStart = nullptr;
    std::size_t pcmBytes = 0;

    std::size_t pos = 12;
    while (pos + 8 <= bytes.size())
    {
        const std::uint32_t chunkSize = readU32LE(bytes.data() + pos + 4);
        if (pos + 8 + chunkSize > bytes.size())
            break;

        if (std::memcmp(bytes.data() + pos, "fmt ", 4) == 0 && chunkSize >= 16)
        {
            fmtTag = readU16LE(bytes.data() + pos + 8);
            channels = readU16LE(bytes.data() + pos + 10);
            sampleRate = readU32LE(bytes.data() + pos + 12);
            bitsPerSample = readU16LE(bytes.data() + pos + 22);
        }
        else if (std::memcmp(bytes.data() + pos, "data", 4) == 0)
        {
            pcmStart = bytes.data() + pos + 8;
            pcmBytes = chunkSize;
            break;
        }

        pos += 8 + chunkSize + (chunkSize & 1u);
    }

    if (fmtTag != 1 || bitsPerSample != 16 || channels == 0 ||
        sampleRate == 0 || !pcmStart || pcmBytes == 0)
    {
        return false;
    }

    out.sampleRate = static_cast<int>(sampleRate);
    out.channels = static_cast<int>(channels);
    const std::size_t sampleCount = pcmBytes / sizeof(std::int16_t);
    out.samples.resize(sampleCount);
    std::memcpy(out.samples.data(), pcmStart, sampleCount * sizeof(std::int16_t));

    if (channels == 1)
    {
        std::vector<std::int16_t> stereo(sampleCount * 2);
        for (std::size_t i = 0; i < sampleCount; ++i)
        {
            stereo[i * 2] = out.samples[i];
            stereo[i * 2 + 1] = out.samples[i];
        }
        out.samples = std::move(stereo);
        out.channels = 2;
    }

    out.loaded = true;
    return true;
}

bool NdsUiAudioPlayer::load(SoundSlot slot)
{
    const std::size_t index = static_cast<std::size_t>(slot);
    if (index >= m_sounds.size())
        return false;
    if (m_sounds[index].loaded)
        return true;

    char path[128] = {};
    std::snprintf(path, sizeof(path), "%s%s", kSoundRoot, soundFileName(slot));
    if (!loadWav(path, m_sounds[index]))
    {
        appendStubLog("GBAStationNDSStub: ui wav audio load failed path=%s", path);
        return false;
    }
    return true;
}

bool NdsUiAudioPlayer::play(NdsMenuSound sound, float pitch)
{
    if (!m_init)
        return false;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pendingSlot = slotForMenuSound(sound);
        m_pendingPitch = pitch;
        m_hasPending = true;
    }
    m_cv.notify_one();
    return true;
}

void NdsUiAudioPlayer::playbackThread()
{
    while (m_running.load(std::memory_order_acquire))
    {
        SoundSlot slot = SoundSlot::Click;
        float pitch = 1.0f;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this] {
                return m_hasPending || !m_running.load(std::memory_order_acquire);
            });
            if (!m_running.load(std::memory_order_acquire))
                break;
            slot = m_pendingSlot;
            pitch = m_pendingPitch;
            m_hasPending = false;
        }

        const std::size_t index = static_cast<std::size_t>(slot);
        if (index >= m_sounds.size())
            continue;
        if (!m_sounds[index].loaded && !load(slot))
            continue;
        playSoundDirect(m_sounds[index], pitch);
    }
}

void NdsUiAudioPlayer::playSoundDirect(const WavData& wav, float pitch)
{
    if (!m_init || wav.samples.empty() || wav.channels <= 0 || wav.sampleRate <= 0)
        return;

    const double effectivePitch = pitch > 0.1f ? static_cast<double>(pitch) : 1.0;
    const std::size_t inFrames = wav.samples.size() / static_cast<std::size_t>(wav.channels);
    if (inFrames == 0)
        return;

    const double ratio = static_cast<double>(kSwitchOutRate) /
                         (static_cast<double>(wav.sampleRate) * effectivePitch);
    const std::size_t outFrames = static_cast<std::size_t>(static_cast<double>(inFrames) * ratio + 0.5);
    if (outFrames == 0)
        return;

    const std::size_t dataBytes = outFrames * 2 * sizeof(std::int16_t);
    const std::size_t alignedBytes = (dataBytes + 0xFFFu) & ~static_cast<std::size_t>(0xFFFu);
    void* rawBuffer = std::aligned_alloc(0x1000, alignedBytes);
    if (!rawBuffer)
    {
        appendStubLog("GBAStationNDSStub: ui wav audio alloc failed bytes=%u",
                      static_cast<unsigned>(alignedBytes));
        return;
    }
    std::memset(rawBuffer, 0, alignedBytes);

    auto* dst = static_cast<std::int16_t*>(rawBuffer);
    for (std::size_t i = 0; i < outFrames; ++i)
    {
        std::size_t srcFrame = static_cast<std::size_t>(static_cast<double>(i) / ratio);
        srcFrame = std::min(srcFrame, inFrames - 1);
        const std::size_t src = srcFrame * static_cast<std::size_t>(wav.channels);
        const std::int16_t left = wav.samples[src];
        const std::int16_t right = wav.channels > 1 ? wav.samples[src + 1] : left;
        dst[i * 2] = left;
        dst[i * 2 + 1] = right;
    }
    armDCacheFlush(rawBuffer, dataBytes);

    AudioOutBuffer buffer {};
    buffer.buffer = rawBuffer;
    buffer.buffer_size = alignedBytes;
    buffer.data_size = dataBytes;
    buffer.data_offset = 0;

    if (R_SUCCEEDED(audoutAppendAudioOutBuffer(&buffer)))
    {
        AudioOutBuffer* released = nullptr;
        std::uint32_t releasedCount = 0;
        const std::uint64_t waitNs =
            static_cast<std::uint64_t>(inFrames) * 2000000000ULL /
            static_cast<std::uint64_t>(wav.sampleRate) + 200000000ULL;
        audoutWaitPlayFinish(&released, &releasedCount, waitNs);
    }

    std::free(rawBuffer);
}

} // namespace beiklive::nds_stub
