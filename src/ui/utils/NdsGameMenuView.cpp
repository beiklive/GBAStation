#include "NdsGameMenuView.hpp"

namespace beiklive
{

    NdsGameMenuView::NdsGameMenuView(beiklive::GameEntry gameData)
        : m_gameEntry(std::move(gameData))
    {
        _initLayout();
    }

    NdsGameMenuView::~NdsGameMenuView()
    {
    }

    void NdsGameMenuView::_initLayout()
    {
        this->setFocusable(false);
        this->setAxis(brls::Axis::COLUMN);
        HIDE_BRLS_HIGHLIGHT(this);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 240));
        this->setWidthPercentage(100.f);
        this->setHeightPercentage(100.f);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setAlignItems(brls::AlignItems::CENTER);

        m_panel = new beiklive::TabFrame();
        HIDE_BRLS_HIGHLIGHT(m_panel);

        this->getHeader()->setTitle("NDS 游戏菜单");

        m_panel->addTab(
            "返回游戏",
            BK_RES("img/ui/menu/back.png"),
            [this]()
            {
                if (m_onResume)
                    m_onResume();
            });

        m_panel->registerAction("返回", brls::BUTTON_B, [this](brls::View *) -> bool
                                {
            brls::sync([this]() {
                if (m_onResume) m_onResume();
            });
            return true; });

        m_panel->addDivider();

        m_panel->addTab(
            "退出游戏",
            BK_RES("img/ui/menu/exit.png"),
            [this]()
            {
                if (m_onExit)
                    m_onExit();
            });

        m_panel->addFinish();
        this->getContentBox()->addView(m_panel);
    }

    void NdsGameMenuView::draw(NVGcontext *vg, float x, float y, float w, float h,
                               brls::Style style, brls::FrameContext *ctx)
    {
        Box::draw(vg, x, y, w, h, style, ctx);
    }

    void NdsGameMenuView::onShow()
    {
        if (m_panel) m_panel->onShow();
    }

} // namespace beiklive
