#pragma once

#include <borealis.hpp>
#include <glad/glad.h>

#include <atomic>
#include <thread>

#include "common.hpp"
#include "Game/LibretroLoader.hpp"
#include "Game/DisplayConfig.hpp"

class GameView : public brls::Box
{
  public:
    /// Construct with path to the ROM file that will be loaded automatically.
    explicit GameView(std::string romPath);
    GameView();
    ~GameView() override;

    void draw(NVGcontext* vg, float x, float y, float width, float height,
              brls::Style style, brls::FrameContext* ctx) override;
    void onFocusGained() override;
    void onFocusLost() override;
    void onLayout() override;

  private:
    std::string  m_romPath;
    bool         m_initialized  = false;
    bool         m_coreFailed   = false;

    // ---- libretro core ----------------------------------------------
    beiklive::LibretroLoader m_core;

    // ---- OpenGL texture for game frame ------------------------------
    GLuint   m_texture   = 0;
    unsigned m_texWidth  = 0;
    unsigned m_texHeight = 0;

    // ---- NanoVG image handle wrapping the GL texture ----------------
    int  m_nvgImage = -1;

    // ---- Display configuration (scaling / filtering) ----------------
    beiklive::DisplayConfig  m_display;
    beiklive::FilterMode     m_activeFilter = beiklive::FilterMode::Nearest;

    // ---- Independent game thread ------------------------------------
    std::thread       m_gameThread;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_fastForward{false};

    /// Multiplier applied when ZR is held: runsThisIter = FAST_FORWARD_MULT
    static constexpr unsigned FAST_FORWARD_MULT = 4u;

    // ---- Helper methods ---------------------------------------------
    void initialize();
    void cleanup();

    /// Start the independent emulation thread (called from initialize()).
    void startGameThread();

    /// Stop and join the emulation thread (called from cleanup()).
    void stopGameThread();

    /// Resolve path to mgba_libretro shared library (.dll/.so/.dylib).
    static std::string resolveCoreLibPath();

    /// Upload the latest video frame from the libretro core to m_texture.
    void uploadFrame(NVGcontext* vg, const beiklive::LibretroLoader::VideoFrame& frame);

    /// Map borealis controller state to libretro button states.
    void pollInput();
};