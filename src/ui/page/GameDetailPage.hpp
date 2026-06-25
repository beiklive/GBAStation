#pragma once

#include "core/common.h"
#include "ui/widget/Box.hpp"
#include "ui/widget/ButtonBox.hpp"
#include "ui/widget/GridBox.hpp"
#include "ui/widget/GridItem.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace beiklive
{
    /**
     * SaveFileInfo – 单个存档文件的信息
     */
    struct SaveFileInfo
    {
        int         slot      = -1;   ///< 存档槽位编号（-1 = 未知）
        std::string statePath;        ///< 存档文件完整路径
        std::string thumbPath;        ///< 缩略图路径（可能为空）
        std::string timeStr;          ///< 存档修改时间字符串
        bool        exists    = false;
    };

    /**
     * GameDetailPage – 游戏详情页面
     *
     * 布局：左右分栏
     *   - 左侧：功能列表按钮（存档 / 金手指 / 成就）
     *   - 右侧：对应内容面板
     *     - 存档面板：GridBox，每项对应一个存档槽位，X 键删除
     *     - 金手指面板：读取 .cht 文件，ButtonBox 列表展示，X 改代码/Y 删除/ZR 改名
     *     - 成就面板：占位（待实现）
     */
    class GameDetailPage : public beiklive::Box
    {
    public:
        explicit GameDetailPage(const beiklive::GameEntry& entry);
        ~GameDetailPage() = default;

    private:
        beiklive::GameEntry m_entry; ///< 对应游戏信息

        // ── 布局组件 ──────────────────────────────────────────────────────
        brls::Box* m_leftPanel  = nullptr;
        brls::Box* m_rightPanel = nullptr;

        // ── 右侧面板 ──────────────────────────────────────────────────────
        brls::View* m_savePanel    = nullptr; ///< 存档面板
        brls::Box*  m_cheatPanel   = nullptr; ///< 金手指面板
        brls::Box*  m_achievePanel = nullptr; ///< 成就面板（占位）

        std::vector<brls::View*>        m_allPanels;
        beiklive::GridBox*              m_saveGrid = nullptr;
        std::vector<beiklive::GridItem*> m_saveItems;

        // ── 金手指面板组件 ────────────────────────────────────────────────
        brls::Box*                      m_cheatListBox = nullptr; ///< 金手指按钮容器
        brls::ScrollingFrame*           m_cheatScroll  = nullptr;
        std::vector<beiklive::CheatEntry> m_cheatEntries;          ///< 当前解析的金手指条目

        void _initLayout();
        void _hideAllPanels();

        /// 创建左侧菜单按钮，聚焦时切换右侧面板
        beiklive::ButtonBox* _createMenuButton(const std::string& text,
                                               brls::View* panel);

        /// 创建存档面板（扫描并显示该游戏的所有存档槽位）
        brls::View* _createSavePanel();

        /// 异步扫描存档目录并刷新列表
        void _refreshSaveList();

        /// 删除指定槽位的存档文件和缩略图
        void _deleteSaveFile(int slot);

        /// 获取存档文件路径：{saveDir}/{stem}.ss{slot}
        std::string _getStatePath(int slot) const;
        std::string _getStateThumbPath(int slot) const;

        /// 格式化存档槽位名称
        static std::string _slotName(int slot);

        // ── 金手指面板 ────────────────────────────────────────────────────

        /// 确定金手指文件路径（优先使用 entry.cheatPath，否则使用默认路径）
        std::string _getCheatPath() const;

        /// 创建金手指面板
        brls::View* _createCheatPanel();

        /// 刷新金手指列表
        void _refreshCheatList();

        /// 保存金手指到文件
        void _saveCheats();

        /// 删除指定索引的金手指条目
        void _deleteCheat(int index);

        // ── 成就面板（占位）────────────────────────────────────────────────
        brls::View* _createAchievePanel();
    };

} // namespace beiklive
