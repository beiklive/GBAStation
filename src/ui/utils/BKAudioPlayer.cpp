#include "ui/utils/BKAudioPlayer.hpp"

#include "core/common.h"
#include "game/audio/AudioManager.hpp"
#include <borealis/core/logger.hpp>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

// ============================================================
// 平台相关头文件
// ============================================================
#ifdef __SWITCH__
// Switch 平台：使用 libnx audout 播放 WAV 音效
#include <switch.h>

#elif defined(BK_AUDIO_ALSA)
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

std::atomic<bool> BKAudioPlayer::s_gameAudioActive{false};

void BKAudioPlayer::setGameAudioActive(bool active)
{
    s_gameAudioActive.store(active, std::memory_order_release);
}

bool BKAudioPlayer::isGameAudioActive()
{
    return s_gameAudioActive.load(std::memory_order_acquire);
}

// ============================================================
// 平台相关常量
// ============================================================
#ifdef __SWITCH__
// Switch audout 固定输出采样率
static constexpr int SWITCH_OUT_RATE = 48000;

#elif defined(BK_AUDIO_ALSA)
constexpr unsigned ALSA_LATENCY_US = 100000; // 100ms 延迟

#elif defined(BK_AUDIO_COREAUDIO)
constexpr auto    PLAYBACK_TIMEOUT       = std::chrono::seconds(5);
constexpr double  kTrailSilenceSec       = 0.015;  // 尾部静音时长（秒），防止停止时的爆破音
#endif

// ============================================================
// 音效名称表：brls::Sound 枚举 → WAV 文件名映射
// ============================================================
static const char* SOUND_FILE_NAMES[brls::_SOUND_MAX] = {
    nullptr,          // SOUND_NONE
    "SeGiftReceive.wav", // SOUND_FOCUS_CHANGE
    "SeKeyErrorCursor.wav", // SOUND_FOCUS_ERROR
    "SeBtnDecide.wav", // SOUND_CLICK
    "SeFooterDecideFinish.wav", // SOUND_BACK
    "SeNaviFocus.wav", // SOUND_FOCUS_SIDEBAR
    "SeKeyError.wav", // SOUND_CLICK_ERROR
    "SeUnlockKeyZR.wav", // SOUND_HONK
    "SeNaviDecide.wav", // SOUND_CLICK_SIDEBAR
    "SeTouchUnfocus.wav", // SOUND_TOUCH_UNFOCUS
    "SeTouch.wav", // SOUND_TOUCH
    "SeSliderTickOver.wav", // SOUND_SLIDER_TICK
    "SeSliderRelease.wav" // SOUND_SLIDER_RELEASE
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

static float buttonSfxVolume()
{
    return std::clamp(
        static_cast<float>(GET_SETTING_KEY_INT(
            beiklive::SettingKey::KEY_AUDIO_BUTTON_SFX_VOLUME, 100)) / 100.0f,
        0.0f, 1.0f);
}

static float masterVolume()
{
    return std::clamp(
        static_cast<float>(GET_SETTING_KEY_INT(
            beiklive::SettingKey::KEY_AUDIO_MASTER_VOLUME, 100)) / 100.0f,
        0.0f, 1.0f);
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
// Switch 平台：初始化 audout 服务
// ============================================================

#ifdef __SWITCH__
void BKAudioPlayer::_initSwitch()
{
    // 初始化 audout 服务（引用计数，允许多次调用）
    Result rc = audoutInitialize();
    if (R_FAILED(rc))
    {
        brls::Logger::warning("BKAudioPlayer: audout服务初始化失败: {:#x}", rc);
        return;
    }

    // 启动音频输出（若已由其他模块启动，忽略错误继续）
    rc = audoutStartAudioOut();
    if (R_FAILED(rc))
    {
        brls::Logger::debug("BKAudioPlayer: audout已由其他模块启动（{:#x}），使用共享流", rc);
    }

    m_switchInit = true;
    brls::Logger::info("BKAudioPlayer: Switch WAV播放器初始化成功（audout）");
}
#endif // __SWITCH__

BKAudioPlayer::BKAudioPlayer()
{
#ifdef __SWITCH__
    _initSwitch();
#endif // __SWITCH__

#ifdef BK_AUDIO_COREAUDIO
    _setupCAUnit();
#endif

    m_running = true;
    m_thread  = std::thread(&BKAudioPlayer::playbackThread, this);
}

BKAudioPlayer::~BKAudioPlayer()
{
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable())
        m_thread.join();

#ifdef __SWITCH__
    if (m_switchInit)
    {
        audoutStopAudioOut();
        audoutExit();
        m_switchInit = false;
    }
#endif // __SWITCH__

#ifdef BK_AUDIO_COREAUDIO
    if (m_caUnit)
    {
        AudioUnitUninitialize(m_caUnit);
        AudioComponentInstanceDispose(m_caUnit);
        m_caUnit = nullptr;
    }
#endif
}

// ============================================================
// CoreAudio AudioUnit 预初始化
// ============================================================

#ifdef BK_AUDIO_COREAUDIO
void BKAudioPlayer::_setupCAUnit()
{
    AudioComponentDescription desc{};
    desc.componentType         = kAudioUnitType_Output;
    desc.componentSubType      = kAudioUnitSubType_DefaultOutput;
    desc.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent comp = AudioComponentFindNext(nullptr, &desc);
    if (!comp) {
        brls::Logger::error("BKAudioPlayer: 找不到默认音频输出组件");
        return;
    }

    OSStatus status = AudioComponentInstanceNew(comp, &m_caUnit);
    if (status != noErr) {
        brls::Logger::error("BKAudioPlayer: 创建AudioUnit实例失败 ({})", static_cast<int>(status));
        m_caUnit = nullptr;
        return;
    }

    // 设置默认流格式（实际播放时按需调整）
    AudioStreamBasicDescription fmt{};
    fmt.mSampleRate       = 44100.0;
    fmt.mFormatID         = kAudioFormatLinearPCM;
    fmt.mFormatFlags      = kLinearPCMFormatFlagIsSignedInteger
                          | kLinearPCMFormatFlagIsPacked;
    fmt.mFramesPerPacket  = 1;
    fmt.mChannelsPerFrame = 2;
    fmt.mBitsPerChannel   = 16;
    fmt.mBytesPerFrame    = 4;
    fmt.mBytesPerPacket   = 4;

    status = AudioUnitSetProperty(m_caUnit, kAudioUnitProperty_StreamFormat,
                                   kAudioUnitScope_Input, 0,
                                   &fmt, sizeof(fmt));
    if (status != noErr) {
        brls::Logger::error("BKAudioPlayer: 设置AudioUnit流格式失败 ({})", static_cast<int>(status));
        AudioComponentInstanceDispose(m_caUnit);
        m_caUnit = nullptr;
        return;
    }

    status = AudioUnitInitialize(m_caUnit);
    if (status != noErr) {
        brls::Logger::error("BKAudioPlayer: AudioUnit初始化失败 ({})", static_cast<int>(status));
        AudioComponentInstanceDispose(m_caUnit);
        m_caUnit = nullptr;
        return;
    }

    brls::Logger::info("BKAudioPlayer: CoreAudio AudioUnit预初始化完成");
}
#endif // BK_AUDIO_COREAUDIO

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

    // 所有平台统一从 WAV 文件加载
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

        if (m_sounds[idx].loaded) {
            // The stored WAV remains immutable so changing the selector takes
            // effect on the very next UI sound without cumulative scaling.
            WavData playback = m_sounds[idx];
            const float volume = buttonSfxVolume() * masterVolume();
            if (volume < 0.999f) {
                for (auto& sample : playback.samples) {
                    const int scaled = static_cast<int>(std::lround(
                        static_cast<float>(sample) * volume));
                    sample = static_cast<int16_t>(std::clamp(scaled, -32768, 32767));
                }
            }
            playSoundDirect(idx, playback, pitch);
        }
        // 若文件缺失则静默跳过
    }
}

// ============================================================
// 平台相关单次播放实现
// ============================================================

// ---- Switch (libnx audout + WAV) ----------------------------
#ifdef __SWITCH__

void BKAudioPlayer::playSoundDirect(int /*soundIdx*/, const WavData& wav, float pitch)
{
    if (!m_switchInit || wav.samples.empty())
        return;

    // 游戏音频系统运行时跳过UI音效播放：
    // AudioManager 与 BKAudioPlayer 共用同一 audout 设备，若同时向硬件队列提交缓冲区，
    // AudioManager 的线程可能"偷走" BKAudioPlayer 的完成通知，导致 BKAudioPlayer 在
    // 音频缓冲区仍被硬件 DMA 读取时提前 free()，产生 use-after-free，引发撕裂音或音调异常。
    if (AudioManager::instance().isRunning() || BKAudioPlayer::isGameAudioActive())
        return;

    // pitch 影响重采样比率：pitch > 1.0 表示升调（加速播放），< 1.0 表示降调（减速播放）
    double effectivePitch = (pitch > 0.1f) ? static_cast<double>(pitch) : 1.0;

    // 计算输入帧数（输入可能是单/双声道）
    size_t inFrames = wav.samples.size() / static_cast<size_t>(wav.channels);

    // 最近邻重采样：输入采样率 * pitch → 48000Hz（Switch audout固定输出率）
    // pitch > 1 时采样更快（音调升高），< 1 时采样更慢（音调降低）
    double ratio = static_cast<double>(SWITCH_OUT_RATE) / (wav.sampleRate * effectivePitch);
    size_t outFrames = static_cast<size_t>(inFrames * ratio + 0.5);
    if (outFrames == 0)
        return;

    // Switch要求 AudioOutBuffer 的 buffer 字段按 0x1000 字节对齐
    size_t dataBytes    = outFrames * 2 * sizeof(int16_t); // 固定双声道输出
    size_t alignedBytes = (dataBytes + 0xFFF) & ~(size_t)0xFFF;

    void* rawBuf = aligned_alloc(0x1000, alignedBytes);
    if (!rawBuf)
    {
        brls::Logger::error("BKAudioPlayer: Switch音效缓冲区内存分配失败（{}字节）", alignedBytes);
        return;
    }
    memset(rawBuf, 0, alignedBytes);

    // 最近邻重采样并转换为双声道
    int16_t* dst = static_cast<int16_t*>(rawBuf);
    for (size_t i = 0; i < outFrames; ++i)
    {
        size_t srcFrame = static_cast<size_t>(i / ratio);
        if (srcFrame >= inFrames)
            srcFrame = inFrames - 1;
        int16_t L = wav.samples[srcFrame * wav.channels];
        int16_t R = (wav.channels > 1) ? wav.samples[srcFrame * wav.channels + 1] : L;
        dst[i * 2]     = L;
        dst[i * 2 + 1] = R;
    }

    // 提交缓冲区到 audout 队列播放
    AudioOutBuffer buf = {};
    buf.next        = nullptr;
    buf.buffer      = rawBuf;
    buf.buffer_size = alignedBytes;
    buf.data_size   = dataBytes;
    buf.data_offset = 0;

    if (R_SUCCEEDED(audoutAppendAudioOutBuffer(&buf)))
    {
        // 标记正在播放：外部系统（如 AudioManager::init）可通过 isPlaying() 等待本缓冲区完成
        m_isPlaying.store(true, std::memory_order_release);
        // 等待音频播放完成，超时为音频时长的2倍 + 200ms
        u64 waitNs = static_cast<u64>(inFrames) * 2000000000ULL / wav.sampleRate + 200000000ULL;
        AudioOutBuffer* released = nullptr;
        u32 relCount = 0;
        audoutWaitPlayFinish(&released, &relCount, waitNs);
        m_isPlaying.store(false, std::memory_order_release);
    }

    free(rawBuf);
}

// ---- ALSA ---------------------------------------------------
#elif defined(BK_AUDIO_ALSA)

void BKAudioPlayer::playSoundDirect(int /*soundIdx*/, const WavData& wav, float /*pitch*/)
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

void BKAudioPlayer::playSoundDirect(int /*soundIdx*/, const WavData& wav, float /*pitch*/)
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

/// CoreAudio 渲染回调的播放状态
///
/// 渲染回调由 CoreAudio 高优先级实时线程调用，不可阻塞或加锁。
/// 通过 std::atomic 标记完成状态，播放线程在 playSoundDirect 中轮询。
struct CAPlayState
{
    const int16_t*    audioPtr    = nullptr;  ///< 当前读取位置
    size_t            audioRemain = 0;        ///< 剩余音频立体声帧数
    size_t            trailRemain = 0;        ///< 剩余尾部静音帧数
    std::atomic<bool> finished{false};        ///< 音频 + 尾部静音均已输出完成
};

/// 尾部静音帧数（根据采样率计算）
inline size_t trailSilenceFrames(double sampleRate)
{
    return static_cast<size_t>(sampleRate * kTrailSilenceSec);
}

/// CoreAudio 渲染回调：按需输出音频数据，耗尽后填充尾部静音再标记完成
static OSStatus caRenderCallback(void*                       inRefCon,
                                  AudioUnitRenderActionFlags* /*ioFlags*/,
                                  const AudioTimeStamp*       /*inTS*/,
                                  UInt32                      /*inBusNum*/,
                                  UInt32                       inNumFrames,
                                  AudioBufferList*             ioData)
{
    auto* s   = static_cast<CAPlayState*>(inRefCon);
    auto* dst = static_cast<int16_t*>(ioData->mBuffers[0].mData);

    // 先写入剩余音频数据
    size_t toCopy = std::min(static_cast<size_t>(inNumFrames), s->audioRemain);
    if (toCopy > 0) {
        memcpy(dst, s->audioPtr, toCopy * 4); // 2ch × 2B = 4B/frame
        s->audioPtr    += toCopy * 2;
        s->audioRemain -= toCopy;
    }

    size_t filled = toCopy;

    // 音频耗尽后，填充尾部静音以平滑硬件管线末端
    if (filled < inNumFrames) {
        size_t silenceNeeded = inNumFrames - filled;
        size_t trail = std::min(silenceNeeded, s->trailRemain);
        if (trail > 0) {
            memset(dst + filled * 2, 0, trail * 4);
            filled += trail;
            s->trailRemain -= trail;
        }
        // 尾部静音也已耗尽，剩余全部填充静音并标记完成
        if (filled < inNumFrames) {
            memset(dst + filled * 2, 0, (inNumFrames - filled) * 4);
            s->finished.store(true, std::memory_order_release);
        }
    }

    return noErr;
}

} // anonymous namespace

void BKAudioPlayer::playSoundDirect(int /*soundIdx*/, const WavData& wav, float /*pitch*/)
{
    // 复用预创建的 AudioUnit，避免每次播放重新创建导致延迟
    if (!m_caUnit)
        return;

    // 设置当前音效的流格式（不同 WAV 文件采样率可能不同）
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

    OSStatus status = AudioUnitSetProperty(m_caUnit,
                                           kAudioUnitProperty_StreamFormat,
                                           kAudioUnitScope_Input, 0,
                                           &fmt, sizeof(fmt));
    if (status != noErr) {
        brls::Logger::warning("BKAudioPlayer: 设置流格式失败 ({})", static_cast<int>(status));
        return;
    }

    // 设置渲染回调与播放状态
    CAPlayState state;
    state.audioPtr    = wav.samples.data();
    state.audioRemain = wav.samples.size() / 2; // 立体声帧数
    state.trailRemain = trailSilenceFrames(static_cast<double>(wav.sampleRate));

    AURenderCallbackStruct cb{ caRenderCallback, &state };
    status = AudioUnitSetProperty(m_caUnit,
                                  kAudioUnitProperty_SetRenderCallback,
                                  kAudioUnitScope_Input, 0,
                                  &cb, sizeof(cb));
    if (status != noErr) {
        brls::Logger::warning("BKAudioPlayer: 设置渲染回调失败 ({})", static_cast<int>(status));
        return;
    }

    // 启动播放
    status = AudioOutputUnitStart(m_caUnit);
    if (status != noErr) {
        brls::Logger::warning("BKAudioPlayer: 启动AudioUnit失败 ({})", static_cast<int>(status));
        return;
    }

    // 等待音频数据与尾部静音全部输出完毕
    auto deadline = std::chrono::steady_clock::now() + PLAYBACK_TIMEOUT;
    while (!state.finished.load(std::memory_order_acquire)
           && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::yield();
    }

    // 停止播放（保留 AudioUnit 实例供下次复用）
    AudioOutputUnitStop(m_caUnit);
}

// ---- 无音频后端（空实现）-------------------------------------
#else

void BKAudioPlayer::playSoundDirect(int /*soundIdx*/, const WavData& /*wav*/, float /*pitch*/)
{
    // 无音频后端，静默空操作
}

#endif // 平台后端

} // namespace beiklive
