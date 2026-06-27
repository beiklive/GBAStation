#include "GameLibraryPage.hpp"
#include "ui/widget/GridItem.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "core/ThreadPool.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include <borealis/views/dropdown.hpp>
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace
{

bool deleteGameFileIfExists(const std::string& path)
{
    if (path.empty())
        return true;

    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return true;

    ec.clear();
    return std::filesystem::remove(path, ec) && !ec;
}

} // namespace

namespace beiklive
{

    GameLibraryPage::GameLibraryPage()
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("游戏库");
        this->setFocusable(false);

        m_grid = new GameGridView();
        m_grid->spanCount = 3;
        m_grid->estimatedRowHeight = 120;
        m_grid->estimatedRowSpace = 8;
        m_grid->setTitleFontSize(GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_LIBRARY_TITLE_SIZE, 0));
        m_grid->setMarginLeft(15.0f);
        m_grid->setMarginTop(0.0f);
        m_grid->setMarginBottom(10.0f);
        m_grid->setWidthPercentage(100.f);
        m_grid->setHeightPercentage(100.f);
        m_grid->setGrow(1.f);

        this->getContentBox()->addView(m_grid);

        m_grid->registerAction("退出游戏库", brls::BUTTON_B, [this](brls::View*) -> bool {
            beiklive::popActivity(this);
            return true;
        });



        m_grid->registerAction("分类", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _showFilterDropdown();
            return true;
        });

        m_grid->registerAction("设置", brls::BUTTON_X, [this](brls::View*) -> bool {
            if (m_grid->isMultiSelectMode()) {
                _showMultiSelectSidebar();
                return true;
            }
            int idx = m_grid->getSelectedIndex();
            if (idx < 0 || static_cast<size_t>(idx) >= m_entries.size())
                return true;
            m_grid->setInteractionDisabled(true);
            _showGameOptionsPanel(m_entries[idx]);
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
                            m_dataSource = new GameLibraryDS(this);
                            m_grid->setDataSource(m_dataSource);
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

        m_grid->registerAction("排序", brls::BUTTON_LT, [this](brls::View*) -> bool {
            _showSortSelector();
            return true;
        });

        m_grid->onNextPage([this]() {
            if (!m_loadingMore) _loadNextPage();
        });

        m_grid->setFocusChangeCallback([this](int idx) {
            _currentFocusedIndex = idx;
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

    size_t GameLibraryPage::GameLibraryDS::getItemCount()
    {
        return m_page ? static_cast<size_t>(m_page->m_visibleCount) : 0;
    }

    void GameLibraryPage::GameLibraryDS::populateItem(GridDrawItem& item, size_t index)
    {
        if (!m_page || index >= m_page->m_entries.size()) {
            beiklive::GridItem::populateEmpty(item, "空");
            return;
        }
        const auto& entry = m_page->m_entries[index];
        beiklive::GridItem::populateFromGameEntry(item, entry, GridItemMode::GAME_LIBRARY);
    }

    void GameLibraryPage::GameLibraryDS::onItemSelected(size_t index)
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

    void GameLibraryPage::_loadAndShowEntries()
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

    void GameLibraryPage::_filterEntries()
    {
        if (m_platformFilter == PlatformFilter::FAVORITE)
        {
            m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                [](const GameEntry& e) { return !e.favourite; }), m_entries.end());
        }
        else if (m_platformFilter != PlatformFilter::ALL)
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

        switch (m_sortMode) {
        case SortMode::PLAY_TIME:
            std::sort(m_entries.begin(), m_entries.end(),
                [](const GameEntry& a, const GameEntry& b) { return a.playTime > b.playTime; });
            break;
        case SortMode::FIRST_LETTER:
            std::sort(m_entries.begin(), m_entries.end(),
                [](const GameEntry& a, const GameEntry& b) {
                    return _titleToSortKey(a.title) < _titleToSortKey(b.title);
                });
            break;
        default:
            std::sort(m_entries.begin(), m_entries.end(),
                [](const GameEntry& a, const GameEntry& b) { return a.lastPlayed > b.lastPlayed; });
            break;
        }
    }

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

    void GameLibraryPage::_showFilterDropdown()
    {
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            auto ae = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            bool hG = false, hC = false, hB = false, hN = false, hS = false, hD = false;
            int favCount = 0;
            for (auto& e : ae) {
                if (e.favourite) favCount++;
                switch (static_cast<beiklive::enums::EmuPlatform>(e.platform)) {
                    case beiklive::enums::EmuPlatform::EmuGBA: hG = true; break;
                    case beiklive::enums::EmuPlatform::EmuGBC: hC = true; break;
                    case beiklive::enums::EmuPlatform::EmuGB:  hB = true; break;
                    case beiklive::enums::EmuPlatform::EmuNES: hN = true; break;
                    case beiklive::enums::EmuPlatform::EmuSNES: hS = true; break;
                    case beiklive::enums::EmuPlatform::EmuNDS: hD = true; break;
                    default: break;
                }
            }
            brls::sync([this, alive, hG, hC, hB, hN, hS, hD, favCount]() {
                if (!alive->load()) return;
                std::vector<std::string> opts;
                std::vector<PlatformFilter> map;
                opts.push_back("所有"); map.push_back(PlatformFilter::ALL);
                if (favCount > 0) { opts.push_back("收藏 (" + std::to_string(favCount) + ")"); map.push_back(PlatformFilter::FAVORITE); }
                if (hG) { opts.push_back("GBA"); map.push_back(PlatformFilter::GBA); }
                if (hC) { opts.push_back("GBC"); map.push_back(PlatformFilter::GBC); }
                if (hB) { opts.push_back("GB");  map.push_back(PlatformFilter::GB);  }
                if (hN) { opts.push_back("FC"); map.push_back(PlatformFilter::NES); }
                if (hS) { opts.push_back("SFC"); map.push_back(PlatformFilter::SNES); }
                if (hD) { opts.push_back("NDS"); map.push_back(PlatformFilter::NDS); }
                int cur = 0;
                for (size_t i = 0; i < map.size(); i++)
                    if (map[i] == m_platformFilter) { cur = (int)i; break; }
                auto* dd = new brls::Dropdown("游戏分类", opts,
                    [this, map, alive](int sel) {
                        if (sel < 0 || sel >= (int)map.size()) return;
                        if (map[sel] == m_platformFilter) return;
                        m_platformFilter = map[sel];
                        ThreadPool::instance().enqueue([this, alive]() {
                            if (!alive->load()) return;
                            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                            _filterEntries();
                            if (m_platformFilter == PlatformFilter::FAVORITE && m_entries.empty()) {
                                m_platformFilter = PlatformFilter::ALL;
                                m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                                _filterEntries();
                                brls::Application::notify("收藏列表为空，已切换至所有游戏");
                            }
                            brls::sync([this, alive]() {
                                if (!alive->load()) return;
                                m_visibleCount = std::min(PAGE_SIZE, (int)m_entries.size());
                                m_grid->setDefaultCellFocus(0);
                                m_dataSource = new GameLibraryDS(this);
                                m_grid->setDataSource(m_dataSource);
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
            case PlatformFilter::ALL:      fs = "所有"; break;
            case PlatformFilter::GBA:      fs = "GBA";  break;
            case PlatformFilter::GBC:      fs = "GBC";  break;
            case PlatformFilter::GB:       fs = "GB";   break;
            case PlatformFilter::NES:      fs = "FC";  break;
            case PlatformFilter::SNES:     fs = "SFC"; break;
            case PlatformFilter::NDS:      fs = "NDS"; break;
            case PlatformFilter::FAVORITE: fs = "收藏"; break;
        }
        this->getHeader()->setPath((m_isSearching ? "搜索" : "分类") + (": " + fs));
    }

    std::string GameLibraryPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "";
        return beiklive::tools::formatPlayTime(seconds);
    }

    void GameLibraryPage::_showSortSelector()
    {
        std::vector<std::string> opts = {"最近游玩", "游玩时长", "首字母"};
        int cur = static_cast<int>(m_sortMode);
        auto* dd = new brls::Dropdown("排序方式", opts,
            [this](int sel) {
                if (sel < 0 || sel >= 3) return;
                auto newMode = static_cast<SortMode>(sel);
                if (newMode == m_sortMode) return;
                m_sortMode = newMode;
                _reloadEntries();
            },
            cur);
        brls::Application::pushActivity(new brls::Activity(dd));
    }

    std::string GameLibraryPage::_titleToSortKey(const std::string& title)
    {
        static nlohmann::json pinyinMap;
        static bool loaded = false;
        if (!loaded) {
            std::ifstream f(BK_RES("pinyin/pingyin.json"));
            if (f.is_open()) {
                f >> pinyinMap;
                loaded = true;
            }
        }

        std::string key;
        for (size_t i = 0; i < title.size(); i++) {
            std::string ch(1, title[i]);
            unsigned char c = static_cast<unsigned char>(title[i]);
            if (c >= 0x80 && i + 2 < title.size()) {
                ch = title.substr(i, 3);
                i += 2;
            }
            if (pinyinMap.contains(ch)) {
                key += pinyinMap[ch].get<std::string>();
            } else if (c >= 0x80) {
                key += "\xFF"; // unknown CJK, sort after everything
            } else if (std::isdigit(c)) {
                key += std::string(1, '\x00') + ch; // digits first
            } else if (std::isalpha(c)) {
                key += std::string(1, '\x01') + std::string(1, static_cast<char>(std::tolower(c)));
            } else {
                key += std::string(1, '\x02') + ch;
            }
        }
        return key;
    }

    void GameLibraryPage::_reloadEntries()
    {
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _filterEntries();
            if (m_platformFilter == PlatformFilter::FAVORITE && m_entries.empty()) {
                m_platformFilter = PlatformFilter::ALL;
                m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                _filterEntries();
                brls::Application::notify("收藏列表为空，已切换至所有游戏");
            }
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

    void GameLibraryPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        std::string path = entry.path;
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);
        std::string fn = beiklive::tools::getFileNameWithoutExtension(entry.path);
        auto enterMultiSelect = [this](bool selectAll) {
            _hideGameOptionsPanel();
            m_grid->setMultiSelectMode(true);
            if (selectAll)
            {
                m_grid->selectAllForDelete(m_entries.size());
                brls::Application::notify("已全选当前列表中的全部游戏");
            }
            m_grid->setInteractionDisabled(false);
            brls::Application::giveFocus(m_grid);
        };

        m_gameOptionsSidebar->addButton("修改映射名称", BK_RES("img/ui/setting/emu.png"),
            [this, path, title = entry.title, fn, idx = m_grid->getSelectedIndex()](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) { m_grid->setInteractionDisabled(false); return; }
                ime->openForText(
                    [this, path, fn, idx](std::string text) {
                        if (!text.empty() && beiklive::GameDB) {
                            beiklive::GameDB->set(path, "title", nlohmann::json(text));
                            beiklive::GameDB->flush();
                            beiklive::NameMappingManager->Set(fn, text, true);
                            beiklive::NameMappingManager->Save();
                            m_grid->setItemTitle(idx, text);
                        }
                        m_grid->setInteractionDisabled(false);
                    },
                    "编辑游戏名称", "", 128, title,
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            });

        m_gameOptionsSidebar->addButton("设置封面图", BK_RES("img/ui/setting/display.png"),
            [this, path, idx = m_grid->getSelectedIndex()](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                beiklive::openFilePicker({"png", "jpg"},
                    [this, path, idx](const std::string& selectedPath) {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(path, "logoPath", nlohmann::json(selectedPath));
                            beiklive::GameDB->flush();
                            m_grid->setItemImagePath(idx, selectedPath);
                        }
                        m_grid->setInteractionDisabled(false);
                    }, beiklive::path::GetRootPath());
            });

        if (beiklive::GetCoreOptions(entry.platform).size() > 1)
        {
            m_gameOptionsSidebar->addButton("核心选择", BK_RES("img/ui/setting/emu.png"),
                [this, path, platform = entry.platform, core = entry.core,
                 idx = m_grid->getSelectedIndex()](const beiklive::GameEntry&) {
                    _hideGameOptionsPanel();

                    const auto options = beiklive::GetCoreOptions(platform);
                    std::vector<std::string> names;
                    names.reserve(options.size());
                    for (const auto& option : options)
                        names.push_back(option.name);

                    auto* dropdown = new brls::Dropdown(
                        "核心选择",
                        names,
                        [this, path, idx, options](int selected) {
                            if (selected < 0 || selected >= static_cast<int>(options.size()))
                                return;
                            if (beiklive::GameDB) {
                                beiklive::GameDB->set(path, "core", nlohmann::json(options[selected].id));
                                beiklive::GameDB->flush();
                                if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size())
                                    m_entries[idx].core = options[selected].id;
                                brls::Application::notify("已切换核心：" + options[selected].name);
                            }
                            m_grid->setInteractionDisabled(false);
                        },
                        beiklive::GetCoreSelectionIndex(platform, core),
                        [this](int) {
                            m_grid->setInteractionDisabled(false);
                        });
                    brls::Application::pushActivity(new brls::Activity(dropdown));
                });
        }

        m_gameOptionsSidebar->addButton("删除游戏", BK_RES("img/ui/menu/exit.png"),
            [this, path](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                auto* removeDlg = new brls::Dialog("是否从游戏库移除该游戏？");
                removeDlg->addButton("是", [this, path]() {
                    auto* romDlg = new brls::Dialog("是否删除 ROM 文件？");
                    auto deleteGame = [this, path](bool deleteRomFile) {
                        if (beiklive::GameDB) {
                            bool removedRecord = false;
                            if ((int)beiklive::GameDB->getAll().size() <= 1)
                            {
                                beiklive::GameDB->clearAll();
                                removedRecord = true;
                            }
                            else {
                                removedRecord = beiklive::GameDB->removeByPath(path);
                                if (removedRecord)
                                    beiklive::GameDB->flush();
                            }

                            bool removedFile = true;
                            if (removedRecord && deleteRomFile)
                                removedFile = deleteGameFileIfExists(path);

                            if (!removedRecord)
                                brls::Application::notify("删除失败");
                            else if (deleteRomFile)
                                brls::Application::notify(removedFile ? "已删除游戏" : "已移除记录，ROM 文件删除失败");
                            else
                                brls::Application::notify("已从游戏库移除该游戏");
                            _reloadEntries();
                        } else {
                            brls::Application::notify("删除失败");
                        }
                        m_grid->setInteractionDisabled(false);
                    };
                    romDlg->addButton("是", [deleteGame]() { deleteGame(true); });
                    romDlg->addButton("否", [deleteGame]() { deleteGame(false); });
                    romDlg->addButton("不删了", [this]() { m_grid->setInteractionDisabled(false); });
                    romDlg->open();
                });
                removeDlg->addButton("否", [this]() { m_grid->setInteractionDisabled(false); });
                removeDlg->addButton("不删了", [this]() { m_grid->setInteractionDisabled(false); });
                removeDlg->open();
            });

        m_gameOptionsSidebar->addButton(
            entry.favourite ? "取消收藏" : "加入收藏",
            BK_RES("img/ui/setting/emu.png"),
            [this, path, fav = entry.favourite, idx = m_grid->getSelectedIndex()](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                std::string msg = fav ? "确定要取消收藏吗？" : "确定要加入收藏吗？";
                auto* dlg = new brls::Dialog(msg);
                dlg->addButton("确认", [this, path, fav, idx]() {
                    if (beiklive::GameDB) {
                        beiklive::GameDB->set(path, "favourite", nlohmann::json(!fav));
                        beiklive::GameDB->flush();
                        m_grid->setItemFavourite(idx, !fav);
                    }
                    m_grid->setInteractionDisabled(false);
                });
                dlg->addButton("取消", [this]() { m_grid->setInteractionDisabled(false); });
                dlg->open();
            });

        m_gameOptionsSidebar->addButton(
            "多选",
            BK_RES("img/ui/setting/emu.png"),
            [enterMultiSelect](const beiklive::GameEntry&) {
                enterMultiSelect(false);
            });

        m_gameOptionsSidebar->addButton(
            "全选",
            BK_RES("img/ui/setting/emu.png"),
            [enterMultiSelect](const beiklive::GameEntry&) {
                enterMultiSelect(true);
            });

        m_gameOptionsSidebar->onClosed = [this]() {
            m_grid->setInteractionDisabled(false);
            brls::Application::giveFocus(m_grid);
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        };
        this->addView(m_gameOptionsSidebar);
        m_grid->setInteractionDisabled(true);
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

    void GameLibraryPage::_showMultiSelectSidebar()
    {
        m_grid->setInteractionDisabled(true);
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);

        size_t count = m_grid->getDeleteSelection().size();

        m_gameOptionsSidebar->addButton(
            "取消多选",
            BK_RES("img/ui/setting/emu.png"),
            [this](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                m_grid->clearDeleteSelection();
                m_grid->setInteractionDisabled(false);
                brls::Application::notify("已取消多选模式");
            });

        if (count > 0) {
            m_gameOptionsSidebar->addButton(
                "删除已选游戏 (" + std::to_string(count) + ")",
                BK_RES("img/ui/menu/exit.png"),
                [this](const beiklive::GameEntry&) {
                    _hideGameOptionsPanel();
                    std::vector<int> sel(m_grid->getDeleteSelection().begin(),
                                         m_grid->getDeleteSelection().end());
                    size_t n = sel.size();
                    std::string msg = "是否从游戏库移除这 " + std::to_string(n) + " 款游戏？";
                    auto* removeDlg = new brls::Dialog(msg);
                    removeDlg->addButton("是", [this, sel]() {
                        auto* romDlg = new brls::Dialog("是否删除 ROM 文件？");
                        auto deleteSelected = [this, sel](bool deleteRomFiles) {
                            bool allFilesRemoved = true;
                            bool removedAnyRecord = false;
                            if (beiklive::GameDB) {
                                if ((int)beiklive::GameDB->getAll().size() <= (int)sel.size())
                                {
                                    if (deleteRomFiles) {
                                        for (int idx : sel) {
                                            if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size())
                                                allFilesRemoved = deleteGameFileIfExists(m_entries[idx].path) && allFilesRemoved;
                                        }
                                    }
                                    beiklive::GameDB->clearAll();
                                    removedAnyRecord = true;
                                }
                                else {
                                    for (int idx : sel) {
                                        if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size()) {
                                            const auto& path = m_entries[idx].path;
                                            if (beiklive::GameDB->removeByPath(path)) {
                                                removedAnyRecord = true;
                                                if (deleteRomFiles)
                                                    allFilesRemoved = deleteGameFileIfExists(path) && allFilesRemoved;
                                            }
                                        }
                                    }
                                    beiklive::GameDB->flush();
                                }
                            }
                            if (!removedAnyRecord)
                                brls::Application::notify("删除失败");
                            else if (deleteRomFiles && !allFilesRemoved)
                                brls::Application::notify("已移除记录，部分 ROM 文件删除失败");
                            else
                                brls::Application::notify(deleteRomFiles ? "已删除所选游戏" : "已从游戏库移除所选游戏");
                            m_grid->clearDeleteSelection();
                            _reloadEntries();
                            m_grid->setInteractionDisabled(false);
                        };
                        romDlg->addButton("是", [deleteSelected]() { deleteSelected(true); });
                        romDlg->addButton("否", [deleteSelected]() { deleteSelected(false); });
                        romDlg->addButton("不删了", [this]() { m_grid->setInteractionDisabled(false); });
                        romDlg->open();
                    });
                    removeDlg->addButton("否", [this]() {
                        m_grid->setInteractionDisabled(false);
                    });
                    removeDlg->addButton("不删了", [this]() {
                        m_grid->setInteractionDisabled(false);
                    });
                    removeDlg->open();
                });
        }

        m_gameOptionsSidebar->onClosed = [this]() {
            m_grid->setInteractionDisabled(false);
            brls::Application::giveFocus(m_grid);
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        };
        this->addView(m_gameOptionsSidebar);
        m_gameOptionsSidebar->open(beiklive::GameEntry{});
    }

} // namespace beiklive
