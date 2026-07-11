#include "MgbaGameView.hpp"
#include "GameMenuView.hpp"
#include "RewindSelectorView.hpp"
#include "emulator/IEmulatorAudioOutput.hpp"
#include "game/audio/AudioManager.hpp"
#include "game/control/InputMappingDefaults.hpp"
#include "ui/utils/BKAudioPlayer.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "ui/utils/UiHelper.hpp"
#include "core/Tools.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <cmath>
#include <mutex>
#include <utility>

// stb_image_write 用于保存存档缩略图（PNG 格式）
#include "third_party/borealis/library/lib/extern/glfw/deps/stb_image_write.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <mmsystem.h> // timeBeginPeriod / timeEndPeriod
#endif

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace
{
    constexpr int kNdsTargetLatencyFloorMs = 120;
    constexpr int kNdsMaxLatencyFloorMs = 240;
    constexpr float kNdsMaxAudioSyncStrength = 0.008f;
    constexpr double kNdsMinAudioCorrection = 0.99;
    constexpr double kNdsMaxAudioCorrection = 1.01;

    std::string normalizeNdsScreenRotation(std::string value)
    {
        value.erase(std::remove_if(value.begin(), value.end(),
            [](unsigned char c) { return std::isspace(c) != 0; }), value.end());
        std::transform(value.begin(), value.end(), value.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (value == "0" || value == "0deg" || value == "0°" || value == "vertical")
            return "0";
        if (value == "90" || value == "90deg" || value == "90°" || value == "horizontal")
            return "90";
        if (value == "180" || value == "180deg" || value == "180°" || value == "vertical_reverse")
            return "180";
        if (value == "270" || value == "270deg" || value == "270°" || value == "horizontal_reverse")
            return "270";
        return "0";
    }

    bool isNdsPlatform(int platform)
    {
        return platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
    }

    bool isMgbaNativePlatform(int platform)
    {
        using beiklive::enums::EmuPlatform;
        return platform == static_cast<int>(EmuPlatform::EmuGBA) ||
               platform == static_cast<int>(EmuPlatform::EmuGBC) ||
               platform == static_cast<int>(EmuPlatform::EmuGB);
    }

    bool shouldSetupCoreOnGameThread(int platform)
    {
        return isNdsPlatform(platform) || isMgbaNativePlatform(platform);
    }

    std::pair<unsigned, unsigned> rewindThumbSizeForFrame(unsigned srcW, unsigned srcH)
    {
        if (srcW == 0 || srcH == 0)
            return {beiklive::RewindFrame::DEFAULT_THUMB_W, beiklive::RewindFrame::DEFAULT_THUMB_H};

        const double scale = std::min(1.0,
            std::min(static_cast<double>(beiklive::RewindFrame::MAX_THUMB_W) / srcW,
                     static_cast<double>(beiklive::RewindFrame::MAX_THUMB_H) / srcH));
        return {std::max(1u, static_cast<unsigned>(std::lround(srcW * scale))),
                std::max(1u, static_cast<unsigned>(std::lround(srcH * scale)))};
    }

    beiklive::LibretroLoader::VideoFrame rewindThumbToPreviewFrame(const std::vector<uint16_t>& thumb,
                                                                   unsigned width,
                                                                   unsigned height)
    {
        beiklive::LibretroLoader::VideoFrame frame;
        if (width == 0 || height == 0 ||
            thumb.size() < static_cast<std::size_t>(width) * height)
            return frame;

        frame.width = width;
        frame.height = height;
        frame.pixels.resize(static_cast<std::size_t>(frame.width) * frame.height);

        for (std::size_t i = 0; i < frame.pixels.size(); ++i)
        {
            const uint16_t px = thumb[i];
            const uint8_t r5 = static_cast<uint8_t>((px >> 11) & 0x1F);
            const uint8_t g6 = static_cast<uint8_t>((px >> 5) & 0x3F);
            const uint8_t b5 = static_cast<uint8_t>(px & 0x1F);
            const uint32_t r = static_cast<uint32_t>((r5 << 3) | (r5 >> 2));
            const uint32_t g = static_cast<uint32_t>((g6 << 2) | (g6 >> 4));
            const uint32_t b = static_cast<uint32_t>((b5 << 3) | (b5 >> 2));
            frame.pixels[i] = r | (g << 8) | (b << 16) | (0xFFu << 24);
        }

        return frame;
    }

    beiklive::IEmulatorAudioOutput* coreAudioOutput(beiklive::IEmulatorCore* core)
    {
        auto* output = dynamic_cast<beiklive::IEmulatorAudioOutput*>(core);
        return output && output->HandlesAudioOutput() ? output : nullptr;
    }

    bool isNdsRightStickMapping(const char* suffix)
    {
        return std::strncmp(suffix, "rstick_", 7) == 0;
    }
}

namespace beiklive
{
    void MgbaGameView::setGameMenuView(GameMenuView* menuView)
    {
        m_gameMenuView = menuView;
        if (!m_gameMenuView ||
            m_gameEntry.platform != static_cast<int>(beiklive::enums::EmuPlatform::EmuGB))
            return;

        auto *header = new brls::Header();
        header->setTitle("GB 配色");
        m_gameMenuView->addCoreDisplaySettingView(header);

        const auto& colors = beiklive::GetGbColorPresets();
        const std::string current = GET_SETTING_KEY_STR("core.mgba_gb_colors", "Grayscale");
        int selected = 0;
        for (int i = 0; i < static_cast<int>(colors.size()); ++i)
        {
            if (colors[i] == current)
            {
                selected = i;
                break;
            }
        }

        auto *colorCell = new beiklive::SelectorButton();
        colorCell->setText("GB 配色");
        colorCell->setOptions(colors, selected);
        colorCell->setOnSelect([this](int index) {
            const auto& presets = beiklive::GetGbColorPresets();
            if (index < 0 || index >= static_cast<int>(presets.size()))
                return;
            SET_SETTING_KEY_STR("core.mgba_gb_colors", presets[index]);
            _onConfigUpdated();
        });
        m_gameMenuView->addCoreDisplaySettingView(colorCell);
        m_gameMenuView->addCoreDisplaySettingView(
            beiklive::ui::makeHint("为 GB 单色游戏着色，不影响 GBA 游戏"));
    }


    MgbaGameView::MgbaGameView(beiklive::GameEntry gameData) : m_gameEntry(std::move(gameData))
    {
        m_gameEntry.core = beiklive::NormalizeCoreId(m_gameEntry.platform, m_gameEntry.core);
        brls::Logger::debug("[MgbaGameView] constructor: platform={}, path={}",
            m_gameEntry.platform, m_gameEntry.path);
        const bool isNds = m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
        _brls_inputLocked = false;
        GameInputManager::instance().sayHello();
        GameInputManager::instance().setNesDualPlayerEnabled(false);
        HIDE_BRLS_HIGHLIGHT(this);

        // 从 GameEntry 加载画面模式（默认 Fit）
        m_screenMode = static_cast<beiklive::ScreenMode>(m_gameEntry.displayMode);

        // 从配置读取倒带相关设置
        m_rewindSaveInterval = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SAVE_INTERVAL, 1);
        // 确保间隔值在合法范围内（与设置页面的选项匹配：1/2/4/8/16/60/120）
        if (m_rewindSaveInterval < 1)   m_rewindSaveInterval = 1;
        if (m_rewindSaveInterval > 120) m_rewindSaveInterval = 120;
        m_rewindBufferSize = static_cast<unsigned>(
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_BUFFER_SIZE, 600));
        if (m_rewindBufferSize < 10)   m_rewindBufferSize = 10;
        if (m_rewindBufferSize > 1800) m_rewindBufferSize = 1800;
        m_rewindEnabled = GET_SETTING_KEY_INT("rewind.enabled", 0) != 0;
        m_rewindShowUI = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SHOW_UI, 0) != 0;
        if (isNds) {
            m_rewindEnabled = false;
            m_rewindShowUI = false;
            GameSignal::instance().requestRewind(false);
            brls::Logger::info("MgbaGameView: rewind disabled for NDS core");
        }

        // 缓存缩略图压缩模式（避免每帧读取配置）
        m_cachedThumbCompression = GET_SETTING_KEY_INT(
            beiklive::SettingKey::KEY_REWIND_THUMB_COMPRESSION, 0);
        if (isNds) {
            m_ndsLayout = m_gameEntry.ndsScreenLayout.empty() ? "priority_top" : m_gameEntry.ndsScreenLayout;
            if (m_ndsLayout == "separate")
                m_ndsLayout = "custom";
            if (m_ndsLayout != "vertical" && m_ndsLayout != "horizontal" &&
                m_ndsLayout != "priority_top" &&
                m_ndsLayout != "custom" && m_ndsLayout != "hybrid" &&
                m_ndsLayout != "top" && m_ndsLayout != "bottom")
                m_ndsLayout = "priority_top";
            m_gameEntry.ndsScreenLayout = m_ndsLayout;

            m_ndsScreenOrientation = normalizeNdsScreenRotation(m_gameEntry.ndsScreenOrientation);
            m_gameEntry.ndsScreenOrientation = m_ndsScreenOrientation;
            m_ndsIntegerScale = m_gameEntry.ndsIntegerScale;
#ifdef __SWITCH__
            m_gameEntry.ndsInternalResolution = 1;
#endif
        }

        // 读取连发速率（Hz）
        {
            float turboHz = GET_SETTING_KEY_FLOAT("turbo.rate", 10.0f);
            if (turboHz < 1.0f) turboHz = 1.0f;
            if (turboHz > 30.0f) turboHz = 30.0f;
            m_turboToggleInterval = std::max(1, static_cast<int>(30.0f / turboHz));
        }

#ifdef __SWITCH__
        _registerAppletHook();
#endif
        _registerGameInput();
        _registerTouchInput();
        _registerGameRuntime();
    }

    MgbaGameView::~MgbaGameView()
    {
        brls::Logger::debug("[MgbaGameView] destructor: platform={}, path={}",
            m_gameEntry.platform, m_gameEntry.path);
#ifdef __SWITCH__
        _unregisterAppletHook();
#endif
        _stopGameThread();

        if (m_core) {
            delete m_core;
            m_core = nullptr;
        }
        m_coreAudioOutput = nullptr;

        if (m_overlayImage) {
            m_overlayImage->clear();
            delete m_overlayImage;
            m_overlayImage = nullptr;
        }

        GameInputManager::instance().clearEmuFunctionKeys();
        GameInputManager::instance().setNesDualPlayerEnabled(false);
        GameInputManager::instance().dropInput();
    }

    void MgbaGameView::prepareExitCleanup()
    {
        brls::Logger::debug("[MgbaGameView] prepareExitCleanup begin");
#ifdef __SWITCH__
        _unregisterAppletHook();
#endif
        GameInputManager::instance().setInputEnabled(false);
        GameInputManager::instance().clearEmuFunctionKeys();
        GameInputManager::instance().setNesDualPlayerEnabled(false);
        GameInputManager::instance().dropInput();

        _stopGameThread();

        if (m_core) {
            delete m_core;
            m_core = nullptr;
        }
        m_coreAudioOutput = nullptr;

        m_rendererReady = false;
        m_ndsSplitShaderRenderer = false;
        m_renderer.deinit();
        m_ndsTopRenderer.deinit();
        m_ndsBottomRenderer.deinit();

        if (m_overlayImage) {
            m_overlayImage->clear();
            delete m_overlayImage;
            m_overlayImage = nullptr;
        }

        {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            m_pendingFrame = {};
            m_lastRawFrame = {};
            m_ndsTopUploadFrame = {};
            m_ndsBottomUploadFrame = {};
            m_frameReady = false;
            m_hasLastRawFrame = false;
        }
        m_firstFrameUploaded.store(false, std::memory_order_release);

        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            std::deque<RewindFrame>().swap(m_rewindBuffer);
            m_rewindFrameCounter = 0;
        }

        m_audioDrainBuf.clear();
        m_audioDrainBuf.shrink_to_fit();

        brls::Logger::debug("[MgbaGameView] prepareExitCleanup end");
    }

    void MgbaGameView::onFocusGained()
    {
        Box::onFocusGained();
        brls::Logger::debug("MgbaGameView gained focus");

        // 获得焦点时恢复游戏运行
        GameSignal::instance().requestPause(false);
        GameInputManager::instance().setInputEnabled(true);

        if (!_brls_inputLocked)
        {
            _brls_inputLocked = true;
            brls::Application::blockInputs(true);
        }
    }

    void MgbaGameView::onFocusLost()
    {
        Box::onFocusLost();
        brls::Logger::debug("MgbaGameView lost focus");

        // 失去焦点时暂停游戏
        GameSignal::instance().requestPause(true);
        GameInputManager::instance().setInputEnabled(false);
        GameInputManager::instance().dropInput();

        if (_brls_inputLocked)
        {
            _brls_inputLocked = false;
            brls::Application::unblockInputs();
        }
    }

#ifdef __SWITCH__
    void MgbaGameView::_appletHook(AppletHookType hook, void* param)
    {
        auto* self = static_cast<MgbaGameView*>(param);
        if (!self || hook != AppletHookType_OnFocusState)
            return;

        self->m_switchFocusState.store(static_cast<int>(appletGetFocusState()),
                                       std::memory_order_release);
    }

    void MgbaGameView::_registerAppletHook()
    {
        if (m_switchAppletHooked)
            return;

        m_switchFocusState.store(static_cast<int>(appletGetFocusState()),
                                 std::memory_order_release);
        appletHook(&m_appletHookCookie, &MgbaGameView::_appletHook, this);
        m_switchAppletHooked = true;
    }

    void MgbaGameView::_unregisterAppletHook()
    {
        if (!m_switchAppletHooked)
            return;

        appletUnhook(&m_appletHookCookie);
        m_switchAppletHooked = false;
    }

    void MgbaGameView::_updateSwitchFocusState()
    {
        const auto focusState = static_cast<AppletFocusState>(
            m_switchFocusState.load(std::memory_order_acquire));

        if (focusState == AppletFocusState_InFocus) {
            if (m_switchBackgroundPaused) {
                brls::Logger::info("MgbaGameView: Switch applet resumed, resetting playtime clock");
                m_playStartTime = std::chrono::steady_clock::now();
                m_switchBackgroundPaused = false;
                if (!m_switchPauseBeforeBackground)
                    GameSignal::instance().requestPause(false);
            }
            return;
        }

        if (!m_switchBackgroundPaused) {
            brls::Logger::info("MgbaGameView: Switch applet backgrounded, pausing playtime");
            m_switchPauseBeforeBackground = GameSignal::instance().isPaused();
            _savePlayTimeCheckpoint();
            m_playStartTime = {};
            _flushAudioForTransition();
            m_switchBackgroundPaused = true;
        }

        GameSignal::instance().requestPause(true);
    }
#endif

    void MgbaGameView::draw(NVGcontext *vg, float x, float y, float width, float height,
                        brls::Style style, brls::FrameContext *ctx)
    {
        Box::draw(vg, x, y, width, height, style, ctx);

        GameInputManager::instance().setActivePlatform(m_gameEntry.platform);
        GameInputManager::instance().handleInput(); // 每帧获取输入
        const bool isNdsView = isNdsPlatform(m_gameEntry.platform);
        if (isNdsView)
        {
            _pollNdsTouchInput();
            _updateNdsVirtualPointer();
        }

        // // 消费退出信号：异步弹出活动，本帧仍继续渲染避免闪烁
        // if (GameSignal::instance().consumeExit()) {
        //     brls::sync([this](){ 
        //         beiklive::popActivity(); 
        //     });
        //     // 不提前返回：继续渲染最后一帧，防止画面出现黑帧闪烁
        // }

        // 消费打开菜单信号：异步触发菜单入场，本帧仍继续渲染避免闪烁
        if (GameSignal::instance().consumeOpenMenu()) {
            GameSignal::instance().requestPause(true);
            if (m_gameMenuView) {
                brls::sync([this](){
                    // 菜单从底部滑入，入场动画（120ms）
                    AnimationHelper::slideInFromBottom(m_gameMenuView, 60.f, 120, [this]() {
                        m_gameMenuView->onShow();
                    });
                });
            }
            // 不提前返回：继续渲染当前游戏帧，防止菜单弹出时出现黑帧闪烁
        }

        // 消费打开倒带UI信号：暂停游戏并弹出可视化倒带选择界面
        if (GameSignal::instance().consumeOpenRewindUI()) {
            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
                GameSignal::instance().requestRewind(false);
            } else if (m_rewindSelectorView) {
                GameSignal::instance().requestPause(true);
                // 取出缩略图快照（游戏已暂停，可安全读取缓冲区）
                auto thumbs = snapshotRewindThumbs();
                brls::sync([this, thumbs = std::move(thumbs)]() mutable {
                    m_rewindSelectorView->openWithFrames(std::move(thumbs));
                    AnimationHelper::slideInFromBottom(m_rewindSelectorView, 80.f, 220);
                    brls::Application::giveFocus(m_rewindSelectorView);
                });
            }
        }

        // 初始化渲染器（首帧时，GL 上下文已就绪）
        if (!m_rendererReady && m_core && m_core->IsReady()) {
            unsigned gw = m_core->GameWidth()  > 0 ? m_core->GameWidth()  : beiklive::GetGamePixelWidth(m_gameEntry.platform);
            unsigned gh = m_core->GameHeight() > 0 ? m_core->GameHeight() : beiklive::GetGamePixelHeight(m_gameEntry.platform);
            // 若游戏条目启用了着色器且路径有效，则传入着色器路径初始化渲染链
            std::string shaderPath;
            bool allowShader = m_gameEntry.shaderEnabled && !m_gameEntry.shaderPath.empty();
#ifdef __SWITCH__
            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS))
                allowShader = false;
#else
            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
                m_gameEntry.ndsInternalResolution > 1)
                allowShader = false;
#endif
            if (allowShader) {
                shaderPath = m_gameEntry.shaderPath;
                // _onShaderToggle(true); // 同步着色器开关状态，确保启用着色器
            }
            if (_initGameRenderers(gw, gh, shaderPath)) {
                m_rendererReady = true;
                brls::Logger::info("MgbaGameView: 渲染器初始化完成 ({}x{} shader={})",
                                   gw, gh,
                                   shaderPath.empty() ? "无" : shaderPath);
                // 初始化 FPS 计时
                m_fpsLastTime = std::chrono::steady_clock::now();
            }
        }

        // 上传待渲染帧（游戏线程已写入 m_pendingFrame）
        if (m_rendererReady) {
            std::lock_guard<std::recursive_mutex> glLock(beiklive::EmulatorGLMutex());
            _uploadPendingFrame();
        }

        // 根据画面模式计算绘制矩形，将游戏帧绘制到视图区域
        if (m_rendererReady) {
            float windowScale = brls::Application::windowScale;
            int   windowW     = brls::Application::windowWidth;
            int   windowH     = brls::Application::windowHeight;

            unsigned gw = m_renderer.texWidth()  > 0 ? m_renderer.texWidth()  : beiklive::GetGamePixelWidth(m_gameEntry.platform);
            unsigned gh = m_renderer.texHeight() > 0 ? m_renderer.texHeight() : beiklive::GetGamePixelHeight(m_gameEntry.platform);
            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
                if (m_ndsLayout == "custom" || m_ndsLayout == "hybrid") {
                    gw = 1280;
                    gh = 720;
                } else if (m_ndsLayout == "priority_top") {
                    gw = 1024;
                    gh = 768;
                } else if (m_ndsLayout == "horizontal") {
                    gw = 512;
                    gh = 192;
                } else if (m_ndsLayout == "top" || m_ndsLayout == "bottom") {
                    gw = 256;
                    gh = 192;
                } else {
                    gw = 256;
                    gh = 384;
                }
                if (m_ndsLayout != "custom" && m_ndsLayout != "hybrid" && m_ndsLayout != "priority_top" &&
                    (m_ndsScreenOrientation == "90" || m_ndsScreenOrientation == "270"))
                    std::swap(gw, gh);
            }

            int intScale = static_cast<int>(m_gameEntry.integerAspectRatio);
            beiklive::ScreenMode drawMode = m_screenMode;
            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
                (m_ndsLayout == "custom" || m_ndsLayout == "hybrid" || m_ndsLayout == "priority_top"))
                drawMode = beiklive::ScreenMode::Fit;
            else if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
                     m_ndsIntegerScale) {
                drawMode = beiklive::ScreenMode::IntegerScale;
                intScale = 0;
            }
            beiklive::DisplayRect rect;
            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
                m_ndsLayout == "priority_top") {
                rect = {x, y, width, height};
            } else {
                rect = beiklive::computeDisplayRect(
                    drawMode, x, y, width, height, gw, gh,
                    m_gameEntry.customScale, m_gameEntry.customOffsetX, m_gameEntry.customOffsetY,
                    intScale);
            }
            m_gameDrawRect = rect;
            _updateNdsTouchRect(rect);

            {
                std::lock_guard<std::recursive_mutex> glLock(beiklive::EmulatorGLMutex());
                _clearGameViewBackground(x, y, width, height, windowScale, windowW, windowH);
                if (m_ndsSplitShaderRenderer) {
                    const beiklive::DisplayRect layoutRect = _unrotateNdsRect(rect, rect);
                    const auto uv = _ndsOrientationUv();
                    for (const auto& screenRect : _computeNdsScreenDrawRects(layoutRect)) {
                        auto& renderer = screenRect.topScreen ? m_ndsTopRenderer : m_ndsBottomRenderer;
                        const auto rotatedRect = _rotateNdsScreenRect(screenRect.rect, layoutRect, rect);
                        renderer.drawToScreen(rotatedRect.x, rotatedRect.y,
                                              rotatedRect.w, rotatedRect.h,
                                              windowScale, windowW, windowH, uv);
                    }
                } else if (_drawNdsAcceleratedTexture(rect, windowScale, windowW, windowH)) {
                    // NDS OpenGL/Compute 3D renderer output was copied into the normal renderer texture.
                } else if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
                           m_ndsLayout == "priority_top") {
                    const unsigned texW = m_renderer.texWidth();
                    const unsigned texH = m_renderer.texHeight();
                    const GLuint tex = m_renderer.texId();
                    if (tex != 0 && texW >= 256 && texH >= 384) {
                        const float scale = static_cast<float>(texW) / 256.0f;
                        const float topV0 = 0.0f;
                        const float topV1 = (192.0f * scale) / static_cast<float>(texH);
                        const float bottomV0 = topV1;
                        const float bottomV1 = 1.0f;
                        const beiklive::DisplayRect layoutRect = _unrotateNdsRect(rect, rect);
                        const auto orientationUv = _ndsOrientationUv();
                        for (const auto& screenRect : _computeNdsScreenDrawRects(layoutRect)) {
                            const float v0 = screenRect.topScreen ? topV0 : bottomV0;
                            const float v1 = screenRect.topScreen ? topV1 : bottomV1;
                            std::array<float, 8> uv{};
                            for (size_t i = 0; i < 4; ++i) {
                                const float u = orientationUv[i * 2];
                                const float v = orientationUv[i * 2 + 1];
                                uv[i * 2] = u;
                                uv[i * 2 + 1] = v0 + (v1 - v0) * v;
                            }
                            const auto rotatedRect = _rotateNdsScreenRect(screenRect.rect, layoutRect, rect);
                            m_renderer.drawExternalTexture(tex, texW, texH,
                                                           rotatedRect.x, rotatedRect.y,
                                                           rotatedRect.w, rotatedRect.h,
                                                           windowScale, windowW, windowH,
                                                           uv, false);
                        }
                    }
                } else if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS)) {
                    const std::array<float, 8> bakedCanvasUv = {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
                    m_renderer.drawToScreen(rect.x, rect.y, rect.w, rect.h,
                                            windowScale, windowW, windowH,
                                            (m_ndsLayout == "custom" || m_ndsLayout == "hybrid" || m_ndsLayout == "priority_top")
                                                ? bakedCanvasUv
                                                : _ndsOrientationUv());
                } else {
                    m_renderer.drawToScreen(rect.x, rect.y, rect.w, rect.h, windowScale, windowW, windowH);
                }
            }

            // 绘制遮罩层（覆盖整个 MgbaGameView 区域）
            if (m_gameEntry.overlayEnabled && !m_gameEntry.overlayPath.empty())
            {
                if (!m_overlayImage)
                {
                    m_overlayImage = new brls::Image();
                    m_overlayImage->setScalingType(brls::ImageScalingType::STRETCH);
                    m_overlayImage->setWidth(1280.f);
                    m_overlayImage->setHeight(720.f);
                }
                if (!m_overlayImage->getTexture() && std::filesystem::exists(m_gameEntry.overlayPath))
                    m_overlayImage->setImageFromFile(m_gameEntry.overlayPath);
                if (m_overlayImage->getTexture())
                    m_overlayImage->draw(vg, x, y, width, height, style, ctx);
            }
            else if (m_overlayImage && m_overlayImage->getTexture())
            {
                m_overlayImage->clear();
            }
        }

        // 绘制状态覆盖层
        if (isNdsView)
            _drawNdsVirtualPointer(vg);
        _drawOverlays(vg, x, y, width, height);
    }

    // ============================================================
    // _drawOverlays – 绘制 FPS/快进/倒带/暂停/静音覆盖层
    // ============================================================
    void MgbaGameView::_drawOverlays(NVGcontext* vg, float x, float y, float w, float h)
    {
        auto& sig = GameSignal::instance();

        // FPS 覆盖层（左上角）
        if (GET_SETTING_KEY_INT("display.showFps", 0))
        {
            float fps = 0.f;
            {
                std::lock_guard<std::mutex> lk(m_fpsMutex);
                fps = m_currentFps;
            }
            if (fps > 0.f)
                GameOverlayRenderer::drawFps(vg, x, y, fps);
        }

        // 快进覆盖层（右上角）
        if (sig.isFastForward() && GET_SETTING_KEY_INT("display.showFfOverlay", 1))
            GameOverlayRenderer::drawFastForward(vg, x, y, w);

        // 倒带覆盖层（顶部居中）
        if (sig.isRewinding() && GET_SETTING_KEY_INT("display.showRewindOverlay", 1))
            GameOverlayRenderer::drawRewind(vg, x, y, w);

        // 暂停覆盖层（顶部居中，快进/倒带时不另外显示）
        if (sig.isPaused() && !sig.isFastForward() && !sig.isRewinding())
            GameOverlayRenderer::drawPaused(vg, x, y, w);

        // 静音覆盖层（右下角）
        if (sig.isMuted() && GET_SETTING_KEY_INT("display.showMuteOverlay", 1))
            GameOverlayRenderer::drawMute(vg, x, y, w, h);
    }

    // ============================================================
    // _uploadPendingFrame – 将游戏线程产出的最新帧上传到 GPU
    // ============================================================
    void MgbaGameView::_uploadPendingFrame()
    {
        if (!m_rendererReady) return;

        LibretroLoader::VideoFrame frame;
        bool hasFrame = false;
        {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            if (m_frameReady) {
                frame       = std::move(m_pendingFrame);
                m_frameReady = false;
                hasFrame     = true;
            }
        }

        if (hasFrame) {
            if (_useNdsAcceleratedTexture())
                return;
            if (m_ndsSplitShaderRenderer) {
                _uploadNdsSplitShaderFrame(frame);
                m_firstFrameUploaded.store(true, std::memory_order_release);
                return;
            }
            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
                m_ndsLayout != "priority_top")
                frame = _layoutNdsFrame(frame);
            const bool isNds = m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS);
            const auto uploadStart = std::chrono::steady_clock::now();
            m_renderer.uploadFrame(frame);
            m_firstFrameUploaded.store(true, std::memory_order_release);
            if (isNds) {
                const auto uploadUs = std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - uploadStart).count();
                if (uploadUs > 3000)
                    brls::Logger::warning("MgbaGameView: NDS uploadFrame took {} us ({}x{})",
                                          uploadUs, frame.width, frame.height);
            }
        }
    }

    bool MgbaGameView::_useNdsSplitShader() const
    {
        if (m_gameEntry.platform != static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS))
            return false;
#ifndef __SWITCH__
        return m_gameEntry.shaderEnabled &&
               !m_gameEntry.shaderPath.empty() &&
               m_gameEntry.ndsInternalResolution <= 1;
#else
        return false;
#endif
    }

    bool MgbaGameView::_useNdsAcceleratedTexture() const
    {
#ifdef __SWITCH__
        return false;
#else
        auto* textureCore = dynamic_cast<beiklive::IEmulatorVideoTexture*>(m_core);
        return m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
               !m_ndsSplitShaderRenderer &&
               textureCore != nullptr &&
               textureCore->IsVideoTextureReady();
#endif
    }

    void MgbaGameView::_syncNdsVideoFrameMode()
    {
        if (!isNdsPlatform(m_gameEntry.platform) || !m_core)
            return;

        auto* frameMode = dynamic_cast<beiklive::IEmulatorVideoFrameMode*>(m_core);
        if (!frameMode)
            return;

        const bool readbackEnabled =
#ifdef __SWITCH__
            false;
#else
            _useNdsSplitShader() ||
            (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
             m_gameEntry.ndsInternalResolution > 1);
#endif
        frameMode->SetAcceleratedFrameReadbackEnabled(readbackEnabled);
        brls::Logger::info("MgbaGameView: NDS accelerated readback {}",
                           readbackEnabled ? "enabled" : "disabled");
    }

    void MgbaGameView::_applySavedShaderParams(beiklive::GameRenderer& renderer) const
    {
        if (m_gameEntry.shaderParaNames.empty())
            return;

        if (m_gameEntry.shaderParaPath != m_gameEntry.shaderPath) {
            brls::Logger::debug("MgbaGameView: 跳过旧着色器参数，预设路径不匹配");
            return;
        }

        const auto& params = renderer.getShaderParams();
        if (params.size() != m_gameEntry.shaderParaNames.size() ||
            params.size() != m_gameEntry.shaderParaValues.size()) {
            brls::Logger::debug("MgbaGameView: 跳过旧着色器参数，参数数量不匹配");
            return;
        }

        for (size_t i = 0; i < params.size(); ++i) {
            if (params[i].name != m_gameEntry.shaderParaNames[i]) {
                brls::Logger::debug("MgbaGameView: 跳过旧着色器参数，参数名称不匹配");
                return;
            }
        }

        for (size_t i = 0; i < params.size(); ++i)
            renderer.setShaderParam(m_gameEntry.shaderParaNames[i], m_gameEntry.shaderParaValues[i]);
    }

    bool MgbaGameView::_initGameRenderers(unsigned gw, unsigned gh, const std::string& shaderPath)
    {
        m_renderer.deinit();
        m_ndsTopRenderer.deinit();
        m_ndsBottomRenderer.deinit();
        m_ndsSplitShaderRenderer = _useNdsSplitShader();
        _syncNdsVideoFrameMode();

        if (!m_ndsSplitShaderRenderer) {
            if (!m_renderer.init(gw, gh, false, shaderPath))
                return false;
            if (!shaderPath.empty())
                _applySavedShaderParams(m_renderer);
            return true;
        }

        if (!m_renderer.init(256, 384, false, ""))
            return false;
        if (!m_ndsTopRenderer.init(256, 192, false, shaderPath)) {
            m_renderer.deinit();
            return false;
        }
        if (!m_ndsBottomRenderer.init(256, 192, false, shaderPath)) {
            m_ndsTopRenderer.deinit();
            m_renderer.deinit();
            return false;
        }
        _applySavedShaderParams(m_ndsTopRenderer);
        _applySavedShaderParams(m_ndsBottomRenderer);
        brls::Logger::info("MgbaGameView: NDS split-screen shader renderer enabled");
        return true;
    }

    void MgbaGameView::_clearGameViewBackground(float x, float y, float w, float h,
                                            float windowScale, int windowW, int windowH)
    {
        if (w <= 0.0f || h <= 0.0f || windowScale <= 0.0f || windowW <= 0 || windowH <= 0)
            return;

        const GLint sx = static_cast<GLint>(std::floor(x * windowScale));
        const GLint sy = static_cast<GLint>(std::floor(windowH - (y + h) * windowScale));
        const GLsizei sw = static_cast<GLsizei>(std::ceil(w * windowScale));
        const GLsizei sh = static_cast<GLsizei>(std::ceil(h * windowScale));
        if (sw <= 0 || sh <= 0)
            return;

        GLboolean scissorEnabled = glIsEnabled(GL_SCISSOR_TEST);
        GLint prevScissor[4] = {};
        GLfloat prevClearColor[4] = {};
        GLboolean prevColorMask[4] = {};

        glGetIntegerv(GL_SCISSOR_BOX, prevScissor);
        glGetFloatv(GL_COLOR_CLEAR_VALUE, prevClearColor);
        glGetBooleanv(GL_COLOR_WRITEMASK, prevColorMask);

        glEnable(GL_SCISSOR_TEST);
        glScissor(sx, std::max<GLint>(0, sy), sw, sh);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (scissorEnabled)
            glEnable(GL_SCISSOR_TEST);
        else
            glDisable(GL_SCISSOR_TEST);
        glScissor(prevScissor[0], prevScissor[1], prevScissor[2], prevScissor[3]);
        glClearColor(prevClearColor[0], prevClearColor[1], prevClearColor[2], prevClearColor[3]);
        glColorMask(prevColorMask[0], prevColorMask[1], prevColorMask[2], prevColorMask[3]);
    }

    bool MgbaGameView::_drawNdsAcceleratedTexture(const beiklive::DisplayRect& rect,
                                              float windowScale, int windowW, int windowH)
    {
        if (!_useNdsAcceleratedTexture())
            return false;

        auto* textureCore = dynamic_cast<beiklive::IEmulatorVideoTexture*>(m_core);
        beiklive::EmulatorVideoTexture videoTex;
        if (!textureCore || !textureCore->GetVideoTexture(videoTex) ||
            videoTex.texture == 0 || videoTex.width == 0 || videoTex.height == 0)
            return false;

        const GLuint tex = static_cast<GLuint>(videoTex.texture);
        const unsigned texW = videoTex.width;
        const unsigned texH = videoTex.height;
        const float fullH = static_cast<float>(texH);
        const float topV0 = 0.0f;
        const float topV1 = (192.0f * static_cast<float>(videoTex.scale)) / fullH;
        const float bottomV0 = ((192.0f + 2.0f) * static_cast<float>(videoTex.scale)) / fullH;
        const float bottomV1 = 1.0f;

        const beiklive::DisplayRect layoutRect = _unrotateNdsRect(rect, rect);
        const auto orientationUv = _ndsOrientationUv();
        for (const auto& screenRect : _computeNdsScreenDrawRects(layoutRect)) {
            const float v0 = screenRect.topScreen ? topV0 : bottomV0;
            const float v1 = screenRect.topScreen ? topV1 : bottomV1;
            std::array<float, 8> uv{};
            for (size_t i = 0; i < 4; ++i) {
                const float u = orientationUv[i * 2];
                const float v = orientationUv[i * 2 + 1];
                uv[i * 2] = u;
                uv[i * 2 + 1] = v0 + (v1 - v0) * v;
            }
            const auto rotatedRect = _rotateNdsScreenRect(screenRect.rect, layoutRect, rect);
            m_renderer.drawExternalTexture(tex, texW, texH,
                                           rotatedRect.x, rotatedRect.y,
                                           rotatedRect.w, rotatedRect.h,
                                           windowScale, windowW, windowH,
                                           uv,
                                           true);
        }
        return true;
    }

    void MgbaGameView::_uploadNdsSplitShaderFrame(const LibretroLoader::VideoFrame& frame)
    {
        if (frame.width == 0 || frame.height == 0 ||
            frame.width % 256u != 0 || frame.height != 384u * (frame.width / 256u) ||
            frame.pixels.size() < static_cast<size_t>(frame.width) * frame.height)
            return;

        const unsigned scale = frame.width / 256u;
        const unsigned screenW = 256u * scale;
        const unsigned screenH = 192u * scale;
        const size_t screenPixels = static_cast<size_t>(screenW) * screenH;

        m_ndsTopUploadFrame.width = screenW;
        m_ndsTopUploadFrame.height = screenH;
        m_ndsTopUploadFrame.pixels.resize(screenPixels);
        std::copy(frame.pixels.begin(), frame.pixels.begin() + screenPixels,
                  m_ndsTopUploadFrame.pixels.begin());

        m_ndsBottomUploadFrame.width = screenW;
        m_ndsBottomUploadFrame.height = screenH;
        m_ndsBottomUploadFrame.pixels.resize(screenPixels);
        std::copy(frame.pixels.begin() + screenPixels, frame.pixels.begin() + screenPixels * 2,
                  m_ndsBottomUploadFrame.pixels.begin());

        m_ndsTopRenderer.uploadFrame(m_ndsTopUploadFrame);
        m_ndsBottomRenderer.uploadFrame(m_ndsBottomUploadFrame);
    }

    std::vector<MgbaGameView::NdsScreenDrawRect> MgbaGameView::_computeNdsScreenDrawRects(
        const beiklive::DisplayRect& layoutRect) const
    {
        std::vector<NdsScreenDrawRect> rects;
        if (layoutRect.w <= 0.f || layoutRect.h <= 0.f)
            return rects;

        auto add = [&](bool topScreen, float x, float y, float w, float h,
                       float canvasW, float canvasH) {
            rects.push_back({
                _mapNdsSourceScreen(topScreen),
                {
                    layoutRect.x + layoutRect.w * (x / canvasW),
                    layoutRect.y + layoutRect.h * (y / canvasH),
                    layoutRect.w * (w / canvasW),
                    layoutRect.h * (h / canvasH)
                }
            });
        };

        if (m_ndsLayout == "custom") {
            constexpr float canvasW = 1280.f;
            constexpr float canvasH = 720.f;
            auto place = [&](bool topScreen) {
                const float scale = std::clamp(topScreen ? m_gameEntry.ndsTopScale : m_gameEntry.ndsBottomScale,
                                               1.0f, 10.0f);
                const float offsetX = topScreen ? m_gameEntry.ndsTopOffsetX : m_gameEntry.ndsBottomOffsetX;
                const float offsetY = topScreen ? m_gameEntry.ndsTopOffsetY : m_gameEntry.ndsBottomOffsetY;
                const float baseX = topScreen ? 224.0f : 800.0f;
                const float baseY = 264.0f;
                const int dstW = std::max(4, static_cast<int>(std::lround(256.f * scale / 4.0f)) * 4);
                const int dstH = std::max(3, (dstW * 3) / 4);
                const float dstX = baseX + offsetX - (static_cast<float>(dstW) - 256.f) * 0.5f;
                const float dstY = baseY + offsetY - (static_cast<float>(dstH) - 192.f) * 0.5f;
                add(topScreen, dstX, dstY, static_cast<float>(dstW), static_cast<float>(dstH),
                    canvasW, canvasH);
            };
            place(true);
            place(false);
            return rects;
        }

        if (m_ndsLayout == "hybrid") {
            constexpr float canvasW = 1280.f;
            constexpr float canvasH = 720.f;
            add(true, 0.f, 40.f, 853.f, 640.f, canvasW, canvasH);
            add(true, 853.f, 40.f, 427.f, 320.f, canvasW, canvasH);
            add(false, 853.f, 360.f, 427.f, 320.f, canvasW, canvasH);
            return rects;
        }

        if (m_ndsLayout == "priority_top") {
            constexpr float kAspect = 4.0f / 3.0f;
            float topH = layoutRect.h;
            float topW = topH * kAspect;
            float bottomW = std::max(0.0f, layoutRect.w - topW);
            float bottomH = std::min(layoutRect.h, bottomW / kAspect);

            if (m_ndsIntegerScale) {
                float topScale = 3.0f;
                float bottomScale = 2.0f;
                if (layoutRect.w < 1280.0f || layoutRect.h < 576.0f) {
                    topScale = std::floor(std::min(layoutRect.w / 256.0f, layoutRect.h / 192.0f));
                    topScale = std::max(1.0f, topScale);
                    bottomScale = 0.0f;
                    for (; topScale >= 1.0f; topScale -= 1.0f) {
                        const float remainingW = layoutRect.w - topScale * 256.0f;
                        bottomScale = std::floor(std::min(layoutRect.h / 192.0f, remainingW / 256.0f));
                        if (bottomScale >= 1.0f)
                            break;
                    }
                    topScale = std::max(1.0f, topScale);
                    bottomScale = std::max(1.0f, bottomScale);
                }
                topW = topScale * 256.0f;
                topH = topScale * 192.0f;
                bottomW = bottomScale * 256.0f;
                bottomH = bottomScale * 192.0f;
            }

            const float topY = layoutRect.y + (layoutRect.h - topH) * 0.5f;
            const float bottomX = layoutRect.x + topW;
            const float bottomY = layoutRect.y + (layoutRect.h - bottomH) * 0.5f;
            rects.push_back({_mapNdsSourceScreen(true), {layoutRect.x, topY, topW, topH}});
            if (bottomW > 0.0f && bottomH > 0.0f)
                rects.push_back({_mapNdsSourceScreen(false), {bottomX, bottomY, bottomW, bottomH}});
            return rects;
        }

        if (m_ndsLayout == "horizontal") {
            rects.push_back({true, {layoutRect.x, layoutRect.y, layoutRect.w * 0.5f, layoutRect.h}});
            rects.push_back({false, {layoutRect.x + layoutRect.w * 0.5f, layoutRect.y,
                                     layoutRect.w * 0.5f, layoutRect.h}});
            return rects;
        }

        if (m_ndsLayout == "top") {
            rects.push_back({true, layoutRect});
            return rects;
        }

        if (m_ndsLayout == "bottom") {
            rects.push_back({false, layoutRect});
            return rects;
        }

        rects.push_back({true, {layoutRect.x, layoutRect.y, layoutRect.w, layoutRect.h * 0.5f}});
        rects.push_back({false, {layoutRect.x, layoutRect.y + layoutRect.h * 0.5f,
                                 layoutRect.w, layoutRect.h * 0.5f}});
        return rects;
    }

    bool MgbaGameView::_mapNdsSourceScreen(bool layoutTopScreen) const
    {
        return m_ndsScreensSwapped ? !layoutTopScreen : layoutTopScreen;
    }

    beiklive::DisplayRect MgbaGameView::_unrotateNdsRect(
        const beiklive::DisplayRect& orientedRect, const beiklive::DisplayRect&) const
    {
        if (m_ndsLayout == "custom" || m_ndsLayout == "hybrid" || m_ndsLayout == "priority_top")
            return orientedRect;

        if (m_ndsScreenOrientation == "90" || m_ndsScreenOrientation == "270")
        {
            const float cx = orientedRect.x + orientedRect.w * 0.5f;
            const float cy = orientedRect.y + orientedRect.h * 0.5f;
            return {cx - orientedRect.h * 0.5f, cy - orientedRect.w * 0.5f,
                    orientedRect.h, orientedRect.w};
        }
        return orientedRect;
    }

    beiklive::DisplayRect MgbaGameView::_rotateNdsScreenRect(
        const beiklive::DisplayRect& screenRect,
        const beiklive::DisplayRect& layoutRect,
        const beiklive::DisplayRect& orientedRect) const
    {
        if (m_ndsLayout == "custom" || m_ndsLayout == "hybrid" || m_ndsLayout == "priority_top") {
            if (m_ndsScreenOrientation == "90" || m_ndsScreenOrientation == "270") {
                const float cx = screenRect.x + screenRect.w * 0.5f;
                const float cy = screenRect.y + screenRect.h * 0.5f;
                return {cx - screenRect.h * 0.5f, cy - screenRect.w * 0.5f,
                        screenRect.h, screenRect.w};
            }
            return screenRect;
        }

        if (m_ndsScreenOrientation == "0")
            return screenRect;

        const float relX = screenRect.x - layoutRect.x;
        const float relY = screenRect.y - layoutRect.y;

        if (m_ndsScreenOrientation == "180") {
            return {
                orientedRect.x + (layoutRect.w - relX - screenRect.w),
                orientedRect.y + (layoutRect.h - relY - screenRect.h),
                screenRect.w,
                screenRect.h
            };
        }

        if (m_ndsScreenOrientation == "90") {
            return {
                orientedRect.x + (layoutRect.h - relY - screenRect.h),
                orientedRect.y + relX,
                screenRect.h,
                screenRect.w
            };
        }

        if (m_ndsScreenOrientation == "270") {
            return {
                orientedRect.x + relY,
                orientedRect.y + (layoutRect.w - relX - screenRect.w),
                screenRect.h,
                screenRect.w
            };
        }

        return screenRect;
    }

    std::array<float, 8> MgbaGameView::_ndsOrientationUv() const
    {
        if (m_ndsScreenOrientation == "90")
            return {0.f, 1.f, 0.f, 0.f, 1.f, 0.f, 1.f, 1.f};
        if (m_ndsScreenOrientation == "180")
            return {1.f, 1.f, 0.f, 1.f, 0.f, 0.f, 1.f, 0.f};
        if (m_ndsScreenOrientation == "270")
            return {1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f};
        return {0.f, 0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f};
    }

    bool MgbaGameView::_mapNdsPointToUnrotated(float x, float y,
                                           const beiklive::DisplayRect& orientedRect,
                                           const beiklive::DisplayRect& layoutRect,
                                           float& outX, float& outY) const
    {
        if (orientedRect.w <= 0.f || orientedRect.h <= 0.f ||
            layoutRect.w <= 0.f || layoutRect.h <= 0.f)
            return false;

        const float rx = x - orientedRect.x;
        const float ry = y - orientedRect.y;
        if (rx < 0.f || ry < 0.f || rx > orientedRect.w || ry > orientedRect.h)
            return false;

        if (m_ndsScreenOrientation == "90") {
            outX = layoutRect.x + ry;
            outY = layoutRect.y + (layoutRect.h - rx);
        } else if (m_ndsScreenOrientation == "270") {
            outX = layoutRect.x + (layoutRect.w - ry);
            outY = layoutRect.y + rx;
        } else if (m_ndsScreenOrientation == "180") {
            outX = layoutRect.x + (layoutRect.w - rx);
            outY = layoutRect.y + (layoutRect.h - ry);
        } else {
            outX = layoutRect.x + rx;
            outY = layoutRect.y + ry;
        }
        return true;
    }

    void MgbaGameView::_requestLastFrameUpload()
    {
        std::lock_guard<std::mutex> lk(m_frameMutex);
        if (!m_hasLastRawFrame || m_lastRawFrame.pixels.empty())
            return;

        m_pendingFrame = m_lastRawFrame;
        m_frameReady = true;
    }

    // ============================================================
    // _registerGameInput – 注册游戏热键
    // ============================================================
    void MgbaGameView::_registerGameInput()
    {
        GameSignal::instance().clearGameButtonMask(0);
        GameSignal::instance().clearGameButtonMask(1);
        // ---- 读取摇杆模式配置 -------------------------------------------
        bool joystickEnabled  = GET_SETTING_KEY_INT("input.joystick.enabled",  1) != 0;
        bool joystickDiagonal = GET_SETTING_KEY_INT("input.joystick.diagonal", 1) != 0;
        GameInputManager::instance().setDiagonalMode(joystickDiagonal);
        const bool isNds = isNdsPlatform(m_gameEntry.platform);
        const std::string mappingPrefix = beiklive::input_mapping::platformPrefix(m_gameEntry.platform);
        const unsigned platformMask = beiklive::input_mapping::platformMaskForPlatform(m_gameEntry.platform);
        auto readMapping = [&mappingPrefix](const std::string& key, const std::string& def) {
            return GET_SETTING_KEY_STR(beiklive::input_mapping::makeKey(mappingPrefix, key), def);
        };

        // ---- 游戏按键绑定（从配置读取多 combo 按键映射）--------------------
        // 按住时持续置位，松开时清除，使用 GameSignal 按键位掩码传入游戏帧。
        // GameBtnInfo：游戏按键配置项，存储模拟器功能键、配置后缀和 libretro 手柄 ID 的映射关系。
        struct GameBtnInfo {
            EmuFunctionKey emuKey;      ///< 模拟器功能键枚举值
            const char*    cfgSuffix;   ///< 配置键后缀（"handle.<suffix>" 为完整键）
            unsigned       retroId;     ///< libretro 手柄 ID（RETRO_DEVICE_ID_JOYPAD_*）
        };
        static const GameBtnInfo gameBtnInfos[] = {
            { EMU_A,      "a",      8  }, // RETRO_DEVICE_ID_JOYPAD_A
            { EMU_B,      "b",      0  }, // RETRO_DEVICE_ID_JOYPAD_B
            { EMU_X,      "x",      9  }, // RETRO_DEVICE_ID_JOYPAD_X
            { EMU_Y,      "y",      1  }, // RETRO_DEVICE_ID_JOYPAD_Y
            { EMU_UP,     "up",     4  }, // RETRO_DEVICE_ID_JOYPAD_UP
            { EMU_DOWN,   "down",   5  }, // RETRO_DEVICE_ID_JOYPAD_DOWN
            { EMU_LEFT,   "left",   6  }, // RETRO_DEVICE_ID_JOYPAD_LEFT
            { EMU_RIGHT,  "right",  7  }, // RETRO_DEVICE_ID_JOYPAD_RIGHT
            { EMU_L,      "l",      10 }, // RETRO_DEVICE_ID_JOYPAD_L
            { EMU_R,      "r",      11 }, // RETRO_DEVICE_ID_JOYPAD_R
            { EMU_L2,     "l2",     12 }, // RETRO_DEVICE_ID_JOYPAD_L2
            { EMU_R2,     "r2",     13 }, // RETRO_DEVICE_ID_JOYPAD_R2
            { EMU_L3,     "l3",     14 }, // RETRO_DEVICE_ID_JOYPAD_L3
            { EMU_R3,     "r3",     15 }, // RETRO_DEVICE_ID_JOYPAD_R3
            { EMU_START,  "start",  3  }, // RETRO_DEVICE_ID_JOYPAD_START
            { EMU_SELECT, "select", 2  }, // RETRO_DEVICE_ID_JOYPAD_SELECT
        };
        for (const auto& info : gameBtnInfos) {
            const auto* defaultValue = beiklive::input_mapping::defaultHandleValue(info.cfgSuffix);
            bool supported = false;
            for (const auto& entry : beiklive::input_mapping::kGameButtonDefaults) {
                if (std::string(info.cfgSuffix) == entry.suffix) {
                    supported = (entry.platformMask & platformMask) != 0;
                    break;
                }
            }
            if (!supported)
                continue;
            const std::string cfgKey = beiklive::input_mapping::makeHandleKey(mappingPrefix, info.cfgSuffix);
            std::string val = GET_SETTING_KEY_STR(
                cfgKey,
                defaultValue);
            const bool usePolledGameInput =
                GET_SETTING_KEY_INT("input.polled_game_input", 1) != 0;
            if (usePolledGameInput)
                continue;
            if (val == "none" || val.empty()) continue;
            auto combos = beiklive::tools::parseMultiCombo(val);
            if (combos.empty()) continue;
            unsigned rid = info.retroId;
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    info.emuKey, {combo},
                    [rid]() { GameSignal::instance().pressGameButton(rid); },
                    TriggerType::HOLD);
                GameInputManager::instance().registerEmuFunctionKey(
                    info.emuKey, {combo},
                    [rid]() { GameSignal::instance().releaseGameButton(rid); },
                    TriggerType::RELEASE);
            }
        }

        // ---- 摇杆方向键映射（从配置读取，受 joystickEnabled 控制）---------
        // retroId 对应 RETRO_DEVICE_ID_JOYPAD：UP=4, DOWN=5, LEFT=6, RIGHT=7
        if (joystickEnabled) {
            // StickBtnInfo：摇杆方向键配置项，存储功能键枚举、配置后缀和 libretro ID 的映射关系。
            struct StickBtnInfo {
                EmuFunctionKey emuKey;      ///< 模拟器功能键枚举值
                const char*    cfgSuffix;   ///< 配置键后缀（"handle.<suffix>"）
                unsigned       retroId;     ///< libretro 手柄 ID
            };
            static const StickBtnInfo stickBtnInfos[] = {
                { EMU_LEFT_STICK_UP,     "lstick_up",    4  }, // RETRO_DEVICE_ID_JOYPAD_UP
                { EMU_LEFT_STICK_DOWN,   "lstick_down",  5  }, // RETRO_DEVICE_ID_JOYPAD_DOWN
                { EMU_LEFT_STICK_LEFT,   "lstick_left",  6  }, // RETRO_DEVICE_ID_JOYPAD_LEFT
                { EMU_LEFT_STICK_RIGHT,  "lstick_right", 7  }, // RETRO_DEVICE_ID_JOYPAD_RIGHT
                { EMU_RIGHT_STICK_UP,    "rstick_up",    4  }, // RETRO_DEVICE_ID_JOYPAD_UP
                { EMU_RIGHT_STICK_DOWN,  "rstick_down",  5  }, // RETRO_DEVICE_ID_JOYPAD_DOWN
                { EMU_RIGHT_STICK_LEFT,  "rstick_left",  6  }, // RETRO_DEVICE_ID_JOYPAD_LEFT
                { EMU_RIGHT_STICK_RIGHT, "rstick_right", 7  }, // RETRO_DEVICE_ID_JOYPAD_RIGHT
            };
            for (const auto& info : stickBtnInfos) {
                const std::string cfgKey = beiklive::input_mapping::makeHandleKey(mappingPrefix, info.cfgSuffix);
                std::string val = GET_SETTING_KEY_STR(
                    cfgKey,
                    beiklive::input_mapping::defaultHandleValue(info.cfgSuffix));
                const bool usePolledGameInput =
                    GET_SETTING_KEY_INT("input.polled_game_input", 1) != 0;
                if (usePolledGameInput)
                    continue;
                auto combos = beiklive::tools::parseMultiCombo(val);
                if (combos.empty()) continue;
                unsigned rid = info.retroId;
                const bool ndsRightStick = isNds && isNdsRightStickMapping(info.cfgSuffix);
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        info.emuKey, {combo},
                        [this, rid, ndsRightStick]() {
                            if (ndsRightStick && m_ndsVirtualPointerMode)
                                return;
                            GameSignal::instance().pressGameButton(rid);
                        },
                        TriggerType::HOLD);
                    GameInputManager::instance().registerEmuFunctionKey(
                        info.emuKey, {combo},
                        [rid]() { GameSignal::instance().releaseGameButton(rid); },
                        TriggerType::RELEASE);
                }
            }
        }

        // ---- 功能热键绑定（从配置读取多 combo）----------------------------

        // 打开菜单
        {
            std::string val = readMapping("hotkey.menu.pad", "PAD_LT+PAD_RT");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_OPEN_MENU, {combo},
                    [this]() {
                        brls::Logger::debug("打开菜单热键触发！");
                        GameSignal::instance().requestPause(true);
                        GameSignal::instance().requestOpenMenu();
                        this->setFocusable(false);
                    });
            }
        }

        // 快进（支持按住/切换两种模式）
        {
            std::string val = readMapping("handle.fastforward", "PAD_LSB");
            std::string mode = GET_SETTING_KEY_STR("fastforward.mode", "hold");
            auto combos = beiklive::tools::parseMultiCombo(val);
            if (mode == "hold") {
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_FAST_FORWARD, {combo},
                        []() { GameSignal::instance().requestFastForward(true); },
                        TriggerType::HOLD);
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_FAST_FORWARD, {combo},
                        []() { GameSignal::instance().requestFastForward(false); },
                        TriggerType::RELEASE);
                }
            } else {
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_FAST_FORWARD, {combo},
                        []() {
                            bool cur = GameSignal::instance().isFastForward();
                            GameSignal::instance().requestFastForward(!cur);
                            brls::Logger::debug("快进切换：{}", !cur);
                        });
                }
            }
        }

        // 倒带切换：若启用可视化倒带界面则打开UI，否则执行传统倒带
        if (!isNds) {
            std::string val  = readMapping("handle.rewind", "none");
            std::string mode = GET_SETTING_KEY_STR("rewind.mode", "hold");
            auto combos = beiklive::tools::parseMultiCombo(val);
            bool showUI = m_rewindShowUI;
            for (const auto& combo : combos) {
                if (showUI) {
                    // 可视化倒带模式：按键触发时打开倒带选择界面
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        [this]() {
                            brls::Logger::debug("倒带UI触发！");
                            GameSignal::instance().requestOpenRewindUI();
                            this->setFocusable(false);
                        });
                } else if (mode == "hold") {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        []() { GameSignal::instance().requestRewind(true); },
                        TriggerType::HOLD);
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        []() { GameSignal::instance().requestRewind(false); },
                        TriggerType::RELEASE);
                } else {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_REWIND, {combo},
                        []() {
                            bool cur = GameSignal::instance().isRewinding();
                            GameSignal::instance().requestRewind(!cur);
                            brls::Logger::debug("倒带切换：{}", !cur);
                        });
                }
            }
        }

        // 快速保存（默认槽位 1）
        {
            std::string val = readMapping("hotkey.quicksave.pad", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_QUICK_SAVE, {combo},
                    []() { GameSignal::instance().requestQuickSave(1); });
            }
        }

        // 快速读取（默认槽位 1）
        {
            std::string val = readMapping("hotkey.quickload.pad", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_QUICK_LOAD, {combo},
                    []() { GameSignal::instance().requestQuickLoad(1); });
            }
        }

        // 截图
        {
            std::string val = readMapping("hotkey.screenshot.pad", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_SCREENSHOT, {combo},
                    []() { GameSignal::instance().requestScreenshot(); });
            }
        }

        // 静音切换
        {
            std::string val = readMapping("hotkey.mute.pad", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_MUTE, {combo},
                    []() {
                        bool cur = GameSignal::instance().isMuted();
                        GameSignal::instance().requestMute(!cur);
                    });
            }
        }

        // NDS 虚拟指针
        if (isNds) {
            {
                std::string val = readMapping("hotkey.pointer_mode.pad", "none");
                auto combos = beiklive::tools::parseMultiCombo(val);
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_NDS_POINTER_MODE, {combo},
                        [this]() { _toggleNdsVirtualPointerMode(); });
                }
            }
            {
                std::string val = readMapping("hotkey.pointer_click.pad", "none");
                auto combos = beiklive::tools::parseMultiCombo(val);
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_NDS_POINTER_CLICK, {combo},
                        [this]() { _setNdsVirtualPointerClick(true); },
                        TriggerType::HOLD);
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_NDS_POINTER_CLICK, {combo},
                        [this]() { _setNdsVirtualPointerClick(false); },
                        TriggerType::RELEASE);
                }
            }
            {
                std::string val = readMapping("hotkey.swap_screens.pad", "none");
                auto combos = beiklive::tools::parseMultiCombo(val);
                for (const auto& combo : combos) {
                    GameInputManager::instance().registerEmuFunctionKey(
                        EmuFunctionKey::EMU_NDS_SWAP_SCREENS, {combo},
                        [this]() {
                            m_ndsScreensSwapped = !m_ndsScreensSwapped;
                            m_ndsTouchRect = {};
                            _releaseNdsVirtualPointerTouch();
                            _requestLastFrameUpload();
                            brls::Logger::debug("MgbaGameView: NDS screens swapped {}", m_ndsScreensSwapped);
                        });
                }
            }
        }

        // 连发 A（Turbo A）
        {
            std::string val = readMapping("handle.a_turbo", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_A_TURBO, {combo},
                    [this]() { m_turboAheld.store(true, std::memory_order_release); },
                    TriggerType::HOLD);
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_A_TURBO, {combo},
                    [this]() { m_turboAheld.store(false, std::memory_order_release); },
                    TriggerType::RELEASE);
            }
        }

        // 连发 B（Turbo B）
        {
            std::string val = readMapping("handle.b_turbo", "none");
            auto combos = beiklive::tools::parseMultiCombo(val);
            for (const auto& combo : combos) {
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_B_TURBO, {combo},
                    [this]() { m_turboBheld.store(true, std::memory_order_release); },
                    TriggerType::HOLD);
                GameInputManager::instance().registerEmuFunctionKey(
                    EmuFunctionKey::EMU_B_TURBO, {combo},
                    [this]() { m_turboBheld.store(false, std::memory_order_release); },
                    TriggerType::RELEASE);
            }
        }
    }

    void MgbaGameView::_registerTouchInput()
    {
        addGestureRecognizer(new brls::TapGestureRecognizer(
            [this](brls::TapGestureStatus status, brls::Sound*) {
                const bool down = status.state == brls::GestureState::START ||
                                  status.state == brls::GestureState::UNSURE;
                _submitTouchPoint(status.position.x, status.position.y, down);
            }));

        addGestureRecognizer(new brls::PanGestureRecognizer(
            [this](brls::PanGestureStatus status, brls::Sound*) {
                const bool down = status.state == brls::GestureState::START ||
                                  status.state == brls::GestureState::STAY;
                _submitTouchPoint(status.position.x, status.position.y, down);
            },
            brls::PanAxis::ANY));
    }

    void MgbaGameView::_pollNdsTouchInput()
    {
        if (m_gameEntry.platform != static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) || !m_core)
            return;

        if (brls::Application::getCurrentFocus() != this) {
            if (m_ndsTouchActive) {
                m_ndsTouchActive = false;
                _submitTouchPoint(0.f, 0.f, false);
            }
            return;
        }

        std::vector<brls::RawTouchState> states;
        brls::Application::getPlatform()->getInputManager()->updateTouchStates(&states);
        if (!states.empty() && states.front().pressed) {
            m_ndsTouchActive = true;
            _submitTouchPoint(states.front().position.x, states.front().position.y, true);
            return;
        }

        brls::RawMouseState mouse;
        brls::Application::getPlatform()->getInputManager()->updateMouseStates(&mouse);
        if (mouse.leftButton) {
            m_ndsTouchActive = true;
            _submitTouchPoint(mouse.position.x, mouse.position.y, true);
        } else if (m_ndsTouchActive) {
            m_ndsTouchActive = false;
            _submitTouchPoint(0.f, 0.f, false);
        }
    }

    void MgbaGameView::_submitTouchPoint(float x, float y, bool down)
    {
        if (m_gameEntry.platform != static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) || !m_core)
            return;

        auto* touchCore = dynamic_cast<beiklive::IEmulatorTouchInput*>(m_core);
        if (!touchCore)
            return;

        const auto& orientedRect = m_ndsTouchRect;
        if (orientedRect.w <= 0.f || orientedRect.h <= 0.f)
        {
            touchCore->SetTouch(0, 0, false);
            return;
        }

        beiklive::DisplayRect layoutRect = _unrotateNdsRect(orientedRect, orientedRect);
        if (!down)
        {
            touchCore->SetTouch(0, 0, false);
            return;
        }

        for (const auto& screenRect : _computeNdsScreenDrawRects(layoutRect)) {
            if (screenRect.topScreen)
                continue;
            const auto rect = _rotateNdsScreenRect(screenRect.rect, layoutRect, orientedRect);
            const float localX = x - rect.x;
            const float localY = y - rect.y;
            if (localX < 0.f || localY < 0.f || localX > rect.w || localY > rect.h ||
                rect.w <= 0.f || rect.h <= 0.f)
                continue;
            const float nx = std::clamp(localX / rect.w, 0.f, 1.f);
            const float ny = std::clamp(localY / rect.h, 0.f, 1.f);
            float ndsU = nx;
            float ndsV = ny;
            if (m_ndsScreenOrientation == "90") {
                ndsU = ny;
                ndsV = 1.f - nx;
            } else if (m_ndsScreenOrientation == "180") {
                ndsU = 1.f - nx;
                ndsV = 1.f - ny;
            } else if (m_ndsScreenOrientation == "270") {
                ndsU = 1.f - ny;
                ndsV = nx;
            }
            const int ndsX = static_cast<int>(ndsU * 255.f + 0.5f);
            const int ndsY = static_cast<int>(ndsV * 191.f + 0.5f);
            touchCore->SetTouch(ndsX, ndsY, true);
            return;
        }

        touchCore->SetTouch(0, 0, false);
    }

    void MgbaGameView::_toggleNdsVirtualPointerMode()
    {
        if (!isNdsPlatform(m_gameEntry.platform))
            return;
        m_ndsVirtualPointerMode = !m_ndsVirtualPointerMode;
        m_ndsVirtualPointerLastUpdate = std::chrono::steady_clock::now();
        _releaseNdsVirtualPointerTouch();
        GameSignal::instance().releaseGameButton(4);
        GameSignal::instance().releaseGameButton(5);
        GameSignal::instance().releaseGameButton(6);
        GameSignal::instance().releaseGameButton(7);
        brls::Logger::debug("MgbaGameView: NDS pointer mode {}", m_ndsVirtualPointerMode ? "on" : "off");
    }

    void MgbaGameView::_setNdsVirtualPointerClick(bool down)
    {
        if (!isNdsPlatform(m_gameEntry.platform))
            return;
        m_ndsVirtualPointerClickHeld = down;
        if (!down)
            _releaseNdsVirtualPointerTouch();
    }

    void MgbaGameView::_releaseNdsVirtualPointerTouch()
    {
        if (!m_ndsVirtualPointerTouchDown)
            return;
        if (auto* touchCore = dynamic_cast<beiklive::IEmulatorTouchInput*>(m_core))
            touchCore->SetTouch(0, 0, false);
        m_ndsVirtualPointerTouchDown = false;
    }

    void MgbaGameView::_updateNdsVirtualPointer()
    {
        if (!isNdsPlatform(m_gameEntry.platform) || !m_core)
            return;

        const auto now = std::chrono::steady_clock::now();
        if (m_ndsVirtualPointerLastUpdate.time_since_epoch().count() == 0)
            m_ndsVirtualPointerLastUpdate = now;
        float dt = std::chrono::duration<float>(now - m_ndsVirtualPointerLastUpdate).count();
        m_ndsVirtualPointerLastUpdate = now;
        dt = std::clamp(dt, 0.f, 0.05f);

        if (brls::Application::getCurrentFocus() != this || !m_ndsVirtualPointerMode)
        {
            _releaseNdsVirtualPointerTouch();
            return;
        }

        if (m_ndsTouchActive)
        {
            _releaseNdsVirtualPointerTouch();
            return;
        }

        const auto state = GameInputManager::instance().getGamepadState(0);
        constexpr float kStickMax = 32767.f;
        constexpr float kDeadzone = 0.18f;
        constexpr float kPointerSpeed = 320.f;
        float sx = static_cast<float>(state.rightStickX) / kStickMax;
        float sy = static_cast<float>(state.rightStickY) / kStickMax;
        if (std::fabs(sx) < kDeadzone) sx = 0.f;
        if (std::fabs(sy) < kDeadzone) sy = 0.f;

        m_ndsVirtualPointerX = std::clamp(m_ndsVirtualPointerX + sx * kPointerSpeed * dt, 0.f, 255.f);
        m_ndsVirtualPointerY = std::clamp(m_ndsVirtualPointerY - sy * kPointerSpeed * dt, 0.f, 191.f);

        auto* touchCore = dynamic_cast<beiklive::IEmulatorTouchInput*>(m_core);
        if (!touchCore)
            return;
        if (m_ndsVirtualPointerClickHeld)
        {
            touchCore->SetTouch(static_cast<int>(m_ndsVirtualPointerX + 0.5f),
                                static_cast<int>(m_ndsVirtualPointerY + 0.5f),
                                true);
            m_ndsVirtualPointerTouchDown = true;
        }
        else
        {
            _releaseNdsVirtualPointerTouch();
        }
    }

    void MgbaGameView::_drawNdsVirtualPointer(NVGcontext* vg)
    {
        if (!vg || !isNdsPlatform(m_gameEntry.platform) || !m_ndsVirtualPointerMode)
            return;
        if (m_ndsTouchRect.w <= 0.f || m_ndsTouchRect.h <= 0.f)
            return;

        const auto& orientedRect = m_ndsTouchRect;
        const auto layoutRect = _unrotateNdsRect(orientedRect, orientedRect);
        for (const auto& screenRect : _computeNdsScreenDrawRects(layoutRect))
        {
            if (screenRect.topScreen)
                continue;

            const auto rect = _rotateNdsScreenRect(screenRect.rect, layoutRect, orientedRect);
            if (rect.w <= 0.f || rect.h <= 0.f)
                return;

            const float u = std::clamp(m_ndsVirtualPointerX / 255.f, 0.f, 1.f);
            const float v = std::clamp(m_ndsVirtualPointerY / 191.f, 0.f, 1.f);
            float nx = u;
            float ny = v;
            if (m_ndsScreenOrientation == "90") {
                nx = 1.f - v;
                ny = u;
            } else if (m_ndsScreenOrientation == "180") {
                nx = 1.f - u;
                ny = 1.f - v;
            } else if (m_ndsScreenOrientation == "270") {
                nx = v;
                ny = 1.f - u;
            }

            const float px = rect.x + nx * rect.w;
            const float py = rect.y + ny * rect.h;
            constexpr float kLen = 8.f;
            nvgBeginPath(vg);
            nvgMoveTo(vg, px - kLen, py);
            nvgLineTo(vg, px + kLen, py);
            nvgMoveTo(vg, px, py - kLen);
            nvgLineTo(vg, px, py + kLen);
            nvgStrokeWidth(vg, 2.5f);
            nvgStrokeColor(vg, m_ndsVirtualPointerClickHeld
                                   ? nvgRGBA(255, 64, 64, 255)
                                   : nvgRGBA(255, 255, 255, 255));
            nvgStroke(vg);
            return;
        }
    }

    LibretroLoader::VideoFrame MgbaGameView::_layoutNdsFrame(const LibretroLoader::VideoFrame& frame) const
    {
        if (frame.width == 0 || frame.height == 0 ||
            frame.width % 256u != 0 || frame.height != 384u * (frame.width / 256u) ||
            frame.pixels.size() < static_cast<size_t>(frame.width) * frame.height)
            return frame;

        const unsigned scale = frame.width / 256u;
        const unsigned screenW = 256u * scale;
        const unsigned screenH = 192u * scale;
        LibretroLoader::VideoFrame out;

        auto blitScaled = [&](LibretroLoader::VideoFrame& dst,
                              const LibretroLoader::VideoFrame& src,
                              unsigned srcOffsetY,
                              int dstX,
                              int dstY,
                              int dstW,
                              int dstH) {
            if (dstW <= 0 || dstH <= 0)
                return;
            for (int y = 0; y < dstH; ++y) {
                const int outY = dstY + y;
                if (outY < 0 || outY >= static_cast<int>(dst.height))
                    continue;
                const unsigned srcY = srcOffsetY + std::min<unsigned>(
                    screenH - 1,
                    static_cast<unsigned>((static_cast<uint64_t>(y) * screenH) / static_cast<unsigned>(dstH)));
                const uint32_t* srcRow = src.pixels.data() + static_cast<size_t>(srcY) * src.width;
                uint32_t* dstRow = dst.pixels.data() + static_cast<size_t>(outY) * dst.width;
                for (int x = 0; x < dstW; ++x) {
                    const int outX = dstX + x;
                    if (outX < 0 || outX >= static_cast<int>(dst.width))
                        continue;
                    const unsigned srcX = std::min<unsigned>(
                        screenW - 1,
                        static_cast<unsigned>((static_cast<uint64_t>(x) * screenW) / static_cast<unsigned>(dstW)));
                    dstRow[outX] = srcRow[srcX];
                }
            }
        };

        auto blitScaledOriented = [&](LibretroLoader::VideoFrame& dst,
                                      const LibretroLoader::VideoFrame& src,
                                      unsigned srcOffsetY,
                                      int dstX,
                                      int dstY,
                                      int dstW,
                                      int dstH) {
            if (m_ndsScreenOrientation == "0") {
                blitScaled(dst, src, srcOffsetY, dstX, dstY, dstW, dstH);
                return;
            }

            int drawX = dstX;
            int drawY = dstY;
            int drawW = dstW;
            int drawH = dstH;
            if (m_ndsScreenOrientation == "90" || m_ndsScreenOrientation == "270") {
                const int cx2 = dstX * 2 + dstW;
                const int cy2 = dstY * 2 + dstH;
                drawW = dstH;
                drawH = dstW;
                drawX = (cx2 - drawW) / 2;
                drawY = (cy2 - drawH) / 2;
            }

            if (drawW <= 0 || drawH <= 0)
                return;

            for (int y = 0; y < drawH; ++y) {
                const int outY = drawY + y;
                if (outY < 0 || outY >= static_cast<int>(dst.height))
                    continue;
                uint32_t* dstRow = dst.pixels.data() + static_cast<size_t>(outY) * dst.width;
                for (int x = 0; x < drawW; ++x) {
                    const int outX = drawX + x;
                    if (outX < 0 || outX >= static_cast<int>(dst.width))
                        continue;

                    const float nx = drawW > 1 ? static_cast<float>(x) / static_cast<float>(drawW - 1) : 0.f;
                    const float ny = drawH > 1 ? static_cast<float>(y) / static_cast<float>(drawH - 1) : 0.f;
                    float srcNormX = nx;
                    float srcNormY = ny;
                    if (m_ndsScreenOrientation == "90") {
                        srcNormX = ny;
                        srcNormY = 1.f - nx;
                    } else if (m_ndsScreenOrientation == "180") {
                        srcNormX = 1.f - nx;
                        srcNormY = 1.f - ny;
                    } else if (m_ndsScreenOrientation == "270") {
                        srcNormX = 1.f - ny;
                        srcNormY = nx;
                    }

                    const unsigned srcX = std::min<unsigned>(
                        screenW - 1,
                        static_cast<unsigned>(std::clamp(srcNormX, 0.f, 1.f) * static_cast<float>(screenW - 1) + 0.5f));
                    const unsigned srcY = srcOffsetY + std::min<unsigned>(
                        screenH - 1,
                        static_cast<unsigned>(std::clamp(srcNormY, 0.f, 1.f) * static_cast<float>(screenH - 1) + 0.5f));
                    dstRow[outX] = src.pixels[static_cast<size_t>(srcY) * src.width + srcX];
                }
            }
        };

        if (m_ndsLayout == "custom") {
            constexpr unsigned canvasW = 1280;
            constexpr unsigned canvasH = 720;
            constexpr float baseScale = 1.0f;
            out.width = canvasW;
            out.height = canvasH;
            out.pixels.assign(static_cast<size_t>(out.width) * out.height, 0xFF000000u);

            auto place = [&](bool topScreen) {
                const float scale = std::clamp(topScreen ? m_gameEntry.ndsTopScale : m_gameEntry.ndsBottomScale,
                                               1.0f, 10.0f);
                const float offsetX = topScreen ? m_gameEntry.ndsTopOffsetX : m_gameEntry.ndsBottomOffsetX;
                const float offsetY = topScreen ? m_gameEntry.ndsTopOffsetY : m_gameEntry.ndsBottomOffsetY;
                const float baseX = topScreen ? 224.0f : 800.0f;
                const float baseY = 264.0f;
                const int dstW = std::max(4, static_cast<int>(std::lround(256.0f * baseScale * scale / 4.0f)) * 4);
                const int dstH = std::max(3, (dstW * 3) / 4);
                const int dstX = static_cast<int>(std::lround(baseX + offsetX - (dstW - 256) * 0.5f));
                const int dstY = static_cast<int>(std::lround(baseY + offsetY - (dstH - 192) * 0.5f));
                blitScaledOriented(out, frame, _mapNdsSourceScreen(topScreen) ? 0u : screenH,
                                   dstX, dstY, dstW, dstH);
            };

            place(true);
            place(false);
            return out;
        }

        if (m_ndsLayout == "hybrid") {
            constexpr unsigned canvasW = 1280;
            constexpr unsigned canvasH = 720;
            out.width = canvasW;
            out.height = canvasH;
            out.pixels.assign(static_cast<size_t>(out.width) * out.height, 0xFF000000u);
            blitScaledOriented(out, frame, _mapNdsSourceScreen(true) ? 0u : screenH, 0, 40, 853, 640);
            blitScaledOriented(out, frame, _mapNdsSourceScreen(true) ? 0u : screenH, 853, 40, 427, 320);
            blitScaledOriented(out, frame, _mapNdsSourceScreen(false) ? 0u : screenH, 853, 360, 427, 320);
            return out;
        }

        if (m_ndsLayout == "priority_top") {
            constexpr unsigned canvasW = 1024;
            constexpr unsigned canvasH = 768;
            out.width = canvasW;
            out.height = canvasH;
            out.pixels.assign(static_cast<size_t>(out.width) * out.height, 0xFF000000u);

            const int topW = 768;
            const int topH = 576;
            const int topY = (static_cast<int>(canvasH) - topH) / 2;

            const int bottomW = static_cast<int>(canvasW) - topW;
            const int bottomH = (bottomW * 3) / 4;
            const int bottomX = topW;
            const int bottomY = (static_cast<int>(canvasH) - bottomH) / 2;

            blitScaledOriented(out, frame, _mapNdsSourceScreen(true) ? 0u : screenH,
                               0, topY, topW, topH);
            blitScaledOriented(out, frame, _mapNdsSourceScreen(false) ? 0u : screenH,
                               bottomX, bottomY, bottomW, bottomH);
            return out;
        }

        if (m_ndsLayout == "horizontal") {
            constexpr unsigned gap = 0u;
            out.width = screenW * 2;
            out.height = screenH;
            out.pixels.assign(static_cast<size_t>(out.width) * out.height, 0xFF000000u);
            for (unsigned y = 0; y < screenH; ++y) {
                const uint32_t* top = frame.pixels.data() +
                    (static_cast<size_t>(_mapNdsSourceScreen(true) ? y : screenH + y) * screenW);
                const uint32_t* bottom = frame.pixels.data() +
                    (static_cast<size_t>(_mapNdsSourceScreen(false) ? y : screenH + y) * screenW);
                uint32_t* dst = out.pixels.data() + static_cast<size_t>(y) * out.width;
                std::copy(top, top + screenW, dst);
                std::copy(bottom, bottom + screenW, dst + screenW + gap);
            }
            return out;
        }

        if (m_ndsLayout == "top" || m_ndsLayout == "bottom") {
            out.width = screenW;
            out.height = screenH;
            out.pixels.resize(static_cast<size_t>(screenW) * screenH);
            const bool sourceTop = _mapNdsSourceScreen(m_ndsLayout == "top");
            const size_t offset = sourceTop ? 0 : static_cast<size_t>(screenW) * screenH;
            std::copy(frame.pixels.data() + offset,
                      frame.pixels.data() + offset + out.pixels.size(),
                      out.pixels.data());
            return out;
        }

        return frame;
    }

    void MgbaGameView::_updateNdsTouchRect(const beiklive::DisplayRect& rect)
    {
        m_ndsTouchRect = {};
        if (m_gameEntry.platform != static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS))
            return;
        if (rect.w <= 0.f || rect.h <= 0.f)
            return;

        m_ndsTouchRect = rect;
    }

    void MgbaGameView::_waitForUiAudioPlayer()
    {
        auto* player = dynamic_cast<beiklive::BKAudioPlayer*>(
            brls::Application::getAudioPlayer());
        if (!player)
            return;

        constexpr std::chrono::milliseconds kAudioPlayerWaitTimeout{500};
        auto deadline = std::chrono::steady_clock::now() + kAudioPlayerWaitTimeout;
        while (player->isPlaying() &&
               std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void MgbaGameView::_initAudioForCore(double fps, double sampleRate)
    {
        if (fps <= 0.0) fps = 59.7;
        if (sampleRate <= 0.0) sampleRate = 32768.0;

        _waitForUiAudioPlayer();

        m_coreAudioOutput = coreAudioOutput(m_core);
        if (auto* output = m_coreAudioOutput) {
            brls::Logger::debug("[MgbaGameView] audio init: core-managed output sampleRate={:.0f}", sampleRate);
            beiklive::BKAudioPlayer::setGameAudioActive(true);
            const bool waitForFirstFrame = isMgbaNativePlatform(m_gameEntry.platform) &&
                                           !m_firstFrameUploaded.load(std::memory_order_acquire);
            output->SetAudioOutputEnabled(!waitForFirstFrame);
            output->SetAudioOutputSpeed(1.0f);
            m_audioSpeed = 1.0f;
            m_audioOutputSuppressed = waitForFirstFrame;
            m_loggedFirstAudioPush = false;
            m_audioEmptyLogCount = 0;
            return;
        }

        beiklive::BKAudioPlayer::setGameAudioActive(false);

        int targetMs = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_AUDIO_TARGET_LATENCY_MS, 90);
        int maxMs = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_AUDIO_MAX_LATENCY_MS, 180);
        targetMs = std::clamp(targetMs, 30, 300);
        maxMs = std::clamp(maxMs, targetMs + 20, 500);
        if (isNdsPlatform(m_gameEntry.platform)) {
            targetMs = std::max(targetMs, kNdsTargetLatencyFloorMs);
            maxMs = std::max(maxMs, kNdsMaxLatencyFloorMs);
            maxMs = std::clamp(maxMs, targetMs + 20, 500);
        }

        brls::Logger::debug("[MgbaGameView] audio init: fps={:.2f} sampleRate={:.0f} target={}ms max={}ms",
                            fps, sampleRate, targetMs, maxMs);
        if (!AudioManager::instance().init(static_cast<int>(sampleRate), 2))
            brls::Logger::warning("[MgbaGameView] audio init failed, continuing without audio output");
        AudioManager::instance().configureLatencyMs(targetMs, maxMs);
        AudioManager::instance().setSpeed(1.0f);
        m_audioSpeed = 1.0f;
        m_audioOutputSuppressed = false;
        m_loggedFirstAudioPush = false;
        m_audioEmptyLogCount = 0;

        std::vector<int16_t> initAudioDiscard;
        m_core->DrainAudio(initAudioDiscard);
    }

    void MgbaGameView::_flushAudioForTransition()
    {
        if (auto* output = m_coreAudioOutput) {
            output->FlushAudioOutput();
            return;
        }

        int fadeMs = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_AUDIO_TRANSITION_FADE_MS, 6);
        fadeMs = std::clamp(fadeMs, 0, 20);
        AudioManager::instance().flushRingBufferWithFade(fadeMs);
    }

    void MgbaGameView::_pauseAudioForTransition()
    {
        if (auto* output = m_coreAudioOutput) {
            output->SetAudioOutputEnabled(false);
            beiklive::BKAudioPlayer::setGameAudioActive(false);
            return;
        }

        AudioManager::instance().pauseOutput();
    }

    void MgbaGameView::_resumeAudioForTransition()
    {
        if (auto* output = m_coreAudioOutput) {
            _waitForUiAudioPlayer();
            beiklive::BKAudioPlayer::setGameAudioActive(true);
            output->SetAudioOutputEnabled(true);
            return;
        }

        int fadeMs = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_AUDIO_TRANSITION_FADE_MS, 6);
        fadeMs = std::clamp(fadeMs, 0, 20);
        AudioManager::instance().resumeOutputWithFade(fadeMs);
    }

    // ============================================================
    // _registerGameRuntime – 创建并初始化核心，启动游戏线程
    // ============================================================
    void MgbaGameView::_registerGameRuntime()
    {
        brls::Logger::debug("[MgbaGameView] _registerGameRuntime: platform={}", m_gameEntry.platform);
        m_firstFrameUploaded.store(false, std::memory_order_release);
        m_core = CreateEmulatorCore(m_gameEntry);
        m_coreAudioOutput = nullptr;
        if (!m_core) {
            brls::Logger::warning("[MgbaGameView] _registerGameRuntime: unsupported platform={}", m_gameEntry.platform);
            return;
        }

        _syncNdsVideoFrameMode();

        if (shouldSetupCoreOnGameThread(m_gameEntry.platform))
        {
            GameSignal::instance().resetAll();
            brls::Logger::debug("[MgbaGameView] deferring {} core setup to game thread...",
                                beiklive::tools::platformName(m_gameEntry.platform));
            _startGameThread();
            return;
        }

        _waitForUiAudioPlayer();
        if (m_core->SetupGame(m_gameEntry))
        {
            brls::Logger::debug("核心已初始化，平台={}, 路径={}",
                                beiklive::tools::platformName(m_gameEntry.platform),
                                m_gameEntry.path);
            double fps = m_core->Fps();
            double srate = m_core->SampleRate();
            _initAudioForCore(fps, srate);
            GameSignal::instance().resetAll();
            _initPlayTimeTracking();
            brls::Logger::debug("[MgbaGameView] starting game thread...");
            _startGameThread();
        }
        else
        {
            brls::Logger::error("核心初始化失败，平台={}, 路径={}",
                                beiklive::tools::platformName(m_gameEntry.platform),
                                m_gameEntry.path);
            delete m_core;
            m_core = nullptr;
        }
    }

    // ============================================================
    // _startGameThread / _stopGameThread
    // ============================================================
    void MgbaGameView::_startGameThread()
    {
        brls::Logger::debug("[MgbaGameView] _startGameThread");
        m_running.store(true, std::memory_order_release);
        m_gameThread = std::thread(&MgbaGameView::_gameLoop, this);
    }

    void MgbaGameView::_stopGameThread()
    {
        brls::Logger::debug("[MgbaGameView] _stopGameThread begin");
        m_running.store(false, std::memory_order_release);
        if (auto* stopRequest = dynamic_cast<beiklive::IEmulatorStopRequest*>(m_core))
            stopRequest->RequestStop();
        if (m_gameThread.joinable())
            m_gameThread.join();
        brls::Logger::debug("[MgbaGameView] _stopGameThread end");
    }

    // ============================================================
    // _saveRewindState – 序列化当前核心状态并存入倒带缓冲区
    // 支持间隔保存（每 m_rewindSaveInterval 帧保存一次）
    // 若 m_rewindShowUI 开启则同时捕获 RGB565 缩略图
    // ============================================================
    void MgbaGameView::_saveRewindState()
    {
        if (!m_rewindEnabled)
            return;

        // 间隔控制：每 m_rewindSaveInterval 帧才保存一次
        ++m_rewindFrameCounter;
        if (m_rewindFrameCounter < static_cast<unsigned>(m_rewindSaveInterval))
            return;
        m_rewindFrameCounter = 0;

        RewindFrame frame;
        if (!m_core->Serialize(frame.state) || frame.state.empty())
            return;

        // 若启用可视化倒带界面，则同时捕获并压缩缩略图
        if (m_rewindShowUI) {
            auto videoFrame = m_core->GetVideoFrame();
            if (!videoFrame.pixels.empty() && videoFrame.width > 0 && videoFrame.height > 0) {
                const auto [thumbW, thumbH] = rewindThumbSizeForFrame(videoFrame.width, videoFrame.height);
                frame.thumbW = thumbW;
                frame.thumbH = thumbH;
                frame.thumb = _downsampleToRGB565(
                    videoFrame.pixels, videoFrame.width, videoFrame.height,
                    thumbW, thumbH);
            }
        }

        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            m_rewindBuffer.push_front(std::move(frame));
            // 根据保存间隔计算实际最大条目数：
            // m_rewindBufferSize 表示"最多缓存多少帧游戏时间"（如 1800 = 60fps × 30s = 30秒）。
            // 每个条目覆盖 m_rewindSaveInterval 帧，因此最大条目数 = bufferSize / saveInterval。
            // 这样无论 saveInterval 取何值，实际缓冲时长始终等于 bufferSize/60 秒。
            // 使用 std::max(1u, ...) 避免 saveInterval 意外为 0 时的除零错误
            unsigned saveInterval = static_cast<unsigned>(std::max(1, m_rewindSaveInterval));
            unsigned maxEntries = std::max(1u, m_rewindBufferSize / saveInterval);
            while (m_rewindBuffer.size() > maxEntries)
                m_rewindBuffer.pop_back();
        }
    }

    // ============================================================
    // _stepRewind – 从倒带缓冲区弹出状态并恢复，返回是否成功
    // 优化：Unserialize（可能较慢）在锁外执行，减少临界区
    // ============================================================
    bool MgbaGameView::_stepRewind()
    {
        // 在锁内取出待恢复的状态副本，锁外执行反序列化
        std::vector<uint8_t> stateToRestore;
        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            if (m_rewindBuffer.empty()) return false;

            for (unsigned step = 0; step < REWIND_STEP && !m_rewindBuffer.empty(); ++step) {
                stateToRestore = std::move(m_rewindBuffer.front().state);
                m_rewindBuffer.pop_front();
                // 仅保留最后弹出的帧用于恢复（快退效果：弹出多帧后恢复最后一帧 = 快退 REWIND_STEP 帧）
            }
        }

        if (stateToRestore.empty()) return false;

        // 锁外执行反序列化（可能涉及内存分配、状态重建，较耗时）
        if (!m_core->Unserialize(stateToRestore)) {
            brls::Logger::warning("MgbaGameView: 倒带状态反序列化失败，丢弃该帧");
            return false;
        }

        // 运行一帧以刷新视频输出，保证倒带画面流畅
        m_core->RunFrame();
        return true;
    }

    // ============================================================
    // _stepFrame – 执行正常/快进帧，返回本次运行的帧数
    // ============================================================
    unsigned MgbaGameView::_stepFrame(bool ff)
    {
        if (!ff) {
            _saveRewindState();
            m_core->RunFrame();
            return 1u;
        }

        if (isNdsPlatform(m_gameEntry.platform) && m_ffMultiplier >= 1.0f) {
            _saveRewindState();
            m_core->RunFrame();
            return 1u;
        }

        if (m_ffMultiplier >= 1.0f) {
            unsigned integerPart = static_cast<unsigned>(m_ffMultiplier);
            float fracPart = m_ffMultiplier - static_cast<float>(integerPart);

            unsigned frames = integerPart;

            // 小数部分累加：累加器满 1.0 时多跑一帧
            m_ffSlowAccum += fracPart;
            if (m_ffSlowAccum >= 1.0f) {
                m_ffSlowAccum -= 1.0f;
                ++frames;
            }

            if (frames == 0) frames = 1u;
            for (unsigned i = 0; i < frames; ++i) {
                if (i == 0) _saveRewindState();
                m_core->RunFrame();
            }
            return frames;
        }

        // 慢动作：使用累加器，仅在累积满 1 帧时才执行一帧
        m_ffSlowAccum += m_ffMultiplier;
        if (m_ffSlowAccum >= 1.0f) {
            m_ffSlowAccum -= 1.0f;
            _saveRewindState();
            m_core->RunFrame();
            return 1u;
        }
        return 0u;
    }

    // ============================================================
    // _captureVideoFrame – 取出最新视频帧并暂存，等待 UI 线程上传
    // ============================================================
    void MgbaGameView::_captureVideoFrame()
    {
        {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            if (m_frameReady)
                return;
        }

        auto frame = m_core->GetVideoFrame();
        if (!frame.pixels.empty()) {
            std::lock_guard<std::mutex> lk(m_frameMutex);
            m_lastRawFrame = frame;
            m_hasLastRawFrame = true;
            m_pendingFrame = std::move(frame);
            m_frameReady   = true;
        }
    }

    // ============================================================
    // _pushFrameAudio – 推送音频数据（快进时限制推送量）
    // ============================================================
    void MgbaGameView::_pushFrameAudio(bool ff)
    {
        const bool rewindMuted = GameSignal::instance().isRewinding() &&
                                 GET_SETTING_KEY_INT("rewind.mute", 0) != 0;
        const bool isNds = isNdsPlatform(m_gameEntry.platform);
        const bool waitingFirstMgbaFrame = isMgbaNativePlatform(m_gameEntry.platform) &&
                                           !m_firstFrameUploaded.load(std::memory_order_acquire);
        const bool suppressAudio = GameSignal::instance().isMuted() ||
                                   (ff && m_ffMute && !isNds) ||
                                   rewindMuted ||
                                   waitingFirstMgbaFrame;

        if (auto* output = m_coreAudioOutput) {
            output->SetAudioOutputEnabled(!suppressAudio);
            m_audioOutputSuppressed = suppressAudio;
            return;
        }

        if (suppressAudio) {
            m_core->DrainAudio(m_audioDrainBuf);
            if (!m_audioOutputSuppressed)
                _flushAudioForTransition();
            m_audioOutputSuppressed = true;
            return;
        }

        if (m_audioOutputSuppressed) {
            _flushAudioForTransition();
            m_audioOutputSuppressed = false;
        }

        if (!m_core->DrainAudio(m_audioDrainBuf) || m_audioDrainBuf.empty()) {
            if (isMgbaNativePlatform(m_gameEntry.platform) && m_audioEmptyLogCount < 5) {
                ++m_audioEmptyLogCount;
                brls::Logger::debug("[MgbaGameView] mGBA audio drain empty count={}", m_audioEmptyLogCount);
            }
            return;
        }

        size_t frames = m_audioDrainBuf.size() / 2;
        if (isMgbaNativePlatform(m_gameEntry.platform) && !m_loggedFirstAudioPush) {
            brls::Logger::info("[MgbaGameView] first mGBA audio push frames={} ringBefore={} running={}",
                               frames,
                               AudioManager::instance().available(),
                               AudioManager::instance().isRunning() ? 1 : 0);
            m_loggedFirstAudioPush = true;
        }

        if (ff || isNdsPlatform(m_gameEntry.platform)) {
            AudioManager::instance().pushSamplesNoBlocking(m_audioDrainBuf.data(), frames);
            return;
        }

        AudioManager::instance().pushSamples(m_audioDrainBuf.data(), frames);
    }

    // ============================================================
    // _updateFpsStats – 更新 FPS 统计（游戏线程侧）
    // ============================================================
    void MgbaGameView::_updateFpsStats(unsigned framesRan,
                                   std::chrono::steady_clock::time_point& lastTime,
                                   unsigned& counter)
    {
        using Clock = std::chrono::steady_clock;

        counter += framesRan;
        auto now    = Clock::now();
        double elap = std::chrono::duration<double>(now - lastTime).count();
        if (elap >= FPS_UPDATE_INTERVAL && elap > 0.0) {
            float fps = static_cast<float>(counter / elap);
            {
                std::lock_guard<std::mutex> lk(m_fpsMutex);
                m_currentFps = fps;
            }
            counter  = 0;
            lastTime = now;
        }
    }

    // ============================================================
    // _accumulatePlayTime – 按真实运行时间累计时长
    // ============================================================
    void MgbaGameView::_accumulatePlayTime()
    {
        auto now = std::chrono::steady_clock::now();
        if (m_playStartTime.time_since_epoch().count() == 0) {
            m_playStartTime = now;
            return;
        }

        double elapsed = std::chrono::duration<double>(now - m_playStartTime).count();
        m_playStartTime = now;
        if (elapsed <= 0.0)
            return;
        if (elapsed > PLAYTIME_SUSPEND_GAP_SEC) {
            brls::Logger::info("MgbaGameView: ignored playtime gap {:.2f}s", elapsed);
            return;
        }

        m_playTimeFraction += elapsed;
        if (m_playTimeFraction < 1.0)
            return;

        int wholeSeconds = static_cast<int>(m_playTimeFraction);
        m_gameEntry.playTime += wholeSeconds;
        m_playTimeFraction -= static_cast<double>(wholeSeconds);

        if (!m_playTimeTempPath.empty() && m_lastPlayTimeTempWrite != m_gameEntry.playTime) {
            std::ofstream f(m_playTimeTempPath, std::ios::trunc);
            if (f) {
                f << m_gameEntry.playTime;
                f.close();
                m_lastPlayTimeTempWrite = m_gameEntry.playTime;
            }
        }
    }

    // ============================================================
    // _savePlayTimeCheckpoint – 计时累加到 playTime 并写入临时文件
    // ============================================================
    void MgbaGameView::_savePlayTimeCheckpoint()
    {
        _accumulatePlayTime();

        if (!m_playTimeTempPath.empty()) {
            std::ofstream f(m_playTimeTempPath, std::ios::trunc);
            if (f) {
                f << m_gameEntry.playTime;
                f.close();
                m_lastPlayTimeTempWrite = m_gameEntry.playTime;
            }
        }
    }

    // ============================================================
    // _saveAndCommitPlayTime – 累加剩余时长并提交到 GameDB
    // ============================================================
    void MgbaGameView::_saveAndCommitPlayTime()
    {
        if (m_playTimeTempPath.empty()) return;

        _savePlayTimeCheckpoint();

        if (beiklive::GameDB && m_gameEntry.playTime > 0) {
            // beiklive::GameDB->upsert(m_gameEntry);
            beiklive::GameDB->set(m_gameEntry.path, "playTime", nlohmann::json(m_gameEntry.playTime));
            beiklive::GameDB->flush();
        }
        std::error_code ec;
        std::filesystem::remove(m_playTimeTempPath, ec);
    }

    // ============================================================
    // _initPlayTimeTracking – 启动时检查遗留临时文件并合并到 GameDB
    // ============================================================
    void MgbaGameView::_initPlayTimeTracking()
    {
        namespace fs = std::filesystem;

        // 确定存档目录（与 getStatePath 逻辑一致）
        std::string dir = m_gameEntry.savePath;
        if (dir.empty()) dir = beiklive::path::savePath();

        // 提取 ROM 文件名（不含扩展名）
        std::string stem;
        if (!m_gameEntry.path.empty())
            stem = fs::path(m_gameEntry.path).stem().string();
        else
            stem = "game";

        std::error_code ec;
        fs::create_directories(dir, ec);

        m_playTimeTempPath = dir + "/" + stem + ".playtime";
        m_lastPlayTimeTempWrite = -1;

        // 检查是否存在遗留的临时文件（上次异常退出或未正常终止）
        if (fs::exists(m_playTimeTempPath, ec) && !ec) {
            try {
                std::ifstream f(m_playTimeTempPath);
                if (f) {
                    long long legacySeconds = 0;
                    f >> legacySeconds;
                    f.close();
                    if (legacySeconds > 0) {
                        // 取遗留值和当前 GameEntry 中保存值的最大值，避免数据倒退
                        m_gameEntry.playTime = std::max(m_gameEntry.playTime, static_cast<int>(legacySeconds));
                        if (beiklive::GameDB) {
                            beiklive::GameDB->upsertByPath(m_gameEntry);
                            beiklive::GameDB->flush();
                            brls::Logger::info("MgbaGameView: 已合并遗留时长 {} 秒到 GameDB，清理临时文件",
                                               legacySeconds);
                        }
                        // 合并后删除临时文件，由热路径重新创建
                        fs::remove(m_playTimeTempPath, ec);
                    }
                }
            } catch (...) {
                brls::Logger::warning("MgbaGameView: 读取遗留时长临时文件失败");
            }
        }
    }

    // ============================================================
    // _throttleFrameRate – 帧率限制器
    //
    // 使用 nextTarget 累加模式（而非每帧重新取 Clock::now()）：
    //   - nextTarget 每帧递增一个 frameDurNs，避免睡眠超时引发的帧率漂移；
    //   - 若某帧耗时超过目标时间（nextTarget 落在过去），直接重置到 now，不补偿；
    //   - 快进状态同样按倍率后的目标时间限速，避免倍率被“多跑帧 + 不等待”
    //     双重放大。
    // ============================================================
    void MgbaGameView::_throttleFrameRate(bool /*ff*/,
                                      std::chrono::steady_clock::time_point& nextTarget,
                                      std::chrono::nanoseconds frameDurNs,
                                      std::chrono::nanoseconds spinGuardNs)
    {
        using Clock = std::chrono::steady_clock;

        nextTarget += frameDurNs;

        auto now = Clock::now();
        if (nextTarget < now) {
            nextTarget = now;
            return;
        }

        auto coarse = nextTarget - now - spinGuardNs;
        if (coarse.count() > 0)
            std::this_thread::sleep_for(coarse);

#ifdef __SWITCH__
        {
            auto remaining = std::chrono::duration_cast<std::chrono::nanoseconds>(nextTarget - Clock::now());
            if (remaining.count() > 0)
                svcSleepThread(static_cast<s64>(remaining.count()));
            return;
        }
#endif
        while (Clock::now() < nextTarget)
            std::this_thread::yield();
    }

    // ============================================================
    // _gameLoop – 游戏主循环（独立线程）
    //
    // 按核心目标帧率执行游戏逻辑，支持：
    //   - 暂停：跳过帧执行
    //   - 倒带：从缓冲区恢复历史状态
    //   - 快进：每迭代运行 FF_MULTIPLIER 帧
    //   - 帧率控制：nextFrameTarget 累加模式，严格对齐目标帧率
    // ============================================================
    void MgbaGameView::_gameLoop()
    {
        using Clock = std::chrono::steady_clock;

#ifdef __SWITCH__
        if (isMgbaNativePlatform(m_gameEntry.platform))
        {
            Result rc = svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
            if (R_FAILED(rc))
                brls::Logger::warning("MgbaGameView: failed to pin mGBA game thread to core1 rc={:#x}", rc);
            else
                brls::Logger::info("MgbaGameView: mGBA game thread pinned to core1");
        }
#endif

        if (!m_core) return;

        brls::Logger::debug("[MgbaGameView] _gameLoop enter");
        const bool isNds = isNdsPlatform(m_gameEntry.platform);
        const bool setupOnGameThread = shouldSetupCoreOnGameThread(m_gameEntry.platform);

#ifdef _WIN32
        timeBeginPeriod(1); // 提升 Windows 定时器精度至 1ms
#endif

        if (setupOnGameThread && !m_core->IsReady())
        {
#ifdef __SWITCH__
            if (!isMgbaNativePlatform(m_gameEntry.platform))
                svcSetThreadCoreMask(CUR_THREAD_HANDLE, 1, 1ULL << 1);
#endif

            brls::Logger::info("MgbaGameView: initializing {} core on game thread",
                               beiklive::tools::platformName(m_gameEntry.platform));
            _waitForUiAudioPlayer();
            if (!m_core->SetupGame(m_gameEntry))
            {
                brls::Logger::error("核心初始化失败，平台={}, 路径={}",
                                    beiklive::tools::platformName(m_gameEntry.platform),
                                    m_gameEntry.path);
                m_running.store(false, std::memory_order_release);
                return;
            }

            brls::Logger::debug("核心已初始化，平台={}, 路径={}",
                                beiklive::tools::platformName(m_gameEntry.platform),
                                m_gameEntry.path);
            double fps = m_core->Fps();
            double srate = m_core->SampleRate();
            _captureVideoFrame();
            _initAudioForCore(fps, srate);
            _initPlayTimeTracking();
        }

        if (!m_core->IsReady()) return;

        // 从核心获取目标帧率
        double coreFps = m_core->Fps();
        if (coreFps <= 0.0 || coreFps > MAX_REASONABLE_FPS)
            coreFps = 59.7275;

        // 预计算帧时长（nanoseconds 避免浮点精度损失）
        const auto baseFrameDurNs = std::chrono::nanoseconds(
            static_cast<long long>(1e9 / coreFps));
        auto frameDurNs         = baseFrameDurNs;
        const auto spinGuardNs  = std::chrono::nanoseconds(
            static_cast<long long>(std::min(SPIN_GUARD_SEC, 1.0 / coreFps * 0.1) * 1e9));

        // 帧率限制：nextFrameTarget 累加目标时间点
        auto nextFrameTarget = Clock::now();

        // FPS 统计
        auto     fpsLastTime  = Clock::now();
        unsigned fpsCount     = 0u;

        GameTimer::instance().start();

        brls::Logger::info("MgbaGameView: 游戏循环开始 playTime={} coreFps={:.2f}",
                           m_gameEntry.playTime, coreFps);

        m_playStartTime = Clock::now();
        m_playTimeFraction = 0.0;
        m_lastPlayTimeTempWrite = -1;
        bool wasPaused  = false;

        // 初始化 SRAM 检测时间
        m_sramLastCheck = Clock::now();

        // ── 读取自动存档配置 ──
        int autoLoadSlot = GET_SETTING_KEY_INT("save.autoLoadState0", 0);
        int autoSaveSlot = GET_SETTING_KEY_INT("save.autoSaveState", 0);
        int autoSaveSecs = GET_SETTING_KEY_INT("save.autoSaveInterval", 0);
        m_autoSaveTimer = Clock::now();
        bool autoLoadDone = false;

        // 读取快进倍率配置
        m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
        if (m_ffMultiplier <= 0.0f) m_ffMultiplier = 1.0f;
        m_ffSlowAccum = 0.0f;
        m_ffMute = GET_SETTING_KEY_INT("fastforward.mute", 1) != 0;

        // 读取连发速率
        {
            float turboHz = GET_SETTING_KEY_FLOAT("turbo.rate", 10.0f);
            if (turboHz < 1.0f) turboHz = 1.0f;
            if (turboHz > 30.0f) turboHz = 30.0f;
            m_turboToggleInterval = std::max(1, static_cast<int>(coreFps / (turboHz * 2.0f)));
        }


        // 读取shader开关配置
        // _onShaderToggle(GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, 0));

        auto processCheatSignals = [this](GameSignal& sig, bool paused) {
            if (!m_core)
                return;

            auto pathReq = sig.consumeCheatPathUpdate();
            if (pathReq.pending)
            {
                brls::Logger::info("MgbaGameView: consume cheat path update paused={} path={}",
                                   paused, pathReq.path);
                m_gameEntry.cheatPath = pathReq.path;
                m_core->SetCheatPath(pathReq.path);
            }

            auto applyReq = sig.consumeApplyCheats();
            if (applyReq.pending)
            {
                size_t enabled = 0;
                for (const auto& cheat : applyReq.cheats)
                {
                    if (cheat.enabled)
                        ++enabled;
                }
                brls::Logger::info("MgbaGameView: consume cheat list paused={} entries={} enabled={}",
                                   paused, applyReq.cheats.size(), enabled);
                m_core->ApplyCheats(applyReq.cheats);
            }
        };


        while (m_running.load(std::memory_order_acquire))
        {
#ifdef __SWITCH__
            _updateSwitchFocusState();
#endif
            auto& sig = GameSignal::instance();

            // ---- 自动加载即时存档 ----
            if (!sig.isPaused() && !autoLoadDone && autoLoadSlot > 0 && m_core && m_core->IsReady()) {
                if (stateExists(autoLoadSlot - 1)) {
                    _doLoadState(autoLoadSlot - 1);
                    brls::Logger::info("MgbaGameView: 自动加载存档槽 {}", autoLoadSlot - 1);
                }
                autoLoadDone = true;
            }

            // ---- 自动保存即时存档 ----
            if (!sig.isPaused() && !wasPaused && autoSaveSlot > 0 && autoSaveSecs > 0) {
                auto now = Clock::now();
                double sinceSave = std::chrono::duration<double>(now - m_autoSaveTimer).count();
                if (sinceSave >= static_cast<double>(autoSaveSecs)) {
                    _doSaveState(autoSaveSlot - 1);
                    m_autoSaveTimer = now;
                    brls::Logger::debug("MgbaGameView: 自动保存存档槽 {} (间隔 {}s)", autoSaveSlot - 1, autoSaveSecs);
                }
            }

            // ---- 暂停处理 ----
            if (sig.isPaused()) {
                if (!wasPaused) {
                    _savePlayTimeCheckpoint();
                    m_playStartTime = {};
                    if (m_sramDirty && m_core && m_core->IsReady())
                        m_core->saveSram();
                    _pauseAudioForTransition(); // 暂停期间停止继续提交硬件音频缓冲
                    wasPaused = true;
                }
                bool hasAnyFrame = false;
                {
                    std::lock_guard<std::mutex> lk(m_frameMutex);
                    hasAnyFrame = m_hasLastRawFrame || m_frameReady;
                }
                if (!hasAnyFrame)
                    _captureVideoFrame();
                // 暂停菜单中切换/编辑金手指时，也要及时同步到核心。
                processCheatSignals(sig, true);
                // 暂停时允许截图，便于在菜单暂停后保存当前画面。
                if (sig.consumeScreenshot())
                    _doScreenshot();
                // 暂停时仍可消费退出自动存档信号
                {
                    int exitSaveSlot = sig.consumeAutoSave();
                    if (exitSaveSlot >= 0) {
                        _doSaveState(exitSaveSlot);
                        sig.markAutoSaveDone();
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                auto now     = Clock::now();
                nextFrameTarget = now;
                fpsLastTime     = now;
                continue;
            }
            if (wasPaused) {
                m_playStartTime = Clock::now();
                m_autoSaveTimer = Clock::now();
                _resumeAudioForTransition();
                wasPaused = false;
            }

            // ---- 重置请求 ----
            if (sig.consumeReset()) {
                m_core->Reset();
                _flushAudioForTransition();
                std::lock_guard<std::mutex> lk(m_rewindMutex);
                m_rewindBuffer.clear(); // 重置后清空倒带缓冲区
            }

            // ---- 快速存档 ----
            int saveSlot = sig.consumeQuickSave();
            if (saveSlot >= 0)
                _doSaveState(saveSlot);

            // ---- 快速读档 ----
            int loadSlot = sig.consumeQuickLoad();
            if (loadSlot >= 0)
                _doLoadState(loadSlot);

            // ---- 截图 ----
            if (sig.consumeScreenshot())
                _doScreenshot();

            // ---- 倒带帧恢复（由可视化倒带UI触发）----
            int restoreIdx = sig.consumeRewindRestore();
            if (restoreIdx >= 0 && !isNds) {
                std::lock_guard<std::mutex> lk(m_rewindMutex);
                if (restoreIdx < static_cast<int>(m_rewindBuffer.size())) {
                    if (!m_core->Unserialize(m_rewindBuffer[restoreIdx].state)) {
                        brls::Logger::warning("MgbaGameView: 倒带帧恢复失败 idx={}", restoreIdx);
                    } else {
                        for (int i = 0; i < restoreIdx && !m_rewindBuffer.empty(); ++i)
                            m_rewindBuffer.pop_front();
                        m_core->RunFrame();
                    }
                }
            }

            // ---- 金手指更新 ----
            processCheatSignals(sig, false);

            // ---- 核心配置刷新 ----
            if (sig.consumeConfigUpdate() && m_core) {
                m_core->NotifyConfigUpdated();
                _syncNdsVideoFrameMode();
            }

            // ---- 退出自动存档 ----
            {
                int exitSaveSlot = sig.consumeAutoSave();
                if (exitSaveSlot >= 0) {
                    _doSaveState(exitSaveSlot);
                    sig.markAutoSaveDone();
                }
            }

            // ---- 从输入管理器更新游戏按键状态 ----
            GameInputManager::instance().setActivePlatform(m_gameEntry.platform);
            GameInputManager::instance().refreshPlayerInputStatesForPlatform(m_gameEntry.platform);
            GameSignal::instance().setGameButtonMask(
                0, GameInputManager::instance().getPlayerInputState(0).buttonMask);
            GameSignal::instance().setGameButtonMask(
                1, GameInputManager::instance().getPlayerInputState(1).buttonMask);

            // 先处理连发（Turbo）状态
            m_turboFrameCount++;
            if (m_turboFrameCount >= m_turboToggleInterval) {
                m_turboFrameCount = 0;
                if (m_turboAheld.load(std::memory_order_acquire)) {
                    m_turboAon = !m_turboAon;
                    if (m_turboAon)
                        GameSignal::instance().pressGameButton(8); // RETRO_DEVICE_ID_JOYPAD_A
                    else
                        GameSignal::instance().releaseGameButton(8);
                }
                if (m_turboBheld.load(std::memory_order_acquire)) {
                    m_turboBon = !m_turboBon;
                    if (m_turboBon)
                        GameSignal::instance().pressGameButton(0); // RETRO_DEVICE_ID_JOYPAD_B
                    else
                        GameSignal::instance().releaseGameButton(0);
                }
            }
            // 连发键释放时确保对应按键松开
            if (!m_turboAheld.load(std::memory_order_acquire) && m_turboAon) {
                m_turboAon = false;
                GameSignal::instance().releaseGameButton(8);
            }
            if (!m_turboBheld.load(std::memory_order_acquire) && m_turboBon) {
                m_turboBon = false;
                GameSignal::instance().releaseGameButton(0);
            }
            m_core->SetButtonsFromSignal(0);
            m_core->SetButtonsFromSignal(1);

            // ---- 决定本帧行为 ----
            bool ff      = sig.isFastForward();
            bool rew     = isNds ? false : sig.isRewinding();
            if (isNds && sig.isRewinding())
                sig.requestRewind(false);

            // 倒带时禁用快进，防止逻辑冲突
            if (rew) ff = false;

            // 每帧读取快进倍率（支持菜单中实时调整）
            m_ffMultiplier = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
            if (m_ffMultiplier <= 0.0f)
                m_ffMultiplier = 1.0f;
            else if (m_ffMultiplier < 0.1f)
                m_ffMultiplier = 0.1f;
            m_ffMute = GET_SETTING_KEY_INT("fastforward.mute", 1) != 0;

            const bool fastForwardActive = ff && m_ffMultiplier >= 1.0f;

            // 通知核心当前快进状态（供 RETRO_ENVIRONMENT_GET_FASTFORWARDING 查询）。
            // 慢动作同样由快进热键触发，但不能让核心进入快进模式。
            m_core->SetFastForwarding(fastForwardActive);

            // 同步倍速到音频重采样器
            if (auto* output = m_coreAudioOutput) {
                float curSpeed = (fastForwardActive && !m_ffMute && !isNds) ? m_ffMultiplier : 1.0f;
                if (curSpeed != m_audioSpeed) {
                    output->SetAudioOutputSpeed(curSpeed);
                    m_audioSpeed = curSpeed;
                }
            } else {
                float curSpeed = (ff && !m_ffMute && !isNds) ? m_ffMultiplier : 1.0f;
                if (curSpeed != m_audioSpeed) {
                    AudioManager::instance().setSpeed(curSpeed);
                    m_audioSpeed = curSpeed;
                }
            }

            unsigned framesRan = 1u;

            if (rew) {
                // 倒带：从历史缓冲区恢复状态
                _stepRewind();
            } else {
                // 正常 / 快进：运行核心并保存倒带状态
                framesRan = _stepFrame(ff);
            }

            // ---- 取出视频帧暂存（慢动作跳过帧时不捕获）----
            if (framesRan > 0)
                _captureVideoFrame();

            // ---- 推送音频（慢动作跳过帧时不推送）----
            if (framesRan > 0)
                _pushFrameAudio(ff);

            // ---- FPS 统计（慢动作跳过帧时仍计入时间）----
            _updateFpsStats(framesRan, fpsLastTime, fpsCount);

            // ---- 游玩时长统计（按真实运行时间累计，忽略暂停）----
            if (framesRan > 0)
                _accumulatePlayTime();

            // ---- SRAM 自动落盘检测 ----
            _checkAndAutoSaveSram();

            // ── 音频 PLL：围绕目标缓冲量做轻微双向修正 ──
            if (framesRan > 0 && !ff) {
                const size_t ringFill = AudioManager::instance().available();
                const double targetFill = static_cast<double>(AudioManager::instance().targetLatencySamples());
                float gain = GET_SETTING_KEY_FLOAT(beiklive::SettingKey::KEY_AUDIO_SYNC_STRENGTH, 0.015f);
                if (isNds)
                    gain = std::min(gain, kNdsMaxAudioSyncStrength);
                gain = std::clamp(gain, 0.0f, 0.05f);
                if (targetFill > 0.0 && gain > 0.0f) {
                    double errorRatio = (static_cast<double>(ringFill) - targetFill) / targetFill;
                    double correction = 1.0 + errorRatio * static_cast<double>(gain);
                    correction = isNds
                        ? std::clamp(correction, kNdsMinAudioCorrection, kNdsMaxAudioCorrection)
                        : std::clamp(correction, 0.98, 1.02);
                    frameDurNs = std::chrono::nanoseconds(
                        static_cast<long long>(baseFrameDurNs.count() * correction));
                }
            } else {
                if (fastForwardActive && framesRan > 0) {
                    const double frameScale = static_cast<double>(framesRan) /
                                              static_cast<double>(m_ffMultiplier);
                    frameDurNs = std::chrono::nanoseconds(
                        static_cast<long long>(baseFrameDurNs.count() * frameScale));
                } else {
                    frameDurNs = baseFrameDurNs;
                }
            }

            // ---- 帧率限制（倍率决定间隔；慢动作与快进都走节流）----
            _throttleFrameRate(fastForwardActive, nextFrameTarget, frameDurNs, spinGuardNs);
        }

        // ---- 提交时长记录 ----
        _saveAndCommitPlayTime();

        // ---- 强制保存 SRAM ----
        if (m_core && m_core->IsReady()) {
            m_core->saveSram();
            brls::Logger::info("MgbaGameView: SRAM saved on exit");
        }
        brls::Logger::info("MgbaGameView: 游戏循环结束 playTime={}",
                           m_gameEntry.playTime);

        // ---- 音频清理 ----
        AudioManager::instance().deinit();
        beiklive::BKAudioPlayer::setGameAudioActive(false);

#ifdef _WIN32
        timeEndPeriod(1);
#endif
        brls::Logger::debug("[MgbaGameView] _gameLoop exit");
    }

    // ============================================================
    // 即时存档路径计算
    //
    // 存档文件命名：{romStem}.ss{slot}  (slot=0 为自动存档)
    // 缩略图：       {statePath}.png
    // 存档目录优先级：GameEntry.savePath → 全局 saves 目录
    // ============================================================

    std::string MgbaGameView::getStatePath(int slot) const
    {
        std::string dir = m_gameEntry.savePath;
        if (dir.empty()) dir = beiklive::path::savePath();
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return beiklive::tools::getStatePath(dir, m_gameEntry.path, slot);
    }

    std::string MgbaGameView::getStateThumbPath(int slot) const
    {
        return beiklive::tools::getStateThumbPath(
            m_gameEntry.savePath.empty() ? beiklive::path::savePath() : m_gameEntry.savePath,
            m_gameEntry.path, slot);
    }

    bool MgbaGameView::stateExists(int slot) const
    {
        std::string dir = m_gameEntry.savePath;
        if (dir.empty()) dir = beiklive::path::savePath();
        return beiklive::tools::stateExists(dir, m_gameEntry.path, slot);
    }

    // ============================================================
    // _doSaveState – 序列化核心状态并保存截图（游戏线程调用）
    // ============================================================

    void MgbaGameView::_doSaveState(int slot)
    {
        if (!m_core || !m_core->IsReady()) return;

        std::vector<uint8_t> buf;
        if (!m_core->Serialize(buf) || buf.empty()) {
            brls::Logger::warning("MgbaGameView: 存档序列化失败 (slot {})", slot);
            brls::sync([slot](){
                brls::Application::notify("存档失败 (slot " + std::to_string(slot) + ")");
            });
            return;
        }

        std::string path = getStatePath(slot);
        std::ofstream f(path, std::ios::binary | std::ios::trunc);
        if (!f) {
            brls::Logger::warning("MgbaGameView: 无法打开存档文件写入: {}", path);
            brls::sync([slot](){
                brls::Application::notify("存档失败：无法写入文件 (slot " + std::to_string(slot) + ")");
            });
            return;
        }
        f.write(reinterpret_cast<const char*>(buf.data()),
                static_cast<std::streamsize>(buf.size()));
        f.close();

        brls::Logger::info("MgbaGameView: 已保存到 {} ({} bytes)", path, buf.size());
        if (m_core->saveSram())
            m_sramDirty = false;
        else
            brls::Logger::warning("MgbaGameView: 即时存档后刷新 SRAM 失败");

        // 保存缩略图
        auto frame = m_core->GetVideoFrame();
        if (!frame.pixels.empty() && frame.width > 0 && frame.height > 0) {
            std::string thumbPath = getStateThumbPath(slot);
            stbi_write_png(thumbPath.c_str(),
                           static_cast<int>(frame.width),
                           static_cast<int>(frame.height),
                           4,   // RGBA
                           frame.pixels.data(),
                           static_cast<int>(frame.width * 4));
        }

        // UI 线程通知
        brls::sync([slot](){
            std::string msg = (slot == 0) ? "已保存到自动存档" : "已保存到槽位 " + std::to_string(slot);
            brls::Application::notify(msg);
        });
    }

    // ============================================================
    // _doScreenshot – 保存当前画面截图（游戏线程调用）
    // ============================================================

    void MgbaGameView::_doScreenshot()
    {
        if (!m_core || !m_core->IsReady()) return;

        auto frame = m_core->GetVideoFrame();
        if (frame.pixels.empty() || frame.width == 0 || frame.height == 0) {
            brls::sync([]() {
                brls::Application::notify("截图失败：没有可用画面");
            });
            return;
        }

        std::string dir = m_gameEntry.savePath;
        if (dir.empty()) dir = beiklive::path::savePath();

        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        if (ec) {
            brls::Logger::warning("MgbaGameView: 创建截图目录失败: {} ({})", dir, ec.message());
            brls::sync([]() {
                brls::Application::notify("截图失败：无法创建存档目录");
            });
            return;
        }

        std::string timestamp = beiklive::tools::getTimestampString();
        std::replace(timestamp.begin(), timestamp.end(), ' ', '_');
        std::filesystem::path outPath = std::filesystem::path(dir) / ("screenshot_" + timestamp + ".png");
        for (int suffix = 1; std::filesystem::exists(outPath, ec) && suffix < 1000; ++suffix) {
            ec.clear();
            outPath = std::filesystem::path(dir) /
                ("screenshot_" + timestamp + "_" + std::to_string(suffix) + ".png");
        }

        const int ok = stbi_write_png(outPath.string().c_str(),
                                      static_cast<int>(frame.width),
                                      static_cast<int>(frame.height),
                                      4,
                                      frame.pixels.data(),
                                      static_cast<int>(frame.width * 4));
        if (!ok) {
            brls::Logger::warning("MgbaGameView: 截图保存失败: {}", outPath.string());
            brls::sync([]() {
                brls::Application::notify("截图保存失败");
            });
            return;
        }

        brls::Logger::info("MgbaGameView: 截图已保存: {}", outPath.string());
        brls::sync([]() {
            brls::Application::notify("截图已保存到存档目录");
        });
    }

    // ============================================================
    // _doLoadState – 从文件反序列化核心状态（游戏线程调用）
    // ============================================================

    void MgbaGameView::_doLoadState(int slot)
    {
        if (!m_core || !m_core->IsReady()) return;

        std::string path = getStatePath(slot);
        if (!std::filesystem::exists(path)) {
            brls::Logger::warning("MgbaGameView: 存档文件不存在: {}", path);
            brls::sync([slot](){
                brls::Application::notify("读取失败：槽位 " + std::to_string(slot) + " 无存档");
            });
            return;
        }

        std::ifstream f(path, std::ios::binary);
        if (!f) {
            brls::Logger::warning("MgbaGameView: 无法打开存档文件读取: {}", path);
            brls::sync([slot](){
                brls::Application::notify("读取失败：无法读取文件 (slot " + std::to_string(slot) + ")");
            });
            return;
        }

        f.seekg(0, std::ios::end);
        std::streampos fileSize = f.tellg();
        f.seekg(0, std::ios::beg);
        if (fileSize <= 0) {
            brls::sync([slot](){
                brls::Application::notify("读取失败：存档文件为空 (slot " + std::to_string(slot) + ")");
            });
            return;
        }

        std::vector<uint8_t> buf(static_cast<size_t>(fileSize));
        f.read(reinterpret_cast<char*>(buf.data()), fileSize);
        std::streamsize got = f.gcount();
        f.close();

        // 先用当前核心状态大小做一次轻量兼容性探测，避免把旧核心或损坏的状态
        // 直接喂给新核心，导致反序列化失败后内部状态被部分污染。
        // {
        //     std::vector<uint8_t> probe;
        //     if (m_core->Serialize(probe) && !probe.empty() &&
        //         probe.size() != static_cast<size_t>(got)) {
        //         brls::Logger::warning(
        //             "MgbaGameView: 存档大小与当前核心不匹配 (slot {} file={} expected={})",
        //             slot, static_cast<size_t>(got), probe.size());
        //         m_core->Reset();
        //         AudioManager::instance().flushRingBuffer();
        //         brls::sync([slot](){
        //             std::string msg = (slot == 0)
        //                 ? "读取失败：自动存档与当前核心不兼容，已重置游戏"
        //                 : "读取失败：槽位 " + std::to_string(slot) + " 与当前核心不兼容";
        //             brls::Application::notify(msg);
        //         });
        //         return;
        //     }
        // }

        if (!m_core->Unserialize(buf)) {
            brls::Logger::warning("MgbaGameView: 存档反序列化失败 (slot {})", slot);
            _flushAudioForTransition();
            brls::sync([slot](){
                std::string msg = (slot == 0)
                    ? "读取失败：自动存档无效或与当前 BIOS 设置不兼容"
                    : "读取失败：槽位 " + std::to_string(slot) + " 无效或与当前 BIOS 设置不兼容";
                brls::Application::notify(msg);
            });
            return;
        }
        _flushAudioForTransition();

        // 读档后清空倒带缓冲区，避免时序混乱
        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            m_rewindBuffer.clear();
        }

        brls::Logger::info("MgbaGameView: 已从 {} 读取状态 ({} bytes)", path, got);
        brls::sync([slot](){
            std::string msg = (slot == 0) ? "已从自动存档读取" : "已从槽位 " + std::to_string(slot) + " 读取";
            brls::Application::notify(msg);
        });
    }

    // ============================================================
    // snapshotRewindThumbs – 获取倒带缓冲区缩略图快照（UI 线程调用）
    // 游戏已暂停时调用，自动根据保存间隔计算 item 数量（每秒 1 个 item）
    // ============================================================
    std::vector<RewindThumbSnapshot>
    MgbaGameView::snapshotRewindThumbs() const
    {
        std::lock_guard<std::mutex> lk(m_rewindMutex);
        std::vector<RewindThumbSnapshot> result;

        int total = static_cast<int>(m_rewindBuffer.size());
        if (total == 0) return result;

        // 根据保存间隔自动计算每秒对应 1 个 item 时所需 item 数量（上限 120）
        // 公式：每条目代表 saveInterval 帧，60帧约1秒；缓冲总时长(秒) = total*saveInterval/60
        // 当缓冲时长小于 1 秒时，clamp 为 1（至少显示最新帧）
        int maxItems = std::max(1, std::min(120,
            total * m_rewindSaveInterval / 60));

        if (maxItems <= 0 || total <= maxItems) {
            // 条目数不超过限制，全部返回
            result.reserve(total);
            for (int i = 0; i < total; ++i) {
                RewindThumbSnapshot snap;
                snap.bufferIdx  = i;
                snap.secondsAgo = i * m_rewindSaveInterval / 60;
                snap.thumb      = m_rewindBuffer[i].thumb;
                snap.thumbW     = m_rewindBuffer[i].thumbW;
                snap.thumbH     = m_rewindBuffer[i].thumbH;
                result.push_back(std::move(snap));
            }
        } else if (maxItems == 1) {
            // 只取最新帧
            RewindThumbSnapshot snap;
            snap.bufferIdx  = 0;
            snap.secondsAgo = 0;
            snap.thumb      = m_rewindBuffer[0].thumb;
            snap.thumbW     = m_rewindBuffer[0].thumbW;
            snap.thumbH     = m_rewindBuffer[0].thumbH;
            result.push_back(std::move(snap));
        } else {
            // 均匀采样：在 [0, total-1] 范围内选取 maxItems 个索引（maxItems >= 2，不会除零）
            result.reserve(maxItems);
            for (int k = 0; k < maxItems; ++k) {
                int idx = k * (total - 1) / (maxItems - 1);
                RewindThumbSnapshot snap;
                snap.bufferIdx  = idx;
                snap.secondsAgo = idx * m_rewindSaveInterval / 60;
                snap.thumb      = m_rewindBuffer[idx].thumb;
                snap.thumbW     = m_rewindBuffer[idx].thumbW;
                snap.thumbH     = m_rewindBuffer[idx].thumbH;
                result.push_back(std::move(snap));
            }
        }

        // 反转顺序：使最旧的帧排在最前，最新的帧排在最后
        // 显示时最旧帧在左侧，最新帧在右侧，焦点默认放在最右边（最新帧）
        std::reverse(result.begin(), result.end());
        return result;
    }

    // ============================================================
    // requestPreviewRewindFrame – 预览倒带帧缩略图（UI线程调用）
    // ============================================================
    void MgbaGameView::requestPreviewRewindFrame(int frameIndex)
    {
        if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS))
            return;
        if (!m_rendererReady || frameIndex < 0)
            return;

        std::vector<uint16_t> thumb;
        unsigned thumbW = 0;
        unsigned thumbH = 0;
        {
            std::lock_guard<std::mutex> lk(m_rewindMutex);
            if (frameIndex >= static_cast<int>(m_rewindBuffer.size()))
                return;
            const auto& rewindFrame = m_rewindBuffer[static_cast<std::size_t>(frameIndex)];
            thumb = rewindFrame.thumb;
            thumbW = rewindFrame.thumbW;
            thumbH = rewindFrame.thumbH;
        }

        auto frame = rewindThumbToPreviewFrame(thumb, thumbW, thumbH);
        if (!frame.pixels.empty())
            m_renderer.uploadFrame(frame);
    }

    // ============================================================
    // requestRestoreRewindFrame – 通过 GameSignal 请求恢复指定帧
    // ============================================================
    void MgbaGameView::requestRestoreRewindFrame(int frameIndex)
    {
        if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS))
            return;
        GameSignal::instance().requestRewindRestore(frameIndex);
    }

    void MgbaGameView::requestCheatPathUpdate(const std::string& path)
    {
        m_gameEntry.cheatPath = path;
        brls::Logger::info("MgbaGameView: queue cheat path update path={}", path);
        GameSignal::instance().requestCheatPathUpdate(path);
    }

    void MgbaGameView::applyCheatsUpdate(const std::vector<CheatEntry>& cheats)
    {
        size_t enabled = 0;
        for (const auto& cheat : cheats)
        {
            if (cheat.enabled)
                ++enabled;
        }
        brls::Logger::info("MgbaGameView: queue cheat list entries={} enabled={}",
                           cheats.size(), enabled);
        GameSignal::instance().requestApplyCheats(cheats);
    }

    void MgbaGameView::_onShaderToggle(bool on)
    {
        if (!m_rendererReady) return;

        m_gameEntry.shaderEnabled = on;
        // 持久化到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "shaderEnabled",
                nlohmann::json(m_gameEntry.shaderEnabled));
        }
        brls::Logger::debug("MgbaGameView: Shader {} (enabled={})", m_gameEntry.shaderPath, m_gameEntry.shaderEnabled);
        unsigned gw = m_core && m_core->GameWidth() > 0 ? m_core->GameWidth() : beiklive::GetGamePixelWidth(m_gameEntry.platform);
        unsigned gh = m_core && m_core->GameHeight() > 0 ? m_core->GameHeight() : beiklive::GetGamePixelHeight(m_gameEntry.platform);
        const bool skipNdsShaderForAccel =
            m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
#ifdef __SWITCH__
            true;
#else
            m_gameEntry.ndsInternalResolution > 1;
#endif
        const std::string path = (on && !skipNdsShaderForAccel && !m_gameEntry.shaderPath.empty())
            ? m_gameEntry.shaderPath
            : "";
        m_rendererReady = _initGameRenderers(gw, gh, path);
        _requestLastFrameUpload();
    }

    void MgbaGameView::_onShaderPathChange(const std::string& path)
    {
        if (!m_rendererReady) return;
        bool shaderOn = m_gameEntry.shaderEnabled;
        const bool pathChanged = m_gameEntry.shaderPath != path;
        m_gameEntry.shaderPath = path;
        if (pathChanged) {
            m_gameEntry.shaderParaPath.clear();
            m_gameEntry.shaderParaNames.clear();
            m_gameEntry.shaderParaValues.clear();
        }
        brls::Logger::debug("MgbaGameView: Shader path changed to {} (enabled={})", m_gameEntry.shaderPath, shaderOn);
        // 持久化到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "shaderPath",
                nlohmann::json(m_gameEntry.shaderPath));
            if (pathChanged) {
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaPath",
                    nlohmann::json(m_gameEntry.shaderParaPath));
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaNames",
                    nlohmann::json(m_gameEntry.shaderParaNames));
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaValues",
                    nlohmann::json(m_gameEntry.shaderParaValues));
            }
        }
        unsigned gw = m_core && m_core->GameWidth() > 0 ? m_core->GameWidth() : beiklive::GetGamePixelWidth(m_gameEntry.platform);
        unsigned gh = m_core && m_core->GameHeight() > 0 ? m_core->GameHeight() : beiklive::GetGamePixelHeight(m_gameEntry.platform);
        const bool skipNdsShaderForAccel =
            m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
#ifdef __SWITCH__
            true;
#else
            m_gameEntry.ndsInternalResolution > 1;
#endif
        const std::string shaderPath = (shaderOn && !skipNdsShaderForAccel && !path.empty()) ? path : "";
        m_rendererReady = _initGameRenderers(gw, gh, shaderPath);
        _requestLastFrameUpload();
    }

    void MgbaGameView::_onDisplayModeChange(const std::string& mode)
    {
        // 画面模式: fit/fill/original/4:3/integer/custom
        if (mode == "fit")      m_screenMode = beiklive::ScreenMode::Fit;
        else if (mode == "fill") m_screenMode = beiklive::ScreenMode::Fill;
        else if (mode == "four_three" || mode == "4:3") m_screenMode = beiklive::ScreenMode::FourThree;
        else if (mode == "integer") m_screenMode = beiklive::ScreenMode::IntegerScale;
        else if (mode == "custom")  m_screenMode = beiklive::ScreenMode::FreeScale;
        else m_screenMode = beiklive::ScreenMode::Fit;
        m_gameEntry.displayMode = static_cast<int>(m_screenMode);

        // 持久化画面模式到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "displayMode",
                nlohmann::json(m_gameEntry.displayMode));
            beiklive::GameDB->flush();
        }
    }

    void MgbaGameView::_onIntegerScaleChange(float scale)
    {
        m_gameEntry.integerAspectRatio = scale;

        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "integerAspectRatio",
                nlohmann::json(static_cast<float>(scale)));
            beiklive::GameDB->flush();
        }
    }

    void MgbaGameView::_onCustomValuesChanged(float x, float y, float scale)
    {
        m_gameEntry.customOffsetX = x;
        m_gameEntry.customOffsetY = y;
        m_gameEntry.customScale  = scale;

        // 持久化自定义值到数据库
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "customOffsetX",
                nlohmann::json(static_cast<double>(x)));
            beiklive::GameDB->set(m_gameEntry.path, "customOffsetY",
                nlohmann::json(static_cast<double>(y)));
            beiklive::GameDB->set(m_gameEntry.path, "customScale",
                nlohmann::json(static_cast<double>(scale)));
            beiklive::GameDB->flush();
        }
    }

    void MgbaGameView::_onOverlayToggle(bool enabled)
    {
        m_gameEntry.overlayEnabled = enabled;
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "overlayEnabled",
                nlohmann::json(enabled));
        }
    }

    void MgbaGameView::_onOverlayPathChange(const std::string& path)
    {
        m_gameEntry.overlayPath = path;
        // 清除已缓存的遮罩纹理，使其重新加载
        if (m_overlayImage) m_overlayImage->clear();
        if (beiklive::GameDB && !m_gameEntry.path.empty()) {
            beiklive::GameDB->set(m_gameEntry.path, "overlayPath",
                nlohmann::json(path));
        }
    }

    void MgbaGameView::_onFilterChange(const std::string& filter)
    {
        if (!m_rendererReady) return;
        if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
            m_ndsLayout == "hybrid") {
            m_renderer.setFilter(false);
            m_ndsTopRenderer.setFilter(false);
            m_ndsBottomRenderer.setFilter(false);
            return;
        }
        m_renderer.setFilter(filter == "linear");
        m_ndsTopRenderer.setFilter(filter == "linear");
        m_ndsBottomRenderer.setFilter(filter == "linear");
    }

    void MgbaGameView::_onConfigUpdated()
    {
        GameSignal::instance().requestConfigUpdate();
    }

    std::vector<ShaderParamInfo> MgbaGameView::_getShaderParams() const
    {
        if (m_ndsSplitShaderRenderer)
            return m_ndsTopRenderer.getShaderParams();
        return m_renderer.getShaderParams();
    }

    void MgbaGameView::_setShaderParam(const std::string& name, float val)
    {
        m_renderer.setShaderParam(name, val);
        m_ndsTopRenderer.setShaderParam(name, val);
        m_ndsBottomRenderer.setShaderParam(name, val);
    }

    // ============================================================
    // _downsampleToRGB565 – RGBA8888 降采样并转换为 RGB565
    // 支持最近邻（NearestNeighbor）和双线性（Bilinear）两种压缩策略
    // ============================================================
    std::vector<uint16_t> MgbaGameView::_downsampleToRGB565(
        const std::vector<uint32_t>& src,
        unsigned srcW, unsigned srcH,
        unsigned dstW, unsigned dstH)
    {
        std::vector<uint16_t> dst(dstW * dstH, 0);
        if (src.empty() || srcW == 0 || srcH == 0) return dst;

        // 使用缓存的压缩策略设置
        bool useBilinear = (m_cachedThumbCompression == static_cast<int>(
            beiklive::RewindThumbCompression::Bilinear));

        for (unsigned y = 0; y < dstH; ++y) {
            for (unsigned x = 0; x < dstW; ++x) {
                uint8_t r, g, b;

                if (useBilinear) {
                    // 双线性插值：计算源坐标（浮点）并对四邻域加权
                    float sx = (x + 0.5f) * static_cast<float>(srcW) / static_cast<float>(dstW) - 0.5f;
                    float sy = (y + 0.5f) * static_cast<float>(srcH) / static_cast<float>(dstH) - 0.5f;
                    int x0 = static_cast<int>(sx);
                    int y0 = static_cast<int>(sy);
                    int x1 = x0 + 1;
                    int y1 = y0 + 1;
                    // 钳位到边界
                    if (x0 < 0) x0 = 0;
                    if (y0 < 0) y0 = 0;
                    if (x1 >= static_cast<int>(srcW)) x1 = static_cast<int>(srcW) - 1;
                    if (y1 >= static_cast<int>(srcH)) y1 = static_cast<int>(srcH) - 1;
                    float fx = sx - static_cast<float>(x0);
                    float fy = sy - static_cast<float>(y0);
                    // 双线性加权混合四个源像素
                    // makeRGBA8888 存储格式（字节序）：字节0=R，字节1=G，字节2=B，字节3=A
                    // 对应 uint32 移位：R=(px>>0)&0xFF, G=(px>>8)&0xFF, B=(px>>16)&0xFF
                    auto getPixelAt = [&](int px, int py) -> uint32_t {
                        return src[static_cast<unsigned>(py) * srcW + static_cast<unsigned>(px)];
                    };
                    uint32_t p00 = getPixelAt(x0, y0), p10 = getPixelAt(x1, y0);
                    uint32_t p01 = getPixelAt(x0, y1), p11 = getPixelAt(x1, y1);
                    auto extractChannel = [](uint32_t p, int shift) -> float {
                        return static_cast<float>((p >> shift) & 0xFF);
                    };
                    r = static_cast<uint8_t>(
                        extractChannel(p00, 0)*(1-fx)*(1-fy) + extractChannel(p10, 0)*fx*(1-fy) +
                        extractChannel(p01, 0)*(1-fx)*fy     + extractChannel(p11, 0)*fx*fy);
                    g = static_cast<uint8_t>(
                        extractChannel(p00, 8)*(1-fx)*(1-fy) + extractChannel(p10, 8)*fx*(1-fy) +
                        extractChannel(p01, 8)*(1-fx)*fy     + extractChannel(p11, 8)*fx*fy);
                    b = static_cast<uint8_t>(
                        extractChannel(p00,16)*(1-fx)*(1-fy) + extractChannel(p10,16)*fx*(1-fy) +
                        extractChannel(p01,16)*(1-fx)*fy     + extractChannel(p11,16)*fx*fy);
                } else {
                    // 最近邻采样（默认）
                    unsigned sx = x * srcW / dstW;
                    unsigned sy = y * srcH / dstH;
                    // makeRGBA8888 存储格式（字节序）：字节0=R，字节1=G，字节2=B，字节3=A
                    // 对应 uint32 移位：R=(px>>0)&0xFF, G=(px>>8)&0xFF, B=(px>>16)&0xFF
                    uint32_t px = src[sy * srcW + sx];
                    r = static_cast<uint8_t>( px        & 0xFF);  // R：字节偏移 0
                    g = static_cast<uint8_t>((px >> 8)  & 0xFF);  // G：字节偏移 1
                    b = static_cast<uint8_t>((px >> 16) & 0xFF);  // B：字节偏移 2
                }

                // 打包为 RGB565：R(5) | G(6) | B(5)
                dst[y * dstW + x] = static_cast<uint16_t>(
                    ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
            }
        }
        return dst;
    }

    // ============================================================
    // _crc32Sram – 简单 CRC32 计算（缓冲区版本）
    // ============================================================
    uint32_t MgbaGameView::_crc32Sram(const void* data, size_t size)
    {
        if (!data || size == 0) return 0;
        uint32_t crc = 0xFFFFFFFF;
        const uint8_t* p = static_cast<const uint8_t*>(data);
        static const uint32_t table[256] = {
            0x00000000,0x77073096,0xEE0E612C,0x990951BA,0x076DC419,0x706AF48F,0xE963A535,0x9E6495A3,
            0x0EDB8832,0x79DCB8A4,0xE0D5E91E,0x97D2D988,0x09B64C2B,0x7EB17CBD,0xE7B82D07,0x90BF1D91,
            0x1DB71064,0x6AB020F2,0xF3B97148,0x84BE41DE,0x1ADAD47D,0x6DDDE4EB,0xF4D4B551,0x83D385C7,
            0x136C9856,0x646BA8C0,0xFD62F97A,0x8A65C9EC,0x14015C4F,0x63066CD9,0xFA0F3D63,0x8D080DF5,
            0x3B6E20C8,0x4C69105E,0xD56041E4,0xA2677172,0x3C03E4D1,0x4B04D447,0xD20D85FD,0xA50AB56B,
            0x35B5A8FA,0x42B2986C,0xDBBBC9D6,0xACBCF940,0x32D86CE3,0x45DF5C75,0xDCD60DCF,0xABD13D59,
            0x26D930AC,0x51DE003A,0xC8D75180,0xBFD06116,0x21B4F4B5,0x56B3C423,0xCFBA9599,0xB8BDA50F,
            0x2802B89E,0x5F058808,0xC60CD9B2,0xB10BE924,0x2F6F7C7B,0x58684C11,0xC1611DAB,0xB6662D3D,
            0x76DC4190,0x01DB7106,0x98D220BC,0xEFD5102A,0x71B18589,0x06B6B51F,0x9FBFE4A5,0xE8B8D433,
            0x7807C9A2,0x0F00F934,0x9609A88E,0xE10E9818,0x7F6A0DBB,0x086D3D2D,0x91646C97,0xE6635C01,
            0x6B6B51F4,0x1C6C6162,0x856530D8,0xF262004E,0x6C0695ED,0x1B01A57B,0x8208F4C1,0xF50FC457,
            0x65B0D9C6,0x12B7E950,0x8BBEB8EA,0xFCB9887C,0x62DD1DDF,0x15DA2D49,0x8CD37CF3,0xFBD44C65,
            0x4DB26158,0x3AB551CE,0xA3BC0074,0xD4BB30E2,0x4ADFA541,0x3DD895D7,0xA4D1C46D,0xD3D6F4FB,
            0x4369E96A,0x346ED9FC,0xAD678846,0xDA60B8D0,0x44042D73,0x33031DE5,0xAA0A4C5F,0xDD0D7CC9,
            0x5005713C,0x270241AA,0xBE0B1010,0xC90C2086,0x5768B525,0x206F85B3,0xB966D409,0xCE61E49F,
            0x5EDEF90E,0x29D9C998,0xB0D09822,0xC7D7A8B4,0x59B33D17,0x2EB40D81,0xB7BD5C3B,0xC0BA6CAD,
            0xEDB88320,0x9ABFB3B6,0x03B6E20C,0x74B1D29A,0xEAD54739,0x9DD277AF,0x04DB2615,0x73DC1683,
            0xE3630B12,0x94643B84,0x0D6D6A3E,0x7A6A5AA8,0xE40ECF0B,0x9309FF9D,0x0A00AE27,0x7D079EB1,
            0xF00F9344,0x8708A3D2,0x1E01F268,0x6906C2FE,0xF762575D,0x806567CB,0x196C3671,0x6E6B06E7,
            0xFED41B76,0x89D32BE0,0x10DA7A5A,0x67DD4ACC,0xF9B9DF6F,0x8EBEEFF9,0x17B7BE43,0x60B08ED5,
            0xD6D6A3E8,0xA1D1937E,0x38D8C2C4,0x4FDFF252,0xD1BB67F1,0xA6BC5767,0x3FB506DD,0x48B2364B,
            0xD80D2BDA,0xAF0A1B4C,0x36034AF6,0x41047A60,0xDF60EFC3,0xA867DF55,0x316E8EEF,0x4669BE79,
            0xCB61B38C,0xBC66831A,0x256FD2A0,0x527F2236,0xCC0C7795,0xBB0B4703,0x220216B9,0x5505262F,
            0xC5BA3BBE,0xB2BD0B28,0x2BB45A92,0x5CB30A04,0xC2D7FFA7,0xB5D0CF31,0x2CD99E8B,0x5BDEAE1D,
            0x9B64C2B0,0xEC63F226,0x756AA39C,0x026D930A,0x9C0906A9,0xEB0E363F,0x72076785,0x05005713,
            0x95BF4A82,0xE2B87A14,0x7BB12BAE,0x0CB61B38,0x92D28E9B,0xE5D5BE0D,0x7CDCEFB7,0x0BDBDF21,
            0x86D3D2D4,0xF1D4E242,0x68DDB3F8,0x1FDA836E,0x81BE16CD,0xF6B9265B,0x6FB077E1,0x18B74777,
            0x88085AE6,0xFF0F6A70,0x66063BCA,0x11010B5C,0x8F659EFF,0xF862AE69,0x616BFFD3,0x166CCF45,
            0xA00AE278,0xD70DD2EE,0x4E048354,0x3903B3C2,0xA7672661,0xD06016F7,0x4969474D,0x3E6E77DB,
            0xAED16A4A,0xD9D65ADC,0x40DF0B66,0x37D83BF0,0xA9BCAE53,0xDEBB9EC5,0x47B2CF7F,0x30B5FFE9,
            0xBDBDF21C,0xCABAC28A,0x53B39330,0x24B4A3A6,0xBAD03605,0xCDD70693,0x54DE5729,0x23D967BF,
            0xB3667A2E,0xC4614AB8,0x5D681B02,0x2A6F2B94,0xB40BBE37,0xC30C8EA1,0x5A05DF1B,0x2D02EF8D,
        };
        for (size_t i = 0; i < size; ++i)
            crc = table[(crc ^ p[i]) & 0xFF] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFF;
    }

    // ============================================================
    // _checkAndAutoSaveSram – CRC 检测 + 延迟自动落盘
    // 每秒检查一次 SRAM CRC，变化后等待 2 秒写盘
    // ============================================================
    void MgbaGameView::_checkAndAutoSaveSram()
    {
        if (!m_core || !m_core->IsReady()) return;

        auto now = std::chrono::steady_clock::now();
        double sinceCheck = std::chrono::duration<double>(now - m_sramLastCheck).count();
        if (sinceCheck < SRAM_CHECK_INTERVAL) return;
        m_sramLastCheck = now;

        size_t sz = m_core->getSramSize();
        const void* ptr = m_core->getSramData();
        if (!ptr || sz == 0) return;

        uint32_t crc = _crc32Sram(ptr, sz);
        if (crc != m_sramLastCRC)
        {
            m_sramLastCRC  = crc;
            m_sramDirty    = true;
            m_sramDirtyTime = now;
        }
        else if (m_sramDirty)
        {
            double sinceDirty = std::chrono::duration<double>(now - m_sramDirtyTime).count();
            if (sinceDirty >= SRAM_FLUSH_DELAY)
            {
                m_core->saveSram();
                m_sramDirty = false;
                brls::Logger::debug("MgbaGameView: SRAM auto-saved");
            }
        }
    }
}
