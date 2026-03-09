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

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <filesystem>

#ifdef __SWITCH__
#  include <switch.h>
#endif

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#  include <mmsystem.h>   // timeBeginPeriod / timeEndPeriod
#endif

// ============================================================
// Game thread constants
// ============================================================
static constexpr double MAX_REASONABLE_FPS = 240.0; ///< Safety cap for core-reported FPS
static constexpr size_t STEREO_CHANNELS    = 2;     ///< Samples per audio frame (L + R)
static constexpr double FPS_UPDATE_INTERVAL = 1.0;  ///< Seconds between FPS counter updates
// Spin-wait guard (subtract from sleep to ensure we don't overshoot)
static constexpr double SPIN_GUARD_SEC = 0.002;     ///< 2 ms spin-wait budget per frame

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
// Default button mapping: borealis ControllerButton → retro joypad ID
// Used as fallback when config does not override individual buttons.
// BUTTON_X is reserved as the in-game exit key (not mapped to game).
// ============================================================
struct DefaultButtonMap {
    brls::ControllerButton brl;
    unsigned               retroId;
};

// Default keyboard mapping: BrlsKeyboardScancode → retro joypad ID
// Keys: Z=B, X=A, A=Select, S=Start, arrows=Dpad, Q=L, W=R
struct DefaultKbMap {
    int      scancode;  // BrlsKeyboardScancode value
    unsigned retroId;
};

static const DefaultButtonMap k_defaultButtonMap[] = {
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

// Default keyboard mapping: Z→B, X→A, A→Y, S→Select, arrows→Dpad, Q→L, W→R, E→L2, R→R2
static const DefaultKbMap k_defaultKbMap[] = {
    { brls::BRLS_KBD_KEY_X,     RETRO_DEVICE_ID_JOYPAD_A      },
    { brls::BRLS_KBD_KEY_Z,     RETRO_DEVICE_ID_JOYPAD_B      },
    { brls::BRLS_KBD_KEY_A,     RETRO_DEVICE_ID_JOYPAD_Y      },
    { brls::BRLS_KBD_KEY_UP,    RETRO_DEVICE_ID_JOYPAD_UP     },
    { brls::BRLS_KBD_KEY_DOWN,  RETRO_DEVICE_ID_JOYPAD_DOWN   },
    { brls::BRLS_KBD_KEY_LEFT,  RETRO_DEVICE_ID_JOYPAD_LEFT   },
    { brls::BRLS_KBD_KEY_RIGHT, RETRO_DEVICE_ID_JOYPAD_RIGHT  },
    { brls::BRLS_KBD_KEY_Q,     RETRO_DEVICE_ID_JOYPAD_L      },
    { brls::BRLS_KBD_KEY_W,     RETRO_DEVICE_ID_JOYPAD_R      },
    { brls::BRLS_KBD_KEY_E,     RETRO_DEVICE_ID_JOYPAD_L2     },
    { brls::BRLS_KBD_KEY_R,     RETRO_DEVICE_ID_JOYPAD_R2     },
    { brls::BRLS_KBD_KEY_ENTER, RETRO_DEVICE_ID_JOYPAD_START  },
    { brls::BRLS_KBD_KEY_S,     RETRO_DEVICE_ID_JOYPAD_SELECT },
};

// retro button name → RETRO_DEVICE_ID_JOYPAD_* (for config parsing)
struct RetroNameMap { const char* name; unsigned id; };
static const RetroNameMap k_retroNames[] = {
    { "a",      RETRO_DEVICE_ID_JOYPAD_A      },
    { "b",      RETRO_DEVICE_ID_JOYPAD_B      },
    { "x",      RETRO_DEVICE_ID_JOYPAD_X      },
    { "y",      RETRO_DEVICE_ID_JOYPAD_Y      },
    { "up",     RETRO_DEVICE_ID_JOYPAD_UP     },
    { "down",   RETRO_DEVICE_ID_JOYPAD_DOWN   },
    { "left",   RETRO_DEVICE_ID_JOYPAD_LEFT   },
    { "right",  RETRO_DEVICE_ID_JOYPAD_RIGHT  },
    { "l",      RETRO_DEVICE_ID_JOYPAD_L      },
    { "r",      RETRO_DEVICE_ID_JOYPAD_R      },
    { "l2",     RETRO_DEVICE_ID_JOYPAD_L2     },
    { "r2",     RETRO_DEVICE_ID_JOYPAD_R2     },
    { "l3",     RETRO_DEVICE_ID_JOYPAD_L3     },
    { "r3",     RETRO_DEVICE_ID_JOYPAD_R3     },
    { "start",  RETRO_DEVICE_ID_JOYPAD_START  },
    { "select", RETRO_DEVICE_ID_JOYPAD_SELECT },
};

static unsigned retroNameToId(const std::string& name)
{
    for (auto& m : k_retroNames)
        if (name == m.name) return m.id;
    return RETRO_DEVICE_ID_JOYPAD_MASK; // sentinel = not found
}

// ============================================================
// Keyboard scancode name ↔ BrlsKeyboardScancode lookup
// Allows config entries like   keyboard.a = X
// instead of numeric values    keyboard.a = 88
// ============================================================
struct KbdKeyName { const char* name; int scancode; };
static const KbdKeyName k_kbdKeyNames[] = {
    // Printable letters
    { "A", brls::BRLS_KBD_KEY_A }, { "B", brls::BRLS_KBD_KEY_B },
    { "C", brls::BRLS_KBD_KEY_C }, { "D", brls::BRLS_KBD_KEY_D },
    { "E", brls::BRLS_KBD_KEY_E }, { "F", brls::BRLS_KBD_KEY_F },
    { "G", brls::BRLS_KBD_KEY_G }, { "H", brls::BRLS_KBD_KEY_H },
    { "I", brls::BRLS_KBD_KEY_I }, { "J", brls::BRLS_KBD_KEY_J },
    { "K", brls::BRLS_KBD_KEY_K }, { "L", brls::BRLS_KBD_KEY_L },
    { "M", brls::BRLS_KBD_KEY_M }, { "N", brls::BRLS_KBD_KEY_N },
    { "O", brls::BRLS_KBD_KEY_O }, { "P", brls::BRLS_KBD_KEY_P },
    { "Q", brls::BRLS_KBD_KEY_Q }, { "R", brls::BRLS_KBD_KEY_R },
    { "S", brls::BRLS_KBD_KEY_S }, { "T", brls::BRLS_KBD_KEY_T },
    { "U", brls::BRLS_KBD_KEY_U }, { "V", brls::BRLS_KBD_KEY_V },
    { "W", brls::BRLS_KBD_KEY_W }, { "X", brls::BRLS_KBD_KEY_X },
    { "Y", brls::BRLS_KBD_KEY_Y }, { "Z", brls::BRLS_KBD_KEY_Z },
    // Digits
    { "0", brls::BRLS_KBD_KEY_0 }, { "1", brls::BRLS_KBD_KEY_1 },
    { "2", brls::BRLS_KBD_KEY_2 }, { "3", brls::BRLS_KBD_KEY_3 },
    { "4", brls::BRLS_KBD_KEY_4 }, { "5", brls::BRLS_KBD_KEY_5 },
    { "6", brls::BRLS_KBD_KEY_6 }, { "7", brls::BRLS_KBD_KEY_7 },
    { "8", brls::BRLS_KBD_KEY_8 }, { "9", brls::BRLS_KBD_KEY_9 },
    // Special / control keys
    { "SPACE",        brls::BRLS_KBD_KEY_SPACE         },
    { "ENTER",        brls::BRLS_KBD_KEY_ENTER         },
    { "BACKSPACE",    brls::BRLS_KBD_KEY_BACKSPACE      },
    { "TAB",          brls::BRLS_KBD_KEY_TAB            },
    { "ESC",          brls::BRLS_KBD_KEY_ESCAPE         },
    { "ESCAPE",       brls::BRLS_KBD_KEY_ESCAPE         },
    { "GRAVE",        brls::BRLS_KBD_KEY_GRAVE_ACCENT   },
    { "GRAVE_ACCENT", brls::BRLS_KBD_KEY_GRAVE_ACCENT   },
    // Arrow keys
    { "UP",    brls::BRLS_KBD_KEY_UP    },
    { "DOWN",  brls::BRLS_KBD_KEY_DOWN  },
    { "LEFT",  brls::BRLS_KBD_KEY_LEFT  },
    { "RIGHT", brls::BRLS_KBD_KEY_RIGHT },
    // Function keys
    { "F1",  brls::BRLS_KBD_KEY_F1  }, { "F2",  brls::BRLS_KBD_KEY_F2  },
    { "F3",  brls::BRLS_KBD_KEY_F3  }, { "F4",  brls::BRLS_KBD_KEY_F4  },
    { "F5",  brls::BRLS_KBD_KEY_F5  }, { "F6",  brls::BRLS_KBD_KEY_F6  },
    { "F7",  brls::BRLS_KBD_KEY_F7  }, { "F8",  brls::BRLS_KBD_KEY_F8  },
    { "F9",  brls::BRLS_KBD_KEY_F9  }, { "F10", brls::BRLS_KBD_KEY_F10 },
    { "F11", brls::BRLS_KBD_KEY_F11 }, { "F12", brls::BRLS_KBD_KEY_F12 },
};

/// Parse a keyboard scancode from a config value: accepts integer strings
/// (e.g. "88") or named strings (e.g. "x", "TAB", "enter") – case-insensitive.
static int parseKbdScancode(const std::string& s)
{
    // Convert input to uppercase for case-insensitive comparison
    std::string upper(s.size(), '\0');
    std::transform(s.begin(), s.end(), upper.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    for (auto& kn : k_kbdKeyNames) {
        if (upper == kn.name) return kn.scancode;
    }
    // Fall back to integer parse
    try { return std::stoi(s); } catch (...) {}
    return -1;
}

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

    // ---- Load display settings from config
    if (gameRunner && gameRunner->settingConfig)
        m_display.load(*gameRunner->settingConfig);

    // Register mgba core variable defaults and wire up SettingManager.
    // SetDefault() only writes the value when the key is not already present,
    // so user-saved values in the config file take precedence.
    if (gameRunner && gameRunner->settingConfig) {
        beiklive::ConfigManager* cfg = gameRunner->settingConfig;
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

        // ---- Fast-forward defaults -----------------------------------
        cfg->SetDefault("fastforward.multiplier", CV(4.0f));
        cfg->SetDefault("fastforward.mute",       CV(std::string("true")));
        cfg->SetDefault("fastforward.mode",       CV(std::string("hold")));

        // ---- Rewind defaults -----------------------------------------
        cfg->SetDefault("rewind.enabled",    CV(std::string("false")));
        cfg->SetDefault("rewind.bufferSize", CV(3600));
        cfg->SetDefault("rewind.step",       CV(2));
        cfg->SetDefault("rewind.mute",       CV(std::string("false")));
        cfg->SetDefault("rewind.mode",       CV(std::string("hold")));

        // ---- FPS display default -------------------------------------
        cfg->SetDefault("display.showFps", CV(std::string("false")));

        // ---- Handle (gamepad) button mapping defaults ----------------
        cfg->SetDefault("handle.a",           CV(static_cast<int>(brls::BUTTON_A)));
        cfg->SetDefault("handle.b",           CV(static_cast<int>(brls::BUTTON_B)));
        cfg->SetDefault("handle.x",           CV(static_cast<int>(brls::BUTTON_X)));
        cfg->SetDefault("handle.y",           CV(static_cast<int>(brls::BUTTON_Y)));
        cfg->SetDefault("handle.up",          CV(static_cast<int>(brls::BUTTON_UP)));
        cfg->SetDefault("handle.down",        CV(static_cast<int>(brls::BUTTON_DOWN)));
        cfg->SetDefault("handle.left",        CV(static_cast<int>(brls::BUTTON_LEFT)));
        cfg->SetDefault("handle.right",       CV(static_cast<int>(brls::BUTTON_RIGHT)));
        cfg->SetDefault("handle.start",       CV(static_cast<int>(brls::BUTTON_START)));
        cfg->SetDefault("handle.select",      CV(static_cast<int>(brls::BUTTON_BACK)));
        cfg->SetDefault("handle.l",           CV(static_cast<int>(brls::BUTTON_LB)));
        cfg->SetDefault("handle.r",           CV(static_cast<int>(brls::BUTTON_RB)));
        cfg->SetDefault("handle.l2",          CV(static_cast<int>(brls::BUTTON_LT)));
        cfg->SetDefault("handle.r2",          CV(static_cast<int>(brls::BUTTON_RT)));
        cfg->SetDefault("handle.fastforward", CV(static_cast<int>(brls::BUTTON_RT)));
        cfg->SetDefault("handle.rewind",      CV(static_cast<int>(brls::BUTTON_LT)));

        // ---- Keyboard mapping defaults (use readable key names) -----------
        cfg->SetDefault("keyboard.a",           CV(std::string("X")));
        cfg->SetDefault("keyboard.b",           CV(std::string("Z")));
        cfg->SetDefault("keyboard.x",           CV(std::string("C")));
        cfg->SetDefault("keyboard.y",           CV(std::string("A")));
        cfg->SetDefault("keyboard.up",          CV(std::string("UP")));
        cfg->SetDefault("keyboard.down",        CV(std::string("DOWN")));
        cfg->SetDefault("keyboard.left",        CV(std::string("LEFT")));
        cfg->SetDefault("keyboard.right",       CV(std::string("RIGHT")));
        cfg->SetDefault("keyboard.start",       CV(std::string("ENTER")));
        cfg->SetDefault("keyboard.select",      CV(std::string("S")));
        cfg->SetDefault("keyboard.l",           CV(std::string("Q")));
        cfg->SetDefault("keyboard.r",           CV(std::string("W")));
        cfg->SetDefault("keyboard.l2",          CV(std::string("E")));
        cfg->SetDefault("keyboard.r2",          CV(std::string("R")));
        cfg->SetDefault("keyboard.fastforward", CV(std::string("TAB")));
        cfg->SetDefault("keyboard.rewind",      CV(std::string("GRAVE")));

        cfg->Save();
        m_core.setConfigManager(cfg);

        // ---- Read runtime config values ----------------------------------
        auto getFloat = [&](const std::string& key, float def) -> float {
            auto v = cfg->Get(key);
            if (v) {
                if (auto f = v->AsFloat()) return *f;
                if (auto i = v->AsInt())   return static_cast<float>(*i);
            }
            return def;
        };
        auto getInt = [&](const std::string& key, int def) -> int {
            auto v = cfg->Get(key);
            if (v) {
                if (auto i = v->AsInt()) return *i;
                if (auto f = v->AsFloat()) return static_cast<int>(*f);
            }
            return def;
        };
        auto getBool = [&](const std::string& key, bool def) -> bool {
            auto v = cfg->Get(key);
            if (v) {
                if (auto s = v->AsString()) return (*s == "true" || *s == "1" || *s == "yes");
                if (auto i = v->AsInt())    return (*i != 0);
            }
            return def;
        };
        auto getString = [&](const std::string& key, const std::string& def) -> std::string {
            auto v = cfg->Get(key);
            if (v) { if (auto s = v->AsString()) return *s; }
            return def;
        };

        m_ffMultiplier    = getFloat("fastforward.multiplier", 4.0f);
        if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 4.0f;
        m_ffMute          = getBool("fastforward.mute", true);
        m_ffToggleMode    = (getString("fastforward.mode", "hold") == "toggle");

        m_rewindEnabled    = getBool("rewind.enabled", false);
        m_rewindBufSize    = static_cast<unsigned>(getInt("rewind.bufferSize", 3600));
        m_rewindStep       = static_cast<unsigned>(getInt("rewind.step", 2));
        if (m_rewindStep == 0) m_rewindStep = 1;
        m_rewindMute       = getBool("rewind.mute", false);
        m_rewindToggleMode = (getString("rewind.mode", "hold") == "toggle");

        m_showFps = getBool("display.showFps", false);

        // Fast-forward and rewind buttons from config
        m_ffButton     = getInt("handle.fastforward", static_cast<int>(brls::BUTTON_RT));
        m_rewindButton = getInt("handle.rewind",      static_cast<int>(brls::BUTTON_LT));
    }

    // ---- Load configurable button maps ----------------------------------
    loadButtonMaps();

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
// loadButtonMaps – build m_buttonMap / m_kbButtonMap from config
// ============================================================

void GameView::loadButtonMaps()
{
    // Helper: get int from config or fall back to default
    auto getCfgInt = [](beiklive::ConfigManager* cfg,
                        const std::string& key, int def) -> int {
        if (!cfg) return def;
        auto v = cfg->Get(key);
        if (v) {
            if (auto i = v->AsInt())   return *i;
            if (auto f = v->AsFloat()) return static_cast<int>(*f);
        }
        return def;
    };

    beiklive::ConfigManager* cfg = gameRunner ? gameRunner->settingConfig : nullptr;

    // Build handle (gamepad) button map from config
    m_buttonMap.clear();
    for (auto& def : k_defaultButtonMap) {
        // Find which retro button this corresponds to
        unsigned retroId = def.retroId;
        // Look up which handle.X config key maps to this retro button
        // We do it by reverse lookup: find the retro name for this ID
        for (auto& rn : k_retroNames) {
            if (rn.id == retroId) {
                std::string cfgKey = std::string("handle.") + rn.name;
                int brlsBtn = getCfgInt(cfg, cfgKey, static_cast<int>(def.brl));
                if (brlsBtn >= 0 && brlsBtn < static_cast<int>(brls::_BUTTON_MAX)) {
                    m_buttonMap.push_back({brlsBtn, retroId});
                }
                break;
            }
        }
    }
    // Add NAV buttons (always mapped to dpad)
    m_buttonMap.push_back({static_cast<int>(brls::BUTTON_NAV_UP),    RETRO_DEVICE_ID_JOYPAD_UP});
    m_buttonMap.push_back({static_cast<int>(brls::BUTTON_NAV_DOWN),  RETRO_DEVICE_ID_JOYPAD_DOWN});
    m_buttonMap.push_back({static_cast<int>(brls::BUTTON_NAV_LEFT),  RETRO_DEVICE_ID_JOYPAD_LEFT});
    m_buttonMap.push_back({static_cast<int>(brls::BUTTON_NAV_RIGHT), RETRO_DEVICE_ID_JOYPAD_RIGHT});

    // Build keyboard button map from config
    m_kbButtonMap.clear();
    for (auto& def : k_defaultKbMap) {
        // Find retro ID for this default keyboard key
        unsigned retroId = def.retroId;
        for (auto& rn : k_retroNames) {
            if (rn.id == retroId) {
                std::string cfgKey = std::string("keyboard.") + rn.name;
                int sc = def.scancode; // fallback default
                if (cfg) {
                    auto v = cfg->Get(cfgKey);
                    if (v) {
                        // Support both numeric (e.g. 88) and named (e.g. "X") values
                        if (auto i = v->AsInt())   sc = *i;
                        else if (auto s = v->AsString()) sc = parseKbdScancode(*s);
                    }
                }
                m_kbButtonMap.push_back({sc, retroId});
                break;
            }
        }
    }
}

// ============================================================
// startGameThread – launches the emulation loop in a new thread
// ============================================================

void GameView::startGameThread()
{
    m_fpsLastTime    = std::chrono::steady_clock::now();
    m_fpsFrameCount  = 0;
    m_currentFps     = 0.0f;

    // Configure audio backpressure threshold: ~4 game-frames worth of audio.
    // This keeps the ring buffer lean so latency cannot build up over time.
    {
        double coreFps = m_core.fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS) coreFps = 60.0;
        size_t maxLatencyFrames = static_cast<size_t>(32768.0 / coreFps * 4.0 + 0.5);
        beiklive::AudioManager::instance().setMaxLatencyFrames(maxLatencyFrames);
    }

    m_running.store(true, std::memory_order_release);
    m_gameThread = std::thread([this]() {
        double fps = m_core.fps();
        if (fps <= 0.0 || fps > MAX_REASONABLE_FPS) fps = 60.0;

        using Clock    = std::chrono::steady_clock;
        using Duration = std::chrono::duration<double>;
        const Duration frameDuration(1.0 / fps);
        // Guard duration: subtract from coarse sleep so spin-wait finishes on time
        const Duration spinGuard(SPIN_GUARD_SEC);

#ifdef _WIN32
        // Request 1 ms Windows timer resolution for accurate sleep_for()
        timeBeginPeriod(1);
#endif

        // Per-thread FPS counter
        Clock::time_point fpsTimerStart = Clock::now();
        unsigned          fpsCounter    = 0;

        while (m_running.load(std::memory_order_acquire)) {
            auto frameStart = Clock::now();

            // Poll controller input and forward to the core
            pollInput();

            bool ff      = m_fastForward.load(std::memory_order_relaxed);
            bool rew     = m_rewinding.load(std::memory_order_relaxed);

            if (rew && m_rewindEnabled) {
                // ---- Rewind: restore from buffer then run to update video ----
                bool didRestore = false;
                {
                    std::lock_guard<std::mutex> lk(m_rewindMutex);
                    for (unsigned step = 0; step < m_rewindStep && !m_rewindBuffer.empty(); ++step) {
                        const auto& state = m_rewindBuffer.front();
                        m_core.unserialize(state.data(), state.size());
                        m_rewindBuffer.pop_front();
                        didRestore = true;
                    }
                }
                // Run core after restoring state to produce a fresh video frame.
                if (didRestore) {
                    m_core.run();
                }
                // Drain audio (mute or pass through based on config)
                {
                    std::vector<int16_t> dummy;
                    bool hasSamples = m_core.drainAudio(dummy) && !dummy.empty();
                    if (!m_rewindMute && hasSamples) {
                        size_t frames = dummy.size() / STEREO_CHANNELS;
                        beiklive::AudioManager::instance().pushSamples(dummy.data(), frames);
                    }
                }
            } else {
                // ---- Normal or fast-forward: run core --------------------
                // Compute how many frames to run this iteration.
                // For fast-forward with multiplier = N, run N frames per normal
                // frame-period so effective speed = N × fps (exact multiplier).
                // For sub-1x, run 1 frame but stretch the sleep duration.
                unsigned runsThisIter = 1u;
                if (ff) {
                    runsThisIter = (m_ffMultiplier >= 1.0f)
                        ? static_cast<unsigned>(std::round(m_ffMultiplier))
                        : 1u;
                }

                // Save state for rewind buffer (before running)
                if (m_rewindEnabled && !ff) {
                    size_t sz = m_core.serializeSize();
                    if (sz > 0) {
                        std::vector<uint8_t> state(sz);
                        if (m_core.serialize(state.data(), sz)) {
                            std::lock_guard<std::mutex> lk(m_rewindMutex);
                            m_rewindBuffer.push_front(std::move(state));
                            while (m_rewindBuffer.size() > m_rewindBufSize)
                                m_rewindBuffer.pop_back();
                        }
                    }
                }

                for (unsigned i = 0; i < runsThisIter; ++i) {
                    m_core.run();
                }

                // Drain audio samples.
                {
                    std::vector<int16_t> samples;
                    bool hasSamples = m_core.drainAudio(samples) && !samples.empty();
                    // Mute conditions:
                    //   - fast-forward with mute enabled
                    //   - no samples available
                    bool mute = (ff && m_ffMute) || !hasSamples;
                    if (!mute) {
                        size_t frames = samples.size() / STEREO_CHANNELS;
                        // pushSamples() blocks if ring is too full, which
                        // naturally synchronises game speed to audio output rate.
                        beiklive::AudioManager::instance().pushSamples(
                            samples.data(), frames);
                    }
                }
            }

            // ---- FPS counter (game-thread side) -------------------------
            ++fpsCounter;
            auto now = Clock::now();
            double elapsed = std::chrono::duration<double>(now - fpsTimerStart).count();
            if (elapsed >= FPS_UPDATE_INTERVAL) {
                float newFps = static_cast<float>(static_cast<double>(fpsCounter) / elapsed);
                {
                    std::lock_guard<std::mutex> lk(m_fpsMutex);
                    m_currentFps    = newFps;
                    m_fpsFrameCount = fpsCounter;
                }
                fpsCounter   = 0;
                fpsTimerStart = now;
            }

            // ---- Frame-rate control -------------------------------------
            // Target: complete one 'frameDuration' period per iteration.
            //
            // fast-forward (multiplier >= 1x):
            //   runsThisIter frames were run; sleep for the same frameDuration.
            //   Effective speed = runsThisIter / frameDuration = multiplier × fps. ✓
            //
            // fast-forward (sub-1x):
            //   Stretch to 1/(fps*multiplier) per frame.
            //
            // normal / rewind:
            //   Sleep for frameDuration.
            //
            // Hybrid sleep: coarse sleep for (target − spinGuard), then
            // spin-wait for the remainder to compensate for OS timer
            // granularity (up to ~15 ms on Windows without timeBeginPeriod).
            {
                Duration targetDur = frameDuration;
                if (ff && m_ffMultiplier < 1.0f) {
                    targetDur = Duration(1.0 / (fps * static_cast<double>(m_ffMultiplier)));
                }

                auto sleepTarget = frameStart + targetDur;
                auto nowPre = Clock::now();
                if (nowPre < sleepTarget) {
                    // Coarse sleep (leave spinGuard for spin-wait)
                    auto coarseDur = (sleepTarget - nowPre) - spinGuard;
                    if (coarseDur.count() > 0)
                        std::this_thread::sleep_for(coarseDur);
                    // Spin-wait for precise deadline
                    while (Clock::now() < sleepTarget) { /* busy spin */ }
                }
            }
        }

#ifdef _WIN32
        timeEndPeriod(1);
#endif
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
    // Signal AudioManager to stop and wake any pushSamples() waiter BEFORE
    // joining the game thread.  Without this, the game thread may be blocked
    // inside pushSamples() waiting for ring space, causing a deadlock when
    // stopGameThread() tries to join it.
    beiklive::AudioManager::instance().deinit();

    // Now safe to stop and join the emulation thread
    stopGameThread();

    // Clear rewind buffer
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        m_rewindBuffer.clear();
    }

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
// Supports:
//   - Configurable handle (gamepad) button mapping
//   - Raw keyboard mapping via platform input manager
//   - Toggle / hold mode for fast-forward and rewind
//   - Auto-detection of keyboard vs gamepad input
// ============================================================

void GameView::pollInput()
{
    const brls::ControllerState& state = brls::Application::getControllerState();

    // ---- Detect keyboard vs gamepad input type -----------------------
    // Borealis InputType is GAMEPAD for both keyboard and gamepad on PC;
    // we distinguish by checking if the platform input manager reports any
    // active keyboard key.
    bool keyboardActive = false;
#ifndef __SWITCH__
    auto* platform = brls::Application::getPlatform();
    if (platform) {
        auto* im = platform->getInputManager();
        if (im) {
            // Sample a few common movement/action keys
            keyboardActive =
                im->getKeyboardKeyState(brls::BRLS_KBD_KEY_UP)    ||
                im->getKeyboardKeyState(brls::BRLS_KBD_KEY_DOWN)  ||
                im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT)  ||
                im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT) ||
                im->getKeyboardKeyState(brls::BRLS_KBD_KEY_X)     ||
                im->getKeyboardKeyState(brls::BRLS_KBD_KEY_Z)     ||
                im->getKeyboardKeyState(brls::BRLS_KBD_KEY_ENTER);
        }
    }
    if (keyboardActive) m_useKeyboard = true;
    else {
        // Check if any gamepad button is pressed (switch back to gamepad mode)
        for (int i = 0; i < static_cast<int>(brls::_BUTTON_MAX); ++i) {
            if (state.buttons[i]) { m_useKeyboard = false; break; }
        }
    }
#endif

    // ---- Fast-forward: hold or toggle mode ---------------------------
    bool ffKey = false;
    if (m_ffButton >= 0 && m_ffButton < static_cast<int>(brls::_BUTTON_MAX))
        ffKey = state.buttons[m_ffButton];
    // Also check keyboard fast-forward key
#ifndef __SWITCH__
    if (!ffKey) {
        auto* platform2 = brls::Application::getPlatform();
        if (platform2) {
            auto* im2 = platform2->getInputManager();
            if (im2) {
                // keyboard.fastforward (default: TAB) – supports named or numeric values
                int kbFfKey = static_cast<int>(brls::BRLS_KBD_KEY_TAB);
                if (gameRunner && gameRunner->settingConfig) {
                    auto v = gameRunner->settingConfig->Get("keyboard.fastforward");
                    if (v) {
                        if (auto i = v->AsInt())   kbFfKey = *i;
                        else if (auto s = v->AsString()) kbFfKey = parseKbdScancode(*s);
                    }
                }
                ffKey = im2->getKeyboardKeyState(static_cast<brls::BrlsKeyboardScancode>(kbFfKey));
            }
        }
    }
#endif

    if (m_ffToggleMode) {
        // Toggle: fire on rising edge
        if (ffKey && !m_ffPrevKey)
            m_ffToggled = !m_ffToggled;
        m_ffPrevKey = ffKey;
        m_fastForward.store(m_ffToggled, std::memory_order_relaxed);
    } else {
        // Hold mode
        m_fastForward.store(ffKey, std::memory_order_relaxed);
    }

    // ---- Rewind: hold or toggle mode ---------------------------------
    bool rewKey = false;
    if (m_rewindEnabled) {
        if (m_rewindButton >= 0 && m_rewindButton < static_cast<int>(brls::_BUTTON_MAX))
            rewKey = state.buttons[m_rewindButton];
        // Also check keyboard rewind key
#ifndef __SWITCH__
        if (!rewKey) {
            auto* platform3 = brls::Application::getPlatform();
            if (platform3) {
                auto* im3 = platform3->getInputManager();
                if (im3) {
                    // keyboard.rewind (default: GRAVE) – supports named or numeric values
                    int kbRewKey = static_cast<int>(brls::BRLS_KBD_KEY_GRAVE_ACCENT);
                    if (gameRunner && gameRunner->settingConfig) {
                        auto v = gameRunner->settingConfig->Get("keyboard.rewind");
                        if (v) {
                            if (auto i = v->AsInt())   kbRewKey = *i;
                            else if (auto s = v->AsString()) kbRewKey = parseKbdScancode(*s);
                        }
                    }
                    rewKey = im3->getKeyboardKeyState(static_cast<brls::BrlsKeyboardScancode>(kbRewKey));
                }
            }
        }
#endif
        if (m_rewindToggleMode) {
            if (rewKey && !m_rewindPrevKey)
                m_rewindToggled = !m_rewindToggled;
            m_rewindPrevKey = rewKey;
            m_rewinding.store(m_rewindToggled, std::memory_order_relaxed);
        } else {
            m_rewinding.store(rewKey, std::memory_order_relaxed);
        }
    }

    // ---- Game buttons -----------------------------------------------
    if (m_useKeyboard && !m_kbButtonMap.empty()) {
        // Keyboard mode: use raw keyboard key states
        auto* platform4 = brls::Application::getPlatform();
        auto* im4 = platform4 ? platform4->getInputManager() : nullptr;
        for (const auto& mapping : m_kbButtonMap) {
            bool pressed = false;
            if (im4 && mapping.scancode >= 0)
                pressed = im4->getKeyboardKeyState(
                    static_cast<brls::BrlsKeyboardScancode>(mapping.scancode));
            m_core.setButtonState(mapping.retroId, pressed);
        }
    } else {
        // Gamepad mode: use ControllerState buttons
        for (const auto& mapping : m_buttonMap) {
            bool pressed = false;
            if (mapping.brlsBtn >= 0 && mapping.brlsBtn < static_cast<int>(brls::_BUTTON_MAX))
                pressed = state.buttons[mapping.brlsBtn];
            m_core.setButtonState(mapping.retroId, pressed);
        }
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

    // ---- FPS overlay -------------------------------------------------
    if (m_showFps) {
        float currentFps = 0.0f;
        {
            std::lock_guard<std::mutex> lk(m_fpsMutex);
            currentFps = m_currentFps;
        }
        char fpsBuf[32];
        snprintf(fpsBuf, sizeof(fpsBuf), "FPS: %.1f", currentFps);

        // Semi-transparent background
        float fw = 90.0f, fh = 22.0f;
        float fx = x + 4.0f, fy = y + 4.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, fx, fy, fw, fh, 4.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 160));
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(0, 255, 80, 230));
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgText(vg, fx + 6.0f, fy + fh * 0.5f, fpsBuf, nullptr);
    }

    // ---- Rewind status overlay ---------------------------------------
    if (m_rewindEnabled && m_rewinding.load(std::memory_order_relaxed)) {
        const char* rewText = "<<< REWIND";
        float rw = 110.0f, rh = 22.0f;
        float rx = x + width * 0.5f - rw * 0.5f;
        float ry = y + 4.0f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rx, ry, rw, rh, 4.0f);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 160));
        nvgFill(vg);

        nvgFontSize(vg, 14.0f);
        nvgFontFace(vg, "regular");
        nvgFillColor(vg, nvgRGBA(255, 200, 0, 230));
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgText(vg, rx + rw * 0.5f, ry + rh * 0.5f, rewText, nullptr);
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

