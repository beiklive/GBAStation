#include "GameMenuView.hpp"

namespace beiklive
{
    GameMenuView::GameMenuView(beiklive::GameEntry gameData)
        : m_gameEntry(std::move(gameData))
    {
        _initLayout();
    }

    GameMenuView::~GameMenuView()
    {
    }

    void GameMenuView::_initLayout()
    {
        // 全屏透明容器，绘制半透明遮罩背景
        this->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(this);
        this->setBackground(brls::ViewBackground::NONE);
        this->setWidthPercentage(100.f);
        this->setHeightPercentage(100.f);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setAlignItems(brls::AlignItems::CENTER);

        // 居中面板
        m_panel = new brls::Box();
        m_panel->setAxis(brls::Axis::COLUMN);
        m_panel->setJustifyContent(brls::JustifyContent::CENTER);
        m_panel->setAlignItems(brls::AlignItems::CENTER);
        m_panel->setWidth(320.f);
        m_panel->setHeight(220.f);
        m_panel->setCornerRadius(12.f);
        m_panel->setBackground(brls::ViewBackground::NONE); // 通过 draw() 手动绘制
        m_panel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_panel);

        // 标题
        m_title = new brls::Label();
        m_title->setText("游戏菜单");
        m_title->setFontSize(24.f);
        m_title->setMarginBottom(24.f);
        m_title->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_panel->addView(m_title);

        // "返回游戏"按钮
        m_btnResume = _createMenuButton("▶  返回游戏", [this]() {
            if (m_onResume) m_onResume();
        });
        m_panel->addView(m_btnResume);

        // "退出游戏"按钮
        m_btnExit = _createMenuButton("✕  退出游戏", [this]() {
            if (m_onExit) m_onExit();
        });
        m_panel->addView(m_btnExit);

        this->addView(m_panel);
    }

    brls::Box* GameMenuView::_createMenuButton(const std::string& text, std::function<void()> onClick)
    {
        brls::Box* btn = new brls::Box();
        btn->setAxis(brls::Axis::ROW);
        btn->setJustifyContent(brls::JustifyContent::CENTER);
        btn->setAlignItems(brls::AlignItems::CENTER);
        btn->setWidth(240.f);
        btn->setHeight(48.f);
        btn->setCornerRadius(8.f);
        btn->setMarginBottom(16.f);
        btn->setFocusable(true);
        btn->setHideHighlightBackground(false);
        btn->setHideHighlightBorder(false);
        btn->setHideClickAnimation(false);

        brls::Label* lbl = new brls::Label();
        lbl->setText(text);
        lbl->setFontSize(18.f);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        btn->addView(lbl);

        btn->registerClickAction([btn, onClick](brls::View*) -> bool {
            onClick();
            return true;
        });

        return btn;
    }

    void GameMenuView::draw(NVGcontext* vg, float x, float y, float w, float h,
                            brls::Style style, brls::FrameContext* ctx)
    {
        // 半透明黑色全屏遮罩
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 160));
        nvgFill(vg);

        // 面板背景（通过父 Box 子坐标绘制）
        if (m_panel) {
            float px = m_panel->getX();
            float py = m_panel->getY();
            float pw = m_panel->getWidth();
            float ph = m_panel->getHeight();
            nvgBeginPath(vg);
            nvgRoundedRect(vg, px, py, pw, ph, 12.f);
            nvgFillColor(vg, nvgRGBA(30, 30, 40, 220));
            nvgFill(vg);
        }

        // 绘制子控件
        Box::draw(vg, x, y, w, h, style, ctx);
    }

} // namespace beiklive

