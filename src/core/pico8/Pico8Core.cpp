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
    constexpr size_t MAX_LUA_STATE_SIZE = 32 * 1024 * 1024;
    constexpr std::array<uint8_t, 8> STATE_MAGIC{
        {'P', '8', 'S', 'T', 'A', 'T', 'E', '1'}};
    std::once_flag g_loggerInit;

    template <typename Tag, typename Tag::type Member>
    struct PrivateMemberAccess
    {
        friend typename Tag::type getPrivateMember(Tag) { return Member; }
    };

    struct VmLuaStateTag
    {
        using type = lua_State* Vm::*;
        friend type getPrivateMember(VmLuaStateTag);
    };

    template struct PrivateMemberAccess<VmLuaStateTag, &Vm::_luaState>;

    lua_State* getVmLuaState(Vm* vm)
    {
        return vm ? vm->*getPrivateMember(VmLuaStateTag{}) : nullptr;
    }

    bool initializeStatePersistence(Vm* vm, std::string& error)
    {
        lua_State* state = getVmLuaState(vm);
        if (!state) {
            error = "Lua state is unavailable";
            return false;
        }
        const int stackTop = lua_gettop(state);
        lua_getglobal(state, "eris");
        if (!lua_istable(state, -1)) {
            error = "global eris table is unavailable";
            lua_settop(state, stackTop);
            return false;
        }
        lua_getfield(state, -1, "init_persist_all");
        if (!lua_isfunction(state, -1)) {
            error = "eris.init_persist_all is unavailable";
            lua_settop(state, stackTop);
            return false;
        }
        if (lua_pcall(state, 0, 0, 0) != LUA_OK) {
            const char* message = lua_tostring(state, -1);
            error = message ? message : "eris.init_persist_all failed";
            lua_settop(state, stackTop);
            return false;
        }
        lua_settop(state, stackTop);
        return true;
    }

    bool hasRestorableRuntime(lua_State* state)
    {
        if (!state)
            return false;
        const int stackTop = lua_gettop(state);
        lua_getglobal(state, "__z8_loop");
        const bool hasLoop = lua_isthread(state, -1);
        lua_getglobal(state, "__cart_sandbox");
        const bool hasSandbox = lua_istable(state, -1);
        lua_settop(state, stackTop);
        return hasLoop && hasSandbox;
    }

    bool persistLuaState(lua_State* state, std::vector<uint8_t>& output,
                         std::string& error)
    {
        if (!state || !hasRestorableRuntime(state)) {
            error = "cart Lua coroutine is not ready";
            return false;
        }
        const int stackTop = lua_gettop(state);
        lua_getglobal(state, "eris");
        if (!lua_istable(state, -1)) {
            error = "global eris table is unavailable";
            lua_settop(state, stackTop);
            return false;
        }
        lua_getfield(state, -1, "persist_all");
        if (!lua_isfunction(state, -1)) {
            error = "eris.persist_all is unavailable";
            lua_settop(state, stackTop);
            return false;
        }
        if (lua_pcall(state, 0, 1, 0) != LUA_OK) {
            const char* message = lua_tostring(state, -1);
            error = message ? message : "eris.persist_all failed";
            lua_settop(state, stackTop);
            return false;
        }
        size_t size = 0;
        const char* bytes = lua_tolstring(state, -1, &size);
        if (!bytes || size == 0 || size > MAX_LUA_STATE_SIZE) {
            error = "eris.persist_all returned an invalid state";
            lua_settop(state, stackTop);
            return false;
        }
        output.assign(reinterpret_cast<const uint8_t*>(bytes),
                      reinterpret_cast<const uint8_t*>(bytes) + size);
        lua_settop(state, stackTop);
        return true;
    }

    bool restoreLuaState(lua_State* state, const uint8_t* data, size_t size,
                         std::string& error)
    {
        if (!state || !data || size == 0 || size > MAX_LUA_STATE_SIZE) {
            error = "serialized Lua state is invalid";
            return false;
        }
        const int stackTop = lua_gettop(state);
        lua_getglobal(state, "eris");
        if (!lua_istable(state, -1)) {
            error = "global eris table is unavailable";
            lua_settop(state, stackTop);
            return false;
        }
        lua_getfield(state, -1, "restore_all");
        if (!lua_isfunction(state, -1)) {
            error = "eris.restore_all is unavailable";
            lua_settop(state, stackTop);
            return false;
        }
        lua_pushlstring(state, reinterpret_cast<const char*>(data), size);
        if (lua_pcall(state, 1, 0, 0) != LUA_OK) {
            const char* message = lua_tostring(state, -1);
            error = message ? message : "eris.restore_all failed";
            lua_settop(state, stackTop);
            return false;
        }

        lua_getglobal(state, "__cart_sandbox");
        if (!lua_istable(state, -1)) {
            error = "restored cart sandbox is unavailable";
            lua_settop(state, stackTop);
            return false;
        }
        lua_setfield(state, LUA_REGISTRYINDEX, "__PICO8_SANDBOX");
        const bool ready = hasRestorableRuntime(state);
        lua_settop(state, stackTop);
        if (!ready)
            error = "restored cart coroutine is unavailable";
        return ready;
    }

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
        bool statePersistenceReady = false;

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
        std::string persistenceError;
        m_impl->statePersistenceReady = initializeStatePersistence(
            m_impl->vm.get(), persistenceError);
        if (!m_impl->statePersistenceReady) {
            brls::Logger::warning(
                "Pico8Core: save-state persistence unavailable error={}",
                persistenceError);
            Logger_Write("Save-state persistence unavailable: %s\n",
                         persistenceError.c_str());
        } else {
            Logger_Write("Save-state persistence initialized\n");
        }
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
        if (!m_impl->loaded || !m_impl->vm ||
            !m_impl->statePersistenceReady)
            return false;
        PicoRam* memory = m_impl->vm->getPicoRam();
        if (!memory)
            return false;

        std::vector<uint8_t> luaState;
        std::string stateError;
        if (!persistLuaState(getVmLuaState(m_impl->vm.get()), luaState,
                            stateError)) {
            brls::Logger::error(
                "Pico8Core: quick save failed path={} error={}",
                m_impl->gamePath, stateError);
            Logger_Write("Quick save failed: %s\n", stateError.c_str());
            return false;
        }
        const size_t luaSize = luaState.size();

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
        if (!m_impl->loaded || !m_impl->vm ||
            !m_impl->statePersistenceReady || !data ||
            size < STATE_HEADER_SIZE + RAM_SIZE ||
            std::memcmp(data, STATE_MAGIC.data(), STATE_MAGIC.size()) != 0)
            return false;
        const uint32_t version = readStateValue<uint32_t>(data, 8);
        const uint32_t ramSize = readStateValue<uint32_t>(data, 12);
        const uint64_t luaSize = readStateValue<uint64_t>(data, 16);
        if (version != 1 || ramSize != RAM_SIZE || luaSize == 0 ||
            luaSize > MAX_LUA_STATE_SIZE ||
            STATE_HEADER_SIZE + RAM_SIZE + luaSize != size)
            return false;
        const std::string gamePath = m_impl->gamePath;
        m_impl->outputAudio.pause();

        auto buildRuntime = [&](std::unique_ptr<Host>& host,
                                std::unique_ptr<Vm>& vm,
                                bool restoreState,
                                std::string& error) -> bool {
            host = std::make_unique<Host>(WIDTH, HEIGHT);
            vm = std::make_unique<Vm>(host.get());
            if (!initializeStatePersistence(vm.get(), error))
                return false;
            if (!vm->LoadCart(gamePath, false)) {
                error = vm->GetBiosError();
                if (error.empty())
                    error = "failed to reload cart";
                return false;
            }
            vm->vm_run();
            error = vm->GetBiosError();
            if (!error.empty())
                return false;
            if (!restoreState)
                return true;

            PicoRam* candidateMemory = vm->getPicoRam();
            if (!candidateMemory) {
                error = "candidate PICO-8 memory is unavailable";
                return false;
            }
            if (!restoreLuaState(
                    getVmLuaState(vm.get()),
                    data + STATE_HEADER_SIZE + RAM_SIZE,
                    static_cast<size_t>(luaSize), error))
                return false;
            std::memcpy(candidateMemory->data,
                        data + STATE_HEADER_SIZE, RAM_SIZE);
            return true;
        };

        auto replaceRuntime = [&](std::unique_ptr<Host> host,
                                  std::unique_ptr<Vm> vm) {
            m_impl->vm = std::move(vm);
            m_impl->host = std::move(host);
        };

        std::unique_ptr<Host> candidateHost;
        std::unique_ptr<Vm> candidateVm;
        std::string stateError;
        if (!buildRuntime(candidateHost, candidateVm, true, stateError)) {
            brls::Logger::error(
                "Pico8Core: quick load failed path={} error={}",
                gamePath, stateError);
            Logger_Write("Quick load failed: %s\n", stateError.c_str());

            // FAKE-08 binds its Lua API to process-global VM pointers. Once a
            // candidate VM has been constructed, a failed restore cannot
            // safely resume the old VM. Rebuild the current cart so input and
            // Lua callbacks always point at live objects.
            candidateVm.reset();
            candidateHost.reset();
            std::string recoveryError;
            if (buildRuntime(candidateHost, candidateVm, false,
                             recoveryError)) {
                replaceRuntime(std::move(candidateHost),
                               std::move(candidateVm));
                m_impl->statePersistenceReady = true;
                m_impl->frameAccumulator = 0.f;
                m_impl->audioAccumulator = 0.0;
                m_impl->input = {};
                m_impl->paused = false;
                m_impl->runtimeErrorLogged = false;
                m_impl->error.clear();
                m_impl->convertFrame();
                m_impl->outputAudio.initialize();
                brls::Logger::warning(
                    "Pico8Core: quick-load recovery restarted cart path={}",
                    gamePath);
                Logger_Write("Quick-load recovery restarted cart\n");
            } else {
                brls::Logger::error(
                    "Pico8Core: quick-load recovery failed path={} error={}",
                    gamePath, recoveryError);
                Logger_Write("Quick-load recovery failed: %s\n",
                             recoveryError.c_str());
                if (m_impl->vm)
                    m_impl->vm->CloseCart();
                m_impl->vm.reset();
                m_impl->host.reset();
                m_impl->loaded = false;
                m_impl->initialized = false;
                m_impl->statePersistenceReady = false;
                m_impl->gamePath.clear();
                m_impl->error = recoveryError;
            }
            return false;
        }

        replaceRuntime(std::move(candidateHost), std::move(candidateVm));
        m_impl->statePersistenceReady = true;
        m_impl->frameAccumulator = 0.f;
        m_impl->audioAccumulator = 0.0;
        m_impl->input = {};
        m_impl->paused = false;
        m_impl->runtimeErrorLogged = false;
        m_impl->error.clear();
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
