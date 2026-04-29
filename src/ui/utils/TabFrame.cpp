#include "TabFrame.hpp"


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
        brls::View *contentView)
    {
        // 创建标签按钮
        auto* tabButton = new beiklive::ButtonBox();
        tabButton->setText(text);
        tabButton->setIcon(iconPath);
        tabButton->registerClickAction([onClick, contentView](brls::View*) -> bool {
            if (contentView) {
                brls::Application::giveFocus(contentView->getDefaultFocus());
            }else{
                if(onClick)
                    onClick();
            }
            return true;
        });
        m_tabBar->addView(tabButton);
        
        if(contentView) {
            contentView->setVisibility(brls::Visibility::GONE);
            m_contentArea->addView(contentView);
            // 聚焦时显示内容，失焦时隐藏内容            
            tabButton->onFocusGainedCallback = [contentView, onFocus]() {
                contentView->setVisibility(brls::Visibility::VISIBLE);
                contentView->setFocusable(true);
                if(onFocus) onFocus();
            };
            tabButton->onFocusLostCallback = [contentView, onBlur]() {
                contentView->setVisibility(brls::Visibility::GONE);
                contentView->setFocusable(false);
                if(onBlur) onBlur();
            };
            tabButton->setCustomNavigationRoute(brls::FocusDirection::RIGHT, contentView);

        }else{
            // 无子面板时禁止向右路由
            tabButton->setCustomNavigationRoute(brls::FocusDirection::RIGHT, tabButton);
        }

    }

    void TabFrame::addFinish()
    {
        // 添加完成后默认聚焦第一个标签
        if (!m_tabBar->getChildren().empty()) {
            brls::Application::giveFocus(m_tabBar->getChildren()[0]);
        }

        // 设置首尾路由
        if (!m_tabBar->getChildren().empty()) {
            auto* firstTab = m_tabBar->getChildren()[0];
            auto* lastTab = m_tabBar->getChildren().back();
            firstTab->setCustomNavigationRoute(brls::FocusDirection::UP, lastTab);
            lastTab->setCustomNavigationRoute(brls::FocusDirection::DOWN, firstTab);
        }
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
        m_tabBar->setHeightPercentage(100.f);
        m_tabBar->setBackground(brls::ViewBackground::NONE);
        m_tabBar->setPadding(16.f);
        m_tabBar->setMargins(10.f,10.f,10.f,10.f);
        m_tabBar->setLineRight(1.f);
        m_tabBar->setLineColor(nvgRGBA(255, 255, 255, 50));
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