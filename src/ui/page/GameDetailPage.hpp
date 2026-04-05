#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"
#include "ui/utils/ButtonBox.hpp"
#include "ui/utils/GridBox.hpp"
#include "ui/utils/GridItem.hpp"

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
     *     - 金手指面板：占位（待实现）
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
        brls::Box*  m_cheatPanel   = nullptr; ///< 金手指面板（占位）
        brls::Box*  m_achievePanel = nullptr; ///< 成就面板（占位）

        std::vector<brls::View*>        m_allPanels;
        beiklive::GridBox*              m_saveGrid = nullptr;
        std::vector<beiklive::GridItem*> m_saveItems;

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
    };

} // namespace beiklive
