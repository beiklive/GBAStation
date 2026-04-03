#include "DataManagementPage.hpp"
#include "GameDetailPage.hpp"

namespace beiklive
{
    DataManagementPage::DataManagementPage()
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("数据管理");
        this->setFocusable(false);
        _initLayout();
    }

    // ============================================================
    // _initLayout – 建立左右分栏布局
    // ============================================================

    void DataManagementPage::_initLayout()
    {
        // 主容器：横向排列
        auto* mainRow = new brls::Box(brls::Axis::ROW);
        mainRow->setGrow(1.f);
        mainRow->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(mainRow);

        // ── 左侧平台按钮栏 ──────────────────────────────────────────────
        m_leftPanel = new brls::Box(brls::Axis::COLUMN);
        m_leftPanel->setWidthPercentage(25.f);
        m_leftPanel->setHeightPercentage(100.f);
        m_leftPanel->setAlignItems(brls::AlignItems::FLEX_END);
        m_leftPanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_leftPanel->setPadding(24.f);
        m_leftPanel->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
        m_leftPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_leftPanel);

        // ── 右侧游戏列表区域 ─────────────────────────────────────────────
        m_rightPanel = new brls::Box(brls::Axis::COLUMN);
        m_rightPanel->setGrow(1.f);
        m_rightPanel->setHeightPercentage(100.f);
        m_rightPanel->setAlignItems(brls::AlignItems::FLEX_START);
        m_rightPanel->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_rightPanel->setPadding(16.f);
        m_rightPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_rightPanel);

        // 游戏列表面板（GridBox）
        m_gameListPanel = new brls::Box(brls::Axis::COLUMN);
        m_gameListPanel->setGrow(1.f);
        m_gameListPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_gameListPanel);

        m_grid = new beiklive::GridBox(1);
        m_grid->setGrow(1.f);

        m_gameListPanel->addView(m_grid);
        m_rightPanel->addView(m_gameListPanel);

        // ── 创建三个平台按钮 ─────────────────────────────────────────────
        m_btnGBA = _createPlatformButton("GBA", beiklive::enums::EmuPlatform::EmuGBA);
        m_btnGBC = _createPlatformButton("GBC", beiklive::enums::EmuPlatform::EmuGBC);
        m_btnGB  = _createPlatformButton("GB",  beiklive::enums::EmuPlatform::EmuGB);

        m_leftPanel->addView(m_btnGBA);
        m_leftPanel->addView(m_btnGBC);
        m_leftPanel->addView(m_btnGB);

        mainRow->addView(m_leftPanel);
        mainRow->addView(m_rightPanel);

        this->getContentBox()->addView(mainRow);

        // 默认显示 GBA 平台
        _loadPlatformGames(beiklive::enums::EmuPlatform::EmuGBA);
        brls::sync([this]() {
            brls::Application::giveFocus(m_btnGBA);
        });
    }

    // ============================================================
    // _createPlatformButton – 创建平台选择按钮
    // ============================================================

    beiklive::ButtonBox* DataManagementPage::_createPlatformButton(
        const std::string& text,
        beiklive::enums::EmuPlatform platform)
    {
        auto* btn = new beiklive::ButtonBox();
        btn->setAxis(brls::Axis::ROW);
        btn->setJustifyContent(brls::JustifyContent::CENTER);
        btn->setAlignItems(brls::AlignItems::CENTER);
        btn->setWidthPercentage(100.f);
        btn->setHeight(52.f);
        btn->setMarginBottom(12.f);
        btn->setFocusable(true);
        btn->setHideHighlightBackground(true);
        btn->setHideHighlightBorder(true);
        btn->setHideClickAnimation(false);

        auto* lbl = new brls::Label();
        lbl->setText(text);
        lbl->setFontSize(20.f);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        lbl->setFocusable(false);
        btn->addView(lbl);

        // 焦点获得：高亮按钮并切换右侧游戏列表
        btn->onFocusGainedCallback = [this, btn, platform]() {
            btn->setBackgroundColor(nvgRGBA(255, 255, 255, 30));
            _loadPlatformGames(platform);
        };
        btn->onFocusLostCallback = [btn]() {
            btn->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        };

        // 点击：将焦点转入右侧游戏列表
        btn->registerClickAction([this](brls::View*) -> bool {
            brls::Application::giveFocus(m_grid);
            return true;
        });

        return btn;
    }

    // ============================================================
    // _loadPlatformGames – 加载指定平台游戏并刷新 GridBox
    // ============================================================

    void DataManagementPage::_loadPlatformGames(beiklive::enums::EmuPlatform platform)
    {
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, platform]() {
            std::vector<GameEntry> entries;
            if (beiklive::GameDB)
                entries = beiklive::GameDB->getByPlatform(platform);

            ASYNC_RELEASE
            brls::sync([this, entries = std::move(entries), platform]() {
                m_currentEntries = entries;
                m_grid->clearItems();

                for (int i = 0; i < static_cast<int>(m_currentEntries.size()); ++i)
                {
                    const GameEntry& entry = m_currentEntries[i];
                    int captIdx = i;

                    m_grid->addItem([this, entry, captIdx, platform]() -> brls::View* {
                        auto* item = new beiklive::GridItem(GridItemMode::GAME_LIBRARY, captIdx);

                        // 设置封面图片
                        if (!entry.logoPath.empty())
                            item->setImagePath(entry.logoPath);

                        // 设置平台徽标
                        std::string badgeText;
                        switch (platform)
                        {
                            case beiklive::enums::EmuPlatform::EmuGBA: badgeText = "GBA"; break;
                            case beiklive::enums::EmuPlatform::EmuGBC: badgeText = "GBC"; break;
                            case beiklive::enums::EmuPlatform::EmuGB:  badgeText = "GB";  break;
                            default: break;
                        }
                        if (!badgeText.empty())
                            item->setBadge(badgeText, _platformBadge(platform));

                        // 设置游戏名称
                        item->setTitle(entry.title.empty() ? entry.path : entry.title);

                        // 设置上次游玩时间
                        item->setSubText(entry.lastPlayed.empty() ? "从未游玩" : entry.lastPlayed);

                        // 设置游玩时长
                        item->setPlayTime(_formatPlayTime(entry.playTime));

                        item->setDataLoaded();

                        return item;
                    });
                }

                // 点击条目：打开游戏详情页
                m_grid->onItemClicked = [this](int index) {
                    if (index < 0 || index >= static_cast<int>(m_currentEntries.size())) return;
                    const GameEntry& entry = m_currentEntries[index];
                    auto* detailPage = new beiklive::GameDetailPage(entry);
                    auto* frame      = new brls::AppletFrame(detailPage);
                    HIDE_BRLS_BAR(frame);
                    brls::sync([frame]() {
                        brls::Application::pushActivity(new brls::Activity(frame));
                    });
                };
            });
        });
    }

    // ============================================================
    // _formatPlayTime
    // ============================================================

    std::string DataManagementPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "";
        int hours   = seconds / 3600;
        int minutes = (seconds % 3600) / 60;
        char buf[64];
        if (hours > 0)
            std::snprintf(buf, sizeof(buf), "%d 小时 %d 分钟", hours, minutes);
        else if (minutes > 0)
            std::snprintf(buf, sizeof(buf), "%d 分钟", minutes);
        else
            std::snprintf(buf, sizeof(buf), "不到 1 分钟");
        return std::string(buf);
    }

    // ============================================================
    // _platformBadge
    // ============================================================

    PlatformBadgeColor DataManagementPage::_platformBadge(beiklive::enums::EmuPlatform platform)
    {
        switch (platform)
        {
            case beiklive::enums::EmuPlatform::EmuGBA: return PlatformBadgeColor::GBA;
            case beiklive::enums::EmuPlatform::EmuGBC: return PlatformBadgeColor::GBC;
            case beiklive::enums::EmuPlatform::EmuGB:  return PlatformBadgeColor::GB;
            default:                                    return PlatformBadgeColor::NONE;
        }
    }

} // namespace beiklive
