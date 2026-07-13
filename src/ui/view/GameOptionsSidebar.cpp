#include "GameOptionsSidebar.hpp"
#include "core/Tools.hpp"
#include "ui/widget/HintsBar.hpp"

#include <algorithm>
#include <cmath>

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
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 72));
        
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

    void GameOptionsSidebar::open(const beiklive::GameEntry& entry)
    {
        if (m_isOpen)
            _destroyUI();

        m_entry  = entry;
        m_isOpen = true;
        m_openProgress = 0.f;
        m_lastFrameTime = std::chrono::steady_clock::now();
        _buildUI(entry);
        this->setVisibility(brls::Visibility::VISIBLE);
        if (m_panel)
            m_panel->setTranslationX(460.f);
    }

    void GameOptionsSidebar::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);
        if (!m_isOpen || !m_panel || m_openProgress >= 1.f)
            return;

        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        m_lastFrameTime = now;
        if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
        m_openProgress = std::min(1.f, m_openProgress + dt * 6.0f);
        const float eased = 1.f - std::pow(1.f - m_openProgress, 3.f);
        m_panel->setTranslationX((1.f - eased) * 460.f);
    }

    void GameOptionsSidebar::close()
    {
        if (!m_isOpen) return;
        _destroyUI();
        m_isOpen = false;
        this->setVisibility(brls::Visibility::GONE);

        if (onClosed)
            onClosed();
    }

    void GameOptionsSidebar::_buildUI(const beiklive::GameEntry& entry)
    {
        m_btnInstances.clear();

        m_panel = new brls::Box(brls::Axis::COLUMN);
        m_panel->setFocusable(false);
        m_panel->setWidth(460.f);
        m_panel->setHeightPercentage(100.f);
        m_panel->setBackgroundColor(nvgRGBA(255, 255, 255, 24));
        m_panel->setLineColor(nvgRGBA(255, 255, 255, 62));
        m_panel->setLineLeft(1.f);
        m_panel->setPadding(30.f, 26.f, 0.f, 26.f);
        m_panel->setAlignItems(brls::AlignItems::STRETCH);

        auto* titlebox = new brls::Box(brls::Axis::ROW);
        titlebox->setFocusable(false);
        titlebox->setAlignItems(brls::AlignItems::CENTER);
        titlebox->setMarginBottom(24.f);
        m_panel->addView(titlebox);

        m_iconImage = new brls::Image();
        m_iconImage->setWidth(96.f);
        m_iconImage->setHeight(126.f);
        m_iconImage->setCornerRadius(6.f);
        m_iconImage->setScalingType(brls::ImageScalingType::FIT);
        m_iconImage->setMarginRight(22.f);
        m_iconImage->setFocusable(false);
        if (!entry.logoPath.empty())
            m_iconImage->setImageFromFile(entry.logoPath);
        titlebox->addView(m_iconImage);

        auto* titleInfo = new brls::Box(brls::Axis::COLUMN);
        titleInfo->setFocusable(false);
        titleInfo->setGrow(1.f);
        titleInfo->setJustifyContent(brls::JustifyContent::CENTER);

        m_titleLabel = new brls::Label();
        m_titleLabel->setText(entry.title.empty() ? "未知游戏" : entry.title);
        m_titleLabel->setFontSize(24.f);
        m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_titleLabel->setFocusable(false);
        m_titleLabel->setSingleLine(true);
        m_titleLabel->setAnimated(true);
        m_titleLabel->setMarginBottom(12.f);
        titleInfo->addView(m_titleLabel);

        std::string meta = beiklive::tools::platformBadgeName(entry.platform);
        if (entry.playTime > 0) {
            if (!meta.empty()) meta += "  ·  ";
            meta += beiklive::tools::formatPlayTime(entry.playTime);
        }
        auto* metaLabel = new brls::Label();
        metaLabel->setText(meta.empty() ? "游戏选项" : meta);
        metaLabel->setFontSize(16.f);
        metaLabel->setTextColor(nvgRGBA(208, 214, 226, 210));
        metaLabel->setFocusable(false);
        titleInfo->addView(metaLabel);
        titlebox->addView(titleInfo);

        auto* divider = new brls::Rectangle(nvgRGBA(255, 255, 255, 50));
        divider->setWidthPercentage(100);
        divider->setHeight(1.f);
        divider->setMarginBottom(18.f);
        m_panel->addView(divider);

        auto* sectionLabel = new brls::Label();
        sectionLabel->setText("游戏操作");
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
            emptyLabel->setText("无可用操作");
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
                btn->registerAction("关闭", brls::BUTTON_B, closeAction);

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
        panelWrapper->setJustifyContent(brls::JustifyContent::FLEX_END);
        panelWrapper->setBackground(brls::ViewBackground::NONE);
        panelWrapper->addView(m_panel);

        this->addView(panelWrapper);

        // 焦点定位到第一个按钮
        if (!m_btnInstances.empty())
            brls::Application::giveFocus(m_btnInstances.front());
    }

    void GameOptionsSidebar::_destroyUI()
    {
        this->clearViews(true);
        m_btnInstances.clear();
        m_panel      = nullptr;
        m_titleLabel = nullptr;
        m_iconImage  = nullptr;
    }

} // namespace beiklive
