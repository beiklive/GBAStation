#include "IisuLayout.hpp"

#include "core/Translation.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "WidgetFactory.hpp"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float HOLD_DELAY = 0.30f;
    constexpr float HOLD_REPEAT = 0.085f;
} // namespace

namespace beiklive
{
    IisuLayout::IisuLayout() : Layout()
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
        _captureInputState();

        // 主页面：GameCover（第一条游戏）+ 图片 + 平台文件夹（有游戏数据时）
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
        for (size_t i = 0; i < std::min<size_t>(2, platforms.size()); ++i)
            mainPage.push_back({WidgetType::Folder,
                                "platform:" + std::to_string(platforms[i]),
                                "", 4, static_cast<int>(i), 2, 1, true});
        m_uiContext.setMainPage(mainPage);
    }

    IisuLayout::~IisuLayout()
    {
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

    void IisuLayout::removeGameByPath(const std::string& /*path*/)
    {
        // TODO: iisu 布局删除动画
    }

    void IisuLayout::completeGameRemoval(std::function<void()> completion)
    {
        if (completion)
            completion();
    }

    void IisuLayout::cancelGameRemoval()
    {
        // TODO: iisu 布局删除动画取消
    }

    int IisuLayout::acquireSelectedCoverTexture()
    {
        return 0;
    }

    void IisuLayout::releaseSelectedCoverTexture()
    {
    }

    void IisuLayout::playEntranceAnimation()
    {
        // TODO: iisu 布局入场动画
    }

    void IisuLayout::playExitAnimation(std::function<void()> completion)
    {
        if (completion)
            completion();
    }

    void IisuLayout::playPico8ExitAnimation(std::function<void()> completion)
    {
        if (completion)
            completion();
    }

    void IisuLayout::beginPico8ReturnAnimation()
    {
        // TODO: iisu 布局 PICO-8 返回动画
    }

    void IisuLayout::setPico8ReturnProgress(float /*progress*/)
    {
    }

    void IisuLayout::finishPico8ReturnAnimation()
    {
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

        _layout().update(dt);
        _handleInput(dt);
        invalidate();
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

        if (!isFocused() || brls::Application::isInputBlocks() ||
            m_functionClickAnimating) {
            m_holdLeft = m_holdRight = 0.f;
            m_repeatLeft = m_repeatRight = 0.f;
            m_prevLeft = left;
            m_prevRight = right;
            m_prevUp = up;
            m_prevDown = down;
            m_prevA = a;
            m_prevB = b;
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
    }

    void IisuLayout::_moveLeft()
    {
        if (m_focusArea == FocusArea::GRID) {
            _layout().moveFocus(UIAction::Left);
        } else {
            _moveFunctionHorizontal(-1);
        }
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void IisuLayout::_moveRight()
    {
        if (m_focusArea == FocusArea::GRID) {
            _layout().moveFocus(UIAction::Right);
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
            if (_layout().focus().cellY() > 0) {
                _layout().moveFocus(UIAction::Up);
                brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
            }
        }
    }

    void IisuLayout::_moveDown()
    {
        if (m_focusArea == FocusArea::GRID) {
            // 网格内先逐行下移，到最底行后才切到底部功能区
            const int rows = _layout().grid().config().rows;
            if (_layout().focus().cellY() < rows - 1) {
                _layout().moveFocus(UIAction::Down);
            } else {
                m_focusArea = FocusArea::FUNCTIONS;
            }
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        }
    }

    void IisuLayout::_handleBack()
    {
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

    void IisuLayout::_activateCurrent()
    {
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        if (m_focusArea == FocusArea::GRID) {
            // 交由 Widget 处理（FolderWidget 展开子布局）
            if (auto* current = _layout().currentItem()) {
                if (current->widget)
                    current->widget->onActivate();
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

    void IisuLayout::draw(NVGcontext* vg, float x, float y, float w, float h,
                          brls::Style style, brls::FrameContext* ctx)
    {
        brls::Box::draw(vg, x, y, w, h, style, ctx);

        // ── 占位绘制：后续替换为 iisu 完整布局 ──────────────────────────
        if (m_fontId < 0)
            m_fontId = brls::Application::getDefaultFont();

        nvgSave(vg);
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
            if (auto* current = _layout().currentItem()) {
                focusInfo += "#" + std::to_string(current->id) + " " +
                    std::to_string(current->w) + "x" +
                    std::to_string(current->h) + "@" +
                    std::to_string(current->x) + "," +
                    std::to_string(current->y);
            } else {
                // 空白格：显示单元格坐标
                focusInfo += L("空格 ") + "(" +
                    std::to_string(_layout().focus().cellX()) + "," +
                    std::to_string(_layout().focus().cellY()) + ")";
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
        // 焦点切到底部功能区时隐藏网格焦点框
        _layout().setFocusVisible(m_focusArea == FocusArea::GRID);
        _layout().draw(vg, m_time);

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
