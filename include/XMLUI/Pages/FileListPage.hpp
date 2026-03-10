#pragma once

#include <borealis.hpp>

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "XMLUI/Utils/Utils.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  FileListItem  –  describes one entry in the file list
// ─────────────────────────────────────────────────────────────────────────────
struct FileListItem
{
    std::string fileName;   ///< actual file / dir name (no path)
    std::string mappedName; ///< display alias from NameMappingManager (empty = none)
    bool        isDir       = false;
    std::string fullPath;   ///< absolute path
    std::uintmax_t fileSize = 0; ///< bytes (files only)
    int         childCount  = 0; ///< direct-child count (dirs only)
    std::string logoPath;   ///< logo image path from logoManager (may be empty)

    /// Returns mappedName when available, else fileName
    const std::string& displayName() const
    {
        return mappedName.empty() ? fileName : mappedName;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  Forward declarations
// ─────────────────────────────────────────────────────────────────────────────
class FileListPage;

// ─────────────────────────────────────────────────────────────────────────────
//  FileListCell  –  one row in the recycler
// ─────────────────────────────────────────────────────────────────────────────
class FileListCell : public brls::RecyclerCell
{
  public:
    FileListCell();

    /// Populate the cell with item data.  index is stored for callbacks.
    void setItem(const FileListItem& item, int index);

    /// Called when an item gains focus (cell notifies the page)
    std::function<void(int)> onItemFocused;

    void onFocusGained() override;
    void onFocusLost() override;

    static FileListCell* create();

  private:
    brls::Rectangle* m_accent    = nullptr;
    brls::Image*     m_icon      = nullptr;
    brls::Label*     m_nameLabel = nullptr;
    brls::Label*     m_infoLabel = nullptr;
    int              m_index     = -1;
};

// ─────────────────────────────────────────────────────────────────────────────
//  FileListDataSource  –  feeds data into RecyclerFrame
// ─────────────────────────────────────────────────────────────────────────────
class FileListDataSource : public brls::RecyclerDataSource
{
  public:
    explicit FileListDataSource(FileListPage* page);

    int                  numberOfRows(brls::RecyclerFrame* recycler, int section) override;
    brls::RecyclerCell*  cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
    void                 didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath index) override;

    std::vector<FileListItem> items;

  private:
    FileListPage* m_page;
};

// ─────────────────────────────────────────────────────────────────────────────
//  FileListPage  –  pure-code file-browser page
// ─────────────────────────────────────────────────────────────────────────────
class FileListPage : public brls::Box
{
  public:
    enum class FilterMode { Whitelist, Blacklist };
    enum class LayoutMode  { ListOnly, ListAndDetail };

    FileListPage();
    ~FileListPage() override;

    // ── Navigation ──────────────────────────────────────────────────────────
    /// Enter the given directory and refresh the list.
    void navigateTo(const std::string& path);

    // ── Filter ──────────────────────────────────────────────────────────────
    /// Set the suffix filter list and mode.
    /// In Whitelist mode only listed suffixes are shown.
    /// In Blacklist mode listed suffixes are hidden.
    void setFilter(const std::vector<std::string>& suffixes,
                   FilterMode mode = FilterMode::Whitelist);
    void setFilterEnabled(bool enabled);

    // ── File callbacks ───────────────────────────────────────────────────────
    /// Register a callback invoked when a file with the given suffix is activated.
    void setFileCallback(const std::string& suffix,
                         std::function<void(const FileListItem&)> cb);
    /// Fallback used when no suffix-specific callback is registered.
    void setDefaultFileCallback(std::function<void(const FileListItem&)> cb);

    // ── Layout ──────────────────────────────────────────────────────────────
    void      setLayoutMode(LayoutMode mode);
    LayoutMode getLayoutMode() const { return m_layoutMode; }

    // ── Clipboard ────────────────────────────────────────────────────────────
    bool             hasClipboardItem() const { return m_hasClipboard; }
    const FileListItem& getClipboardItem() const { return m_clipboardItem; }

    // ── State ────────────────────────────────────────────────────────────────
    const std::string& getCurrentPath() const { return m_currentPath; }

    // ── Called by DataSource / Cell ──────────────────────────────────────────
    void onItemFocused(int index);
    void onItemActivated(int index);
    void openSidebar(int itemIndex);

    // ── Settings panel callback (set by StartPageView) ───────────────────────
    /// Called when the user presses BUTTON_X to open the settings / operation panel.
    /// The host (StartPageView) should display its own overlay panel.
    std::function<void(const FileListItem&, int)> onOpenSettings;

    // ── File-operation helpers exposed for external callers ──────────────────
    void doRenamePublic(int itemIndex)    { doRename(itemIndex); }
    void doSetMappingPublic(int itemIndex){ doSetMapping(itemIndex); }
    void doCutPublic(int itemIndex)       { doCut(itemIndex); }
    void doPastePublic()                  { doPaste(); }
    void doDeletePublic(int itemIndex)    { doDelete(itemIndex); }
    void doNewFolder();

  private:
    // ── UI components ────────────────────────────────────────────────────────
    beiklive::UI::BrowserHeader*    m_header      = nullptr;
    brls::Box*       m_contentBox  = nullptr; ///< row box: list + optional detail
    brls::Box*       m_listBox     = nullptr; ///< contains recycler
    brls::RecyclerFrame* m_recycler = nullptr;

    // Detail panel (LayoutMode::ListAndDetail)
    brls::Box*   m_detailPanel      = nullptr;
    brls::Image* m_detailThumb      = nullptr;
    brls::Label* m_detailName       = nullptr;
    brls::Label* m_detailMappedName = nullptr;
    brls::Label* m_detailInfo       = nullptr;

    brls::BottomBar* m_bottomBar = nullptr;

    // ── Data ─────────────────────────────────────────────────────────────────
    std::string         m_currentPath;
    FileListDataSource* m_dataSource = nullptr;

    // ── Drive-list mode (Windows only) ───────────────────────────────────────
    bool m_inDriveListMode = false; ///< true when showing Windows drive letters

    // ── Clipboard ────────────────────────────────────────────────────────────
    FileListItem m_clipboardItem;
    bool         m_hasClipboard = false;

    // ── Filter ──────────────────────────────────────────────────────────────
    std::vector<std::string> m_filterSuffixes;
    FilterMode               m_filterMode    = FilterMode::Whitelist;
    bool                     m_filterEnabled = false;

    // ── Callbacks ────────────────────────────────────────────────────────────
    std::unordered_map<std::string, std::function<void(const FileListItem&)>> m_fileCallbacks;
    std::function<void(const FileListItem&)> m_defaultFileCallback;

    // ── Layout ──────────────────────────────────────────────────────────────
    LayoutMode m_layoutMode = LayoutMode::ListOnly;

    // ── Internal helpers ─────────────────────────────────────────────────────
    void buildUI();
    void buildDetailPanel();
    void refreshList(const std::string& path);
    bool passesFilter(const std::string& suffix) const;
    void openItem(const FileListItem& item);
    void navigateUp();
    void updateHeader();
    void updateDetailPanel(const FileListItem& item);
    void clearDetailPanel();
    std::string lookupMappedName(const std::string& name, bool isDir) const;
    std::string lookupLogoPath(const std::string& name, bool isDir) const;

    // Drive-list helpers (Windows)
    void showDriveList();

    // Sidebar actions
    void doRename(int itemIndex);
    void doSetMapping(int itemIndex);
    void doCut(int itemIndex);
    void doPaste();
    void doDelete(int itemIndex);
};