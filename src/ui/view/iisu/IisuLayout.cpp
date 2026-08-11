#include "IisuLayout.hpp"

#include "core/Translation.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "WidgetFactory.hpp"

#include <algorithm>
#include <cmath>

namespace
{
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
} // namespace

namespace beiklive
{
    IisuLayout::IisuLayout() : Layout(), m_editor(&m_uiContext.layout())
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
        m_lastFrameTime = std::chrono::steady_clock::now();

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

        // 占位阶段：吞掉所有方向键/确认键，避免焦点泄漏到其他视图
        auto consume = [](brls::View*) -> bool { return true; };
        registerAction("", brls::BUTTON_A, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_B, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_UP, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_DOWN, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_UP, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_DOWN, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LEFT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RIGHT, consume, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LB, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RB, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_LT, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RT, consume, true, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_X, consume, true, false, brls::SOUND_NONE);
        _captureInputState();

        // 优先加载保存的布局 JSON，失败时回退默认演示布局
        if (m_uiContext.loadMainPageFromFile("home.json"))
            return;

        // 默认布局：GameCover（第一条游戏）+ 图片 + LiveWidget + 平台文件夹
        std::string coverGameId;
        std::vector<int> platforms;
        if (beiklive::GameDB) {
            const auto all = beiklive::GameDB->getAll();
            if (!all.empty())
                coverGameId = all.front().path;
            for (const auto& entry : all) {
                if (entry.platform <= 0)
                    continue;
                if (std::find(platforms.begin(), platforms.end(),
                              entry.platform) == platforms.end())
                    platforms.push_back(entry.platform);
            }
        }

        std::vector<beiklive::FolderItemDescriptor> mainPage;
        const std::string logoPath = BK_RES("img/pico8_logo_vector.png");
        if (coverGameId.empty())
            mainPage.push_back({WidgetType::Image, "", logoPath,
                                0, 0, 2, 2, true});
        else
            mainPage.push_back({WidgetType::GameCover, coverGameId, "",
                                0, 0, 2, 2, true});
        mainPage.push_back({WidgetType::Image, "", logoPath,
                            2, 0, 1, 1, true});
        mainPage.push_back({WidgetType::Image, "",
                            BK_RES("img/ui/light/GameList_64.png"),
                            2, 1, 1, 1, true});
        mainPage.push_back({WidgetType::Live, "clock", "",
                            2, 2, 1, 1, true});
        mainPage.push_back({WidgetType::Live, "system_info", "",
                            3, 2, 1, 1, true});
        mainPage.push_back({WidgetType::Live, "recent_games", "",
                            4, 2, 2, 1, true});
        for (size_t i = 0; i < std::min<size_t>(2, platforms.size()); ++i)
            mainPage.push_back({WidgetType::Folder,
                                "platform:" + std::to_string(platforms[i]),
                                "", 4, static_cast<int>(i), 2, 1, true});
        m_uiContext.setMainPage(mainPage);
    }

    IisuLayout::~IisuLayout()
    {
        releaseSelectedCoverTexture();
        m_uiContext.textures().clear(brls::Application::getNVGContext());
        if (auto* vg = brls::Application::getNVGContext()) {
            for (const auto& function : m_functions) {
                if (function.imageHandle > 0)
                    nvgDeleteImage(vg, function.imageHandle);
            }
        }
    }

    void IisuLayout::refreshGameList(beiklive::GameList gameList)
    {
        m_games = std::move(gameList);
        invalidate();
    }

    void IisuLayout::restoreCardFocus(bool /*animated*/)
    {
        brls::Application::giveFocus(this);
    }

    void IisuLayout::resetCardFocusToFirst()
    {
        restoreCardFocus(false);
    }

    void IisuLayout::removeGameByPath(const std::string& path)
    {
        if (path.empty() || isDeleteAnimationRunning())
            return;

        auto findTarget = [&path](LayoutManager& layout)
            -> std::optional<size_t> {
            const auto& items = layout.items();
            for (size_t i = 0; i < items.size(); ++i) {
                const auto& item = items[i];
                if (item.widget && item.widget->typeName() == "game_cover" &&
                    item.widget->dataId() == path)
                    return i;
            }
            return std::nullopt;
        };

        LayoutManager* layout = &m_uiContext.layout();
        auto index = findTarget(*layout);
        if (!index && m_uiContext.isFolderOpen()) {
            layout = &m_uiContext.panelLayout();
            index = findTarget(*layout);
        }
        if (!index)
            return;

        m_deleteLayout = layout;
        m_deleteIndex = *index;
        m_deletePath = path;
        auto& item = m_deleteLayout->items()[m_deleteIndex];
        m_uiContext.animations().add({
            0.20f, 0.f,
            [&item](float t) {
                item.transform.alpha = 1.f - t;
                item.transform.scale = 1.f - t * 0.18f;
            },
            { }, false,
        });
        invalidate();
    }

    void IisuLayout::completeGameRemoval(std::function<void()> completion)
    {
        if (!isDeleteAnimationRunning()) {
            if (completion)
                completion();
            return;
        }

        // 后端完成得很快时，先取消等待阶段的抖动动画，避免其继续引用
        // 即将从 vector 删除的 LayoutItem。
        m_uiContext.animations().clear();
        if (m_deleteIndex < m_deleteLayout->items().size()) {
            auto& item = m_deleteLayout->items()[m_deleteIndex];
            item.transform.alpha = 1.f;
            item.transform.scale = 1.f;
        }
        m_deleteCompletion = std::move(completion);
        LayoutManager* layout = m_deleteLayout;
        const size_t index = m_deleteIndex;
        const std::string path = m_deletePath;
        m_uiContext.animations().add({
            0.16f, 0.f, { },
            [this, layout, index, path]() {
                if (layout && index < layout->items().size()) {
                    const auto& item = layout->items()[index];
                    if (item.widget && item.widget->dataId() == path)
                        layout->removeItem(index);
                    layout->resetFocusToFirst();
                }
                m_deleteLayout = nullptr;
                m_deletePath.clear();
                if (m_deleteCompletion) {
                    auto done = std::move(m_deleteCompletion);
                    m_deleteCompletion = nullptr;
                    done();
                }
                invalidate();
            }, false,
        });
    }

    void IisuLayout::cancelGameRemoval()
    {
        if (!m_deleteLayout)
            return;
        if (m_deleteIndex < m_deleteLayout->items().size()) {
            auto& item = m_deleteLayout->items()[m_deleteIndex];
            item.transform.alpha = 1.f;
            item.transform.scale = 1.f;
        }
        m_uiContext.animations().clear();
        m_deleteLayout = nullptr;
        m_deletePath.clear();
        m_deleteCompletion = nullptr;
        invalidate();
    }

    int IisuLayout::acquireSelectedCoverTexture()
    {
        const auto entry = _currentGameEntry();
        if (!entry || entry->logoPath.empty())
            return -1;
        if (!m_selectedCoverPath.empty() &&
            m_selectedCoverPath != entry->logoPath)
            return -1;
        const int texture = m_uiContext.textures().loadTexture(
            brls::Application::getNVGContext(), entry->logoPath);
        if (texture <= 0)
            return -1;
        m_selectedCoverPath = entry->logoPath;
        ++m_selectedCoverReferences;
        return texture;
    }

    void IisuLayout::releaseSelectedCoverTexture()
    {
        if (m_selectedCoverReferences <= 0 || m_selectedCoverPath.empty())
            return;
        m_uiContext.textures().releaseTexture(
            brls::Application::getNVGContext(), m_selectedCoverPath);
        if (--m_selectedCoverReferences == 0)
            m_selectedCoverPath.clear();
    }

    void IisuLayout::playEntranceAnimation()
    {
        m_exitAnimationRunning = false;
        m_exitCompletion = nullptr;
        m_pageOpacity = 0.f;
        m_uiContext.animations().clear();
        _animateEntrance(m_uiContext.layout());
        if (m_uiContext.isFolderOpen())
            _animateEntrance(m_uiContext.panelLayout(), 0.06f);
        m_uiContext.animations().add({
            0.24f, 0.f,
            [this](float t) { m_pageOpacity = t; },
            { }, false,
        });
        _captureInputState();
        invalidate();
    }

    void IisuLayout::playExitAnimation(std::function<void()> completion)
    {
        if (m_exitAnimationRunning) {
            if (completion)
                m_exitCompletion = std::move(completion);
            return;
        }
        m_exitAnimationRunning = true;
        m_exitCompletion = std::move(completion);
        m_uiContext.animations().add({
            0.20f, 0.f,
            [this](float t) { m_pageOpacity = 1.f - t; },
            [this]() {
                m_exitAnimationRunning = false;
                if (m_exitCompletion) {
                    auto done = std::move(m_exitCompletion);
                    m_exitCompletion = nullptr;
                    done();
                }
            }, false,
        });
        _captureInputState();
        invalidate();
    }

    void IisuLayout::playPico8ExitAnimation(std::function<void()> completion)
    {
        brls::Application::blockInputs();
        playExitAnimation(std::move(completion));
    }

    void IisuLayout::beginPico8ReturnAnimation()
    {
        m_exitAnimationRunning = false;
        m_exitCompletion = nullptr;
        m_pageOpacity = 0.f;
        _captureInputState();
        invalidate();
    }

    void IisuLayout::setPico8ReturnProgress(float progress)
    {
        m_pageOpacity = std::clamp(progress, 0.f, 1.f);
        invalidate();
    }

    void IisuLayout::finishPico8ReturnAnimation()
    {
        m_pageOpacity = 1.f;
        _animateEntrance(m_uiContext.layout());
        _captureInputState();
        invalidate();
    }

    void IisuLayout::setPico8ShortcutVisible(bool visible)
    {
        m_pico8ShortcutVisible = visible;
        invalidate();
    }

    void IisuLayout::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;

        m_time += dt;

        for (size_t i = 0; i < m_functionFocus.size(); ++i) {
            const bool selected = m_focusArea == FocusArea::FUNCTIONS &&
                static_cast<int>(i) == m_selectedFunction;
            const float target = selected ? 1.f : 0.f;
            m_functionFocus[i] += (target - m_functionFocus[i]) *
                std::min(1.f, dt * 12.f);
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

        m_uiContext.animations().update(dt);
        _applyEditShake();
        m_uiContext.layout().update(dt);
        if (m_uiContext.isFolderOpen())
            m_uiContext.panelLayout().update(dt);
        _handleInput(dt);
        invalidate();
    }

    void IisuLayout::_applyEditShake()
    {
        auto& items = m_uiContext.layout().items();
        if (m_editor.isActive()) {
            // 编辑模式：所有 Widget 轻微抖动（被抬起的除外）
            LayoutItem* lifted = m_editor.lifted();
            for (auto& item : items) {
                if (!item.visible || &item == lifted)
                    continue;
                const float phase =
                    static_cast<float>(item.x * 7 + item.y * 13);
                item.transform.offsetX =
                    std::sin(m_time * 7.f + phase) * 1.2f;
                item.transform.offsetY =
                    std::cos(m_time * 7.f + phase) * 1.2f;
            }
        } else {
            // 普通模式：偏移归零（注意：若未来动画系统使用 Transform.offset
            // 做位移动画，此处会覆盖其值，需要改为仅清理编辑抖动残留）
            for (auto& item : items) {
                if (item.transform.offsetX != 0.f ||
                    item.transform.offsetY != 0.f) {
                    item.transform.offsetX = 0.f;
                    item.transform.offsetY = 0.f;
                }
            }
        }
    }

    void IisuLayout::_captureInputState()
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
        m_prevB = state.buttons[static_cast<int>(brls::BUTTON_B)];
        // Switch 减号键（Xbox BACK）映射到 BUTTON_BACK
        m_prevMinus = state.buttons[static_cast<int>(brls::BUTTON_BACK)];
    }

    void IisuLayout::_handleInput(float dt)
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
        const bool b = state.buttons[static_cast<int>(brls::BUTTON_B)];
        // Switch 减号键（Xbox BACK）
        const bool minus = state.buttons[static_cast<int>(brls::BUTTON_BACK)];

        if (!isFocused() || brls::Application::isInputBlocks() ||
            m_exitAnimationRunning || isDeleteAnimationRunning() ||
            m_functionClickAnimating) {
            m_holdLeft = m_holdRight = 0.f;
            m_repeatLeft = m_repeatRight = 0.f;
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
            m_prevMinus = minus;
            return;
        }

        // 键盘 - 或手柄减号（BUTTON_BACK）：切换编辑模式
        if (minus && !m_prevMinus) {
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
            m_prevMinus = minus;
            _toggleEditMode();
            return;
        }

        // 卡片操作浮层：独占输入
        if (m_cardPanelOpen) {
            _handleCardPanelInput(dt);
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
            m_prevMinus = minus;
            return;
        }

        // 卡片编辑面板：左/右调整 GIF 速度，B 返回
        if (m_cardEditOpen) {
            if (left && !m_prevLeft)
                _adjustCardSpeed(-1);
            if (right && !m_prevRight)
                _adjustCardSpeed(1);
            if (b && !m_prevB)
                _closeCardEditPanel();
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
            m_prevMinus = minus;
            return;
        }

        // 编辑模式：方向键移动焦点/抬起项，A 抬起放下，B 退出
        if (m_editor.isActive()) {
            _handleEditInput(dt);
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
            m_prevMinus = minus;
            return;
        }

        if (left && !m_prevLeft)
            _moveLeft();
        if (right && !m_prevRight)
            _moveRight();
        if (up && !m_prevUp)
            _moveUp();
        if (down && !m_prevDown)
            _moveDown();

        if (b && !m_prevB) {
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
            m_prevMinus = minus;
            _handleBack();
            return;
        }

        const bool activate = a && !m_prevA;
        if (activate) {
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
            m_prevMinus = minus;
            _activateCurrent();
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
                if (direction < 0)
                    _moveLeft();
                else
                    _moveRight();
            }
        };
        repeat(left, m_holdLeft, m_repeatLeft, -1);
        repeat(right, m_holdRight, m_repeatRight, 1);

        m_prevLeft = left;
        m_prevRight = right;
        m_prevUp = up;
        m_prevDown = down;
        m_prevA = a;
        m_prevB = b;
        m_prevMinus = minus;
    }

    void IisuLayout::_toggleEditMode()
    {
        if (m_editor.isActive()) {
            _exitEditMode();
            return;
        }
        // 浮层打开时不进入编辑
        if (m_uiContext.isFolderOpen() || m_cardPanelOpen || m_cardEditOpen)
            return;
        m_focusArea = FocusArea::GRID;
        m_editor.enter();
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    }

    void IisuLayout::_exitEditMode()
    {
        if (!m_editor.isActive())
            return;
        m_editor.exit();
        // 退出时静默保存布局
        m_editor.save("home.json");
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    }

    void IisuLayout::_handleEditInput(float dt)
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
        const bool b = state.buttons[static_cast<int>(brls::BUTTON_B)];

        auto blockedFeedback = [this](int dx, int dy) {
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
            _layout().setFocusShake(static_cast<float>(dx),
                                    static_cast<float>(dy));
        };

        if (m_editor.isLifted()) {
            // 抬起状态：方向键带着 Widget 移动
            auto tryMove = [this, &blockedFeedback](int dx, int dy,
                                                    bool rising) {
                if (!rising)
                    return;
                if (m_editor.moveItem(dx, dy))
                    brls::Application::getAudioPlayer()->play(
                        brls::SOUND_FOCUS_CHANGE);
                else
                    blockedFeedback(dx, dy);
            };
            tryMove(-1, 0, left && !m_prevLeft);
            tryMove(1, 0, right && !m_prevRight);
            tryMove(0, -1, up && !m_prevUp);
            tryMove(0, 1, down && !m_prevDown);

            // 长按连发移动
            auto repeat = [this, dt, &blockedFeedback](bool held, float& hold,
                                                       float& timer, int dx) {
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
                    if (m_editor.moveItem(dx, 0))
                        brls::Application::getAudioPlayer()->play(
                            brls::SOUND_FOCUS_CHANGE);
                    else
                        blockedFeedback(dx, 0);
                }
            };
            repeat(left, m_holdLeft, m_repeatLeft, -1);
            repeat(right, m_holdRight, m_repeatRight, 1);
        } else {
            // 未抬起：方向键移动焦点（不切功能区）
            auto tryFocus = [this, &blockedFeedback](
                                UIAction action, bool rising,
                                int dx, int dy) {
                if (!rising)
                    return;
                const int bx = _layout().focus().cellX();
                const int by = _layout().focus().cellY();
                _layout().moveFocus(action);
                if (_layout().focus().cellX() == bx &&
                    _layout().focus().cellY() == by)
                    blockedFeedback(dx, dy);
                else
                    brls::Application::getAudioPlayer()->play(
                        brls::SOUND_FOCUS_CHANGE);
            };
            tryFocus(UIAction::Left, left && !m_prevLeft, -1, 0);
            tryFocus(UIAction::Right, right && !m_prevRight, 1, 0);
            tryFocus(UIAction::Up, up && !m_prevUp, 0, -1);
            tryFocus(UIAction::Down, down && !m_prevDown, 0, 1);
        }

        // A：抬起 / 放下当前 Widget
        if (a && !m_prevA) {
            if (m_editor.toggleLift())
                brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
            else
                brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_ERROR);
        }

        // B：退出编辑（触发静默保存）
        if (b && !m_prevB)
            _exitEditMode();
    }

    LayoutManager& IisuLayout::_activeLayout()
    {
        return m_uiContext.isFolderOpen()
            ? m_uiContext.panelLayout()
            : m_uiContext.layout();
    }

    void IisuLayout::_moveLeft()
    {
        if (m_focusArea == FocusArea::GRID) {
            _activeLayout().moveFocus(UIAction::Left);
        } else {
            _moveFunctionHorizontal(-1);
        }
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void IisuLayout::_moveRight()
    {
        if (m_focusArea == FocusArea::GRID) {
            _activeLayout().moveFocus(UIAction::Right);
        } else {
            _moveFunctionHorizontal(1);
        }
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void IisuLayout::_moveUp()
    {
        if (m_focusArea == FocusArea::FUNCTIONS) {
            m_focusArea = FocusArea::GRID;
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        } else if (m_focusArea == FocusArea::GRID) {
            // 网格内先逐行上移
            if (_activeLayout().focus().cellY() > 0) {
                _activeLayout().moveFocus(UIAction::Up);
                brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
            }
        }
    }

    void IisuLayout::_moveDown()
    {
        if (m_focusArea == FocusArea::GRID) {
            // 网格内先逐行下移；浮层打开时禁止切到功能区
            const int rows = _activeLayout().grid().config().rows;
            if (_activeLayout().focus().cellY() < rows - 1) {
                _activeLayout().moveFocus(UIAction::Down);
            } else if (!m_uiContext.isFolderOpen()) {
                m_focusArea = FocusArea::FUNCTIONS;
            }
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        }
    }

    void IisuLayout::_handleBack()
    {
        // 编辑模式下 B 由 _handleEditInput 处理退出
        // 文件夹子布局中按 B 返回上一级
        if (m_focusArea == FocusArea::GRID && m_uiContext.isFolderOpen()) {
            m_uiContext.closeFolder();
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        }
    }

    void IisuLayout::_moveFunctionHorizontal(int direction)
    {
        if (m_functions.empty())
            return;
        const int count = static_cast<int>(m_functions.size());
        m_selectedFunction = (m_selectedFunction + direction + count) % count;
    }

    void IisuLayout::enterEditMode()
    {
        if (m_uiContext.isFolderOpen() || m_cardPanelOpen || m_cardEditOpen)
            return;
        m_focusArea = FocusArea::GRID;
        m_editor.enter();
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    }

    void IisuLayout::requestCardSettings()
    {
        // 当前聚焦在卡片上才弹出占位面板
        if (m_uiContext.isFolderOpen())
            return;
        if (auto* item = _layout().currentItem()) {
            if (item->widget)
                _openCardEditPanel();
        }
    }

    void IisuLayout::_openCardPanel()
    {
        auto* item = _activeLayout().currentItem();
        if (!item || !item->widget)
            return;

        m_cardPanelItem = item;
        m_cardPanelSelected = 0;
        m_cardPanelImage = 0;
        m_cardPanelTextureRequested = false;

        // 缩略图来源：图片直接用路径，封面通过数据提供者解析
        std::string imgPath;
        const std::string type = item->widget->typeName();
        if (type == "image") {
            imgPath = item->widget->dataId();
        } else if (type == "game_cover") {
            const auto info =
                m_uiContext.gameProvider().getGame(item->widget->dataId());
            if (info)
                imgPath = info->coverPath;
        }
        m_cardPanelImagePath = std::move(imgPath);

        m_cardPanelOpen = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    }

    void IisuLayout::_closeCardPanel()
    {
        if (!m_cardPanelOpen)
            return;
        if (m_cardPanelImage > 0) {
            m_uiContext.textures().releaseTexture(
                brls::Application::getNVGContext(), m_cardPanelImagePath);
            m_cardPanelImage = 0;
        }
        m_cardPanelOpen = false;
        m_cardPanelItem = nullptr;
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
    }

    void IisuLayout::_handleCardPanelInput(float /*dt*/)
    {
        auto& state = brls::Application::getControllerState();
        const float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
        const float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
        const float navY = std::abs(ry) > std::abs(ly) ? ry : ly;
        const bool up = state.buttons[static_cast<int>(brls::BUTTON_UP)] || navY < -0.5f;
        const bool down = state.buttons[static_cast<int>(brls::BUTTON_DOWN)] || navY > 0.5f;
        const bool a = state.buttons[static_cast<int>(brls::BUTTON_A)];
        const bool b = state.buttons[static_cast<int>(brls::BUTTON_B)];

        constexpr int kActionCount = 4; // 启动/游戏选项/卡片设置/关闭
        if (up && !m_prevUp) {
            m_cardPanelSelected =
                (m_cardPanelSelected + kActionCount - 1) % kActionCount;
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        }
        if (down && !m_prevDown) {
            m_cardPanelSelected = (m_cardPanelSelected + 1) % kActionCount;
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        }

        if (a && !m_prevA) {
            if (m_cardPanelSelected == 0) {
                const auto entry = _currentGameEntry();
                _closeCardPanel();
                if (entry && onGameActivated)
                    onGameActivated(*entry);
                return;
            }
            if (m_cardPanelSelected == 1) {
                const auto entry = _currentGameEntry();
                _closeCardPanel();
                if (entry && onGameOptions)
                    onGameOptions(*entry);
                return;
            }
            // 卡片设置：关闭本面板并打开卡片编辑面板
            if (m_cardPanelSelected == 2) {
                _closeCardPanel();
                _openCardEditPanel();
                return;
            }
            if (m_cardPanelSelected == kActionCount - 1) {
                _closeCardPanel();
            }
        }

        if (b && !m_prevB)
            _closeCardPanel();
    }

    void IisuLayout::_drawRoundedImage(NVGcontext* vg, int texture,
                                       const GridRect& rect)
    {
        if (texture <= 0)
            return;
        int imageW = 0;
        int imageH = 0;
        nvgImageSize(vg, texture, &imageW, &imageH);
        if (imageW <= 0 || imageH <= 0)
            return;
        const float scale = std::max(
            rect.width / static_cast<float>(imageW),
            rect.height / static_cast<float>(imageH));
        const float drawW = static_cast<float>(imageW) * scale;
        const float drawH = static_cast<float>(imageH) * scale;
        const float drawX = rect.left + (rect.width - drawW) * 0.5f;
        const float drawY = rect.top + (rect.height - drawH) * 0.5f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, rect.left, rect.top,
                       rect.width, rect.height, m_uiContext.layout()
                           .grid().config().radius - 6.f);
        NVGpaint paint = nvgImagePattern(
            vg, drawX, drawY, drawW, drawH, 0.f, texture, 1.f);
        nvgFillPaint(vg, paint);
        nvgFill(vg);
    }

    void IisuLayout::_drawCardPanel(NVGcontext* vg, float x, float y,
                                    float w, float h)
    {
        if (!m_cardPanelOpen)
            return;

        // 遮罩
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 130));
        nvgFill(vg);

        constexpr float panelW = 640.f;
        constexpr float panelH = 380.f;
        const float panelX = x + (w - panelW) * 0.5f;
        const float panelY = y + (h - panelH) * 0.5f;
        constexpr float radius = 18.f;

        // 阴影 + 背景
        NVGpaint panelShadow = nvgBoxGradient(
            vg, panelX + 6.f, panelY + 8.f, panelW, panelH,
            radius, 8.f, nvgRGBA(0, 0, 0, 150), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, panelX - 2.f, panelY, panelW + 14.f, panelH + 18.f);
        nvgRoundedRect(vg, panelX, panelY, panelW, panelH, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, panelShadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, panelX, panelY, panelW, panelH, radius);
        nvgFillColor(vg, nvgRGBA(28, 32, 44, 242));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 80));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        const int fontId = m_fontId;

        // 左列：缩略图 + 名称
        constexpr float imgSize = 150.f;
        const GridRect imgRect{panelX + 28.f, panelY + 28.f,
                               imgSize, imgSize};
        if (!m_cardPanelTextureRequested) {
            m_cardPanelTextureRequested = true;
            if (!m_cardPanelImagePath.empty())
                m_cardPanelImage = m_uiContext.textures().loadTexture(
                    vg, m_cardPanelImagePath);
        }
        if (m_cardPanelImage > 0) {
            _drawRoundedImage(vg, m_cardPanelImage, imgRect);
        } else {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, imgRect.left, imgRect.top,
                           imgRect.width, imgRect.height, 12.f);
            nvgFillColor(vg, nvgRGBA(90, 95, 110, 160));
            nvgFill(vg);
        }

        const std::string name = m_cardPanelItem && m_cardPanelItem->widget
            ? m_cardPanelItem->widget->displayName()
            : "";
        nvgFontFaceId(vg, fontId);
        nvgFontSize(vg, 16.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
        nvgFillColor(vg, nvgRGBA(242, 245, 251, 235));
        nvgText(vg, panelX + 28.f + imgSize * 0.5f, panelY + 28.f + imgSize + 12.f,
                name.c_str(), nullptr);

        // 右列：按钮列表（占位）
        const float btnX0 = panelX + 212.f;
        const float btnW = panelW - 212.f - 28.f;
        constexpr float btnH = 46.f;
        constexpr float btnGap = 10.f;
        static const std::string kActions[] = {
            L("启动游戏"), L("游戏选项"), L("卡片设置"), L("关闭"),
        };
        for (int i = 0; i < 4; ++i) {
            const bool selected = i == m_cardPanelSelected;
            const float by = panelY + 26.f +
                static_cast<float>(i) * (btnH + btnGap);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, btnX0, by, btnW, btnH, 10.f);
            nvgFillColor(vg, selected
                ? nvgRGBA(91, 193, 255, 225)
                : nvgRGBA(255, 255, 255, 12));
            nvgFill(vg);
            nvgStrokeColor(vg, selected
                ? nvgRGBA(168, 224, 255, 230)
                : nvgRGBA(255, 255, 255, 45));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);

            nvgFontFaceId(vg, fontId);
            nvgFontSize(vg, 19.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected
                ? nvgRGBA(18, 24, 34, 245)
                : nvgRGBA(220, 228, 240, 225));
            nvgText(vg, btnX0 + btnW * 0.5f, by + btnH * 0.5f,
                    kActions[i].c_str(), nullptr);
        }

        // 底部提示
        nvgFontSize(vg, 14.f);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(190, 200, 218, 180));
        nvgText(vg, panelX + panelW - 16.f, panelY + panelH - 22.f,
                L("A 选择  B 返回").c_str(), nullptr);
    }

    void IisuLayout::_openCardEditPanel()
    {
        auto* item = _layout().currentItem();
        if (!item || !item->widget)
            return;
        m_cardEditName = item->widget->displayName();
        m_cardEditItem = item;
        m_cardSpeedIndex = 0;
        if (item->widget->typeName() == "image") {
            if (auto* image = dynamic_cast<ImageWidget*>(item->widget.get()))
                m_cardSpeedIndex = _snapSpeedIndex(image->speed());
        }
        m_cardEditOpen = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
    }

    int IisuLayout::_snapSpeedIndex(float speed)
    {
        const std::vector<float>& options = _speedOptions();
        int best = 0;
        float bestDist = 1e9f;
        for (size_t i = 0; i < options.size(); ++i) {
            const float d = std::abs(options[i] - speed);
            if (d < bestDist) {
                bestDist = d;
                best = static_cast<int>(i);
            }
        }
        return best;
    }

    const std::vector<float>& IisuLayout::_speedOptions()
    {
        static const std::vector<float> kOptions = {
            0.25f, 0.5f, 1.f, 2.f, 4.f,
        };
        return kOptions;
    }

    void IisuLayout::_adjustCardSpeed(int dir)
    {
        auto* item = m_cardEditItem;
        if (!item || !item->widget || item->widget->typeName() != "image")
            return;
        auto* image = dynamic_cast<ImageWidget*>(item->widget.get());
        if (!image)
            return;

        const std::vector<float>& options = _speedOptions();
        const int count = static_cast<int>(options.size());
        m_cardSpeedIndex =
            (m_cardSpeedIndex + dir + count) % count;
        image->setSpeed(options[static_cast<size_t>(m_cardSpeedIndex)]);
        // 立即静默保存，保证速度持久化
        m_editor.save("home.json");
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void IisuLayout::_closeCardEditPanel()
    {
        if (!m_cardEditOpen)
            return;
        m_cardEditOpen = false;
        m_cardEditItem = nullptr;
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
    }

    void IisuLayout::_drawCardEditPanel(NVGcontext* vg, float x, float y,
                                        float w, float h)
    {
        if (!m_cardEditOpen)
            return;

        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 130));
        nvgFill(vg);

        constexpr float panelW = 560.f;
        constexpr float panelH = 240.f;
        const float panelX = x + (w - panelW) * 0.5f;
        const float panelY = y + (h - panelH) * 0.5f;
        constexpr float radius = 18.f;

        NVGpaint panelShadow = nvgBoxGradient(
            vg, panelX + 6.f, panelY + 8.f, panelW, panelH,
            radius, 8.f, nvgRGBA(0, 0, 0, 150), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, panelX - 2.f, panelY, panelW + 14.f, panelH + 18.f);
        nvgRoundedRect(vg, panelX, panelY, panelW, panelH, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, panelShadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, panelX, panelY, panelW, panelH, radius);
        nvgFillColor(vg, nvgRGBA(28, 32, 44, 242));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 80));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 24.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(242, 245, 251, 240));
        nvgText(vg, panelX + panelW * 0.5f, panelY + 58.f,
                L("卡片设置").c_str(), nullptr);

        nvgFontSize(vg, 16.f);
        nvgFillColor(vg, nvgRGBA(200, 209, 225, 200));
        nvgText(vg, panelX + panelW * 0.5f, panelY + 94.f,
                m_cardEditName.c_str(), nullptr);

        // GIF 图片：播放速度调节
        bool isGifImage = false;
        float speed = 1.f;
        if (m_cardEditItem && m_cardEditItem->widget &&
            m_cardEditItem->widget->typeName() == "image") {
            if (auto* image =
                    dynamic_cast<ImageWidget*>(m_cardEditItem->widget.get())) {
                isGifImage = m_uiContext.textures().isGifTexture(
                    image->dataId());
                speed = image->speed();
            }
        }

        if (isGifImage) {
            const std::string speedText = std::to_string(
                static_cast<int>(speed * 100.f + 0.5f)) + "%";
            nvgFontSize(vg, 22.f);
            nvgFillColor(vg, nvgRGBA(91, 193, 255, 240));
            nvgText(vg, panelX + panelW * 0.5f, panelY + 140.f,
                    (L("播放速度: ") + speedText).c_str(), nullptr);

            // 速度档位指示
            nvgFontSize(vg, 13.f);
            nvgFillColor(vg, nvgRGBA(170, 180, 200, 160));
            const std::vector<float>& options = _speedOptions();
            std::string bar;
            for (size_t i = 0; i < options.size(); ++i)
                bar += (i == static_cast<size_t>(m_cardSpeedIndex))
                    ? "●  " : "○  ";
            nvgText(vg, panelX + panelW * 0.5f, panelY + 172.f,
                    bar.c_str(), nullptr);

            nvgFontSize(vg, 14.f);
            nvgFillColor(vg, nvgRGBA(190, 200, 218, 180));
            nvgText(vg, panelX + panelW * 0.5f, panelY + panelH - 26.f,
                    L("←/→ 调整速度  B 返回").c_str(), nullptr);
        } else {
            nvgFontSize(vg, 14.f);
            nvgFillColor(vg, nvgRGBA(170, 180, 200, 160));
            nvgText(vg, panelX + panelW * 0.5f, panelY + 150.f,
                    L("（占位功能，等待实现）").c_str(), nullptr);

            nvgFontSize(vg, 14.f);
            nvgFillColor(vg, nvgRGBA(190, 200, 218, 180));
            nvgText(vg, panelX + panelW * 0.5f, panelY + panelH - 26.f,
                    L("B 返回").c_str(), nullptr);
        }
    }

    void IisuLayout::_activateCurrent()
    {
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        if (m_focusArea == FocusArea::GRID) {
            if (auto* current = _activeLayout().currentItem()) {
                if (current->widget) {
                    // 文件夹展开；游戏封面进入与主页共用的游戏操作链路；
                    // 图片等装饰组件直接打开其设置，而不显示无效游戏操作。
                    if (current->widget->typeName() == "folder")
                        current->widget->onActivate();
                    else if (current->widget->typeName() == "game_cover")
                        _openCardPanel();
                    else if (current->widget->typeName() == "image") {
                        const bool isPico8Logo = current->widget->dataId() ==
                            BK_RES("img/pico8_logo_vector.png");
                        if (isPico8Logo && m_pico8ShortcutVisible && onPico8Opened)
                            playPico8ExitAnimation(onPico8Opened);
                        else
                            _openCardEditPanel();
                    }
                }
            }
            return;
        }
        if (!m_functionClickAnimating) {
            m_functionClickAnimating = true;
            m_functionClickIndex = m_selectedFunction;
            m_functionClickTime = 0.f;
        }
    }

    void IisuLayout::_activateFunction(int index)
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

    std::optional<beiklive::GameEntry> IisuLayout::_currentGameEntry() const
    {
        const auto& layout = m_uiContext.isFolderOpen()
            ? m_uiContext.panelLayout() : m_uiContext.layout();
        auto* item = const_cast<LayoutManager&>(layout).currentItem();
        if (!item || !item->widget || item->widget->typeName() != "game_cover" ||
            !beiklive::GameDB)
            return std::nullopt;
        return beiklive::GameDB->findByPath(item->widget->dataId());
    }

    void IisuLayout::_animateEntrance(LayoutManager& layout, float delay)
    {
        auto& items = layout.items();
        for (size_t i = 0; i < items.size(); ++i) {
            auto& item = items[i];
            if (!item.visible)
                continue;
            item.transform.alpha = 0.f;
            item.transform.scale = 0.92f;
            const float duration = 0.20f + std::min(0.12f,
                static_cast<float>(i) * 0.025f) + delay;
            m_uiContext.animations().add({
                duration, 0.f,
                [&item, delay, duration](float t) {
                    const float local = duration > delay
                        ? std::clamp((t * duration - delay) /
                                         (duration - delay), 0.f, 1.f)
                        : 1.f;
                    item.transform.alpha = local;
                    item.transform.scale = 0.92f + local * 0.08f;
                },
                { }, false,
            });
        }
    }

    void IisuLayout::draw(NVGcontext* vg, float x, float y, float w, float h,
                          brls::Style style, brls::FrameContext* ctx)
    {
        brls::Box::draw(vg, x, y, w, h, style, ctx);

        // ── 占位绘制：后续替换为 iisu 完整布局 ──────────────────────────
        if (m_fontId < 0)
            m_fontId = brls::Application::getDefaultFont();

        nvgSave(vg);
        nvgGlobalAlpha(vg, std::clamp(m_pageOpacity, 0.f, 1.f));
        nvgIntersectScissor(vg, x, y, w, h);

        const float cx = x + w * 0.5f;

        // ── 顶部占位区：主体区域在顶部占位区与底部功能区之间 ─────────
        constexpr float topBarH = 64.f;
        constexpr float bottomBarH = 88.f; // 与 _drawFunctions 一致
        constexpr float barMargin = 10.f;
        const float topY = y + barMargin;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 12.f, topY, w - 24.f, topBarH, 16.f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 10));
        nvgFill(vg);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 60));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgFontFaceId(vg, m_fontId);
        nvgFontSize(vg, 13.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(190, 200, 218, 140));
        nvgText(vg, cx, topY + topBarH * 0.68f,
                L("顶部功能区（占位）").c_str(), nullptr);

        // 当前聚焦信息：顶部占位区中间偏上，随聚焦动画淡入
        std::string focusInfo;
        float focusAlpha = 1.f;
        if (m_focusArea == FocusArea::GRID) {
            // 文件夹子布局中显示文件夹名
            if (m_uiContext.isFolderOpen()) {
                const auto folder = m_uiContext.folderProvider().getFolder(
                    m_uiContext.currentFolderId());
                if (folder)
                    focusInfo = "[" + folder->title + "] ";
            }
            if (auto* current = _activeLayout().currentItem()) {
                focusInfo += "#" + std::to_string(current->id) + " " +
                    std::to_string(current->w) + "x" +
                    std::to_string(current->h) + "@" +
                    std::to_string(current->x) + "," +
                    std::to_string(current->y);
            } else {
                // 空白格：显示单元格坐标
                focusInfo += L("空格 ") + "(" +
                    std::to_string(_activeLayout().focus().cellX()) + "," +
                    std::to_string(_activeLayout().focus().cellY()) + ")";
            }
        } else if (!m_functions.empty()) {
            focusInfo =
                m_functions[static_cast<size_t>(m_selectedFunction)].label;
            focusAlpha =
                static_cast<size_t>(m_selectedFunction) < m_functionFocus.size()
                ? m_functionFocus[static_cast<size_t>(m_selectedFunction)]
                : 1.f;
        }
        if (!focusInfo.empty()) {
            nvgFontSize(vg, 24.f);
            nvgFillColor(vg, nvgRGBA(242, 245, 251,
                static_cast<unsigned char>(235.f * focusAlpha)));
            nvgText(vg, cx, topY + topBarH * 0.30f,
                    focusInfo.c_str(), nullptr);
        }

        // ── 布局主体网格：顶部占位区与底部功能区之间 ────────────────────
        const float bodyX = x + 12.f;
        const float bodyY = y + barMargin + topBarH;
        const float bodyW = w - 24.f;
        const float bodyH = h - 2.f * barMargin - topBarH - bottomBarH;
        _layout().setArea(bodyX, bodyY, bodyW, bodyH);
        GridDebugRenderer::draw(vg, _layout().grid(), _layout().items());
        // 焦点切到底部功能区或浮层打开时隐藏主界面焦点框
        _layout().setFocusVisible(m_focusArea == FocusArea::GRID &&
                                  !m_uiContext.isFolderOpen());
        _layout().draw(vg, m_time);

        // 编辑模式覆盖层
        if (m_editor.isActive())
            m_editor.draw(vg, m_fontId);

        // 卡片操作浮层（非文件夹 Widget 按 A 弹出）
        _drawCardPanel(vg, x, y, w, h);

        // 卡片编辑占位面板（START → 卡片设置）
        _drawCardEditPanel(vg, x, y, w, h);

        // ── 文件夹浮层：悬浮在当前界面上 ────────────────────────────────
        if (m_uiContext.isFolderOpen()) {
            // 遮罩
            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h);
            nvgFillColor(vg, nvgRGBA(0, 0, 0, 120));
            nvgFill(vg);

            // 面板：上下边距 20，左右边距 40
            const float panelX = x + 40.f;
            const float panelY = y + 20.f;
            const float panelW = w - 80.f;
            const float panelH = h - 40.f;
            constexpr float headerH = 100.f;
            constexpr float panelRadius = 18.f;

            // 面板阴影
            NVGpaint panelShadow = nvgBoxGradient(
                vg, panelX + 6.f, panelY + 8.f, panelW, panelH,
                panelRadius, 8.f, nvgRGBA(0, 0, 0, 150), nvgRGBA(0, 0, 0, 0));
            nvgBeginPath(vg);
            nvgRect(vg, panelX - 2.f, panelY, panelW + 14.f, panelH + 18.f);
            nvgRoundedRect(vg, panelX, panelY, panelW, panelH, panelRadius);
            nvgPathWinding(vg, NVG_HOLE);
            nvgFillPaint(vg, panelShadow);
            nvgFill(vg);

            // 面板背景
            nvgBeginPath(vg);
            nvgRoundedRect(vg, panelX, panelY, panelW, panelH, panelRadius);
            nvgFillColor(vg, nvgRGBA(28, 32, 44, 240));
            nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 80));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);

            // Header：文件夹图标 + 名称 + 分隔线
            const auto folder = m_uiContext.folderProvider().getFolder(
                m_uiContext.currentFolderId());
            const std::string panelTitle =
                folder ? folder->title : L("文件夹");
            const float headerCenterY = panelY + headerH * 0.5f;

            const int materialFontId =
                brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
            if (materialFontId >= 0) {
                const std::string glyph =
                    encodeUtf8(beiklive::material::FOLDER);
                nvgFontFaceId(vg, materialFontId);
                nvgFontSize(vg, 34.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(235, 242, 255, 235));
                nvgText(vg, panelX + 28.f, headerCenterY,
                        glyph.c_str(), nullptr);
            }

            nvgFontFaceId(vg, m_fontId);
            nvgFontSize(vg, 26.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(242, 245, 251, 235));
            nvgText(vg, panelX + 56.f, headerCenterY,
                    panelTitle.c_str(), nullptr);

            nvgFontSize(vg, 14.f);
            nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(190, 200, 218, 180));
            nvgText(vg, panelX + panelW - 16.f, headerCenterY,
                    L("B 返回").c_str(), nullptr);

            // 分隔线
            nvgBeginPath(vg);
            nvgMoveTo(vg, panelX, panelY + headerH);
            nvgLineTo(vg, panelX + panelW, panelY + headerH);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 40));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);

            // 主体：3 行 N 列，向右延伸，横向滚动
            auto& panel = m_uiContext.panelLayout();
            panel.setArea(panelX, panelY + headerH,
                          panelW, panelH - headerH);
            panel.setFocusVisible(true);
            panel.draw(vg, m_time);
        }

        if (!m_uiContext.isFolderOpen())
            _drawFunctions(vg, x, y, w, h);

        nvgResetScissor(vg);
        nvgRestore(vg);
    }

    void IisuLayout::_drawFunctions(NVGcontext* vg, float x, float y,
                                    float w, float h)
    {
        if (m_functions.empty())
            return;
        const float pitch = 92.f;
        const float barW = pitch * static_cast<float>(m_functions.size());
        const float barH = 88.f;
        const float barX = x + w * 0.5f - barW * 0.5f;
        // 贴底布局：功能区固定于画面底部
        const float barY = y + h - barH - 10.f;
        const float centerY = barY + barH * 0.5f;

        nvgSave(vg);
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
                m_focusArea == FocusArea::FUNCTIONS &&
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
                    float drawW = 42.f;
                    float drawH = drawW / aspect;
                    if (drawH > 42.f) {
                        drawH = 42.f;
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
                constexpr float focusSize = 84.f;
                beiklive::ui::drawGradientFocusCircle(
                    vg, cx, centerY, focusSize, 6.f, focus,
                    beiklive::ui::gradientFocusAnimationOffset(m_time));
            }

            nvgRestore(vg);
        }
    }
} // namespace beiklive
