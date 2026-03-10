#include "XMLUI/Pages/FileListPage.hpp"

#include <filesystem>
#include <iomanip>
#include <sstream>

#include "Utils/fileUtils.hpp"
#include "Utils/strUtils.hpp"

using namespace brls::literals; // for _i18n

namespace fs = std::filesystem;

static constexpr float CELL_HEIGHT     = 66.f;
static constexpr float ACCENT_W        = 4.f;
static constexpr float CELL_PAD_H      = 12.f;
static constexpr float CELL_PAD_V      = 10.f;
static constexpr float INFO_FONT_SIZE  = 24.f;
static constexpr float NAME_FONT_SIZE  = 26.f;
static constexpr float DETAIL_THUMB_SZ = 180.f;

// ─────────────────────────────────────────────────────────────────────────────
//  Helpers
// ─────────────────────────────────────────────────────────────────────────────

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
//  FileListCell
// ─────────────────────────────────────────────────────────────────────────────

FileListCell::FileListCell()
{
    setFocusable(true);
    setAxis(brls::Axis::ROW);
    setAlignItems(brls::AlignItems::CENTER);
    setHeight(CELL_HEIGHT);
    setWidth(brls::View::AUTO);

    // Accent rectangle on the left
    m_accent = new brls::Rectangle();
    m_accent->setWidth(ACCENT_W);
    m_accent->setHeight(CELL_HEIGHT - 2 * CELL_PAD_V);
    m_accent->setMarginLeft(CELL_PAD_H);
    m_accent->setMarginRight(CELL_PAD_H);
    m_accent->setColor(nvgRGBA(79, 193, 255, 255));
    m_accent->setVisibility(brls::Visibility::INVISIBLE);
    addView(m_accent);

    // Name label (grows to fill space)
    m_nameLabel = new brls::Label();
    m_nameLabel->setFontSize(NAME_FONT_SIZE);
    m_nameLabel->setSingleLine(true);
    m_nameLabel->setGrow(1.0f);
    m_nameLabel->setMarginRight(CELL_PAD_H);
    addView(m_nameLabel);

    // Info label on the right (gray)
    m_infoLabel = new brls::Label();
    m_infoLabel->setFontSize(INFO_FONT_SIZE);
    m_infoLabel->setSingleLine(true);
    m_infoLabel->setTextColor(nvgRGBA(160, 160, 160, 255));
    m_infoLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
    m_infoLabel->setMarginRight(CELL_PAD_H * 2);
    addView(m_infoLabel);
}

FileListCell* FileListCell::create()
{
    return new FileListCell();
}

void FileListCell::setItem(const FileListItem& item, int index)
{
    m_index = index;
    m_nameLabel->setText(item.displayName());

    if (item.isDir)
    {
        m_infoLabel->setText(std::to_string(item.childCount) + " items");
    }
    else
    {
        m_infoLabel->setText(formatFileSize(item.fileSize));
    }
}

void FileListCell::onFocusGained()
{
    brls::RecyclerCell::onFocusGained();
    m_accent->setVisibility(brls::Visibility::VISIBLE);
    if (onItemFocused)
        onItemFocused(m_index);
}

void FileListCell::onFocusLost()
{
    brls::RecyclerCell::onFocusLost();
    m_accent->setVisibility(brls::Visibility::INVISIBLE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  FileListDataSource
// ─────────────────────────────────────────────────────────────────────────────

static const std::string CELL_ID = "FileListCell";

FileListDataSource::FileListDataSource(FileListPage* page)
    : m_page(page)
{
}

int FileListDataSource::numberOfRows(brls::RecyclerFrame* /*recycler*/, int /*section*/)
{
    return static_cast<int>(items.size());
}

brls::RecyclerCell* FileListDataSource::cellForRow(brls::RecyclerFrame* recycler,
                                                    brls::IndexPath      indexPath)
{
    auto* cell = dynamic_cast<FileListCell*>(recycler->dequeueReusableCell(CELL_ID));
    if (!cell)
        return nullptr;

    const int row = indexPath.row;
    if (row < 0 || row >= static_cast<int>(items.size()))
        return cell;

    cell->setItem(items[row], row);
    cell->onItemFocused = [this](int idx) {
        if (m_page)
            m_page->onItemFocused(idx);
    };

    // ButtonA → activate item
    cell->registerAction("beiklive/hints/confirm"_i18n,
                         brls::BUTTON_A,
                         [this, row](brls::View*) {
                             if (m_page)
                                 m_page->onItemActivated(row);
                             return true;
                         },
                         false, false, brls::SOUND_CLICK);

    // ButtonX → open sidebar
    cell->registerAction("beiklive/hints/operate"_i18n,
                         brls::BUTTON_X,
                         [this, row](brls::View*) {
                             if (m_page)
                                 m_page->openSidebar(row);
                             return true;
                         });

    return cell;
}

void FileListDataSource::didSelectRowAt(brls::RecyclerFrame* /*recycler*/,
                                         brls::IndexPath       indexPath)
{
    // Handled via registerAction in cellForRow
    (void)indexPath;
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
    // m_recycler (child view) is destroyed before this destructor body runs.
    // Since we passed deleteDataSource=false, we must free m_dataSource here.
    delete m_dataSource;
    m_dataSource = nullptr;
    bklog::debug("FileListPage destroyed.");
}

// ─────────── buildUI ─────────────────────────────────────────────────────────

void FileListPage::buildUI()
{
    // ── Header ────────────────────────────────────────────────────────────
    m_header = new brls::Header();
    m_header->setTitle("beiklive/file/file_select"_i18n);
    m_header->setSubtitle("");
    addView(m_header);

    // ── Content row (list + optional detail panel) ────────────────────────
    m_contentBox = new brls::Box(brls::Axis::ROW);
    m_contentBox->setGrow(1.0f);
    m_contentBox->setWidth(brls::View::AUTO);
    addView(m_contentBox);

    // List container
    m_listBox = new brls::Box(brls::Axis::COLUMN);
    m_listBox->setGrow(1.0f);
    m_contentBox->addView(m_listBox);

    // RecyclerFrame
    m_recycler = new brls::RecyclerFrame();
    m_recycler->setWidth(brls::View::AUTO);
    m_recycler->setHeight(brls::View::AUTO);
    m_recycler->setGrow(1.0f);
    m_recycler->estimatedRowHeight = CELL_HEIGHT;
    m_recycler->registerCell(CELL_ID, FileListCell::create);
    m_listBox->addView(m_recycler);

    // DataSource (RecyclerFrame takes ownership)
    m_dataSource = new FileListDataSource(this);
    m_recycler->setDataSource(m_dataSource, false); // we manage lifetime via m_dataSource

    // ── Detail panel (hidden initially) ──────────────────────────────────
    buildDetailPanel();
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
    m_detailPanel->setBackgroundColor(nvgRGBA(40, 40, 40, 80));
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
    m_detailName->setSingleLine(false);
    m_detailName->setTextColor(nvgRGBA(220, 220, 220, 255));
    m_detailPanel->addView(m_detailName);

    m_detailMappedName = new brls::Label();
    m_detailMappedName->setFontSize(20.f);
    m_detailMappedName->setMarginTop(6.f);
    m_detailMappedName->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    m_detailMappedName->setSingleLine(false);
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
        m_listBox->setWidthPercentage(66.67f);
        m_detailPanel->setWidthPercentage(33.33f);
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
    updateHeader();
    clearDetailPanel();

    auto rawList = beiklive::file::listDir(path, beiklive::file::SortBy::TypeThenName);

    m_dataSource->items.clear();

    // ".." entry (navigate up)
    if (!beiklive::file::is_root_directory(path))
    {
        FileListItem up;
        up.fileName  = "..";
        up.isDir     = true;
        up.fullPath  = beiklive::file::getParentPath(path);
        up.childCount = 0;
        m_dataSource->items.push_back(std::move(up));
    }

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

        m_dataSource->items.push_back(std::move(item));
    }

    m_recycler->reloadData();
}

void FileListPage::updateHeader()
{
    if (m_header)
        m_header->setSubtitle(m_currentPath);
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

    // Try to find a thumbnail image with the same base name next to the file
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
    m_detailThumb->setImageFromFile(BK_APP_DEFAULT_LOGO);
}

void FileListPage::navigateUp()
{
    if (beiklive::file::is_root_directory(m_currentPath))
        return;
    std::string parent = beiklive::file::getParentPath(m_currentPath);
    if (parent.empty())
        parent = "/";
    refreshList(parent);
}

void FileListPage::openItem(const FileListItem& item)
{
    if (item.isDir)
    {
        if (item.fileName == "..")
            navigateUp();
        else
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

// ─────────── Callbacks from DataSource/Cell ──────────────────────────────────

void FileListPage::onItemFocused(int index)
{
    if (m_layoutMode != LayoutMode::ListAndDetail)
        return;
    if (index < 0 || index >= static_cast<int>(m_dataSource->items.size()))
        return;
    updateDetailPanel(m_dataSource->items[index]);
}

void FileListPage::onItemActivated(int index)
{
    if (index < 0 || index >= static_cast<int>(m_dataSource->items.size()))
        return;
    openItem(m_dataSource->items[index]);
}

void FileListPage::openSidebar(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_dataSource->items.size()))
        return;

    const FileListItem& item = m_dataSource->items[itemIndex];

    // Build option list and record which index maps to paste/delete
    std::vector<std::string> options = {
        "beiklive/sidebar/rename"_i18n,    // 0
        "beiklive/sidebar/set_mapping"_i18n, // 1
        "beiklive/sidebar/cut"_i18n          // 2
    };

    const bool hasPaste  = m_hasClipboard;
    const int  pasteIdx  = hasPaste ? 3 : -1;
    const int  deleteIdx = hasPaste ? 4 : 3;

    if (hasPaste)
        options.push_back("beiklive/sidebar/paste"_i18n);
    options.push_back("beiklive/sidebar/delete"_i18n);

    auto* dropdown = new brls::Dropdown(
        item.displayName(),
        options,
        [this, itemIndex, pasteIdx, deleteIdx](int sel) {
            if (sel < 0)
                return;
            if (sel == 0)           doRename(itemIndex);
            else if (sel == 1)      doSetMapping(itemIndex);
            else if (sel == 2)      doCut(itemIndex);
            else if (sel == pasteIdx && pasteIdx >= 0) doPaste();
            else if (sel == deleteIdx) doDelete(itemIndex);
        });

    brls::Application::pushActivity(new brls::Activity(dropdown));
}

// ─────────── Sidebar actions ─────────────────────────────────────────────────

void FileListPage::doRename(int itemIndex)
{
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_dataSource->items.size()))
        return;
    // Capture the data we need by value — the IME callback runs asynchronously
    // and `m_dataSource->items` may be reallocated before it fires.
    const std::string oldFullPath = m_dataSource->items[itemIndex].fullPath;
    const std::string oldFileName = m_dataSource->items[itemIndex].fileName;

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
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_dataSource->items.size()))
        return;
    // Capture necessary data by value for the async callback
    const FileListItem itemCopy = m_dataSource->items[itemIndex];

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
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_dataSource->items.size()))
        return;
    m_clipboardItem = m_dataSource->items[itemIndex];
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
    if (itemIndex < 0 || itemIndex >= static_cast<int>(m_dataSource->items.size()))
        return;
    const FileListItem& item = m_dataSource->items[itemIndex];

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

