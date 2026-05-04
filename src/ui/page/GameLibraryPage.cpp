#include "GameLibraryPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include <algorithm>

namespace beiklive
{
    static constexpr float SIDEBAR_WIDTH = 240.f;

    GameLibraryPage::GameLibraryPage()
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("游戏库");
        this->setFocusable(false);

        // 主布局：ROW（左侧侧边栏 + 右侧网格）
        auto mainBox = new brls::Box(brls::Axis::ROW);
        mainBox->setGrow(1.f);
        mainBox->setWidthPercentage(100.f);

        // 左侧侧边栏
        m_sidebar = new brls::Box(brls::Axis::COLUMN);
        m_sidebar->setWidth(SIDEBAR_WIDTH);
        m_sidebar->setGrow(0.f);
        m_sidebar->setMarginTop(10.f);
        m_sidebar->setMarginLeft(10.f);
        m_sidebar->setMarginRight(10.f);
        m_sidebar->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_sidebar);

        // 分类按钮创建
        auto makeCell = [this](const std::string& text, FilterType type) -> beiklive::DetailCell* {
            auto cell = new beiklive::DetailCell();
            cell->setLeftText(text);
            cell->setHeight(50.f);
            auto arrow = cell->addRightImage(BK_RES("img/ui/menu/right.png"));
            if (arrow) {
                arrow->setWidth(20.f);
                arrow->setHeight(20.f);
            }
            cell->registerAction(
                "选择",
                brls::BUTTON_A,
                [this, type](brls::View*) -> bool {
                    if (m_filterType != type) {
                        m_filterType = type;
                        ASYNC_RETAIN
                        brls::async([ASYNC_TOKEN]() {
                            _applyFilter();
                            ASYNC_RELEASE
                            brls::sync([this]() {
                                _rebuildGrid();
                                _updateSidebar();
                                brls::Application::giveFocus(m_grid);
                            });
                        });
                    }
                    return true;
                });
            return cell;
        };

        m_allCell = makeCell("全部游戏", FilterType::All);
        m_favCell = makeCell("收藏游戏", FilterType::Favourite);
        m_gbaCell = makeCell("GBA", FilterType::GBA);
        m_gbcCell = makeCell("GBC", FilterType::GBC);
        m_gbCell  = makeCell("GB", FilterType::GB);

        m_sidebar->addView(m_allCell);
        m_sidebar->addView(m_favCell);
        m_sidebar->addView(m_gbaCell);
        m_sidebar->addView(m_gbcCell);
        m_sidebar->addView(m_gbCell);

        mainBox->addView(m_sidebar);

        // 右侧：2 列 GridBox
        m_grid = new beiklive::GridBox(2);
        m_grid->setGrow(1.f);

        auto gridWrapper = new brls::Box(brls::Axis::COLUMN);
        gridWrapper->setGrow(1.f);
        // gridWrapper->setWidthPercentage(100.f);
        gridWrapper->setMarginTop(10.f);
        gridWrapper->addView(m_grid);

        mainBox->addView(gridWrapper);

        this->getContentBox()->addView(mainBox);

        // Y 键：弹出排序方式 Dropdown
        m_grid->registerAction("排序", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _showSortDropdown();
            return true;
        });

        m_grid->registerAction("设置", brls::BUTTON_X, [this](brls::View*) -> bool {
            if (_currentFocusedIndex < 0 || _currentFocusedIndex >= static_cast<int>(m_filteredEntries.size()))
                return true;
            _showGameOptionsPanel(m_filteredEntries[_currentFocusedIndex]);
            return true;
        });

        // 加载并显示游戏数据
        _loadAndShowEntries();
    }

    void GameLibraryPage::_loadAndShowEntries()
    {
        brls::Application::blockInputs(true);
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]() {
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            m_filterType = FilterType::All;
            _sortEntries();
            _applyFilter();
            ASYNC_RELEASE
            brls::sync([this]() {
                _rebuildGrid();
                _updateSidebar();
                brls::Application::giveFocus(m_grid);
                brls::Application::unblockInputs();
            });
        });
    }

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

    void GameLibraryPage::_applyFilter()
    {
        m_filteredEntries.clear();
        switch (m_filterType)
        {
            case FilterType::All:
                m_filteredEntries = m_entries;
                break;
            case FilterType::Favourite:
                for (auto& e : m_entries)
                    if (e.favourite) m_filteredEntries.push_back(e);
                break;
            case FilterType::GBA:
                for (auto& e : m_entries)
                    if (e.platform == (int)beiklive::enums::EmuPlatform::EmuGBA)
                        m_filteredEntries.push_back(e);
                break;
            case FilterType::GBC:
                for (auto& e : m_entries)
                    if (e.platform == (int)beiklive::enums::EmuPlatform::EmuGBC)
                        m_filteredEntries.push_back(e);
                break;
            case FilterType::GB:
                for (auto& e : m_entries)
                    if (e.platform == (int)beiklive::enums::EmuPlatform::EmuGB)
                        m_filteredEntries.push_back(e);
                break;
        }
    }

    void GameLibraryPage::_updateSidebar()
    {
        int totalCount  = static_cast<int>(m_entries.size());
        int favCount    = 0, gbaCount = 0, gbcCount = 0, gbCount = 0;
        for (auto& e : m_entries) {
            if (e.favourite) favCount++;
            if (e.platform == (int)beiklive::enums::EmuPlatform::EmuGBA) gbaCount++;
            if (e.platform == (int)beiklive::enums::EmuPlatform::EmuGBC) gbcCount++;
            if (e.platform == (int)beiklive::enums::EmuPlatform::EmuGB)  gbCount++;
        }

        auto updateCell = [](beiklive::DetailCell* cell, const std::string& label, int count) {
            if (cell) {
                cell->setLeftText(label + "  " + std::to_string(count));
            }
        };

        updateCell(m_allCell, "全部游戏", totalCount);
        updateCell(m_favCell, "收藏游戏", favCount);
        updateCell(m_gbaCell, "GBA", gbaCount);
        updateCell(m_gbcCell, "GBC", gbcCount);
        updateCell(m_gbCell,  "GB",  gbCount);
    }

    void GameLibraryPage::_rebuildGrid()
    {
        m_grid->clearItems();

        for (int i = 0; i < static_cast<int>(m_filteredEntries.size()); ++i)
        {
            const beiklive::GameEntry& entry = m_filteredEntries[i];
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
                    default: break;
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

                // 收藏回调
                int crc = entry.crc32;
                item->isFavourite = [crc](int) -> bool {
                    if (!beiklive::GameDB) return false;
                    auto j = beiklive::GameDB->get(crc, "favourite", nlohmann::json(false));
                    return j.is_boolean() ? j.get<bool>() : false;
                };
                item->toggleFavourite = [this, crc](int) {
                    if (!beiklive::GameDB) return;
                    auto j = beiklive::GameDB->get(crc, "favourite", nlohmann::json(false));
                    bool cur = j.is_boolean() ? j.get<bool>() : false;
                    beiklive::GameDB->set(crc, "favourite", nlohmann::json(!cur));
                    beiklive::GameDB->flush();
                    // 刷新内存中的 entry
                    for (auto& e : m_entries)
                        if (e.crc32 == crc) { e.favourite = !cur; break; }
                    for (auto& e : m_filteredEntries)
                        if (e.crc32 == crc) { e.favourite = !cur; break; }
                    _updateSidebar();
                };

                return item;
            });
        }

        m_grid->onItemClicked = [this](int index) {
            if (index < 0 || index >= static_cast<int>(m_filteredEntries.size())) return;
            if (onGameSelected)
                onGameSelected(m_filteredEntries[index]);
        };
        m_grid->onItemFocused = [this](int index) {
            _currentFocusedIndex = index;
        };
    }

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
                    _applyFilter();
                    ASYNC_RELEASE
                    brls::sync([this]() {
                        _rebuildGrid();
                        brls::Application::giveFocus(m_grid);
                    });
                });
            });

        brls::Application::pushActivity(new brls::Activity(dropdown));
    }

    void GameLibraryPage::_reloadEntries()
    {
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]() {
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _sortEntries();
            _applyFilter();
            ASYNC_RELEASE
            brls::sync([this]() {
                _rebuildGrid();
                _updateSidebar();
                brls::Application::giveFocus(m_grid);
            });
        });
    }

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
                dialog->addButton("取消", [this]() {});
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

} // namespace beiklive
