#include "Pico8Core.hpp"

#include "Pico8Filesystem.hpp"

#include "PicoRam.h"
#include "host.h"
#include "hostVmShared.h"
#include "nibblehelpers.h"
#include "logger.h"
#include "vm.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <mutex>

#include <borealis.hpp>

namespace
{
    constexpr int WIDTH = 128;
    constexpr int HEIGHT = 128;
    constexpr size_t RAM_SIZE = 0x10000;
    constexpr size_t STATE_HEADER_SIZE = 24;
    constexpr size_t LUA_STATE_CAPACITY = 16 * 1024 * 1024;
    constexpr std::array<uint8_t, 8> STATE_MAGIC{
        {'P', '8', 'S', 'T', 'A', 'T', 'E', '1'}};
    std::once_flag g_loggerInit;

    template <typename T>
    void writeStateValue(std::vector<uint8_t>& output, size_t offset, T value)
    {
        std::memcpy(output.data() + offset, &value, sizeof(value));
    }

    template <typename T>
    T readStateValue(const uint8_t* data, size_t offset)
    {
        T value{};
        std::memcpy(&value, data + offset, sizeof(value));
        return value;
    }
}

namespace beiklive::pico8
{
    struct Core::Impl
    {
        std::unique_ptr<Host> host;
        std::unique_ptr<Vm> vm;
        Audio outputAudio;
        InputState input;
        std::vector<uint8_t> rgba = std::vector<uint8_t>(WIDTH * HEIGHT * 4, 0);
        std::vector<uint32_t> audioFrames;
        std::string gamePath;
        std::string error;
        float frameAccumulator = 0.f;
        double audioAccumulator = 0.0;
        bool initialized = false;
        bool loaded = false;
        bool paused = false;
        bool runtimeErrorLogged = false;

        void convertFrame()
        {
            if (!vm) return;
            uint8_t* framebuffer = vm->GetPicoInteralFb();
            uint8_t* paletteMap = vm->GetScreenPaletteMap();
            PicoRam* memory = vm->getPicoRam();
            Color* palette = host->GetPaletteColors();
            if (!framebuffer || !paletteMap || !memory || !palette)
                return;

            const uint8_t mode = memory->drawState.drawMode;
            for (int y = 0; y < HEIGHT; ++y) {
                for (int x = 0; x < WIDTH; ++x) {
                    int sx = x;
                    int sy = y;
                    switch (mode) {
                        case 1: sx = x / 2; break;
                        case 2: sy = y / 2; break;
                        case 3: sx = x / 2; sy = y / 2; break;
                        case 129: sx = WIDTH - 1 - x; break;
                        case 130: sy = HEIGHT - 1 - y; break;
                        case 131: sx = WIDTH - 1 - x; sy = HEIGHT - 1 - y; break;
                        case 133: sx = y; sy = WIDTH - 1 - x; break;
                        case 134: sx = WIDTH - 1 - x; sy = HEIGHT - 1 - y; break;
                        case 135: sx = HEIGHT - 1 - y; sy = x; break;
                        default: break;
                    }
                    const uint8_t colorIndex = getPixelNibble(sx, sy, framebuffer);
                    const Color color = palette[paletteMap[colorIndex] & 0x8f];
                    const size_t offset = static_cast<size_t>(y * WIDTH + x) * 4;
                    rgba[offset + 0] = color.Red;
                    rgba[offset + 1] = color.Green;
                    rgba[offset + 2] = color.Blue;
                    rgba[offset + 3] = color.Alpha;
                }
            }
        }

        void produceAudio(int fps)
        {
            if (!vm || !outputAudio.isInitialized() || fps <= 0)
                return;
            audioAccumulator += 22050.0 / static_cast<double>(fps);
            const size_t frames = static_cast<size_t>(audioAccumulator);
            audioAccumulator -= static_cast<double>(frames);
            if (frames == 0)
                return;
            audioFrames.resize(frames);
            vm->FillAudioBuffer(audioFrames.data(), 0, frames);
            outputAudio.submit(
                reinterpret_cast<const int16_t*>(audioFrames.data()), frames);
        }
    };

    Core::Core() : m_impl(std::make_unique<Impl>()) {}
    Core::~Core() { Shutdown(); }

    bool Core::Initialize()
    {
        if (m_impl->initialized)
            return true;
        Filesystem::ensureDirectories();
        std::call_once(g_loggerInit, []() {
            const std::string prefix = Filesystem::rootPath() + "/";
            Logger_Initialize(prefix.c_str());
            Logger_Write("BeikLiveStation FAKE-08 runtime log started\n");
        });
        brls::Logger::info("Pico8Core: initializing FAKE-08 runtime root={}",
                           Filesystem::rootPath());
        m_impl->host = std::make_unique<Host>(128, 128);
        m_impl->vm = std::make_unique<Vm>(m_impl->host.get());
        m_impl->initialized = true;
        brls::Logger::info("Pico8Core: runtime initialized");
        return true;
    }

    void Core::Shutdown()
    {
        if (!m_impl)
            return;
        UnloadGame();
        m_impl->outputAudio.shutdown();
        m_impl->vm.reset();
        m_impl->host.reset();
        m_impl->initialized = false;
    }

    bool Core::LoadGame(const std::string& path)
    {
        if (!Initialize() || path.empty())
            return false;
        UnloadGame();
        std::error_code fileError;
        const auto fileSize = std::filesystem::file_size(path, fileError);
        brls::Logger::info("Pico8Core: loading cart path={} size={}", path,
                           fileError ? 0 : fileSize);
        Logger_Write("Loading cart: %s (size=%llu)\n", path.c_str(),
                     static_cast<unsigned long long>(fileError ? 0 : fileSize));
        if (!m_impl->vm->LoadCart(path, false)) {
            m_impl->error = m_impl->vm->GetBiosError();
            if (m_impl->error.empty())
                m_impl->error = "无法加载 PICO-8 游戏";
            brls::Logger::error("Pico8Core: cart load failed path={} error={}",
                                path, m_impl->error);
            Logger_Write("Cart load failed: %s\n", m_impl->error.c_str());
            return false;
        }
        m_impl->vm->vm_run();
        const std::string startupError = m_impl->vm->GetBiosError();
        if (!startupError.empty()) {
            m_impl->error = startupError;
            brls::Logger::error(
                "Pico8Core: cart startup failed path={} error={}",
                path, m_impl->error);
            Logger_Write("Cart startup failed: %s\n", m_impl->error.c_str());
            m_impl->vm->CloseCart();
            return false;
        }
        if (!m_impl->vm->ExecuteLua(
                "eris.init_persist_all() "
                "eris.original_G.__cart_sandbox=nil "
                "eris.original_G.__z8_loop=nil", "")) {
            brls::Logger::warning(
                "Pico8Core: save-state persistence initialization failed path={}",
                path);
            Logger_Write("Save-state persistence initialization failed\n");
        }
        m_impl->gamePath = path;
        m_impl->frameAccumulator = 0.f;
        m_impl->audioAccumulator = 0.0;
        m_impl->paused = false;
        m_impl->loaded = true;
        m_impl->runtimeErrorLogged = false;
        m_impl->error.clear();
        m_impl->outputAudio.initialize();
        m_impl->convertFrame();
        brls::Logger::info("Pico8Core: cart started path={} targetFps={}",
                           path, targetFps());
        Logger_Write("Cart started successfully at %d fps\n", targetFps());
        return true;
    }

    void Core::UnloadGame()
    {
        if (!m_impl || !m_impl->loaded)
            return;
        m_impl->outputAudio.pause();
        if (m_impl->vm)
            m_impl->vm->CloseCart();
        m_impl->loaded = false;
        m_impl->paused = false;
        m_impl->gamePath.clear();
        host_bridge::setInput(0, 0);
    }

    bool Core::RunFrame(float deltaSeconds)
    {
        if (!m_impl->loaded || m_impl->paused || !m_impl->vm)
            return false;
        const int fps = std::max(1, targetFps());
        const float frameTime = 1.f / static_cast<float>(fps);
        m_impl->frameAccumulator += std::max(0.f, std::min(deltaSeconds, 0.1f));
        bool ran = false;
        int steps = 0;
        while (m_impl->frameAccumulator >= frameTime && steps < 3) {
            host_bridge::setInput(m_impl->input.down, m_impl->input.held);
            if (!m_impl->vm->Step()) {
                m_impl->error = m_impl->vm->GetBiosError();
                if (m_impl->error.empty())
                    m_impl->error = "FAKE-08 main loop stopped without details";
                if (!m_impl->runtimeErrorLogged) {
                    m_impl->runtimeErrorLogged = true;
                    brls::Logger::error(
                        "Pico8Core: runtime step failed path={} error={}",
                        m_impl->gamePath, m_impl->error);
                    Logger_Write("Runtime step failed: %s\n",
                                 m_impl->error.c_str());
                }
                return false;
            }
            m_impl->input.down = 0;
            m_impl->produceAudio(fps);
            m_impl->frameAccumulator -= frameTime;
            ran = true;
            ++steps;
        }
        if (ran)
            m_impl->convertFrame();
        return ran;
    }

    const uint8_t* Core::GetFrameBuffer() const
    {
        return m_impl->rgba.data();
    }

    void Core::SetInput(const InputState& state)
    {
        m_impl->input.down |= state.down;
        m_impl->input.held = state.held;
    }

    void Core::Reset()
    {
        const std::string path = m_impl->gamePath;
        if (!path.empty())
            LoadGame(path);
    }

    void Core::Pause()
    {
        m_impl->paused = true;
        m_impl->outputAudio.pause();
    }

    void Core::Resume()
    {
        m_impl->paused = false;
        m_impl->outputAudio.resume();
    }

    bool Core::SaveState(std::vector<uint8_t>& output)
    {
        output.clear();
        if (!m_impl->loaded || !m_impl->vm)
            return false;
        PicoRam* memory = m_impl->vm->getPicoRam();
        if (!memory)
            return false;

        std::vector<char> luaState(LUA_STATE_CAPACITY);
        const size_t luaSize = m_impl->vm->serializeLuaState(luaState.data());
        if (luaSize == 0 || luaSize > luaState.size()) {
            brls::Logger::error(
                "Pico8Core: quick save failed path={} luaSize={}",
                m_impl->gamePath, luaSize);
            Logger_Write("Quick save failed: lua serialization returned %llu\n",
                static_cast<unsigned long long>(luaSize));
            return false;
        }

        output.resize(STATE_HEADER_SIZE + RAM_SIZE + luaSize);
        std::memcpy(output.data(), STATE_MAGIC.data(), STATE_MAGIC.size());
        writeStateValue<uint32_t>(output, 8, 1);
        writeStateValue<uint32_t>(output, 12, static_cast<uint32_t>(RAM_SIZE));
        writeStateValue<uint64_t>(output, 16, static_cast<uint64_t>(luaSize));
        std::memcpy(output.data() + STATE_HEADER_SIZE, memory->data, RAM_SIZE);
        std::memcpy(output.data() + STATE_HEADER_SIZE + RAM_SIZE,
                    luaState.data(), luaSize);
        brls::Logger::info("Pico8Core: quick save created bytes={}", output.size());
        return true;
    }

    bool Core::LoadState(const uint8_t* data, size_t size)
    {
        if (!m_impl->loaded || !m_impl->vm || !data ||
            size < STATE_HEADER_SIZE + RAM_SIZE ||
            std::memcmp(data, STATE_MAGIC.data(), STATE_MAGIC.size()) != 0)
            return false;
        const uint32_t version = readStateValue<uint32_t>(data, 8);
        const uint32_t ramSize = readStateValue<uint32_t>(data, 12);
        const uint64_t luaSize = readStateValue<uint64_t>(data, 16);
        if (version != 1 || ramSize != RAM_SIZE || luaSize == 0 ||
            luaSize > LUA_STATE_CAPACITY ||
            STATE_HEADER_SIZE + RAM_SIZE + luaSize != size)
            return false;
        PicoRam* memory = m_impl->vm->getPicoRam();
        if (!memory)
            return false;

        m_impl->outputAudio.pause();
        std::memcpy(memory->data, data + STATE_HEADER_SIZE, RAM_SIZE);
        m_impl->vm->deserializeLuaState(
            reinterpret_cast<const char*>(
                data + STATE_HEADER_SIZE + RAM_SIZE),
            static_cast<size_t>(luaSize));
        m_impl->vm->ExecuteLua(
            "rawset(debug.getregistry(), '__PICO8_SANDBOX', __cart_sandbox)",
            "");
        m_impl->frameAccumulator = 0.f;
        m_impl->audioAccumulator = 0.0;
        m_impl->input = {};
        host_bridge::setInput(0, 0);
        m_impl->convertFrame();
        m_impl->outputAudio.initialize();
        brls::Logger::info("Pico8Core: quick state loaded bytes={}", size);
        return true;
    }

    size_t Core::GetStateSize() const { return 0; }
    bool Core::isInitialized() const { return m_impl->initialized; }
    bool Core::isGameLoaded() const { return m_impl->loaded; }
    int Core::targetFps() const
    {
        return m_impl->vm ? std::max(1, m_impl->vm->GetTargetFps()) : 60;
    }
    const std::string& Core::lastError() const { return m_impl->error; }
}
