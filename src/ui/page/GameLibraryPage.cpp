#include "GameLibraryPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "core/ThreadPool.hpp"
#include <algorithm>
#include <cctype>

namespace beiklive
{

    GameLibraryPage::GameLibraryPage()
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("游戏库");
        this->setFocusable(false);

        m_grid = new beiklive::RecyclingGrid(3, GridItem::ITEM_HEIGHT, 10.0f);
        m_grid->setGrow(1.f);
        m_grid->registerCell("GridItem", []() -> RecyclingGridItem* {
            return new GridItem(GridItemMode::GAME_LIBRARY, 0);
        });

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

        m_grid->registerAction("搜索", brls::BUTTON_RT, [this](brls::View*) -> bool {
            auto* ime = brls::Application::getPlatform()->getImeManager();
            if (!ime) return true;
            ime->openForText(
                [this](std::string text) {
                    m_isSearching = !text.empty();
                    m_searchTerm = text;

                    auto* alive = &m_alive;
                    ThreadPool::instance().enqueue([this, alive]() {
                        if (!alive->load()) return;
                        m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                        _filterEntries();
                        brls::sync([this, alive]() {
                            if (!alive->load()) return;
                            if (m_isSearching && m_entries.empty())
                            {
                                auto* dialog = new brls::Dialog("当前分类下无 \"" + m_searchTerm + "\"");
                                dialog->addButton("确认", []() {});
                                dialog->open();
                                return;
                            }

                            m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                            if (m_dataSource)
                                m_grid->reloadData();
                            _updateHeader();
                            brls::Application::giveFocus(m_grid);
                        });
                    });
                },
                "搜索游戏",
                "",
                128,
                m_searchTerm,
                brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            return true;
        });

        m_grid->onNextPage = [this]() {
            if (!m_loadingMore)
                _loadNextPage();
        };

        _loadAndShowEntries();
    }

    GameLibraryPage::~GameLibraryPage()
    {
        m_alive.store(false);
    }

    void GameLibraryPage::draw(NVGcontext* vg, float x, float y, float w, float h,
                                brls::Style style, brls::FrameContext* ctx)
    {
        beiklive::Box::draw(vg, x, y, w, h, style, ctx);
    }

    // ============================================================
    // GameLibraryDS
    // ============================================================

    size_t GameLibraryPage::GameLibraryDS::getItemCount() const
    {
        return m_page ? m_page->m_visibleCount : 0;
    }

    RecyclingGridItem* GameLibraryPage::GameLibraryDS::cellForRow(RecyclingGrid* grid, size_t index)
    {
        auto* cell = static_cast<GridItem*>(grid->dequeueReusableCell("GridItem"));
        if (!cell) return nullptr;

        if (!m_page || index >= m_page->m_entries.size())
        {
            cell->setEmpty("空");
            return cell;
        }

        const auto& entry = m_page->m_entries[index];
        GridItemData data = _buildItemData(entry);

        cell->setImagePathDeferred(data.logoPath);

        if (!data.badgeText.empty())
            cell->setBadge(data.badgeText, data.badgeColor);

        cell->setImageLayerDeferred(data.logoLayerPath, data.showLogoLayer);

        cell->setTitle(data.title);

        cell->setSubText(data.subText);

        cell->setPlayTime(data.playTime);

        cell->setDataLoaded();

        cell->onItemClicked = [this, idx = index](int) {
            if (m_page && idx < (int)m_page->m_entries.size() && m_page->onGameSelected)
                m_page->onGameSelected(m_page->m_entries[idx]);
        };

        return cell;
    }

    // ============================================================
    // _loadAndShowEntries – 首次加载前 PAGE_SIZE 条
    // ============================================================

    void GameLibraryPage::_loadAndShowEntries()
    {
        brls::Application::blockInputs(true);
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _filterEntries();
            brls::sync([this, alive]() {
                if (!alive->load()) return;
                m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                m_dataSource = new GameLibraryDS(this);
                m_grid->setDataSource(m_dataSource);
                m_grid->reloadData();
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
        if (m_platformFilter != PlatformFilter::ALL)
        {
            int targetPlatform = static_cast<int>(m_platformFilter);
            m_entries.erase(
                std::remove_if(m_entries.begin(), m_entries.end(),
                    [targetPlatform](const GameEntry& e) {
                        return e.platform != targetPlatform;
                    }),
                m_entries.end());
        }

        if (m_isSearching && !m_searchTerm.empty())
        {
            std::string lowerTerm = m_searchTerm;
            std::transform(lowerTerm.begin(), lowerTerm.end(), lowerTerm.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            m_entries.erase(
                std::remove_if(m_entries.begin(), m_entries.end(),
                    [&lowerTerm](const GameEntry& e) {
                        std::string lowerTitle = e.title;
                        std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(),
                                       [](unsigned char c) { return std::tolower(c); });
                        return lowerTitle.find(lowerTerm) == std::string::npos;
                    }),
                m_entries.end());
        }

        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameEntry& a, const GameEntry& b) {
                return a.lastPlayed > b.lastPlayed;
            });
    }

    // ============================================================
    // _loadNextPage – 增量追加
    // ============================================================

    void GameLibraryPage::_loadNextPage()
    {
        if (!m_alive.load()) return;
        if (m_loadingMore) return;
        if (m_visibleCount >= static_cast<int>(m_entries.size())) return;

        m_loadingMore = true;

        m_visibleCount = std::min(m_visibleCount + PAGE_SIZE, static_cast<int>(m_entries.size()));

        if (m_dataSource)
            m_grid->notifyDataChanged();

        m_loadingMore = false;
    }

    // ============================================================
    // _showFilterDropdown
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

                auto* alive = &m_alive;
                ThreadPool::instance().enqueue([this, alive]() {
                    if (!alive->load()) return;
                    m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                    _filterEntries();
                    brls::sync([this, alive]() {
                        if (!alive->load()) return;
                        m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                        if (m_dataSource)
                            m_grid->reloadData();
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
        if (m_isSearching)
        {
            this->getHeader()->setInfo("搜索 \"" + m_searchTerm + "\" — " + std::to_string(m_entries.size()) + " 款");
        }
        else
        {
            this->getHeader()->setInfo("共 " + std::to_string(m_entries.size()) + " 款游戏");
        }
        std::string filterStr;
        switch (m_platformFilter)
        {
            case PlatformFilter::ALL: filterStr = "所有"; break;
            case PlatformFilter::GBA: filterStr = "GBA";  break;
            case PlatformFilter::GBC: filterStr = "GBC";  break;
            case PlatformFilter::GB:  filterStr = "GB";   break;
        }
        this->getHeader()->setPath((m_isSearching ? "搜索" : "分类") + (": " + filterStr));
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
        return beiklive::tools::formatPlayTime(seconds);
    }

    // ============================================================
    // _buildItemData
    // ============================================================

    GridItemData GameLibraryPage::_buildItemData(const beiklive::GameEntry& entry)
    {
        GridItemData d;
        d.logoPath = entry.logoPath;
        d.badgeText = beiklive::tools::platformBadgeName(entry.platform);
        d.badgeColor = _platformBadge(entry.platform);
        d.logoLayerPath = GetGameLogoLayerPath(entry.platform);
        d.showLogoLayer = !d.logoLayerPath.empty();
        d.title = entry.title.empty() ? entry.path : entry.title;
        d.subText = entry.lastPlayed.empty() ? "从未游玩" : beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
        d.playTime = _formatPlayTime(entry.playTime);
        return d;
    }

    // ============================================================
    // _reloadEntries
    // ============================================================

    void GameLibraryPage::_reloadEntries()
    {
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _filterEntries();
            brls::sync([this, alive]() {
                if (!alive->load()) return;
                m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                if (m_dataSource)
                    m_grid->reloadData();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
            });
        });
    }

    // ============================================================
    // _showGameOptionsPanel / _hideGameOptionsPanel
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
