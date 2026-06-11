#include "TabFrame.hpp"
#include "ui/utils/AnimationHelper.hpp"


namespace beiklive
{
    TabFrame::TabFrame()
    {
        _initLayout();
    }

    TabFrame::~TabFrame()
    {
    }

    void TabFrame::draw(NVGcontext* vg, float x, float y, float w, float h,
                        brls::Style style, brls::FrameContext* ctx)
    {
        brls::Box::draw(vg, x, y, w, h, style, ctx);
    }

    void TabFrame::addDivider()
    {
        auto* divider = new brls::Rectangle();
        divider->setWidthPercentage(100.f);
        divider->setHeight(1.f);
        divider->setColor(nvgRGBA(255, 255, 255, 50));
        divider->setMarginBottom(16.f);
        m_tabBar->addView(divider);
    }

    void TabFrame::addTab(
        const std::string &text, 
        const std::string &iconPath, 
        std::function<void()> onClick, 
        std::function<void()> onFocus, 
        std::function<void()> onBlur, 
        brls::View *contentView,
        brls::View* contentDefaultFocus)
    {
        auto* tabButton = new beiklive::ButtonBox();
        tabButton->setText(text);
        tabButton->setIcon(iconPath);

        m_tabBar->addView(tabButton);
        
        if (contentView) {
            contentView->setVisibility(brls::Visibility::GONE);
            m_contentArea->addView(contentView);

            tabButton->registerClickAction([contentView, contentDefaultFocus](brls::View*) -> bool {
                brls::Application::giveFocus(contentDefaultFocus ? contentDefaultFocus : contentView);
                return true;
            });

            tabButton->onFocusGainedCallback = [this, contentView, onFocus]() {
                bool alreadyVisible = (contentView->getVisibility() == brls::Visibility::VISIBLE);
                for (auto* child : m_contentArea->getChildren()) {
                    if (child != contentView)
                        child->setVisibility(brls::Visibility::GONE);
                }
                if (alreadyVisible) {
                    if (onFocus) onFocus();
                } else if (m_animationEnabled) {
                    float dist = m_contentArea->getWidth();
                    if (dist <= 0.f) dist = 960.f;
                    AnimationHelper::slideInFromRight(contentView, dist, 250,
                        [onFocus]() { if (onFocus) onFocus(); });
                } else {
                    contentView->setVisibility(brls::Visibility::VISIBLE);
                    if (onFocus) onFocus();
                }
            };
            tabButton->onFocusLostCallback = [onBlur]() {
                if (onBlur) onBlur();
            };
            tabButton->setCustomNavigationRoute(brls::FocusDirection::RIGHT,
                contentDefaultFocus ? contentDefaultFocus : contentView);
        } else {
            tabButton->registerClickAction([onClick](brls::View*) -> bool {
                if (onClick) onClick();
                return true;
            });

            tabButton->onFocusGainedCallback = [this, onFocus]() {
                for (auto* child : m_contentArea->getChildren())
                    child->setVisibility(brls::Visibility::GONE);
                if (onFocus) onFocus();
            };
            tabButton->onFocusLostCallback = [onBlur]() {
                if (onBlur) onBlur();
            };
            tabButton->setCustomNavigationRoute(brls::FocusDirection::RIGHT, tabButton);
        }
    }

    void TabFrame::addFinish()
    {
        if (!m_tabBar->getChildren().empty()) {
            brls::Application::giveFocus(m_tabBar->getChildren()[0]);
        }

        if (!m_tabBar->getChildren().empty()) {
            auto* firstTab = m_tabBar->getChildren()[0];
            auto* lastTab = m_tabBar->getChildren().back();
            firstTab->setCustomNavigationRoute(brls::FocusDirection::UP, lastTab);
            lastTab->setCustomNavigationRoute(brls::FocusDirection::DOWN, firstTab);
        }
    }

    void TabFrame::onShow()
    {
        brls::Application::giveFocus(m_tabBar->getChildren()[0]);
    }

    void TabFrame::_initLayout()
    {
        this->setAxis(brls::Axis::ROW);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setWidthPercentage(100.f);
        this->setGrow(1.f);
        this->setBackground(brls::ViewBackground::NONE);
        this->setFocusable(false);

        // 标签栏
        m_tabBar = new brls::Box();
        m_tabBar->setAxis(brls::Axis::COLUMN);
        m_tabBar->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_tabBar->setAlignItems(brls::AlignItems::FLEX_START);
        m_tabBar->setWidthPercentage(25.f);
        m_tabBar->setHeightPercentage(90.f);
        m_tabBar->setBackground(brls::ViewBackground::NONE);
        m_tabBar->setPadding(16.f);
        m_tabBar->setMargins(10.f, 10.f, 10.f, 10.f);
        m_tabBar->setLineRight(1.f);
        m_tabBar->setLineColor(nvgRGBA(255, 255, 255, 10));
        this->addView(m_tabBar);

        // 内容区域
        m_contentArea = new brls::Box();
        m_contentArea->setAxis(brls::Axis::COLUMN);
        m_contentArea->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_contentArea->setAlignItems(brls::AlignItems::FLEX_START);
        m_contentArea->setHeightPercentage(100.f);
        m_contentArea->setGrow(1.f);
        m_contentArea->setFocusable(false);
        m_contentArea->setBackground(brls::ViewBackground::NONE);
        m_contentArea->setPadding(16.f);
        this->addView(m_contentArea);

        m_contentArea->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            brls::sync([this]() {
                brls::Application::giveFocus(m_tabBar->getDefaultFocus());
            });
            return true;
        });
    }
}
