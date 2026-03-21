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

// ─────────────────────────────────────────────────────────────────────────────
//  布局常量
// ─────────────────────────────────────────────────────────────────────────────
static constexpr int   GRID_COLS     = 6;      ///< 每行列数
static constexpr float ITEM_W        = 190.f;  ///< 每个元素宽度
static constexpr float ITEM_H        = 210.f;  ///< 每个元素高度（图片 + 标签）
static constexpr float ITEM_IMG_SZ   = 180.f;  ///< 封面图尺寸
static constexpr float ITEM_MARGIN   = 3.f;   ///< 元素间距
static constexpr float ITEM_LABEL_H  = 40.f;   ///< 标题标签区域高度
static constexpr float ITEM_LABEL_FS = 18.f;   ///< 标题标签字号

/// gamedataManager 中 lastopen 字段的"从未游玩"占位值（与 common.hpp initGameData 保持一致）
static const std::string LASTOPEN_UNPLAYED = "从未游玩";

// ─────────────────────────────────────────────────────────────────────────────
//  辅助函数
// ─────────────────────────────────────────────────────────────────────────────

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

// ─────────────────────────────────────────────────────────────────────────────
//  GameLibraryItem
// ─────────────────────────────────────────────────────────────────────────────

GameLibraryItem::GameLibraryItem(const GameLibraryEntry& entry)
    : brls::Box(brls::Axis::COLUMN)
    , m_entry(entry)
{
    setAlignItems(brls::AlignItems::CENTER);
    setWidth(ITEM_W);
    setHeight(ITEM_H);
    setMarginRight(ITEM_MARGIN);
    setMarginBottom(ITEM_MARGIN);
    setHideHighlightBackground(true);
    setHideClickAnimation(true);

    // ── 封面图（ProImage 异步加载） ──
    m_coverImage = new beiklive::UI::ProImage();
    m_coverImage->setWidth(ITEM_IMG_SZ);
    m_coverImage->setHeight(ITEM_IMG_SZ);
    m_coverImage->setCornerRadius(8.f);
    m_coverImage->setScalingType(brls::ImageScalingType::FIT);
    m_coverImage->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_coverImage->setBackgroundColor(nvgRGBA(31, 31, 31, 50));
    m_coverImage->setHideHighlightBackground(true);
    m_coverImage->setShadowVisibility(true);
    m_coverImage->setShadowType(brls::ShadowType::GENERIC);
    m_coverImage->setHighlightCornerRadius(10.f);
    m_coverImage->setFocusable(true);

    if (!entry.logoPath.empty()) {
        m_coverImage->setImageFromFileAsync(entry.logoPath);
    } else {
        m_coverImage->setImageFromFile(BK_APP_DEFAULT_LOGO);
    }
    addView(m_coverImage);

    // ── 标题标签（默认隐藏，焦点时显示） ──
    m_label = new brls::Label();
    m_label->setText(entry.displayName.empty() ? "—" : entry.displayName);
    m_label->setFontSize(ITEM_LABEL_FS);
    m_label->setSingleLine(true);
    m_label->setAutoAnimate(true);
    m_label->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_label->setTextColor(GET_THEME_COLOR("brls/text"));
    m_label->setWidth(ITEM_W - 8.f);
    m_label->setHeight(ITEM_LABEL_H);
    m_label->setMarginTop(3.f);
    m_label->setVisibility(brls::Visibility::INVISIBLE);
    addView(m_label);

    // ── 按键注册 ──
    beiklive::swallow(this, brls::BUTTON_A);
    beiklive::swallow(this, brls::BUTTON_X);

    // A 键：触发点击弹性动画，动画结束后回调 onActivated
    registerAction("beiklive/hints/start"_i18n, brls::BUTTON_A, [this](brls::View*) {
        triggerClickBounce();
        return true;
    }, false, false, brls::SOUND_CLICK);

    // X 键：选项菜单
    registerAction("beiklive/hints/set"_i18n, brls::BUTTON_X, [this](brls::View*) {
        if (onOptions) onOptions(m_entry);
        return true;
    });

    addGestureRecognizer(new brls::TapGestureRecognizer(this));
}

void GameLibraryItem::triggerClickBounce()
{
    m_clickAnimating = true;
    m_clickT         = 0.0f;
    m_clickScale     = 1.0f;
    invalidate();
}

void GameLibraryItem::onChildFocusGained(brls::View* directChild, brls::View* focusedView)
{
    Box::onChildFocusGained(directChild, focusedView);
    if (m_label) m_label->setVisibility(brls::Visibility::VISIBLE);
    m_focused = true;
    invalidate(); // 触发 draw() 启动缩放动画
    if (onFocused) onFocused(m_entry); // 通知详情面板更新
}

void GameLibraryItem::onChildFocusLost(brls::View* directChild, brls::View* focusedView)
{
    Box::onChildFocusLost(directChild, focusedView);
    if (m_label) m_label->setVisibility(brls::Visibility::INVISIBLE);
    m_focused = false;
    invalidate(); // 触发 draw() 启动恢复动画
}

void GameLibraryItem::draw(NVGcontext* vg,
                           float x, float y, float w, float h,
                           brls::Style style, brls::FrameContext* ctx)
{
    // 聚焦缩放动画：聚焦时 1.0，失焦时 0.9，平滑插值
    float target = m_focused ? 1.0f : 0.9f;
    float delta  = target - m_scale;
    if (std::abs(delta) > 0.002f) {
        m_scale += delta * 0.3f;
        invalidate();
    } else {
        m_scale = target;
    }

    // 点击弹性动画（先线性压缩，再指数衰减阻尼回弹）
    if (m_clickAnimating) {
        m_clickT += 1.0f / 60.0f; // 近似按 60fps 推进
        if (m_clickT < 0.06f) {
            float t      = m_clickT / 0.06f;        // 归一化进度 0~1
            m_clickScale = 1.0f - 0.10f * t;        // 线性压缩到 0.90
        } else {
            float u      = m_clickT - 0.06f;
            m_clickScale = 1.0f + 0.12f * std::exp(-14.0f * u) * std::sin(45.0f * u);
            // 动画结束判定
            if (u > 0.28f && std::abs(m_clickScale - 1.0f) < 0.003f) {
                m_clickScale     = 1.0f;
                m_clickAnimating = false;
                if (onActivated) onActivated(m_entry); // 动画完成后触发回调
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

void GameLibraryItem::updateCover(const std::string& newLogoPath)
{
    m_entry.logoPath = newLogoPath;
    if (!newLogoPath.empty())
        m_coverImage->setImageFromFileAsync(newLogoPath);
    else
        m_coverImage->setImageFromFile(BK_APP_DEFAULT_LOGO);
}

void GameLibraryItem::updateTitle(const std::string& newTitle)
{
    m_entry.displayName = newTitle;
    if (m_label) m_label->setText(newTitle.empty() ? "—" : newTitle);
}

// ─────────────────────────────────────────────────────────────────────────────
//  GameLibraryPage
// ─────────────────────────────────────────────────────────────────────────────

/// 详情面板固定宽度（逻辑像素）
static constexpr float LIB_DETAIL_PANEL_W  = 320.f;
/// 详情面板缩略图尺寸
static constexpr float LIB_DETAIL_THUMB_SZ = 260.f;

GameLibraryPage::GameLibraryPage()
{
    setAxis(brls::Axis::COLUMN);
    setGrow(1.0f);

    // ── 标题栏 ────────────────────────────────────────────────────────────────
    m_header = new beiklive::UI::BrowserHeader();
    m_header->setTitle("beiklive/library/title"_i18n);
    addView(m_header);

    // ── 主体：横向内容区（网格 + 右侧详情面板） ──────────────────────────────
    auto* contentBox = new brls::Box(brls::Axis::ROW);
    contentBox->setGrow(1.0f);
    contentBox->setWidth(brls::View::AUTO);

    // 网格滚动区（自适应宽度）
    m_scroll = new brls::ScrollingFrame();
    m_scroll->setGrow(1.0f);
    m_scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    m_scroll->setScrollingIndicatorVisible(false);

    m_gridBox = new brls::Box(brls::Axis::COLUMN);
    m_gridBox->setPadding(
        GET_STYLE("brls/applet_frame/header_padding_top_bottom"),
        GET_STYLE("brls/applet_frame/header_padding_sides"),
        GET_STYLE("brls/applet_frame/header_padding_top_bottom"),
        GET_STYLE("brls/applet_frame/header_padding_sides")
    );
    m_scroll->setContentView(m_gridBox);
    contentBox->addView(m_scroll);

    // 右侧详情面板（固定宽度，常驻显示）
    buildDetailPanel();
    contentBox->addView(m_detailPanel);

    addView(contentBox);

    // ── 底栏 ─────────────────────────────────────────────────────────────────
    addView(new brls::BottomBar());

    // ── Y 键：弹出排序菜单 ──────────────────────────────────────────────────
    registerAction("beiklive/library/sort"_i18n, brls::BUTTON_Y, [this](brls::View*) {
        showSortDropdown();
        return true;
    });

    // ── 加载并显示游戏数据 ──────────────────────────────────────────────────
    loadEntries();
    sortEntries();
    rebuildGrid();
    updateHeader();
}

void GameLibraryPage::buildDetailPanel()
{
    m_detailPanel = new brls::Box(brls::Axis::COLUMN);
    m_detailPanel->setAlignItems(brls::AlignItems::CENTER);
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

    // 显示名称
    m_detailName->setText(entry.displayName.empty() ? "—" : entry.displayName);

    // 文件名
    m_detailFileName->setText(entry.fileName);

    // 上次游玩时间
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

    // 平台名称
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
        // "从未游玩" 排最后，其余按字典序降序（YYYY-MM-DD HH:MM 格式可直接比较）
        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameLibraryEntry& a, const GameLibraryEntry& b) {
        // LASTOPEN_UNPLAYED 与 common.hpp initGameData 中初始化的值保持一致
                bool aUnplayed = (a.lastOpen == LASTOPEN_UNPLAYED || a.lastOpen.empty());
                bool bUnplayed = (b.lastOpen == LASTOPEN_UNPLAYED || b.lastOpen.empty());
                if (aUnplayed != bUnplayed) return !aUnplayed; // 未玩过的排后
                return a.lastOpen > b.lastOpen; // 时间字符串降序
            });
        break;
    case SortMode::ByTotalTime:
        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameLibraryEntry& a, const GameLibraryEntry& b) {
                return a.totalTime > b.totalTime; // 时长降序
            });
        break;
    case SortMode::ByName:
        std::sort(m_entries.begin(), m_entries.end(),
            [](const GameLibraryEntry& a, const GameLibraryEntry& b) {
                return a.displayName < b.displayName; // 名称升序
            });
        break;
    }
}

void GameLibraryPage::rebuildGrid()
{
    m_gridBox->clearViews(/*free=*/true);
    m_scroll->setContentOffsetY(0, /*animated=*/false);

    if (m_entries.empty()) {
        clearDetailPanel();
        auto* lbl = new brls::Label();
        lbl->setText("beiklive/library/empty"_i18n);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);

        lbl->setMarginTop(40.f);
        m_gridBox->addView(lbl);
        return;
    }
    m_gridBox->setMarginLeft(25.f); // 左侧额外内边距，视觉上让第一列和左边界有点距离
    GameLibraryItem* firstItem = nullptr;
    brls::Box* rowBox = nullptr;

    for (int i = 0; i < static_cast<int>(m_entries.size()); ++i) {
        // 每 GRID_COLS 个元素新建一行
        if (i % GRID_COLS == 0) {
            rowBox = new brls::Box(brls::Axis::ROW);
            rowBox->setAlignItems(brls::AlignItems::CENTER);
            rowBox->setJustifyContent(brls::JustifyContent::FLEX_START);
            m_gridBox->addView(rowBox);
        }

        const auto& entry = m_entries[i];
        auto* item = new GameLibraryItem(entry);

        item->onActivated = [this](const GameLibraryEntry& e) { onItemActivated(e); };
        item->onOptions   = [this](const GameLibraryEntry& e) { onItemOptions(e); };
        item->onFocused   = [this](const GameLibraryEntry& e) { updateDetailPanel(e); };

        rowBox->addView(item);
        if (i == 0) firstItem = item;
    }

    // 重建后将焦点移到第一个元素，并更新详情面板
    if (firstItem) {
        brls::View* focus = firstItem->getDefaultFocus();
        if (!focus) focus = firstItem;
        brls::Application::giveFocus(focus);
        updateDetailPanel(m_entries[0]);
    }
}

void GameLibraryPage::updateHeader()
{
    int count = static_cast<int>(m_entries.size());
    m_header->setInfo(std::to_string(count) + " "
        + (count == 1 ? "beiklive/library/game_singular"_i18n
                      : "beiklive/library/game_plural"_i18n));
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
            rebuildGrid();
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
    // 若外部设置了回调，则由外部处理；否则走内部默认菜单
    if (onGameOptions) {
        onGameOptions(entry);
        return;
    }

    // ── 默认选项菜单（与 AppPage GameCard 相同结构） ──────────────────────
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
                // 更新当前网格中对应条目的标题
                std::string newTitle = mapped.empty()
                    ? fs::path(entry.fileName).stem().string()
                    : mapped;
                // 同步更新 m_entries 中的 displayName
                for (auto& e : m_entries) {
                    if (e.fileName == entry.fileName)
                        e.displayName = newTitle;
                }
                // 更新网格中已存在的 GameLibraryItem
                for (auto* rowView : m_gridBox->getChildren()) {
                    auto* row = dynamic_cast<brls::Box*>(rowView);
                    if (!row) continue;
                    for (auto* child : row->getChildren()) {
                        auto* item = dynamic_cast<GameLibraryItem*>(child);
                        if (item && item->getEntry().fileName == entry.fileName) {
                            item->updateTitle(newTitle);
                        }
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
            // 更新内存中的 logoPath
            for (auto& e : m_entries) {
                if (e.fileName == entry.fileName)
                    e.logoPath = imgItem.fullPath;
            }
            // 更新网格中对应 GameLibraryItem 的封面
            for (auto* rowView : m_gridBox->getChildren()) {
                auto* row = dynamic_cast<brls::Box*>(rowView);
                if (!row) continue;
                for (auto* child : row->getChildren()) {
                    auto* item = dynamic_cast<GameLibraryItem*>(child);
                    if (item && item->getEntry().fileName == entry.fileName) {
                        item->updateCover(imgItem.fullPath);
                    }
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

    // 从游戏库移除（清除 gamedataManager 中的 gamepath，并从当前列表移除）
    opts.push_back({"beiklive/library/remove_from_library"_i18n, [entry, this]() {
        // 清除 gamepath 字段（保留其他字段）
        setGameDataStr(entry.fileName, GAMEDATA_FIELD_GAMEPATH, "");
        // 同时从最近游戏队列中移除
        removeRecentGame(entry.fileName);
        // 从内存列表中移除并重建网格
        m_entries.erase(
            std::remove_if(m_entries.begin(), m_entries.end(),
                [&entry](const GameLibraryEntry& e) { return e.fileName == entry.fileName; }),
            m_entries.end());
        rebuildGrid();
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
