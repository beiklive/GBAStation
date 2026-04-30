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
        // 主视图：3 列 GridBox
        m_grid = new beiklive::GridBox(3);
        m_grid->setGrow(1.f);
        // m_grid->setFocusable(false); 

        this->getContentBox()->addView(m_grid);

        // Y 键：弹出排序方式 Dropdown
        m_grid->registerAction("排序", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _showSortDropdown();
            return true;
        });

        m_grid->registerAction("设置", brls::BUTTON_X, [this](brls::View*) -> bool {
            if (_currentFocusedIndex < 0 || _currentFocusedIndex >= static_cast<int>(m_entries.size()))
                return true; // 无效索引，忽略
            const GameEntry& entry = m_entries[_currentFocusedIndex];
            _showGameOptionsPanel(entry);

            return true;
        });


        // 加载并显示游戏数据
        _loadAndShowEntries();
    }

    // ============================================================
    // _loadAndShowEntries – 从数据库加载数据，排序后构建 Grid
    // ============================================================

    void GameLibraryPage::_loadAndShowEntries()
    {
        brls::Application::blockInputs(true);
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]() {
            // 从全局游戏数据库获取所有游戏条目
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _sortEntries();
            ASYNC_RELEASE
            brls::sync([this]() {
                _rebuildGrid();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
                brls::Application::unblockInputs();
            });
        });
    }

    // ============================================================
    // _sortEntries – 按当前排序方式对 m_entries 排序
    // ============================================================

    void GameLibraryPage::_sortEntries()
    {
        switch (m_sortMode)
        {
            case SortMode::ByLastPlayed:
                // 按最近游玩时间降序（字符串比较，格式 "YYYY-MM-DD HH:MM:SS"）
                std::sort(m_entries.begin(), m_entries.end(),
                    [](const GameEntry& a, const GameEntry& b) {
                        return a.lastPlayed > b.lastPlayed;
                    });
                break;
            case SortMode::ByPlayTime:
                // 按总游玩时长降序
                std::sort(m_entries.begin(), m_entries.end(),
                    [](const GameEntry& a, const GameEntry& b) {
                        return a.playTime > b.playTime;
                    });
                break;
            case SortMode::ByName:
                // 按游戏名称升序
                std::sort(m_entries.begin(), m_entries.end(),
                    [](const GameEntry& a, const GameEntry& b) {
                        return a.title < b.title;
                    });
                break;
        }
    }

    // ============================================================
    // _rebuildGrid – 清空并重新创建 GridItem
    // ============================================================

    void GameLibraryPage::_rebuildGrid()
    {
        m_grid->clearItems();

        for (int i = 0; i < static_cast<int>(m_entries.size()); ++i)
        {
            const beiklive::GameEntry& entry = m_entries[i];
            int captIdx = i;

            m_grid->addItem([this, entry, captIdx]() -> brls::View* {
                auto* item = new beiklive::GridItem(GridItemMode::GAME_LIBRARY, captIdx);

                // 设置封面图片
                if (!entry.logoPath.empty())
                    item->setImagePath(entry.logoPath);

                // 设置平台徽标
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
                // 设置封面图层（如果有的话）
                std::string logoLayerPath = GetGameLogoLayerPath(entry.platform);
                if (!logoLayerPath.empty())
                    item->setImageLayer(logoLayerPath, true);

                // 设置游戏名称
                item->setTitle(entry.title.empty() ? entry.path : entry.title);

                // 设置上次游玩时间
                std::string lastPlayed = entry.lastPlayed.empty() ? "从未游玩" : beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
                item->setSubText(lastPlayed);

                // 设置游玩时长
                item->setPlayTime(_formatPlayTime(entry.playTime));

                // 切换到有数据状态
                item->setDataLoaded();


                // // X 键：选项面板
                // item->registerAction("选项", brls::BUTTON_LB, [this, entry](brls::View*) -> bool {
                //     brls::Logger::debug("GameLibraryPage: X 键触发，游戏: {}", entry.title);
                //     _showGameOptionsPanel(entry);
                //     return true;
                // });

                return item;
            });
        }

        // GridBox 的 onItemClicked 触发游戏启动
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
    // _showSortDropdown – 弹出排序方式 Dropdown
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
            [](int) {}, // 选中回调（通过 dismissCb 处理）
            current,
            [this](int selected) {
                if (selected < 0) return;
                SortMode newMode = static_cast<SortMode>(selected);
                if (newMode == m_sortMode) return;
                m_sortMode = newMode;

                ASYNC_RETAIN
                brls::async([ASYNC_TOKEN]() {
                    ASYNC_RELEASE
                    brls::sync([this]() {
                        _sortEntries();
                        _rebuildGrid();
                        _updateHeader();
                        brls::Application::giveFocus(m_grid);
                    });
                });
            });

        brls::Application::pushActivity(new brls::Activity(dropdown));
    }

    // ============================================================
    // _updateHeader – 更新标题栏游戏数量信息
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
    // _platformBadge – 平台 → 徽标颜色
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
    // _formatPlayTime – 游玩时长格式化
    // ============================================================

    std::string GameLibraryPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return ""; // setPlayTime 会处理为"未游玩"

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
    // _reloadEntries – 重新加载数据库并重建网格（操作后刷新用）
    // ============================================================

    void GameLibraryPage::_reloadEntries()
    {
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]() {
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _sortEntries();
            ASYNC_RELEASE
            brls::sync([this]() {
                _rebuildGrid();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
            });
        });
    }

    // ============================================================
    // _showGameOptionsPanel – X 键游戏选项侧边栏
    // ============================================================

    void GameLibraryPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        int crc = entry.crc32;

        _hideGameOptionsPanel();

        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);

        // ── 修改映射名称 ──
        m_gameOptionsSidebar->addButton("修改映射名称", BK_RES("img/ui/setting/emu.png"),
            [this, crc, title = entry.title](const beiklive::GameEntry& e) {
        _hideGameOptionsPanel();
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) return;
                ime->openForText(
                    [this, crc](std::string text) {
                        if (!text.empty() && beiklive::GameDB) {
                            beiklive::GameDB->set(crc, "title", nlohmann::json(text));
                            _reloadEntries();
                        }
                    },
                    "编辑游戏名称",
                    "",
                    128,
                    title,
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            });

        // ── 设置封面图 ──
        m_gameOptionsSidebar->addButton("设置封面图", BK_RES("img/ui/setting/display.png"),
            [this, crc](const beiklive::GameEntry& e) {
        _hideGameOptionsPanel();
                std::string startDir;
                if (!e.logoPath.empty())
                    startDir = beiklive::tools::getParentPath(e.logoPath);
                beiklive::openFilePicker({"png", "jpg"},
                    [this, crc](const std::string& selectedPath) {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(crc, "logoPath", nlohmann::json(selectedPath));
                            _reloadEntries();
                        }
                    },
                    startDir);
            });

        // ── 删除游戏 ──
        m_gameOptionsSidebar->addButton("删除游戏", BK_RES("img/ui/menu/exit.png"),
            [this, crc](const beiklive::GameEntry& e) {
        _hideGameOptionsPanel();
                auto* dialog = new brls::Dialog("确定要删除该游戏吗？\n此操作将清除游戏记录与存档数据。");
                dialog->addButton("确认删除", [this, crc]() {
                    _hideGameOptionsPanel();
                    if (beiklive::GameDB && beiklive::GameDB->removeByCrc32(crc)) {
                        brls::Application::notify("已删除游戏");
                        _reloadEntries();
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
