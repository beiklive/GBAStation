#include "UI/Pages/GameLibraryPage.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

#include "UI/Pages/FileListPage.hpp"
#include "Utils/strUtils.hpp"
#include "Utils/fileUtils.hpp"

namespace fs = std::filesystem;
using namespace brls::literals; // for _i18n

// 布局常量
static constexpr float LIST_CELL_HEIGHT  = 66.f;   ///< 列表行高
static constexpr float LIST_ICON_SZ      = 50.f;   ///< 封面图标尺寸
static constexpr float LIST_ACCENT_W     = 4.f;    ///< 左侧强调色矩形宽度
static constexpr float LIST_PAD_H        = 12.f;   ///< 水平内边距
static constexpr float LIST_PAD_V        = 10.f;   ///< 垂直内边距
static constexpr float LIST_NAME_FS      = 20.f;   ///< 名称标签字号
static constexpr float LIST_INFO_FS      = 12.f;   ///< 信息标签字号

/// gamedataManager 中 lastopen 字段的"从未游玩"占位值（与 common.hpp initGameData 保持一致）
static const std::string LASTOPEN_UNPLAYED = "从未游玩";

// 辅助函数

/// 从 gamedataManager 收集所有 gamepath 条目，返回 (fileName, gamePath) 对列表
static std::vector<std::pair<std::string, std::string>> collectLibraryGameEntries()
{
    std::vector<std::pair<std::string, std::string>> result;
    if (!gamedataManager) return result;

    static const std::string SUFFIX = std::string(".") + GAMEDATA_FIELD_GAMEPATH;
    for (const auto& key : gamedataManager->GetAllKeys()) {
        if (key.size() <= SUFFIX.size()) continue;
        if (key.compare(key.size() - SUFFIX.size(), SUFFIX.size(), SUFFIX) != 0) continue;
        auto v = gamedataManager->Get(key);
        if (!v) continue;
        auto s = v->AsString();
        if (!s || s->empty()) continue;
        std::string fileName = fs::path(*s).filename().string();
        result.push_back({ fileName, *s });
    }
    return result;
}

/// 获取游戏的显示名称：优先从 NameMappingManager 查询，否则返回文件名（不含后缀）
static std::string getLibDisplayName(const std::string& fileName)
{
    if (NameMappingManager) {
        std::string mapKey = gamedataKeyPrefix(fileName);
        auto mv = NameMappingManager->Get(mapKey);
        if (mv && mv->AsString() && !mv->AsString()->empty())
            return *mv->AsString();
    }
    return fs::path(fileName).stem().string();
}

// GameLibraryListItem

GameLibraryListItem::GameLibraryListItem(const GameLibraryEntry& entry, int index)
    : brls::Box(brls::Axis::ROW)
    , m_entry(entry)
    , m_index(index)
{
    setMarginTop(5.f);
    setFocusable(true);
    setAlignItems(brls::AlignItems::CENTER);
    setHeight(LIST_CELL_HEIGHT);
    setWidth(brls::View::AUTO);
    setHideHighlightBackground(true);

    // 左侧强调色矩形（焦点时显示）
    m_accent = new brls::Rectangle();
    m_accent->setWidth(LIST_ACCENT_W);
    m_accent->setHeight(LIST_CELL_HEIGHT - 2 * LIST_PAD_V);
    m_accent->setMarginLeft(LIST_PAD_H);
    m_accent->setMarginRight(LIST_PAD_H);
    m_accent->setColor(nvgRGBA(79, 193, 255, 255));
    m_accent->setVisibility(brls::Visibility::INVISIBLE);
    addView(m_accent);

    // 封面图标（小图）
    m_icon = new beiklive::UI::ProImage();
    m_icon->setWidth(LIST_ICON_SZ);
    m_icon->setHeight(LIST_ICON_SZ);
    m_icon->setCornerRadius(4.f);
    m_icon->setScalingType(brls::ImageScalingType::FIT);
    m_icon->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_icon->setMarginRight(LIST_PAD_H);
    m_icon->setBackgroundColor(nvgRGBA(31, 31, 31, 50));
    if (!entry.logoPath.empty()) {
        m_icon->setImageFromFileAsync(entry.logoPath);
    } else {
        m_icon->setImageFromFile(BK_APP_DEFAULT_LOGO);
    }
    addView(m_icon);

    // 游戏显示名称（自动填充剩余空间）
    m_nameLabel = new brls::Label();
    m_nameLabel->setFontSize(LIST_NAME_FS);
    m_nameLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_nameLabel->setSingleLine(true);
    m_nameLabel->setAutoAnimate(true);
    m_nameLabel->setGrow(1.0f);
    m_nameLabel->setMarginRight(LIST_PAD_H);
    m_nameLabel->setMarginTop(8.f);
    m_nameLabel->setText(entry.displayName.empty() ? "—" : entry.displayName);
    addView(m_nameLabel);

    // 右侧信息（上次游玩时间）
    auto* infoBox = new brls::Box(brls::Axis::COLUMN);
    infoBox->setHeight(LIST_CELL_HEIGHT);
    m_infoLabel = new brls::Label();
    m_infoLabel->setFontSize(LIST_INFO_FS);
    m_infoLabel->setSingleLine(true);
    m_infoLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_infoLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
    m_infoLabel->setMarginRight(LIST_PAD_H * 2);
    if (entry.lastOpen == LASTOPEN_UNPLAYED || entry.lastOpen.empty()) {
        m_infoLabel->setText("从未游玩");
    } else {
        m_infoLabel->setText(entry.lastOpen);
    }
    infoBox->addView(new brls::Padding());
    infoBox->addView(m_infoLabel);
    addView(infoBox);

    addGestureRecognizer(new brls::TapGestureRecognizer(this));

    // A 键：激活游戏
    registerAction("beiklive/hints/start"_i18n, brls::BUTTON_A, [this](brls::View*) {
        if (onActivated) onActivated(m_index);
        return true;
    }, false, false, brls::SOUND_CLICK);

    // X 键：选项菜单
    registerAction("beiklive/hints/set"_i18n, brls::BUTTON_X, [this](brls::View*) {
        if (onOptions) onOptions(m_index);
        return true;
    });
}

void GameLibraryListItem::onFocusGained()
{
    brls::Box::onFocusGained();
    if (m_accent) m_accent->setVisibility(brls::Visibility::VISIBLE);
    if (onFocused) onFocused(m_index);
}

void GameLibraryListItem::onFocusLost()
{
    brls::Box::onFocusLost();
    if (m_accent) m_accent->setVisibility(brls::Visibility::INVISIBLE);
}

void GameLibraryListItem::updateCover(const std::string& newLogoPath)
{
    m_entry.logoPath = newLogoPath;
    if (!newLogoPath.empty())
        m_icon->setImageFromFileAsync(newLogoPath);
    else
        m_icon->setImageFromFile(BK_APP_DEFAULT_LOGO);
}

void GameLibraryListItem::updateTitle(const std::string& newTitle)
{
    m_entry.displayName = newTitle;
    if (m_nameLabel) m_nameLabel->setText(newTitle.empty() ? "—" : newTitle);
}

// GameLibraryPage

/// 详情面板固定宽度（逻辑像素）
static constexpr float LIB_DETAIL_PANEL_W  = 400.f;
/// 详情面板缩略图尺寸
static constexpr float LIB_DETAIL_THUMB_SZ = 260.f;

GameLibraryPage::GameLibraryPage()
{
    setAxis(brls::Axis::COLUMN);
    setGrow(1.0f);

    // 标题栏
    m_header = new beiklive::UI::BrowserHeader();
    m_header->setTitle("beiklive/library/title"_i18n);
    addView(m_header);

    // 主体：横向内容区（列表 + 右侧详情面板）
    auto* contentBox = new brls::Box(brls::Axis::ROW);
    contentBox->setGrow(1.0f);
    contentBox->setWidth(brls::View::AUTO);

    // 列表滚动区（自适应宽度）
    auto* listBox = new brls::Box(brls::Axis::COLUMN);
    listBox->setGrow(1.0f);
    listBox->setWidth(brls::View::AUTO);

    m_scroll = new brls::ScrollingFrame();
    m_scroll->setGrow(1.0f);
    m_scroll->setWidth(brls::View::AUTO);
    m_scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    m_scroll->setScrollingIndicatorVisible(false);

    m_itemsBox = new brls::Box(brls::Axis::COLUMN);
    m_itemsBox->setWidth(brls::View::AUTO);
    m_itemsBox->setPadding(
        GET_STYLE("brls/applet_frame/header_padding_top_bottom"),
        GET_STYLE("brls/applet_frame/header_padding_sides"),
        GET_STYLE("brls/applet_frame/header_padding_top_bottom"),
        GET_STYLE("brls/applet_frame/header_padding_sides")
    );
    m_scroll->setContentView(m_itemsBox);
    listBox->addView(m_scroll);
    contentBox->addView(listBox);

    // 右侧详情面板（固定宽度，常驻显示）
    buildDetailPanel();
    contentBox->addView(m_detailPanel);

    addView(contentBox);

    // 底栏
    addView(new brls::BottomBar());

    // Y 键：弹出排序菜单
    registerAction("beiklive/library/sort"_i18n, brls::BUTTON_Y, [this](brls::View*) {
        showSortDropdown();
        return true;
    });

    // 加载并显示游戏数据
    loadEntries();
    sortEntries();
    rebuildList();
    updateHeader();
}

void GameLibraryPage::buildDetailPanel()
{
    m_detailPanel = new brls::Box(brls::Axis::COLUMN);
    m_detailPanel->setAlignItems(brls::AlignItems::CENTER);
    m_detailPanel->setJustifyContent(brls::JustifyContent::CENTER);
    m_detailPanel->setWidth(LIB_DETAIL_PANEL_W);
    m_detailPanel->setPadding(20.f, 12.f, 20.f, 12.f);
    m_detailPanel->setMarginLeft(8.f);
    m_detailPanel->setBackgroundColor(nvgRGBA(40, 40, 40, 20));

    // 封面缩略图
    m_detailThumb = new beiklive::UI::ProImage();
    m_detailThumb->setWidth(LIB_DETAIL_THUMB_SZ);
    m_detailThumb->setHeight(LIB_DETAIL_THUMB_SZ);
    m_detailThumb->setCornerRadius(8.f);
    m_detailThumb->setScalingType(brls::ImageScalingType::FIT);
    m_detailThumb->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_detailThumb->setClipsToBounds(false);
    m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
    m_detailPanel->addView(m_detailThumb);

    // 显示名称
    m_detailName = new brls::Label();
    m_detailName->setFontSize(22.f);
    m_detailName->setMarginTop(12.f);
    m_detailName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailName->setSingleLine(true);
    m_detailName->setAutoAnimate(true);
    m_detailName->setTextColor(nvgRGBA(220, 220, 220, 255));
    m_detailPanel->addView(m_detailName);

    // 文件名
    m_detailFileName = new brls::Label();
    m_detailFileName->setFontSize(14.f);
    m_detailFileName->setMarginTop(6.f);
    m_detailFileName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailFileName->setSingleLine(true);
    m_detailFileName->setAutoAnimate(true);
    m_detailFileName->setTextColor(nvgRGBA(160, 160, 160, 255));
    m_detailPanel->addView(m_detailFileName);

    // 上次游玩时间
    m_detailLastOpen = new brls::Label();
    m_detailLastOpen->setFontSize(16.f);
    m_detailLastOpen->setMarginTop(10.f);
    m_detailLastOpen->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailLastOpen->setTextColor(nvgRGBA(140, 200, 140, 255));
    m_detailPanel->addView(m_detailLastOpen);

    // 游玩总时长
    m_detailTotalTime = new brls::Label();
    m_detailTotalTime->setFontSize(16.f);
    m_detailTotalTime->setMarginTop(4.f);
    m_detailTotalTime->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailTotalTime->setTextColor(nvgRGBA(200, 200, 140, 255));
    m_detailPanel->addView(m_detailTotalTime);

    // 平台名称
    m_detailPlatform = new brls::Label();
    m_detailPlatform->setFontSize(16.f);
    m_detailPlatform->setMarginTop(4.f);
    m_detailPlatform->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailPlatform->setTextColor(nvgRGBA(140, 160, 220, 255));
    m_detailPanel->addView(m_detailPlatform);
}

void GameLibraryPage::updateDetailPanel(const GameLibraryEntry& entry)
{
    if (!m_detailPanel) return;

    // 封面图
    if (!entry.logoPath.empty() &&
        beiklive::file::getPathType(entry.logoPath) == beiklive::file::PathType::File)
    {
        m_detailThumb->setImageFromFileAsync(entry.logoPath);
    } else {
        m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
    }

    m_detailName->setText(entry.displayName.empty() ? "—" : entry.displayName);
    m_detailFileName->setText(entry.fileName);
    m_detailLastOpen->setText("上次游玩: " + entry.lastOpen);

    // 游玩总时长（格式化为 Xh Ym Zs）
    int totalSeconds = entry.totalTime;
    int h = totalSeconds / 3600;
    int m = (totalSeconds % 3600) / 60;
    int s = totalSeconds % 60;
    std::string timeStr;
    if (h > 0) timeStr += std::to_string(h) + "h ";
    if (h > 0 || m > 0) timeStr += std::to_string(m) + "m ";
    timeStr += std::to_string(s) + "s";
    m_detailTotalTime->setText("游玩时长: " + timeStr);

    std::string platStr = getGameDataStr(entry.fileName, GAMEDATA_FIELD_PLATFORM, "");
    m_detailPlatform->setText(platStr.empty() ? "" : "平台: " + platStr);
}

void GameLibraryPage::clearDetailPanel()
{
    if (!m_detailPanel) return;
    m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
    m_detailName->setText("");
    m_detailFileName->setText("");
    m_detailLastOpen->setText("");
    m_detailTotalTime->setText("");
    m_detailPlatform->setText("");
}

void GameLibraryPage::loadEntries()
{
    m_entries.clear();
    auto rawEntries = collectLibraryGameEntries();
    for (const auto& [fileName, gamePath] : rawEntries) {
        GameLibraryEntry e;
        e.fileName    = fileName;
        e.gamePath    = gamePath;
        e.displayName = getLibDisplayName(fileName);
        e.logoPath    = getGameDataStr(fileName, GAMEDATA_FIELD_LOGOPATH, "");
        e.lastOpen    = getGameDataStr(fileName, GAMEDATA_FIELD_LASTOPEN, LASTOPEN_UNPLAYED);
        e.totalTime   = getGameDataInt(fileName, GAMEDATA_FIELD_TOTALTIME, 0);
        e.playCount   = getGameDataInt(fileName, GAMEDATA_FIELD_PLAYCOUNT, 0);
        m_entries.push_back(std::move(e));
    }
}

void GameLibraryPage::sortEntries()
{
    switch (m_sortMode) {
    case SortMode::ByLastOpen:
        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameLibraryEntry& a, const GameLibraryEntry& b) {
                bool aUnplayed = (a.lastOpen == LASTOPEN_UNPLAYED || a.lastOpen.empty());
                bool bUnplayed = (b.lastOpen == LASTOPEN_UNPLAYED || b.lastOpen.empty());
                if (aUnplayed != bUnplayed) return !aUnplayed;
                return a.lastOpen > b.lastOpen;
            });
        break;
    case SortMode::ByTotalTime:
        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameLibraryEntry& a, const GameLibraryEntry& b) {
                return a.totalTime > b.totalTime;
            });
        break;
    case SortMode::ByName:
        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameLibraryEntry& a, const GameLibraryEntry& b) {
                return a.displayName < b.displayName;
            });
        break;
    }
}

void GameLibraryPage::rebuildList()
{
    m_itemsBox->clearViews(/*free=*/true);
    m_scroll->setContentOffsetY(0, /*animated=*/false);

    if (m_entries.empty()) {
        clearDetailPanel();
        auto* lbl = new brls::Label();
        lbl->setText("beiklive/library/empty"_i18n);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        lbl->setMarginTop(40.f);
        m_itemsBox->addView(lbl);
        return;
    }

    GameLibraryListItem* firstItem = nullptr;
    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        auto* item = new GameLibraryListItem(m_entries[i], i);

        item->onActivated = [this](int idx) {
            if (idx >= 0 && idx < static_cast<int>(m_entries.size()))
                onItemActivated(m_entries[idx]);
        };
        item->onOptions = [this](int idx) {
            if (idx >= 0 && idx < static_cast<int>(m_entries.size()))
                onItemOptions(m_entries[idx]);
        };
        item->onFocused = [this](int idx) {
            if (idx >= 0 && idx < static_cast<int>(m_entries.size())) {
                updateDetailPanel(m_entries[idx]);
                updateHeaderFocused(m_entries[idx]);
            }
        };

        m_itemsBox->addView(item);
        if (i == 0) firstItem = item;
    }

    // 重建后将焦点移到第一个元素，并更新详情面板和标题栏
    if (firstItem) {
        brls::Application::giveFocus(firstItem);
        updateDetailPanel(m_entries[0]);
        updateHeaderFocused(m_entries[0]);
    }
}

void GameLibraryPage::updateHeader()
{
    int count = static_cast<int>(m_entries.size());
    m_header->setInfo(std::to_string(count) + " "
        + (count == 1 ? "beiklive/library/game_singular"_i18n
                      : "beiklive/library/game_plural"_i18n));
}

void GameLibraryPage::updateHeaderFocused(const GameLibraryEntry& entry)
{
    // 在标题栏路径区实时显示当前聚焦的游戏名
    m_header->setPath(entry.displayName.empty() ? entry.fileName : entry.displayName);
}

void GameLibraryPage::showSortDropdown()
{
    std::vector<std::string> labels = {
        "beiklive/library/sort_lastopen"_i18n,
        "beiklive/library/sort_totaltime"_i18n,
        "beiklive/library/sort_name"_i18n,
    };

    int currentSel = static_cast<int>(m_sortMode);

    auto* dropdown = new brls::Dropdown(
        "beiklive/library/sort"_i18n,
        labels,
        [](int) {},
        currentSel,
        [this](int sel) {
            if (sel < 0 || sel > 2) return;
            m_sortMode = static_cast<SortMode>(sel);
            sortEntries();
            rebuildList();
            updateHeader();
        });
    brls::Application::pushActivity(new brls::Activity(dropdown));
}

void GameLibraryPage::onItemActivated(const GameLibraryEntry& entry)
{
    if (onGameSelected)
        onGameSelected(entry);
}

void GameLibraryPage::onItemOptions(const GameLibraryEntry& entry)
{
    if (onGameOptions) {
        onGameOptions(entry);
        return;
    }

    struct Option {
        std::string label;
        std::function<void()> action;
    };
    std::vector<Option> opts;

    // 设置显示名
    opts.push_back({"beiklive/sidebar/set_mapping"_i18n, [entry, this]() {
        std::string key = gamedataKeyPrefix(entry.fileName);
        std::string currentMapped;
        if (NameMappingManager) {
            auto mv = NameMappingManager->Get(key);
            if (mv && mv->AsString() && !mv->AsString()->empty())
                currentMapped = *mv->AsString();
        }
        brls::Application::getPlatform()->getImeManager()->openForText(
            [key, entry, this](const std::string& mapped) {
                if (!NameMappingManager) return;
                if (mapped.empty())
                    NameMappingManager->Remove(key);
                else
                    NameMappingManager->Set(key, mapped);
                NameMappingManager->Save();
                std::string newTitle = mapped.empty()
                    ? fs::path(entry.fileName).stem().string()
                    : mapped;
                for (auto& e : m_entries) {
                    if (e.fileName == entry.fileName) {
                        e.displayName = newTitle;
                        break; // fileName 唯一
                    }
                }
                for (auto* child : m_itemsBox->getChildren()) {
                    auto* item = dynamic_cast<GameLibraryListItem*>(child);
                    if (item && item->getEntry().fileName == entry.fileName) {
                        item->updateTitle(newTitle);
                        break; // fileName 唯一，找到后无需继续遍历
                    }
                }
            },
            "beiklive/sidebar/set_mapping"_i18n,
            "",
            128,
            currentMapped);
    }});

    // 选择封面
    opts.push_back({"beiklive/sidebar/select_logo"_i18n, [entry, this]() {
        std::string startPath = beiklive::file::getParentPath(entry.gamePath);
        if (startPath.empty() ||
            beiklive::file::getPathType(startPath) != beiklive::file::PathType::Directory) {
            startPath = "/";
#ifdef _WIN32
            startPath = "C:\\";
#endif
        }
        auto* flPage = new FileListPage();
        flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
        flPage->setLayoutMode(FileListPage::LayoutMode::ListOnly);
        flPage->setDefaultFileCallback([entry, this](const FileListItem& imgItem) {
            setGameDataStr(entry.fileName, GAMEDATA_FIELD_LOGOPATH, imgItem.fullPath);
            for (auto& e : m_entries) {
                if (e.fileName == entry.fileName) {
                    e.logoPath = imgItem.fullPath;
                    break; // fileName 唯一
                }
            }
            for (auto* child : m_itemsBox->getChildren()) {
                auto* item = dynamic_cast<GameLibraryListItem*>(child);
                if (item && item->getEntry().fileName == entry.fileName) {
                    item->updateCover(imgItem.fullPath);
                    break; // fileName 唯一，找到后无需继续遍历
                }
            }
            brls::sync([]() { brls::Application::popActivity(); });
        });
        flPage->navigateTo(startPath);

        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setGrow(1.0f);
        container->addView(flPage);
        container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                                  [](brls::View*) {
                                      brls::Application::popActivity();
                                      return true;
                                  });
        auto* frame = new brls::AppletFrame(container);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
    }});

    // 从游戏库移除
    opts.push_back({"beiklive/library/remove_from_library"_i18n, [entry, this]() {
        setGameDataStr(entry.fileName, GAMEDATA_FIELD_GAMEPATH, "");
        removeRecentGame(entry.fileName);
        m_entries.erase(
            std::remove_if(m_entries.begin(), m_entries.end(),
                [&entry](const GameLibraryEntry& e) { return e.fileName == entry.fileName; }),
            m_entries.end());
        rebuildList();
        updateHeader();
    }});

    std::vector<std::string> labels;
    for (const auto& o : opts)
        labels.push_back(o.label);

    std::string title = entry.displayName.empty()
        ? fs::path(entry.fileName).stem().string()
        : entry.displayName;

    auto* dropdown = new brls::Dropdown(
        title,
        labels,
        [](int) {},
        -1,
        [opts](int sel) {
            if (sel >= 0 && sel < static_cast<int>(opts.size()))
                opts[sel].action();
        });
    brls::Application::pushActivity(new brls::Activity(dropdown));
}
