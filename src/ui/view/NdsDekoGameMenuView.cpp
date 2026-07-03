#include "ui/view/NdsDekoGameMenuView.hpp"

#include <borealis/views/label.hpp>

namespace beiklive {

NdsDekoGameMenuView::NdsDekoGameMenuView(beiklive::GameEntry gameEntry)
    : m_gameEntry(std::move(gameEntry))
{
    _initLayout();
}

void NdsDekoGameMenuView::_initLayout()
{
    showHeader(false);
    showFooter(false);
    showBackground(false);
    showShader(false);

    setWidthPercentage(100.f);
    setHeightPercentage(100.f);
    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::CENTER);
    setJustifyContent(brls::JustifyContent::CENTER);
    setFocusable(true);
    setBackground(brls::ViewBackground::NONE);

    auto* title = new brls::Label();
    title->setText("NDS Deko3D 专属菜单\nA/B：继续游戏    X：退出");
    title->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    title->setFontSize(24.f);
    getContentBox()->addView(title);

    registerAction("继续", brls::BUTTON_A, [this](brls::View*) -> bool {
        if (m_onResume)
            m_onResume();
        return true;
    });
    registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
        if (m_onResume)
            m_onResume();
        return true;
    });
    registerAction("退出", brls::BUTTON_X, [this](brls::View*) -> bool {
        if (m_onExit)
            m_onExit();
        return true;
    });
}

void NdsDekoGameMenuView::onShow()
{
    brls::Logger::info("NdsDekoGameMenuView: show for {}", m_gameEntry.title);
}

} // namespace beiklive
