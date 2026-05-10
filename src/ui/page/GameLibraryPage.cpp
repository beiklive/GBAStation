#include "GameLibraryPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include <algorithm>

namespace beiklive
{

    GameLibraryPage::GameLibraryPage()
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("游戏库");
        this->setFocusable(false);

        m_grid = new beiklive::GridBox(3);
        m_grid->setGrow(1.f);

        this->getContentBox()->addView(m_grid);

        m_grid->registerAction("排序", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _showSortDropdown();
            return true;
        });

        m_grid->registerAction("设置", brls::BUTTON_X, [this](brls::View*) -> bool {
            if (_currentFocusedIndex < 0 || _currentFocusedIndex >= static_cast<int>(m_entries.size()))
                return true;
            const GameEntry& entry = m_entries[_currentFocusedIndex];
            _showGameOptionsPanel(entry);
            return true;
        });

        _loadAndShowEntries();
    }

    // ============================================================
    // draw – 每帧检测触底，增量加载
    // ============================================================

    void GameLibraryPage::draw(NVGcontext* vg, float x, float y, float w, float h,
                                brls::Style style, brls::FrameContext* ctx)
    {
        beiklive::Box::draw(vg, x, y, w, h, style, ctx);

        if (m_loadingMore)
            return;

        if (m_visibleCount <= 0)
            return;

        if (static_cast<size_t>(m_visibleCount) >= m_entries.size())
            return;

        auto* sf = m_grid->getScrollFrame();
        float contentH = sf->getContentHeight();
        float areaH    = sf->getScrollingAreaHeight();
        if (contentH <= areaH)
            return;

        float offset      = sf->getContentOffsetY();
        float bottomLimit = contentH - areaH;

        if (offset >= bottomLimit - 50.f)
        {
            _loadNextPage();
        }
    }

    // ============================================================
    // _loadAndShowEntries – 首次加载前 PAGE_SIZE 条
    // ============================================================

    void GameLibraryPage::_loadAndShowEntries()
    {
        brls::Application::blockInputs(true);
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]() {
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _sortEntries();
            ASYNC_RELEASE
            brls::sync([this]() {
                m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                _rebuildGrid();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
                brls::Application::unblockInputs();
            });
        });
    }

    // ============================================================
    // _sortEntries
    // ============================================================

    void GameLibraryPage::_sortEntries()
    {
        switch (m_sortMode)
        {
            case SortMode::ByLastPlayed:
                std::sort(m_entries.begin(), m_entries.end(),
                    [](const GameEntry& a, const GameEntry& b) {
                        return a.lastPlayed > b.lastPlayed;
                    });
                break;
            case SortMode::ByPlayTime:
                std::sort(m_entries.begin(), m_entries.end(),
                    [](const GameEntry& a, const GameEntry& b) {
                        return a.playTime > b.playTime;
                    });
                break;
            case SortMode::ByName:
                std::sort(m_entries.begin(), m_entries.end(),
                    [](const GameEntry& a, const GameEntry& b) {
                        return a.title < b.title;
                    });
                break;
        }
    }

    // ============================================================
    // _rebuildGrid – 全量重建（仅在首次/排序变更/刷新时调用）
    // ============================================================

    void GameLibraryPage::_rebuildGrid()
    {
        m_grid->clearItems();

        int count = std::min(m_visibleCount, static_cast<int>(m_entries.size()));

        for (int i = 0; i < count; ++i)
        {
            const beiklive::GameEntry& entry = m_entries[i];
            int captIdx = i;

            m_grid->addItem([this, entry, captIdx]() -> brls::View* {
                auto* item = new beiklive::GridItem(GridItemMode::GAME_LIBRARY, captIdx);

                if (!entry.logoPath.empty())
                    item->setImagePath(entry.logoPath);

                std::string badgeText;
                PlatformBadgeColor badgeColor = _platformBadge(entry.platform);
                switch (static_cast<beiklive::enums::EmuPlatform>(entry.platform))
                {
                    case beiklive::enums::EmuPlatform::EmuGBA: badgeText = "GBA"; break;
                    case beiklive::enums::EmuPlatform::EmuGBC: badgeText = "GBC"; break;
                    case beiklive::enums::EmuPlatform::EmuGB:  badgeText = "GB";  break;
                    default:                                                        break;
                }
                if (!badgeText.empty())
                    item->setBadge(badgeText, badgeColor);

                std::string logoLayerPath = GetGameLogoLayerPath(entry.platform);
                if (!logoLayerPath.empty())
                    item->setImageLayer(logoLayerPath, true);

                item->setTitle(entry.title.empty() ? entry.path : entry.title);

                std::string lastPlayed = entry.lastPlayed.empty() ? "从未游玩" : beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
                item->setSubText(lastPlayed);

                item->setPlayTime(_formatPlayTime(entry.playTime));

                item->setDataLoaded();

                return item;
            });
        }

        m_grid->commit();

        m_grid->onItemClicked = [this](int index) {
            if (index < 0 || index >= static_cast<int>(m_entries.size())) return;
            if (onGameSelected)
                onGameSelected(m_entries[index]);
        };
        m_grid->onItemFocused = [this](int index) {
            _currentFocusedIndex = index;
            brls::Logger::debug("GameLibraryPage: 游戏聚焦，索引: {}, 游戏: {}", index, m_entries[index].title);
        };
    }

    // ============================================================
    // _loadNextPage – 增量追加，不重建已有视图
    // ============================================================

    void GameLibraryPage::_loadNextPage()
    {
        if (m_loadingMore)
            return;
        if (static_cast<size_t>(m_visibleCount) >= m_entries.size())
            return;

        m_loadingMore = true;

        int oldVisible = m_visibleCount;
        m_visibleCount = std::min(m_visibleCount + PAGE_SIZE, static_cast<int>(m_entries.size()));
        int newItems = m_visibleCount - oldVisible;

        for (int i = 0; i < newItems; ++i)
        {
            const beiklive::GameEntry& entry = m_entries[oldVisible + i];
            int captIdx = oldVisible + i;

            m_grid->addItem([this, entry, captIdx]() -> brls::View* {
                auto* item = new beiklive::GridItem(GridItemMode::GAME_LIBRARY, captIdx);

                if (!entry.logoPath.empty())
                    item->setImagePath(entry.logoPath);

                std::string badgeText;
                PlatformBadgeColor badgeColor = _platformBadge(entry.platform);
                switch (static_cast<beiklive::enums::EmuPlatform>(entry.platform))
                {
                    case beiklive::enums::EmuPlatform::EmuGBA: badgeText = "GBA"; break;
                    case beiklive::enums::EmuPlatform::EmuGBC: badgeText = "GBC"; break;
                    case beiklive::enums::EmuPlatform::EmuGB:  badgeText = "GB";  break;
                    default:                                                        break;
                }
                if (!badgeText.empty())
                    item->setBadge(badgeText, badgeColor);

                std::string logoLayerPath = GetGameLogoLayerPath(entry.platform);
                if (!logoLayerPath.empty())
                    item->setImageLayer(logoLayerPath, true);

                item->setTitle(entry.title.empty() ? entry.path : entry.title);

                std::string lastPlayed = entry.lastPlayed.empty() ? "从未游玩" : beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
                item->setSubText(lastPlayed);

                item->setPlayTime(_formatPlayTime(entry.playTime));

                item->setDataLoaded();

                return item;
            });
        }

        // 增量追加：不触动已有视图，只添加新行
        m_grid->commitAppend();

        m_loadingMore = false;
    }

    // ============================================================
    // _showSortDropdown
    // ============================================================

    void GameLibraryPage::_showSortDropdown()
    {
        std::vector<std::string> options = {
            "按最近游玩排序",
            "按游戏时长排序",
            "按游戏名称排序",
        };

        int current = static_cast<int>(m_sortMode);

        auto* dropdown = new brls::Dropdown(
            "排序方式",
            options,
            [](int) {},
            current,
            [this](int selected) {
                if (selected < 0) return;
                SortMode newMode = static_cast<SortMode>(selected);
                if (newMode == m_sortMode) return;
                m_sortMode = newMode;

                ASYNC_RETAIN
                brls::async([ASYNC_TOKEN]() {
                    _sortEntries();
                    ASYNC_RELEASE
                    brls::sync([this]() {
                        m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                        _rebuildGrid();
                        _updateHeader();
                        brls::Application::giveFocus(m_grid);
                    });
                });
            });

        brls::Application::pushActivity(new brls::Activity(dropdown));
    }

    // ============================================================
    // _updateHeader
    // ============================================================

    void GameLibraryPage::_updateHeader()
    {
        this->getHeader()->setInfo("共 " + std::to_string(m_entries.size()) + " 款游戏");
        std::string sortStr;
        switch (m_sortMode)
        {
            case SortMode::ByLastPlayed: sortStr = "最近游玩"; break;
            case SortMode::ByPlayTime:   sortStr = "游玩时长"; break;
            case SortMode::ByName:       sortStr = "名称";     break;
        }
        this->getHeader()->setPath("排序：" + sortStr);
    }

    // ============================================================
    // _platformBadge
    // ============================================================

    PlatformBadgeColor GameLibraryPage::_platformBadge(int platform)
    {
        switch (static_cast<beiklive::enums::EmuPlatform>(platform))
        {
            case beiklive::enums::EmuPlatform::EmuGBA: return PlatformBadgeColor::GBA;
            case beiklive::enums::EmuPlatform::EmuGBC: return PlatformBadgeColor::GBC;
            case beiklive::enums::EmuPlatform::EmuGB:  return PlatformBadgeColor::GB;
            default:                                    return PlatformBadgeColor::NONE;
        }
    }

    // ============================================================
    // _formatPlayTime
    // ============================================================

    std::string GameLibraryPage::_formatPlayTime(int seconds)
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
    // _reloadEntries – 操作后刷新（重置为第一页）
    // ============================================================

    void GameLibraryPage::_reloadEntries()
    {
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]() {
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _sortEntries();
            ASYNC_RELEASE
            brls::sync([this]() {
                m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                _rebuildGrid();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
            });
        });
    }

    // ============================================================
    // _showGameOptionsPanel
    // ============================================================

    void GameLibraryPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        int crc = entry.crc32;

        _hideGameOptionsPanel();

        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);
        std::string filename = beiklive::tools::getFileNameWithoutExtension(entry.path);

        m_gameOptionsSidebar->addButton("修改映射名称", BK_RES("img/ui/setting/emu.png"),
            [this, crc, title = entry.title, filename](const beiklive::GameEntry& e) {
                _hideGameOptionsPanel();
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) return;
                ime->openForText(
                    [this, crc, filename](std::string text) {
                        if (!text.empty() && beiklive::GameDB) {
                            beiklive::GameDB->set(crc, "title", nlohmann::json(text));
                            _reloadEntries();
                            beiklive::NameMappingManager->Set(filename, text, true);
                            beiklive::GameDB->flush();
                            beiklive::NameMappingManager->Save();
                        }
                    },
                    "编辑游戏名称",
                    "",
                    128,
                    title,
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            });

        m_gameOptionsSidebar->addButton("设置封面图", BK_RES("img/ui/setting/display.png"),
            [this, crc](const beiklive::GameEntry& e) {
                _hideGameOptionsPanel();
                std::string startDir;
                beiklive::openFilePicker({"png", "jpg"},
                    [this, crc](const std::string& selectedPath) {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(crc, "logoPath", nlohmann::json(selectedPath));
                            _reloadEntries();
                            beiklive::GameDB->flush();
                        }
                    },
                    beiklive::path::GetRootPath());
            });

        m_gameOptionsSidebar->addButton("删除游戏", BK_RES("img/ui/menu/exit.png"),
            [this, crc](const beiklive::GameEntry& e) {
                _hideGameOptionsPanel();
                auto* dialog = new brls::Dialog("确定要删除该游戏吗？\n此操作将清除游戏记录与存档数据。");
                dialog->addButton("确认删除", [this, crc]() {
                    _hideGameOptionsPanel();
                    if (beiklive::GameDB && beiklive::GameDB->removeByCrc32(crc)) {
                        brls::Application::notify("已删除游戏");
                        _reloadEntries();
                        beiklive::GameDB->flush();
                    } else {
                        brls::Application::notify("删除失败");
                    }
                });
                dialog->addButton("取消", [this]() {
                });
                dialog->open();
            });

        m_gameOptionsSidebar->onClosed = [this]() {
            brls::Application::giveFocus(m_grid);
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        };

        this->addView(m_gameOptionsSidebar);
        m_gameOptionsSidebar->open(entry);
    }

    void GameLibraryPage::_hideGameOptionsPanel()
    {
        if (m_gameOptionsSidebar)
        {
            m_gameOptionsSidebar->close();
            m_gameOptionsSidebar->removeFromSuperView(true);
            m_gameOptionsSidebar = nullptr;
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        }
    }

} // namespace beiklive
