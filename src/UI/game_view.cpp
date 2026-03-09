#include "UI/game_view.hpp"
#include "Audio/AudioManager.hpp"

#include <borealis.hpp>
#include <borealis/core/application.hpp>

// ---- NanoVG GL backend include (must match the active borealis backend) ----
#ifdef BOREALIS_USE_OPENGL
#  ifdef USE_GLES3
#    define NANOVG_GLES3
#  elif defined(USE_GLES2)
#    define NANOVG_GLES2
#  elif defined(USE_GL2)
#    define NANOVG_GL2
#  else
#    define NANOVG_GL3
#  endif
#  include <nanovg/nanovg_gl.h>
#endif

#include <chrono>
#include <cstring>
#include <vector>
#include <filesystem>

#ifdef __SWITCH__
#  include <switch.h>
#endif

// ============================================================
// Game thread constants
// ============================================================
static constexpr double MAX_REASONABLE_FPS = 240.0; ///< Safety cap for core-reported FPS
static constexpr size_t STEREO_CHANNELS    = 2;     ///< Samples per audio frame (L + R)

// ============================================================
// NanoVG helper: create NVG image from an existing GL texture.
// Selects the correct function based on the active GL backend.
// ============================================================
static int nvgImageFromGLTexture(NVGcontext* vg, GLuint tex,
                                  int w, int h,
                                  beiklive::FilterMode filter = beiklive::FilterMode::Nearest)
{
    // NVG_IMAGE_NODELETE ensures NanoVG won't delete our texture.
    // NVG_IMAGE_NEAREST selects nearest-neighbor sampling in NanoVG.
    int flags = NVG_IMAGE_NODELETE;
    if (filter == beiklive::FilterMode::Nearest)
        flags |= NVG_IMAGE_NEAREST;
#if defined(USE_GLES2)
    return nvglCreateImageFromHandleGLES2(vg, tex, w, h, flags);
#elif defined(USE_GLES3)
    return nvglCreateImageFromHandleGLES3(vg, tex, w, h, flags);
#elif defined(USE_GL2)
    return nvglCreateImageFromHandleGL2(vg, tex, w, h, flags);
#else
    return nvglCreateImageFromHandleGL3(vg, tex, w, h, flags);
#endif
}

// ============================================================
// Button mapping: borealis ControllerButton → retro joypad ID
// BUTTON_X is reserved as the in-game exit key (not mapped to game).
// ============================================================
struct ButtonMap {
    brls::ControllerButton brl;
    unsigned               retroId;
};

static const ButtonMap k_buttonMap[] = {
    { brls::BUTTON_A,          RETRO_DEVICE_ID_JOYPAD_A      },
    { brls::BUTTON_B,          RETRO_DEVICE_ID_JOYPAD_B      },
    { brls::BUTTON_Y,          RETRO_DEVICE_ID_JOYPAD_Y      },
    { brls::BUTTON_UP,         RETRO_DEVICE_ID_JOYPAD_UP     },
    { brls::BUTTON_DOWN,       RETRO_DEVICE_ID_JOYPAD_DOWN   },
    { brls::BUTTON_LEFT,       RETRO_DEVICE_ID_JOYPAD_LEFT   },
    { brls::BUTTON_RIGHT,      RETRO_DEVICE_ID_JOYPAD_RIGHT  },
    { brls::BUTTON_LB,         RETRO_DEVICE_ID_JOYPAD_L      },
    { brls::BUTTON_RB,         RETRO_DEVICE_ID_JOYPAD_R      },
    { brls::BUTTON_LT,         RETRO_DEVICE_ID_JOYPAD_L2     },
    { brls::BUTTON_RT,         RETRO_DEVICE_ID_JOYPAD_R2     },
    { brls::BUTTON_START,      RETRO_DEVICE_ID_JOYPAD_START  },
    { brls::BUTTON_BACK,       RETRO_DEVICE_ID_JOYPAD_SELECT },
    { brls::BUTTON_NAV_UP,     RETRO_DEVICE_ID_JOYPAD_UP     },
    { brls::BUTTON_NAV_DOWN,   RETRO_DEVICE_ID_JOYPAD_DOWN   },
    { brls::BUTTON_NAV_LEFT,   RETRO_DEVICE_ID_JOYPAD_LEFT   },
    { brls::BUTTON_NAV_RIGHT,  RETRO_DEVICE_ID_JOYPAD_RIGHT  },
};

// ============================================================
// Resolve the mgba_libretro shared library path
// ============================================================
std::string GameView::resolveCoreLibPath()
{
#if defined(__SWITCH__)
    // On Switch, mgba_libretro.a is statically linked into the binary.
    // LibretroLoader::load() ignores the path in this case.
    return "";
#elif defined(_WIN32)
    return std::string(BK_GAME_CORE_DIR) + std::string("mgba_libretro.dll");
#elif defined(__APPLE__)
    return std::string(BK_GAME_CORE_DIR) + std::string("mgba_libretro.dylib");
#else
    return std::string(BK_GAME_CORE_DIR) + std::string("mgba_libretro.so");
#endif
}

// ============================================================
// Constructors / Destructor
// ============================================================

GameView::GameView(std::string romPath) : GameView()
{
    m_romPath = std::move(romPath);
}

GameView::GameView()
{
    setFocusable(true);
    setHideHighlight(true);

    // Swallow all game buttons so Borealis doesn't process them as navigation.
    // BUTTON_A: swallow during gameplay; pop activity if core failed to load.
    registerAction("", brls::BUTTON_A, [this](brls::View*) {
        if (m_coreFailed) {
            brls::Application::popActivity();
        }
        return true;
    }, true);
    beiklive::swallow(this, brls::BUTTON_B);
    beiklive::swallow(this, brls::BUTTON_Y);
    beiklive::swallow(this, brls::BUTTON_UP);
    beiklive::swallow(this, brls::BUTTON_DOWN);
    beiklive::swallow(this, brls::BUTTON_LEFT);
    beiklive::swallow(this, brls::BUTTON_RIGHT);
    beiklive::swallow(this, brls::BUTTON_NAV_UP);
    beiklive::swallow(this, brls::BUTTON_NAV_DOWN);
    beiklive::swallow(this, brls::BUTTON_NAV_LEFT);
    beiklive::swallow(this, brls::BUTTON_NAV_RIGHT);
    beiklive::swallow(this, brls::BUTTON_LB);
    beiklive::swallow(this, brls::BUTTON_RB);
    beiklive::swallow(this, brls::BUTTON_LT);
    beiklive::swallow(this, brls::BUTTON_RT);
    beiklive::swallow(this, brls::BUTTON_START);
    beiklive::swallow(this, brls::BUTTON_BACK);

    // BUTTON_X: exit game and return to main menu.
    registerAction("beiklive/hints/exit_game"_i18n, brls::BUTTON_X, [this](brls::View*) {
        bklog::info("GameView: exit requested via BUTTON_X");
        stopGameThread();
        brls::Application::popActivity();
        return true;
    }, false, false, brls::SOUND_CLICK);
}

GameView::~GameView()
{
    cleanup();
}

// ============================================================
// initialize – deferred to first draw so GL context is ready
// ============================================================

void GameView::initialize()
{
    if (m_initialized || m_coreFailed) return;

    setHideHighlight(true);
    setHideClickAnimation(true);
    setHideHighlightBackground(true);
    setHideHighlightBorder(true);

    // Load display settings from config
    if (gameRunner && gameRunner->settingConfig)
        m_display.load(*gameRunner->settingConfig);

    // Register mgba core variable defaults and wire up SettingManager.
    // SetDefault() only writes the value when the key is not already present,
    // so user-saved values in the config file take precedence.
    if (gameRunner && gameRunner->settingConfig) {
        ConfigManager* cfg = gameRunner->settingConfig;
        using CV = beiklive::ConfigValue;
        cfg->SetDefault("core.mgba_gb_model",                  CV(std::string("Autodetect")));
        cfg->SetDefault("core.mgba_use_bios",                   CV(std::string("ON")));
        cfg->SetDefault("core.mgba_skip_bios",                  CV(std::string("OFF")));
        cfg->SetDefault("core.mgba_gb_colors",                  CV(std::string("Grayscale")));
        cfg->SetDefault("core.mgba_gb_colors_preset",           CV(std::string("0")));
        cfg->SetDefault("core.mgba_sgb_borders",                CV(std::string("ON")));
        cfg->SetDefault("core.mgba_audio_low_pass_filter",      CV(std::string("disabled")));
        cfg->SetDefault("core.mgba_audio_low_pass_range",       CV(std::string("60")));
        cfg->SetDefault("core.mgba_allow_opposing_directions",  CV(std::string("no")));
        cfg->SetDefault("core.mgba_solar_sensor_level",         CV(std::string("0")));
        cfg->SetDefault("core.mgba_force_gbp",                  CV(std::string("OFF")));
        cfg->SetDefault("core.mgba_idle_optimization",          CV(std::string("Remove Known")));
        cfg->SetDefault("core.mgba_frameskip",                  CV(std::string("0")));
        cfg->Save();
        m_core.setConfigManager(cfg);
    }

    // ---- Load libretro core -------------------------------------------
    std::string corePath = resolveCoreLibPath();
    bklog::info("Loading libretro core: {}", corePath);

    if (!m_core.load(corePath)) {
        bklog::error("Failed to load libretro core from: {}", corePath);
        m_coreFailed = true;
        return;
    }

    if (!m_core.initCore()) {
        bklog::error("retro_init() failed");
        m_core.unload();
        m_coreFailed = true;
        return;
    }

    // ---- Load ROM -------------------------------------------------------
    if (!m_romPath.empty()) {
        if (!std::filesystem::exists(m_romPath)) {
            bklog::error("ROM not found: {}", m_romPath);
            m_core.deinitCore();
            m_core.unload();
            m_coreFailed = true;
            return;
        }

        if (!m_core.loadGame(m_romPath)) {
            bklog::error("retro_load_game() failed for: {}", m_romPath);
            m_core.deinitCore();
            m_core.unload();
            m_coreFailed = true;
            return;
        }
        bklog::info("ROM loaded: {} ({}x{} @ {:.2f} fps)",
                    m_romPath,
                    m_core.gameWidth(), m_core.gameHeight(),
                    m_core.fps());
    }

    // ---- Create GL texture for video output ----------------------------
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    // Apply filter mode from display config (Nearest or Linear)
    m_activeFilter     = m_display.filterMode;
    GLenum glFilter    = (m_activeFilter == beiklive::FilterMode::Nearest)
                         ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Pre-allocate with game dimensions using GL_RGBA (universally supported)
    unsigned gw = m_core.gameWidth()  > 0 ? m_core.gameWidth()  : 256;
    unsigned gh = m_core.gameHeight() > 0 ? m_core.gameHeight() : 224;
    std::vector<uint32_t> blank(gw * gh, 0xFF000000u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(gw),
                 static_cast<GLsizei>(gh), 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 blank.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    m_texWidth  = gw;
    m_texHeight = gh;

    // ---- Start audio manager ------------------------------------------
    if (!beiklive::AudioManager::instance().init(32768, 2)) {
        bklog::warning("AudioManager init failed; game will run without audio");
    }

    m_initialized = true;
    bklog::info("GameView initialized successfully");

    // ---- Start independent emulation thread ---------------------------
    startGameThread();
}

// ============================================================
// startGameThread – launches the emulation loop in a new thread
// ============================================================

void GameView::startGameThread()
{
    m_running.store(true, std::memory_order_release);
    m_gameThread = std::thread([this]() {
        double fps = m_core.fps();
        if (fps <= 0.0 || fps > MAX_REASONABLE_FPS) fps = 60.0;

        using Clock    = std::chrono::steady_clock;
        using Duration = std::chrono::duration<double>;
        const Duration frameDuration(1.0 / fps);

        while (m_running.load(std::memory_order_acquire)) {
            auto frameStart = Clock::now();
            bool ff = m_fastForward.load(std::memory_order_relaxed);

            // Poll controller input and forward to the core
            pollInput();

            // Run one frame normally, or FAST_FORWARD_MULT frames when ZR held
            unsigned runsThisIter = ff ? FAST_FORWARD_MULT : 1u;
            for (unsigned i = 0; i < runsThisIter; ++i) {
                m_core.run();
            }

            // Drain audio samples.
            // During fast-forward the buffer would overflow, so samples are
            // discarded to keep the ring buffer empty.
            {
                std::vector<int16_t> samples;
                if (m_core.drainAudio(samples) && !samples.empty() && !ff) {
                    size_t frames = samples.size() / STEREO_CHANNELS;
                    beiklive::AudioManager::instance().pushSamples(
                        samples.data(), frames);
                }
            }

            // Frame-rate control: sleep only during normal-speed playback
            if (!ff) {
                auto elapsed = Clock::now() - frameStart;
                if (elapsed < frameDuration) {
                    std::this_thread::sleep_for(frameDuration - elapsed);
                }
            }
        }
    });
}

// ============================================================
// stopGameThread – signals and joins the emulation thread
// ============================================================

void GameView::stopGameThread()
{
    m_running.store(false, std::memory_order_release);
    if (m_gameThread.joinable()) {
        m_gameThread.join();
    }
}

// ============================================================
// cleanup
// ============================================================

void GameView::cleanup()
{
    // Stop the emulation thread first (before destroying core/GL resources)
    stopGameThread();

    if (m_nvgImage >= 0) {
        // NVG_IMAGE_NODELETE is set, so NanoVG won't delete the GL texture
        // itself; we only need to free the NVG handle.
        // We can't call nvgDeleteImage here because we may not have a valid
        // NVGcontext.  Setting to -1 is sufficient – the texture lifetime is
        // managed by m_texture below.
        m_nvgImage = -1;
    }

    if (m_texture) {
        glDeleteTextures(1, &m_texture);
        m_texture = 0;
    }

    if (m_core.isLoaded()) {
        m_core.unloadGame();
        m_core.deinitCore();
        m_core.unload();
    }

    beiklive::AudioManager::instance().deinit();

    m_initialized = false;
}

// ============================================================
// uploadFrame – copy RGBA8888 pixels from libretro into the GL texture
// ============================================================

void GameView::uploadFrame(NVGcontext* vg,
                            const beiklive::LibretroLoader::VideoFrame& frame)
{
    if (!frame.width || !frame.height || frame.pixels.empty()) return;

    glBindTexture(GL_TEXTURE_2D, m_texture);

    bool sizeChanged = (frame.width  != m_texWidth ||
                        frame.height != m_texHeight);
    if (sizeChanged) {
        // Resize the texture (pixels are RGBA8888)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     static_cast<GLsizei>(frame.width),
                     static_cast<GLsizei>(frame.height),
                     0, GL_RGBA, GL_UNSIGNED_BYTE,
                     frame.pixels.data());
        m_texWidth  = frame.width;
        m_texHeight = frame.height;

        // Recreate the NanoVG image handle with new dimensions
        if (m_nvgImage >= 0) {
            nvgDeleteImage(vg, m_nvgImage);
        }
        m_nvgImage = nvgImageFromGLTexture(vg, m_texture,
                                           static_cast<int>(m_texWidth),
                                           static_cast<int>(m_texHeight),
                                           m_activeFilter);
    } else {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        static_cast<GLsizei>(frame.width),
                        static_cast<GLsizei>(frame.height),
                        GL_RGBA, GL_UNSIGNED_BYTE,
                        frame.pixels.data());
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

// ============================================================
// pollInput – map borealis controller state to libretro buttons
// ============================================================

void GameView::pollInput()
{
    const brls::ControllerState& state = brls::Application::getControllerState();

    // ZR (BUTTON_RT) → fast-forward: run multiple frames per iteration
    if (static_cast<int>(brls::BUTTON_RT) < static_cast<int>(brls::_BUTTON_MAX))
        m_fastForward.store(state.buttons[brls::BUTTON_RT], std::memory_order_relaxed);

    for (const auto& mapping : k_buttonMap) {
        bool pressed = false;
        if (mapping.brl < brls::_BUTTON_MAX)
            pressed = state.buttons[mapping.brl];
        m_core.setButtonState(mapping.retroId, pressed);
    }
}

// ============================================================
// draw – per-frame render entry point (GL upload + NVG render only)
// ============================================================

void GameView::draw(NVGcontext* vg, float x, float y, float width, float height,
                    brls::Style style, brls::FrameContext* ctx)
{
    // Deferred initialization (GL context guaranteed to exist at this point)
    if (!m_initialized && !m_coreFailed) {
        initialize();
    }

    if (!m_initialized) {
        // Draw error/placeholder rectangle
        nvgBeginPath(vg);
        nvgRect(vg, x, y, width, height);
        nvgFillColor(vg, nvgRGBA(30, 30, 30, 255));
        nvgFill(vg);

        nvgFontSize(vg, 20.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(200, 60, 60, 255));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, x + width * 0.5f, y + height * 0.5f - 15.0f,
                "Failed to load emulator core", nullptr);

        // Hint: press A to dismiss the screen
        nvgFontSize(vg, 16.0f);
        nvgFillColor(vg, nvgRGBA(200, 200, 200, 200));
        nvgText(vg, x + width * 0.5f, y + height * 0.5f + 15.0f,
                "beiklive/hints/close_on_a"_i18n.c_str(), nullptr);
        return;
    }

    // ---- Upload the latest video frame to the GL texture -------------
    // (The emulation runs in a separate thread; we only consume the result.)
    {
        auto frame = m_core.getVideoFrame();
        if (!frame.pixels.empty()) {
            uploadFrame(vg, frame);
        }
    }

    // ---- Handle run-time filter mode changes -------------------------
    // If the display config filter changes (e.g. user updates settings),
    // update the GL texture parameters and recreate the NVG image.
    if (m_display.filterMode != m_activeFilter && m_texture) {
        m_activeFilter  = m_display.filterMode;
        GLenum glFilter = (m_activeFilter == beiklive::FilterMode::Nearest)
                          ? GL_NEAREST : GL_LINEAR;
        glBindTexture(GL_TEXTURE_2D, m_texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, glFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, glFilter);
        glBindTexture(GL_TEXTURE_2D, 0);
        if (m_nvgImage >= 0) {
            nvgDeleteImage(vg, m_nvgImage);
            m_nvgImage = -1;
        }
    }

    // ---- Create NVG image handle on first valid frame ----------------
    if (m_nvgImage < 0 && m_texWidth > 0 && m_texHeight > 0) {
        m_nvgImage = nvgImageFromGLTexture(vg, m_texture,
                                           static_cast<int>(m_texWidth),
                                           static_cast<int>(m_texHeight),
                                           m_activeFilter);
    }

    // ---- Render the game texture using NanoVG ------------------------
    if (m_nvgImage >= 0) {
        beiklive::DisplayRect rect = m_display.computeRect(x, y, width, height,
                                                            m_texWidth, m_texHeight);

        NVGpaint imgPaint = nvgImagePattern(vg,
                                            rect.x, rect.y,
                                            rect.w, rect.h,
                                            0.0f,
                                            m_nvgImage,
                                            1.0f);
        nvgBeginPath(vg);
        nvgRect(vg, rect.x, rect.y, rect.w, rect.h);
        nvgFillPaint(vg, imgPaint);
        nvgFill(vg);
    }

    this->invalidate();
}

// ============================================================
// Focus / Layout callbacks
// ============================================================

void GameView::onFocusGained()
{
    Box::onFocusGained();
}

void GameView::onFocusLost()
{
    Box::onFocusLost();
}

void GameView::onLayout()
{
    Box::onLayout();
}

