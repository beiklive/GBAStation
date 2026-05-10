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

        m_grid->registerAction("分类", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _showFilterDropdown();
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
            _filterEntries();
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
    // _filterEntries – 按平台过滤 m_entries
    // ============================================================

    void GameLibraryPage::_filterEntries()
    {
        if (m_platformFilter == PlatformFilter::ALL)
        {
            std::sort(m_entries.begin(), m_entries.end(),
                [](const GameEntry& a, const GameEntry& b) {
                    return a.lastPlayed > b.lastPlayed;
                });
            return;
        }

        int targetPlatform = static_cast<int>(m_platformFilter);

        m_entries.erase(
            std::remove_if(m_entries.begin(), m_entries.end(),
                [targetPlatform](const GameEntry& e) {
                    return e.platform != targetPlatform;
                }),
            m_entries.end());

        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameEntry& a, const GameEntry& b) {
                return a.lastPlayed > b.lastPlayed;
            });
    }

    // ============================================================
    // _rebuildGrid – 全量重建（仅在首次/排序变更/刷新时调用）
    // ============================================================

    void GameLibraryPage::_rebuildGrid()
    {
        beiklive::GridItem::cancelDeferredLoads();
        m_grid->clearItems();

        int count = std::min(m_visibleCount, static_cast<int>(m_entries.size()));

        for (int i = 0; i < count; ++i)
        {
            const beiklive::GameEntry& entry = m_entries[i];
            int captIdx = i;

            m_grid->addItem([this, entry, captIdx]() -> brls::View* {
                auto* item = new beiklive::GridItem(GridItemMode::GAME_LIBRARY, captIdx);

                item->setImagePathDeferred(entry.logoPath);

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
                item->setImageLayerDeferred(logoLayerPath, !logoLayerPath.empty());

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

                item->setImagePathDeferred(entry.logoPath);

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
                item->setImageLayerDeferred(logoLayerPath, !logoLayerPath.empty());

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
    // _showFilterDropdown – Y 键弹出平台分类选择
    // ============================================================

    void GameLibraryPage::_showFilterDropdown()
    {
        auto allEntries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
        bool hasGBA = false, hasGBC = false, hasGB = false;
        for (const auto& e : allEntries)
        {
            switch (static_cast<beiklive::enums::EmuPlatform>(e.platform))
            {
                case beiklive::enums::EmuPlatform::EmuGBA: hasGBA = true; break;
                case beiklive::enums::EmuPlatform::EmuGBC: hasGBC = true; break;
                case beiklive::enums::EmuPlatform::EmuGB:  hasGB  = true; break;
                default: break;
            }
        }

        std::vector<std::string> options;
        std::vector<PlatformFilter> filterMapping;

        options.push_back("所有");
        filterMapping.push_back(PlatformFilter::ALL);

        if (hasGBA) { options.push_back("GBA"); filterMapping.push_back(PlatformFilter::GBA); }
        if (hasGBC) { options.push_back("GBC"); filterMapping.push_back(PlatformFilter::GBC); }
        if (hasGB)  { options.push_back("GB");  filterMapping.push_back(PlatformFilter::GB);  }

        int current = 0;
        for (size_t i = 0; i < filterMapping.size(); ++i)
        {
            if (filterMapping[i] == m_platformFilter)
            {
                current = static_cast<int>(i);
                break;
            }
        }

        auto* dropdown = new brls::Dropdown(
            "游戏分类",
            options,
            [](int) {},
            current,
            [this, filterMapping](int selected) {
                if (selected < 0 || selected >= static_cast<int>(filterMapping.size())) return;
                PlatformFilter newFilter = filterMapping[selected];
                if (newFilter == m_platformFilter) return;
                m_platformFilter = newFilter;

                ASYNC_RETAIN
                brls::async([ASYNC_TOKEN]() {
                    m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                    _filterEntries();
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
        std::string filterStr;
        switch (m_platformFilter)
        {
            case PlatformFilter::ALL: filterStr = "所有"; break;
            case PlatformFilter::GBA: filterStr = "GBA";  break;
            case PlatformFilter::GBC: filterStr = "GBC";  break;
            case PlatformFilter::GB:  filterStr = "GB";   break;
        }
        this->getHeader()->setPath("分类：" + filterStr);
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
            _filterEntries();
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
