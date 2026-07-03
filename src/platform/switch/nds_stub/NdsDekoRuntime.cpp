#include "platform/switch/nds_stub/NdsDekoRuntime.hpp"

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

#include "../../../../third_party/ArcDelta_melonDS/src/Config.h"
#include "../../../../third_party/ArcDelta_melonDS/src/GPU.h"
#include "../../../../third_party/ArcDelta_melonDS/src/GPU2D_Deko.h"
#include "../../../../third_party/ArcDelta_melonDS/src/NDS.h"
#include "../../../../third_party/ArcDelta_melonDS/src/NDSCart.h"
#include "../../../../third_party/ArcDelta_melonDS/src/Platform.h"
#include "../../../../third_party/ArcDelta_melonDS/src/SPU.h"
#include "../../../../third_party/ArcDelta_melonDS/src/frontend/switch/PlatformConfig.h"
#include "../../../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"
#include "platform/switch/nds_stub/NdsStubMelonPlatform.hpp"

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;
constexpr int kDsWidth = 256;
constexpr int kDsHeight = 192;

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

struct RectF {
    float x;
    float y;
    float w;
    float h;
};

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
    press(HidNpadButton_ZR, kNdsKeyR);
    press(HidNpadButton_Plus, kNdsKeyStart);
    press(HidNpadButton_Minus, kNdsKeySelect);
    press(HidNpadButton_StickR, kNdsKeyStart);
    press(HidNpadButton_StickL, kNdsKeySelect);
    press(HidNpadButton_AnyUp, kNdsKeyUp);
    press(HidNpadButton_AnyDown, kNdsKeyDown);
    press(HidNpadButton_AnyLeft, kNdsKeyLeft);
    press(HidNpadButton_AnyRight, kNdsKeyRight);
    return mask;
}

RectF makeTopRect()
{
    constexpr float scale = 2.0f;
    return {kScreenWidth * 0.5f - kDsWidth * scale - 22.0f,
            kScreenHeight * 0.5f - kDsHeight * scale * 0.5f,
            kDsWidth * scale,
            kDsHeight * scale};
}

RectF makeBottomRect()
{
    constexpr float scale = 2.0f;
    return {kScreenWidth * 0.5f + 22.0f,
            kScreenHeight * 0.5f - kDsHeight * scale * 0.5f,
            kDsWidth * scale,
            kDsHeight * scale};
}

bool touchFromScreen(const RectF& bottomRect, u16& outX, u16& outY)
{
    HidTouchScreenState state {};
    if (!hidGetTouchScreenStates(&state, 1) || state.count == 0)
        return false;

    const float sx = static_cast<float>(state.touches[0].x);
    const float sy = static_cast<float>(state.touches[0].y);
    if (sx < bottomRect.x || sx >= bottomRect.x + bottomRect.w ||
        sy < bottomRect.y || sy >= bottomRect.y + bottomRect.h)
        return false;

    outX = static_cast<u16>(std::clamp((sx - bottomRect.x) * kDsWidth / bottomRect.w, 0.0f, 255.0f));
    outY = static_cast<u16>(std::clamp((sy - bottomRect.y) * kDsHeight / bottomRect.h, 0.0f, 191.0f));
    return true;
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

        Result rc = audoutInitialize();
        if (R_FAILED(rc))
        {
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audoutInitialize failed rc=0x%x", rc);
            return false;
        }

        rc = audoutStartAudioOut();
        if (R_FAILED(rc))
            beiklive::nds_stub::appendStubLog("GBAStationNDSStub: deko audoutStartAudioOut rc=0x%x", rc);

        for (size_t i = 0; i < m_bufferData.size(); ++i)
        {
            m_bufferData[i] = static_cast<int16_t*>(std::aligned_alloc(0x1000, kBufferBytes));
            if (!m_bufferData[i])
            {
                stop();
                return false;
            }
            std::memset(m_bufferData[i], 0, kBufferBytes);
            m_outBuffers[i] = {};
            m_outBuffers[i].buffer = m_bufferData[i];
            m_outBuffers[i].buffer_size = kBufferBytes;
            m_outBuffers[i].data_size = kBufferBytes;
            m_outBuffers[i].data_offset = 0;
            m_freeList[m_freeCount++] = static_cast<int>(i);
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_ring.fill(0);
            m_read = 0;
            m_write = 0;
            m_available = 0;
        }

        m_running.store(true, std::memory_order_release);
        m_thread = std::thread(&DekoAudioOutput::threadMain, this);
        return true;
    }

    void stop()
    {
        if (m_running.exchange(false, std::memory_order_acq_rel))
        {
            m_cv.notify_all();
            if (m_thread.joinable())
                m_thread.join();
        }

        drainQueuedBuffers();
        audoutStopAudioOut();
        audoutExit();

        for (auto*& ptr : m_bufferData)
        {
            std::free(ptr);
            ptr = nullptr;
        }

        m_outBuffers = {};
        m_queued = {};
        m_freeCount = 0;
    }

    void push(const int16_t* samples, size_t stereoFrames)
    {
        if (!samples || stereoFrames == 0 || !m_running.load(std::memory_order_acquire))
            return;

        const size_t values = stereoFrames * 2;
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < values; ++i)
        {
            if (m_available == m_ring.size())
            {
                m_read = (m_read + 1) % m_ring.size();
                --m_available;
            }
            m_ring[m_write] = samples[i];
            m_write = (m_write + 1) % m_ring.size();
            ++m_available;
        }
        m_cv.notify_one();
    }

private:
    static constexpr size_t kBufferSamples = 2048 * 2;
    static constexpr size_t kBufferBytes = kBufferSamples * sizeof(int16_t);
    static constexpr size_t kBufferCount = 4;
    static constexpr size_t kRingSamples = 48000 * 2;

    void threadMain()
    {
        while (m_running.load(std::memory_order_acquire))
        {
            collectReleased(nullptr);

            int idx = -1;
            if (m_freeCount > 0)
                idx = m_freeList[--m_freeCount];

            if (idx < 0)
            {
                svcSleepThread(1000000);
                continue;
            }

            size_t copied = 0;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                if (m_available < kBufferSamples / 2)
                    m_cv.wait_for(lock, std::chrono::milliseconds(3));

                while (copied < kBufferSamples && m_available > 0)
                {
                    m_bufferData[idx][copied++] = m_ring[m_read];
                    m_read = (m_read + 1) % m_ring.size();
                    --m_available;
                }
            }

            if (copied < kBufferSamples)
                std::memset(m_bufferData[idx] + copied, 0, (kBufferSamples - copied) * sizeof(int16_t));

            m_outBuffers[idx].data_size = kBufferBytes;
            const Result rc = audoutAppendAudioOutBuffer(&m_outBuffers[idx]);
            if (R_SUCCEEDED(rc))
            {
                m_queued[idx] = true;
            }
            else
            {
                m_freeList[m_freeCount++] = idx;
                svcSleepThread(1000000);
            }
        }
    }

    void collectReleased(u32* releasedCount)
    {
        AudioOutBuffer* released = nullptr;
        u32 count = 0;
        const Result rc = audoutGetReleasedAudioOutBuffer(&released, &count);
        if (R_FAILED(rc) || !released || count == 0)
        {
            if (releasedCount)
                *releasedCount = 0;
            return;
        }

        for (u32 i = 0; i < count; ++i)
        {
            for (size_t j = 0; j < m_outBuffers.size(); ++j)
            {
                if (&m_outBuffers[j] == &released[i] || m_outBuffers[j].buffer == released[i].buffer)
                {
                    if (m_queued[j])
                    {
                        m_queued[j] = false;
                        m_freeList[m_freeCount++] = static_cast<int>(j);
                    }
                    break;
                }
            }
        }
        if (releasedCount)
            *releasedCount = count;
    }

    void drainQueuedBuffers()
    {
        for (int tries = 0; tries < 16; ++tries)
        {
            u32 released = 0;
            collectReleased(&released);
            bool anyQueued = false;
            for (bool queued : m_queued)
                anyQueued = anyQueued || queued;
            if (!anyQueued)
                break;
            svcSleepThread(1000000);
        }
    }

    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::array<int16_t, kRingSamples> m_ring {};
    size_t m_read = 0;
    size_t m_write = 0;
    size_t m_available = 0;
    std::array<int16_t*, kBufferCount> m_bufferData {};
    std::array<AudioOutBuffer, kBufferCount> m_outBuffers {};
    std::array<bool, kBufferCount> m_queued {};
    std::array<int, kBufferCount> m_freeList {};
    int m_freeCount = 0;
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

    configureArcDelta();

    Gfx::Init();
    NDS::Init();
    GPU::InitRenderer(0);
    GPU::RenderSettings settings {true, 1, false};
    GPU::SetRenderSettings(0, settings);

    std::array<std::array<u32, 2>, 2> framebufferTextures {};
    auto* deko2d = static_cast<GPU2D::DekoRenderer*>(GPU::GPU2D_Renderer.get());
    for (int front = 0; front < 2; ++front)
        for (int screen = 0; screen < 2; ++screen)
            framebufferTextures[front][screen] = Gfx::TextureCreateExternal(kDsWidth, kDsHeight, deko2d->GetFramebuffer(front, screen));

    bool loaded = NDS::LoadROM(options.romPath.c_str(), savePath.c_str(), true);
    appendStubLog("GBAStationNDSStub: Deko LoadROM loaded=%d save=%s", loaded ? 1 : 0, savePath.c_str());

    DekoAudioOutput audio;
    audio.start();

    bool running = loaded;
    bool menuVisible = false;
    bool pendingReturn = false;
    double fps = 0.0;
    int fpsFrames = 0;
    long long lastRunMs = 0;
    auto fpsStart = std::chrono::steady_clock::now();

    while (appletMainLoop() && running)
    {
        const auto frameBegin = std::chrono::steady_clock::now();
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);

        if (down & HidNpadButton_Plus)
            menuVisible = !menuVisible;
        if (down & HidNpadButton_Minus)
            menuVisible = true;

        if (menuVisible)
        {
            if (down & HidNpadButton_B)
                menuVisible = false;
            if (down & HidNpadButton_X)
            {
                NDS::Reset();
                NDS::SetupDirectBoot();
            }
            if (down & HidNpadButton_A)
            {
                pendingReturn = true;
                running = false;
            }
        }

        const RectF topRect = makeTopRect();
        const RectF bottomRect = makeBottomRect();
        const uint32_t keyMask = menuVisible ? 0x0FFFu : dsKeyMaskFromPad(pad);
        NDS::SetKeyMask(keyMask);

        u16 touchX = 0;
        u16 touchY = 0;
        if (!menuVisible && touchFromScreen(bottomRect, touchX, touchY))
            NDS::TouchScreen(touchX, touchY);
        else
            NDS::ReleaseScreen();

        const auto runBegin = std::chrono::steady_clock::now();
        NDS::RunFrame();
        const auto runEnd = std::chrono::steady_clock::now();
        lastRunMs = std::chrono::duration_cast<std::chrono::milliseconds>(runEnd - runBegin).count();

        std::array<int16_t, 4096> samples {};
        int available = SPU::GetOutputSize();
        while (available > 0)
        {
            const int toRead = std::min<int>(available, static_cast<int>(samples.size() / 2));
            const int read = SPU::ReadOutput(samples.data(), toRead);
            if (read <= 0)
                break;
            audio.push(samples.data(), static_cast<size_t>(read));
            available = SPU::GetOutputSize();
        }

        Gfx::StartFrame();
        Gfx::SetSampler(Gfx::sampler_Linear | Gfx::sampler_ClampToEdge);
        Gfx::WaitForFenceReady(deko2d->FramebufferReady[GPU::FrontBuffer]);
        Gfx::DrawRectangle(framebufferTextures[GPU::FrontBuffer][0],
                           {topRect.x, topRect.y},
                           {topRect.w, topRect.h},
                           {0.0f, 0.0f},
                           {static_cast<float>(kDsWidth), static_cast<float>(kDsHeight)},
                           {1.0f, 1.0f, 1.0f, 1.0f});
        Gfx::DrawRectangle(framebufferTextures[GPU::FrontBuffer][1],
                           {bottomRect.x, bottomRect.y},
                           {bottomRect.w, bottomRect.h},
                           {0.0f, 0.0f},
                           {static_cast<float>(kDsWidth), static_cast<float>(kDsHeight)},
                           {1.0f, 1.0f, 1.0f, 1.0f});
        Gfx::SignalFence(deko2d->FramebufferPresented[GPU::FrontBuffer]);

        Gfx::DrawText(Gfx::SystemFontStandard,
                      {28.0f, 24.0f},
                      20.0f,
                      {0.78f, 0.90f, 1.0f, 1.0f},
                      "FPS %.1f  RUN %lldMS  DEKO", fps, lastRunMs);

        if (menuVisible)
        {
            Gfx::DrawRectangle({390.0f, 160.0f}, {500.0f, 360.0f}, {0.04f, 0.06f, 0.08f, 0.88f}, true);
            Gfx::DrawText(Gfx::SystemFontStandard, {430.0f, 205.0f}, 28.0f, {0.95f, 0.98f, 1.0f, 1.0f}, "游戏菜单");
            Gfx::DrawText(Gfx::SystemFontStandard, {430.0f, 275.0f}, 22.0f, {0.80f, 0.90f, 0.98f, 1.0f}, "B 返回游戏");
            Gfx::DrawText(Gfx::SystemFontStandard, {430.0f, 325.0f}, 22.0f, {0.80f, 0.90f, 0.98f, 1.0f}, "X 重置游戏");
            Gfx::DrawText(Gfx::SystemFontStandard, {430.0f, 375.0f}, 22.0f, {1.00f, 0.78f, 0.42f, 1.0f}, "A 保存并退出");
        }

        Gfx::EndFrame({0.015f, 0.020f, 0.026f, 1.0f}, 0);

        ++fpsFrames;
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
        if (used < frameBudget)
        {
            const auto sleepUs = std::chrono::duration_cast<std::chrono::microseconds>(frameBudget - used).count();
            if (sleepUs > 500)
                svcSleepThread(static_cast<int64_t>(sleepUs) * 1000);
        }
    }

    audio.stop();
    NDSCart::FlushSRAMFile();
    for (int front = 0; front < 2; ++front)
        for (int screen = 0; screen < 2; ++screen)
            Gfx::TextureDelete(framebufferTextures[front][screen]);
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
