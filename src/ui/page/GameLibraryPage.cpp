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

        m_grid = new RecyclingGrid();
        m_grid->spanCount = 3;
        m_grid->estimatedRowHeight = 120;
        m_grid->estimatedRowSpace = 8;
        m_grid->setMarginLeft(10.0f);
        m_grid->setMarginTop(10.0f);
        m_grid->setMarginBottom(10.0f);
        // m_grid->setPaddingLeft(5.0f);
        m_grid->setGrow(1.f);

        m_grid->registerCell("GridItem", []() -> RecyclingGridItem* {
            auto* item = new beiklive::GridItem(GridItemMode::GAME_LIBRARY, 0);
            item->reuseIdentifier = "GridItem";
            return item;
        });

        this->getContentBox()->addView(m_grid);

        m_grid->registerAction("分类", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _showFilterDropdown();
            return true;
        });

        m_grid->registerAction("设置", brls::BUTTON_X, [this](brls::View*) -> bool {
            if (_currentFocusedIndex < 0 || _currentFocusedIndex >= static_cast<int>(m_entries.size()))
                return true;
            _showGameOptionsPanel(m_entries[_currentFocusedIndex]);
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
                            if (m_isSearching && m_entries.empty()) {
                                auto* dialog = new brls::Dialog("当前分类下无 \"" + m_searchTerm + "\"");
                                dialog->addButton("确认", []() {});
                                dialog->open();
                                return;
                            }
                            m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                            m_grid->setDefaultCellFocus(0);
                            m_grid->reloadData();
                            _updateHeader();
                            brls::Application::giveFocus(m_grid);
                        });
                    });
                },
                "搜索游戏", "", 128, m_searchTerm,
                brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            return true;
        });

        m_grid->onNextPage([this]() {
            if (!m_loadingMore) _loadNextPage();
        });

        m_grid->setFocusChangeCallback([this](size_t idx) {
            _currentFocusedIndex = static_cast<int>(idx);
        });

        _loadAndShowEntries();
    }

    GameLibraryPage::~GameLibraryPage()
    {
        m_alive.store(false);
    }

    void GameLibraryPage::willAppear(bool resetState)
    {
        brls::Box::willAppear(resetState);
        if (m_firstAppear) {
            m_firstAppear = false;
            return;
        }
        _reloadEntries();
    }

    // ============================================================
    // GameLibraryDS
    // ============================================================

    size_t GameLibraryPage::GameLibraryDS::getItemCount()
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

        cell->setImagePath(entry.logoPath);

        std::string badgeText = beiklive::tools::platformBadgeName(entry.platform);
        PlatformBadgeColor badgeColor;
        switch (static_cast<beiklive::enums::EmuPlatform>(entry.platform))
        {
            case beiklive::enums::EmuPlatform::EmuGBA: badgeColor = PlatformBadgeColor::GBA; break;
            case beiklive::enums::EmuPlatform::EmuGBC: badgeColor = PlatformBadgeColor::GBC; break;
            case beiklive::enums::EmuPlatform::EmuGB:  badgeColor = PlatformBadgeColor::GB;  break;
            default: badgeColor = PlatformBadgeColor::NONE; break;
        }
        if (!badgeText.empty())
            cell->setBadge(badgeText, badgeColor);

        std::string logoLayerPath = GetGameLogoLayerPath(entry.platform);
        cell->setImageLayer(logoLayerPath, !logoLayerPath.empty());

        cell->setTitle(entry.title.empty() ? entry.path : entry.title);
        std::string lastPlayed = entry.lastPlayed.empty() ? "从未游玩" : beiklive::tools::formatTimestampForDisplay(entry.lastPlayed);
        cell->setSubText(lastPlayed);
        cell->setPlayTime(_formatPlayTime(entry.playTime));
        cell->setDataLoaded();

        return cell;
    }

    void GameLibraryPage::GameLibraryDS::onItemSelected(RecyclingGrid*, size_t index)
    {
        if (!m_page || index >= m_page->m_entries.size()) return;
        if (m_page->onGameSelected) {
            auto& cached = m_page->m_entries[index];
            auto fresh = beiklive::GameDB
                ? beiklive::GameDB->findByPath(cached.path)
                : std::optional<beiklive::GameEntry>{};
            m_page->onGameSelected(fresh.has_value() ? *fresh : cached);
        }
    }

    void GameLibraryPage::GameLibraryDS::clearData()
    {
    }

    // ============================================================
    // _loadAndShowEntries
    // ============================================================

    void GameLibraryPage::_loadAndShowEntries()
    {
        // brls::Application::blockInputs(true);
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _filterEntries();
            brls::sync([this, alive]() {
                if (!alive->load()) return;
                m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                m_grid->setDefaultCellFocus(0);
                m_dataSource = new GameLibraryDS(this);
                m_grid->setDataSource(m_dataSource);
                m_grid->reloadData();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
                // brls::Application::unblockInputs();
            });
        });
    }

    // ============================================================
    // _filterEntries
    // ============================================================

    void GameLibraryPage::_filterEntries()
    {
        if (m_platformFilter != PlatformFilter::ALL)
        {
            int target = static_cast<int>(m_platformFilter);
            m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                [target](const GameEntry& e) { return e.platform != target; }), m_entries.end());
        }
        if (m_isSearching && !m_searchTerm.empty())
        {
            std::string lt = m_searchTerm;
            std::transform(lt.begin(), lt.end(), lt.begin(), [](unsigned char c) { return std::tolower(c); });
            m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                [&lt](const GameEntry& e) {
                    std::string t = e.title;
                    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::tolower(c); });
                    return t.find(lt) == std::string::npos;
                }), m_entries.end());
        }
        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameEntry& a, const GameEntry& b) { return a.lastPlayed > b.lastPlayed; });
    }

    // ============================================================
    // _loadNextPage
    // ============================================================

    void GameLibraryPage::_loadNextPage()
    {
        brls::Logger::info("_loadNextPage: alive={} loading={} visible={} total={}",
            m_alive.load(), m_loadingMore, m_visibleCount, m_entries.size());
        if (!m_alive.load() || m_loadingMore) return;
        if (m_visibleCount >= static_cast<int>(m_entries.size())) return;

        m_loadingMore = true;
        m_visibleCount = std::min(m_visibleCount + PAGE_SIZE, static_cast<int>(m_entries.size()));
        m_grid->notifyDataChanged();
        m_loadingMore = false;
    }

    // ============================================================
    // _showFilterDropdown
    // ============================================================

    void GameLibraryPage::_showFilterDropdown()
    {
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            auto ae = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            bool hG = false, hC = false, hB = false;
            for (auto& e : ae) {
                switch (static_cast<beiklive::enums::EmuPlatform>(e.platform)) {
                    case beiklive::enums::EmuPlatform::EmuGBA: hG = true; break;
                    case beiklive::enums::EmuPlatform::EmuGBC: hC = true; break;
                    case beiklive::enums::EmuPlatform::EmuGB:  hB = true; break;
                    default: break;
                }
            }
            brls::sync([this, alive, hG, hC, hB]() {
                if (!alive->load()) return;
                std::vector<std::string> opts;
                std::vector<PlatformFilter> map;
                opts.push_back("所有"); map.push_back(PlatformFilter::ALL);
                if (hG) { opts.push_back("GBA"); map.push_back(PlatformFilter::GBA); }
                if (hC) { opts.push_back("GBC"); map.push_back(PlatformFilter::GBC); }
                if (hB) { opts.push_back("GB");  map.push_back(PlatformFilter::GB);  }
                int cur = 0;
                for (size_t i = 0; i < map.size(); i++)
                    if (map[i] == m_platformFilter) { cur = (int)i; break; }
                auto* dd = new brls::Dropdown("游戏分类", opts, [](int){}, cur,
                    [this, map, alive](int sel) {
                        if (sel < 0 || sel >= (int)map.size()) return;
                        if (map[sel] == m_platformFilter) return;
                        m_platformFilter = map[sel];
                        ThreadPool::instance().enqueue([this, alive]() {
                            if (!alive->load()) return;
                            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                            _filterEntries();
                            brls::sync([this, alive]() {
                                if (!alive->load()) return;
                                m_visibleCount = std::min(PAGE_SIZE, (int)m_entries.size());
                                m_grid->setDefaultCellFocus(0);
                                m_grid->reloadData();
                                _updateHeader();
                                brls::Application::giveFocus(m_grid);
                            });
                        });
                    });
                brls::Application::pushActivity(new brls::Activity(dd));
            });
        });
    }

    void GameLibraryPage::_updateHeader()
    {
        if (m_isSearching)
            this->getHeader()->setInfo("搜索 \"" + m_searchTerm + "\" — " + std::to_string(m_entries.size()) + " 款");
        else
            this->getHeader()->setInfo("共 " + std::to_string(m_entries.size()) + " 款游戏");
        std::string fs;
        switch (m_platformFilter) {
            case PlatformFilter::ALL: fs = "所有"; break;
            case PlatformFilter::GBA: fs = "GBA";  break;
            case PlatformFilter::GBC: fs = "GBC";  break;
            case PlatformFilter::GB:  fs = "GB";   break;
        }
        this->getHeader()->setPath((m_isSearching ? "搜索" : "分类") + (": " + fs));
    }

    std::string GameLibraryPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "";
        return beiklive::tools::formatPlayTime(seconds);
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
                m_grid->setDefaultCellFocus(0);
                m_dataSource = new GameLibraryDS(this);
                m_grid->setDataSource(m_dataSource);
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
        std::string path = entry.path;
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);
        std::string fn = beiklive::tools::getFileNameWithoutExtension(entry.path);

        m_gameOptionsSidebar->addButton("修改映射名称", BK_RES("img/ui/setting/emu.png"),
            [this, path, title = entry.title, fn](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) return;
                ime->openForText(
                    [this, path, fn](std::string text) {
                        if (!text.empty() && beiklive::GameDB) {
                            beiklive::GameDB->set(path, "title", nlohmann::json(text));
                            _reloadEntries();
                            beiklive::NameMappingManager->Set(fn, text, true);
                            beiklive::GameDB->flush();
                            beiklive::NameMappingManager->Save();
                        }
                    },
                    "编辑游戏名称", "", 128, title,
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            });

        m_gameOptionsSidebar->addButton("设置封面图", BK_RES("img/ui/setting/display.png"),
            [this, path](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                beiklive::openFilePicker({"png", "jpg"},
                    [this, path](const std::string& selectedPath) {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(path, "logoPath", nlohmann::json(selectedPath));
                            _reloadEntries();
                            beiklive::GameDB->flush();
                        }
                    }, beiklive::path::GetRootPath());
            });

        m_gameOptionsSidebar->addButton("删除游戏", BK_RES("img/ui/menu/exit.png"),
            [this, path](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                auto* dlg = new brls::Dialog("确定要删除该游戏吗？\n此操作将清除游戏记录与存档数据。");
                dlg->addButton("确认删除", [this, path]() {
                    _hideGameOptionsPanel();
                    if (beiklive::GameDB && beiklive::GameDB->removeByPath(path)) {
                        brls::Application::notify("已删除游戏");
                        _reloadEntries();
                        beiklive::GameDB->flush();
                    } else brls::Application::notify("删除失败");
                });
                dlg->addButton("取消", [](){});
                dlg->open();
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
        if (m_gameOptionsSidebar) {
            m_gameOptionsSidebar->close();
            m_gameOptionsSidebar->removeFromSuperView(true);
            m_gameOptionsSidebar = nullptr;
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        }
    }

} // namespace beiklive
