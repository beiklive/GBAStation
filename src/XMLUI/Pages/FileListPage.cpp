#include "XMLUI/Pages/FileListPage.hpp"

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

static constexpr float CELL_HEIGHT     = 66.f;
static constexpr float ICON_HEIGHT     = 50.f;
static constexpr float ACCENT_W        = 4.f;
static constexpr float CELL_PAD_H      = 12.f;
static constexpr float CELL_PAD_V      = 10.f;
static constexpr float INFO_FONT_SIZE  = 16.f;
static constexpr float NAME_FONT_SIZE  = 26.f;
static constexpr float DETAIL_THUMB_SZ = 180.f;

// ─────────────────────────────────────────────────────────────────────────────
//  File-type extension tables
// ─────────────────────────────────────────────────────────────────────────────

/// Game Boy / Game Boy Color ROM extensions
static const std::vector<std::string> k_gbExtensions  = { "gb", "gbc" };
/// Game Boy Advance ROM extensions
static const std::vector<std::string> k_gbaExtensions = { "gba" };
/// Common image file extensions
static const std::vector<std::string> k_imageExtensions = {
    "png", "jpg", "jpeg", "bmp", "gif", "webp", "tga"
};
/// Common archive / compressed-file extensions
static const std::vector<std::string> k_zipExtensions = {
    "zip", "rar", "7z", "gz", "tar", "bz2", "xz", "zst"
};

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Returns the resource path of the icon that should represent @p item.
/// The icon variant (_light / _dark) is selected based on @p theme.
static std::string getFileIconPath(const FileListItem& item, brls::ThemeVariant theme)
{
    // const std::string variant = (theme == brls::ThemeVariant::LIGHT) ? "_light" : "_dark";

    if (item.isDir)
        return BK_RES(std::string("/img/ui/icon_folder.png"));

    const std::string suffix = beiklive::string::getFileSuffix(item.fileName);

    auto matchesSuffix = [&suffix](const std::vector<std::string>& exts) {
        for (const auto& ext : exts)
            if (beiklive::string::iequals(ext, suffix))
                return true;
        return false;
    };

    if (matchesSuffix(k_gbExtensions))    return BK_RES(std::string("img/ui/icon_gb.png"));
    if (matchesSuffix(k_gbaExtensions))   return BK_RES(std::string("img/ui/icon_gba.png"));
    if (matchesSuffix(k_imageExtensions)) return BK_RES(std::string("img/ui/icon_image.png"));
    if (matchesSuffix(k_zipExtensions))   return BK_RES(std::string("img/ui/icon_zip.png"));

    return BK_RES(std::string("img/ui/icon_file.png"));
}


/// Format a file size in bytes as a human-readable string (KB / MB / GB).
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

/// Count the direct children of a directory (0 if it cannot be read).
static int countChildren(const std::string& path)
{
    int n = 0;
    try
    {
        for (const auto& e : fs::directory_iterator(path))
        {
            (void)e;
            ++n;
        }
    }
    catch (...)
    {
        // Ignore permission errors etc.
    }
    return n;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Windows: enumerate available drive letters (A: … Z:)
// ─────────────────────────────────────────────────────────────────────────────
#ifdef _WIN32
/// Returns a list of available drive root paths such as {"C:\\", "D:\\", "F:\\"}.
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
    setBackground(brls::ViewBackground::NONE);

    // Accent rectangle on the left
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
    m_icon->setImageFromFile(BK_RES("img/file/file_light.png"));
    addView(m_icon);

    // Name label (grows to fill space)
    m_nameLabel = new brls::Label();
    m_nameLabel->setFontSize(NAME_FONT_SIZE);
    m_nameLabel->setSingleLine(true);
    // Scroll (marquee) the label when the item is focused and text overflows.
    m_nameLabel->setAutoAnimate(true);
    m_nameLabel->setGrow(1.0f);
    m_nameLabel->setMarginRight(CELL_PAD_H);
    m_nameLabel->setMarginTop(8.f);
    addView(m_nameLabel);

    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setHeight(CELL_HEIGHT);
    // Info label on the right (gray)
    m_infoLabel = new brls::Label();
    m_infoLabel->setFontSize(INFO_FONT_SIZE);
    m_infoLabel->setSingleLine(true);
    m_infoLabel->setTextColor(nvgRGBA(160, 160, 160, 255));
    m_infoLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
    m_infoLabel->setMarginRight(CELL_PAD_H * 2);
    box->addView(new brls::Padding());
    box->addView(m_infoLabel);
    addView(box);
}

void FileListItemView::setItem(const FileListItem& item, int index)
{
    m_index = index;
    m_nameLabel->setText(item.displayName());
    auto theme = brls::Application::getPlatform()->getThemeVariant();

    if (item.isDir)
    {
        m_icon->setImageFromFile(BK_RES("img/ui/icon_folder.png"));
        m_infoLabel->setText(std::to_string(item.childCount) + " items");
    }
    else
    {
        m_icon->setImageFromFile(getFileIconPath(item, theme));
        m_infoLabel->setText(formatFileSize(item.fileSize));
    }

    // ButtonA → activate item
    registerAction("beiklive/hints/confirm"_i18n,
                   brls::BUTTON_A,
                   [this](brls::View*) {
                       if (onItemActivated)
                           onItemActivated(m_index);
                       return true;
                   },
                   false, false, brls::SOUND_CLICK);

    // ButtonX → open options/sidebar
    registerAction("beiklive/hints/operate"_i18n,
                   brls::BUTTON_X,
                   [this](brls::View*) {
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
    setAxis(brls::Axis::COLUMN);
    setWidth(brls::View::AUTO);
    setHeight(brls::View::AUTO);
    setGrow(1.0f);

    buildUI();

    const std::string key = "UI.fileListLayoutMode";
    SettingManager->SetDefault(key, static_cast<int>(LayoutMode::ListOnly));

    // Initialise logo load mode setting default (1 = load on focus)
    SettingManager->SetDefault("UI.logoLoadMode", 1);

    int layoutModeInt = *SettingManager->Get(key)->AsInt();
    setLayoutMode(static_cast<LayoutMode>(layoutModeInt));

    // B → navigate up
    registerAction("beiklive/hints/UP"_i18n,
                   brls::BUTTON_B,
                   [this](brls::View*) {
                       navigateUp();
                       return true;
                   });
}

FileListPage::~FileListPage()
{
    bklog::debug("FileListPage destroyed.");
}

// ─────────── buildUI ─────────────────────────────────────────────────────────

void FileListPage::buildUI()
{
    // ── Header ────────────────────────────────────────────────────────────
    m_header = new beiklive::UI::BrowserHeader();
    m_header->setTitle("beiklive/file/file_select"_i18n);
    m_header->setPath("/");
    m_header->setInfo("");
    addView(m_header);

    // ── Content row (list + optional detail panel) ────────────────────────
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

    // List container
    m_listBox = new brls::Box(brls::Axis::COLUMN);
    m_listBox->setGrow(1.0f);
    m_contentBox->addView(m_listBox);

    // ScrollingFrame replaces RecyclerFrame.
    // Items are added as direct children of m_itemsBox (no recycling) so that
    // focus management is straightforward and never suffers from the LIFO
    // cell-reuse ordering bug of RecyclerFrame.
    m_scrollFrame = new brls::ScrollingFrame();
    m_scrollFrame->setWidth(brls::View::AUTO);
    m_scrollFrame->setGrow(1.0f);
    m_scrollFrame->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

    m_itemsBox = new brls::Box(brls::Axis::COLUMN);
    m_itemsBox->setWidth(brls::View::AUTO);
    m_scrollFrame->setContentView(m_itemsBox);

    m_listBox->addView(m_scrollFrame);

    // DataSource is no longer needed; items are managed directly in m_items.

    // ── Detail panel (hidden initially) ──────────────────────────────────
    buildDetailPanel();
    m_detailPanel->setMarginLeft(20.f);
    m_detailPanel->setVisibility(brls::Visibility::GONE);
    m_contentBox->addView(m_detailPanel);

    // ── Bottom bar ────────────────────────────────────────────────────────
    m_bottomBar = new brls::BottomBar();
    addView(m_bottomBar);
}

void FileListPage::buildDetailPanel()
{
    m_detailPanel = new brls::Box(brls::Axis::COLUMN);
    m_detailPanel->setAlignItems(brls::AlignItems::CENTER);
    m_detailPanel->setPadding(20.f, 12.f, 20.f, 12.f);
    m_detailPanel->setBackgroundColor(nvgRGBA(40, 40, 40, 20));
    // Width will be set to ~33% in setLayoutMode()

    m_detailThumb = new brls::Image();
    m_detailThumb->setWidth(DETAIL_THUMB_SZ);
    m_detailThumb->setHeight(DETAIL_THUMB_SZ);
    m_detailThumb->setScalingType(brls::ImageScalingType::FIT);
    m_detailThumb->setInterpolation(brls::ImageInterpolation::LINEAR);
    m_detailThumb->setCornerRadius(8.f);
    m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
    m_detailPanel->addView(m_detailThumb);

    m_detailName = new brls::Label();
    m_detailName->setFontSize(22.f);
    m_detailName->setMarginTop(12.f);
    m_detailName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailName->setSingleLine(true);
    m_detailName->setAutoAnimate(true);
    m_detailName->setTextColor(nvgRGBA(220, 220, 220, 255));
    m_detailPanel->addView(m_detailName);

    m_detailMappedName = new brls::Label();
    m_detailMappedName->setFontSize(20.f);
    m_detailMappedName->setMarginTop(6.f);
    m_detailMappedName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailMappedName->setSingleLine(true);
    m_detailMappedName->setAutoAnimate(true);
    m_detailMappedName->setTextColor(nvgRGBA(120, 190, 255, 255));
    m_detailPanel->addView(m_detailMappedName);

    m_detailInfo = new brls::Label();
    m_detailInfo->setFontSize(20.f);
    m_detailInfo->setMarginTop(8.f);
    m_detailInfo->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailInfo->setTextColor(nvgRGBA(160, 160, 160, 255));
    m_detailPanel->addView(m_detailInfo);
}

// ─────────── Public API ──────────────────────────────────────────────────────

void FileListPage::navigateTo(const std::string& path)
{
    refreshList(path);

}

void FileListPage::resetFocusToTop()
{
    const auto& children = m_itemsBox->getChildren();
    if (children.empty())
        return;

    m_scrollFrame->setContentOffsetY(0, /*animated=*/false);
    brls::Application::giveFocus(children[0]);
}

void FileListPage::rebuildItemViews()
{
    // clearViews(true) schedules old views for deferred deletion via borealis'
    // deletionPool, so Application::currentFocus remains a valid pointer until
    // the end of the current frame even if it pointed to one of the removed
    // views.  We immediately give focus to the first new item, ensuring the
    // focus is safe before the old views are eventually freed.
    m_itemsBox->clearViews(/*free=*/true);
    m_scrollFrame->setContentOffsetY(0, /*animated=*/false);

    FileListItemView* firstItem = nullptr;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
    {
        auto* itemView = new FileListItemView();
        itemView->setItem(m_items[i], i);

        itemView->onItemFocused   = [this](int idx) { onItemFocused(idx); };
        itemView->onItemActivated = [this](int idx) { onItemActivated(idx); };
        itemView->onItemOptions   = [this](int idx) { openSidebar(idx); };

        m_itemsBox->addView(itemView);
        if (i == 0)
            firstItem = itemView;
    }

    if (firstItem)
        brls::Application::giveFocus(firstItem);
}

void FileListPage::setFilter(const std::vector<std::string>& suffixes, FilterMode mode)
{
    m_filterSuffixes = suffixes;
    m_filterMode     = mode;
    m_filterEnabled  = !suffixes.empty();
}

void FileListPage::setFilterEnabled(bool enabled)
{
    m_filterEnabled = enabled;
}

void FileListPage::setFileCallback(const std::string& suffix,
                                    std::function<void(const FileListItem&)> cb)
{
    m_fileCallbacks[suffix] = std::move(cb);
}

void FileListPage::setDefaultFileCallback(std::function<void(const FileListItem&)> cb)
{
    m_defaultFileCallback = std::move(cb);
}

void FileListPage::setLayoutMode(LayoutMode mode)
{
    m_layoutMode = mode;

    if (m_layoutMode == LayoutMode::ListOnly)
    {
        // List takes full width; detail panel is hidden
        m_listBox->setWidthPercentage(100.f);
        m_detailPanel->setVisibility(brls::Visibility::GONE);
    }
    else // ListAndDetail
    {
        // List occupies ~2/3; detail panel ~1/3
        m_listBox->setWidthPercentage(65.f);
        m_detailPanel->setWidthPercentage(30.f);
        m_detailPanel->setVisibility(brls::Visibility::VISIBLE);
    }
}

// ─────────── Internal helpers ────────────────────────────────────────────────

std::string FileListPage::lookupMappedName(const std::string& name, bool isDir) const
{
    if (!NameMappingManager)
        return {};

    // Key = file name without extension for files; directory name for dirs
    std::string key;
    if (isDir)
    {
        key = name;
    }
    else
    {
        auto dot = name.rfind('.');
        key      = (dot != std::string::npos) ? name.substr(0, dot) : name;
    }

    if (auto val = NameMappingManager->Get(key); val && val->AsString())
        return *val->AsString();
    return {};
}

std::string FileListPage::lookupLogoPath(const std::string& name, bool isDir) const
{
    if (!logoManager)
        return {};

    std::string key;
    if (isDir)
    {
        key = name;
    }
    else
    {
        auto dot = name.rfind('.');
        key      = (dot != std::string::npos) ? name.substr(0, dot) : name;
    }

    if (auto val = logoManager->Get(key); val && val->AsString())
        return *val->AsString();
    return {};
}

bool FileListPage::passesFilter(const std::string& suffix) const
{
    if (!m_filterEnabled || m_filterSuffixes.empty())
        return true;

    bool found = false;
    for (const auto& s : m_filterSuffixes)
    {
        if (beiklive::string::iequals(s, suffix))
        {
            found = true;
            break;
        }
    }

    return (m_filterMode == FilterMode::Whitelist) ? found : !found;
}

void FileListPage::refreshList(const std::string& path)
{
    m_currentPath = path;
    m_inDriveListMode = false;
    updateHeader();
    clearDetailPanel();

    auto rawList = beiklive::file::listDir(path, beiklive::file::SortBy::TypeThenName);

    m_items.clear();

    // Determine the logo load mode from settings:
    // 0 = do not display; 1 = load on focus (default); 2 = prefetch
    int logoMode = 1;
    SettingManager->SetDefault("UI.logoLoadMode", 1);
    if (SettingManager && SettingManager->Contains("UI.logoLoadMode"))
        logoMode = *SettingManager->Get("UI.logoLoadMode")->AsInt();

    for (const auto& fullPath : rawList)
    {
        auto pathType = beiklive::file::getPathType(fullPath);
        bool isDir    = (pathType == beiklive::file::PathType::Directory);

        std::string name = beiklive::string::extractFileName(fullPath);

        if (!isDir)
        {
            std::string suffix = beiklive::string::getFileSuffix(name);
            if (!passesFilter(suffix))
                continue;
        }

        FileListItem item;
        item.fileName   = name;
        item.fullPath   = fullPath;
        item.isDir      = isDir;
        item.mappedName = lookupMappedName(name, isDir);

        // Prefetch logo path when logoMode == 2 (prefetch) or 1 (on-focus will
        // also do the lookup in updateDetailPanel, but we store the key here)
        if (logoMode != 0)
            item.logoPath = lookupLogoPath(name, isDir);

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

    // When the filtered list is empty, ensure there is at least one item.
    if (m_items.empty())
    {
#ifdef _WIN32
        // On Windows root directories, show the drive-letter list instead so the
        // user can navigate elsewhere.  showDriveList() always populates items.
        if (beiklive::file::is_root_directory(m_currentPath))
        {
            showDriveList();
            return; // showDriveList handles rebuild and header update
        }
#endif
        // Non-root (or non-Windows root): add a ".." entry so the user can
        // navigate back even when every file is hidden by the active filter.
        FileListItem dotdot;
        dotdot.fileName   = "..";
        dotdot.fullPath   = beiklive::file::getParentPath(m_currentPath);
        dotdot.isDir      = true;
        dotdot.childCount = 0;
        m_items.push_back(std::move(dotdot));
    }

    if (m_header)
    {
        int total = static_cast<int>(m_items.size());
        m_header->setInfo("0/" + std::to_string(total));
    }

    // Rebuild item views and reset focus to the first item.
    // Unlike RecyclerFrame::reloadData(), this directly sets focus to the
    // first view, which is completely reliable regardless of any internal
    // recycler ordering or layout timing.
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
    m_detailName->setText("");
    m_detailMappedName->setText("");
    m_detailInfo->setText("");
}

void FileListPage::updateDetailPanel(const FileListItem& item)
{
    if (!m_detailPanel || m_layoutMode == LayoutMode::ListOnly)
        return;

    m_detailName->setText(item.fileName);
    m_detailMappedName->setText(item.mappedName.empty() ? "" : item.mappedName);

    if (item.isDir)
        m_detailInfo->setText(std::to_string(item.childCount) + " items");
    else
        m_detailInfo->setText(formatFileSize(item.fileSize));

    // Priority 1: logo path from logoManager
    if (!item.logoPath.empty() &&
        beiklive::file::getPathType(item.logoPath) == beiklive::file::PathType::File)
    {
        m_detailThumb->setImageFromFile(item.logoPath);
        return;
    }

    // Priority 2: thumbnail image next to the file (same base name)
    if (!item.isDir)
    {
        auto dot = item.fullPath.rfind('.');
        if (dot != std::string::npos)
        {
            std::string thumbPath = item.fullPath.substr(0, dot) + ".png";
            if (beiklive::file::getPathType(thumbPath) == beiklive::file::PathType::File)
            {
                m_detailThumb->setImageFromFile(thumbPath);
                return;
            }
        }
    }

    // Priority 3: default logo
    m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
}

void FileListPage::showDriveList()
{
#ifdef _WIN32
    m_inDriveListMode = true;
    m_currentPath     = "";

    if (m_header)
    {
        m_header->setPath("beiklive/file/drives"_i18n);
        m_header->setInfo("");
    }
    clearDetailPanel();

    m_items.clear();

    for (const auto& drivePath : getWindowsDrives())
    {
        FileListItem item;
        item.fileName   = drivePath.substr(0, 2); // e.g. "C:"
        item.fullPath   = drivePath;              // e.g. "C:\\"
        item.isDir      = true;
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
#ifdef _WIN32
    // When in drive-list mode, there is nowhere to go further up – silently ignore
    if (m_inDriveListMode)
        return;

    // When at a drive root (e.g. "C:\"), go up to the drive-letter list
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
    refreshList(parent);
}

void FileListPage::openItem(const FileListItem& item)
{
    if (item.isDir)
    {
        refreshList(item.fullPath);
        return;
    }

    // File: find and invoke the appropriate callback
    std::string suffix = beiklive::string::getFileSuffix(item.fileName);
    for (auto& kv : m_fileCallbacks)
    {
        if (beiklive::string::iequals(kv.first, suffix))
        {
            kv.second(item);
            return;
        }
    }
    if (m_defaultFileCallback)
        m_defaultFileCallback(item);
}

void FileListPage::doNewFolder()
{
    brls::Application::getPlatform()->getImeManager()->openForText(
        [this](const std::string& folderName) {
            if (folderName.empty())
                return;
            try
            {
                fs::path newDir = fs::path(m_currentPath) / folderName;
                fs::create_directory(newDir);
                bklog::info("Created folder: {}", newDir.string());
                refreshList(m_currentPath);
            }
            catch (const std::exception& e)
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
    if (index < 0 || index >= static_cast<int>(m_items.size()))
        return;
    openItem(m_items[index]);
}

void FileListPage::openSidebar(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size()))
        return;

    // If an external settings panel handler is set, delegate to it (StartPageView)
    if (onOpenSettings)
    {
        onOpenSettings(m_items[itemIndex], itemIndex);
        return;
    }

    // Fallback: use a built-in Dropdown (should not normally be reached when
    // the host correctly sets onOpenSettings)
    const FileListItem& item = m_items[itemIndex];

    // Build option list – use a struct so we avoid index-arithmetic bugs when
    // rename is conditionally excluded on non-Switch platforms.
    struct Option {
        std::string label;
        std::function<void()> action;
    };
    std::vector<Option> opts;

#ifdef __SWITCH__
    // Rename is only supported on Switch (IME + filesystem write access)
    opts.push_back({ "beiklive/sidebar/rename"_i18n,
                     [this, itemIndex]() { doRename(itemIndex); } });
#endif

    opts.push_back({ "beiklive/sidebar/set_mapping"_i18n,
                     [this, itemIndex]() { doSetMapping(itemIndex); } });
    opts.push_back({ "beiklive/sidebar/cut"_i18n,
                     [this, itemIndex]() { doCut(itemIndex); } });

    if (m_hasClipboard)
        opts.push_back({ "beiklive/sidebar/paste"_i18n,
                         [this]() { doPaste(); } });

    opts.push_back({ "beiklive/sidebar/delete"_i18n,
                     [this, itemIndex]() { doDelete(itemIndex); } });

    std::vector<std::string> labels;
    for (const auto& o : opts)
        labels.push_back(o.label);

    auto* dropdown = new brls::Dropdown(
        item.displayName(),
        labels,
        [opts](int sel) {
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
        [this, oldFullPath](const std::string& newName) {
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
            catch (const std::exception& e)
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
        key      = (dot != std::string::npos) ? itemCopy.fileName.substr(0, dot) : itemCopy.fileName;
    }

    brls::Application::getPlatform()->getImeManager()->openForText(
        [this, key, fullPath = itemCopy.fullPath](const std::string& mapped) {
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

void FileListPage::doCut(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size()))
        return;
    m_clipboardItem = m_items[itemIndex];
    m_hasClipboard  = true;
    bklog::info("Cut: {}", m_clipboardItem.fullPath);
}

void FileListPage::doPaste()
{
    if (!m_hasClipboard)
        return;

    try
    {
        fs::path src(m_clipboardItem.fullPath);
        fs::path dst = fs::path(m_currentPath) / src.filename();
        fs::rename(src, dst);
        bklog::info("Pasted: {} → {}", src.string(), dst.string());
        m_hasClipboard = false;
        refreshList(m_currentPath);
    }
    catch (const std::exception& e)
    {
        bklog::error("Paste failed: {}", e.what());
    }
}

void FileListPage::doDelete(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_items.size()))
        return;
    const FileListItem& item = m_items[itemIndex];

    // Confirm via a second Dropdown
    auto* confirm = new brls::Dropdown(
        "beiklive/sidebar/delete_confirm"_i18n,
        { "beiklive/hints/confirm"_i18n, "brls/hints/cancel"_i18n },
        [this, path = item.fullPath](int sel) {
            if (sel != 0)
                return;
            try
            {
                fs::remove_all(path);
                bklog::info("Deleted: {}", path);
                refreshList(m_currentPath);
            }
            catch (const std::exception& e)
            {
                bklog::error("Delete failed: {}", e.what());
            }
        });

    brls::Application::pushActivity(new brls::Activity(confirm));
}

