#include "UI/Pages/AppPage.hpp"

static constexpr float CARD_SIZE     = 270.f;  // 封面图区域边长
static constexpr float CARD_MARGIN   = 2.f;   // 卡片右间距
static constexpr float ROW_H_PAD     = 80.f;   // 卡片行左右留白
static constexpr float SCROLL_HEIGHT = 300.f;  // 横向滚动区高度（卡片+缩放动画留白）

// ============================================================
// GameCard
// ============================================================
GameCard::GameCard(const GameEntry& entry)
    : m_entry(entry)
{
    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::CENTER);
    setWidth(CARD_SIZE);
    setHeight(CARD_SIZE);
    setMarginRight(CARD_MARGIN);
    setHideHighlightBackground(true);
    setHideClickAnimation(true);

    
    m_titleLabel = new brls::Label();
    m_titleLabel->setText(entry.title.empty() ? "—" : entry.title);
    m_titleLabel->setFontSize(30.f);
    m_titleLabel->setSingleLine(true);
    m_titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);

    addView(m_titleLabel);

    m_coverImage = new brls::Image();
    m_coverImage->setCornerRadius(10.f);
    m_coverImage->setBackgroundColor(nvgRGBA(31, 31, 31, 50));
    m_coverImage->setHighlightPadding(3.f);
    m_coverImage->setHideHighlightBackground(true);
    m_coverImage->setShadowVisibility(true);
    m_coverImage->setShadowType(brls::ShadowType::GENERIC);
    m_coverImage->setHighlightCornerRadius(12.f);
    m_coverImage->setFocusable(true);
    m_coverImage->setMarginTop(20.f);
    m_coverImage->setScalingType(brls::ImageScalingType::FIT);
    m_coverImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    {
        // 使用 resolveGameCoverPath 统一处理无封面时的回退逻辑
        std::string cover = beiklive::resolveGameCoverPath(entry.cover, entry.path);
        if (!cover.empty())
            m_coverImage->setImageFromFile(cover);
        else
            m_coverImage->setImageFromFile(BK_APP_DEFAULT_LOGO);
        addView(m_coverImage);
    }


    beiklive::swallow(this, brls::BUTTON_A);
    beiklive::swallow(this, brls::BUTTON_X);

    // 手柄 A 键启动
    registerAction("beiklive/hints/start"_i18n, brls::BUTTON_A, [this](brls::View*) {
        triggerClickBounce(); 
        // if (onActivated) onActivated(m_entry);
        return true;
    }, false, false, brls::SOUND_CLICK);
    registerAction("beiklive/hints/set"_i18n, brls::BUTTON_X, [this](brls::View*) {
        if (onOptions) onOptions(m_entry);
        return true;
    });

    // 触屏点击启动
    addGestureRecognizer(new brls::TapGestureRecognizer(this));

}
void GameCard::triggerClickBounce()
{
    m_clickAnimating = true;
    m_clickT         = 0.0f;
    m_clickScale     = 1.0f;
    invalidate();
}
void GameCard::onChildFocusGained(brls::View* directChild, brls::View* focusedView)
{
    // 子节点（图片）获得焦点时调用

    Box::onChildFocusGained(directChild, focusedView);

    // brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);

    m_titleLabel->setVisibility(brls::Visibility::VISIBLE);
    m_focused = true;
    if (onFocused) onFocused(m_entry);
    invalidate();   // 触发 draw() 启动缩放动画
}

void GameCard::onChildFocusLost(brls::View* directChild, brls::View* focusedView)
{
    // 子节点（图片）失去焦点时调用

    Box::onChildFocusLost(directChild, focusedView);

    m_titleLabel->setVisibility(brls::Visibility::INVISIBLE);
    m_focused = false;
    invalidate();   // 触发 draw() 启动恢复动画
}




void GameCard::draw(NVGcontext* vg,
                    float x, float y, float w, float h,
                    brls::Style style, brls::FrameContext* ctx)
{
    // 焦点缩放
    float target = m_focused ? 1.0f : 0.9f;
    float delta  = target - m_scale;
    if (std::abs(delta) > 0.002f) {
        m_scale += delta * 0.3f;
        invalidate();
    } else {
        m_scale = target;
    }

    // 点击弹性动画（先压缩，再阻尼回弹）
    if (m_clickAnimating) {
        m_clickT += 1.0f / 60.0f; // 近似按 60fps 推进
        if (m_clickT < 0.06f) {
            float t = m_clickT / 0.06f;            // 0~1
            m_clickScale = 1.0f - 0.10f * t;       // 压到 0.90
        } else {
            float u = m_clickT - 0.06f;
            m_clickScale = 1.0f + 0.12f * std::exp(-14.0f * u) * std::sin(45.0f * u);

            if (u > 0.28f && std::abs(m_clickScale - 1.0f) < 0.003f) {
                m_clickScale = 1.0f;
                m_clickAnimating = false;
                if (onActivated) onActivated(m_entry);
            }
        }
        invalidate();
    }

    float finalScale = m_scale * m_clickScale;

    const float cx = x + w * 0.5f;
    const float cy = y + h * 0.5f;
    nvgSave(vg);
    nvgTranslate(vg,  cx,  cy);
    nvgScale(vg, finalScale, finalScale);
    nvgTranslate(vg, -cx, -cy);
    brls::Box::draw(vg, x, y, w, h, style, ctx);
    nvgRestore(vg);
}


AppPage::AppPage()
{
    setWidth(View::AUTO);
    setHeight(View::AUTO);
    setAxis(brls::Axis::COLUMN);
    setBackground(brls::ViewBackground::NONE);


    setGrow(1.0f);



    m_titleRow = new brls::Box(brls::Axis::ROW);
    m_titleRow->setHeight(100.f);
    // m_titleRow->setBackgroundColor(nvgRGBA(31, 31, 31, 50));
    addView(m_titleRow);


    // ── 横向滚动区 ──────────────────────────────────────────
    m_scroll = new brls::HScrollingFrame();
    m_scroll->setWidth(View::AUTO);
    m_scroll->setHeight(SCROLL_HEIGHT);
    m_scroll->setGrow(1.0f);
    m_scroll->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
    m_scroll->setScrollingIndicatorVisible(false);
    // m_scroll->setBackgroundColor(nvgRGBA(156, 194, 65, 50));

    m_cardRow = new brls::Box(brls::Axis::ROW);
    m_cardRow->setAlignItems(brls::AlignItems::CENTER);
    m_cardRow->setPaddingLeft(ROW_H_PAD);
    m_cardRow->setPaddingRight(ROW_H_PAD);

    m_scroll->setContentView(m_cardRow);
    m_scroll->setGrow(0.0f);

    addView(m_scroll);
    std::string path_prefix = "img/ui/" + 
    std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");

    m_ButtonRow = new beiklive::UI::ButtonBar();
    m_ButtonRow->setGrow(1.0f);
    m_ButtonRow->addButton(BK_RES(path_prefix + "GameList_64.png"), "游戏库", [this]() {
        if (onOpenGameLibrary) onOpenGameLibrary();
    });
    m_ButtonRow->addButton(BK_RES(path_prefix + "wenjianjia_64.png"), "beiklive/app/btn_file_list"_i18n, [this]() {
        if (onOpenFileList) onOpenFileList();
    });
    m_ButtonRow->addButton(BK_RES(path_prefix + "jifen_64.png"), "beiklive/app/btn_data_mgr"_i18n, [this]() {
        if (onOpenDataPage) onOpenDataPage();
    });
    m_ButtonRow->addButton(BK_RES(path_prefix + "shezhi_64.png"), "beiklive/app/btn_settings"_i18n, [this]() {
        if(onOpenSettings) onOpenSettings();
    });
    m_ButtonRow->addButton(BK_RES(path_prefix + "bangzhu_64.png"), "beiklive/app/btn_about"_i18n, [this]() {
        if (onOpenAboutPage) onOpenAboutPage();
    });
    m_ButtonRow->addButton(BK_RES(path_prefix + "tuichu_64.png"), "beiklive/app/btn_exit"_i18n, []() {
            auto dialog = new brls::Dialog("hints/exit_hint"_i18n);
            dialog->addButton("hints/cancel"_i18n, []() {});
            dialog->addButton("hints/ok"_i18n, []()
                { brls::Application::quit(); });
            dialog->open();
            return true;
    });

    addView(m_ButtonRow);

    // 填充
    m_bottomRow = new brls::BottomBar();
    // m_bottomRow->setGrow(1.0f);
    // addView(m_bottomRow);

}

void AppPage::addGame(const GameEntry& entry)
{
    auto* card = new GameCard(entry);
    card->onFocused   = [this](const GameEntry& e) { onCardFocused(e);   };
    card->onActivated = [this](const GameEntry& e) { onCardActivated(e); };
    card->onOptions   = [this](const GameEntry& e) { if (onGameOptions) onGameOptions(e); };
    m_cardRow->addView(card);
}

void AppPage::setGames(const std::vector<GameEntry>& games)
{
    m_cardRow->clearViews();
    for (const auto& g : games)
        addGame(g);
}

void AppPage::onCardFocused(const GameEntry& entry)
{
    // m_titleLabel->setText(entry.title.empty() ? "—" : entry.title);
    // m_pathLabel->setText(entry.path);
}

void AppPage::onCardActivated(const GameEntry& entry)
{
    if (onGameSelected)
        onGameSelected(entry);
}

void AppPage::removeGame(const std::string& gamePath)
{
    brls::View* toRemove = nullptr;
    for (auto* child : m_cardRow->getChildren()) {
        auto* card = dynamic_cast<GameCard*>(child);
        if (card && card->getEntry().path == gamePath) {
            toRemove = card;
            break;
        }
    }
    if (!toRemove)
        return;

    m_cardRow->removeView(toRemove, true);

    // 移除后将焦点恢复到有效视图
    const auto& remaining = m_cardRow->getChildren();
    if (!remaining.empty()) {
        auto* nextCard = remaining.front();
        auto* focus = nextCard->getDefaultFocus();
        if (!focus)
            focus = nextCard;
        brls::Application::giveFocus(focus);
    } else {
        auto* focus = getDefaultFocus();
        if (focus)
            brls::Application::giveFocus(focus);
    }
}

void AppPage::updateGameLogo(const std::string& gamePath, const std::string& newLogoPath)
{
    for (auto* child : m_cardRow->getChildren()) {
        auto* card = dynamic_cast<GameCard*>(child);
        if (card && card->getEntry().path == gamePath) {
            card->updateCover(newLogoPath);
            break;
        }
    }
}

void AppPage::updateGameTitle(const std::string& gamePath, const std::string& newTitle)
{
    for (auto* child : m_cardRow->getChildren()) {
        auto* card = dynamic_cast<GameCard*>(child);
        if (card && card->getEntry().path == gamePath) {
            card->updateTitle(newTitle);
            break;
        }
    }
}

void GameCard::updateCover(const std::string& newCoverPath)
{
    m_entry.cover = newCoverPath;
    if (!newCoverPath.empty())
        m_coverImage->setImageFromFile(newCoverPath);
    else
        m_coverImage->setImageFromFile(BK_APP_DEFAULT_LOGO);
}

void GameCard::updateTitle(const std::string& newTitle)
{
    m_entry.title = newTitle;
    m_titleLabel->setText(newTitle.empty() ? "—" : newTitle);
}
