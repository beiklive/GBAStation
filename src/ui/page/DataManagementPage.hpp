#pragma once

#include "core/common.h"
#include "ui/utils/Box.hpp"
#include "ui/utils/ButtonBox.hpp"
#include "ui/utils/GridBox.hpp"
#include "ui/utils/GridItem.hpp"

namespace beiklive
{
    /**
     * DataManagementPage – 数据管理页面
     *
     * 布局：左右分栏
     *   - 左侧：平台按钮列表（GBA / GBC / GB），聚焦时切换右侧游戏列表
     *   - 右侧：对应平台下的游戏列表（GridItem，GAME_LIBRARY 模式）
     *
     * 点击游戏条目：弹出 GameDetailPage，展示该游戏的存档/金手指/成就。
     */
    class DataManagementPage : public beiklive::Box
    {
    public:
        DataManagementPage();
        ~DataManagementPage() = default;

    private:
        // ── 布局组件 ──────────────────────────────────────────────────────
        brls::Box*  m_leftPanel  = nullptr; ///< 左侧平台按钮栏
        brls::Box*  m_rightPanel = nullptr; ///< 右侧游戏列表区域

        // 当前选中平台的游戏列表面板
        brls::Box*              m_gameListPanel = nullptr;
        beiklive::GridBox*      m_grid          = nullptr;
        std::vector<GameEntry>  m_currentEntries;

        // ── 平台按钮 ──────────────────────────────────────────────────────
        beiklive::ButtonBox* m_btnGBA = nullptr;
        beiklive::ButtonBox* m_btnGBC = nullptr;
        beiklive::ButtonBox* m_btnGB  = nullptr;

        void _initLayout();

        /**
         * 创建平台选择按钮
         * @param text      按钮显示文字
         * @param platform  对应的游戏平台枚举
         */
        beiklive::ButtonBox* _createPlatformButton(const std::string& text,
                                                   beiklive::enums::EmuPlatform platform);

        /// 从数据库加载指定平台的游戏并刷新右侧列表
        void _loadPlatformGames(beiklive::enums::EmuPlatform platform);

        /// 格式化游玩时长（秒 → 可读字符串）
        static std::string _formatPlayTime(int seconds);

        /// 平台 → GridItem 徽标颜色
        static PlatformBadgeColor _platformBadge(beiklive::enums::EmuPlatform platform);
    };

} // namespace beiklive
