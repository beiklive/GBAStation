#include "GameOptionsSidebar.hpp"
#include "core/Translation.hpp"
#include "core/Tools.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/widget/HintsBar.hpp"

#include <algorithm>
#include <cmath>

namespace
{
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
}

namespace beiklive
{

    GameOptionsSidebar::GameOptionsSidebar()
        : brls::Box(brls::Axis::COLUMN)
    {
        this->setVisibility(brls::Visibility::GONE);
        this->setFocusable(false);
        this->setHideHighlight(true);
        this->setPositionType(brls::PositionType::ABSOLUTE);
        this->setPositionTop(0);
        this->setPositionLeft(0);
        this->setWidth(1280.f);
        this->setHeightPercentage(100.f);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        m_nanoFontId = brls::Application::getDefaultFont();
        m_nanoMaterialFontId = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        m_nanoSwitchFontId = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        
    }

    GameOptionsSidebar::~GameOptionsSidebar()
    {
        if (m_nanoImageHandle >= 0 && m_nanoOwnsImageHandle) {
            if (auto* vg = brls::Application::getNVGContext())
                nvgDeleteImage(vg, m_nanoImageHandle);
        }
        m_nanoImageHandle = -1;
        m_nanoOwnsImageHandle = false;
    }

    brls::View* GameOptionsSidebar::getDefaultFocus()
    {
        return m_nanoVgMenu ? this : brls::Box::getDefaultFocus();
    }

    brls::View* GameOptionsSidebar::getNextFocus(
        brls::FocusDirection direction, brls::View* currentView)
    {
        if (m_nanoVgMenu)
            return this;
        return brls::Box::getNextFocus(direction, currentView);
    }

    void GameOptionsSidebar::addButton(const std::string& text,
                                        char32_t iconCodepoint,
                                        std::function<void(const beiklive::GameEntry&)> callback)
    {
        m_buttons.push_back({text, iconCodepoint, std::move(callback)});
    }

    void GameOptionsSidebar::clearButtons()
    {
        m_buttons.clear();
    }

    int GameOptionsSidebar::addSubmenu(const std::string& text,
                                       char32_t iconCodepoint)
    {
        m_buttons.push_back({text, iconCodepoint, nullptr, {}});
        return static_cast<int>(m_buttons.size()) - 1;
    }

    int GameOptionsSidebar::addSubmenuButton(
        int submenuIndex,
        const std::string& text,
        char32_t iconCodepoint,
        std::function<void(const beiklive::GameEntry&)> callback)
    {
        if (submenuIndex < 0 ||
            static_cast<size_t>(submenuIndex) >= m_buttons.size())
            return -1;
        auto& children = m_buttons[static_cast<size_t>(submenuIndex)].children;
        children.push_back({text, iconCodepoint, std::move(callback), {}});
        return static_cast<int>(children.size()) - 1;
    }

    int GameOptionsSidebar::addNestedSubmenu(
        int submenuIndex,
        const std::string& text,
        char32_t iconCodepoint)
    {
        if (submenuIndex < 0 ||
            static_cast<size_t>(submenuIndex) >= m_buttons.size())
            return -1;
        auto& children = m_buttons[static_cast<size_t>(submenuIndex)].children;
        children.push_back({text, iconCodepoint, nullptr, {}});
        return static_cast<int>(children.size()) - 1;
    }

    void GameOptionsSidebar::addNestedSubmenuButton(
        int submenuIndex,
        int nestedSubmenuIndex,
        const std::string& text,
        char32_t iconCodepoint,
        std::function<void(const beiklive::GameEntry&)> callback)
    {
        if (submenuIndex < 0 ||
            static_cast<size_t>(submenuIndex) >= m_buttons.size())
            return;
        auto& children = m_buttons[static_cast<size_t>(submenuIndex)].children;
        if (nestedSubmenuIndex < 0 ||
            static_cast<size_t>(nestedSubmenuIndex) >= children.size())
            return;
        children[static_cast<size_t>(nestedSubmenuIndex)].children.push_back(
            {text, iconCodepoint, std::move(callback), {}});
    }

    void GameOptionsSidebar::open(const beiklive::GameEntry& entry)
    {
        if (m_isOpen)
            _destroyUI();

        m_entry  = entry;
        m_isOpen = true;
        m_isClosing = false;
        m_launchClosing = false;
        m_openProgress = 0.f;
        m_nanoInSubmenu = false;
        m_nanoInNestedSubmenu = false;
        m_nanoRootSelected = 0;
        m_nanoChildSelected = 0;
        m_nanoActiveSubmenu = -1;
        m_nanoSubmenuProgress = 0.f;
        m_nanoNestedSelected = 0;
        m_nanoActiveNestedSubmenu = -1;
        m_nanoNestedProgress = 0.f;
        m_nanoFloatTime = 0.f;
        m_nanoFinalizeClose = false;
        m_nanoLaunchFinishTime = 0.f;
        {
            auto& state = brls::Application::getControllerState();
            const float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
            const float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
            const float navY = std::abs(ry) > std::abs(ly) ? ry : ly;
            m_nanoPrevUp = state.buttons[static_cast<int>(brls::BUTTON_UP)] || navY < -0.5f;
            m_nanoPrevDown = state.buttons[static_cast<int>(brls::BUTTON_DOWN)] || navY > 0.5f;
            m_nanoPrevA = state.buttons[static_cast<int>(brls::BUTTON_A)];
            m_nanoPrevB = state.buttons[static_cast<int>(brls::BUTTON_B)];
        }
        m_lastFrameTime = std::chrono::steady_clock::now();
        if (m_nanoImageHandle >= 0 && m_nanoOwnsImageHandle) {
            if (auto* vg = brls::Application::getNVGContext())
                nvgDeleteImage(vg, m_nanoImageHandle);
        }
        m_nanoImageHandle = -1;
        m_nanoOwnsImageHandle = false;
        if (m_nanoVgMenu && m_nanoHasProvidedImage) {
            m_nanoImageHandle = m_nanoProvidedImageHandle;
        } else if (m_nanoVgMenu && !entry.logoPath.empty()) {
            if (auto* vg = brls::Application::getNVGContext())
                m_nanoImageHandle = nvgCreateImage(vg, entry.logoPath.c_str(), 0);
            m_nanoOwnsImageHandle = m_nanoImageHandle >= 0;
        }
        _buildUI(entry);
        this->setVisibility(brls::Visibility::VISIBLE);
        if (m_nanoVgMenu)
            brls::Application::giveFocus(this);
        if (m_panel)
            m_panel->setTranslationX(0.f);
        if (m_previewCard) {
            m_previewCard->setAlpha(0.f);
            m_previewCard->setTranslationY(34.f);
        }
        for (auto* btn : m_btnInstances)
            btn->setTranslationX(520.f);
    }

    void GameOptionsSidebar::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);
        if (!m_isOpen)
            return;

        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
        if (m_nanoVgMenu) {
            _frameNanoVg(dt);
            return;
        }
        if (!m_panel)
            return;
        if (m_isClosing)
            m_openProgress = std::max(0.f, m_openProgress - dt * 4.2f);
        else
            m_openProgress = std::min(1.f, m_openProgress + dt * 6.0f);
        const float eased = 1.f - std::pow(1.f - m_openProgress, 3.f);
        this->setBackgroundColor(nvgRGBA(0, 0, 0,
            static_cast<unsigned char>(155.f * eased)));
        if (m_previewCard) {
            if (m_isClosing && m_launchClosing) {
                const float launchProgress = 1.f - m_openProgress;
                m_previewCard->setAlpha(1.f);
                m_previewCard->setTranslationY(0.f);
                m_previewCard->setTranslationX(launchProgress * 315.f);
            } else {
                m_previewCard->setAlpha(eased);
                m_previewCard->setTranslationY((1.f - eased) * 34.f);
                m_previewCard->setTranslationX(0.f);
            }
        }
        for (size_t i = 0; i < m_btnInstances.size(); ++i) {
            const float delay = static_cast<float>(i) * 0.065f;
            const float local = std::max(0.f, std::min(1.f,
                (m_openProgress - delay) / std::max(0.01f, 1.f - delay)));
            const float buttonEased = 1.f - std::pow(1.f - local, 3.f);
            m_btnInstances[i]->setTranslationX((1.f - buttonEased) * 520.f);
            m_btnInstances[i]->setAlpha(buttonEased);
        }

        if (m_isClosing && m_openProgress <= 0.f) {
            auto completion = std::move(m_closeCompletion);
            _destroyUI();
            m_isOpen = false;
            m_isClosing = false;
            m_launchClosing = false;
            this->setVisibility(brls::Visibility::GONE);
            if (onClosed)
                onClosed();
            if (completion)
                completion();
        }
    }

    void GameOptionsSidebar::close(std::function<void()> completion)
    {
        if (!m_isOpen || m_isClosing) return;
        m_closeCompletion = std::move(completion);
        m_launchClosing = false;
        m_isClosing = true;
    }

    void GameOptionsSidebar::closeForLaunch(std::function<void()> completion)
    {
        if (!m_isOpen || m_isClosing) return;
        m_closeCompletion = std::move(completion);
        m_launchClosing = true;
        m_isClosing = true;
    }

    void GameOptionsSidebar::_frameNanoVg(float dt)
    {
        m_nanoFloatTime += dt;
        if (m_isClosing)
            m_openProgress = std::max(0.f, m_openProgress - dt *
                (m_launchClosing ? 2.8f : 3.8f));
        else
            m_openProgress = std::min(1.f, m_openProgress + dt * 4.2f);

        const float submenuTarget = m_nanoInSubmenu ? 1.f : 0.f;
        m_nanoSubmenuProgress += (submenuTarget - m_nanoSubmenuProgress) *
            std::min(1.f, dt * 13.f);
        const float nestedTarget = m_nanoInNestedSubmenu ? 1.f : 0.f;
        m_nanoNestedProgress += (nestedTarget - m_nanoNestedProgress) *
            std::min(1.f, dt * 14.f);

        auto& state = brls::Application::getControllerState();
        const float ly = state.axes[static_cast<int>(brls::LEFT_Y)];
        const float ry = state.axes[static_cast<int>(brls::RIGHT_Y)];
        const float navY = std::abs(ry) > std::abs(ly) ? ry : ly;
        const bool up = state.buttons[static_cast<int>(brls::BUTTON_UP)] || navY < -0.5f;
        const bool down = state.buttons[static_cast<int>(brls::BUTTON_DOWN)] || navY > 0.5f;
        const bool a = state.buttons[static_cast<int>(brls::BUTTON_A)];
        const bool b = state.buttons[static_cast<int>(brls::BUTTON_B)];

        if (!m_isClosing && m_openProgress > 0.72f) {
            const std::vector<ButtonConfig>* menu = &m_buttons;
            int* selected = &m_nanoRootSelected;
            if (m_nanoInSubmenu && m_nanoActiveSubmenu >= 0 &&
                static_cast<size_t>(m_nanoActiveSubmenu) < m_buttons.size()) {
                menu = &m_buttons[static_cast<size_t>(m_nanoActiveSubmenu)].children;
                selected = &m_nanoChildSelected;
                if (m_nanoInNestedSubmenu && m_nanoActiveNestedSubmenu >= 0 &&
                    static_cast<size_t>(m_nanoActiveNestedSubmenu) < menu->size()) {
                    menu = &(*menu)[static_cast<size_t>(m_nanoActiveNestedSubmenu)].children;
                    selected = &m_nanoNestedSelected;
                }
            }

            if (!menu->empty()) {
                if (up && !m_nanoPrevUp) {
                    *selected = (*selected - 1 + static_cast<int>(menu->size())) %
                        static_cast<int>(menu->size());
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
                if (down && !m_nanoPrevDown) {
                    *selected = (*selected + 1) % static_cast<int>(menu->size());
                    brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                }
                if (a && !m_nanoPrevA) {
                    auto& item = (*menu)[static_cast<size_t>(*selected)];
                    if (!item.children.empty()) {
                        if (!m_nanoInSubmenu) {
                            m_nanoActiveSubmenu = *selected;
                            m_nanoChildSelected = 0;
                            m_nanoInSubmenu = true;
                        } else if (!m_nanoInNestedSubmenu) {
                            m_nanoActiveNestedSubmenu = *selected;
                            m_nanoNestedSelected = 0;
                            m_nanoInNestedSubmenu = true;
                        }
                        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_SIDEBAR);
                    } else if (item.callback) {
                        auto callback = item.callback;
                        callback(m_entry);
                    }
                }
            }

        }

        // B 从入场首帧起即可反向关闭；A 和方向键仍等待菜单基本展开。
        if (!m_isClosing && b && !m_nanoPrevB) {
            if (m_nanoInNestedSubmenu) {
                m_nanoInNestedSubmenu = false;
                brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            } else if (m_nanoInSubmenu) {
                m_nanoInSubmenu = false;
                brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            } else {
                if (onCloseRequested)
                    onCloseRequested();
                else
                    close();
                brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            }
        }

        m_nanoPrevUp = up;
        m_nanoPrevDown = down;
        m_nanoPrevA = a;
        m_nanoPrevB = b;

        if (m_isClosing && m_openProgress <= 0.f && m_launchClosing) {
            if (m_launchFadeToBlack) {
                m_nanoLaunchFinishTime += dt;
                if (m_nanoLaunchFinishTime < 1.18f)
                    return;
            }
            if (!m_nanoFinalizeClose) {
                m_nanoFinalizeClose = true;
                return;
            }
        }
        if (m_isClosing && m_openProgress <= 0.f) {
            auto completion = std::move(m_closeCompletion);
            _destroyUI();
            m_isOpen = false;
            m_isClosing = false;
            m_launchClosing = false;
            m_nanoFinalizeClose = false;
            this->setVisibility(brls::Visibility::GONE);
            if (onClosed)
                onClosed();
            if (completion)
                completion();
        }
    }

    void GameOptionsSidebar::draw(NVGcontext* vg, float x, float y, float w, float h,
                                  brls::Style style, brls::FrameContext* ctx)
    {
        if (m_nanoVgMenu) {
            _drawNanoVg(vg, x, y, w, h);
            return;
        }
        brls::Box::draw(vg, x, y, w, h, style, ctx);
    }

    void GameOptionsSidebar::_drawNanoVg(NVGcontext* vg, float x, float y,
                                         float w, float h)
    {
        if (!vg || !m_isOpen)
            return;
        const float openEased = 1.f - std::pow(1.f - m_openProgress, 3.f);
        const float closeEased = m_openProgress * m_openProgress *
            (3.f - 2.f * m_openProgress);
        const float visualProgress = m_isClosing && !m_launchClosing
            ? closeEased
            : openEased;
        const float launchMorph = m_launchClosing ? 1.f - m_openProgress : 0.f;
        const float maskProgress = m_launchClosing ? 1.f : visualProgress;

        nvgSave(vg);
        nvgIntersectScissor(vg, x, y, w, h);
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0,
            static_cast<unsigned char>(210.f * maskProgress)));
        nvgFill(vg);

        const float floatOffset = m_launchClosing
            ? std::sin(launchMorph * 6.2831853f) * 5.f
            : std::sin(m_nanoFloatTime * 2.5f) * 6.f;
        const float baseCardW = 330.f;
        const float baseCardH = 470.f;
        const float baseCardX = x + w * 0.25f - baseCardW * 0.5f;
        const float baseCardY = y + 118.f + floatOffset;
        const float targetCardW = 650.f;
        const float targetCardH = 260.f;
        const float targetCardX = x + (w - targetCardW) * 0.5f;
        const float targetCardY = y + (h - targetCardH) * 0.5f - 4.f +
            floatOffset;
        const float cardX = baseCardX + (targetCardX - baseCardX) * launchMorph;
        const float cardY = baseCardY + (targetCardY - baseCardY) * launchMorph;
        const float cardW = baseCardW + (targetCardW - baseCardW) * launchMorph;
        const float cardH = baseCardH + (targetCardH - baseCardH) * launchMorph;

        float cardEntrance = visualProgress;
        if (!m_isClosing) {
            const float t = m_openProgress - 1.f;
            cardEntrance = 1.f + 2.70158f * t * t * t + 1.70158f * t * t;
        }
        const float cardScale = m_launchClosing
            ? 1.f
            : 0.84f + 0.16f * cardEntrance;
        nvgSave(vg);
        nvgTranslate(vg, cardX + cardW * 0.5f, cardY + cardH * 0.5f);
        nvgScale(vg, cardScale, cardScale);
        nvgTranslate(vg, -(cardX + cardW * 0.5f),
                     -(cardY + cardH * 0.5f));

        const bool iconPreview = m_nanoPreviewIcon != 0 && !m_launchClosing;
        if (!iconPreview) {
            const float cardBodyAlpha = m_launchClosing ? 1.f : visualProgress;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, cardX, cardY, cardW, cardH, 16.f);
            nvgFillColor(vg, launchMorph > 0.2f
                ? nvgRGBA(28, 31, 38,
                    static_cast<unsigned char>(230.f * cardBodyAlpha))
                : (m_entry.favourite
                    ? nvgRGBA(224, 166, 87,
                        static_cast<unsigned char>(42.f * cardBodyAlpha))
                    : nvgRGBA(255, 255, 255,
                        static_cast<unsigned char>(13.f * cardBodyAlpha))));
            nvgFill(vg);
            nvgStrokeColor(vg, m_entry.favourite && launchMorph < 0.2f
                ? nvgRGBA(224, 166, 87,
                    static_cast<unsigned char>(190.f * cardBodyAlpha))
                : nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(115.f * cardBodyAlpha)));
            nvgStrokeWidth(vg, 1.3f);
            nvgStroke(vg);
        }

        const float baseImageX = baseCardX + 16.f;
        const float baseImageY = baseCardY + 16.f;
        const float baseImageW = baseCardW - 32.f;
        const float baseImageH = 330.f;
        const float targetImageX = targetCardX + 24.f;
        const float targetImageY = targetCardY + 24.f;
        const float targetImageW = 190.f;
        const float targetImageH = targetCardH - 48.f;
        const float imageBoxX = baseImageX + (targetImageX - baseImageX) * launchMorph;
        const float imageBoxY = baseImageY + (targetImageY - baseImageY) * launchMorph;
        const float imageBoxW = baseImageW + (targetImageW - baseImageW) * launchMorph;
        const float imageBoxH = baseImageH + (targetImageH - baseImageH) * launchMorph;

        nvgBeginPath(vg);
        nvgRoundedRect(vg, imageBoxX, imageBoxY, imageBoxW, imageBoxH, 9.f);
        if (iconPreview) {
            const float iconCenterX = x + w * 0.25f;
            const float iconCenterY = y + h * 0.46f + floatOffset;
            nvgBeginPath(vg);
            nvgCircle(vg, iconCenterX, iconCenterY, 104.f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(20.f * visualProgress)));
            nvgFill(vg);
            nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(90.f * visualProgress)));
            nvgStrokeWidth(vg, 1.5f);
            nvgStroke(vg);
            nvgFontFaceId(vg, m_nanoMaterialFontId);
            nvgFontSize(vg, 104.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(245, 247, 252,
                static_cast<unsigned char>(240.f * visualProgress)));
            const std::string previewIcon = encodeUtf8(m_nanoPreviewIcon);
            nvgText(vg, iconCenterX, iconCenterY, previewIcon.c_str(), nullptr);
            if (!m_nanoPreviewLabel.empty()) {
                nvgFontFaceId(vg, m_nanoFontId);
                nvgFontSize(vg, 24.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_TOP);
                nvgFillColor(vg, nvgRGBA(235, 239, 247,
                    static_cast<unsigned char>(235.f * visualProgress)));
                nvgText(vg, iconCenterX, iconCenterY + 132.f,
                        m_nanoPreviewLabel.c_str(), nullptr);
            }
        } else if (m_nanoImageHandle >= 0) {
            int iw = 0, ih = 0;
            nvgImageSize(vg, m_nanoImageHandle, &iw, &ih);
            float drawW = imageBoxW;
            float drawH = iw > 0 ? drawW * static_cast<float>(ih) / static_cast<float>(iw) : imageBoxH;
            if (drawH > imageBoxH) {
                drawH = imageBoxH;
                drawW = ih > 0 ? drawH * static_cast<float>(iw) / static_cast<float>(ih) : imageBoxW;
            }
            const float drawX = imageBoxX + (imageBoxW - drawW) * 0.5f;
            const float drawY = imageBoxY + (imageBoxH - drawH) * 0.5f;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, drawX, drawY, drawW, drawH, 9.f);
            const float contentAlpha = m_launchClosing ? 1.f : visualProgress;
            NVGpaint paint = nvgImagePattern(vg, drawX, drawY, drawW, drawH,
                                             0.f, m_nanoImageHandle, contentAlpha);
            nvgFillPaint(vg, paint);
            nvgFill(vg);
        } else {
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(22.f *
                    (m_launchClosing ? 1.f : visualProgress))));
            nvgFill(vg);
        }

        const float baseTitleX = baseCardX + 18.f;
        const float baseTitleY = baseCardY + 368.f;
        const float targetTitleX = targetCardX + 242.f;
        const float targetTitleY = targetCardY + 48.f;
        const float titleX = baseTitleX + (targetTitleX - baseTitleX) * launchMorph;
        const float titleY = baseTitleY + (targetTitleY - baseTitleY) * launchMorph;
        nvgFontFaceId(vg, m_nanoFontId);
        nvgFontSize(vg, 22.f + launchMorph * 6.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        const float cardContentAlpha = m_launchClosing ? 1.f : visualProgress;
        nvgFillColor(vg, nvgRGBA(255, 255, 255,
            static_cast<unsigned char>(245.f * cardContentAlpha)));
        nvgSave(vg);
        nvgIntersectScissor(vg, titleX, titleY,
            launchMorph > 0.5f ? 376.f : 294.f, 66.f);
        if (!iconPreview)
            nvgText(vg, titleX, titleY,
                    m_entry.title.empty() ? L("未知游戏").c_str() : m_entry.title.c_str(), nullptr);
        nvgRestore(vg);

        const std::string meta = beiklive::tools::platformBadgeName(m_entry.platform) +
            (m_entry.playTime > 0
                ? "  ·  " + beiklive::tools::formatPlayTime(m_entry.playTime)
                : "");
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(207, 214, 228,
            static_cast<unsigned char>(210.f * (1.f - launchMorph) * visualProgress)));
        if (!iconPreview)
            nvgText(vg, baseCardX + 18.f, baseCardY + 417.f, meta.c_str(), nullptr);

        if (launchMorph > 0.f) {
            const float infoX = targetCardX + 242.f;
            const unsigned char launchAlpha = static_cast<unsigned char>(235.f * launchMorph);
            nvgFontSize(vg, 18.f);
            nvgFillColor(vg, nvgRGBA(215, 221, 232, launchAlpha));
            nvgText(vg, infoX, targetCardY + targetCardH - 92.f,
                    L("启动中...").c_str(), nullptr);
        }
        const bool launchPanelReady = m_launchClosing &&
            m_openProgress <= 0.001f;
        if (launchPanelReady) {
            const float infoX = targetCardX + 242.f;
            const float infoW = targetCardW - 270.f;
            const float loadProgress = m_launchFadeToBlack
                ? std::max(0.f, std::min(1.f, m_nanoLaunchFinishTime))
                : 0.f;
            const float barY = targetCardY + targetCardH - 54.f;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, infoX, barY, infoW, 12.f, 6.f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                30));
            nvgFill(vg);
            if (loadProgress > 0.f) {
                nvgBeginPath(vg);
                nvgRoundedRect(vg, infoX, barY,
                               std::max(12.f, infoW * loadProgress),
                               12.f, 6.f);
                NVGpaint progressPaint = nvgLinearGradient(
                    vg, infoX, barY, infoX + infoW, barY,
                    nvgRGBA(94, 185, 255, 235),
                    nvgRGBA(247, 197, 104, 235));
                nvgFillPaint(vg, progressPaint);
                nvgFill(vg);
            }
        }
        nvgRestore(vg);

        const float menuAlpha = m_launchClosing
            ? closeEased
            : visualProgress;
        const float rightCenterX = x + w * 0.75f;
        const float rootBaseX = rightCenterX - 315.f * 0.5f;
        const float childBaseX = rightCenterX - 350.f * 0.5f;
        const float rootX = rootBaseX - m_nanoSubmenuProgress * 350.f;
        const float childX = childBaseX + (1.f - m_nanoSubmenuProgress) * 340.f
            - m_nanoNestedProgress * 370.f;
        const float nestedX = childBaseX + (1.f - m_nanoNestedProgress) * 360.f;
        auto menuHeight = [](size_t count, bool compact) {
            if (count == 0)
                return 0.f;
            const float itemH = compact ? 45.f : 64.f;
            const float gap = compact ? 8.f : 12.f;
            return static_cast<float>(count) * itemH +
                static_cast<float>(count - 1) * gap;
        };
        const float rootMenuY = y + (h - menuHeight(m_buttons.size(), false)) * 0.5f;
        float childMenuY = rootMenuY;
        float nestedMenuY = rootMenuY;
        if (m_nanoActiveSubmenu >= 0 &&
            static_cast<size_t>(m_nanoActiveSubmenu) < m_buttons.size()) {
            const auto& children =
                m_buttons[static_cast<size_t>(m_nanoActiveSubmenu)].children;
            childMenuY = y + (h - menuHeight(children.size(), false)) * 0.5f;
            if (m_nanoActiveNestedSubmenu >= 0 &&
                static_cast<size_t>(m_nanoActiveNestedSubmenu) < children.size()) {
                nestedMenuY = y + (h - menuHeight(
                    children[static_cast<size_t>(m_nanoActiveNestedSubmenu)].children.size(),
                    false)) * 0.5f;
            }
        }
        const float menuAnimationProgress = m_launchClosing
            ? closeEased
            : visualProgress;

        auto drawMenu = [&](const std::vector<ButtonConfig>& menu, int selected,
                            float mx, float my, float mw, float alpha,
                            bool compact) {
            const float itemH = compact ? 45.f : 64.f;
            const float gap = compact ? 8.f : 12.f;
            for (size_t i = 0; i < menu.size(); ++i) {
                const float iy = my + static_cast<float>(i) * (itemH + gap);
                const float delay = std::min(0.28f, static_cast<float>(i) * 0.055f);
                const float localProgress = menuAnimationProgress <= delay
                    ? 0.f
                    : std::min(1.f, (menuAnimationProgress - delay) /
                        std::max(0.01f, 1.f - delay));
                const float itemEased = 1.f -
                    std::pow(1.f - localProgress, 3.f);
                const float ix = mx + (1.f - itemEased) *
                    (compact ? 58.f : 110.f);
                const float itemAlpha = alpha * itemEased;
                const bool focused = static_cast<int>(i) == selected;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, ix, iy, mw, itemH, compact ? 8.f : 11.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>((focused ? 48.f : 13.f) * itemAlpha)));
                nvgFill(vg);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(55.f * itemAlpha)));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
                if (focused) {
                    beiklive::ui::drawGradientFocusBorder(
                        vg, ix, iy, mw, itemH,
                        compact ? 8.f : 11.f, 3.f, itemAlpha,
                        beiklive::ui::gradientFocusAnimationOffset(
                            m_nanoFloatTime));
                }

                const std::string icon = encodeUtf8(menu[i].iconCodepoint);
                nvgFontFaceId(vg, m_nanoFontId);
                nvgFontSize(vg, compact ? 14.f : 23.f);
                float textBounds[4]{};
                nvgTextBounds(vg, 0.f, 0.f, menu[i].text.c_str(), nullptr,
                              textBounds);
                const float iconSize = compact ? 18.f : 29.f;
                const float contentGap = compact ? 8.f : 17.f;
                const float contentW = iconSize + contentGap +
                    (textBounds[2] - textBounds[0]);
                const float contentCenterX = ix + mw * 0.5f -
                    (!menu[i].children.empty() ? 8.f : 0.f);
                const float contentX = contentCenterX - contentW * 0.5f;
                nvgFontFaceId(vg, m_nanoMaterialFontId);
                nvgFontSize(vg, iconSize);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(245, 247, 252,
                    static_cast<unsigned char>(245.f * itemAlpha)));
                nvgText(vg, contentX + iconSize * 0.5f, iy + itemH * 0.5f,
                        icon.c_str(), nullptr);

                nvgFontFaceId(vg, m_nanoFontId);
                nvgFontSize(vg, compact ? 14.f : 23.f);
                nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                nvgText(vg, contentX + iconSize + contentGap,
                        iy + itemH * 0.5f,
                        menu[i].text.c_str(), nullptr);
                if (!menu[i].children.empty()) {
                    nvgFontSize(vg, compact ? 16.f : 26.f);
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgText(vg, ix + mw - 22.f, iy + itemH * 0.5f, "›", nullptr);
                }
            }
        };

        if (menuAlpha > 0.01f) {
            drawMenu(m_buttons, m_nanoRootSelected, rootX, rootMenuY, 315.f,
                     menuAlpha * (1.f - m_nanoSubmenuProgress * 0.65f), false);
            if (m_nanoActiveSubmenu >= 0 &&
                static_cast<size_t>(m_nanoActiveSubmenu) < m_buttons.size()) {
                const auto& children = m_buttons[static_cast<size_t>(m_nanoActiveSubmenu)].children;
                if (m_nanoSubmenuProgress > 0.02f)
                    drawMenu(children, m_nanoChildSelected, childX, childMenuY, 350.f,
                             menuAlpha * m_nanoSubmenuProgress *
                                (1.f - m_nanoNestedProgress * 0.65f), false);
                if (m_nanoActiveNestedSubmenu >= 0 &&
                    static_cast<size_t>(m_nanoActiveNestedSubmenu) < children.size() &&
                    m_nanoNestedProgress > 0.02f) {
                    const auto& nested = children[
                        static_cast<size_t>(m_nanoActiveNestedSubmenu)].children;
                    drawMenu(nested, m_nanoNestedSelected, nestedX, nestedMenuY,
                             350.f, menuAlpha * m_nanoNestedProgress, false);
                }
            }
            if (!m_nanoInSubmenu && m_nanoSubmenuProgress < 0.04f &&
                m_nanoRootSelected >= 0 &&
                static_cast<size_t>(m_nanoRootSelected) < m_buttons.size()) {
                const auto& preview = m_buttons[static_cast<size_t>(m_nanoRootSelected)].children;
                if (!preview.empty())
                    drawMenu(preview, -1, x + w - 172.f,
                              y + (h - menuHeight(preview.size(), true)) * 0.5f,
                              160.f, menuAlpha * 0.72f, true);
            }
            if (m_nanoInSubmenu && !m_nanoInNestedSubmenu &&
                m_nanoActiveSubmenu >= 0 &&
                static_cast<size_t>(m_nanoActiveSubmenu) < m_buttons.size()) {
                const auto& children =
                    m_buttons[static_cast<size_t>(m_nanoActiveSubmenu)].children;
                if (m_nanoChildSelected >= 0 &&
                    static_cast<size_t>(m_nanoChildSelected) < children.size()) {
                    const auto& preview = children[
                        static_cast<size_t>(m_nanoChildSelected)].children;
                    if (!preview.empty())
                        drawMenu(preview, -1, x + w - 172.f,
                            y + (h - menuHeight(preview.size(), true)) * 0.5f,
                            160.f, menuAlpha * 0.72f, true);
                }
            }
        }

        auto drawHint = [&](brls::ControllerButton button,
                            const std::string& label, float hx) {
            const float hintY = y + h - 36.f;
            const std::string glyph = brls::Hint::getKeyIcon(button);
            nvgFontFaceId(vg, m_nanoSwitchFontId);
            nvgFontSize(vg, 30.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(255, 255, 255,
                static_cast<unsigned char>(245.f * menuAlpha)));
            nvgText(vg, hx, hintY, glyph.c_str(), nullptr);
            nvgFontFaceId(vg, m_nanoFontId);
            nvgFontSize(vg, 22.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgText(vg, hx + 22.f, hintY, label.c_str(), nullptr);
        };
        drawHint(brls::BUTTON_A, L("选择"), x + w - 214.f);
        drawHint(brls::BUTTON_B, L("返回"), x + w - 104.f);

        if (m_launchClosing && m_launchFadeToBlack &&
            m_nanoLaunchFinishTime > 1.f) {
            const float fade = std::max(0.f, std::min(
                1.f, (m_nanoLaunchFinishTime - 1.f) / 0.18f));
            const float easedFade = fade * fade * (3.f - 2.f * fade);
            nvgBeginPath(vg);
            nvgRect(vg, x, y, w, h);
            nvgFillColor(vg, nvgRGBA(0, 0, 0,
                static_cast<unsigned char>(255.f * easedFade)));
            nvgFill(vg);
        }

        nvgResetScissor(vg);
        nvgRestore(vg);
    }

    void GameOptionsSidebar::_buildUI(const beiklive::GameEntry& entry)
    {
        m_btnInstances.clear();

        if (m_nanoVgMenu) {
            this->setFocusable(true);
            this->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
            this->setCustomNavigationRoute(brls::FocusDirection::UP, this);
            this->setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
            this->setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
            this->setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);
            this->registerAction("", brls::BUTTON_A,
                [](brls::View*) -> bool { return true; }, true, false,
                brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_B,
                [](brls::View*) -> bool { return true; }, true, false,
                brls::SOUND_NONE);
            auto consumeNavigation = [](brls::View*) -> bool { return true; };
            this->registerAction("", brls::BUTTON_NAV_UP, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_NAV_DOWN, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_NAV_LEFT, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_NAV_RIGHT, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_UP, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_DOWN, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_LEFT, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            this->registerAction("", brls::BUTTON_RIGHT, consumeNavigation,
                                 true, true, brls::SOUND_NONE);
            return;
        }

        m_panel = new brls::Box(brls::Axis::COLUMN);
        m_panel->setFocusable(false);
        m_panel->setWidth(460.f);
        m_panel->setHeightPercentage(100.f);
        m_panel->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        m_panel->setLineLeft(0.f);
        m_panel->setPadding(30.f, 26.f, 0.f, 26.f);
        m_panel->setAlignItems(brls::AlignItems::STRETCH);

        auto* divider = new brls::Rectangle(nvgRGBA(255, 255, 255, 50));
        divider->setWidthPercentage(100);
        divider->setHeight(1.f);
        divider->setMarginBottom(18.f);
        m_panel->addView(divider);

        auto* sectionLabel = new brls::Label();
        sectionLabel->setText(entry.path.empty() ? L("批量操作") : L("游戏操作"));
        sectionLabel->setFontSize(17.f);
        sectionLabel->setTextColor(nvgRGBA(205, 211, 224, 220));
        sectionLabel->setFocusable(false);
        sectionLabel->setMarginBottom(12.f);
        m_panel->addView(sectionLabel);

        // B 键关闭
        auto closeAction = [this](brls::View*) {
            close();
            return true;
        };

        // ── 动态添加按钮 ──
        if (m_buttons.empty())
        {
            // 无按钮时显示提示
            auto* emptyLabel = new brls::Label();
            emptyLabel->setText(L("无可用操作"));
            emptyLabel->setFontSize(18.f);
            emptyLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
            emptyLabel->setFocusable(false);
            emptyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            emptyLabel->setMarginBottom(20.f);
            m_panel->addView(emptyLabel);
        }
        else
        {
            for (size_t i = 0; i < m_buttons.size(); ++i)
            {
                const auto& cfg = m_buttons[i];

                auto* btn = new beiklive::ButtonBox();
                btn->setText(cfg.text);
                if (m_showButtonIcons)
                    btn->setIcon(cfg.iconCodepoint);
                btn->setHeight(54.f);
                btn->setCornerRadius(6.f);
                btn->setBackgroundColor(nvgRGBA(255, 255, 255, 12));
                btn->setMarginBottom(8.f);
                btn->onFocusGainedCallback = [btn]() {
                    btn->setBackgroundColor(nvgRGBA(255, 255, 255, 38));
                };
                btn->onFocusLostCallback = [btn]() {
                    btn->setBackgroundColor(nvgRGBA(255, 255, 255, 12));
                };

                // A 键：触发回调
                auto cb = cfg.callback; // 拷贝
                btn->registerClickAction([this, cb](brls::View*) -> bool {
                    if (cb) cb(m_entry);
                    return true;
                });

                // B 键：关闭面板
                btn->registerAction(L("关闭"), brls::BUTTON_B, closeAction);

                // 禁用左右导航，防止焦点飞出
                btn->setCustomNavigationRoute(brls::FocusDirection::LEFT,  btn);
                btn->setCustomNavigationRoute(brls::FocusDirection::RIGHT, btn);

                m_btnInstances.push_back(btn);
                m_panel->addView(btn);
            }

            // 首尾循环导航
            if (m_btnInstances.size() >= 2)
            {
                m_btnInstances.front()->setCustomNavigationRoute(
                    brls::FocusDirection::UP, m_btnInstances.back());
                m_btnInstances.back()->setCustomNavigationRoute(
                    brls::FocusDirection::DOWN, m_btnInstances.front());
            }
        }

        // 弹性空间
        m_panel->addView(new brls::Padding());

        // HintsBar 按钮提示栏
        auto* hintsBar = new beiklive::HintsBar();
        m_panel->addView(hintsBar);

        // 面板包装器：右对齐
        auto* panelWrapper = new brls::Box(brls::Axis::ROW);
        panelWrapper->setFocusable(false);
        panelWrapper->setWidthPercentage(100);
        panelWrapper->setHeightPercentage(100);
        panelWrapper->setJustifyContent(entry.path.empty()
            ? brls::JustifyContent::FLEX_END
            : brls::JustifyContent::SPACE_BETWEEN);
        panelWrapper->setBackground(brls::ViewBackground::NONE);
        if (!entry.path.empty()) {
            auto* previewHolder = new brls::Box(brls::Axis::COLUMN);
            previewHolder->setFocusable(false);
            previewHolder->setWidth(650.f);
            previewHolder->setHeightPercentage(100.f);
            previewHolder->setAlignItems(brls::AlignItems::CENTER);
            previewHolder->setJustifyContent(brls::JustifyContent::CENTER);

            m_previewCard = new brls::Box(brls::Axis::COLUMN);
            m_previewCard->setFocusable(false);
            m_previewCard->setWidth(320.f);
            m_previewCard->setHeight(460.f);
            m_previewCard->setPadding(16.f, 16.f, 16.f, 16.f);
            m_previewCard->setCornerRadius(12.f);
            m_previewCard->setBackgroundColor(entry.favourite
                ? nvgRGBA(224, 166, 87, 34)
                : nvgRGBA(255, 255, 255, 10));
            m_previewCard->setLineColor(entry.favourite
                ? nvgRGBA(224, 166, 87, 170)
                : nvgRGBA(255, 255, 255, 72));
            m_previewCard->setLineTop(1.f);
            m_previewCard->setLineBottom(1.f);
            m_previewCard->setLineLeft(1.f);
            m_previewCard->setLineRight(1.f);

            m_iconImage = new brls::Image();
            m_iconImage->setWidth(288.f);
            m_iconImage->setHeight(326.f);
            m_iconImage->setCornerRadius(8.f);
            m_iconImage->setScalingType(brls::ImageScalingType::FIT);
            m_iconImage->setFocusable(false);
            if (!entry.logoPath.empty())
                m_iconImage->setImageFromFile(entry.logoPath);
            m_previewCard->addView(m_iconImage);

            m_titleLabel = new brls::Label();
            m_titleLabel->setText(entry.title.empty() ? L("未知游戏") : entry.title);
            m_titleLabel->setFontSize(22.f);
            m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
            m_titleLabel->setFocusable(false);
            m_titleLabel->setSingleLine(true);
            m_titleLabel->setAnimated(true);
            m_titleLabel->setMarginTop(14.f);
            m_previewCard->addView(m_titleLabel);

            std::string meta = beiklive::tools::platformBadgeName(entry.platform);
            if (entry.playTime > 0) {
                if (!meta.empty()) meta += "  ·  ";
                meta += beiklive::tools::formatPlayTime(entry.playTime);
            }
            auto* metaLabel = new brls::Label();
            metaLabel->setText(meta);
            metaLabel->setFontSize(15.f);
            metaLabel->setTextColor(nvgRGBA(208, 214, 226, 210));
            metaLabel->setFocusable(false);
            metaLabel->setMarginTop(8.f);
            m_previewCard->addView(metaLabel);

            previewHolder->addView(m_previewCard);
            panelWrapper->addView(previewHolder);
        }
        panelWrapper->addView(m_panel);

        this->addView(panelWrapper);

        // 焦点定位到第一个按钮
        if (!m_btnInstances.empty())
            brls::Application::giveFocus(m_btnInstances.front());
    }

    void GameOptionsSidebar::_destroyUI()
    {
        this->clearViews(true);
        if (m_nanoImageHandle >= 0 && m_nanoOwnsImageHandle) {
            if (auto* vg = brls::Application::getNVGContext())
                nvgDeleteImage(vg, m_nanoImageHandle);
        }
        m_nanoImageHandle = -1;
        m_nanoOwnsImageHandle = false;
        m_btnInstances.clear();
        m_panel      = nullptr;
        m_previewCard = nullptr;
        m_titleLabel = nullptr;
        m_iconImage  = nullptr;
    }

} // namespace beiklive
