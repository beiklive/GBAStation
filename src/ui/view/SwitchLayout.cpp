#include "SwitchLayout.hpp"
#include "core/Translation.hpp"

#include "core/ThreadPool.hpp"
#include "core/Tools.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/utils/Pico8Transition.hpp"

#include <borealis/extern/nanovg/stb_image.h>
#include <borealis/views/hint.hpp>

#include <algorithm>
#include <cmath>
#include <ctime>

namespace
{
    constexpr size_t MAX_RECENT_GAMES = 10;
    constexpr int HOME_CARD_SLOTS = 10;
    constexpr float CARD_WIDTH = 220.f;
    constexpr float CARD_HEIGHT = 350.f;
    constexpr float CARD_GAP = 20.f;
    constexpr float CARD_PITCH = CARD_WIDTH + CARD_GAP;
    constexpr float CARD_START_X = 30.f;
    constexpr float HOLD_DELAY = 0.30f;
    constexpr float HOLD_REPEAT = 0.085f;

    std::string encodeUtf8(char32_t codepoint)
    {
        std::string out;
        if (codepoint <= 0x7F) {
            out.push_back(static_cast<char>(codepoint));
        } else if (codepoint <= 0x7FF) {
            out.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
            out.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
        }
        return out;
    }

    float clamp01(float value)
    {
        return std::max(0.f, std::min(1.f, value));
    }

    float easeOutCubic(float value)
    {
        value = clamp01(value);
        return 1.f - std::pow(1.f - value, 3.f);
    }

    float easeOutBack(float value)
    {
        value = clamp01(value);
        constexpr float c1 = 1.70158f;
        constexpr float c3 = c1 + 1.f;
        const float t = value - 1.f;
        return 1.f + c3 * t * t * t + c1 * t * t;
    }

    float smoothStep(float value)
    {
        value = clamp01(value);
        return value * value * (3.f - 2.f * value);
    }

    NVGcolor platformBadgeColor(int platform)
    {
        using beiklive::enums::EmuPlatform;
        switch (static_cast<EmuPlatform>(platform)) {
            case EmuPlatform::EmuGBA:  return nvgRGBA(108, 77, 191, 220);
            case EmuPlatform::EmuGBC:  return nvgRGBA(0, 112, 221, 220);
            case EmuPlatform::EmuGB:   return nvgRGBA(0, 168, 107, 220);
            case EmuPlatform::EmuNES:  return nvgRGBA(218, 41, 28, 220);
            case EmuPlatform::EmuSNES: return nvgRGBA(160, 100, 180, 220);
            case EmuPlatform::EmuNDS:  return nvgRGBA(54, 150, 190, 220);
            case EmuPlatform::Emu3DS:  return nvgRGBA(230, 79, 91, 220);
            case EmuPlatform::EmuGenesis: return nvgRGBA(23, 55, 139, 220);
            case EmuPlatform::EmuArcade: return nvgRGBA(236, 134, 44, 220);
            case EmuPlatform::EmuDreamcast: return nvgRGBA(0, 142, 180, 220);
            case EmuPlatform::EmuPSP: return nvgRGBA(67, 118, 226, 220);
            default:                   return nvgRGBA(100, 100, 100, 200);
        }
    }

    std::vector<unsigned char> resizeRgba(const unsigned char* source,
                                          int sourceWidth, int sourceHeight,
                                          int targetWidth, int targetHeight)
    {
        std::vector<unsigned char> output(
            static_cast<size_t>(targetWidth) * static_cast<size_t>(targetHeight) * 4);
        for (int y = 0; y < targetHeight; ++y) {
            const int sy = std::min(sourceHeight - 1,
                                    y * sourceHeight / targetHeight);
            for (int x = 0; x < targetWidth; ++x) {
                const int sx = std::min(sourceWidth - 1,
                                        x * sourceWidth / targetWidth);
                const size_t src =
                    (static_cast<size_t>(sy) * sourceWidth + sx) * 4;
                const size_t dst =
                    (static_cast<size_t>(y) * targetWidth + x) * 4;
                output[dst + 0] = source[src + 0];
                output[dst + 1] = source[src + 1];
                output[dst + 2] = source[src + 2];
                output[dst + 3] = source[src + 3];
            }
        }
        return output;
    }
}

namespace beiklive
{
    SwitchLayout::SwitchLayout() : Layout()
    {
        setFocusable(true);
        setHideHighlightBackground(true);
        setHideHighlightBorder(true);
        setHideClickAnimation(true);
        setBackground(brls::ViewBackground::NONE);
        setClipsToBounds(true);
        setFocusSound(brls::SOUND_NONE);
        setCustomNavigationRoute(brls::FocusDirection::UP, this);
        setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

        m_fontId = brls::Application::getDefaultFont();
        m_materialFontId = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        m_switchIconFontId = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        m_lastFrameTime = std::chrono::steady_clock::now();
        m_textureLoader = std::make_shared<TextureLoaderState>();

        const std::string pathPrefix = "img/ui/" + std::string(
            brls::Application::getPlatform()->getThemeVariant() ==
                    brls::ThemeVariant::DARK
                ? "light/"
                : "dark/");
        m_functions = {
            {L("游戏库"), BK_RES(pathPrefix + "GameList_64.png"), 0},
            {L("文件列表"), BK_RES(pathPrefix + "wenjianjia_64.png"), 0},
            {L("数据管理"), BK_RES(pathPrefix + "jifen_64.png"), 0},
            {L("设置"), BK_RES(pathPrefix + "shezhi_64.png"), 0},
            {L("关于"), BK_RES(pathPrefix + "bangzhu_64.png"), 0},
            {L("退出"), BK_RES(pathPrefix + "tuichu_64.png"), 0},
        };
        m_functionFocus.assign(m_functions.size(), 0.f);
        m_slotFocus.assign(HOME_CARD_SLOTS, 0.f);

        auto consume = [](brls::View*) -> bool { return true; };
        registerAction("", brls::BUTTON_A, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_UP, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_DOWN, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_UP, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_DOWN, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LEFT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RIGHT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LB, consume,
                       true, false, brls::SOUND_NONE);
        _captureInputState();
    }

    SwitchLayout::~SwitchLayout()
    {
        if (m_textureLoader)
            m_textureLoader->alive.store(false);
        if (auto* vg = brls::Application::getNVGContext()) {
            for (const auto& texture : m_textureCache) {
                if (texture.second > 0)
                    nvgDeleteImage(vg, texture.second);
            }
            for (const auto& function : m_functions) {
                if (function.imageHandle > 0)
                    nvgDeleteImage(vg, function.imageHandle);
            }
            if (m_pico8LogoImageHandle > 0)
                nvgDeleteImage(vg, m_pico8LogoImageHandle);
        }
        m_textureCache.clear();
    }

    void SwitchLayout::refreshGameList(beiklive::GameList gameList)
    {
        if (isDeleteAnimationRunning()) {
            m_pendingGameList = std::move(gameList);
            m_hasPendingGameList = true;
            return;
        }

        const bool firstLoad = m_loading;
        m_deleteWaiting = false;
        m_deleteCollapsing = false;
        m_deleteBackendFinished = false;
        m_reflowRunning = false;
        m_deleteAnimationTime = 0.f;
        m_deleteCollapseProgress = 0.f;
        m_reflowProgress = 1.f;
        m_deleteIndex = -1;
        m_reflowStartIndex = -1;
        m_newBlankIndex = -1;
        m_deleteCompletion = nullptr;
        std::string selectedPath;
        if (m_selectedGame >= 0 &&
            static_cast<size_t>(m_selectedGame) < m_games.size())
            selectedPath = m_games[static_cast<size_t>(m_selectedGame)].entry.path;

        m_games.clear();
        m_games.reserve(std::min(MAX_RECENT_GAMES, gameList.size()));
        for (auto& entry : gameList) {
            if (entry.path.empty())
                continue;
            m_games.push_back({std::move(entry), 0.f});
            if (m_games.size() >= MAX_RECENT_GAMES)
                break;
        }

        // 最近一次游戏的即时存档缩略图可能在退出游戏时覆盖同一路径；
        // 每次刷新只强制失效这一张，并处理其他显式要求刷新的封面。
        auto invalidateTexture = [this](const std::string& path) {
            if (path.empty())
                return;
            if (path == m_pinnedTexturePath) {
                m_pinnedTextureInvalidated = true;
                return;
            }
            auto cached = m_textureCache.find(path);
            if (cached != m_textureCache.end()) {
                if (auto* vg = brls::Application::getNVGContext()) {
                    if (cached->second > 0)
                        nvgDeleteImage(vg, cached->second);
                }
                m_textureCache.erase(cached);
            }
            m_failedTextures.erase(path);
        };
        for (const auto& game : m_games) {
            if (beiklive::g_forceRefreshPaths.erase(game.entry.logoPath) > 0)
                invalidateTexture(game.entry.logoPath);
        }

        m_selectedGame = 0;
        if (!selectedPath.empty()) {
            for (size_t i = 0; i < m_games.size(); ++i) {
                if (m_games[i].entry.path == selectedPath) {
                    m_selectedGame = static_cast<int>(i);
                    break;
                }
            }
        }
        if (firstLoad) {
            m_selectedGame = 0;
            m_focusRow = FocusRow::GAMES;
        }

        m_loading = false;
        m_snapScroll = true;
        _resetTextureRequests();
        _requestTexturesByPriority();
        invalidate();
    }

    void SwitchLayout::restoreCardFocus(bool animated)
    {
        m_focusRow = FocusRow::GAMES;
        m_selectedGame = std::max(0, std::min(
            m_selectedGame, HOME_CARD_SLOTS - 1));
        brls::Application::giveFocus(this);
        _updateTargetScroll(getWidth());
        if (!animated) {
            m_scrollX = m_targetScrollX;
            m_snapScroll = true;
        }
    }

    void SwitchLayout::resetCardFocusToFirst()
    {
        m_selectedGame = 0;
        restoreCardFocus(false);
    }

    void SwitchLayout::removeGameByPath(const std::string& path)
    {
        if (path.empty() || m_deleteWaiting || m_deleteCollapsing ||
            m_reflowRunning)
            return;
        for (size_t i = 0; i < m_games.size(); ++i) {
            if (m_games[i].entry.path != path)
                continue;
            m_deleteIndex = static_cast<int>(i);
            m_deleteWaiting = true;
            m_deleteCollapsing = false;
            m_deleteBackendFinished = false;
            m_deleteAnimationTime = 0.f;
            m_deleteCollapseProgress = 0.f;
            m_deleteCompletion = nullptr;
            _captureInputState();
            invalidate();
            return;
        }
    }

    void SwitchLayout::completeGameRemoval(std::function<void()> completion)
    {
        if (!isDeleteAnimationRunning()) {
            if (completion)
                completion();
            return;
        }
        m_deleteBackendFinished = true;
        m_deleteCompletion = std::move(completion);
        invalidate();
    }

    void SwitchLayout::cancelGameRemoval()
    {
        m_deleteWaiting = false;
        m_deleteCollapsing = false;
        m_deleteBackendFinished = false;
        m_reflowRunning = false;
        m_deleteAnimationTime = 0.f;
        m_deleteCollapseProgress = 0.f;
        m_reflowProgress = 1.f;
        m_deleteIndex = -1;
        m_reflowStartIndex = -1;
        m_newBlankIndex = -1;
        m_deleteCompletion = nullptr;

        if (m_hasPendingGameList) {
            auto pending = std::move(m_pendingGameList);
            m_pendingGameList.clear();
            m_hasPendingGameList = false;
            refreshGameList(std::move(pending));
        }
        invalidate();
    }

    int SwitchLayout::acquireSelectedCoverTexture()
    {
        if (m_selectedGame < 0 ||
            static_cast<size_t>(m_selectedGame) >= m_games.size())
            return -1;
        const auto& selectedPath =
            m_games[static_cast<size_t>(m_selectedGame)].entry.logoPath;
        if (selectedPath.empty())
            return -1;
        if (!m_pinnedTexturePath.empty()) {
            if (m_pinnedTexturePath != selectedPath)
                return -1;
            ++m_pinnedTextureReferences;
        } else {
            m_pinnedTexturePath = selectedPath;
            m_pinnedTextureInvalidated = false;
            m_pinnedTextureReferences = 1;
        }
        auto found = m_textureCache.find(m_pinnedTexturePath);
        return found == m_textureCache.end() ? -1 : found->second;
    }

    void SwitchLayout::releaseSelectedCoverTexture()
    {
        if (m_pinnedTexturePath.empty())
            return;
        if (m_pinnedTextureReferences > 1) {
            --m_pinnedTextureReferences;
            return;
        }
        const bool needsRefresh = m_pinnedTextureInvalidated;
        if (needsRefresh) {
            auto cached = m_textureCache.find(m_pinnedTexturePath);
            if (cached != m_textureCache.end()) {
                if (auto* vg = brls::Application::getNVGContext()) {
                    if (cached->second > 0)
                        nvgDeleteImage(vg, cached->second);
                }
                m_textureCache.erase(cached);
            }
            m_failedTextures.erase(m_pinnedTexturePath);
        }
        m_pinnedTexturePath.clear();
        m_pinnedTextureInvalidated = false;
        m_pinnedTextureReferences = 0;
        m_textureCacheDirty = true;
        if (needsRefresh)
            _requestTexturesByPriority();
    }

    void SwitchLayout::playEntranceAnimation()
    {
        if (isDeleteAnimationRunning())
            return;

        m_exitAnimationRunning = false;
        m_exitCompletionArmed = false;
        m_exitCompletion = nullptr;
        m_pico8ExitAnimationRunning = false;
        m_pico8ReturnAnimationRunning = false;
        m_pico8TransitionProgress = 0.f;
        m_pico8HoldActive = false;
        m_pico8ReleaseAnimating = false;
        m_pico8ShortcutScale = 1.f;
        m_pico8ReleaseTime = 0.f;
        m_pageEntrance = 0.f;
        m_contentEntrance = 0.f;
        _captureInputState();
        invalidate();
    }

    void SwitchLayout::playExitAnimation(std::function<void()> completion)
    {
        if (m_exitAnimationRunning) {
            if (completion)
                m_exitCompletion = std::move(completion);
            return;
        }
        m_exitAnimationRunning = true;
        m_pico8ExitAnimationRunning = false;
        m_pico8ReturnAnimationRunning = false;
        m_pico8TransitionProgress = 0.f;
        m_pico8HoldActive = false;
        m_pico8ReleaseAnimating = false;
        m_pico8ShortcutScale = 1.f;
        m_pico8ReleaseTime = 0.f;
        m_exitCompletionArmed = false;
        m_exitCompletion = std::move(completion);
        _captureInputState();
        invalidate();
    }

    void SwitchLayout::playPico8ExitAnimation(
        std::function<void()> completion)
    {
        if (m_exitAnimationRunning) {
            if (completion)
                m_exitCompletion = std::move(completion);
            return;
        }
        m_exitAnimationRunning = true;
        m_pico8ExitAnimationRunning = true;
        m_pico8ReturnAnimationRunning = false;
        m_pico8TransitionProgress = 0.f;
        m_pico8HoldActive = false;
        m_pico8ReleaseAnimating = false;
        m_pico8ShortcutScale = 1.f;
        m_pico8ReleaseTime = 0.f;
        m_exitCompletionArmed = false;
        m_exitCompletion = std::move(completion);
        brls::Application::blockInputs();
        _captureInputState();
        invalidate();
    }

    void SwitchLayout::beginPico8ReturnAnimation()
    {
        m_exitAnimationRunning = false;
        m_pico8ExitAnimationRunning = false;
        m_pico8ReturnAnimationRunning = true;
        m_pico8TransitionProgress = 1.f;
        m_exitCompletionArmed = false;
        m_exitCompletion = nullptr;
        m_pageEntrance = 0.f;
        m_contentEntrance = 0.f;
        _captureInputState();
        invalidate();
    }

    void SwitchLayout::setPico8ReturnProgress(float progress)
    {
        if (!m_pico8ReturnAnimationRunning)
            return;
        m_pico8TransitionProgress = 1.f - clamp01(progress);
        invalidate();
    }

    void SwitchLayout::finishPico8ReturnAnimation()
    {
        m_pico8ReturnAnimationRunning = false;
        m_pico8TransitionProgress = 0.f;
        m_pageEntrance = 1.f;
        m_contentEntrance = 1.f;
        _captureInputState();
        invalidate();
    }

    void SwitchLayout::setPico8ShortcutVisible(bool visible)
    {
        if (m_pico8ShortcutVisible == visible)
            return;
        m_pico8ShortcutVisible = visible;
        if (!visible) {
            m_pico8HoldActive = false;
            m_pico8ReleaseAnimating = false;
            m_pico8ReleaseTime = 0.f;
            m_pico8ShortcutScale = 1.f;
        }
        _captureInputState();
        invalidate();
    }

    void SwitchLayout::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;

        m_time += dt;
        _updateStatusIndicators(dt);
        if (m_exitAnimationRunning) {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 4.8f);
            if (m_pico8ExitAnimationRunning) {
                m_pico8TransitionProgress = std::min(
                    1.f, m_pico8TransitionProgress +
                        dt / beiklive::pico8_transition::TRANSITION_DURATION);
            }
            const bool transitionFinished = m_pageEntrance <= 0.f &&
                (!m_pico8ExitAnimationRunning ||
                 m_pico8TransitionProgress >= 1.f);
            if (transitionFinished && m_exitCompletion) {
                if (!m_exitCompletionArmed) {
                    m_exitCompletionArmed = true;
                    invalidate();
                    return;
                }
                auto completion = std::move(m_exitCompletion);
                m_exitCompletion = nullptr;
                brls::sync(std::move(completion));
            }
        } else {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 4.f);
            m_contentEntrance = std::min(1.f, m_contentEntrance + dt * 3.4f);
        }

        for (size_t i = 0; i < m_slotFocus.size(); ++i) {
            const bool selected = m_focusRow == FocusRow::GAMES &&
                static_cast<int>(i) == m_selectedGame;
            const float target = selected ? 1.f : 0.f;
            m_slotFocus[i] += (target - m_slotFocus[i]) *
                std::min(1.f, dt * 12.f);
        }
        for (size_t i = 0; i < m_functionFocus.size(); ++i) {
            const bool selected = m_focusRow == FocusRow::FUNCTIONS &&
                static_cast<int>(i) == m_selectedFunction;
            const float target = selected ? 1.f : 0.f;
            m_functionFocus[i] += (target - m_functionFocus[i]) *
                std::min(1.f, dt * 12.f);
        }

        if (m_deleteWaiting) {
            m_deleteAnimationTime += dt;
            if (m_deleteBackendFinished && m_deleteAnimationTime >= 0.42f) {
                m_deleteWaiting = false;
                m_deleteCollapsing = true;
                m_deleteBackendFinished = false;
                m_deleteCollapseProgress = 0.f;
            }
        }
        if (m_deleteCollapsing) {
            m_deleteCollapseProgress = std::min(
                1.f, m_deleteCollapseProgress + dt * 6.5f);
            if (m_deleteCollapseProgress >= 1.f && m_deleteIndex >= 0 &&
                static_cast<size_t>(m_deleteIndex) < m_games.size()) {
                const int oldCount = static_cast<int>(m_games.size());
                const int erasedIndex = m_deleteIndex;
                m_games.erase(m_games.begin() + erasedIndex);
                m_deleteCollapsing = false;
                m_deleteIndex = -1;
                m_reflowRunning = true;
                m_reflowProgress = 0.f;
                m_reflowStartIndex = erasedIndex;
                m_newBlankIndex = oldCount - 1;
                if (m_games.empty()) {
                    m_selectedGame = 0;
                } else {
                    m_selectedGame = std::min(
                        erasedIndex, static_cast<int>(m_games.size()) - 1);
                }
                _resetTextureRequests();
                _requestTexturesByPriority();
            }
        }
        if (m_reflowRunning) {
            m_reflowProgress = std::min(1.f, m_reflowProgress + dt * 7.f);
            if (m_reflowProgress >= 1.f) {
                m_reflowRunning = false;
                m_reflowStartIndex = -1;
                m_newBlankIndex = -1;
                if (m_hasPendingGameList) {
                    auto pending = std::move(m_pendingGameList);
                    m_pendingGameList.clear();
                    m_hasPendingGameList = false;
                    refreshGameList(std::move(pending));
                }
                if (m_deleteCompletion) {
                    auto completion = std::move(m_deleteCompletion);
                    m_deleteCompletion = nullptr;
                    brls::sync(std::move(completion));
                }
            }
        }

        if (m_functionClickAnimating) {
            m_functionClickTime += dt;
            if (m_functionClickTime >= 0.38f) {
                const int index = m_functionClickIndex;
                m_functionClickAnimating = false;
                m_functionClickIndex = -1;
                m_functionClickTime = 0.f;
                _activateFunction(index);
            }
        }

        _updateTargetScroll(getWidth());
        if (m_snapScroll) {
            m_scrollX = m_targetScrollX;
            m_snapScroll = false;
        } else {
            m_scrollX += (m_targetScrollX - m_scrollX) *
                std::min(1.f, dt * (m_fastScroll ? 24.f : 9.f));
            if (m_fastScroll &&
                std::abs(m_targetScrollX - m_scrollX) < 0.75f) {
                m_scrollX = m_targetScrollX;
                m_fastScroll = false;
            }
        }

        _updatePico8ShortcutInput(dt);
        _handleInput(dt);
        _requestTexturesByPriority();
        invalidate();
    }

    void SwitchLayout::_captureInputState()
    {
        auto& state = brls::Application::getControllerState();
        const float lx = state.axes[static_cast<int>(brls::LEFT_X)];
        const float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
        const float rx = state.axes[static_cast<int>(brls::RIGHT_X)];
        const float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
        const float navX = std::abs(rx) > std::abs(lx) ? rx : lx;
        const float navY = std::abs(ry) > std::abs(ly) ? ry : ly;
        m_prevLeft = state.buttons[static_cast<int>(brls::BUTTON_LEFT)] || navX < -0.5f;
        m_prevRight = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)] || navX > 0.5f;
        m_prevUp = state.buttons[static_cast<int>(brls::BUTTON_UP)] || navY < -0.5f;
        m_prevDown = state.buttons[static_cast<int>(brls::BUTTON_DOWN)] || navY > 0.5f;
        m_prevA = state.buttons[static_cast<int>(brls::BUTTON_A)];
        m_prevPico8Button = state.buttons[static_cast<int>(brls::BUTTON_LB)];
    }

    void SwitchLayout::_updatePico8ShortcutInput(float dt)
    {
        auto& state = brls::Application::getControllerState();
        const bool pressed =
            state.buttons[static_cast<int>(brls::BUTTON_LB)];
        if (!m_pico8ShortcutVisible) {
            m_pico8HoldActive = false;
            m_pico8ReleaseAnimating = false;
            m_pico8ReleaseTime = 0.f;
            m_pico8ShortcutScale = 1.f;
            m_prevPico8Button = pressed;
            return;
        }
        const bool canInteract = isFocused() &&
            !brls::Application::isInputBlocks() &&
            !m_exitAnimationRunning &&
            !m_pico8ReturnAnimationRunning &&
            !m_functionClickAnimating &&
            !isDeleteAnimationRunning() &&
            m_pageEntrance >= 0.999f;

        if (!canInteract) {
            m_pico8HoldActive = false;
            m_pico8ReleaseAnimating = false;
            m_pico8ReleaseTime = 0.f;
            m_pico8ShortcutScale += (1.f - m_pico8ShortcutScale) *
                std::min(1.f, dt * 20.f);
            m_prevPico8Button = pressed;
            return;
        }

        if (pressed && !m_prevPico8Button) {
            m_pico8ReleaseAnimating = false;
            m_pico8HoldActive = true;
            m_pico8ReleaseTime = 0.f;
        }

        if (!pressed && m_prevPico8Button && m_pico8HoldActive) {
            m_pico8HoldActive = false;
            m_pico8ReleaseAnimating = true;
            m_pico8ReleaseTime = 0.f;
        }

        const float targetScale = m_pico8HoldActive ? 1.065f : 1.f;
        const float scaleSpeed = m_pico8HoldActive ? 13.f : 24.f;
        m_pico8ShortcutScale += (targetScale - m_pico8ShortcutScale) *
            std::min(1.f, dt * scaleSpeed);

        if (m_pico8ReleaseAnimating) {
            m_pico8ReleaseTime += dt;
            if (m_pico8ReleaseTime >= 0.13f &&
                std::abs(m_pico8ShortcutScale - 1.f) < 0.004f) {
                m_pico8ReleaseAnimating = false;
                m_pico8ReleaseTime = 0.f;
                m_pico8ShortcutScale = 1.f;
                brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
                playPico8ExitAnimation([this]() {
                    if (onPico8Opened)
                        onPico8Opened();
                    else
                        brls::Application::unblockInputs();
                });
            }
        }

        m_prevPico8Button = pressed;
    }

    void SwitchLayout::_updateStatusIndicators(float dt)
    {
        m_statusRefreshTimer += dt;
        if (!m_clockText.empty() && m_statusRefreshTimer < 1.f)
            return;
        m_statusRefreshTimer = 0.f;

        const auto now = std::chrono::system_clock::now();
        const std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm localTime{};
#ifdef _WIN32
        localtime_s(&localTime, &nowTime);
#else
        localtime_r(&nowTime, &localTime);
#endif
        char timeBuffer[9]{};
        std::strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &localTime);
        m_clockText = timeBuffer;

        auto* platform = brls::Application::getPlatform();
        if (!platform) {
            m_networkConnected = false;
            return;
        }
        const std::string ip = platform->getIpAddress();
        const bool validIp = !ip.empty() && ip != "-" &&
            ip != "0.0.0.0" && ip != "127.0.0.1";
        m_networkConnected = validIp &&
            (platform->hasWirelessConnection() ||
             platform->hasEthernetConnection());
    }

    void SwitchLayout::_handleInput(float dt)
    {
        auto& state = brls::Application::getControllerState();
        const float lx = state.axes[static_cast<int>(brls::LEFT_X)];
        const float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
        const float rx = state.axes[static_cast<int>(brls::RIGHT_X)];
        const float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
        const float navX = std::abs(rx) > std::abs(lx) ? rx : lx;
        const float navY = std::abs(ry) > std::abs(ly) ? ry : ly;
        const bool left = state.buttons[static_cast<int>(brls::BUTTON_LEFT)] || navX < -0.5f;
        const bool right = state.buttons[static_cast<int>(brls::BUTTON_RIGHT)] || navX > 0.5f;
        const bool up = state.buttons[static_cast<int>(brls::BUTTON_UP)] || navY < -0.5f;
        const bool down = state.buttons[static_cast<int>(brls::BUTTON_DOWN)] || navY > 0.5f;
        const bool a = state.buttons[static_cast<int>(brls::BUTTON_A)];

        if (!isFocused() || brls::Application::isInputBlocks() ||
            m_exitAnimationRunning || m_pico8ReturnAnimationRunning ||
            m_pico8HoldActive || m_pico8ReleaseAnimating ||
            m_functionClickAnimating ||
            m_deleteWaiting || m_deleteCollapsing || m_reflowRunning) {
            m_holdLeft = m_holdRight = 0.f;
            m_repeatLeft = m_repeatRight = 0.f;
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            return;
        }

        if (left && !m_prevLeft)
            _moveHorizontal(-1);
        if (right && !m_prevRight)
            _moveHorizontal(1);
        if (up && !m_prevUp)
            _moveVertical(-1);
        if (down && !m_prevDown)
            _moveVertical(1);
        const bool activate = a && !m_prevA;
        if (activate)
            _activateCurrent();

        if (activate) {
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            return;
        }

        auto repeat = [this, dt](bool held, float& hold, float& timer,
                                 int direction) {
            if (!held) {
                hold = 0.f;
                timer = 0.f;
                return;
            }
            hold += dt;
            if (hold < HOLD_DELAY)
                return;
            timer += dt;
            if (timer >= HOLD_REPEAT) {
                timer = 0.f;
                _moveHorizontal(direction);
            }
        };
        repeat(left, m_holdLeft, m_repeatLeft, -1);
        repeat(right, m_holdRight, m_repeatRight, 1);

        m_prevLeft = left;
        m_prevRight = right;
        m_prevUp = up;
        m_prevDown = down;
        m_prevA = a;
    }

    void SwitchLayout::_moveHorizontal(int direction)
    {
        if (m_focusRow == FocusRow::GAMES) {
            const int count = HOME_CARD_SLOTS;
            const int previous = m_selectedGame;
            m_selectedGame = (m_selectedGame + direction + count) % count;
            if (previous == count - 1 && m_selectedGame == 0)
                m_fastScroll = true;
        } else if (!m_functions.empty()) {
            const int count = static_cast<int>(m_functions.size());
            m_selectedFunction =
                (m_selectedFunction + direction + count) % count;
        }
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void SwitchLayout::_moveVertical(int direction)
    {
        if (direction > 0 && m_focusRow == FocusRow::GAMES) {
            m_focusRow = FocusRow::FUNCTIONS;
        } else if (direction < 0 && m_focusRow == FocusRow::FUNCTIONS) {
            m_focusRow = FocusRow::GAMES;
        } else {
            return;
        }
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void SwitchLayout::_activateCurrent()
    {
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        if (m_focusRow == FocusRow::GAMES) {
            if (m_selectedGame < 0 ||
                static_cast<size_t>(m_selectedGame) >= m_games.size())
                return;
            if (onGameOptions)
                onGameOptions(m_games[static_cast<size_t>(m_selectedGame)].entry);
            return;
        }
        if (!m_functionClickAnimating) {
            m_functionClickAnimating = true;
            m_functionClickIndex = m_selectedFunction;
            m_functionClickTime = 0.f;
        }
    }

    void SwitchLayout::_activateFunction(int index)
    {
        switch (index) {
            case 0: if (onGameLibraryOpened) onGameLibraryOpened(); break;
            case 1: if (onFileBrowserOpened) onFileBrowserOpened(); break;
            case 2: if (onDataManagementOpened) onDataManagementOpened(); break;
            case 3: if (onSettingsOpened) onSettingsOpened(); break;
            case 4: if (onAboutOpened) onAboutOpened(); break;
            case 5: if (onExitRequested) onExitRequested(); break;
            default: break;
        }
    }

    void SwitchLayout::_updateTargetScroll(float width)
    {
        if (width <= 0.f) {
            m_targetScrollX = 0.f;
            return;
        }
        const float contentRight = CARD_START_X +
            static_cast<float>(HOME_CARD_SLOTS - 1) * CARD_PITCH +
            CARD_WIDTH * 1.5f;
        const float maxScroll = std::max(0.f, contentRight - width);
        const float selectedCenter = CARD_START_X +
            static_cast<float>(m_selectedGame) * CARD_PITCH +
            CARD_WIDTH * 0.5f;
        m_targetScrollX = std::max(0.f, std::min(
            maxScroll, selectedCenter - width * 0.5f));
    }

    void SwitchLayout::draw(NVGcontext* vg, float x, float y, float w, float h,
                            brls::Style, brls::FrameContext*)
    {
        if (!vg)
            return;
        _uploadDecodedTextures(vg);
        _evictUnusedTextures(vg);

        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, w, h);
        _drawGames(vg, x, y, w, h);
        _drawFunctions(vg, x, y, w, h);
        _drawFooterHint(vg, x, y, w, h);
        nvgResetScissor(vg);
        _drawPico8Shortcut(vg, x, y, w, h);
        nvgRestore(vg);
    }

    void SwitchLayout::_drawGames(NVGcontext* vg, float x, float y,
                                  float w, float h)
    {
        const float cardY = y + 115.f;
        const float pageHeight = m_exitAnimationRunning
            ? smoothStep(m_pageEntrance)
            : easeOutCubic(m_pageEntrance);
        if (m_loading) {
            for (int i = 0; i < HOME_CARD_SLOTS; ++i) {
                const float ix = x + CARD_START_X +
                    static_cast<float>(i) * CARD_PITCH;
                if (ix + CARD_WIDTH < x - 40.f || ix > x + w + 40.f)
                    continue;
                const float shimmer = 0.5f + 0.5f *
                    std::sin(m_time * 3.f + static_cast<float>(i) * 0.7f);
                nvgSave(vg);
                nvgGlobalAlpha(vg, pageHeight);
                const float coverY = cardY + 45.f;
                nvgTranslate(vg, ix + CARD_WIDTH * 0.5f,
                             coverY + CARD_WIDTH * 0.5f);
                nvgScale(vg, 1.f, std::max(0.001f, pageHeight));
                nvgTranslate(vg, -(ix + CARD_WIDTH * 0.5f),
                             -(coverY + CARD_WIDTH * 0.5f));
                nvgBeginPath(vg);
                nvgRoundedRect(vg, ix, coverY,
                               CARD_WIDTH, CARD_WIDTH, 15.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(12.f + shimmer * 14.f)));
                nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(45.f + shimmer * 28.f)));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
                nvgRestore(vg);
            }
            return;
        }

        const float reflowEased = easeOutCubic(m_reflowProgress);
        for (int i = 0; i < HOME_CARD_SLOTS; ++i) {
            const float delay = std::min(0.30f, static_cast<float>(i) * 0.045f);
            const float entrance = clamp01((m_contentEntrance - delay) /
                std::max(0.01f, 1.f - delay));
            float reflowX = 0.f;
            if (m_reflowRunning && i >= m_reflowStartIndex &&
                static_cast<size_t>(i) < m_games.size())
                reflowX = (1.f - reflowEased) * CARD_PITCH;
            const float ix = x + CARD_START_X +
                static_cast<float>(i) * CARD_PITCH - m_scrollX + reflowX;
            if (ix + CARD_WIDTH < x - 40.f || ix > x + w + 40.f)
                continue;
            if ((m_deleteWaiting || m_deleteCollapsing) && i == m_deleteIndex) {
                _drawDeletingCard(vg, ix, cardY, CARD_WIDTH,
                                  CARD_HEIGHT, entrance);
            } else if (static_cast<size_t>(i) < m_games.size()) {
                _drawGameCard(vg, m_games[static_cast<size_t>(i)], i,
                              ix, cardY, CARD_WIDTH, CARD_HEIGHT, entrance);
            } else {
                float blankScale = 1.f;
                if (m_reflowRunning && i == m_newBlankIndex)
                    blankScale = easeOutBack(m_reflowProgress);
                _drawEmptyCard(vg, i, ix, cardY, CARD_WIDTH, CARD_HEIGHT,
                               entrance, blankScale);
            }
        }
    }

    void SwitchLayout::_drawEmptyCard(NVGcontext* vg, int index,
                                      float x, float y,
                                      float w, float h, float entrance,
                                      float scale)
    {
        const float coverY = y + 45.f;
        const float heightScale = m_exitAnimationRunning
            ? smoothStep(m_pageEntrance)
            : easeOutCubic(m_pageEntrance);
        nvgSave(vg);
        nvgGlobalAlpha(vg, easeOutCubic(entrance) * heightScale);
        nvgTranslate(vg, x + w * 0.5f, coverY + w * 0.5f);
        nvgScale(vg, std::max(0.001f, scale),
                 std::max(0.001f, scale * heightScale));
        nvgTranslate(vg, -(x + w * 0.5f), -(coverY + w * 0.5f));

        NVGpaint cardShadow = nvgBoxGradient(
            vg, x + 4.f, coverY + 5.f, w, w,
            15.f, 5.f, nvgRGBA(0, 0, 0, 82), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, x - 1.f, coverY, w + 10.f, w + 11.f);
        nvgRoundedRect(vg, x, coverY, w, w, 15.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, cardShadow);
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, coverY, w, w, 15.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 5));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 42));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        const bool selected = m_focusRow == FocusRow::GAMES &&
            index == m_selectedGame;
        const float focus = index >= 0 &&
            static_cast<size_t>(index) < m_slotFocus.size()
            ? m_slotFocus[static_cast<size_t>(index)]
            : 0.f;
        if (selected) {
            beiklive::ui::drawGradientFocusBorder(
                vg, x - 1.f, coverY - 1.f, w + 2.f, w + 2.f,
                12.f, 6.f, focus,
                beiklive::ui::gradientFocusAnimationOffset(m_time));
        }
        nvgRestore(vg);
    }

    void SwitchLayout::_drawDeletingCard(NVGcontext* vg, float x, float y,
                                         float w, float h, float entrance)
    {
        const float coverY = y + 45.f;
        const float collapse = m_deleteCollapsing
            ? 1.f - easeOutCubic(m_deleteCollapseProgress)
            : 1.f;
        const float pageAlpha = m_exitAnimationRunning
            ? smoothStep(m_pageEntrance)
            : easeOutCubic(m_pageEntrance);
        const float shakeX = m_deleteWaiting
            ? std::sin(m_deleteAnimationTime * 58.f) * 7.f
            : 0.f;
        const float shakeY = m_deleteWaiting
            ? std::cos(m_deleteAnimationTime * 47.f) * 2.5f
            : 0.f;
        nvgSave(vg);
        nvgGlobalAlpha(vg, easeOutCubic(entrance) * pageAlpha);
        nvgTranslate(vg, x + w * 0.5f, coverY + w * 0.5f);
        nvgScale(vg, std::max(0.001f, collapse),
                 std::max(0.001f, collapse));
        nvgTranslate(vg, -(x + w * 0.5f), -(coverY + w * 0.5f));
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + shakeX, coverY + shakeY, w, w, 15.f);
        nvgFillColor(vg, nvgRGBA(210, 65, 65, 30));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(240, 105, 105, 205));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
        _drawMaterialIcon(vg, beiklive::material::DELETE_ICON,
                          x + w * 0.5f + shakeX,
                          coverY + w * 0.5f + shakeY,
                          58.f, nvgRGBA(246, 130, 130, 245));
        nvgRestore(vg);
    }

    void SwitchLayout::_drawGameCard(NVGcontext* vg, const HomeGame& game,
                                     int index, float x, float y,
                                     float w, float h, float entrance)
    {
        const float focus = index >= 0 &&
            static_cast<size_t>(index) < m_slotFocus.size()
            ? m_slotFocus[static_cast<size_t>(index)]
            : 0.f;
        const float contentAlpha = easeOutCubic(entrance);
        const float heightScale = m_exitAnimationRunning
            ? smoothStep(m_pageEntrance)
            : easeOutCubic(m_pageEntrance);
        const float coverX = x;
        const float coverY = y + 45.f;
        const bool selected = m_focusRow == FocusRow::GAMES &&
            index == m_selectedGame;
        const float textEased = selected ? easeOutCubic(focus) : 0.f;

        nvgSave(vg);
        nvgGlobalAlpha(vg, contentAlpha * heightScale);
        nvgTranslate(vg, x + w * 0.5f, y + h * 0.5f);
        nvgScale(vg, 1.f, std::max(0.001f, heightScale));
        nvgTranslate(vg, -(x + w * 0.5f), -(y + h * 0.5f));

        NVGpaint cardShadow = nvgBoxGradient(
            vg, coverX + 4.f, coverY + 5.f, w, w,
            15.f, 5.f, nvgRGBA(0, 0, 0, 82), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, coverX - 1.f, coverY, w + 10.f, w + 11.f);
        nvgRoundedRect(vg, coverX, coverY, w, w, 15.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, cardShadow);
        nvgFill(vg);

        nvgBeginPath(vg);
        nvgRoundedRect(vg, coverX, coverY, w, w, 15.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 10));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 70));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        _drawCover(vg, game.entry.logoPath, coverX, coverY, w, w, 1.f);

        const float titleX = x - w * 0.5f;
        const float titleY = coverY - 42.f + (1.f - textEased) * 8.f;
        const float titleW = w * 2.f;
        const float titleCenterX = x + w * 0.5f;
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 24.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(250, 251, 255,
            static_cast<unsigned char>(245.f * textEased)));
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, game.entry.title.c_str(), nullptr, bounds);
        const float textW = bounds[2] - bounds[0];
        float marqueeOffset = 0.f;
        if (focus > 0.5f && textW > titleW) {
            const float overflow = textW - titleW;
            const float cycle = std::fmod(m_time * 30.f, overflow + 70.f);
            marqueeOffset = overflow * 0.5f -
                std::max(0.f, std::min(overflow, cycle - 25.f));
        }
        nvgSave(vg);
        nvgIntersectScissor(vg, titleX, titleY, titleW, 32.f);
        nvgText(vg, titleCenterX + marqueeOffset, titleY,
                game.entry.title.c_str(), nullptr);
        nvgRestore(vg);

        const std::string badgeText =
            beiklive::tools::platformBadgeName(game.entry.platform);
        const std::string playTimeText = game.entry.playTime > 0
            ? beiklive::tools::formatPlayTime(game.entry.playTime)
            : L("未游玩");
        const std::string lastPlayed = game.entry.lastPlayed.empty()
            ? L("上次游玩：从未")
            : L("上次游玩：") +
                beiklive::tools::formatTimestampForDisplay(game.entry.lastPlayed);
        const float firstLineProgress = selected
            ? smoothStep(clamp01(focus / 0.72f))
            : 0.f;
        const float secondLineProgress = selected
            ? smoothStep(clamp01((focus - 0.18f) / 0.82f))
            : 0.f;
        const float firstLineX = x + (1.f - firstLineProgress) * 34.f;
        const float firstLineY = coverY + w + 14.f;
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 12.f);
        float badgeBounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, badgeText.c_str(), nullptr, badgeBounds);
        const float badgeWidth = std::max(36.f, (badgeBounds[2] - badgeBounds[0]) + 16.f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, firstLineX, firstLineY, badgeWidth, 20.f, 4.f);
        NVGcolor badgeColor = platformBadgeColor(game.entry.platform);
        badgeColor.a *= firstLineProgress;
        nvgFillColor(vg, badgeColor);
        nvgFill(vg);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255,
            static_cast<unsigned char>(255.f * firstLineProgress)));
        nvgText(vg, firstLineX + badgeWidth * .5f, firstLineY + 10.f,
                badgeText.c_str(), nullptr);
        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(211, 219, 233,
            static_cast<unsigned char>(225.f * firstLineProgress)));
        nvgText(vg, firstLineX + badgeWidth + 10.f, firstLineY + 10.f,
                playTimeText.c_str(), nullptr);

        const float secondLineX = x + (1.f - secondLineProgress) * 34.f;
        const float secondLineY = coverY + w + 43.f;
        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(190, 200, 218,
            static_cast<unsigned char>(205.f * secondLineProgress)));
        nvgText(vg, secondLineX, secondLineY,
                lastPlayed.c_str(), nullptr);

        if (selected) {
            beiklive::ui::drawGradientFocusBorder(
                vg, coverX - 2.f, coverY - 2.f, w + 4.f, w + 4.f,
                12.f, 5.f, focus,
                beiklive::ui::gradientFocusAnimationOffset(m_time));
        }
        nvgRestore(vg);
    }

    void SwitchLayout::_drawCover(NVGcontext* vg, const std::string& path,
                                  float x, float y, float w, float h,
                                  float alpha)
    {
        auto found = m_textureCache.find(path);
        if (found == m_textureCache.end() || found->second <= 0) {
            const float shimmer = 0.5f + 0.5f * std::sin(m_time * 3.2f);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x, y, w, h, 10.f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>((14.f + shimmer * 12.f) * alpha)));
            nvgFill(vg);
            _drawMaterialIcon(vg, beiklive::material::IMAGE_PLACEHOLDER,
                              x + w * 0.5f, y + h * 0.5f, 44.f,
                              nvgRGBA(220, 226, 238,
                                  static_cast<unsigned char>(90.f * alpha)));
            return;
        }

        int imageW = 0;
        int imageH = 0;
        nvgImageSize(vg, found->second, &imageW, &imageH);
        if (imageW <= 0 || imageH <= 0)
            return;
        const float scale = std::max(
            w / static_cast<float>(imageW),
            h / static_cast<float>(imageH));
        const float drawW = static_cast<float>(imageW) * scale;
        const float drawH = static_cast<float>(imageH) * scale;
        const float drawX = x + (w - drawW) * 0.5f;
        const float drawY = y + (h - drawH) * 0.5f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 10.f);
        NVGpaint paint = nvgImagePattern(vg, drawX, drawY, drawW, drawH,
                                         0.f, found->second, alpha);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    }

    void SwitchLayout::_drawFunctions(NVGcontext* vg, float x, float y,
                                      float w, float h)
    {
        if (m_functions.empty())
            return;
        const float pitch = 100.f;
        const float barW = pitch * static_cast<float>(m_functions.size());
        const float barH = 95.f;
        const float barX = x + w * 0.5f - barW * 0.5f;
        const float finalBarY = y + h - 90.f - barH;
        const float pageEased = m_exitAnimationRunning
            ? smoothStep(m_pageEntrance)
            : easeOutBack(m_pageEntrance);
        const float barY = finalBarY + (1.f - pageEased) *
            (y + h - finalBarY + 18.f);
        const float centerY = barY + barH * 0.5f;
        const float barAlpha = easeOutCubic(m_pageEntrance);

        nvgSave(vg);
        nvgGlobalAlpha(vg, barAlpha);
        NVGpaint barShadow = nvgBoxGradient(
            vg, barX + 4.f, barY + 5.f, barW, barH,
            barH * 0.5f, 5.f,
            nvgRGBA(0, 0, 0, 88), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, barX - 1.f, barY, barW + 10.f, barH + 11.f);
        nvgRoundedRect(vg, barX, barY, barW, barH, barH * 0.5f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, barShadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, barX, barY, barW, barH, barH * 0.5f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 12));
        nvgFill(vg);

        constexpr float barStrokeWidth = 1.f;
        constexpr float barStrokeInset = barStrokeWidth * 0.5f;
        nvgBeginPath(vg);
        nvgRoundedRect(
            vg, barX + barStrokeInset, barY + barStrokeInset,
            barW - barStrokeWidth, barH - barStrokeWidth,
            barH * 0.5f - barStrokeInset);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 62));
        nvgStrokeWidth(vg, barStrokeWidth);
        nvgStroke(vg);
        nvgRestore(vg);

        for (size_t i = 0; i < m_functions.size(); ++i) {
            auto& function = m_functions[i];
            if (function.imageHandle == 0 && !function.imagePath.empty())
                function.imageHandle = nvgCreateImage(
                    vg, function.imagePath.c_str(), 0);
            const float focus = i < m_functionFocus.size()
                ? m_functionFocus[i]
                : 0.f;
            const bool selectedFunction =
                m_focusRow == FocusRow::FUNCTIONS &&
                static_cast<int>(i) == m_selectedFunction;
            const float segmentX = barX + static_cast<float>(i) * pitch;
            const float cx = segmentX + pitch * 0.5f;
            float clickScale = 1.f;
            if (m_functionClickAnimating &&
                static_cast<int>(i) == m_functionClickIndex) {
                if (m_functionClickTime < 0.06f) {
                    clickScale = 1.f - 0.12f *
                        (m_functionClickTime / 0.06f);
                } else {
                    const float t = m_functionClickTime - 0.06f;
                    clickScale = 1.f + 0.14f * std::exp(-14.f * t) *
                        std::sin(45.f * t);
                }
            }
            const float scale = (0.94f + 0.06f * focus) * clickScale;
            const float iconY = centerY;

            nvgSave(vg);
            nvgGlobalAlpha(vg, barAlpha);
            nvgTranslate(vg, cx, iconY);
            nvgScale(vg, scale, scale);
            nvgTranslate(vg, -cx, -iconY);

            if (function.imageHandle > 0) {
                int imageW = 0;
                int imageH = 0;
                nvgImageSize(vg, function.imageHandle, &imageW, &imageH);
                if (imageW > 0 && imageH > 0) {
                    const float aspect = static_cast<float>(imageW) /
                        static_cast<float>(imageH);
                    float drawW = 45.f;
                    float drawH = drawW / aspect;
                    if (drawH > 45.f) {
                        drawH = 45.f;
                        drawW = drawH * aspect;
                    }
                    const float drawX = cx - drawW * 0.5f;
                    const float drawY = iconY - drawH * 0.5f;
                    nvgBeginPath(vg);
                    nvgRect(vg, drawX, drawY, drawW, drawH);
                    NVGpaint paint = nvgImagePattern(
                        vg, drawX, drawY, drawW, drawH, 0.f,
                        function.imageHandle, 1.f);
                    nvgFillPaint(vg, paint);
                    nvgFill(vg);
                }
            }

            if (selectedFunction) {
                constexpr float focusSize = 91.f;
                beiklive::ui::drawGradientFocusCircle(
                    vg, cx, centerY, focusSize, 6.f, focus,
                    beiklive::ui::gradientFocusAnimationOffset(m_time));
            }

            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 20.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
            nvgFillColor(vg, nvgRGBA(242, 245, 251,
                static_cast<unsigned char>(245.f * focus)));
            nvgText(vg, cx, barY + barH + 10.f,
                    function.label.c_str(), nullptr);
            nvgRestore(vg);
        }
    }

    void SwitchLayout::_drawFooterHint(NVGcontext* vg, float x, float y,
                                       float w, float h)
    {
        const float eased = easeOutCubic(m_pageEntrance);
        const float cy = y + h - 35.f;
        const std::string label =
            m_focusRow == FocusRow::GAMES ? L("选择") : L("打开");
        auto drawHint = [&](brls::ControllerButton button,
                            const std::string& text,
                            float& cursor) {
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 20.f);
            float bounds[4]{};
            nvgTextBounds(vg, 0.f, 0.f, text.c_str(), nullptr, bounds);
            const float textWidth = bounds[2] - bounds[0];
            const float labelX = cursor - textWidth;
            const std::string glyph = brls::Hint::getKeyIcon(button);
            nvgFontFaceId(vg, m_switchIconFontId);
            nvgFontSize(vg, 29.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(245.f * eased)));
            nvgText(vg, labelX - 20.f, cy, glyph.c_str(), nullptr);
            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 20.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgText(vg, labelX, cy, text.c_str(), nullptr);
            cursor = labelX - 58.f;
        };
        float hintCursor = x + w - 30.f;
        drawHint(brls::BUTTON_A, label, hintCursor);
        drawHint(brls::BUTTON_START, L("设置主页"), hintCursor);

        const char32_t networkCodepoint = m_networkConnected
            ? beiklive::material::WIFI
            : beiklive::material::WIFI_OFF;
        const std::string networkGlyph = encodeUtf8(networkCodepoint);
        constexpr float networkIconX = 30.f;
        constexpr float networkIconSize = 26.f;
        constexpr float networkClockGap = 20.f;
        nvgFontFaceId(vg, m_materialFontId);
        nvgFontSize(vg, networkIconSize);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, m_networkConnected
            ? nvgRGBA(255, 255, 255, static_cast<unsigned char>(245.f * eased))
            : nvgRGBA(155, 160, 170, static_cast<unsigned char>(210.f * eased)));
        nvgText(vg, x + networkIconX, cy, networkGlyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 19.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(245, 247, 251,
            static_cast<unsigned char>(235.f * eased)));
        nvgText(vg, x + networkIconX + networkIconSize * 0.5f +
                    networkClockGap,
                cy, m_clockText.c_str(), nullptr);
    }

    void SwitchLayout::_drawPico8Shortcut(NVGcontext* vg, float x, float y,
                                          float w, float h)
    {
        if (!m_pico8ShortcutVisible &&
            !m_pico8ExitAnimationRunning &&
            !m_pico8ReturnAnimationRunning)
            return;

        if (m_pico8LogoImageHandle == 0)
            m_pico8LogoImageHandle = nvgCreateImage(
                vg, BK_RES("img/pico8_logo_vector.png").c_str(), 0);

        const auto geometry = beiklive::pico8_transition::geometry(
            0.f, 0.f, brls::Application::contentWidth,
            brls::Application::contentHeight);
        const float pageEased = m_exitAnimationRunning
            ? smoothStep(m_pageEntrance)
            : easeOutBack(m_pageEntrance);
        float shortcutX = geometry.shortcutX +
            (pageEased - 1.f) *
                (geometry.shortcutWidth + 16.f);
        float frameAlpha = easeOutCubic(m_pageEntrance);
        auto logoPose = beiklive::pico8_transition::logoPose(geometry, 0.f);
        logoPose.x = shortcutX +
            beiklive::pico8_transition::SHORTCUT_LOGO_X;

        if (m_pico8ExitAnimationRunning || m_pico8ReturnAnimationRunning) {
            const float transition = m_pico8TransitionProgress;
            shortcutX = geometry.shortcutX;
            frameAlpha = 1.f - smoothStep(clamp01(
                m_pico8TransitionProgress / 0.58f));
            logoPose = beiklive::pico8_transition::logoPose(
                geometry, transition);
        }

        const float interactionScale =
            (m_pico8ExitAnimationRunning || m_pico8ReturnAnimationRunning)
                ? 1.f
                : m_pico8ShortcutScale;
        nvgSave(vg);
        if (std::abs(interactionScale - 1.f) > 0.0001f) {
            nvgTranslate(vg, shortcutX, geometry.keyCenterY);
            nvgScale(vg, interactionScale, interactionScale);
            nvgTranslate(vg, -shortcutX, -geometry.keyCenterY);
        }

        if (frameAlpha > 0.001f) {
            nvgSave(vg);
            nvgGlobalAlpha(vg, frameAlpha);
            const float radius = geometry.shortcutHeight * 0.5f;
            const NVGpaint shadow = nvgBoxGradient(
                vg, shortcutX + 4.f, geometry.shortcutY + 5.f,
                geometry.shortcutWidth, geometry.shortcutHeight,
                radius, 5.f, nvgRGBA(0, 0, 0, 88),
                nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, shortcutX - 1.f, geometry.shortcutY,
                    geometry.shortcutWidth + 10.f,
                    geometry.shortcutHeight + 11.f);
            nvgRoundedRectVarying(
                vg, shortcutX, geometry.shortcutY,
                geometry.shortcutWidth, geometry.shortcutHeight,
                0.f, radius, radius, 0.f);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, shadow);
            nvgFill(vg);

            nvgBeginPath(vg);
            nvgRoundedRectVarying(
                vg, shortcutX, geometry.shortcutY,
                geometry.shortcutWidth, geometry.shortcutHeight,
                0.f, radius, radius, 0.f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 12));
            nvgFill(vg);

            constexpr float strokeWidth = 1.f;
            constexpr float inset = strokeWidth * 0.5f;
            nvgBeginPath(vg);
            nvgRoundedRectVarying(
                vg, shortcutX + inset, geometry.shortcutY + inset,
                geometry.shortcutWidth - strokeWidth,
                geometry.shortcutHeight - strokeWidth,
                0.f, radius - inset, radius - inset, 0.f);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 62));
            nvgStrokeWidth(vg, strokeWidth);
            nvgStroke(vg);

            const std::string glyph = brls::Hint::getKeyIcon(brls::BUTTON_LB);
            nvgFontFaceId(vg, m_switchIconFontId);
            nvgFontSize(vg, 30.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 245));
            nvgText(vg, shortcutX +
                        beiklive::pico8_transition::SHORTCUT_L_CENTER_X,
                    geometry.keyCenterY, glyph.c_str(), nullptr);
            nvgRestore(vg);
        }

        const bool pageOwnsReturningLogo = m_pico8ReturnAnimationRunning;
        if (!pageOwnsReturningLogo && m_pico8LogoImageHandle > 0) {
            const float centerX = logoPose.x + logoPose.width * 0.5f;
            const float centerY = logoPose.y + logoPose.height * 0.5f;
            nvgSave(vg);
            nvgTranslate(vg, centerX, centerY);
            nvgRotate(vg, logoPose.rotation);
            nvgTranslate(vg, -centerX, -centerY);
            nvgBeginPath(vg);
            nvgRect(vg, logoPose.x, logoPose.y,
                    logoPose.width, logoPose.height);
            const NVGpaint logoPaint = nvgImagePattern(
                vg, logoPose.x, logoPose.y,
                logoPose.width, logoPose.height, 0.f,
                m_pico8LogoImageHandle, 1.f);
            nvgFillPaint(vg, logoPaint);
            nvgFill(vg);
            nvgRestore(vg);
        }
        nvgRestore(vg);
    }

    void SwitchLayout::_drawMaterialIcon(NVGcontext* vg, char32_t icon,
                                         float x, float y, float size,
                                         NVGcolor color)
    {
        if (m_materialFontId < 0)
            return;
        const std::string text = encodeUtf8(icon);
        nvgFontFaceId(vg, m_materialFontId);
        nvgFontSize(vg, size);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, color);
        nvgText(vg, x, y, text.c_str(), nullptr);
    }

    void SwitchLayout::_resetTextureRequests()
    {
        if (!m_textureLoader)
            return;
        std::unordered_set<std::string> wanted;
        for (const auto& game : m_games) {
            if (!game.entry.logoPath.empty())
                wanted.insert(game.entry.logoPath);
        }
        {
            std::lock_guard<std::mutex> lock(m_textureLoader->mutex);
            ++m_textureLoader->generation;
            m_textureLoader->wanted = std::move(wanted);
            m_textureLoader->ready.clear();
            m_textureLoader->pending.clear();
        }
        m_failedTextures.clear();
        m_textureCacheDirty = true;
    }

    void SwitchLayout::_requestTexturesByPriority()
    {
        if (m_games.empty())
            return;
        const int count = static_cast<int>(m_games.size());
        auto requestIndex = [this, count](int index) {
            if (index < 0 || index >= count)
                return;
            const auto& path = m_games[static_cast<size_t>(index)].entry.logoPath;
            if (!path.empty() && m_textureCache.count(path) == 0 &&
                m_failedTextures.count(path) == 0)
                _requestTexture(path);
        };
        requestIndex(m_selectedGame);
        for (int distance = 1; distance <= 3; ++distance) {
            requestIndex(m_selectedGame + distance);
            requestIndex(m_selectedGame - distance);
        }
        for (int i = 0; i < count; ++i)
            requestIndex(i);
    }

    void SwitchLayout::_requestTexture(const std::string& path)
    {
        if (path.empty() || !m_textureLoader ||
            !m_textureLoader->alive.load())
            return;
        auto state = m_textureLoader;
        uint64_t generation = 0;
        {
            std::lock_guard<std::mutex> lock(state->mutex);
            generation = state->generation;
            auto pending = state->pending.find(path);
            if (pending != state->pending.end() && pending->second == generation)
                return;
            const bool alreadyReady = std::any_of(
                state->ready.begin(), state->ready.end(),
                [&path, generation](const DecodedTexture& decoded) {
                    return decoded.path == path &&
                        decoded.generation == generation;
                });
            if (alreadyReady)
                return;
#ifdef __SWITCH__
            constexpr int maxActiveDecodes = 1;
#else
            constexpr int maxActiveDecodes = 2;
#endif
            if (state->activeDecodes >= maxActiveDecodes)
                return;
            state->pending[path] = generation;
            ++state->activeDecodes;
        }

        beiklive::ThreadPool::instance().enqueue([state, path, generation]() {
            {
                std::lock_guard<std::mutex> lock(state->mutex);
                if (!state->alive.load() || state->generation != generation ||
                    state->wanted.count(path) == 0) {
                    auto pending = state->pending.find(path);
                    if (pending != state->pending.end() &&
                        pending->second == generation)
                        state->pending.erase(pending);
                    state->activeDecodes = std::max(0, state->activeDecodes - 1);
                    return;
                }
            }

            DecodedTexture decoded;
            decoded.path = path;
            decoded.generation = generation;
            int width = 0;
            int height = 0;
            int channels = 0;
            bool dimensionsValid = stbi_info(
                path.c_str(), &width, &height, &channels) != 0;
#ifdef __SWITCH__
            constexpr int64_t maxSourcePixels = 20000000;
#else
            constexpr int64_t maxSourcePixels = 40000000;
#endif
            dimensionsValid = dimensionsValid && width > 0 && height > 0 &&
                static_cast<int64_t>(width) * static_cast<int64_t>(height) <=
                    maxSourcePixels;
            unsigned char* pixels = dimensionsValid
                ? stbi_load(path.c_str(), &width, &height, &channels, 4)
                : nullptr;
            if (!pixels || width <= 0 || height <= 0) {
                decoded.failed = true;
                if (pixels)
                    stbi_image_free(pixels);
            } else {
#ifdef __SWITCH__
                constexpr int maxEdge = 384;
#else
                constexpr int maxEdge = 512;
#endif
                const int longest = std::max(width, height);
                if (longest > maxEdge) {
                    const float scale = static_cast<float>(maxEdge) /
                        static_cast<float>(longest);
                    decoded.width = std::max(1, static_cast<int>(
                        std::round(static_cast<float>(width) * scale)));
                    decoded.height = std::max(1, static_cast<int>(
                        std::round(static_cast<float>(height) * scale)));
                    decoded.pixels = resizeRgba(
                        pixels, width, height, decoded.width, decoded.height);
                } else {
                    decoded.width = width;
                    decoded.height = height;
                    decoded.pixels.assign(
                        pixels,
                        pixels + static_cast<size_t>(width) *
                            static_cast<size_t>(height) * 4);
                }
                stbi_image_free(pixels);
            }

            std::lock_guard<std::mutex> lock(state->mutex);
            auto pending = state->pending.find(path);
            if (pending != state->pending.end() &&
                pending->second == generation)
                state->pending.erase(pending);
            state->activeDecodes = std::max(0, state->activeDecodes - 1);
            if (state->alive.load() && state->generation == generation &&
                state->wanted.count(path) != 0)
                state->ready.push_back(std::move(decoded));
        });
    }

    void SwitchLayout::_uploadDecodedTextures(NVGcontext* vg)
    {
        if (!vg || !m_textureLoader)
            return;
#ifdef __SWITCH__
        constexpr int maxUploads = 2;
#else
        constexpr int maxUploads = 4;
#endif
        for (int i = 0; i < maxUploads; ++i) {
            DecodedTexture decoded;
            {
                std::lock_guard<std::mutex> lock(m_textureLoader->mutex);
                if (m_textureLoader->ready.empty())
                    break;
                decoded = std::move(m_textureLoader->ready.front());
                m_textureLoader->ready.pop_front();
                if (decoded.generation != m_textureLoader->generation ||
                    m_textureLoader->wanted.count(decoded.path) == 0)
                    continue;
            }
            if (decoded.failed || decoded.pixels.empty()) {
                m_failedTextures.insert(decoded.path);
                continue;
            }
            if (m_textureCache.count(decoded.path) != 0)
                continue;
            const int handle = nvgCreateImageRGBA(
                vg, decoded.width, decoded.height, 0, decoded.pixels.data());
            if (handle > 0)
                m_textureCache[decoded.path] = handle;
            else
                m_failedTextures.insert(decoded.path);
        }
    }

    void SwitchLayout::_evictUnusedTextures(NVGcontext* vg)
    {
        if (!vg || !m_textureCacheDirty)
            return;
        std::unordered_set<std::string> wanted;
        for (const auto& game : m_games) {
            if (!game.entry.logoPath.empty())
                wanted.insert(game.entry.logoPath);
        }
        for (auto it = m_textureCache.begin(); it != m_textureCache.end();) {
            if (wanted.count(it->first) == 0 &&
                it->first != m_pinnedTexturePath) {
                if (it->second > 0)
                    nvgDeleteImage(vg, it->second);
                it = m_textureCache.erase(it);
            } else {
                ++it;
            }
        }
        for (auto it = m_failedTextures.begin(); it != m_failedTextures.end();) {
            if (wanted.count(*it) == 0)
                it = m_failedTextures.erase(it);
            else
                ++it;
        }
        m_textureCacheDirty = false;
    }
} // namespace beiklive
