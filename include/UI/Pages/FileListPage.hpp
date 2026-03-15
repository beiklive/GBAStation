#pragma once

#include <borealis.hpp>

#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "common.hpp"
#include "UI/Utils/Utils.hpp"
#include "UI/Utils/ProImage.hpp"

// ─────────────────────────────────────────────────────────────────────────────
//  FileListItem  –  文件列表中的单条目
// ─────────────────────────────────────────────────────────────────────────────
struct FileListItem
{
    std::string fileName;   ///< 文件/目录名（不含路径）
    std::string mappedName; ///< NameMappingManager 的显示别名（空=无）
    bool        isDir       = false;
    std::string fullPath;   ///< 绝对路径
    std::uintmax_t fileSize = 0; ///< 文件大小（字节，仅文件）
    int         childCount  = 0; ///< 直接子项数量（仅目录）
    std::string logoPath;   ///< logo 图片路径（可为空）

    /// 优先返回 mappedName，否则返回 fileName
    const std::string& displayName() const
    {
        return mappedName.empty() ? fileName : mappedName;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
//  前向声明
// ─────────────────────────────────────────────────────────────────────────────
class FileListPage;

// ─────────────────────────────────────────────────────────────────────────────
//  FileListItemView  –  滚动列表的单行视图（无复用）
//  直接继承 brls::Box，焦点管理简单，避免 RecyclerFrame LIFO 复用问题。
// ─────────────────────────────────────────────────────────────────────────────
class FileListItemView : public brls::Box
{
  public:
    FileListItemView();

    /// 用条目数据填充视图，index 用于回调。
    void setItem(const FileListItem& item, int index);

    std::function<void(int)> onItemFocused;
    std::function<void(int)> onItemActivated;
    std::function<void(int)> onItemOptions;

    void onFocusGained() override;
    void onFocusLost() override;

  private:
    brls::Rectangle* m_accent    = nullptr;
    brls::Image*     m_icon      = nullptr;
    brls::Label*     m_nameLabel = nullptr;
    brls::Label*     m_infoLabel = nullptr;
    int              m_index     = -1;
};

// ─────────────────────────────────────────────────────────────────────────────
//  FileListPage  –  纯代码文件浏览页面
// ─────────────────────────────────────────────────────────────────────────────
class FileListPage : public beiklive::UI::BBox
{
  public:
    enum class FilterMode { Whitelist, Blacklist };
    enum class LayoutMode  { ListOnly, ListAndDetail };

    FileListPage();
    ~FileListPage() override;

    // ── 导航 ────────────────────────────────────────────────────────────────
    /// 进入指定目录并刷新列表。
    void navigateTo(const std::string& path);

    /// 将焦点重置到列表第一项（页面切换后使用）。
    void resetFocusToTop();

    // ── 过滤器 ──────────────────────────────────────────────────────────────
    /// 设置后缀过滤列表和模式。
    /// 白名单模式：只显示列出的后缀；黑名单模式：隐藏列出的后缀。
    void setFilter(const std::vector<std::string>& suffixes,
                   FilterMode mode = FilterMode::Whitelist);
    void setFilterEnabled(bool enabled);

    // ── 文件回调 ─────────────────────────────────────────────────────────────
    /// 注册指定后缀文件被激活时的回调。
    void setFileCallback(const std::string& suffix,
                         std::function<void(const FileListItem&)> cb);
    /// 无匹配后缀回调时的默认回调。
    void setDefaultFileCallback(std::function<void(const FileListItem&)> cb);

    // ── 布局 ────────────────────────────────────────────────────────────────
    void      setLayoutMode(LayoutMode mode);
    LayoutMode getLayoutMode() const { return m_layoutMode; }

    // ── 剪贴板 ──────────────────────────────────────────────────────────────
    bool             hasClipboardItem() const { return m_hasClipboard; }
    const FileListItem& getClipboardItem() const { return m_clipboardItem; }

    // ── 状态 ────────────────────────────────────────────────────────────────
    const std::string& getCurrentPath() const { return m_currentPath; }

    // ── ItemView 回调 ────────────────────────────────────────────────────────
    void onItemFocused(int index);
    void onItemActivated(int index);
    void openSidebar(int itemIndex);

    // ── 对外暴露的文件操作接口 ───────────────────────────────────────────────
    void doRenamePublic(int itemIndex)    { doRename(itemIndex); }
    void doSetMappingPublic(int itemIndex){ doSetMapping(itemIndex); }
    void doCutPublic(int itemIndex)       { doCut(itemIndex); }
    void doPastePublic()                  { doPaste(); }
    void doDeletePublic(int itemIndex)    { doDelete(itemIndex); }
    void doNewFolder();

    /// 根据文件扩展名判断 EmuPlatform。
    static beiklive::EmuPlatform detectPlatform(const std::string& fileName);

  private:
    // ── UI 组件 ──────────────────────────────────────────────────────────────
    beiklive::UI::BrowserHeader*    m_header      = nullptr;
    brls::Box*       m_contentBox  = nullptr; ///< 行容器：列表 + 可选详情面板
    brls::Box*       m_listBox     = nullptr; ///< 包含滚动框
    brls::ScrollingFrame* m_scrollFrame = nullptr; ///< 垂直滚动容器
    brls::Box*            m_itemsBox    = nullptr; ///< FileListItemView 的直接父容器

    // 详情面板（LayoutMode::ListAndDetail）
    brls::Box*              m_detailPanel      = nullptr;
    beiklive::UI::ProImage* m_detailThumb      = nullptr;
    brls::Label*            m_detailName       = nullptr;
    brls::Label*            m_detailMappedName = nullptr;
    brls::Label*            m_detailInfo       = nullptr;
    brls::Label*            m_detailLastOpen   = nullptr; ///< gamedataManager: 上次游玩
    brls::Label*            m_detailTotalTime  = nullptr; ///< gamedataManager: 总时长
    brls::Label*            m_detailPlatform   = nullptr; ///< gamedataManager: 平台

    brls::BottomBar* m_bottomBar = nullptr;

    // ── 数据 ─────────────────────────────────────────────────────────────────
    std::string         m_currentPath;
    std::vector<FileListItem> m_items; ///< 当前目录条目

    // ── 驱动器列表模式（仅 Windows）──────────────────────────────────────────
    bool m_inDriveListMode = false; ///< 正在显示 Windows 驱动器列表时为 true

    // ── 剪贴板 ──────────────────────────────────────────────────────────────
    FileListItem m_clipboardItem;
    bool         m_hasClipboard = false;

    // ── 过滤器 ──────────────────────────────────────────────────────────────
    std::vector<std::string> m_filterSuffixes;
    FilterMode               m_filterMode    = FilterMode::Whitelist;
    bool                     m_filterEnabled = false;

    // ── 回调 ─────────────────────────────────────────────────────────────────
    std::unordered_map<std::string, std::function<void(const FileListItem&)>> m_fileCallbacks;
    std::function<void(const FileListItem&)> m_defaultFileCallback;

    // ── 布局 ────────────────────────────────────────────────────────────────
    LayoutMode m_layoutMode = LayoutMode::ListOnly;

    // ── 内部辅助函数 ──────────────────────────────────────────────────────────
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
    std::string lookupLogoPath(const std::string& name) const;

    // 驱动器列表辅助（Windows）
    void showDriveList();

    // 侧栏操作
    void doRename(int itemIndex);
    void doSetMapping(int itemIndex);
    void doCut(int itemIndex);
    void doPaste();
    void doDelete(int itemIndex);

    /// 从 m_items 重建 FileListItemView，滚动到顶部并聚焦第一项。
    void rebuildItemViews();
};