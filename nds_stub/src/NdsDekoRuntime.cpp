#include "nds_stub/NdsDekoRuntime.hpp"

#include "nds_stub/NdsGameLayer.hpp"
#include "nds_stub/NdsMenuLayer.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>

#include <switch.h>

#ifdef OGLRENDERER_ENABLED
#undef OGLRENDERER_ENABLED
#endif

#include "../../third_party/ArcDelta_melonDS/src/Config.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU2D_Deko.h"
#include "../../third_party/ArcDelta_melonDS/src/NDS.h"
#include "../../third_party/ArcDelta_melonDS/src/NDSCart.h"
#include "../../third_party/ArcDelta_melonDS/src/Platform.h"
#include "../../third_party/ArcDelta_melonDS/src/SPU.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/PlatformConfig.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"
#include "nds_stub/StubLog.hpp"

namespace {

constexpr uint32_t kNdsKeyA      = 1u << 0;
constexpr uint32_t kNdsKeyB      = 1u << 1;
constexpr uint32_t kNdsKeySelect = 1u << 2;
constexpr uint32_t kNdsKeyStart  = 1u << 3;
constexpr uint32_t kNdsKeyRight  = 1u << 4;
constexpr uint32_t kNdsKeyLeft   = 1u << 5;
constexpr uint32_t kNdsKeyUp     = 1u << 6;
constexpr uint32_t kNdsKeyDown   = 1u << 7;
constexpr uint32_t kNdsKeyR      = 1u << 8;
constexpr uint32_t kNdsKeyL      = 1u << 9;
constexpr uint32_t kNdsKeyX      = 1u << 10;
constexpr uint32_t kNdsKeyY      = 1u << 11;

std::string pathStem(const std::string& path)
{
    std::string stem = std::filesystem::path(path).stem().string();
    return stem.empty() ? "game" : stem;
}

std::string joinPath(const std::string& dir, const std::string& name)
{
    if (dir.empty())
        return name;
    return (std::filesystem::path(dir) / name).string();
}

std::string defaultSaveDir(const std::string& romPath)
{
    return joinPath(joinPath("sdmc:/GBAStation/save/NDS", pathStem(romPath)), "");
}

std::string resolveSavePath(const beiklive::nds_stub::DekoRunOptions& options)
{
    const std::string saveDir = options.savePath.empty() ? defaultSaveDir(options.romPath) : options.savePath;
    std::error_code ec;
    std::filesystem::create_directories(saveDir, ec);
    return joinPath(saveDir, pathStem(options.romPath) + ".sav");
}

bool fileExists(const char* path)
{
    FILE* fp = std::fopen(path, "rb");
    if (!fp)
        return false;
    std::fclose(fp);
    return true;
}

long long elapsedMs(std::chrono::steady_clock::time_point begin)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - begin).count();
}

bool touchScreenPressed()
{
    HidTouchScreenState state {};
    return hidGetTouchScreenStates(&state, 1) && state.count > 0;
}

uint32_t dsKeyMaskFromPad(const PadState& pad)
{
    uint32_t mask = 0x0FFFu;
    const u64 buttons = padGetButtons(&pad);

    auto press = [&](u64 hid, uint32_t ndsBit) {
        if (buttons & hid)
            mask &= ~ndsBit;
    };

    press(HidNpadButton_A, kNdsKeyA);
    press(HidNpadButton_B, kNdsKeyB);
    press(HidNpadButton_X, kNdsKeyX);
    press(HidNpadButton_Y, kNdsKeyY);
    press(HidNpadButton_L, kNdsKeyL);
    press(HidNpadButton_R, kNdsKeyR);
    press(HidNpadButton_ZL, kNdsKeyL);
    press(HidNpadButton_Plus, kNdsKeyStart);
    press(HidNpadButton_Minus, kNdsKeySelect);
    press(HidNpadButton_StickL, kNdsKeySelect);
    press(HidNpadButton_AnyUp, kNdsKeyUp);
    press(HidNpadButton_AnyDown, kNdsKeyDown);
    press(HidNpadButton_AnyLeft, kNdsKeyLeft);
    press(HidNpadButton_AnyRight, kNdsKeyRight);
    return mask;
}

bool setReturnNro(const std::string& returnNro)
{
    if (returnNro.empty())
        return false;

    std::string quoted = "\"";
    for (char c : returnNro)
    {
        if (c == '"' || c == '\\')
            quoted.push_back('\\');
        quoted.push_back(c);
    }
    quoted.push_back('"');

    const Result rc = envSetNextLoad(returnNro.c_str(), quoted.c_str());
    beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko envSetNextLoad rc=0x%x path=%s",
                                      rc,
                                      returnNro.c_str());
    return R_SUCCEEDED(rc);
}

void configureArcDelta()
{
    std::strncpy(Config::BIOS9Path, "sdmc:/GBAStation/bios/nds/bios9.bin", sizeof(Config::BIOS9Path) - 1);
    std::strncpy(Config::BIOS7Path, "sdmc:/GBAStation/bios/nds/bios7.bin", sizeof(Config::BIOS7Path) - 1);
    std::strncpy(Config::FirmwarePath, "sdmc:/GBAStation/bios/nds/firmware.bin", sizeof(Config::FirmwarePath) - 1);
    Config::DLDIEnable = 0;
    Config::RandomizeMAC = 0;

#ifdef JIT_ENABLED
    Config::JIT_Enable = 1;
    Config::JIT_MaxBlockSize = 32;
    Config::JIT_BranchOptimisations = 1;
    Config::JIT_LiteralOptimisations = 1;
    Config::JIT_FastMemory = 1;
#endif

    Config::ConsoleType = 0;
    Config::DirectBoot = 1;
}

class DekoAudioOutput {
public:
    bool start()
    {
        if (m_running.load(std::memory_order_acquire))
            return true;

        const AudioRendererConfig config = {
            .output_rate = AudioRendererOutputRate_48kHz,
            .num_voices = 4,
            .num_effects = 0,
            .num_sinks = 1,
            .num_mix_objs = 1,
            .num_mix_buffers = 2,
        };

        Result rc = audrenInitialize(&config);
        if (R_FAILED(rc))
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrenInitialize failed rc=0x%x", rc);
            return false;
        }

        rc = audrvCreate(&m_driver, &config, 2);
        if (R_FAILED(rc))
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrvCreate failed rc=0x%x", rc);
            audrenExit();
            return false;
        }

        m_memPool = std::aligned_alloc(AUDREN_MEMPOOL_ALIGNMENT, kPoolBytes);
        if (!m_memPool)
        {
            audrvClose(&m_driver);
            audrenExit();
            return false;
        }
        std::memset(m_memPool, 0, kPoolBytes);

        m_memPoolId = audrvMemPoolAdd(&m_driver, m_memPool, kPoolBytes);
        audrvMemPoolAttach(&m_driver, m_memPoolId);

        static const u8 sinkChannels[] = {0, 1};
        audrvDeviceSinkAdd(&m_driver, AUDREN_DEFAULT_DEVICE_NAME, 2, sinkChannels);
        audrvUpdate(&m_driver);

        rc = audrenStartAudioRenderer();
        if (R_FAILED(rc))
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrenStartAudioRenderer rc=0x%x", rc);

        if (!audrvVoiceInit(&m_driver, 0, 2, PcmFormat_Int16, kInputSampleRate))
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audrvVoiceInit failed");

        audrvVoiceSetDestinationMix(&m_driver, 0, AUDREN_FINAL_MIX_ID);
        audrvVoiceSetMixFactor(&m_driver, 0, 1.0f, 0, 0);
        audrvVoiceSetMixFactor(&m_driver, 0, 1.0f, 1, 1);
        audrvVoiceStart(&m_driver, 0);

        m_running.store(true, std::memory_order_release);
        m_thread = std::thread(&DekoAudioOutput::threadMain, this);
        return true;
    }

    void stop()
    {
        if (m_running.exchange(false, std::memory_order_acq_rel))
        {
            if (m_thread.joinable())
                m_thread.join();
        }

        audrvClose(&m_driver);
        audrenExit();

        if (m_memPool)
        {
            std::free(m_memPool);
            m_memPool = nullptr;
        }
    }

    void pauseForCoreReset()
    {
        m_paused.store(true, std::memory_order_release);

        // Wait until any in-flight SPU::ReadOutput() call has left the melonDS
        // audio buffer before NDS::LoadROM()/NDS::Reset() reinitializes SPU.
        std::lock_guard<std::mutex> lock(m_spuReadMutex);
    }

    void resumeAfterCoreReset()
    {
        m_paused.store(false, std::memory_order_release);
    }

    void setFastForwardActive(bool enabled)
    {
        if (m_fastForwardAudio.exchange(enabled, std::memory_order_acq_rel) == enabled)
            return;

        std::lock_guard<std::mutex> lock(m_spuReadMutex);
        if (enabled)
        {
            SPU::TrimOutput();
        }
        else
        {
            SPU::DrainOutput();
        }
    }

    void push(const int16_t* samples, size_t stereoFrames)
    {
        (void)samples;
        (void)stereoFrames;
    }

private:
    static constexpr int kInputSampleRate = 32823;
    static constexpr size_t kBufferFrames = 768;
    static constexpr size_t kBufferCount = 2;
    static constexpr size_t kBufferBytes = kBufferFrames * 2 * sizeof(int16_t);
    static constexpr size_t kPoolBytes = (kBufferBytes * kBufferCount + (AUDREN_MEMPOOL_ALIGNMENT - 1)) &
                                         ~(AUDREN_MEMPOOL_ALIGNMENT - 1);

    void threadMain()
    {
        std::array<AudioDriverWaveBuf, kBufferCount> buffers {};
        for (size_t i = 0; i < kBufferCount; ++i)
        {
            buffers[i].data_pcm16 = static_cast<int16_t*>(m_memPool);
            buffers[i].size = kBufferBytes;
            buffers[i].start_sample_offset = static_cast<u32>(i * kBufferFrames);
            buffers[i].end_sample_offset = static_cast<u32>((i + 1) * kBufferFrames);
        }

        while (m_running.load(std::memory_order_acquire))
        {
            if (m_paused.load(std::memory_order_acquire))
            {
                svcSleepThread(1000000);
                continue;
            }

            AudioDriverWaveBuf* refill = nullptr;
            for (auto& buffer : buffers)
            {
                if (buffer.state == AudioDriverWaveBufState_Free ||
                    buffer.state == AudioDriverWaveBufState_Done)
                {
                    refill = &buffer;
                    break;
                }
            }

            if (refill)
            {
                auto* data = static_cast<int16_t*>(m_memPool) + refill->start_sample_offset * 2;

                int frames = 0;
                while (m_running.load(std::memory_order_acquire) &&
                       !m_paused.load(std::memory_order_acquire))
                {
                    {
                        std::lock_guard<std::mutex> lock(m_spuReadMutex);
                        if (m_fastForwardAudio.load(std::memory_order_acquire))
                            SPU::Sync(false);
                        frames = SPU::ReadOutput(data, static_cast<int>(kBufferFrames));
                    }
                    if (frames > 0)
                        break;
                    svcSleepThread(10000);
                }

                if (frames > 0)
                {
                    const u32 lastStereo = reinterpret_cast<u32*>(data)[frames - 1];
                    while (frames < static_cast<int>(kBufferFrames))
                        reinterpret_cast<u32*>(data)[frames++] = lastStereo;

                    armDCacheFlush(data, frames * 2 * sizeof(int16_t));
                    refill->end_sample_offset = refill->start_sample_offset + frames;
                    audrvVoiceAddWaveBuf(&m_driver, 0, refill);
                    audrvVoiceStart(&m_driver, 0);
                }
            }

            audrvUpdate(&m_driver);
            audrenWaitFrame();
        }
    }

    std::atomic<bool> m_running{false};
    std::atomic<bool> m_paused{false};
    std::atomic<bool> m_fastForwardAudio{false};
    std::mutex m_spuReadMutex;
    std::thread m_thread;
    AudioDriver m_driver {};
    void* m_memPool = nullptr;
    int m_memPoolId = -1;
};

} // namespace

namespace Platform {

void Init(int, char**) {}
void DeInit() {}
void StopEmu() {}

FILE* OpenFile(const char* path, const char* mode, bool mustexist)
{
    if (mustexist)
    {
        FILE* fp = std::fopen(path, "rb");
        if (!fp)
            return nullptr;
        return std::freopen(path, mode, fp);
    }
    return std::fopen(path, mode);
}

FILE* OpenLocalFile(const char* path, const char* mode)
{
    if (!path || !path[0])
        return nullptr;

    if (std::strncmp(path, "sdmc:/", 6) == 0 || path[0] == '/')
        return std::fopen(path, mode);

    char finalPath[1024];
    std::snprintf(finalPath, sizeof(finalPath), "sdmc:/GBAStation/bios/nds/%s", path);
    FILE* fp = std::fopen(finalPath, mode);
    if (fp)
        return fp;

    std::snprintf(finalPath, sizeof(finalPath), "/GBAStation/bios/nds/%s", path);
    fp = std::fopen(finalPath, mode);
    if (fp)
        return fp;

    return std::fopen(path, mode);
}

FILE* OpenDataFile(const char* path)
{
    return OpenLocalFile(path, "rb");
}

void Sleep(u64 usecs)
{
    svcSleepThread(usecs * 1000);
}

struct ThreadEntryData {
    std::function<void()> entryPoint;
};

void ThreadEntry(void* param)
{
    ThreadEntryData* data = static_cast<ThreadEntryData*>(param);
    data->entryPoint();
    delete data;
}

Thread* Thread_Create(std::function<void()> func)
{
    ::Thread* thread = new ::Thread();
    threadCreate(thread, ThreadEntry, new ThreadEntryData{std::move(func)}, nullptr, 1024 * 1024 * 2, 0x30, -2);
    threadStart(thread);
    return reinterpret_cast<Thread*>(thread);
}

void Thread_Free(Thread* thread)
{
    threadClose(reinterpret_cast<::Thread*>(thread));
    delete reinterpret_cast<::Thread*>(thread);
}

void Thread_Wait(Thread* thread)
{
    threadWaitForExit(reinterpret_cast<::Thread*>(thread));
}

struct MySemaphore {
    ::CondVar condvar;
    ::Mutex mutex;
    u64 count;
};

Semaphore* Semaphore_Create()
{
    MySemaphore* sema = new MySemaphore();
    sema->count = 0;
    mutexInit(&sema->mutex);
    condvarInit(&sema->condvar);
    return reinterpret_cast<Semaphore*>(sema);
}

void Semaphore_Free(Semaphore* sema)
{
    delete reinterpret_cast<MySemaphore*>(sema);
}

void Semaphore_Reset(Semaphore* sema)
{
    MySemaphore* s = reinterpret_cast<MySemaphore*>(sema);
    mutexLock(&s->mutex);
    s->count = 0;
    mutexUnlock(&s->mutex);
}

void Semaphore_Wait(Semaphore* sema)
{
    MySemaphore* s = reinterpret_cast<MySemaphore*>(sema);
    mutexLock(&s->mutex);
    while (s->count == 0)
        condvarWait(&s->condvar, &s->mutex);
    --s->count;
    mutexUnlock(&s->mutex);
}

void Semaphore_Post(Semaphore* sema, int count)
{
    if (count <= 0)
        return;
    MySemaphore* s = reinterpret_cast<MySemaphore*>(sema);
    mutexLock(&s->mutex);
    s->count += static_cast<u64>(count);
    mutexUnlock(&s->mutex);
    condvarWake(&s->condvar, count);
}

Mutex* Mutex_Create()
{
    ::Mutex* mutex = new ::Mutex();
    mutexInit(mutex);
    return reinterpret_cast<Mutex*>(mutex);
}

void Mutex_Free(Mutex* mutex)
{
    delete reinterpret_cast<::Mutex*>(mutex);
}

void Mutex_Lock(Mutex* mutex)
{
    mutexLock(reinterpret_cast<::Mutex*>(mutex));
}

void Mutex_Unlock(Mutex* mutex)
{
    mutexUnlock(reinterpret_cast<::Mutex*>(mutex));
}

bool Mutex_TryLock(Mutex* mutex)
{
    return mutexTryLock(reinterpret_cast<::Mutex*>(mutex));
}

bool MP_Init() { return false; }
void MP_DeInit() {}
int MP_SendPacket(u8*, int) { return 0; }
int MP_RecvPacket(u8*, bool) { return 0; }
bool LAN_Init() { return false; }
void LAN_DeInit() {}
int LAN_SendPacket(u8*, int) { return 0; }
int LAN_RecvPacket(u8*) { return 0; }

} // namespace Platform

namespace beiklive::nds_stub {

bool ShouldUseDekoRuntime()
{
    if (fileExists("sdmc:/GBAStation/config/nds_stub_software.flag") ||
        fileExists("/GBAStation/config/nds_stub_software.flag"))
    {
        appendStubLog("GBAStationNDSStub: Deko runtime disabled by nds_stub_software.flag");
        return false;
    }
    return true;
}

int RunDekoRuntime(const DekoRunOptions& options)
{
    appendStubLog("GBAStationNDSStub: Deko runtime start rom=%s", options.romPath.c_str());
    if (options.romPath.empty())
        return 1;

    if (!fileExists("sdmc:/GBAStation/bios/nds/bios9.bin") ||
        !fileExists("sdmc:/GBAStation/bios/nds/bios7.bin") ||
        !fileExists("sdmc:/GBAStation/bios/nds/firmware.bin"))
    {
        appendStubLog("GBAStationNDSStub: Deko runtime missing DS BIOS/firmware");
        return 1;
    }

    const std::string savePath = resolveSavePath(options);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    hidInitializeTouchScreen();
    PadState pad;
    padInitializeDefault(&pad);

    if (R_FAILED(romfsInit()))
    {
        appendStubLog("GBAStationNDSStub: Deko romfsInit failed");
        return 1;
    }

    auto checkpointBegin = std::chrono::steady_clock::now();
    configureArcDelta();
    appendStubLog("GBAStationNDSStub: Deko checkpoint config done ms=%lld", elapsedMs(checkpointBegin));

    appendStubLog("GBAStationNDSStub: Deko checkpoint Gfx::Init begin");
    checkpointBegin = std::chrono::steady_clock::now();
    Gfx::Init();
    appendStubLog("GBAStationNDSStub: Deko checkpoint Gfx::Init ok ms=%lld", elapsedMs(checkpointBegin));
    appendStubLog("GBAStationNDSStub: Deko checkpoint NDS::Init begin");
    checkpointBegin = std::chrono::steady_clock::now();
    NDS::Init();
    appendStubLog("GBAStationNDSStub: Deko checkpoint NDS::Init ok ms=%lld", elapsedMs(checkpointBegin));
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::InitRenderer begin");
    checkpointBegin = std::chrono::steady_clock::now();
    GPU::InitRenderer(0);
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::InitRenderer ok ms=%lld", elapsedMs(checkpointBegin));
    GPU::RenderSettings settings {true, 1, false};
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::SetRenderSettings begin");
    checkpointBegin = std::chrono::steady_clock::now();
    GPU::SetRenderSettings(0, settings);
    appendStubLog("GBAStationNDSStub: Deko checkpoint GPU::SetRenderSettings ok ms=%lld", elapsedMs(checkpointBegin));

    auto* deko2d = static_cast<GPU2D::DekoRenderer*>(GPU::GPU2D_Renderer.get());
    NdsGameLayer gameLayer;
    appendStubLog("GBAStationNDSStub: Deko checkpoint gameLayer.init begin renderer=%p", deko2d);
    checkpointBegin = std::chrono::steady_clock::now();
    gameLayer.init(deko2d);
    gameLayer.setWaitForFramebufferReady(false);
    appendStubLog("GBAStationNDSStub: Deko display fence mode=signal-presented-only");
    appendStubLog("GBAStationNDSStub: Deko checkpoint gameLayer.init ok ms=%lld", elapsedMs(checkpointBegin));

    appendStubLog("GBAStationNDSStub: Deko checkpoint LoadROM begin");
    checkpointBegin = std::chrono::steady_clock::now();
    bool loaded = NDS::LoadROM(options.romPath.c_str(), savePath.c_str(), true);
    appendStubLog("GBAStationNDSStub: Deko LoadROM loaded=%d ms=%lld save=%s",
                  loaded ? 1 : 0,
                  elapsedMs(checkpointBegin),
                  savePath.c_str());

    DekoAudioOutput audio;
    appendStubLog("GBAStationNDSStub: Deko checkpoint audio.start begin");
    checkpointBegin = std::chrono::steady_clock::now();
    const bool audioStarted = audio.start();
    appendStubLog("GBAStationNDSStub: Deko checkpoint audio.start result=%d ms=%lld",
                  audioStarted ? 1 : 0,
                  elapsedMs(checkpointBegin));

    bool running = loaded;
    bool pendingReturn = false;
    NdsMenuLayer menuLayer;
    double fps = 0.0;
    int fpsFrames = 0;
    uint64_t totalFrames = 0;
    long long lastRunMs = 0;
    auto fpsStart = std::chrono::steady_clock::now();
    bool blockGameInputUntilRelease = false;
    bool lastFastForwardActive = false;
    int currentResolutionScale = 1;

    auto applyResolutionScale = [&](int scale) {
        scale = std::clamp(scale, 1, 4);
        if (scale == currentResolutionScale)
            return;

        appendStubLog("GBAStationNDSStub: Deko resolution scale request x%d", scale);
        if (scale != 1)
        {
            appendStubLog("GBAStationNDSStub: Deko resolution scale x%d temporarily disabled; runtime stays x1", scale);
            currentResolutionScale = 1;
            return;
        }
        Gfx::PresentQueue.waitIdle();
        Gfx::EmuQueue.waitIdle();

        GPU::RenderSettings newSettings {true, scale, false};
        GPU::SetRenderSettings(0, newSettings);
        deko2d->SetScaleFactor(scale);
        currentResolutionScale = scale;
        appendStubLog("GBAStationNDSStub: Deko resolution scale request accepted x%d", scale);
    };

    while (appletMainLoop() && running)
    {
        const bool traceFrame = totalFrames < 5;
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu loop begin",
                          static_cast<unsigned long long>(totalFrames));
        const auto frameBegin = std::chrono::steady_clock::now();
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);
        const u64 held = padGetButtons(&pad);

        const bool wasMenuVisible = menuLayer.visible();
        const NdsMenuAction menuAction = menuLayer.update(down);
        if (wasMenuVisible != menuLayer.visible())
            blockGameInputUntilRelease = true;

        if (menuAction == NdsMenuAction::DisplaySettingsChanged)
        {
            gameLayer.setLinearFiltering(menuLayer.linearFiltering());
            applyResolutionScale(menuLayer.resolutionScale());
            appendStubLog("GBAStationNDSStub: Deko display settings filter=%s ff=x%d res=x%d",
                          menuLayer.linearFiltering() ? "linear" : "nearest",
                          menuLayer.fastForwardMultiplier(),
                          menuLayer.resolutionScale());
        }
        if (menuAction == NdsMenuAction::ResetGame)
        {
            appendStubLog("GBAStationNDSStub: Deko reset begin");
            menuLayer.close();
            NDS::SetKeyMask(0x0FFFu);
            NDS::ReleaseScreen();

            audio.pauseForCoreReset();
            Gfx::PresentQueue.waitIdle();
            Gfx::EmuQueue.waitIdle();
            NDSCart::FlushSRAMFile();

            loaded = NDS::LoadROM(options.romPath.c_str(), savePath.c_str(), true);
            appendStubLog("GBAStationNDSStub: Deko reset LoadROM loaded=%d", loaded ? 1 : 0);
            audio.resumeAfterCoreReset();

            if (!loaded)
            {
                running = false;
                continue;
            }

            fps = 0.0;
            fpsFrames = 0;
            lastRunMs = 0;
            fpsStart = std::chrono::steady_clock::now();
            continue;
        }
        else if (menuAction == NdsMenuAction::ExitGame)
        {
            pendingReturn = true;
            running = false;
        }

        const bool menuVisible = menuLayer.visible();
        const bool touchHeld = touchScreenPressed();
        if (blockGameInputUntilRelease && held == 0 && !touchHeld)
            blockGameInputUntilRelease = false;
        const bool suppressGameInput = menuVisible || blockGameInputUntilRelease;
        const bool fastForwardActive =
            !suppressGameInput &&
            menuLayer.fastForwardMultiplier() > 1;
        if (fastForwardActive != lastFastForwardActive)
        {
            appendStubLog("GBAStationNDSStub: Deko fastforward %s x%d",
                          fastForwardActive ? "on" : "off",
                          menuLayer.fastForwardMultiplier());
            audio.setFastForwardActive(fastForwardActive);
            lastFastForwardActive = fastForwardActive;
        }

        const uint32_t keyMask = suppressGameInput ? 0x0FFFu : dsKeyMaskFromPad(pad);
        NDS::SetKeyMask(keyMask);

        u16 touchX = 0;
        u16 touchY = 0;
        if (!suppressGameInput && gameLayer.readTouch(touchX, touchY))
            NDS::TouchScreen(touchX, touchY);
        else
            NDS::ReleaseScreen();

        const auto runBegin = std::chrono::steady_clock::now();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu RunFrame begin",
                          static_cast<unsigned long long>(totalFrames));
        const int framesToRun = fastForwardActive ? menuLayer.fastForwardMultiplier() : 1;
        for (int i = 0; i < framesToRun; ++i)
        {
            NDS::RunFrame();
            if (fastForwardActive)
                SPU::Sync(false);
        }
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu RunFrame ok",
                          static_cast<unsigned long long>(totalFrames));
        const auto runEnd = std::chrono::steady_clock::now();
        lastRunMs = std::chrono::duration_cast<std::chrono::milliseconds>(runEnd - runBegin).count();

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::StartFrame begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::StartFrame();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::StartFrame ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PushScissor begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::PushScissor(0, 0, kScreenWidth, kScreenHeight);
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PushScissor ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu gameLayer.drawScreens begin",
                          static_cast<unsigned long long>(totalFrames));
        gameLayer.drawScreens();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu gameLayer.drawScreens ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu menuLayer.draw begin",
                          static_cast<unsigned long long>(totalFrames));
        menuLayer.draw(fps, lastRunMs, fastForwardActive);
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu menuLayer.draw ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PopScissor begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::PopScissor();
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::PopScissor ok",
                          static_cast<unsigned long long>(totalFrames));

        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::EndFrame begin",
                          static_cast<unsigned long long>(totalFrames));
        Gfx::EndFrame({0.015f, 0.020f, 0.026f, 1.0f}, 0);
        if (traceFrame)
            appendStubLog("GBAStationNDSStub: Deko checkpoint frame=%llu Gfx::EndFrame ok",
                          static_cast<unsigned long long>(totalFrames));

        fpsFrames += framesToRun;
        ++totalFrames;
        if (totalFrames % 60 == 0)
        {
            appendStubLog("GBAStationNDSStub: Deko heartbeat frame=%llu fps=%.1f run=%lldms ff=%d res=%d filter=%s",
                          static_cast<unsigned long long>(totalFrames),
                          fps,
                          lastRunMs,
                          fastForwardActive ? menuLayer.fastForwardMultiplier() : 1,
                          currentResolutionScale,
                          menuLayer.linearFiltering() ? "linear" : "nearest");
        }
        const auto now = std::chrono::steady_clock::now();
        const auto fpsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsStart).count();
        if (fpsElapsed >= 1000)
        {
            fps = static_cast<double>(fpsFrames) * 1000.0 / static_cast<double>(fpsElapsed);
            fpsFrames = 0;
            fpsStart = now;
        }

        const auto frameEnd = std::chrono::steady_clock::now();
        constexpr auto frameBudget = std::chrono::microseconds(16667);
        const auto used = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameBegin);
        if (!fastForwardActive && used < frameBudget)
        {
            const auto sleepUs = std::chrono::duration_cast<std::chrono::microseconds>(frameBudget - used).count();
            if (sleepUs > 500)
                svcSleepThread(static_cast<int64_t>(sleepUs) * 1000);
        }
    }

    audio.stop();
    NDSCart::FlushSRAMFile();
    gameLayer.deinit();
    GPU::DeInitRenderer();
    NDS::DeInit();
    Gfx::DeInit();
    romfsExit();

    if (pendingReturn)
        setReturnNro(options.returnNroPath);

    appendStubLog("GBAStationNDSStub: Deko runtime exit pendingReturn=%d", pendingReturn ? 1 : 0);
    return pendingReturn ? 0 : 1;
}

} // namespace beiklive::nds_stub
