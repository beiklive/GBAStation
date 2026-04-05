#include "UI/Pages/FileListPage.hpp"

#include <filesystem>
#include <iomanip>
#include <sstream>

#include "Utils/fileUtils.hpp"
#include "Utils/strUtils.hpp"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace brls::literals; // for _i18n

namespace fs = std::filesystem;

static constexpr float CELL_HEIGHT = 66.f;
static constexpr float ICON_HEIGHT = 50.f;
static constexpr float ACCENT_W = 4.f;
static constexpr float CELL_PAD_H = 12.f;
static constexpr float CELL_PAD_V = 10.f;
static constexpr float INFO_FONT_SIZE = 12.f;
static constexpr float NAME_FONT_SIZE = 20.f;
static constexpr float DETAIL_PANEL_WIDTH = 400.f;
static constexpr float DETAIL_THUMB_SZ = DETAIL_PANEL_WIDTH - 100.f; // 面板宽度减去水平内边距

// ─────────────────────────────────────────────────────────────────────────────
//  文件类型扩展名表
// ─────────────────────────────────────────────────────────────────────────────

/// GB / GBC ROM 扩展名
static const std::vector<std::string> k_gbExtensions = {"gb", "gbc"};
/// GBA ROM 扩展名
static const std::vector<std::string> k_gbaExtensions = {"gba"};
/// 常见图片扩展名
static const std::vector<std::string> k_imageExtensions = {
    "png", "jpg", "jpeg", "bmp", "webp", "tga"};
/// 常见压缩包扩展名
static const std::vector<std::string> k_zipExtensions = {
    "zip", "rar", "7z", "gz", "tar", "bz2", "xz", "zst"};

// ─────────────────────────────────────────────────────────────────────────────
//  辅助函数
// ─────────────────────────────────────────────────────────────────────────────

/// 返回代表 item 的图标资源路径，根据 theme 选择 _light/_dark 变体
static std::string getFileIconPath(const FileListItem &item, brls::ThemeVariant theme)
{
    // const std::string variant = (theme == brls::ThemeVariant::LIGHT) ? "_light" : "_dark";
    std::string path_prefix = "img/ui/" + 
    std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");

    if (item.isDir)
        return BK_RES(path_prefix + "wenjianjia.png");

    const std::string suffix = beiklive::string::getFileSuffix(item.fileName);

    auto matchesSuffix = [&suffix](const std::vector<std::string> &exts)
    {
        for (const auto &ext : exts)
            if (beiklive::string::iequals(ext, suffix))
                return true;
        return false;
    };

    if (matchesSuffix(k_gbExtensions))
        return BK_RES(path_prefix + "icon_gb.png");
    if (matchesSuffix(k_gbaExtensions))
        return BK_RES(path_prefix + "icon_gba.png");
    if (matchesSuffix(k_imageExtensions))
        return BK_RES(path_prefix + "tupian.png");
    if (matchesSuffix(k_zipExtensions))
        return BK_RES(path_prefix + "zip.png");

    return BK_RES(path_prefix + "wenjian.png");
}

/// 将文件大小（字节）格式化为可读字符串（KB/MB/GB）
static std::string formatFileSize(std::uintmax_t bytes)
{
    if (bytes < 1024)
    {
        return std::to_string(bytes) + " B";
    }
    else if (bytes < 1024 * 1024)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(1) << bytes / 1024.0 << " KB";
        return ss.str();
    }
    else if (bytes < static_cast<std::uintmax_t>(1024) * 1024 * 1024)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << bytes / (1024.0 * 1024.0) << " MB";
        return ss.str();
    }
    else
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2) << bytes / (1024.0 * 1024.0 * 1024.0) << " GB";
        return ss.str();
    }
}

/// 统计目录直接子项数量（无法读取时返回 0）
static int countChildren(const std::string &path)
{
    int n = 0;
    try
    {
        for (const auto &e : fs::directory_iterator(path))
        {
            (void)e;
            ++n;
        }
    }
    catch (...)
    {
        // 忽略权限错误等
    }
    return n;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Windows：枚举可用驱动器盘符（A: … Z:）
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
/// 返回可用驱动器根路径列表，如 {"C:\\", "D:\\", "F:\\"}
static std::vector<std::string> getWindowsDrives()
{
    std::vector<std::string> drives;
    DWORD mask = GetLogicalDrives();
    for (int i = 0; i < 26; ++i)
    {
        if (mask & (1u << i))
        {
            std::string drive;
            drive += static_cast<char>('A' + i);
            drive += ":\\";
            drives.push_back(drive);
        }
    }
    return drives;
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
//  FileListItemView
// ─────────────────────────────────────────────────────────────────────────────

FileListItemView::FileListItemView()
{
    setMarginTop(5.0f);
    setFocusable(true);
    setAxis(brls::Axis::ROW);
    setAlignItems(brls::AlignItems::CENTER);
    setHeight(CELL_HEIGHT);
    setWidth(brls::View::AUTO);
    setHideHighlightBackground(true);
    // setBackground(brls::ViewBackground::NONE);

    // 左侧强调色矩形
    m_accent = new brls::Rectangle();
    m_accent->setWidth(ACCENT_W);
    m_accent->setHeight(CELL_HEIGHT - 2 * CELL_PAD_V);
    m_accent->setMarginLeft(CELL_PAD_H);
    m_accent->setMarginRight(CELL_PAD_H);
    m_accent->setColor(nvgRGBA(79, 193, 255, 255));
    m_accent->setVisibility(brls::Visibility::INVISIBLE);
    addView(m_accent);

    m_icon = new brls::Image();
    m_icon->setWidth(ICON_HEIGHT);
    m_icon->setHeight(ICON_HEIGHT);
    m_icon->setMarginRight(CELL_PAD_H);
    m_icon->setScalingType(brls::ImageScalingType::FIT);
    m_icon->setInterpolation(brls::ImageInterpolation::LINEAR);
    // m_icon->setImageFromFile(BK_RES("img/file/file_light.png"));
    addView(m_icon);

    // 名称标签（自动填充剩余空间）
    m_nameLabel = new brls::Label();
    m_nameLabel->setFontSize(NAME_FONT_SIZE);
    m_nameLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_nameLabel->setSingleLine(true);
    // 聚焦且文本溢出时滚动显示（跑马灯效果）
    m_nameLabel->setAutoAnimate(true);
    m_nameLabel->setGrow(1.0f);
    m_nameLabel->setMarginRight(CELL_PAD_H);
    m_nameLabel->setMarginTop(8.f);
    addView(m_nameLabel);

    auto *box = new brls::Box(brls::Axis::COLUMN);
    box->setHeight(CELL_HEIGHT);
    // 右侧信息标签（灰色）
    m_infoLabel = new brls::Label();
    m_infoLabel->setFontSize(INFO_FONT_SIZE);
    m_infoLabel->setSingleLine(true);
    m_infoLabel->setTextColor(GET_THEME_COLOR("brls/text"));
    m_infoLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
    m_infoLabel->setMarginRight(CELL_PAD_H * 2);
    box->addView(new brls::Padding());
    box->addView(m_infoLabel);
    addView(box);


    addGestureRecognizer(new brls::TapGestureRecognizer(this));

}

void FileListItemView::setItem(const FileListItem &item, int index)
{
    m_index = index;
    m_nameLabel->setText(item.displayName());
    auto theme = brls::Application::getPlatform()->getThemeVariant();

    if (item.isDir)
    {
        std::string path_prefix = "img/ui/" + 
        std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");
        m_icon->setImageFromFile(BK_RES(path_prefix + "wenjianjia.png"));
        m_infoLabel->setText(std::to_string(item.childCount) + " items");
    }
    else
    {
        m_icon->setImageFromFile(getFileIconPath(item, theme));
        m_infoLabel->setText(formatFileSize(item.fileSize));
    }

    // A键：激活条目
    registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A, [this](brls::View *)
                   {
                       if (onItemActivated)
                           onItemActivated(m_index);
                       return true; }, false, false, brls::SOUND_CLICK);

    // X键：打开选项/侧边栏
    registerAction("beiklive/hints/operate"_i18n,
                   brls::BUTTON_X,
                   [this](brls::View *)
                   {
                       if (onItemOptions)
                           onItemOptions(m_index);
                       return true;
                   });
}

void FileListItemView::onFocusGained()
{
    brls::Box::onFocusGained();
    m_accent->setVisibility(brls::Visibility::VISIBLE);
    if (onItemFocused)
        onItemFocused(m_index);
}

void FileListItemView::onFocusLost()
{
    brls::Box::onFocusLost();
    m_accent->setVisibility(brls::Visibility::INVISIBLE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  FileListPage
// ─────────────────────────────────────────────────────────────────────────────

FileListPage::FileListPage()
{
    bklog::debug("FileListPage: constructing");
    setAxis(brls::Axis::COLUMN);
    setWidth(brls::View::AUTO);
    setHeight(brls::View::AUTO);
    setGrow(1.0f);

    // beiklive::InsertBackground(this);

    buildUI();

    const std::string key = "UI.fileListLayoutMode";
    SettingManager->SetDefault(key, static_cast<int>(LayoutMode::ListOnly));

    // 初始化 logo 加载模式默认值（1=聚焦时加载）
    SettingManager->SetDefault("UI.logoLoadMode", 1);

    int layoutModeInt = *SettingManager->Get(key)->AsInt();
    bklog::debug("FileListPage: layout mode = {}", layoutModeInt);
    setLayoutMode(static_cast<LayoutMode>(layoutModeInt));

    // B键：向上导航
    registerAction("beiklive/hints/UP"_i18n,
                   brls::BUTTON_B,
                   [this](brls::View *)
                   {
                       bklog::debug("FileListPage: BUTTON_B pressed, navigating up");
                       navigateUp();
                       return true;
                   });

    // ZR（BUTTON_RT）：切换详情面板
    registerAction("beiklive/hints/toggle_detail"_i18n,
                   brls::BUTTON_RT,
                   [this](brls::View *)
                   {
                       LayoutMode newMode = (m_layoutMode == LayoutMode::ListOnly)
                           ? LayoutMode::ListAndDetail
                           : LayoutMode::ListOnly;
                       bklog::debug("FileListPage: toggling layout mode to {}", static_cast<int>(newMode));
                       setLayoutMode(newMode);
                       if (SettingManager) {
                           SettingManager->Set("UI.fileListLayoutMode",
                               beiklive::ConfigValue(static_cast<int>(newMode)));
                           SettingManager->Save();
                       }
                       return true;
                   },
                   false, false, brls::SOUND_CLICK);
    bklog::debug("FileListPage: construction complete");
}

FileListPage::~FileListPage()
{
    bklog::debug("FileListPage destroyed.");
}

// ─────────── buildUI ─────────────────────────────────────────────────────────

void FileListPage::buildUI()
{
    // ── 头部 ──────────────────────────────────────────────────────────────
    m_header = new beiklive::UI::BrowserHeader();
    m_header->setTitle("beiklive/file/file_select"_i18n);
    m_header->setPath("/");
    m_header->setInfo("");
    addView(m_header);

    // ── 内容行（列表 + 可选详情面板） ────────────────────────────────────────
    m_contentBox = new brls::Box(brls::Axis::ROW);
    m_contentBox->setGrow(1.0f);
    m_contentBox->setWidth(brls::View::AUTO);
    m_contentBox->setPaddingTop(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
    m_contentBox->setPaddingBottom(GET_STYLE("brls/applet_frame/header_padding_top_bottom"));
    m_contentBox->setMarginRight(GET_STYLE("brls/applet_frame/padding_sides"));
    m_contentBox->setMarginLeft(GET_STYLE("brls/applet_frame/padding_sides"));

    m_contentBox->setPaddingRight(GET_STYLE("brls/applet_frame/header_padding_sides"));
    m_contentBox->setPaddingLeft(GET_STYLE("brls/applet_frame/header_padding_sides"));

    addView(m_contentBox);

    // 列表容器
    m_listBox = new brls::Box(brls::Axis::COLUMN);
    m_listBox->setWidth(brls::View::AUTO);
    m_listBox->setGrow(1.0f);
    m_contentBox->addView(m_listBox);

    // 用 ScrollingFrame 替代 RecyclerFrame。
    // 条目作为 m_itemsBox 的直接子节点管理（无回收），
    // 焦点管理简单可靠，不受 RecyclerFrame LIFO 复用顺序缺陷影响。
    m_scrollFrame = new brls::ScrollingFrame();
    m_scrollFrame->setWidth(brls::View::AUTO);
    m_scrollFrame->setGrow(1.0f);
    m_scrollFrame->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    m_scrollFrame->setScrollingIndicatorVisible(false);

    m_itemsBox = new brls::Box(brls::Axis::COLUMN);
    m_itemsBox->setWidth(brls::View::AUTO);
    m_scrollFrame->setContentView(m_itemsBox);

    m_listBox->addView(m_scrollFrame);

    // 条目直接在 m_items 中管理，无需 DataSource

    // ── 详情面板（初始隐藏） ──────────────────────────────────────────────
    buildDetailPanel();
    m_detailPanel->setMarginLeft(20.f);
    m_detailPanel->setVisibility(brls::Visibility::GONE);
    m_contentBox->addView(m_detailPanel);

    // ── 底栏 ──────────────────────────────────────────────────────────────
    m_bottomBar = new brls::BottomBar();
    addView(m_bottomBar);
}

void FileListPage::buildDetailPanel()
{
    m_detailPanel = new brls::Box(brls::Axis::COLUMN);
    m_detailPanel->setAlignItems(brls::AlignItems::CENTER);
    m_detailPanel->setJustifyContent(brls::JustifyContent::FLEX_START);
    m_detailPanel->setWidth(DETAIL_PANEL_WIDTH);
    m_detailPanel->setPadding(20.f, 12.f, 20.f, 12.f);
    m_detailPanel->setBackgroundColor(nvgRGBA(40, 40, 40, 20));
    // 宽度在 setLayoutMode() 中设为约 33%

    // 使用 ProImage 异步加载 PNG。
    // setClipsToBounds(false) 修复边缘像素拉伸问题：图像只在精确的宽高比矩形内绘制，
    // 不会因边缘像素钳制而渗入周围空白区域。
    m_detailThumb = new beiklive::UI::ProImage();
    m_detailThumb->setWidth(DETAIL_THUMB_SZ);
    m_detailThumb->setHeight(DETAIL_THUMB_SZ);
    m_detailThumb->setScalingType(brls::ImageScalingType::FIT);
    m_detailThumb->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_detailThumb->setCornerRadius(8.f);
    m_detailThumb->setClipsToBounds(false); // 防止边缘像素拉伸
    m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
    m_detailPanel->addView(m_detailThumb);

    m_detailName = new brls::Label();
    m_detailName->setFontSize(22.f);
    m_detailName->setWidth(DETAIL_PANEL_WIDTH - 40.f); // 减去水平内边距
    m_detailName->setMarginTop(12.f);
    m_detailName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailName->setSingleLine(true);
    m_detailName->setAutoAnimate(true);
    m_detailName->setTextColor(nvgRGBA(220, 220, 220, 255));
    m_detailName->setGrow(1.0f);
    m_detailName->setAutoAnimate(true);
    m_detailPanel->addView(m_detailName);

    m_detailMappedName = new brls::Label();
    m_detailMappedName->setWidth(DETAIL_PANEL_WIDTH - 40.f); // 减去水平内边距

    m_detailMappedName->setFontSize(22.f);
    m_detailMappedName->setMarginTop(6.f);
    m_detailMappedName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailMappedName->setSingleLine(true);
    m_detailMappedName->setAutoAnimate(true);
    m_detailMappedName->setTextColor(nvgRGBA(120, 190, 255, 255));
    m_detailMappedName->setGrow(1.0f);
    m_detailMappedName->setAutoAnimate(true);

    m_detailPanel->addView(m_detailMappedName);

    m_detailInfo = new brls::Label();
    m_detailInfo->setFontSize(20.f);
    m_detailInfo->setMarginTop(8.f);
    m_detailInfo->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailInfo->setTextColor(nvgRGBA(160, 160, 160, 255));
    m_detailInfo->setGrow(1.0f);
    m_detailPanel->addView(m_detailInfo);

    // ── Game-data extra labels ────────────────────────────────────────────
    m_detailLastOpen = new brls::Label();
    m_detailLastOpen->setFontSize(16.f);
    m_detailLastOpen->setMarginTop(6.f);
    m_detailLastOpen->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailLastOpen->setTextColor(nvgRGBA(140, 200, 140, 255));
    m_detailLastOpen->setGrow(1.0f);
    m_detailPanel->addView(m_detailLastOpen);

    m_detailTotalTime = new brls::Label();
    m_detailTotalTime->setFontSize(16.f);
    m_detailTotalTime->setMarginTop(4.f);
    m_detailTotalTime->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailTotalTime->setTextColor(nvgRGBA(200, 200, 140, 255));
    m_detailTotalTime->setGrow(1.0f);
    m_detailPanel->addView(m_detailTotalTime);

    m_detailPlatform = new brls::Label();
    m_detailPlatform->setFontSize(16.f);
    m_detailPlatform->setMarginTop(4.f);
    m_detailPlatform->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailPlatform->setTextColor(nvgRGBA(140, 160, 220, 255));
    m_detailPlatform->setGrow(1.0f);
    m_detailPanel->addView(m_detailPlatform); 
}

// ─────────── 公共 API ─────────────────────────────────────────────────────────

void FileListPage::navigateTo(const std::string &path)
{
    bklog::debug("FileListPage::navigateTo: '{}'", path);
    refreshList(path);
}

void FileListPage::resetFocusToTop()
{
    const auto &children = m_itemsBox->getChildren();
    if (children.empty())
        return;

    m_scrollFrame->setContentOffsetY(0, /*animated=*/false);
    brls::Application::giveFocus(children[0]);
}

void FileListPage::rebuildItemViews()
{
    bklog::debug("FileListPage::rebuildItemViews: {} items", static_cast<int>(m_items.size()));
    // clearViews(true) 将旧视图加入 borealis 的延迟删除池，
    // 当前帧结束前 Application::currentFocus 仍为有效指针。
    // 立即将焦点转给第一个新条目，确保旧视图释放前焦点安全。
    m_itemsBox->clearViews(/*free=*/true);
    m_scrollFrame->setContentOffsetY(0, /*animated=*/false);

    FileListItemView *firstItem = nullptr;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
    {
        auto *itemView = new FileListItemView();
        itemView->setItem(m_items[i], i);

        itemView->onItemFocused = [this](int idx)
        { onItemFocused(idx); };
        itemView->onItemActivated = [this](int idx)
        { onItemActivated(idx); };
        itemView->onItemOptions = [this](int idx)
        { openSidebar(idx); };

        m_itemsBox->addView(itemView);
        if (i == 0)
            firstItem = itemView;
    }

    if (firstItem) {
        bklog::debug("FileListPage::rebuildItemViews: giving focus to first item");
        brls::Application::giveFocus(firstItem);
    }
}

void FileListPage::setFilter(const std::vector<std::string> &suffixes, FilterMode mode)
{
    m_filterSuffixes = suffixes;
    m_filterMode = mode;
    m_filterEnabled = !suffixes.empty();
}

void FileListPage::setFilterEnabled(bool enabled)
{
    m_filterEnabled = enabled;
}

void FileListPage::setFileCallback(const std::string &suffix,
                                   std::function<void(const FileListItem &)> cb)
{
    m_fileCallbacks[suffix] = std::move(cb);
}

void FileListPage::setDefaultFileCallback(std::function<void(const FileListItem &)> cb)
{
    m_defaultFileCallback = std::move(cb);
}

void FileListPage::setLayoutMode(LayoutMode mode)
{
    m_layoutMode = mode;

    if (m_layoutMode == LayoutMode::ListOnly)
    {
        // 列表占满宽度；隐藏详情面板
        // m_listBox->setWidthPercentage(100.f);
        m_detailPanel->setVisibility(brls::Visibility::GONE);
    }
    else // ListAndDetail
    {
        // 列表占约 2/3；详情面板约 1/3
        // m_listBox->setWidthPercentage(65.f);
        // m_detailPanel->setWidthPercentage(30.f);
        m_detailPanel->setVisibility(brls::Visibility::VISIBLE);
    }
}

// ─────────── 内部辅助函数 ────────────────────────────────────────────────────

std::string FileListPage::lookupMappedName(const std::string &name, bool isDir) const
{
    if (!NameMappingManager)
        return {};

    // key：文件取无扩展名的文件名，目录取目录名
    std::string key;
    if (isDir)
    {
        key = name;
    }
    else
    {
        auto dot = name.rfind('.');
        key = (dot != std::string::npos) ? name.substr(0, dot) : name;
    }

    if (auto val = NameMappingManager->Get(key); val && val->AsString())
        return *val->AsString();
    return {};
}

std::string FileListPage::lookupLogoPath(const std::string &name) const
{
    if (!gamedataManager)
        return {};

    // key 格式：filename_without_suffix.logopath
    std::string key = gamedataKeyPrefix(name) + "." + GAMEDATA_FIELD_LOGOPATH;
    if (auto val = gamedataManager->Get(key); val && val->AsString())
        return *val->AsString();
    return {};
}

beiklive::EmuPlatform FileListPage::detectPlatform(const std::string &fileName)
{
    const std::string suffix = beiklive::string::getFileSuffix(fileName);
    for (const auto &ext : k_gbExtensions)
        if (beiklive::string::iequals(ext, suffix)) return beiklive::EmuPlatform::GB;
    for (const auto &ext : k_gbaExtensions)
        if (beiklive::string::iequals(ext, suffix)) return beiklive::EmuPlatform::GBA;
    return beiklive::EmuPlatform::None;
}

bool FileListPage::passesFilter(const std::string &suffix) const
{
    if (!m_filterEnabled || m_filterSuffixes.empty())
        return true;

    bool found = false;
    for (const auto &s : m_filterSuffixes)
    {
        if (beiklive::string::iequals(s, suffix))
        {
            found = true;
            break;
        }
    }

    return (m_filterMode == FilterMode::Whitelist) ? found : !found;
}

void FileListPage::refreshList(const std::string &path)
{
    bklog::debug("FileListPage::refreshList: path='{}'", path);
    m_currentPath = path;
    m_inDriveListMode = false;
    updateHeader();
    clearDetailPanel();

    auto rawList = beiklive::file::listDir(path, beiklive::file::SortBy::TypeThenName);
    bklog::debug("FileListPage::refreshList: listDir returned {} entries", static_cast<int>(rawList.size()));

    m_items.clear();

    // 从设置中获取 logo 加载模式：0=不显示；1=聚焦时加载（默认）；2=预加载
    int logoMode = 1;
    SettingManager->SetDefault("UI.logoLoadMode", 1);
    if (SettingManager && SettingManager->Contains("UI.logoLoadMode"))
        logoMode = *SettingManager->Get("UI.logoLoadMode")->AsInt();

    for (const auto &fullPath : rawList)
    {
        auto pathType = beiklive::file::getPathType(fullPath);
        bool isDir = (pathType == beiklive::file::PathType::Directory);

        std::string name = beiklive::string::extractFileName(fullPath);

        if (!isDir)
        {
            std::string suffix = beiklive::string::getFileSuffix(name);
            if (!passesFilter(suffix))
                continue;
        }

        FileListItem item;
        item.fileName = name;
        item.fullPath = fullPath;
        item.isDir = isDir;
        item.mappedName = lookupMappedName(name, isDir);

        // Prefetch logo path when logoMode == 2 (prefetch) or 1 (on-focus will
        // also do the lookup in updateDetailPanel, but we store the key here)
        if (!isDir && logoMode != 0) {
            // 初始化 gamedataManager 条目（如有需要）
            beiklive::EmuPlatform platform = detectPlatform(name);
            if (platform != beiklive::EmuPlatform::None)
                initGameData(name, platform);
            item.logoPath = lookupLogoPath(name);
        }

        if (isDir)
        {
            item.childCount = countChildren(fullPath);
        }
        else
        {
            try
            {
                item.fileSize = fs::file_size(fullPath);
            }
            catch (...)
            {
                item.fileSize = 0;
            }
        }

        m_items.push_back(std::move(item));
    }

    // 过滤后列表为空时确保至少存在一个条目
    if (m_items.empty())
    {
#ifdef _WIN32
        // Windows 根目录下改为显示驱动器盘符列表，方便导航
        if (beiklive::file::is_root_directory(m_currentPath))
        {
            showDriveList();
            return; // showDriveList 负责重建列表和更新头部
        }
#endif
        // 非根目录（或非 Windows 根）：添加 ".." 条目以便回退
        bklog::debug("FileListPage::refreshList: empty, adding '..' entry");
        FileListItem dotdot;
        dotdot.fileName = "..";
        dotdot.fullPath = beiklive::file::getParentPath(m_currentPath);
        dotdot.isDir = true;
        dotdot.childCount = 0;
        m_items.push_back(std::move(dotdot));
    }

    if (m_header)
    {
        int total = static_cast<int>(m_items.size());
        m_header->setInfo("0/" + std::to_string(total));
    }

    bklog::debug("FileListPage::refreshList: {} items after filter, rebuilding views", static_cast<int>(m_items.size()));
    // 重建条目视图并将焦点重置到第一项。
    // 与 RecyclerFrame::reloadData() 不同，此方法直接设置焦点，
    // 不受回收器内部排序或布局时序影响，完全可靠。
    rebuildItemViews();
}

void FileListPage::updateHeader()
{
    if (m_header)
        m_header->setPath(m_currentPath);
}

void FileListPage::clearDetailPanel()
{
    if (!m_detailPanel || m_layoutMode == LayoutMode::ListOnly)
        return;

    m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
    m_detailName->setText("  ");
    m_detailMappedName->setText("  ");
    m_detailInfo->setText("   ");
    if (m_detailLastOpen)  m_detailLastOpen->setText("   ");
    if (m_detailTotalTime) m_detailTotalTime->setText("   ");
    if (m_detailPlatform)  m_detailPlatform->setText("   ");
}

void FileListPage::updateDetailPanel(const FileListItem &item)
{
    if (!m_detailPanel || m_layoutMode == LayoutMode::ListOnly)
        return;

    m_detailName->setText(item.fileName);
    m_detailMappedName->setText(item.mappedName.empty() ? "    " : item.mappedName);

    // ── Game-data extra labels ─────────────────────────────────────────────
    beiklive::EmuPlatform platform = detectPlatform(item.fileName);
    bool isGameFile = (!item.isDir && platform != beiklive::EmuPlatform::None);

    if (isGameFile) {
        // 确保 gamedataManager 中存在该条目
        initGameData(item.fileName, platform);

        std::string lastOpen  = getGameDataStr(item.fileName, GAMEDATA_FIELD_LASTOPEN, "从未游玩");
        // totaltime 字段以整数形式存储，须用 getGameDataInt 读取，
        // 使用 getGameDataStr 会因类型不匹配而始终返回默认值 "0"
        int totalSeconds      = getGameDataInt(item.fileName, GAMEDATA_FIELD_TOTALTIME, 0);
        std::string platStr   = getGameDataStr(item.fileName, GAMEDATA_FIELD_PLATFORM, "");

        if (m_detailLastOpen)  m_detailLastOpen->setText("上次游玩: " + lastOpen);
        if (m_detailPlatform)  m_detailPlatform->setText("平台: " + platStr);

        // 格式化总游玩时长为 Xh Ym Zs
        int h = totalSeconds / 3600;
        int m = (totalSeconds % 3600) / 60;
        int s = totalSeconds % 60;
        std::string timeStr;
        if (h > 0) timeStr += std::to_string(h) + "h ";
        if (h > 0 || m > 0) timeStr += std::to_string(m) + "m ";
        timeStr += std::to_string(s) + "s";
        if (m_detailTotalTime) m_detailTotalTime->setText("游玩时长: " + timeStr);
    } else {
        if (m_detailLastOpen)  m_detailLastOpen->setText("  ");
        if (m_detailTotalTime) m_detailTotalTime->setText("   ");
        if (m_detailPlatform)  m_detailPlatform->setText("   ");
    }

    if (item.isDir) {
        m_detailInfo->setText(std::to_string(item.childCount) + " items");
    } else {
        m_detailInfo->setText(formatFileSize(item.fileSize));
    }

    // ── 缩略图 ─────────────────────────────────────────────────────────────────
    // 优先级1：目录 → 文件夹图标
    if (item.isDir) {
        std::string path_prefix = "img/ui/" +
            std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");
        m_detailThumb->setImageFromFile(BK_RES(path_prefix + "wenjianjia.png"));
        return;
    }

    // 优先级2：gamedataManager 中的 logo 路径（游戏文件）
    if (!item.logoPath.empty() &&
        beiklive::file::getPathType(item.logoPath) == beiklive::file::PathType::File)
    {
        m_detailThumb->setImageFromFileAsync(item.logoPath);
        return;
    }

    // 再次检查 gamedataManager（可能 logoPath 已设置但未存入 item）
    if (isGameFile) {
        std::string storedLogo = getGameDataStr(item.fileName, GAMEDATA_FIELD_LOGOPATH, "");
        std::string cover = beiklive::resolveGameCoverPath(storedLogo, item.fullPath);
        if (!cover.empty())
            m_detailThumb->setImageFromFileAsync(cover);
        else
            m_detailThumb->setImageFromFileAsync(BK_APP_DEFAULT_LOGO);
        return;
    }

    // 优先级3：与文件同名的缩略图（相同基础名）
    {
        auto dot = item.fullPath.rfind('.');
        if (dot != std::string::npos)
        {
            static const char* k_thumbExts[] = { ".png", ".jpg", ".jpeg" };
            for (const char* ext : k_thumbExts)
            {
                std::string thumbPath = item.fullPath.substr(0, dot) + ext;
                if (beiklive::file::getPathType(thumbPath) == beiklive::file::PathType::File)
                {
                    m_detailThumb->setImageFromFileAsync(thumbPath);
                    return;
                }
            }
        }
    }

    // 优先级4：默认 logo
    std::string path_prefix = "img/ui/" +
            std::string((brls::Application::getPlatform()->getThemeVariant() == brls::ThemeVariant::DARK) ? "light/" : "dark/");
    m_detailThumb->setImageFromFile(BK_RES(path_prefix + "wenjian.png"));
}

void FileListPage::showDriveList()
{
#ifdef _WIN32
    m_inDriveListMode = true;
    m_currentPath = "";

    if (m_header)
    {
        m_header->setPath("beiklive/file/drives"_i18n);
        m_header->setInfo("");
    }
    clearDetailPanel();

    m_items.clear();

    for (const auto &drivePath : getWindowsDrives())
    {
        FileListItem item;
        item.fileName = drivePath.substr(0, 2); // 如 "C:"
        item.fullPath = drivePath;              // 如 "C:\\"
        item.isDir = true;
        item.childCount = 0;
        m_items.push_back(std::move(item));
    }

    if (m_header)
    {
        int total = static_cast<int>(m_items.size());
        m_header->setInfo(std::to_string(total) + "/" + std::to_string(total));
    }

    rebuildItemViews();
#endif
}

void FileListPage::navigateUp()
{
    bklog::debug("FileListPage::navigateUp: currentPath='{}'", m_currentPath);
#ifdef _WIN32
    // 驱动器列表模式下无处可向上导航，静默忽略
    if (m_inDriveListMode)
        return;

    // 在驱动器根目录（如 "C:\"）时，向上回到盘符列表
    if (beiklive::file::is_root_directory(m_currentPath))
    {
        showDriveList();
        return;
    }
#else
    if (beiklive::file::is_root_directory(m_currentPath))
        return;
#endif

    std::string parent = beiklive::file::getParentPath(m_currentPath);
    if (parent.empty())
        parent = "/";
    bklog::debug("FileListPage::navigateUp: going to '{}'", parent);
    refreshList(parent);
}

void FileListPage::openItem(const FileListItem &item)
{
    bklog::debug("FileListPage::openItem: '{}' isDir={}", item.fileName, item.isDir);
    if (item.isDir)
    {
        refreshList(item.fullPath);
        return;
    }

    // File: find and invoke the appropriate callback
    std::string suffix = beiklive::string::getFileSuffix(item.fileName);
    bklog::debug("FileListPage::openItem: suffix='{}'", suffix);
    for (auto &kv : m_fileCallbacks)
    {
        if (beiklive::string::iequals(kv.first, suffix))
        {
            bklog::info("FileListPage::openItem: invoking callback for suffix '{}'", suffix);
            kv.second(item);
            return;
        }
    }
    if (m_defaultFileCallback) {
        bklog::info("FileListPage::openItem: invoking default callback for '{}'", item.fileName);
        m_defaultFileCallback(item);
    }
}

void FileListPage::doNewFolder()
{
    brls::Application::getPlatform()->getImeManager()->openForText(
        [this](const std::string &folderName)
        {
            if (folderName.empty())
                return;
            try
            {
                fs::path newDir = fs::path(m_currentPath) / folderName;
                fs::create_directory(newDir);
                bklog::info("Created folder: {}", newDir.string());
                refreshList(m_currentPath);
            }
            catch (const std::exception &e)
            {
                bklog::error("Create folder failed: {}", e.what());
            }
        },
        "beiklive/sidebar/new_folder"_i18n,
        "",
        128,
        "");
}

// ─────────── Callbacks from ItemView ─────────────────────────────────────────

void FileListPage::onItemFocused(int index)
{
    bklog::debug("FileListPage::onItemFocused: index={}", index);
    if (m_header && index >= 0)
    {
        int total = static_cast<int>(m_items.size());
        m_header->setInfo(std::to_string(index + 1) + "/" + std::to_string(total));
    }

    if (m_layoutMode != LayoutMode::ListAndDetail)
        return;
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return;
    updateDetailPanel(m_items[index]);
}

void FileListPage::onItemActivated(int index)
{
    bklog::debug("FileListPage::onItemActivated: index={}", index);
    if (index < 0 || index >= static_cast<int>(m_items.size())) {
        bklog::warning("FileListPage::onItemActivated: index {} out of range (size={})", index, static_cast<int>(m_items.size()));
        return;
    }
    openItem(m_items[index]);
}

void FileListPage::openSidebar(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size()))
        return;

    const FileListItem &item = m_items[itemIndex];

    // Build option list – use a struct so we avoid index-arithmetic bugs when
    // options are conditionally excluded.
    struct Option
    {
        std::string label;
        std::function<void()> action;
    };
    std::vector<Option> opts;

#ifdef __SWITCH__
    // Rename is only supported on Switch (IME + filesystem write access)
    opts.push_back({"beiklive/sidebar/rename"_i18n,
                    [this, itemIndex]()
                    { doRename(itemIndex); }});

    opts.push_back({"beiklive/sidebar/set_mapping"_i18n,
                    [this, itemIndex]()
                    { doSetMapping(itemIndex); }});
#endif

    // Select logo (game files only)
    if (!item.isDir && detectPlatform(item.fileName) != beiklive::EmuPlatform::None)
    {
        std::string captureFileName = item.fileName;
        std::string captureFullPath = item.fullPath;
        opts.push_back({"beiklive/sidebar/select_logo"_i18n,
                        [captureFileName, captureFullPath]()
                        {
                            auto *flPage = new FileListPage();
                            flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
                            flPage->setLayoutMode(FileListPage::LayoutMode::ListOnly);
                            flPage->setDefaultFileCallback([captureFileName](const FileListItem &imgItem)
                                                           {
                                                               setGameDataStr(captureFileName, GAMEDATA_FIELD_LOGOPATH, imgItem.fullPath);
                                                               g_recentGamesDirty = true;
                                                               // 延迟到当前帧回调链结束后再执行，避免崩溃
                                                               brls::sync([]() { brls::Application::popActivity(); });
                                                           });
                            std::string startPath = beiklive::file::getParentPath(captureFullPath);
                            if (startPath.empty() ||
                                beiklive::file::getPathType(startPath) != beiklive::file::PathType::Directory)
                            {
                                startPath = "/";
#ifdef _WIN32
                                startPath = "C:\\";
#endif
                            }
                            flPage->navigateTo(startPath);

                            auto *container = new brls::Box(brls::Axis::COLUMN);
                            container->setGrow(1.0f);
                            container->addView(flPage);
                            container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                                                      [](brls::View *) {
                                                          brls::Application::popActivity();
                                                          return true;
                                                      });
                            auto *frame = new brls::AppletFrame(container);
                            frame->setHeaderVisibility(brls::Visibility::GONE);
                            frame->setFooterVisibility(brls::Visibility::GONE);
                            frame->setBackground(brls::ViewBackground::NONE);
                            brls::Application::pushActivity(new brls::Activity(frame));
                        }});
    }

    // 选择金手指文件（仅限已识别平台的游戏文件）
    if (!item.isDir && detectPlatform(item.fileName) != beiklive::EmuPlatform::None)
    {
        std::string captureFileName = item.fileName;
        std::string captureFullPath = item.fullPath;
        opts.push_back({"beiklive/sidebar/select_cheat"_i18n,
                        [captureFileName, captureFullPath]()
                        {
                            auto *flPage = new FileListPage();
                            flPage->setFilter({"cht"}, FileListPage::FilterMode::Whitelist);
                            flPage->setLayoutMode(FileListPage::LayoutMode::ListOnly);
                            flPage->setDefaultFileCallback([captureFileName](const FileListItem &chtItem)
                                                           {
                                                               setGameDataStr(captureFileName, GAMEDATA_FIELD_CHEATPATH, chtItem.fullPath);
                                                               brls::Application::popActivity();
                                                           });
                            std::string startPath = beiklive::file::getParentPath(captureFullPath);
                            if (startPath.empty() ||
                                beiklive::file::getPathType(startPath) != beiklive::file::PathType::Directory)
                            {
                                startPath = "/";
#ifdef _WIN32
                                startPath = "C:\\";
#endif
                            }
                            flPage->navigateTo(startPath);

                            auto *container = new brls::Box(brls::Axis::COLUMN);
                            container->setGrow(1.0f);
                            container->addView(flPage);
                            container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                                                      [](brls::View *) {
                                                          brls::Application::popActivity();
                                                          return true;
                                                      });
                            auto *frame = new brls::AppletFrame(container);
                            frame->setHeaderVisibility(brls::Visibility::GONE);
                            frame->setFooterVisibility(brls::Visibility::GONE);
                            frame->setBackground(brls::ViewBackground::NONE);
                            brls::Application::pushActivity(new brls::Activity(frame));
                        }});
    }

    // 添加到游戏库（仅限已识别平台的游戏文件）
    {
        beiklive::EmuPlatform platform = detectPlatform(item.fileName);
        if (!item.isDir && platform != beiklive::EmuPlatform::None)
        {
            std::string captureFileName = item.fileName;
            std::string captureFullPath = item.fullPath;
            opts.push_back({"beiklive/sidebar/add_to_library"_i18n,
                            [captureFileName, captureFullPath, platform]()
                            {
                                auto *confirm = new brls::Dropdown(
                                    "beiklive/sidebar/add_to_library_confirm"_i18n,
                                    {"beiklive/hints/confirm"_i18n, "brls/hints/cancel"_i18n},
                                    [captureFileName, captureFullPath, platform](int sel)
                                    {
                                        if (sel != 0)
                                            return;
                                        // 初始化游戏数据条目（如不存在则创建）
                                        initGameData(captureFileName, platform);
                                        // 更新 gamepath 字段
                                        setGameDataStr(captureFileName, GAMEDATA_FIELD_GAMEPATH, captureFullPath);
                                        // 加入最近游戏队列并标记 AppPage 刷新
                                        pushRecentGame(captureFileName);
                                        g_recentGamesDirty = true;
                                        bklog::info("FileListPage: '{}' 已添加到游戏库，路径: {}",
                                                    captureFileName, captureFullPath);
                                    });
                                brls::Application::pushActivity(new brls::Activity(confirm));
                            }});
        }
    }

    opts.push_back({"beiklive/sidebar/new_folder"_i18n,
                    [this]()
                    { doNewFolder(); }});

    std::vector<std::string> labels;
    for (const auto &o : opts)
        labels.push_back(o.label);

    // Bug fix: pass opts action as dismissCb (5th arg) so it executes AFTER
    // the Dropdown activity has been popped from the stack.  Using cb (3rd arg)
    // causes Dropdown::didSelectRowAt to call pushActivity first and then
    // popActivity, which incorrectly pops the newly-pushed activity instead
    // of the Dropdown itself.
    auto *dropdown = new brls::Dropdown(
        item.displayName(),
        labels,
        [](int) {},   // cb: no-op – action executes after dismiss
        -1,
        [opts](int sel)
        {
            if (sel >= 0 && sel < static_cast<int>(opts.size()))
                opts[sel].action();
        });

    brls::Application::pushActivity(new brls::Activity(dropdown));
}

// ─────────── Sidebar actions ─────────────────────────────────────────────────

void FileListPage::doRename(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size()))
        return;
    // Capture the data we need by value — the IME callback runs asynchronously
    // and `m_items` may be reallocated before it fires.
    const std::string oldFullPath = m_items[itemIndex].fullPath;
    const std::string oldFileName = m_items[itemIndex].fileName;

    brls::Application::getPlatform()->getImeManager()->openForText(
        [this, oldFullPath](const std::string &newName)
        {
            if (newName.empty())
                return;
            try
            {
                fs::path oldPath(oldFullPath);
                fs::path newPath = oldPath.parent_path() / newName;
                fs::rename(oldPath, newPath);
                bklog::info("Renamed: {} → {}", oldFullPath, newPath.string());
                refreshList(m_currentPath);
            }
            catch (const std::exception &e)
            {
                bklog::error("Rename failed: {}", e.what());
            }
        },
        "beiklive/sidebar/rename"_i18n,
        "",
        128,
        oldFileName);
}

void FileListPage::doSetMapping(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size()))
        return;
    // Capture necessary data by value for the async callback
    const FileListItem itemCopy = m_items[itemIndex];

    // Build the config key (base name without extension for files)
    std::string key;
    if (itemCopy.isDir)
    {
        key = itemCopy.fileName;
    }
    else
    {
        auto dot = itemCopy.fileName.rfind('.');
        key = (dot != std::string::npos) ? itemCopy.fileName.substr(0, dot) : itemCopy.fileName;
    }

    brls::Application::getPlatform()->getImeManager()->openForText(
        [this, key, fullPath = itemCopy.fullPath](const std::string &mapped)
        {
            if (!NameMappingManager)
                return;
            if (mapped.empty())
                NameMappingManager->Remove(key);
            else
                NameMappingManager->Set(key, mapped);
            NameMappingManager->Save();
            // Refresh list to reflect the new mapped name
            refreshList(m_currentPath);
        },
        "beiklive/sidebar/set_mapping"_i18n,
        "",
        128,
        itemCopy.mappedName);
}
