#include "ui/audio/BKAudioPlayer.hpp"

#include "core/common.h"
#include <borealis/core/logger.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

// ============================================================
// 平台相关头文件
// ============================================================
#ifdef BK_AUDIO_ALSA
#include <alsa/asoundlib.h>

#elif defined(BK_AUDIO_WINMM)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>

#elif defined(BK_AUDIO_COREAUDIO)
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#endif

namespace beiklive {

// ============================================================
// 平台相关常量
// ============================================================
#ifdef BK_AUDIO_ALSA
constexpr unsigned ALSA_LATENCY_US = 100000; // 100ms 延迟
#endif

#ifdef BK_AUDIO_COREAUDIO
constexpr auto PLAYBACK_TIMEOUT = std::chrono::seconds(5);
#endif

// ============================================================
// 音效名称表：brls::Sound 枚举 → WAV 文件名映射
// ============================================================
static const char* SOUND_FILE_NAMES[brls::_SOUND_MAX] = {
    nullptr,          // SOUND_NONE
    "Focus.wav",      // SOUND_FOCUS_CHANGE  - 焦点切换
    "Limit.wav",      // SOUND_FOCUS_ERROR   - 焦点错误（边界限制）
    "Focus.wav",      // SOUND_CLICK         - 点击
    "Scroll.wav",     // SOUND_BACK          - 返回
    "Focus.wav",      // SOUND_FOCUS_SIDEBAR - 侧边栏焦点
    "Error.wav",      // SOUND_CLICK_ERROR   - 点击错误
    "Startup.wav",    // SOUND_HONK          - 提示音
    "Focus.wav",      // SOUND_CLICK_SIDEBAR - 侧边栏点击
    "Scroll.wav",     // SOUND_TOUCH_UNFOCUS - 触摸失焦
    "Focus.wav",      // SOUND_TOUCH         - 触摸
    "Scroll.wav",     // SOUND_SLIDER_TICK   - 滑块刻度
    "Focus.wav",      // SOUND_SLIDER_RELEASE- 滑块释放
};

// ============================================================
// 简易 WAV 加载器（仅支持 16-bit PCM）
// ============================================================

static uint16_t readU16LE(const uint8_t* p)
{
    return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static uint32_t readU32LE(const uint8_t* p)
{
    return static_cast<uint32_t>(p[0])
         | (static_cast<uint32_t>(p[1]) << 8)
         | (static_cast<uint32_t>(p[2]) << 16)
         | (static_cast<uint32_t>(p[3]) << 24);
}

bool BKAudioPlayer::loadWav(const std::string& path, WavData& out)
{
    FILE* f = fopen(path.c_str(), "rb");
    if (!f)
        return false;

    // 读取整个文件
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < 44)
    {
        fclose(f);
        return false;
    }
    std::vector<uint8_t> buf(static_cast<size_t>(size));
    if (fread(buf.data(), 1, buf.size(), f) != buf.size())
    {
        fclose(f);
        return false;
    }
    fclose(f);

    // 验证 RIFF/WAVE 头
    if (memcmp(buf.data(), "RIFF", 4) != 0 || memcmp(buf.data() + 8, "WAVE", 4) != 0)
        return false;

    // 遍历各块
    size_t pos = 12;
    uint16_t fmtTag = 0, channels = 0, bitsPerSample = 0;
    uint32_t sampleRate = 0;
    const uint8_t* pcmStart = nullptr;
    size_t          pcmBytes = 0;

    while (pos + 8 <= buf.size())
    {
        uint32_t chunkSize = readU32LE(buf.data() + pos + 4);
        if (memcmp(buf.data() + pos, "fmt ", 4) == 0 && chunkSize >= 16)
        {
            fmtTag        = readU16LE(buf.data() + pos + 8);
            channels      = readU16LE(buf.data() + pos + 10);
            sampleRate    = readU32LE(buf.data() + pos + 12);
            bitsPerSample = readU16LE(buf.data() + pos + 22);
        }
        else if (memcmp(buf.data() + pos, "data", 4) == 0)
        {
            pcmStart = buf.data() + pos + 8;
            pcmBytes = static_cast<size_t>(chunkSize);
            break;
        }
        pos += 8 + chunkSize;
        if (chunkSize & 1) ++pos; // 字对齐
    }

    if (fmtTag != 1 || bitsPerSample != 16 || channels == 0 || sampleRate == 0
        || pcmStart == nullptr || pcmBytes == 0)
        return false;

    out.sampleRate = static_cast<int>(sampleRate);
    out.channels   = static_cast<int>(channels);

    size_t sampleCount = pcmBytes / sizeof(int16_t);
    out.samples.resize(sampleCount);
    memcpy(out.samples.data(), pcmStart, pcmBytes);

    // 单声道转双声道，统一输出格式
    if (channels == 1)
    {
        std::vector<int16_t> stereo(sampleCount * 2);
        for (size_t i = 0; i < sampleCount; ++i)
        {
            stereo[i * 2]     = out.samples[i];
            stereo[i * 2 + 1] = out.samples[i];
        }
        out.samples  = std::move(stereo);
        out.channels = 2;
    }

    out.loaded = true;
    return true;
}

// ============================================================
// 辅助函数
// ============================================================

/// 检查按钮音效是否已启用（从 SettingManager 读取）
static bool isButtonSfxEnabled()
{
    if (!beiklive::SettingManager)
        return true; // 未配置则默认启用
    auto v = beiklive::SettingManager->Get(beiklive::SettingKey::KEY_AUDIO_BUTTON_SFX);
    if (!v)
        return true;
    if (auto s = v->AsString())
        return (*s != "false" && *s != "0" && *s != "no");
    if (auto i = v->AsInt())
        return (*i != 0);
    return true;
}

std::string BKAudioPlayer::soundsDir()
{
    return beiklive::res_path("sounds/switch/");
}

std::string BKAudioPlayer::soundFileName(brls::Sound sound)
{
    int idx = static_cast<int>(sound);
    if (idx <= 0 || idx >= brls::_SOUND_MAX)
        return {};
    const char* name = SOUND_FILE_NAMES[idx];
    if (!name)
        return {};
    return soundsDir() + name;
}

// ============================================================
// 构造 / 析构
// ============================================================

BKAudioPlayer::BKAudioPlayer()
{
    m_running = true;
    m_thread  = std::thread(&BKAudioPlayer::playbackThread, this);
}

BKAudioPlayer::~BKAudioPlayer()
{
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();
}

// ============================================================
// AudioPlayer 接口
// ============================================================

bool BKAudioPlayer::load(brls::Sound sound)
{
    int idx = static_cast<int>(sound);
    if (idx <= 0 || idx >= brls::_SOUND_MAX)
        return true; // SOUND_NONE 或越界，静默跳过

    if (m_sounds[idx].loaded)
        return true;

    std::string path = soundFileName(sound);
    if (path.empty())
        return false;

    if (!loadWav(path, m_sounds[idx]))
    {
        brls::Logger::warning("BKAudioPlayer: 无法加载 '{}' （文件缺失？）", path);
        return false;
    }

    brls::Logger::debug("BKAudioPlayer: 已加载 '{}'", path);
    return true;
}

bool BKAudioPlayer::play(brls::Sound sound, float pitch)
{
    int idx = static_cast<int>(sound);
    if (idx <= 0 || idx >= brls::_SOUND_MAX)
        return true;

    // 检查按钮音效设置，若禁用则静默返回
    if (!isButtonSfxEnabled())
        return true;

    // 不在 UI 线程中加载音效文件，避免文件 I/O 阻塞渲染导致画面闪烁。
    // 加载操作由后台播放线程（playbackThread）在第一次播放前完成。
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        // 覆盖未播放的待播音效（最新优先）
        m_pendingIdx   = idx;
        m_pendingPitch = pitch;
        m_hasPending   = true;
    }
    m_cv.notify_one();
    return true;
}

// ============================================================
// 后台播放线程
// ============================================================

void BKAudioPlayer::playbackThread()
{
    while (m_running)
    {
        int   idx   = 0;
        float pitch = 1.0f;
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            m_cv.wait(lk, [this] { return m_hasPending || !m_running; });
            if (!m_running)
                break;
            idx          = m_pendingIdx;
            pitch        = m_pendingPitch;
            m_hasPending = false;
        }

        // 在后台线程中按需加载音效，避免 UI 线程因文件 I/O 阻塞导致画面闪烁
        if (!m_sounds[idx].loaded)
        {
            brls::Sound sound = static_cast<brls::Sound>(idx);
            load(sound);
        }

        if (m_sounds[idx].loaded)
            playSoundDirect(m_sounds[idx], pitch);
        // 若文件缺失则静默跳过
    }
}

// ============================================================
// 平台相关单次播放实现
// ============================================================

// ---- ALSA ----------------------------------------------------
#ifdef BK_AUDIO_ALSA

void BKAudioPlayer::playSoundDirect(const WavData& wav, float /*pitch*/)
{
    snd_pcm_t* handle = nullptr;
    if (snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0) < 0)
        return;

    if (snd_pcm_set_params(handle,
                           SND_PCM_FORMAT_S16_LE,
                           SND_PCM_ACCESS_RW_INTERLEAVED,
                           static_cast<unsigned>(wav.channels),
                           static_cast<unsigned>(wav.sampleRate),
                           1 /*允许重采样*/,
                           ALSA_LATENCY_US) < 0)
    {
        snd_pcm_close(handle);
        return;
    }

    const snd_pcm_sframes_t frames = static_cast<snd_pcm_sframes_t>(
        wav.samples.size() / static_cast<size_t>(wav.channels));
    snd_pcm_sframes_t rc = snd_pcm_writei(handle, wav.samples.data(), frames);
    if (rc == -EPIPE)
        snd_pcm_prepare(handle);

    snd_pcm_drain(handle);
    snd_pcm_close(handle);
}

// ---- WinMM ---------------------------------------------------
#elif defined(BK_AUDIO_WINMM)

void BKAudioPlayer::playSoundDirect(const WavData& wav, float /*pitch*/)
{
    // 构建内存 WAV 文件（RIFF/fmt/data）并通过 PlaySoundA 播放
    const uint32_t dataBytes  = static_cast<uint32_t>(wav.samples.size() * sizeof(int16_t));
    const uint32_t riffSize   = 36 + dataBytes;
    const uint16_t blockAlign = static_cast<uint16_t>(wav.channels * 2);
    const uint32_t byteRate   = static_cast<uint32_t>(wav.sampleRate) * blockAlign;

    std::vector<uint8_t> buf;
    buf.reserve(44 + dataBytes);

    auto push4 = [&](const char* s) {
        buf.push_back(static_cast<uint8_t>(s[0]));
        buf.push_back(static_cast<uint8_t>(s[1]));
        buf.push_back(static_cast<uint8_t>(s[2]));
        buf.push_back(static_cast<uint8_t>(s[3]));
    };
    auto pushU16 = [&](uint16_t v) {
        buf.push_back(v & 0xFF);
        buf.push_back((v >> 8) & 0xFF);
    };
    auto pushU32 = [&](uint32_t v) {
        buf.push_back(v & 0xFF);
        buf.push_back((v >> 8) & 0xFF);
        buf.push_back((v >> 16) & 0xFF);
        buf.push_back((v >> 24) & 0xFF);
    };

    push4("RIFF"); pushU32(riffSize); push4("WAVE");
    push4("fmt "); pushU32(16);
    pushU16(1); pushU16(static_cast<uint16_t>(wav.channels));
    pushU32(static_cast<uint32_t>(wav.sampleRate)); pushU32(byteRate);
    pushU16(blockAlign); pushU16(16);
    push4("data"); pushU32(dataBytes);

    const uint8_t* pcm = reinterpret_cast<const uint8_t*>(wav.samples.data());
    buf.insert(buf.end(), pcm, pcm + dataBytes);

    // SND_SYNC：阻塞后台线程直到播放完成
    PlaySoundA(reinterpret_cast<LPCSTR>(buf.data()), nullptr,
               SND_MEMORY | SND_SYNC | SND_NODEFAULT);
}

// ---- CoreAudio -----------------------------------------------
#elif defined(BK_AUDIO_COREAUDIO)

namespace {

struct CAPlayState
{
    const int16_t*    ptr       = nullptr;
    size_t            remaining = 0; // 剩余立体声帧数
    std::atomic<bool> done      { false };
};

static OSStatus caRenderCallback(void*                       inRefCon,
                                  AudioUnitRenderActionFlags* /*ioFlags*/,
                                  const AudioTimeStamp*       /*inTS*/,
                                  UInt32                      /*inBusNum*/,
                                  UInt32                       inNumFrames,
                                  AudioBufferList*             ioData)
{
    auto* s   = static_cast<CAPlayState*>(inRefCon);
    auto* dst = static_cast<int16_t*>(ioData->mBuffers[0].mData);

    size_t toCopy = std::min(static_cast<size_t>(inNumFrames), s->remaining);
    if (toCopy > 0)
    {
        memcpy(dst, s->ptr, toCopy * 4); // 2通道 × 2字节
        s->ptr       += toCopy * 2;
        s->remaining -= toCopy;
    }
    // 填充回调缓冲区剩余部分为静音
    if (toCopy < inNumFrames)
    {
        memset(dst + toCopy * 2, 0,
               (static_cast<size_t>(inNumFrames) - toCopy) * 4);
        s->done = true;
    }
    return noErr;
}

} // anonymous namespace

void BKAudioPlayer::playSoundDirect(const WavData& wav, float /*pitch*/)
{
    AudioComponentDescription desc{};
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp)
        return;

    AudioUnit au = nullptr;
    if (AudioComponentInstanceNew(comp, &au) != noErr)
        return;

    CAPlayState state;
    state.ptr       = wav.samples.data();
    state.remaining = wav.samples.size() / 2; // 立体声帧数

    AURenderCallbackStruct cb { caRenderCallback, &state };
    AudioUnitSetProperty(au, kAudioUnitProperty_SetRenderCallback,
                         kAudioUnitScope_Input, 0, &cb, sizeof(cb));

    AudioStreamBasicDescription fmt{};
    fmt.mSampleRate       = static_cast<Float64>(wav.sampleRate);
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger
                          | kLinearPCMFormatFlagIsPacked;
    fmt.mFramesPerPacket  = 1;
    fmt.mChannelsPerFrame = 2;
    fmt.mBitsPerChannel   = 16;
    fmt.mBytesPerFrame    = 4;
    fmt.mBytesPerPacket   = 4;
    AudioUnitSetProperty(au, kAudioUnitProperty_StreamFormat,
                         kAudioUnitScope_Input, 0, &fmt, sizeof(fmt));

    if (AudioUnitInitialize(au) != noErr)
    {
        AudioComponentInstanceDispose(au);
        return;
    }
    if (AudioOutputUnitStart(au) != noErr)
    {
        AudioUnitUninitialize(au);
        AudioComponentInstanceDispose(au);
        return;
    }

    // 等待渲染回调耗尽缓冲区
    auto deadline = std::chrono::steady_clock::now() + PLAYBACK_TIMEOUT;
    while (!state.done && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

    AudioOutputUnitStop(au);
    AudioUnitUninitialize(au);
    AudioComponentInstanceDispose(au);
}

// ---- 无音频后端（空实现）-------------------------------------
#else

void BKAudioPlayer::playSoundDirect(const WavData& /*wav*/, float /*pitch*/)
{
    // 无音频后端，静默空操作
}

#endif // 平台后端

} // namespace beiklive
