#include <algorithm>
#include <cctype>
#include <chrono>
#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <unordered_map>
#include <vector>
#include <string>

#include <nlohmann/json.hpp>
#include <switch.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include "nanovg/stb_truetype.h"

#include "ARMJIT_Memory.h"
#include "Args.h"
#include "GPU.h"
#include "GPU3D.h"
#include "NDS.h"
#include "NDSCart.h"
#include "RTC.h"
#include "platform/switch/nds_stub/NdsDekoRuntime.hpp"
#include "platform/switch/nds_stub/NdsStubMelonPlatform.hpp"

namespace {

constexpr int kScreenWidth = 1280;
constexpr int kScreenHeight = 720;

struct Rect {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct UiFrame {
    uint32_t* pixels = nullptr;
    int stridePixels = 0;
    int width = kScreenWidth;
    int height = kScreenHeight;
};

struct GameInfo {
    std::string romPath;
    std::string title;
    std::string savePath;
    std::string cheatPath;
    int internalResolution = 1;
};

enum class MenuAction {
    Resume,
    SaveState,
    LoadState,
    Cheats,
    Display,
    Reset,
    Exit,
};

struct MenuItem {
    const char* label;
    MenuAction action;
};

constexpr MenuItem kMenuItems[] = {
    {"返回游戏", MenuAction::Resume},
    {"保存状态", MenuAction::SaveState},
    {"读取状态", MenuAction::LoadState},
    {"金手指", MenuAction::Cheats},
    {"画面设置", MenuAction::Display},
    {"重置游戏", MenuAction::Reset},
    {"退出游戏", MenuAction::Exit},
};

constexpr int kNdsWidth = 256;
constexpr int kNdsScreenHeight = 192;
constexpr int kNdsHeight = kNdsScreenHeight * 2;
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

void appendLog(const char* format, ...);

uint32_t toFramebufferRgba(uint32_t pixel)
{
    return (pixel & 0xFF00FF00u) |
           ((pixel & 0x000000FFu) << 16) |
           ((pixel & 0x00FF0000u) >> 16);
}

bool readWholeFile(const std::string& path, std::vector<uint8_t>& out)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return false;
    const std::streamsize size = file.tellg();
    if (size <= 0 || size > std::numeric_limits<melonDS::u32>::max())
        return false;
    file.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    file.read(reinterpret_cast<char*>(out.data()), size);
    return file.gcount() == size;
}

template <typename ArrayT>
bool readExactFile(const std::string& path, ArrayT& out)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    file.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(out.size()));
    return file.gcount() == static_cast<std::streamsize>(out.size());
}

bool writeWholeFile(const std::string& path, const void* data, size_t size)
{
    if ((!data && size != 0) || path.empty())
        return false;
    std::error_code ec;
    std::filesystem::create_directories(std::filesystem::path(path).parent_path(), ec);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file)
        return false;
    file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
    return static_cast<bool>(file);
}

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

struct NdsTouchState {
    bool down = false;
    int x = 0;
    int y = 0;
};

struct NdsFrameTimings {
    long long totalMs = 0;
    long long runMs = 0;
    long long audioMs = 0;
    long long captureMs = 0;
};

class NdsAudioOutput {
public:
    bool start()
    {
        if (m_running.load(std::memory_order_acquire))
            return true;

        Result rc = audoutInitialize();
        if (R_FAILED(rc))
        {
            appendLog("GBAStationNDSStub: audoutInitialize failed rc=0x%x", rc);
            return false;
        }

        rc = audoutStartAudioOut();
        if (R_FAILED(rc))
            appendLog("GBAStationNDSStub: audoutStartAudioOut rc=0x%x", rc);

        for (size_t i = 0; i < kBufferCount; ++i)
        {
            m_bufferData[i] = static_cast<int16_t*>(std::aligned_alloc(0x1000, kBufferBytes));
            if (!m_bufferData[i])
            {
                appendLog("GBAStationNDSStub: audio buffer alloc failed index=%zu", i);
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
            m_ring.assign(kRingSamples, 0);
            m_read = 0;
            m_write = 0;
            m_available = 0;
        }

        m_running.store(true, std::memory_order_release);
        m_thread = std::thread(&NdsAudioOutput::threadMain, this);
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
        m_enqueued = 0;

        std::lock_guard<std::mutex> lock(m_mutex);
        m_ring.clear();
        m_read = 0;
        m_write = 0;
        m_available = 0;
    }

    void push(const int16_t* samples, size_t sampleCount)
    {
        if (!samples || sampleCount == 0 || !m_running.load(std::memory_order_acquire))
            return;

        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_ring.empty())
            return;

        for (size_t i = 0; i < sampleCount; ++i)
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
    static constexpr size_t kFrames = 1024;
    static constexpr size_t kChannels = 2;
    static constexpr size_t kSamples = kFrames * kChannels;
    static constexpr size_t kBufferBytes = kSamples * sizeof(int16_t);
    static constexpr size_t kBufferCount = 4;
    static constexpr size_t kRingSamples = 48000 * kChannels;

    bool owns(const AudioOutBuffer* buffer, int* index = nullptr) const
    {
        for (size_t i = 0; i < kBufferCount; ++i)
        {
            if (buffer == &m_outBuffers[i])
            {
                if (index)
                    *index = static_cast<int>(i);
                return true;
            }
        }
        return false;
    }

    void markReleased(int index)
    {
        if (index < 0 || index >= static_cast<int>(kBufferCount) || !m_queued[index])
            return;
        m_queued[index] = false;
        if (m_enqueued > 0)
            --m_enqueued;
        if (m_freeCount < static_cast<int>(kBufferCount))
            m_freeList[m_freeCount++] = index;
    }

    void collectReleased(AudioOutBuffer* released)
    {
        for (AudioOutBuffer* buffer = released; buffer != nullptr; buffer = buffer->next)
        {
            int index = -1;
            if (owns(buffer, &index))
                markReleased(index);
        }
    }

    int takeFree()
    {
        if (m_freeCount <= 0)
            return -1;
        return m_freeList[--m_freeCount];
    }

    size_t readSamples(int16_t* out, size_t sampleCount)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait_for(lock, std::chrono::milliseconds(12), [&] {
            return !m_running.load(std::memory_order_acquire) || m_available >= sampleCount;
        });

        size_t count = std::min(sampleCount, m_available);
        for (size_t i = 0; i < count; ++i)
        {
            out[i] = m_ring[m_read];
            m_read = (m_read + 1) % m_ring.size();
        }
        m_available -= count;
        return count;
    }

    void threadMain()
    {
        svcSetThreadCoreMask(CUR_THREAD_HANDLE, 2, 1ULL << 2);
        while (m_running.load(std::memory_order_acquire))
        {
            AudioOutBuffer* released = nullptr;
            u32 releasedCount = 0;
            audoutWaitPlayFinish(&released, &releasedCount, 0);
            if (released)
                collectReleased(released);

            while (m_freeCount == 0 && m_running.load(std::memory_order_acquire))
            {
                released = nullptr;
                releasedCount = 0;
                audoutWaitPlayFinish(&released, &releasedCount, 10000000);
                if (released)
                    collectReleased(released);
            }

            if (!m_running.load(std::memory_order_acquire))
                break;

            const int index = takeFree();
            if (index < 0 || !m_bufferData[index])
                continue;

            size_t got = readSamples(m_bufferData[index], kSamples);
            if (got < kSamples)
            {
                const int16_t lastL = got >= 2 ? m_bufferData[index][got - 2] : 0;
                const int16_t lastR = got >= 2 ? m_bufferData[index][got - 1] : 0;
                while (got + 1 < kSamples)
                {
                    m_bufferData[index][got++] = lastL;
                    m_bufferData[index][got++] = lastR;
                }
            }

            armDCacheFlush(m_bufferData[index], kBufferBytes);
            m_outBuffers[index].next = nullptr;
            Result rc = audoutAppendAudioOutBuffer(&m_outBuffers[index]);
            if (R_SUCCEEDED(rc))
            {
                m_queued[index] = true;
                ++m_enqueued;
            }
            else
            {
                if (m_appendFailLogs < 5)
                {
                    appendLog("GBAStationNDSStub: audoutAppendAudioOutBuffer failed rc=0x%x", rc);
                    ++m_appendFailLogs;
                }
                if (m_freeCount < static_cast<int>(kBufferCount))
                    m_freeList[m_freeCount++] = index;
            }
        }
    }

    void drainQueuedBuffers()
    {
        for (int retry = 0; m_enqueued > 0 && retry < 24; ++retry)
        {
            AudioOutBuffer* released = nullptr;
            u32 releasedCount = 0;
            audoutWaitPlayFinish(&released, &releasedCount, 16000000);
            if (released)
                collectReleased(released);
        }
    }

    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<int16_t> m_ring;
    size_t m_read = 0;
    size_t m_write = 0;
    size_t m_available = 0;
    std::array<int16_t*, kBufferCount> m_bufferData {};
    std::array<AudioOutBuffer, kBufferCount> m_outBuffers {};
    std::array<bool, kBufferCount> m_queued {};
    std::array<int, kBufferCount> m_freeList {};
    int m_freeCount = 0;
    u32 m_enqueued = 0;
    int m_appendFailLogs = 0;
};

class NdsRuntime {
public:
    bool start(const GameInfo& game)
    {
        stop();
        m_game = game;
        m_frame.assign(static_cast<size_t>(kNdsWidth) * kNdsHeight, 0xFF000000u);

        std::string saveDir = game.savePath.empty() ? defaultSaveDir(game.romPath) : game.savePath;
        std::error_code ec;
        std::filesystem::create_directories(saveDir, ec);
        m_savePath = joinPath(saveDir, pathStem(game.romPath) + ".sav");
        m_platformData.savePath = m_savePath;
        m_platformData.firmwarePath = "sdmc:/GBAStation/bios/nds/firmware.bin";

        melonDS::NDSArgs args;
        melonDS::JITArgs jitArgs;
        jitArgs.FastMemory = melonDS::ARMJIT_Memory::IsFastMemSupported();
        args.JIT = jitArgs;
        args.OutputSampleRate = 48000.0;
        args.Renderer3D = std::make_unique<melonDS::SoftRenderer>(true);
        appendLog("GBAStationNDSStub: JIT fastmem requested=%d", jitArgs.FastMemory ? 1 : 0);

        auto arm9 = std::make_unique<melonDS::ARM9BIOSImage>();
        auto arm7 = std::make_unique<melonDS::ARM7BIOSImage>();
        if (!readExactFile("sdmc:/GBAStation/bios/nds/bios9.bin", *arm9))
        {
            m_status = "MISSING BIOS9";
            appendLog("GBAStationNDSStub: missing BIOS sdmc:/GBAStation/bios/nds/bios9.bin");
            return false;
        }
        if (!readExactFile("sdmc:/GBAStation/bios/nds/bios7.bin", *arm7))
        {
            m_status = "MISSING BIOS7";
            appendLog("GBAStationNDSStub: missing BIOS sdmc:/GBAStation/bios/nds/bios7.bin");
            return false;
        }

        std::vector<uint8_t> firmwareData;
        if (!readWholeFile(m_platformData.firmwarePath, firmwareData))
        {
            m_status = "MISSING FIRMWARE";
            appendLog("GBAStationNDSStub: missing firmware %s", m_platformData.firmwarePath.c_str());
            return false;
        }

        args.ARM9BIOS = std::move(arm9);
        args.ARM7BIOS = std::move(arm7);
        args.Firmware = melonDS::Firmware(firmwareData.data(), static_cast<melonDS::u32>(firmwareData.size()));

        try
        {
            m_nds = std::make_unique<melonDS::NDS>(std::move(args), &m_platformData);
        }
        catch (const std::exception& e)
        {
            m_status = "NDS INIT FAILED";
            appendLog("GBAStationNDSStub: NDS init exception: %s", e.what());
            return false;
        }
        catch (...)
        {
            m_status = "NDS INIT FAILED";
            appendLog("GBAStationNDSStub: NDS init unknown exception");
            return false;
        }

        if (!m_nds)
        {
            m_status = "NDS INIT FAILED";
            return false;
        }

        if (!m_nds->IsJITEnabled())
        {
            m_status = "JIT DISABLED";
            appendLog("GBAStationNDSStub: JIT disabled");
            return false;
        }

        if (!loadRom())
            return false;

        syncRtc();
        m_nds->GPU.StartFrame();
        m_nds->Start();
        m_audio.start();
        m_ready = true;
        m_status = "RUNNING";
        appendLog("GBAStationNDSStub: melonDS running rom=%s save=%s", game.romPath.c_str(), m_savePath.c_str());
        return true;
    }

    void stop()
    {
        m_audio.stop();
        if (m_nds)
        {
            if (m_ready)
                saveSram();
            if (m_nds->IsRunning())
                m_nds->Stop();
        }
        m_nds.reset();
        m_ready = false;
    }

    void reset()
    {
        if (!m_nds)
            return;
        m_nds->Reset();
        m_nds->SetupDirectBoot(std::filesystem::path(m_game.romPath).filename().string());
        syncRtc();
        m_nds->GPU.StartFrame();
        m_nds->Start();
        m_status = "RESET";
    }

    NdsFrameTimings runFrame(uint32_t keyMask, const NdsTouchState& touch)
    {
        if (!m_ready || !m_nds)
            return {};
        m_nds->SetKeyMask(keyMask);
        if (touch.down)
            m_nds->TouchScreen(static_cast<melonDS::u16>(std::clamp(touch.x, 0, 255)),
                               static_cast<melonDS::u16>(std::clamp(touch.y, 0, 191)));
        else
            m_nds->ReleaseScreen();

        const auto begin = std::chrono::steady_clock::now();
        m_nds->RunFrame();
        const auto afterRun = std::chrono::steady_clock::now();
        drainAudio();
        const auto afterAudio = std::chrono::steady_clock::now();
        captureFrame();
        const auto end = std::chrono::steady_clock::now();

        NdsFrameTimings timings;
        timings.runMs = std::chrono::duration_cast<std::chrono::milliseconds>(afterRun - begin).count();
        timings.audioMs = std::chrono::duration_cast<std::chrono::milliseconds>(afterAudio - afterRun).count();
        timings.captureMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - afterAudio).count();
        timings.totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
        if (timings.totalMs > 18 && m_slowLogBudget > 0)
        {
            --m_slowLogBudget;
            appendLog("GBAStationNDSStub: slow frame total=%lld run=%lld audio=%lld capture=%lld",
                      timings.totalMs, timings.runMs, timings.audioMs, timings.captureMs);
        }
        return timings;
    }

    const std::vector<uint32_t>& frame() const { return m_frame; }
    bool ready() const { return m_ready; }
    const std::string& status() const { return m_status; }

private:
    bool loadRom()
    {
        std::vector<uint8_t> romBytes;
        if (!readWholeFile(m_game.romPath, romBytes))
        {
            m_status = "ROM READ FAILED";
            appendLog("GBAStationNDSStub: failed to read ROM %s", m_game.romPath.c_str());
            return false;
        }

        auto romData = std::make_unique<uint8_t[]>(romBytes.size());
        std::memcpy(romData.get(), romBytes.data(), romBytes.size());

        melonDS::NDSCart::NDSCartArgs cartArgs;
        std::vector<uint8_t> saveBytes;
        if (readWholeFile(m_savePath, saveBytes) && !saveBytes.empty())
        {
            cartArgs.SRAMLength = static_cast<melonDS::u32>(saveBytes.size());
            cartArgs.SRAM = std::make_unique<melonDS::u8[]>(cartArgs.SRAMLength);
            std::memcpy(cartArgs.SRAM.get(), saveBytes.data(), saveBytes.size());
            appendLog("GBAStationNDSStub: loaded save %s size=%u", m_savePath.c_str(), cartArgs.SRAMLength);
        }

        auto cart = melonDS::NDSCart::ParseROM(std::move(romData),
                                               static_cast<melonDS::u32>(romBytes.size()),
                                               &m_platformData,
                                               std::move(cartArgs));
        if (!cart)
        {
            m_status = "ROM PARSE FAILED";
            appendLog("GBAStationNDSStub: ParseROM failed %s", m_game.romPath.c_str());
            return false;
        }

        m_nds->SetNDSCart(std::move(cart));
        m_nds->Reset();
        m_nds->SetupDirectBoot(std::filesystem::path(m_game.romPath).filename().string());
        return true;
    }

    void syncRtc()
    {
        if (!m_nds)
            return;
        std::time_t now = std::time(nullptr);
        if (now == static_cast<std::time_t>(-1))
            return;
        std::tm local {};
        if (!localtime_r(&now, &local))
            return;
        m_nds->RTC.SetDateTime(local.tm_year + 1900,
                               local.tm_mon + 1,
                               local.tm_mday,
                               local.tm_hour,
                               local.tm_min,
                               local.tm_sec);
    }

    void captureFrame()
    {
        const int front = m_nds->GPU.FrontBuffer;
        const uint32_t* top = m_nds->GPU.Framebuffer[front][0].get();
        const uint32_t* bottom = m_nds->GPU.Framebuffer[front][1].get();
        if (!top || !bottom)
            return;

        const size_t stride = m_nds->GPU.GetRenderer3D().Accelerated ? (256u * 3u + 1u) : kNdsWidth;
        for (int y = 0; y < kNdsScreenHeight; ++y)
        {
            const uint32_t* src = top + static_cast<size_t>(y) * stride;
            uint32_t* dst = m_frame.data() + static_cast<size_t>(y) * kNdsWidth;
            for (int x = 0; x < kNdsWidth; ++x)
                dst[x] = toFramebufferRgba(src[x]);
        }
        for (int y = 0; y < kNdsScreenHeight; ++y)
        {
            const uint32_t* src = bottom + static_cast<size_t>(y) * stride;
            uint32_t* dst = m_frame.data() + static_cast<size_t>(y + kNdsScreenHeight) * kNdsWidth;
            for (int x = 0; x < kNdsWidth; ++x)
                dst[x] = toFramebufferRgba(src[x]);
        }
    }

    void saveSram()
    {
        const uint8_t* data = m_nds ? m_nds->GetNDSSave() : nullptr;
        const uint32_t size = m_nds ? m_nds->GetNDSSaveLength() : 0;
        if (data && size > 0)
        {
            writeWholeFile(m_savePath, data, size);
            appendLog("GBAStationNDSStub: saved SRAM %s size=%u", m_savePath.c_str(), size);
        }
    }

    void drainAudio()
    {
        std::array<int16_t, 4096> temp {};
        int available = m_nds->SPU.GetOutputSize();
        while (available > 0)
        {
            const int toRead = std::min<int>(available, static_cast<int>(temp.size() / 2));
            const int read = m_nds->SPU.ReadOutput(temp.data(), toRead);
            if (read <= 0)
                break;
            m_audio.push(temp.data(), static_cast<size_t>(read) * 2);
            available = m_nds->SPU.GetOutputSize();
        }
    }

    GameInfo m_game;
    beiklive::nds_stub::MelonPlatformData m_platformData;
    std::unique_ptr<melonDS::NDS> m_nds;
    NdsAudioOutput m_audio;
    std::vector<uint32_t> m_frame;
    std::string m_savePath;
    std::string m_status = "NOT STARTED";
    bool m_ready = false;
    int m_slowLogBudget = 60;
};

void appendLog(const char* format, ...)
{
    (void)format;
}

} // namespace

namespace {

std::string quoteArg(const std::string& value)
{
    std::string out;
    out.reserve(value.size() + 2);
    out.push_back('"');
    for (char c : value)
    {
        if (c == '"' || c == '\\')
            out.push_back('\\');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

bool fileExists(const std::string& path)
{
    FILE* fp = std::fopen(path.c_str(), "rb");
    if (!fp)
        return false;
    std::fclose(fp);
    return true;
}

std::string normalizePathForCompare(std::string path)
{
    for (char& c : path)
    {
        if (c == '\\')
            c = '/';
    }

    if (path.rfind("sdmc:", 0) == 0)
        path.erase(0, 5);

    while (path.size() > 1 && path[0] == '/' && path[1] == '/')
        path.erase(0, 1);

    return path;
}

bool endsWithNoCase(const std::string& value, const char* suffix)
{
    const size_t suffixLen = std::strlen(suffix);
    if (value.size() < suffixLen)
        return false;

    const size_t offset = value.size() - suffixLen;
    for (size_t i = 0; i < suffixLen; ++i)
    {
        const char a = value[offset + i];
        const char b = suffix[i];
        if (std::tolower(static_cast<unsigned char>(a)) !=
            std::tolower(static_cast<unsigned char>(b)))
            return false;
    }
    return true;
}

std::string jsonString(const nlohmann::json& item, const char* key)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_string())
        return "";
    return it->get<std::string>();
}

int jsonInt(const nlohmann::json& item, const char* key, int fallback)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_number_integer())
        return fallback;
    return it->get<int>();
}

bool jsonBool(const nlohmann::json& item, const char* key, bool fallback)
{
    const auto it = item.find(key);
    if (it == item.end() || !it->is_boolean())
        return fallback;
    return it->get<bool>();
}

std::optional<nlohmann::json> loadNdsGameDbRecord(const std::string& romPath)
{
    constexpr const char* dbPaths[] = {
        "/GBAStation/data/GameData_NDS.json",
        "sdmc:/GBAStation/data/GameData_NDS.json",
    };

    const std::string normalizedRomPath = normalizePathForCompare(romPath);

    for (const char* dbPath : dbPaths)
    {
        appendLog("GBAStationNDSStub: try GameDB path=%s", dbPath);
        if (!fileExists(dbPath))
        {
            appendLog("GBAStationNDSStub: GameDB file missing path=%s", dbPath);
            continue;
        }

        try
        {
            std::ifstream file(dbPath, std::ios::binary);
            if (!file.is_open())
            {
                appendLog("GBAStationNDSStub: GameDB open failed path=%s", dbPath);
                continue;
            }

            nlohmann::json data;
            file >> data;
            if (!data.is_array())
            {
                appendLog("GBAStationNDSStub: GameDB root is not array path=%s", dbPath);
                continue;
            }

            appendLog("GBAStationNDSStub: GameDB loaded path=%s count=%zu", dbPath, data.size());
            for (const auto& item : data)
            {
                if (!item.is_object())
                    continue;

                const std::string itemPath = jsonString(item, "path");
                if (itemPath == romPath || normalizePathForCompare(itemPath) == normalizedRomPath)
                {
                    appendLog("GBAStationNDSStub: GameDB match path=%s", itemPath.c_str());
                    return item;
                }
            }

            appendLog("GBAStationNDSStub: GameDB no match romPath=%s normalized=%s",
                romPath.c_str(), normalizedRomPath.c_str());
        }
        catch (const std::exception& e)
        {
            appendLog("GBAStationNDSStub: GameDB exception path=%s error=%s", dbPath, e.what());
        }
        catch (...)
        {
            appendLog("GBAStationNDSStub: GameDB unknown exception path=%s", dbPath);
        }
    }

    return std::nullopt;
}

GameInfo buildGameInfo(const std::string& romPath, const std::optional<nlohmann::json>& record)
{
    GameInfo info;
    info.romPath = romPath;

    if (record.has_value())
    {
        info.title = jsonString(*record, "title");
        info.savePath = jsonString(*record, "savePath");
        info.cheatPath = jsonString(*record, "cheatPath");
        info.internalResolution = jsonInt(*record, "ndsInternalResolution", 1);
    }

    if (info.title.empty())
    {
        const size_t slash = romPath.find_last_of("/\\");
        std::string name = slash == std::string::npos ? romPath : romPath.substr(slash + 1);
        const size_t dot = name.find_last_of('.');
        info.title = dot == std::string::npos ? name : name.substr(0, dot);
    }

    if (info.title.empty())
        info.title = "NDS Game";

    return info;
}

void logGameDbRecord(const nlohmann::json& item)
{
    appendLog("GBAStationNDSStub: gameDb.found=1");
    appendLog("GBAStationNDSStub: gameDb.title=%s", jsonString(item, "title").c_str());
    appendLog("GBAStationNDSStub: gameDb.path=%s", jsonString(item, "path").c_str());
    appendLog("GBAStationNDSStub: gameDb.savePath=%s", jsonString(item, "savePath").c_str());
    appendLog("GBAStationNDSStub: gameDb.cheatPath=%s", jsonString(item, "cheatPath").c_str());
    appendLog("GBAStationNDSStub: gameDb.screenShotPath=%s", jsonString(item, "screenShotPath").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsInternalResolution=%d", jsonInt(item, "ndsInternalResolution", 1));
    appendLog("GBAStationNDSStub: gameDb.ndsScreenLayout=%s", jsonString(item, "ndsScreenLayout").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsScreenOrientation=%s", jsonString(item, "ndsScreenOrientation").c_str());
    appendLog("GBAStationNDSStub: gameDb.ndsIntegerScale=%d", jsonBool(item, "ndsIntegerScale", false) ? 1 : 0);
}

uint32_t color(uint8_t r, uint8_t g, uint8_t b)
{
    return RGBA8(r, g, b, 255);
}

uint32_t blendColor(uint32_t dst, uint32_t src, uint8_t alpha)
{
    const uint32_t inv = 255 - alpha;
    const uint32_t sr = src & 0xff;
    const uint32_t sg = (src >> 8) & 0xff;
    const uint32_t sb = (src >> 16) & 0xff;
    const uint32_t dr = dst & 0xff;
    const uint32_t dg = (dst >> 8) & 0xff;
    const uint32_t db = (dst >> 16) & 0xff;
    return RGBA8(
        static_cast<uint8_t>((sr * alpha + dr * inv) / 255),
        static_cast<uint8_t>((sg * alpha + dg * inv) / 255),
        static_cast<uint8_t>((sb * alpha + db * inv) / 255),
        255);
}

uint8_t alphaFromColor(uint32_t rgba)
{
    return static_cast<uint8_t>((rgba >> 24) & 0xff);
}

std::vector<uint32_t> decodeUtf8(const std::string& text)
{
    std::vector<uint32_t> out;
    for (size_t i = 0; i < text.size();)
    {
        const uint8_t c = static_cast<uint8_t>(text[i]);
        if (c < 0x80)
        {
            out.push_back(c);
            ++i;
        }
        else if ((c & 0xE0) == 0xC0 && i + 1 < text.size())
        {
            out.push_back(((c & 0x1F) << 6) |
                          (static_cast<uint8_t>(text[i + 1]) & 0x3F));
            i += 2;
        }
        else if ((c & 0xF0) == 0xE0 && i + 2 < text.size())
        {
            out.push_back(((c & 0x0F) << 12) |
                          ((static_cast<uint8_t>(text[i + 1]) & 0x3F) << 6) |
                          (static_cast<uint8_t>(text[i + 2]) & 0x3F));
            i += 3;
        }
        else if ((c & 0xF8) == 0xF0 && i + 3 < text.size())
        {
            out.push_back(((c & 0x07) << 18) |
                          ((static_cast<uint8_t>(text[i + 1]) & 0x3F) << 12) |
                          ((static_cast<uint8_t>(text[i + 2]) & 0x3F) << 6) |
                          (static_cast<uint8_t>(text[i + 3]) & 0x3F));
            i += 4;
        }
        else
        {
            out.push_back('?');
            ++i;
        }
    }
    return out;
}

class FontRenderer {
public:
    void ensureLoaded()
    {
        if (m_loaded)
            return;
        m_loaded = true;

        if (R_SUCCEEDED(plInitialize(PlServiceType_User)))
        {
            const PlSharedFontType candidates[] = {
                PlSharedFontType_Standard,
                PlSharedFontType_ChineseSimplified,
                PlSharedFontType_ExtChineseSimplified,
                PlSharedFontType_ChineseTraditional,
                PlSharedFontType_KO,
                PlSharedFontType_NintendoExt,
            };

            for (PlSharedFontType type : candidates)
            {
                PlFontData sharedFont {};
                if (R_SUCCEEDED(plGetSharedFontByType(&sharedFont, type)) &&
                    sharedFont.address != nullptr && sharedFont.size > 0)
                {
                    const auto* bytes = static_cast<const uint8_t*>(sharedFont.address);
                    std::vector<uint8_t> data(bytes, bytes + sharedFont.size);
                    addFont(std::move(data));
                }
            }
            plExit();
        }

        if (m_fonts.empty())
        {
            tryAppendFont("sdmc:/GBAStation/resources/font/switch_font.ttf");
            tryAppendFont("sdmc:/GBAStation/resources/font/font.ttf");
            tryAppendFont("sdmc:/GBAStation/font/switch_font.ttf");
            tryAppendFont("romfs:/font/switch_font.ttf");
        }

        tryAppendFont("sdmc:/GBAStation/resources/material/MaterialIcons-Regular.ttf");
        appendLog("GBAStationNDSStub: fonts loaded count=%zu", m_fonts.size());
    }

    bool available()
    {
        ensureLoaded();
        return !m_fonts.empty();
    }

    void draw(UiFrame& frame, int x, int y, const std::string& text, uint32_t rgba, int px)
    {
        ensureLoaded();
        if (m_fonts.empty())
            return;

        int cursor = x;
        const auto codepoints = decodeUtf8(text);
        for (uint32_t cp : codepoints)
        {
            if (cp == '\n')
            {
                cursor = x;
                y += px + 6;
                continue;
            }
            if (cp == '\r')
                continue;
            if (cp == ' ')
            {
                cursor += px / 3;
                continue;
            }

            const Glyph* glyph = glyphFor(cp, px);
            if (!glyph)
            {
                cursor += px / 2;
                continue;
            }

            const int gx = cursor + glyph->xoff;
            const int gy = y + glyph->yoff;
            for (int row = 0; row < glyph->h; ++row)
            {
                const int dy = gy + row;
                if (dy < 0 || dy >= frame.height)
                    continue;
                uint32_t* dst = frame.pixels + dy * frame.stridePixels;
                const uint8_t* src = glyph->bitmap.data() + static_cast<size_t>(row) * glyph->w;
                for (int col = 0; col < glyph->w; ++col)
                {
                    const int dx = gx + col;
                    if (dx < 0 || dx >= frame.width)
                        continue;
                    const uint8_t a = static_cast<uint8_t>((static_cast<int>(src[col]) * alphaFromColor(rgba)) / 255);
                    if (a)
                        dst[dx] = blendColor(dst[dx], rgba, a);
                }
            }
            cursor += glyph->advance;
        }
    }

private:
    struct FontFace {
        std::vector<uint8_t> data;
        stbtt_fontinfo info {};
    };

    struct Glyph {
        int w = 0;
        int h = 0;
        int xoff = 0;
        int yoff = 0;
        int advance = 0;
        std::vector<uint8_t> bitmap;
    };

    uint64_t keyFor(size_t fontIndex, uint32_t cp, int px) const
    {
        return (static_cast<uint64_t>(fontIndex) << 48) |
               (static_cast<uint64_t>(px & 0xffff) << 32) |
               static_cast<uint64_t>(cp);
    }

    void addFont(std::vector<uint8_t> data)
    {
        if (data.empty())
            return;
        FontFace face;
        face.data = std::move(data);
        if (stbtt_InitFont(&face.info, face.data.data(), stbtt_GetFontOffsetForIndex(face.data.data(), 0)))
            m_fonts.push_back(std::move(face));
    }

    void tryAppendFont(const char* path)
    {
        std::vector<uint8_t> data;
        if (readWholeFile(path, data))
            addFont(std::move(data));
    }

    const Glyph* glyphFor(uint32_t cp, int px)
    {
        for (size_t i = 0; i < m_fonts.size(); ++i)
        {
            if (stbtt_FindGlyphIndex(&m_fonts[i].info, static_cast<int>(cp)) == 0)
                continue;

            const uint64_t key = keyFor(i, cp, px);
            auto found = m_cache.find(key);
            if (found != m_cache.end())
                return &found->second;

            const float scale = stbtt_ScaleForPixelHeight(&m_fonts[i].info, static_cast<float>(px));
            int advance = 0;
            int lsb = 0;
            stbtt_GetCodepointHMetrics(&m_fonts[i].info, static_cast<int>(cp), &advance, &lsb);
            int x0 = 0, y0 = 0, x1 = 0, y1 = 0;
            stbtt_GetCodepointBitmapBox(&m_fonts[i].info, static_cast<int>(cp), scale, scale, &x0, &y0, &x1, &y1);

            Glyph glyph;
            glyph.w = std::max(0, x1 - x0);
            glyph.h = std::max(0, y1 - y0);
            glyph.xoff = x0;
            glyph.yoff = y0 + px;
            glyph.advance = std::max(1, static_cast<int>(advance * scale + 0.5f));
            glyph.bitmap.assign(static_cast<size_t>(glyph.w) * glyph.h, 0);
            if (glyph.w > 0 && glyph.h > 0)
            {
                stbtt_MakeCodepointBitmap(&m_fonts[i].info,
                                          glyph.bitmap.data(),
                                          glyph.w,
                                          glyph.h,
                                          glyph.w,
                                          scale,
                                          scale,
                                          static_cast<int>(cp));
            }

            auto [it, inserted] = m_cache.emplace(key, std::move(glyph));
            (void)inserted;
            return &it->second;
        }
        return nullptr;
    }

    bool m_loaded = false;
    std::vector<FontFace> m_fonts;
    std::unordered_map<uint64_t, Glyph> m_cache;
};

FontRenderer g_fontRenderer;

void fillRect(UiFrame& frame, Rect rect, uint32_t rgba)
{
    const int x0 = std::max(0, rect.x);
    const int y0 = std::max(0, rect.y);
    const int x1 = std::min(frame.width, rect.x + rect.w);
    const int y1 = std::min(frame.height, rect.y + rect.h);
    for (int y = y0; y < y1; ++y)
    {
        uint32_t* row = frame.pixels + y * frame.stridePixels;
        for (int x = x0; x < x1; ++x)
            row[x] = rgba;
    }
}

void fillRectAlpha(UiFrame& frame, Rect rect, uint32_t rgba, uint8_t alpha)
{
    const int x0 = std::max(0, rect.x);
    const int y0 = std::max(0, rect.y);
    const int x1 = std::min(frame.width, rect.x + rect.w);
    const int y1 = std::min(frame.height, rect.y + rect.h);
    for (int y = y0; y < y1; ++y)
    {
        uint32_t* row = frame.pixels + y * frame.stridePixels;
        for (int x = x0; x < x1; ++x)
            row[x] = blendColor(row[x], rgba, alpha);
    }
}

void drawRect(UiFrame& frame, Rect rect, uint32_t rgba, int thickness = 2)
{
    fillRect(frame, {rect.x, rect.y, rect.w, thickness}, rgba);
    fillRect(frame, {rect.x, rect.y + rect.h - thickness, rect.w, thickness}, rgba);
    fillRect(frame, {rect.x, rect.y, thickness, rect.h}, rgba);
    fillRect(frame, {rect.x + rect.w - thickness, rect.y, thickness, rect.h}, rgba);
}

uint8_t glyphRow(char ch, int row)
{
    if (ch >= 'a' && ch <= 'z')
        ch = static_cast<char>(ch - 'a' + 'A');

    switch (ch)
    {
    case 'A': { constexpr uint8_t g[7] = {14,17,17,31,17,17,17}; return g[row]; }
    case 'B': { constexpr uint8_t g[7] = {30,17,17,30,17,17,30}; return g[row]; }
    case 'C': { constexpr uint8_t g[7] = {14,17,16,16,16,17,14}; return g[row]; }
    case 'D': { constexpr uint8_t g[7] = {30,17,17,17,17,17,30}; return g[row]; }
    case 'E': { constexpr uint8_t g[7] = {31,16,16,30,16,16,31}; return g[row]; }
    case 'F': { constexpr uint8_t g[7] = {31,16,16,30,16,16,16}; return g[row]; }
    case 'G': { constexpr uint8_t g[7] = {14,17,16,23,17,17,15}; return g[row]; }
    case 'H': { constexpr uint8_t g[7] = {17,17,17,31,17,17,17}; return g[row]; }
    case 'I': { constexpr uint8_t g[7] = {14,4,4,4,4,4,14}; return g[row]; }
    case 'J': { constexpr uint8_t g[7] = {7,2,2,2,18,18,12}; return g[row]; }
    case 'K': { constexpr uint8_t g[7] = {17,18,20,24,20,18,17}; return g[row]; }
    case 'L': { constexpr uint8_t g[7] = {16,16,16,16,16,16,31}; return g[row]; }
    case 'M': { constexpr uint8_t g[7] = {17,27,21,21,17,17,17}; return g[row]; }
    case 'N': { constexpr uint8_t g[7] = {17,25,21,19,17,17,17}; return g[row]; }
    case 'O': { constexpr uint8_t g[7] = {14,17,17,17,17,17,14}; return g[row]; }
    case 'P': { constexpr uint8_t g[7] = {30,17,17,30,16,16,16}; return g[row]; }
    case 'Q': { constexpr uint8_t g[7] = {14,17,17,17,21,18,13}; return g[row]; }
    case 'R': { constexpr uint8_t g[7] = {30,17,17,30,20,18,17}; return g[row]; }
    case 'S': { constexpr uint8_t g[7] = {15,16,16,14,1,1,30}; return g[row]; }
    case 'T': { constexpr uint8_t g[7] = {31,4,4,4,4,4,4}; return g[row]; }
    case 'U': { constexpr uint8_t g[7] = {17,17,17,17,17,17,14}; return g[row]; }
    case 'V': { constexpr uint8_t g[7] = {17,17,17,17,17,10,4}; return g[row]; }
    case 'W': { constexpr uint8_t g[7] = {17,17,17,21,21,21,10}; return g[row]; }
    case 'X': { constexpr uint8_t g[7] = {17,17,10,4,10,17,17}; return g[row]; }
    case 'Y': { constexpr uint8_t g[7] = {17,17,10,4,4,4,4}; return g[row]; }
    case 'Z': { constexpr uint8_t g[7] = {31,1,2,4,8,16,31}; return g[row]; }
    case '0': { constexpr uint8_t g[7] = {14,17,19,21,25,17,14}; return g[row]; }
    case '1': { constexpr uint8_t g[7] = {4,12,4,4,4,4,14}; return g[row]; }
    case '2': { constexpr uint8_t g[7] = {14,17,1,2,4,8,31}; return g[row]; }
    case '3': { constexpr uint8_t g[7] = {30,1,1,14,1,1,30}; return g[row]; }
    case '4': { constexpr uint8_t g[7] = {2,6,10,18,31,2,2}; return g[row]; }
    case '5': { constexpr uint8_t g[7] = {31,16,16,30,1,1,30}; return g[row]; }
    case '6': { constexpr uint8_t g[7] = {14,16,16,30,17,17,14}; return g[row]; }
    case '7': { constexpr uint8_t g[7] = {31,1,2,4,8,8,8}; return g[row]; }
    case '8': { constexpr uint8_t g[7] = {14,17,17,14,17,17,14}; return g[row]; }
    case '9': { constexpr uint8_t g[7] = {14,17,17,15,1,1,14}; return g[row]; }
    case ':': { constexpr uint8_t g[7] = {0,4,4,0,4,4,0}; return g[row]; }
    case '-': { constexpr uint8_t g[7] = {0,0,0,31,0,0,0}; return g[row]; }
    case '.': { constexpr uint8_t g[7] = {0,0,0,0,0,12,12}; return g[row]; }
    case '/': { constexpr uint8_t g[7] = {1,1,2,4,8,16,16}; return g[row]; }
    case '[': { constexpr uint8_t g[7] = {14,8,8,8,8,8,14}; return g[row]; }
    case ']': { constexpr uint8_t g[7] = {14,2,2,2,2,2,14}; return g[row]; }
    default: return 0;
    }
}

void drawText(UiFrame& frame, int x, int y, const std::string& text, uint32_t rgba, int scale = 3)
{
    if (g_fontRenderer.available())
    {
        g_fontRenderer.draw(frame, x, y, text, rgba, std::max(12, scale * 8));
        return;
    }

    int cursor = x;
    for (char ch : text)
    {
        if (ch == ' ')
        {
            cursor += 4 * scale;
            continue;
        }

        for (int row = 0; row < 7; ++row)
        {
            const uint8_t bits = glyphRow(ch, row);
            for (int col = 0; col < 5; ++col)
            {
                if (bits & (1 << (4 - col)))
                    fillRect(frame, {cursor + col * scale, y + row * scale, scale, scale}, rgba);
            }
        }
        cursor += 6 * scale;
    }
}

struct NdsScreenRects {
    Rect top;
    Rect bottom;
};

NdsScreenRects currentNdsScreenRects()
{
    const int screenW = 512;
    const int screenH = 384;
    const int gap = 42;
    const int startX = (kScreenWidth - screenW * 2 - gap) / 2;
    const int screenY = 138;
    return {{startX, screenY, screenW, screenH}, {startX + screenW + gap, screenY, screenW, screenH}};
}

NdsTouchState touchStateFromScreen(bool menuVisible)
{
    if (menuVisible)
        return {};

    HidTouchScreenState touchState {};
    if (!hidGetTouchScreenStates(&touchState, 1) || touchState.count == 0)
        return {};

    const auto rects = currentNdsScreenRects();
    const int x = static_cast<int>(touchState.touches[0].x);
    const int y = static_cast<int>(touchState.touches[0].y);
    if (x < rects.bottom.x || x >= rects.bottom.x + rects.bottom.w ||
        y < rects.bottom.y || y >= rects.bottom.y + rects.bottom.h)
        return {};

    NdsTouchState out;
    out.down = true;
    out.x = std::clamp((x - rects.bottom.x) / 2, 0, 255);
    out.y = std::clamp((y - rects.bottom.y) / 2, 0, 191);
    return out;
}

void blitNdsScreen2x(UiFrame& frame, Rect dst, const std::vector<uint32_t>& ndsFrame, int srcY)
{
    if (ndsFrame.size() < static_cast<size_t>(kNdsWidth) * kNdsHeight)
        return;

    const int scale = 2;
    const int drawW = kNdsWidth * scale;
    const int drawH = kNdsScreenHeight * scale;
    const int x0 = dst.x + (dst.w - drawW) / 2;
    const int y0 = dst.y + (dst.h - drawH) / 2;

    if (x0 >= 0 && y0 >= 0 && x0 + drawW <= frame.width && y0 + drawH <= frame.height)
    {
        for (int y = 0; y < kNdsScreenHeight; ++y)
        {
            const uint32_t* src = ndsFrame.data() + static_cast<size_t>(srcY + y) * kNdsWidth;
            uint32_t* row0 = frame.pixels + static_cast<size_t>(y0 + y * scale) * frame.stridePixels + x0;
            uint32_t* row1 = row0 + frame.stridePixels;
            for (int x = 0; x < kNdsWidth; ++x)
            {
                const uint32_t px = src[x];
                const int dx = x * scale;
                row0[dx] = px;
                row0[dx + 1] = px;
                row1[dx] = px;
                row1[dx + 1] = px;
            }
        }
        return;
    }

    for (int y = 0; y < kNdsScreenHeight; ++y)
    {
        const uint32_t* src = ndsFrame.data() + static_cast<size_t>(srcY + y) * kNdsWidth;
        for (int x = 0; x < kNdsWidth; ++x)
        {
            const uint32_t px = src[x];
            const int dx = x0 + x * scale;
            const int dy = y0 + y * scale;
            if (dx < 0 || dy < 0 || dx + 1 >= frame.width || dy + 1 >= frame.height)
                continue;
            uint32_t* row0 = frame.pixels + dy * frame.stridePixels;
            uint32_t* row1 = frame.pixels + (dy + 1) * frame.stridePixels;
            row0[dx] = px;
            row0[dx + 1] = px;
            row1[dx] = px;
            row1[dx + 1] = px;
        }
    }
}

void drawGameLayer(UiFrame& frame,
                   const GameInfo& game,
                   int tick,
                   const std::string& status,
                   const std::vector<uint32_t>& ndsFrame,
                   bool ndsReady,
                   double fps,
                   const NdsFrameTimings& timings)
{
    fillRect(frame, {0, 0, frame.width, frame.height}, color(10, 13, 18));
    fillRect(frame, {0, 0, frame.width, 74}, color(18, 26, 34));
    drawText(frame, 38, 28, "GBASTATION NDS", color(218, 239, 255), 3);
    drawText(frame, 930, 24, "FPS " + std::to_string(static_cast<int>(fps + 0.5)) +
        "  EMU " + std::to_string(timings.totalMs) + "MS", color(128, 180, 210), 2);
    drawText(frame, 930, 48, "RUN " + std::to_string(timings.runMs) +
        "  AUD " + std::to_string(timings.audioMs) +
        "  CAP " + std::to_string(timings.captureMs), color(98, 145, 172), 2);

    const uint32_t topTint = color(31, 92, 126);
    const uint32_t bottomTint = color(45, 82, 63);

    const auto rects = currentNdsScreenRects();
    Rect top = rects.top;
    Rect bottom = rects.bottom;
    fillRect(frame, top, topTint);
    fillRect(frame, bottom, bottomTint);

    if (ndsReady)
    {
        blitNdsScreen2x(frame, top, ndsFrame, 0);
        blitNdsScreen2x(frame, bottom, ndsFrame, kNdsScreenHeight);
    }
    else
    {
        for (int i = 0; i < 12; ++i)
        {
            const int stripe = (tick * 2 + i * 56) % (top.w + 120) - 120;
            fillRectAlpha(frame, {top.x + stripe, top.y, 28, top.h}, color(154, 214, 238), 38);
            fillRectAlpha(frame, {bottom.x + bottom.w - stripe - 28, bottom.y, 28, bottom.h}, color(180, 228, 175), 34);
        }
    }

    drawRect(frame, top, color(102, 180, 219), 4);
    drawRect(frame, bottom, color(116, 186, 126), 4);
    if (!ndsReady)
    {
        drawText(frame, top.x + 28, top.y + 26, "TOP SCREEN", color(229, 246, 255), 4);
        drawText(frame, bottom.x + 28, bottom.y + 26, "BOTTOM SCREEN", color(232, 255, 230), 4);
        drawText(frame, top.x + 28, top.y + 84, "WAITING FOR CORE", color(174, 218, 239), 2);
        drawText(frame, bottom.x + 28, bottom.y + 84, status, color(185, 229, 184), 2);
    }

    drawText(frame, 44, 612, "游戏: " + game.title, color(226, 229, 231), 2);
    drawText(frame, 44, 644, "分辨率: X" + std::to_string(game.internalResolution) + "  菜单: PLUS", color(145, 174, 190), 2);
    if (!status.empty())
        drawText(frame, 440, 644, status, color(248, 211, 120), 2);
}

void drawMenuLayer(UiFrame& frame, int selected, float transition)
{
    if (transition <= 0.01f)
        return;

    const uint8_t dimAlpha = static_cast<uint8_t>(180 * transition);
    fillRectAlpha(frame, {0, 0, frame.width, frame.height}, color(0, 0, 0), dimAlpha);

    const int panelW = static_cast<int>(1060 * transition);
    const int panelH = static_cast<int>(610 * transition);
    const int panelX = (frame.width - panelW) / 2;
    const int panelY = (frame.height - panelH) / 2;
    if (panelW < 120 || panelH < 120)
        return;

    fillRect(frame, {panelX, panelY, panelW, panelH}, color(18, 20, 25));
    drawRect(frame, {panelX, panelY, panelW, panelH}, color(72, 84, 98), 2);
    fillRect(frame, {panelX, panelY, panelW, 74}, color(28, 32, 40));
    drawText(frame, panelX + 34, panelY + 24, "游戏菜单", color(238, 246, 255), 4);
    drawText(frame, panelX + panelW - 285, panelY + 30, "A 确认  B 返回", color(144, 166, 178), 2);

    const int tabW = panelW / 4;
    const int tabX = panelX + 18;
    const int tabY = panelY + 96;
    const int tabH = panelH - 126;
    const int contentX = panelX + tabW + 34;
    const int contentY = panelY + 96;
    const int contentW = panelW - tabW - 56;
    const int contentH = panelH - 126;

    fillRect(frame, {tabX, tabY, tabW - 26, tabH}, color(20, 23, 29));
    fillRect(frame, {panelX + tabW + 14, tabY, 1, tabH}, color(255, 255, 255));
    fillRectAlpha(frame, {panelX + tabW + 14, tabY, 1, tabH}, color(0, 0, 0), 210);
    fillRect(frame, {contentX, contentY, contentW, contentH}, color(23, 27, 34));
    drawRect(frame, {contentX, contentY, contentW, contentH}, color(44, 54, 66), 2);

    for (int i = 0; i < static_cast<int>(std::size(kMenuItems)); ++i)
    {
        const int y = tabY + 18 + i * 58;
        const bool active = i == selected;
        fillRect(frame, {tabX + 16, y, tabW - 58, 44}, active ? color(55, 105, 134) : color(30, 35, 43));
        if (active)
        {
            fillRect(frame, {tabX + 16, y, 6, 44}, color(245, 198, 96));
            drawRect(frame, {tabX + 16, y, tabW - 58, 44}, color(87, 170, 214), 2);
        }
        fillRect(frame, {tabX + 34, y + 13, 18, 18}, active ? color(245, 198, 96) : color(112, 130, 144));
        drawText(frame, tabX + 66, y + 14, kMenuItems[i].label, active ? color(255, 249, 220) : color(205, 218, 225), 2);
    }

    const MenuItem& active = kMenuItems[selected];
    drawText(frame, contentX + 34, contentY + 34, active.label, color(236, 244, 250), 4);
    fillRect(frame, {contentX + 34, contentY + 88, contentW - 68, 1}, color(75, 88, 102));

    switch (active.action)
    {
    case MenuAction::Resume:
        drawText(frame, contentX + 34, contentY + 130, "返回当前游戏，不改变模拟状态。", color(186, 205, 216), 2);
        drawText(frame, contentX + 34, contentY + 178, "按 A 返回游戏", color(245, 198, 96), 3);
        break;
    case MenuAction::SaveState:
        drawText(frame, contentX + 34, contentY + 130, "即时存档槽位稍后接入。", color(186, 205, 216), 2);
        drawText(frame, contentX + 34, contentY + 178, "当前已支持退出时自动保存 SRAM", color(245, 198, 96), 3);
        break;
    case MenuAction::LoadState:
        drawText(frame, contentX + 34, contentY + 130, "即时读档槽位稍后接入。", color(186, 205, 216), 2);
        drawText(frame, contentX + 34, contentY + 178, "当前启动时会读取电池存档", color(245, 198, 96), 3);
        break;
    case MenuAction::Cheats:
        drawText(frame, contentX + 34, contentY + 130, "金手指列表和开关稍后接入。", color(186, 205, 216), 2);
        drawText(frame, contentX + 34, contentY + 178, "GameDB 金手指路径已读取", color(245, 198, 96), 3);
        break;
    case MenuAction::Display:
        drawText(frame, contentX + 34, contentY + 130, "画面布局、比例和滤镜设置稍后接入。", color(186, 205, 216), 2);
        drawText(frame, contentX + 34, contentY + 178, "当前模式为 X1 软件渲染", color(245, 198, 96), 3);
        break;
    case MenuAction::Reset:
        drawText(frame, contentX + 34, contentY + 130, "重新启动当前 NDS 游戏。", color(186, 205, 216), 2);
        drawText(frame, contentX + 34, contentY + 178, "按 A 重置游戏", color(245, 198, 96), 3);
        break;
    case MenuAction::Exit:
        drawText(frame, contentX + 34, contentY + 130, "保存 SRAM 并返回 GBAStation 主程序。", color(186, 205, 216), 2);
        drawText(frame, contentX + 34, contentY + 178, "按 A 退出游戏", color(245, 198, 96), 3);
        break;
    }
}

bool setReturnNro(const std::string& returnNro)
{
    if (returnNro.empty())
    {
        appendLog("GBAStationNDSStub: returnNro empty, cannot exit to main NRO");
        return false;
    }

    if (!envHasNextLoad())
    {
        appendLog("GBAStationNDSStub: envHasNextLoad=false, cannot exit to main NRO");
        return false;
    }

    const std::string args = quoteArg(returnNro);
    const Result rc = envSetNextLoad(returnNro.c_str(), args.c_str());
    appendLog("GBAStationNDSStub: envSetNextLoad exit rc=0x%x path=%s", rc, returnNro.c_str());
    return R_SUCCEEDED(rc);
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

} // namespace

int main(int argc, char* argv[])
{
    svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
    appendLog("GBAStationNDSStub: start argc=%d", argc);
    for (int i = 0; i < argc; ++i)
        appendLog("GBAStationNDSStub: argv[%d]=%s", i, argv[i] ? argv[i] : "(null)");

    const char* romPath = "";
    const char* returnNro = "";

    for (int i = 1; i < argc; ++i)
    {
        if (!argv[i])
            continue;

        if (std::strcmp(argv[i], "--return") == 0 && i + 1 < argc)
        {
            returnNro = argv[i + 1];
            ++i;
            continue;
        }

        if (!romPath[0] && !endsWithNoCase(argv[i], ".nro"))
        {
            romPath = argv[i];
            break;
        }
    }

    std::string returnNroPath = returnNro && returnNro[0] ? returnNro : "sdmc:/switch/GBAStation.nro";
    appendLog("GBAStationNDSStub: romPath=%s", romPath && romPath[0] ? romPath : "(empty)");
    appendLog("GBAStationNDSStub: returnNro=%s", returnNroPath.c_str());

    std::optional<nlohmann::json> record;
    if (romPath && romPath[0])
    {
        record = loadNdsGameDbRecord(romPath);
        if (record.has_value())
            logGameDbRecord(*record);
        else
            appendLog("GBAStationNDSStub: gameDb.found=0");
    }

    GameInfo game = buildGameInfo(romPath ? romPath : "", record);
    appendLog("GBAStationNDSStub: ui start title=%s", game.title.c_str());

    if (beiklive::nds_stub::ShouldUseDekoRuntime())
    {
        beiklive::nds_stub::DekoRunOptions dekoOptions;
        dekoOptions.romPath = game.romPath;
        dekoOptions.title = game.title;
        dekoOptions.savePath = game.savePath;
        dekoOptions.returnNroPath = returnNroPath;
        return beiklive::nds_stub::RunDekoRuntime(dekoOptions);
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    hidInitializeTouchScreen();
    PadState pad;
    padInitializeDefault(&pad);

    Framebuffer fb {};
    Result rc = framebufferCreate(&fb, nwindowGetDefault(), kScreenWidth, kScreenHeight, PIXEL_FORMAT_RGBA_8888, 2);
    if (R_FAILED(rc))
    {
        appendLog("GBAStationNDSStub: framebufferCreate failed rc=0x%x", rc);
        setReturnNro(returnNroPath);
        return 1;
    }

    rc = framebufferMakeLinear(&fb);
    if (R_FAILED(rc))
    {
        appendLog("GBAStationNDSStub: framebufferMakeLinear failed rc=0x%x", rc);
        framebufferClose(&fb);
        setReturnNro(returnNroPath);
        return 1;
    }

    g_fontRenderer.ensureLoaded();

    bool running = true;
    bool pendingReturnToMain = false;
    bool menuVisible = false;
    int selected = 0;
    int tick = 0;
    float menuTransition = 0.0f;
    std::string status = "LOADING";

    NdsRuntime runtime;
    runtime.start(game);
    status = runtime.status();
    double displayedFps = 0.0;
    int fpsFrames = 0;
    NdsFrameTimings lastTimings;
    auto fpsWindowStart = std::chrono::steady_clock::now();

    while (appletMainLoop() && running)
    {
        const auto frameStart = std::chrono::steady_clock::now();
        padUpdate(&pad);
        const u64 down = padGetButtonsDown(&pad);

        if (down & HidNpadButton_Plus)
            menuVisible = !menuVisible;
        if (down & HidNpadButton_Minus)
            menuVisible = true;

        if (menuVisible)
        {
            if (down & HidNpadButton_AnyUp)
                selected = (selected + static_cast<int>(std::size(kMenuItems)) - 1) % static_cast<int>(std::size(kMenuItems));
            if (down & HidNpadButton_AnyDown)
                selected = (selected + 1) % static_cast<int>(std::size(kMenuItems));
            if (down & HidNpadButton_B)
            {
                menuVisible = false;
                status = "RESUME";
            }
            if (down & HidNpadButton_A)
            {
                const MenuItem& item = kMenuItems[selected];
                appendLog("GBAStationNDSStub: menu action=%s", item.label);
                switch (item.action)
                {
                case MenuAction::Resume:
                    menuVisible = false;
                    status = "RESUME";
                    break;
                case MenuAction::SaveState:
                    status = "SAVE TODO";
                    break;
                case MenuAction::LoadState:
                    status = "LOAD STATE TODO";
                    break;
                case MenuAction::Cheats:
                    status = game.cheatPath.empty() ? "CHEATS TODO" : "CHEATS DB READY";
                    break;
                case MenuAction::Display:
                    status = "DISPLAY TODO";
                    break;
                case MenuAction::Reset:
                    runtime.reset();
                    status = runtime.status();
                    break;
                case MenuAction::Exit:
                    status = "EXITING";
                    pendingReturnToMain = true;
                    running = false;
                    break;
                }
            }
        }

        if (runtime.ready())
        {
            const uint32_t keyMask = menuVisible ? 0x0FFFu : dsKeyMaskFromPad(pad);
            const NdsTouchState touch = touchStateFromScreen(menuVisible);
            lastTimings = runtime.runFrame(keyMask, touch);
            if (status == "RUNNING" || status == "RESUME" || status == "LOADING")
                status = runtime.status();
        }

        const float target = menuVisible ? 1.0f : 0.0f;
        menuTransition += (target - menuTransition) * 0.24f;
        if (!menuVisible && menuTransition < 0.01f)
            menuTransition = 0.0f;
        if (menuVisible && menuTransition > 0.99f)
            menuTransition = 1.0f;

        u32 stride = 0;
        void* frameBuf = framebufferBegin(&fb, &stride);
        if (frameBuf)
        {
            UiFrame frame;
            frame.pixels = static_cast<uint32_t*>(frameBuf);
            frame.stridePixels = static_cast<int>(stride / sizeof(uint32_t));
            drawGameLayer(frame,
                          game,
                          tick,
                          status,
                          runtime.frame(),
                          runtime.ready(),
                          displayedFps,
                          lastTimings);
            drawMenuLayer(frame, selected, menuTransition);
        }
        framebufferEnd(&fb);

        ++tick;
        ++fpsFrames;
        const auto now = std::chrono::steady_clock::now();
        const auto fpsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - fpsWindowStart).count();
        if (fpsElapsed >= 1000)
        {
            displayedFps = static_cast<double>(fpsFrames) * 1000.0 / static_cast<double>(fpsElapsed);
            fpsFrames = 0;
            fpsWindowStart = now;
        }

        const auto frameEnd = std::chrono::steady_clock::now();
        constexpr auto kFrameBudget = std::chrono::microseconds(16667);
        const auto used = std::chrono::duration_cast<std::chrono::microseconds>(frameEnd - frameStart);
        if (used < kFrameBudget)
        {
            const auto sleepUs = std::chrono::duration_cast<std::chrono::microseconds>(kFrameBudget - used).count();
            if (sleepUs > 500)
                svcSleepThread(static_cast<int64_t>(sleepUs) * 1000);
        }
    }

    runtime.stop();
    framebufferClose(&fb);
    if (pendingReturnToMain)
        setReturnNro(returnNroPath);
    appendLog("GBAStationNDSStub: exit");
    return 0;
}
