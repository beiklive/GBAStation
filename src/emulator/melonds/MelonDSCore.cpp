#include "emulator/melonds/MelonDSCore.h"

#include "Args.h"
#include "GPU3D.h"
#include "GPU3D_Compute.h"
#include "GPU3D_OpenGL.h"
#include "GPU3D_Soft.h"
#include "MemConstants.h"
#include "NDS.h"
#include "NDSCart.h"
#include "OpenGLSupport.h"
#include "SPI_Firmware.h"
#include "Savestate.h"
#include "core/GameSignal.hpp"
#include "core/Tools.hpp"
#include "core/cheat/CheatSystem.hpp"
#include "core/game_database.hpp"

#include <borealis.hpp>
#include <borealis/platforms/glfw/glfw_video.hpp>
#ifdef __SWITCH__
#define GLFW_EXPOSE_NATIVE_EGL
#include <GLFW/glfw3native.h>
#include <EGL/egl.h>
#endif
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <ctime>
#include <unordered_map>

namespace beiklive {

std::recursive_mutex& EmulatorGLMutex()
{
    static std::recursive_mutex mutex;
    return mutex;
}

} // namespace beiklive

namespace beiklive::melonds {

struct MelonDSGLContext {
#ifdef __SWITCH__
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    std::vector<EGLContext> previousContextStack;
    std::vector<EGLSurface> previousDrawStack;
    std::vector<EGLSurface> previousReadStack;
#else
    GLFWwindow* window = nullptr;
    std::vector<GLFWwindow*> previousStack;
#endif
    std::vector<std::unique_lock<std::recursive_mutex>> glLockStack;

    bool makeCurrent()
    {
        std::unique_lock<std::recursive_mutex> glLock(beiklive::EmulatorGLMutex());
#ifdef __SWITCH__
        if (display == EGL_NO_DISPLAY || context == EGL_NO_CONTEXT)
            return false;
        const EGLContext previousContext = eglGetCurrentContext();
        const EGLSurface previousDraw = eglGetCurrentSurface(EGL_DRAW);
        const EGLSurface previousRead = eglGetCurrentSurface(EGL_READ);
        if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE)
            return false;
        previousContextStack.push_back(previousContext);
        previousDrawStack.push_back(previousDraw);
        previousReadStack.push_back(previousRead);
        glLockStack.emplace_back(std::move(glLock));
        return true;
#else
        if (window == nullptr)
            return false;
        GLFWwindow* previous = glfwGetCurrentContext();
        glfwMakeContextCurrent(window);
        previousStack.push_back(previous);
        glLockStack.emplace_back(std::move(glLock));
        return true;
#endif
    }

    void restore()
    {
#ifdef __SWITCH__
        EGLContext previousContext = EGL_NO_CONTEXT;
        EGLSurface previousDraw = EGL_NO_SURFACE;
        EGLSurface previousRead = EGL_NO_SURFACE;
        if (!previousContextStack.empty())
        {
            previousContext = previousContextStack.back();
            previousContextStack.pop_back();
        }
        if (!previousDrawStack.empty())
        {
            previousDraw = previousDrawStack.back();
            previousDrawStack.pop_back();
        }
        if (!previousReadStack.empty())
        {
            previousRead = previousReadStack.back();
            previousReadStack.pop_back();
        }
        eglMakeCurrent(display, previousDraw, previousRead, previousContext);
#else
        GLFWwindow* previous = nullptr;
        if (!previousStack.empty())
        {
            previous = previousStack.back();
            previousStack.pop_back();
        }
        glfwMakeContextCurrent(previous);
#endif
        if (!glLockStack.empty())
            glLockStack.pop_back();
    }

    ~MelonDSGLContext()
    {
#ifdef __SWITCH__
        if (display != EGL_NO_DISPLAY)
        {
            if (eglGetCurrentContext() == context)
                eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context != EGL_NO_CONTEXT)
                eglDestroyContext(display, context);
            if (surface != EGL_NO_SURFACE)
                eglDestroySurface(display, surface);
        }
#else
        if (window != nullptr)
            glfwDestroyWindow(window);
#endif
    }
};

struct ScopedMelonDSGLContext {
    explicit ScopedMelonDSGLContext(MelonDSGLContext* context) : ctx(context)
    {
        active = ctx != nullptr && ctx->makeCurrent();
    }

    ~ScopedMelonDSGLContext()
    {
        if (active)
            ctx->restore();
    }

    MelonDSGLContext* ctx = nullptr;
    bool active = false;
};

namespace {

std::string joinPath(const std::string& dir, const std::string& name)
{
    return (std::filesystem::path(dir) / name).string();
}

bool readWholeFile(const std::string& path, std::vector<uint8_t>& out)
{
    return LoadBinaryVector(path, out);
}

bool parseU32Hex(const std::string& text, melonDS::u32& out)
{
    if (text.empty() || text.size() > 8)
        return false;
    try
    {
        size_t consumed = 0;
        unsigned long value = std::stoul(text, &consumed, 16);
        if (consumed != text.size())
            return false;
        out = static_cast<melonDS::u32>(value);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

melonDS::ARCode cheatEntryToArCode(const beiklive::CheatEntry& cheat)
{
    melonDS::ARCode ar {};
    ar.Parent = nullptr;
    ar.Name = cheat.desc;
    ar.Description = "";
    ar.Enabled = cheat.enabled;

    if (!cheat.ndsWords.empty())
    {
        ar.Code.reserve(cheat.ndsWords.size());
        for (const auto word : cheat.ndsWords)
            ar.Code.push_back(static_cast<melonDS::u32>(word));
        return ar;
    }

    auto tokens = beiklive::cheat::extractHexTokens(cheat.code);
    if (tokens.size() % 2 != 0)
        tokens.pop_back();
    ar.Code.reserve(tokens.size());
    for (const auto& token : tokens)
    {
        melonDS::u32 value = 0;
        if (!parseU32Hex(token, value))
        {
            ar.Code.clear();
            break;
        }
        ar.Code.push_back(value);
    }
    return ar;
}

const char* glString(GLenum name)
{
    const auto* value = glGetString(name);
    return value ? reinterpret_cast<const char*>(value) : "(null)";
}

void logGLLoaderState(const char* label)
{
    brls::Logger::info(
        "melonDS: {} GL vendor='{}' renderer='{}' version='{}' glsl='{}'",
        label,
        glString(GL_VENDOR),
        glString(GL_RENDERER),
        glString(GL_VERSION),
        glString(GL_SHADING_LANGUAGE_VERSION));
    brls::Logger::info(
        "melonDS: {} GLAD_GL_VERSION_4_3={} glDispatchCompute={} glBindImageTexture={} glTexStorage2D={}",
        label,
        GLAD_GL_VERSION_4_3,
        glDispatchCompute != nullptr,
        glBindImageTexture != nullptr,
        glTexStorage2D != nullptr);
}

std::unique_ptr<MelonDSGLContext> createSharedGLContext()
{
    auto* platform = brls::Application::getPlatform();
    if (!platform)
        return nullptr;

    auto* video = dynamic_cast<brls::GLFWVideoContext*>(platform->getVideoContext());
    if (!video)
    {
        brls::Logger::warning("melonDS: OpenGL renderer unavailable; current video context is not GLFW/OpenGL");
        return nullptr;
    }

    GLFWwindow* mainWindow = video->getGLFWWindow();
    if (!mainWindow)
        return nullptr;

#ifdef __SWITCH__
    EGLDisplay display = glfwGetEGLDisplay();
    EGLContext mainContext = glfwGetEGLContext(mainWindow);
    EGLSurface mainSurface = glfwGetEGLSurface(mainWindow);
    if (display == EGL_NO_DISPLAY || mainContext == EGL_NO_CONTEXT)
    {
        brls::Logger::warning("melonDS: EGL display/context unavailable for 3D renderer");
        return nullptr;
    }

    EGLint configId = 0;
    if (mainSurface == EGL_NO_SURFACE ||
        eglQuerySurface(display, mainSurface, EGL_CONFIG_ID, &configId) != EGL_TRUE)
    {
        brls::Logger::warning("melonDS: failed to query main EGL config for 3D renderer (err=0x{:x})", eglGetError());
        return nullptr;
    }

    EGLint configAttribs[] = { EGL_CONFIG_ID, configId, EGL_NONE };
    EGLConfig config = nullptr;
    EGLint configCount = 0;
    if (eglChooseConfig(display, configAttribs, &config, 1, &configCount) != EGL_TRUE || configCount <= 0)
    {
        brls::Logger::warning("melonDS: failed to choose main EGL config {} for 3D renderer (err=0x{:x})",
            configId, eglGetError());
        return nullptr;
    }

    eglBindAPI(EGL_OPENGL_API);
    EGLint contextAttribs[] = {
        EGL_CONTEXT_MAJOR_VERSION, 4,
        EGL_CONTEXT_MINOR_VERSION, 3,
        EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,
        EGL_NONE
    };
    EGLContext context = eglCreateContext(display, config, mainContext, contextAttribs);
    if (context == EGL_NO_CONTEXT)
    {
        brls::Logger::warning("melonDS: failed to create shared EGL context for 3D renderer (err=0x{:x})", eglGetError());
        return nullptr;
    }

    EGLSurface surface = EGL_NO_SURFACE;
    const char* extensions = eglQueryString(display, EGL_EXTENSIONS);
    const std::string extensionList = extensions ? extensions : "";
    const bool supportsSurfaceless =
        extensionList.find("EGL_KHR_surfaceless_context") != std::string::npos;
    if (!supportsSurfaceless)
    {
        EGLint surfaceAttribs[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
        surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
        if (surface == EGL_NO_SURFACE)
        {
            brls::Logger::warning("melonDS: failed to create EGL pbuffer surface for 3D renderer (err=0x{:x})", eglGetError());
            eglDestroyContext(display, context);
            return nullptr;
        }
    }

    auto glContext = std::make_unique<MelonDSGLContext>();
    glContext->display = display;
    glContext->surface = surface;
    glContext->context = context;
    if (!glContext->makeCurrent())
    {
        brls::Logger::warning("melonDS: failed to make shared EGL context current for 3D renderer (err=0x{:x})", eglGetError());
        if (surface == EGL_NO_SURFACE)
        {
            EGLint surfaceAttribs[] = { EGL_WIDTH, 16, EGL_HEIGHT, 16, EGL_NONE };
            surface = eglCreatePbufferSurface(display, config, surfaceAttribs);
            if (surface != EGL_NO_SURFACE)
            {
                glContext->surface = surface;
                if (glContext->makeCurrent())
                    brls::Logger::info("melonDS: surfaceless EGL unavailable; using pbuffer surface");
            }
        }
        if (eglGetCurrentContext() != context)
            return nullptr;
    }
    logGLLoaderState("before shared EGL glad load");
    if (!gladLoadGLLoader((GLADloadproc)eglGetProcAddress))
    {
        brls::Logger::warning("melonDS: failed to reload OpenGL functions for shared EGL renderer context (err=0x{:x})", eglGetError());
        logGLLoaderState("after failed shared EGL glad load");
        return nullptr;
    }
    logGLLoaderState("after shared EGL glad load");
    brls::Logger::info("melonDS: 3D EGL context created: {}", glString(GL_VERSION));
    glContext->restore();
    return glContext;
#else
    GLFWwindow* previous = glfwGetCurrentContext();
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* workerWindow = glfwCreateWindow(16, 16, "melonDS 3D", nullptr, mainWindow);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    if (!workerWindow)
    {
        glfwMakeContextCurrent(previous);
        brls::Logger::warning("melonDS: failed to create shared OpenGL context for 3D renderer");
        return nullptr;
    }

    glfwMakeContextCurrent(workerWindow);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        glfwMakeContextCurrent(previous);
        glfwDestroyWindow(workerWindow);
        brls::Logger::warning("melonDS: failed to load OpenGL functions for 3D renderer");
        return nullptr;
    }

    logGLLoaderState("shared GLFW glad load");
    brls::Logger::info("melonDS: 3D GL context created: {}", glString(GL_VERSION));
    glfwMakeContextCurrent(previous);

    auto context = std::make_unique<MelonDSGLContext>();
    context->window = workerWindow;
    return context;
#endif
}

} // namespace

MelonDSCore::MelonDSCore() = default;

MelonDSCore::~MelonDSCore()
{
    Cleanup();
}

bool MelonDSCore::SetupGame(beiklive::GameEntry GameEntry)
{
    m_gameEntry = std::move(GameEntry);
    m_stopRequested.store(false, std::memory_order_release);
    std::filesystem::create_directories(defaultSaveDir());
    std::string saveDir = m_gameEntry.savePath.empty() ? defaultSaveDir() : m_gameEntry.savePath;
    std::filesystem::create_directories(saveDir);
    m_saveFile = joinPath(saveDir, beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".sav");
    m_stateFile = joinPath(saveDir, beiklive::tools::getFileNameWithoutExtension(m_gameEntry.path) + ".state");
    m_platformData.savePath = m_saveFile;

    if (!Initialize())
        return false;
    if (!LoadGame(m_gameEntry.path))
        return false;
    ReloadCheats();

    m_ready.store(true, std::memory_order_release);
    brls::Logger::info("melonDS: ROM loaded: {} ({}x{} @ {:.2f} fps)",
                       m_gameEntry.path, GameWidth(), GameHeight(), Fps());
    return true;
}

void MelonDSCore::Cleanup()
{
    std::lock_guard<std::mutex> lock(m_ndsMutex);
    {
        ScopedMelonDSGLContext glScope(m_glContext.get());
        if (glScope.active && m_usingAcceleratedRenderer)
            melonDS::OpenGL::SaveShaderCache();
        if (glScope.active)
            releaseAcceleratedReadbackPbos();
        if (m_ready.load(std::memory_order_acquire))
            saveSram();
        FlushAsyncBinaryWrites();

        if (m_nds && m_nds->IsRunning())
            m_nds->Stop();
        m_paused.store(false, std::memory_order_release);
        m_nds.reset();
    }
    m_glContext.reset();
    m_romData.clear();
    m_acceleratedReadback.clear();
    m_acceleratedReadbackPbos.fill(0);
    m_acceleratedReadbackPboBytes = 0;
    m_acceleratedReadbackPboIndex = 0;
    m_acceleratedReadbackPboFrames = 0;
    m_audio.Reset();
    m_input.Reset();
    m_video.Reset();
    m_usingAcceleratedRenderer = false;
    m_usingComputeRenderer = false;
    m_acceleratedReadbackFailed = false;
    m_skipAcceleratedReadbackFrames = 0;
    m_slowFrameLogBudget = 40;
    m_ready.store(false, std::memory_order_release);
    m_initialized.store(false, std::memory_order_release);
}

bool MelonDSCore::Initialize()
{
    m_audio.Reset();
    m_input.Reset();
    m_video.Reset();

    melonDS::NDSArgs args;
#ifdef __SWITCH__
    melonDS::JITArgs jitArgs;
    jitArgs.FastMemory = false;
    args.JIT = jitArgs;
#else
    args.JIT = std::nullopt;
#endif
    args.OutputSampleRate = 48000.0;
    args.Renderer3D = createRenderer3D();

    if (!loadBiosFiles(args))
        return false;

    m_nds = std::make_unique<melonDS::NDS>(std::move(args), &m_platformData);
    if (!m_nds)
        return false;

#ifdef __SWITCH__
    if (!m_nds->IsJITEnabled())
    {
        brls::Logger::warning("melonDS: ARM64 JIT requested but not enabled; refusing silent fallback");
        m_nds.reset();
        return false;
    }
    brls::Logger::info("ARM64 JIT enabled");
    brls::Logger::info("melonDS: JIT FastMemory disabled on Switch");
#else
    brls::Logger::info("melonDS: JIT disabled on desktop build; using interpreter");
#endif

    if (m_usingAcceleratedRenderer)
    {
        brls::Logger::info("melonDS: {} 3D renderer enabled, internal resolution x{}",
            m_usingComputeRenderer ? "Compute" : "OpenGL",
            m_internalResolution);
    }
    else
    {
        const auto& soft = static_cast<const melonDS::SoftRenderer&>(m_nds->GPU.GetRenderer3D());
        if (!soft.IsThreaded())
            brls::Logger::warning("melonDS: Threaded Renderer requested but not enabled");
        else
            brls::Logger::info("melonDS: Threaded Renderer enabled");
    }

    m_initialized.store(true, std::memory_order_release);
    return true;
}

bool MelonDSCore::LoadGame(const std::string& path)
{
    if (!m_nds)
        return false;
    std::lock_guard<std::mutex> lock(m_ndsMutex);
    ScopedMelonDSGLContext glScope(m_glContext.get());
    if (path.empty())
    {
        brls::Logger::error("melonDS: ROM path is empty");
        return false;
    }

    std::unique_ptr<uint8_t[]> romData;
    std::uintmax_t fileSize = 0;
    {
        std::error_code ec;
        fileSize = std::filesystem::file_size(path, ec);
        if (ec || fileSize == 0 || fileSize > std::numeric_limits<melonDS::u32>::max())
        {
            brls::Logger::error("melonDS: invalid ROM size: {}", path);
            return false;
        }
    }

    std::ifstream rom(path, std::ios::binary);
    if (!rom)
    {
        brls::Logger::error("melonDS: failed to open ROM: {}", path);
        return false;
    }

    romData = std::make_unique<uint8_t[]>(static_cast<size_t>(fileSize));
    rom.read(reinterpret_cast<char*>(romData.get()), static_cast<std::streamsize>(fileSize));
    if (rom.gcount() != static_cast<std::streamsize>(fileSize))
    {
        brls::Logger::error("melonDS: failed to read ROM: {}", path);
        return false;
    }

    melonDS::NDSCart::NDSCartArgs cartArgs;
    loadBatterySave(cartArgs);

    brls::Logger::debug("melonDS: parsing ROM");
    auto cart = melonDS::NDSCart::ParseROM(std::move(romData),
                                           static_cast<melonDS::u32>(fileSize),
                                           &m_platformData,
                                           std::move(cartArgs));
    if (!cart)
    {
        brls::Logger::error("melonDS: failed to parse NDS ROM: {}", path);
        return false;
    }

    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: SetNDSCart begin");
    m_nds->SetNDSCart(std::move(cart));
    brls::Logger::debug("melonDS: SetNDSCart end");
    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: Reset begin");
    m_nds->Reset();
    brls::Logger::debug("melonDS: Reset end");
    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: SetupDirectBoot begin");
    m_nds->SetupDirectBoot(std::filesystem::path(path).filename().string());
    brls::Logger::debug("melonDS: SetupDirectBoot end");
    syncRtcToHostTime();
    if (m_stopRequested.load(std::memory_order_acquire))
        return false;

    brls::Logger::debug("melonDS: GPU.StartFrame begin");
    m_nds->GPU.StartFrame();
    brls::Logger::debug("melonDS: GPU.StartFrame end");
    brls::Logger::debug("melonDS: Start begin");
    m_nds->Start();
    brls::Logger::debug("melonDS: Start end");
    return true;
}

void MelonDSCore::RunFrame()
{
    if (!m_ready.load(std::memory_order_acquire) || m_paused.load(std::memory_order_acquire) || !m_nds)
        return;

    std::lock_guard<std::mutex> lock(m_ndsMutex);
    ScopedMelonDSGLContext glScope(m_glContext.get());

    m_input.Apply(*m_nds);
    const auto frameStart = std::chrono::steady_clock::now();
    const auto scanlines = m_nds->RunFrame();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - frameStart).count();
    if (elapsedMs > 17 && m_slowFrameLogBudget > 0)
    {
        --m_slowFrameLogBudget;
        brls::Logger::warning(
            "melonDS: slow RunFrame {} ms (scanlines={} renderer={} x{} readback={})",
            elapsedMs,
            scanlines,
            m_usingAcceleratedRenderer ? (m_usingComputeRenderer ? "Compute" : "OpenGL") : "Software",
            m_internalResolution,
            m_acceleratedFrameReadbackEnabled.load(std::memory_order_acquire));
    }

    std::array<int16_t, 4096> temp {};
    int available = m_nds->SPU.GetOutputSize();
    while (available > 0)
    {
        const int toRead = std::min<int>(available, static_cast<int>(temp.size() / 2));
        const int read = m_nds->SPU.ReadOutput(temp.data(), toRead);
        if (read <= 0)
            break;
        m_audio.Push(temp.data(), static_cast<size_t>(read) * 2);
        available = m_nds->SPU.GetOutputSize();
    }

    const bool needAcceleratedReadback =
        m_usingAcceleratedRenderer &&
        m_acceleratedFrameReadbackEnabled.load(std::memory_order_acquire);
    if (needAcceleratedReadback)
    {
        if (!captureAcceleratedFrame())
            m_video.Capture(*m_nds);
    }
    else
    {
        m_video.Capture(*m_nds);
    }
}

void MelonDSCore::Reset()
{
    if (!m_ready.load(std::memory_order_acquire) || !m_nds)
        return;
    std::lock_guard<std::mutex> lock(m_ndsMutex);
    ScopedMelonDSGLContext glScope(m_glContext.get());
    m_nds->Reset();
    m_nds->SetupDirectBoot(std::filesystem::path(m_gameEntry.path).filename().string());
    syncRtcToHostTime();
    m_nds->GPU.StartFrame();
    m_nds->Start();
}

void MelonDSCore::Stop()
{
    std::lock_guard<std::mutex> lock(m_ndsMutex);
    ScopedMelonDSGLContext glScope(m_glContext.get());
    if (m_nds && m_nds->IsRunning())
        m_nds->Stop();
    m_paused.store(false, std::memory_order_release);
}

void MelonDSCore::RequestStop()
{
    m_stopRequested.store(true, std::memory_order_release);
    if (m_nds && m_nds->IsRunning())
        m_nds->Halt();
    m_paused.store(false, std::memory_order_release);
}

void MelonDSCore::Pause(bool paused)
{
    m_paused.store(paused, std::memory_order_release);
}

void MelonDSCore::NotifyConfigUpdated()
{
    if (!m_nds)
        return;

    std::lock_guard<std::mutex> lock(m_ndsMutex);

    int newScale = m_gameEntry.ndsInternalResolution;
    if (beiklive::GameDB && !m_gameEntry.path.empty())
        newScale = beiklive::GameDB->get(m_gameEntry.path, "ndsInternalResolution", 1).get<int>();
#ifdef __SWITCH__
    newScale = 1;
#endif
    newScale = std::clamp(newScale, 1, 4);
    if (newScale == m_internalResolution)
        return;

    brls::Logger::info("melonDS: changing internal resolution x{} -> x{}", m_internalResolution, newScale);
    m_gameEntry.ndsInternalResolution = newScale;

    if (m_glContext)
    {
        m_glContext->makeCurrent();
        m_nds->GPU.Stop();
        glFinish();
        releaseAcceleratedReadbackPbos();
        m_glContext->restore();
    }
    else
    {
        m_nds->GPU.Stop();
    }

    m_acceleratedReadback.clear();
    m_acceleratedReadbackPboBytes = 0;
    m_acceleratedReadbackPboIndex = 0;
    m_acceleratedReadbackPboFrames = 0;
    m_video.Reset();

    auto renderer = createRenderer3D();
    if (m_glContext)
        m_glContext->makeCurrent();
    m_nds->GPU.SetRenderer3D(std::move(renderer));
    m_nds->GPU.StartFrame();
    if (m_glContext)
        m_glContext->restore();
    if (!m_usingAcceleratedRenderer)
        m_glContext.reset();
    m_skipAcceleratedReadbackFrames = m_usingAcceleratedRenderer ? 3 : 0;
    brls::Logger::info("melonDS: internal resolution switch complete; renderer={} x{}",
                       m_usingAcceleratedRenderer ? (m_usingComputeRenderer ? "Compute" : "OpenGL") : "Software",
                       m_internalResolution);
}

bool MelonDSCore::GetVideoTexture(beiklive::EmulatorVideoTexture& out)
{
    out = {};
    if (!m_ready.load(std::memory_order_acquire) || !m_nds || !m_usingAcceleratedRenderer || !m_glContext)
        return false;

    std::unique_lock<std::mutex> lock(m_ndsMutex, std::try_to_lock);
    if (!lock.owns_lock())
        return false;
    ScopedMelonDSGLContext glScope(m_glContext.get());
    if (!glScope.active)
        return false;

    auto& renderer = m_nds->GPU.GetRenderer3D();
    if (!renderer.Accelerated)
        return false;

    const int frontbuf = m_nds->GPU.FrontBuffer;
    GLint previousTexture = 0;
    GLint previousActive = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActive);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);

    renderer.BindOutputTexture(frontbuf);
    GLint texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    if (texture > 0)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
    glActiveTexture(static_cast<GLenum>(previousActive));

    if (texture <= 0)
        return false;

    out.texture = static_cast<uint32_t>(texture);
    out.scale = static_cast<unsigned>(std::max(1, m_internalResolution));
    out.width = 256u * out.scale;
    out.height = (384u + 2u) * out.scale;
    glFlush();
    return true;
}

bool MelonDSCore::Serialize(std::vector<uint8_t>& outBuf) const
{
    if (!m_ready.load(std::memory_order_acquire) || !m_nds)
        return false;

    std::lock_guard<std::mutex> lock(m_ndsMutex);
    ScopedMelonDSGLContext glScope(m_glContext.get());
    if (m_usingAcceleratedRenderer && !glScope.active)
        return false;

    melonDS::Savestate state;
    if (!m_nds->DoSavestate(&state) || state.Error)
        return false;
    state.Finish();
    if (state.Error || state.Length() == 0)
        return false;

    const auto* data = static_cast<const uint8_t*>(state.Buffer());
    outBuf.assign(data, data + state.Length());
    return true;
}

bool MelonDSCore::Unserialize(const std::vector<uint8_t>& buf)
{
    if (!m_ready.load(std::memory_order_acquire) || !m_nds || buf.empty() || buf.size() > std::numeric_limits<melonDS::u32>::max())
        return false;

    std::lock_guard<std::mutex> lock(m_ndsMutex);
    ScopedMelonDSGLContext glScope(m_glContext.get());
    if (m_usingAcceleratedRenderer && !glScope.active)
        return false;

    melonDS::Savestate state(const_cast<uint8_t*>(buf.data()), static_cast<melonDS::u32>(buf.size()), false);
    if (state.Error)
        return false;

    if (m_usingAcceleratedRenderer)
    {
        m_nds->GPU.Stop();
        glFinish();
    }

    m_video.Reset();
    m_audio.Reset();
    m_acceleratedReadback.clear();
    m_acceleratedReadbackPboIndex = 0;
    m_acceleratedReadbackPboFrames = 0;
    m_acceleratedReadbackFailed = false;
    m_skipAcceleratedReadbackFrames = m_usingAcceleratedRenderer ? 3 : 0;

    const bool ok = m_nds->DoSavestate(&state) && !state.Error;
    if (ok)
    {
        syncRtcToHostTime();
        m_nds->GPU.StartFrame();
        if (!m_nds->IsRunning())
            m_nds->Start();
    }
    return ok;
}

bool MelonDSCore::SaveState(const std::string& path)
{
    std::vector<uint8_t> data;
    if (!Serialize(data))
        return false;
    return WriteBinaryFile(path.empty() ? m_stateFile : path, data.data(), data.size());
}

bool MelonDSCore::LoadState(const std::string& path)
{
    std::vector<uint8_t> data;
    if (!readWholeFile(path.empty() ? m_stateFile : path, data))
        return false;
    return Unserialize(data);
}

void MelonDSCore::SetButtonsFromSignal(unsigned player)
{
    if (player == 0)
        m_input.SetButtonsFromMask(GameSignal::instance().getGameButtonMask(player));
}

void MelonDSCore::SetButton(int key, bool pressed)
{
    if (key >= 0)
        m_input.SetButton(static_cast<unsigned>(key), pressed);
}

void MelonDSCore::SetTouch(int x, int y, bool down)
{
    m_input.SetTouch(x, y, down);
}

void MelonDSCore::ApplyCheats(const std::vector<CheatEntry>& cheats)
{
    std::lock_guard<std::mutex> lock(m_ndsMutex);
    m_cheats = cheats;
    UpdateCheats();
}

void MelonDSCore::UpdateCheats()
{
    m_arCheats.clear();
    m_cheatToArIndex.clear();
    m_arCheats.reserve(m_cheats.size());
    m_cheatToArIndex.reserve(m_cheats.size());
    for (const auto& cheat : m_cheats)
    {
        if (cheat.payloadType == beiklive::CheatPayloadType::Category || !cheat.valid)
        {
            m_cheatToArIndex.push_back(-1);
            continue;
        }

        auto ar = cheatEntryToArCode(cheat);
        if (!ar.Code.empty())
        {
            m_cheatToArIndex.push_back(static_cast<int>(m_arCheats.size()));
            m_arCheats.push_back(std::move(ar));
        }
        else
        {
            m_cheatToArIndex.push_back(-1);
        }
    }
    applyArCheatsToEngine();
}

void MelonDSCore::ReloadCheats()
{
    std::lock_guard<std::mutex> lock(m_ndsMutex);

    std::string path = m_gameEntry.cheatPath;
    const std::string fallbackPath = defaultCheatPath();
    if (path.empty() || (!std::filesystem::exists(path) && !fallbackPath.empty()))
        path = fallbackPath;

    const bool datFile = beiklive::cheat::isNdsUsrCheatDat(path, m_gameEntry.platform);
    std::unordered_map<std::string, bool> previousState;
    if (datFile)
    {
        previousState.reserve(m_cheats.size());
        for (const auto& cheat : m_cheats)
            previousState[beiklive::cheat::stateKey(cheat)] = cheat.enabled;
    }

    auto loadedResult = beiklive::cheat::loadCheats({path, m_gameEntry.path, m_gameEntry.platform});
    std::vector<CheatEntry> loaded = std::move(loadedResult.entries);

    if (datFile)
    {
        for (auto& cheat : loaded)
        {
            auto it = previousState.find(beiklive::cheat::stateKey(cheat));
            if (it != previousState.end())
                cheat.enabled = it->second;
        }
    }

    m_gameEntry.cheatPath = path;
    m_cheats = std::move(loaded);
    UpdateCheats();
}

void MelonDSCore::applyArCheatsToEngine()
{
    if (!m_nds)
        return;
    m_nds->AREngine.Cheats = m_arCheats;
}

const void* MelonDSCore::getSramData() const
{
    return m_nds ? m_nds->GetNDSSave() : nullptr;
}

size_t MelonDSCore::getSramSize() const
{
    return m_nds ? static_cast<size_t>(m_nds->GetNDSSaveLength()) : 0;
}

bool MelonDSCore::saveSram()
{
    const void* data = getSramData();
    const size_t size = getSramSize();
    if (!data || size == 0)
        return true;
    const auto* bytes = static_cast<const uint8_t*>(data);
    AsyncWriteBinaryFile(m_saveFile, std::vector<uint8_t>(bytes, bytes + size));
    return true;
}

bool MelonDSCore::loadBiosFiles(melonDS::NDSArgs& args)
{
    m_biosDir = defaultBiosDir();
    m_platformData.firmwarePath = joinPath(m_biosDir, "firmware.bin");

    auto arm9 = std::make_unique<melonDS::ARM9BIOSImage>();
    auto arm7 = std::make_unique<melonDS::ARM7BIOSImage>();
    const std::string bios9 = joinPath(m_biosDir, "bios9.bin");
    const std::string bios7 = joinPath(m_biosDir, "bios7.bin");
    const std::string firmwarePath = joinPath(m_biosDir, "firmware.bin");

    if (!LoadBinaryFile(bios9, arm9->data(), arm9->size()))
    {
        brls::Logger::error("melonDS: missing or invalid BIOS: {}", bios9);
        return false;
    }
    if (!LoadBinaryFile(bios7, arm7->data(), arm7->size()))
    {
        brls::Logger::error("melonDS: missing or invalid BIOS: {}", bios7);
        return false;
    }

    std::vector<uint8_t> firmwareData;
    if (!LoadBinaryVector(firmwarePath, firmwareData))
    {
        brls::Logger::error("melonDS: missing firmware: {}", firmwarePath);
        return false;
    }

    args.ARM9BIOS = std::move(arm9);
    args.ARM7BIOS = std::move(arm7);
    args.Firmware = melonDS::Firmware(firmwareData.data(), static_cast<melonDS::u32>(firmwareData.size()));
    return true;
}

bool MelonDSCore::loadBatterySave(melonDS::NDSCart::NDSCartArgs& args) const
{
    std::vector<uint8_t> data;
    if (!LoadBinaryVector(m_saveFile, data) || data.empty())
        return false;

    args.SRAMLength = static_cast<melonDS::u32>(data.size());
    args.SRAM = std::make_unique<melonDS::u8[]>(args.SRAMLength);
    std::memcpy(args.SRAM.get(), data.data(), data.size());
    return true;
}

void MelonDSCore::syncRtcToHostTime()
{
    if (!m_nds)
        return;

    std::time_t now = std::time(nullptr);
    if (now == static_cast<std::time_t>(-1))
        return;

    std::tm local {};
#if defined(_WIN32)
    if (localtime_s(&local, &now) != 0)
        return;
#else
    if (!localtime_r(&now, &local))
        return;
#endif

    m_nds->RTC.SetDateTime(local.tm_year + 1900,
                           local.tm_mon + 1,
                           local.tm_mday,
                           local.tm_hour,
                           local.tm_min,
                           local.tm_sec);
}

std::unique_ptr<melonDS::Renderer3D> MelonDSCore::createRenderer3D()
{
    m_internalResolution = std::clamp(m_gameEntry.ndsInternalResolution, 1, 4);
#ifdef __SWITCH__
    m_internalResolution = 1;
    m_gameEntry.ndsInternalResolution = 1;
#endif
    m_usingAcceleratedRenderer = false;
    m_usingComputeRenderer = false;

    auto fallbackToSoftware = [this](const char* reason) -> std::unique_ptr<melonDS::Renderer3D> {
        if (reason && reason[0] != '\0')
            brls::Logger::warning("melonDS: {}; falling back to x1 threaded software renderer", reason);
        m_internalResolution = 1;
        m_gameEntry.ndsInternalResolution = 1;
        if (beiklive::GameDB && !m_gameEntry.path.empty())
        {
            beiklive::GameDB->set(m_gameEntry.path, "ndsInternalResolution", nlohmann::json(1));
            beiklive::GameDB->flush();
        }
        return std::make_unique<melonDS::SoftRenderer>(true);
    };

    if (m_internalResolution <= 1)
    {
        brls::Logger::info("melonDS: internal resolution x1; using threaded software 3D renderer");
        return std::make_unique<melonDS::SoftRenderer>(true);
    }

#ifdef __SWITCH__
    return fallbackToSoftware("high internal resolution is disabled on Switch");
#endif

    if (!m_glContext)
        m_glContext = createSharedGLContext();
    ScopedMelonDSGLContext glScope(m_glContext.get());
    if (!glScope.active)
        return fallbackToSoftware("OpenGL/Compute renderer unavailable");
    melonDS::OpenGL::LoadShaderCache();

#ifdef __SWITCH__
    brls::Logger::info("melonDS: trying Compute 3D renderer at x{}", m_internalResolution);
    if (GLAD_GL_VERSION_4_3 && glDispatchCompute && glBindImageTexture && glTexStorage2D)
    {
        if (auto compute = melonDS::ComputeRenderer::New())
        {
            brls::Logger::info("melonDS: Compute 3D renderer created; applying render settings");
            compute->SetRenderSettings(m_internalResolution, false);
            brls::Logger::info("melonDS: Compute high-resolution coordinates disabled on Switch for bandwidth");
            int current = 0;
            int count = 0;
            while (compute->NeedsShaderCompile())
            {
                brls::Logger::debug("melonDS: compiling Compute 3D shader step {}", current);
                compute->ShaderCompileStep(current, count);
            }
            melonDS::OpenGL::SaveShaderCache();

            m_usingAcceleratedRenderer = true;
            m_usingComputeRenderer = true;
            brls::Logger::info("melonDS: Compute 3D renderer enabled, internal resolution x{}", m_internalResolution);
            return compute;
        }
        brls::Logger::warning("melonDS: Compute 3D renderer creation failed");
    }
    else
    {
        brls::Logger::warning(
            "melonDS: Compute 3D renderer unavailable: GLAD_GL_VERSION_4_3={} glDispatchCompute={} glBindImageTexture={} glTexStorage2D={}",
            GLAD_GL_VERSION_4_3,
            glDispatchCompute != nullptr,
            glBindImageTexture != nullptr,
            glTexStorage2D != nullptr);
    }

    return fallbackToSoftware("Compute 3D renderer failed on Switch");
#else
    brls::Logger::info("melonDS: trying OpenGL 3D renderer at x{}", m_internalResolution);
    if (auto glRenderer = melonDS::GLRenderer::New())
    {
        brls::Logger::info("melonDS: OpenGL 3D renderer created; applying render settings");
        glRenderer->SetRenderSettings(false, m_internalResolution);
        m_usingAcceleratedRenderer = true;
        m_usingComputeRenderer = false;
        brls::Logger::info("melonDS: OpenGL 3D renderer enabled, internal resolution x{}", m_internalResolution);
        return glRenderer;
    }

    brls::Logger::warning("melonDS: OpenGL 3D renderer failed; trying Compute 3D renderer");
    if (auto compute = melonDS::ComputeRenderer::New())
    {
        brls::Logger::info("melonDS: Compute 3D renderer created; applying render settings");
        compute->SetRenderSettings(m_internalResolution, true);
        int current = 0;
        int count = 0;
        while (compute->NeedsShaderCompile())
        {
            brls::Logger::debug("melonDS: compiling Compute 3D shader step {}", current);
            compute->ShaderCompileStep(current, count);
        }
        melonDS::OpenGL::SaveShaderCache();

        m_usingAcceleratedRenderer = true;
        m_usingComputeRenderer = true;
        brls::Logger::info("melonDS: Compute 3D renderer enabled, internal resolution x{}", m_internalResolution);
        return compute;
    }

    return fallbackToSoftware("OpenGL/Compute renderer failed");
#endif
}

bool MelonDSCore::ensureAcceleratedReadbackPbos(size_t bytes)
{
    if (bytes == 0)
        return false;

    bool hasPbos = true;
    for (const auto pbo : m_acceleratedReadbackPbos)
        hasPbos = hasPbos && pbo != 0;

    if (hasPbos && m_acceleratedReadbackPboBytes == bytes)
        return true;

    releaseAcceleratedReadbackPbos();

    std::array<GLuint, kAcceleratedReadbackPboCount> pbos {};
    glGenBuffers(static_cast<GLsizei>(pbos.size()), pbos.data());
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        brls::Logger::warning("melonDS: failed to create async readback PBOs (glError=0x{:x})",
                              static_cast<unsigned>(error));
        return false;
    }

    for (size_t i = 0; i < pbos.size(); ++i)
    {
        m_acceleratedReadbackPbos[i] = static_cast<uint32_t>(pbos[i]);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos[i]);
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(bytes), nullptr, GL_STREAM_READ);
    }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);

    error = glGetError();
    if (error != GL_NO_ERROR)
    {
        brls::Logger::warning("melonDS: failed to allocate async readback PBOs ({} bytes, glError=0x{:x})",
                              bytes,
                              static_cast<unsigned>(error));
        releaseAcceleratedReadbackPbos();
        return false;
    }

    m_acceleratedReadbackPboBytes = bytes;
    m_acceleratedReadbackPboIndex = 0;
    m_acceleratedReadbackPboFrames = 0;
    brls::Logger::info("melonDS: async accelerated readback PBOs ready ({} buffers, {} bytes)",
                       kAcceleratedReadbackPboCount,
                       bytes);
    return true;
}

void MelonDSCore::releaseAcceleratedReadbackPbos()
{
    std::array<GLuint, kAcceleratedReadbackPboCount> pbos {};
    GLsizei count = 0;
    for (auto& pbo : m_acceleratedReadbackPbos)
    {
        if (pbo != 0)
            pbos[static_cast<size_t>(count++)] = static_cast<GLuint>(pbo);
        pbo = 0;
    }
    if (count > 0)
        glDeleteBuffers(count, pbos.data());

    m_acceleratedReadbackPboBytes = 0;
    m_acceleratedReadbackPboIndex = 0;
    m_acceleratedReadbackPboFrames = 0;
}

bool MelonDSCore::captureAcceleratedFrame()
{
    if (!m_nds || !m_usingAcceleratedRenderer || !m_glContext)
        return false;
    if (m_skipAcceleratedReadbackFrames > 0)
    {
        --m_skipAcceleratedReadbackFrames;
        return false;
    }

    auto& renderer = m_nds->GPU.GetRenderer3D();
    if (!renderer.Accelerated)
        return false;

    const unsigned scale = static_cast<unsigned>(std::clamp(m_internalResolution, 1, 4));
    const unsigned width = MelonDSVideo::kWidth * scale;
    const unsigned height = (MelonDSVideo::kHeight + 2u) * scale;
    const size_t pixelCount = static_cast<size_t>(width) * height;
    if (m_acceleratedReadback.size() != pixelCount)
        m_acceleratedReadback.resize(pixelCount);

    GLint previousActive = 0;
    GLint previousTexture = 0;
    GLint previousPackAlignment = 0;
    GLint previousPackBuffer = 0;
    GLint previousReadFramebuffer = 0;
    glGetIntegerv(GL_ACTIVE_TEXTURE, &previousActive);
    glActiveTexture(GL_TEXTURE0);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &previousTexture);
    glGetIntegerv(GL_PACK_ALIGNMENT, &previousPackAlignment);
    glGetIntegerv(GL_PIXEL_PACK_BUFFER_BINDING, &previousPackBuffer);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousReadFramebuffer);

    renderer.BindOutputTexture(m_nds->GPU.FrontBuffer);
    GLint texture = 0;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &texture);
    auto restoreReadbackState = [&]() {
        glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<GLuint>(previousReadFramebuffer));
        glBindBuffer(GL_PIXEL_PACK_BUFFER, static_cast<GLuint>(previousPackBuffer));
        glPixelStorei(GL_PACK_ALIGNMENT, previousPackAlignment);
        glBindTexture(GL_TEXTURE_2D, static_cast<GLuint>(previousTexture));
        glActiveTexture(static_cast<GLenum>(previousActive));
    };
    if (texture <= 0)
    {
        restoreReadbackState();
        if (!m_acceleratedReadbackFailed)
        {
            brls::Logger::warning("melonDS: accelerated renderer output texture unavailable; using CPU capture");
            m_acceleratedReadbackFailed = true;
        }
        return false;
    }

    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 4);

#if defined(__SWITCH__) && 0
    const size_t readbackBytes = pixelCount * sizeof(uint32_t);
    if (ensureAcceleratedReadbackPbos(readbackBytes))
    {
        const GLuint writePbo = static_cast<GLuint>(
            m_acceleratedReadbackPbos[m_acceleratedReadbackPboIndex]);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, writePbo);
        glBufferData(GL_PIXEL_PACK_BUFFER, static_cast<GLsizeiptr>(readbackBytes), nullptr, GL_STREAM_READ);

        const auto readbackStart = std::chrono::steady_clock::now();
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        GLenum error = glGetError();
        const auto submitUs = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - readbackStart).count();

        const bool canMapOldFrame = m_acceleratedReadbackPboFrames >= (kAcceleratedReadbackPboCount - 1);
        const size_t readIndex = (m_acceleratedReadbackPboIndex + 1) % kAcceleratedReadbackPboCount;
        m_acceleratedReadbackPboIndex =
            (m_acceleratedReadbackPboIndex + 1) % kAcceleratedReadbackPboCount;
        m_acceleratedReadbackPboFrames =
            std::min(m_acceleratedReadbackPboFrames + 1, kAcceleratedReadbackPboCount);

        if (error == GL_NO_ERROR && canMapOldFrame)
        {
            const GLuint readPbo = static_cast<GLuint>(m_acceleratedReadbackPbos[readIndex]);
            glBindBuffer(GL_PIXEL_PACK_BUFFER, readPbo);
            void* mapped = glMapBufferRange(GL_PIXEL_PACK_BUFFER,
                                            0,
                                            static_cast<GLsizeiptr>(readbackBytes),
                                            GL_MAP_READ_BIT);
            error = glGetError();
            if (mapped && error == GL_NO_ERROR)
            {
                std::memcpy(m_acceleratedReadback.data(), mapped, readbackBytes);
                const GLboolean unmapped = glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
                error = glGetError();
                restoreReadbackState();

                if (unmapped == GL_TRUE && error == GL_NO_ERROR)
                {
                    m_acceleratedReadbackFailed = false;
                    m_video.CaptureAcceleratedRgba(m_acceleratedReadback.data(), width, height, scale);
                    return true;
                }
            }
            else if (mapped)
            {
                glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
            }
        }

        restoreReadbackState();
        if (error == GL_NO_ERROR)
            return true;

        if (!m_acceleratedReadbackFailed)
        {
            brls::Logger::warning("melonDS: async accelerated readback failed (glError=0x{:x}); using sync readback",
                                  static_cast<unsigned>(error));
            m_acceleratedReadbackFailed = true;
        }
        releaseAcceleratedReadbackPbos();
        glActiveTexture(GL_TEXTURE0);
        renderer.BindOutputTexture(m_nds->GPU.FrontBuffer);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
        glPixelStorei(GL_PACK_ALIGNMENT, 4);
    }
#endif

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    const auto readbackStart = std::chrono::steady_clock::now();
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_acceleratedReadback.data());
    const auto readbackMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - readbackStart).count();
    const GLenum error = glGetError();

    restoreReadbackState();

    if (error != GL_NO_ERROR)
    {
        if (!m_acceleratedReadbackFailed)
        {
            brls::Logger::warning("melonDS: accelerated renderer readback failed (glError=0x{:x}); using CPU capture",
                                  static_cast<unsigned>(error));
            m_acceleratedReadbackFailed = true;
        }
        return false;
    }
    if (readbackMs > 50)
        brls::Logger::warning("melonDS: accelerated renderer readback took {} ms at x{}",
                              readbackMs, scale);

    m_acceleratedReadbackFailed = false;
    m_video.CaptureAcceleratedRgba(m_acceleratedReadback.data(), width, height, scale);
    return true;
}

std::string MelonDSCore::defaultSaveDir() const
{
    return beiklive::tools::defaultGameSavePath(
        static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS),
        m_gameEntry.path);
}

std::string MelonDSCore::defaultBiosDir() const
{
#ifdef __SWITCH__
    return "sdmc:/GBAStation/bios/nds";
#else
    return (std::filesystem::path(beiklive::path::biosPath()) / "nds").string();
#endif
}

std::string MelonDSCore::defaultCheatPath() const
{
    return (std::filesystem::path(beiklive::path::cheatPath()) / "usrcheat.dat").string();
}

} // namespace beiklive::melonds
