#pragma once

#include <borealis.hpp>
#include <functional>
#include <string>

#include "core/common.h"

namespace beiklive
{

    /// GridItem 的显示模式
    enum class GridItemMode
    {
        GAME_LIBRARY, ///< 游戏库模式：显示封面、平台徽标、游戏名、上次游玩时间、游玩时长
        SAVE_STATE,   ///< 存档模式：显示槽位名称、存档时间，无封面时居中显示槽位名
    };

    /// 平台徽标颜色（GAME_LIBRARY 模式使用）
    enum class PlatformBadgeColor
    {
        GBA,  ///< GBA 平台 – 紫色
        GBC,  ///< GBC 平台 – 蓝色
        GB,   ///< GB  平台 – 绿色
        NONE, ///< 无徽标
    };

    /**
     * GridItem – 用于 GridBox 的网格列表单元格控件
     *
     * 布局（横向）：
     *   [正方形图片] | [右侧纵向布局]
     *                      Row1: [徽标Box] [主标题Label]
     *                      Row2: [时间Label]
     *                      Row3: [游玩时长Label]（SAVE_STATE 模式下隐藏）
     *
     * 空状态（未加载数据）时，居中显示一个 label（显示槽位名称）。
     * 所有 label 均为单行滚动（setAnimated + setSingleLine）。
     */
    class GridItem : public brls::Box
    {
    public:
        static constexpr float ITEM_HEIGHT = 120.f; ///< 固定高度（px）

        explicit GridItem(GridItemMode mode, int index = 0);
        ~GridItem() = default;

        // ── 数据设置 ──────────────────────────────────────────────────────────

        /// 设置左侧正方形封面图片路径
        void setImagePath(const std::string& path);

        /**
         * 设置平台徽标（GAME_LIBRARY 模式）
         * @param text   徽标显示文字，如 "GBA"、"GBC"、"GB"
         * @param color  徽标背景颜色枚举
         */
        void setBadge(const std::string& text, PlatformBadgeColor color);

        /**
         * 设置主标题文字
         * GAME_LIBRARY: 游戏名称
         * SAVE_STATE:   "自动存档" / "槽位 N"
         */
        void setTitle(const std::string& title);

        /**
         * 设置第二行文字（日期时间字符串）
         * GAME_LIBRARY: 上次游玩时间
         * SAVE_STATE:   存档文件的修改时间
         */
        void setSubText(const std::string& text);

        /**
         * 设置第三行文字（游玩时长）
         * 仅 GAME_LIBRARY 模式下生效；SAVE_STATE 模式此行恒为隐藏
         * @param text  如 "12 小时 34 分钟"；若为空字符串则显示 "未游玩"
         */
        void setPlayTime(const std::string& text);

        /**
         * 切换到空状态：隐藏数据布局，居中显示 emptyLabel
         * @param slotName  槽位名称文字（SAVE_STATE: "自动存档" / "槽位 N"）
         */
        void setEmpty(const std::string& slotName);

        /// 切换到有数据状态：隐藏 emptyLabel，显示数据布局
        void setDataLoaded();

        // ── 查询 ──────────────────────────────────────────────────────────────

        int           getIndex() const { return m_index; }
        GridItemMode  getMode()  const { return m_mode;  }
        bool          isEmpty()  const { return m_isEmpty; }

        // ── 点击回调 ──────────────────────────────────────────────────────────

        /// 被点击（A键）时触发，参数为本控件的索引
        std::function<void(int index)> onItemClicked;

    private:
        GridItemMode m_mode;
        int          m_index;
        bool         m_isEmpty = true;

        // ── 空状态 ────────────────────────────────────────────────────────────
        brls::Label* m_emptyLabel = nullptr;

        // ── 有数据状态主布局 ──────────────────────────────────────────────────
        brls::Box*   m_dataLayout  = nullptr; ///< 横向主容器

        // 左侧：正方形封面图
        brls::Image* m_image       = nullptr;

        // 右侧：纵向容器
        brls::Box*   m_rightBox    = nullptr;

        // Row1：徽标 + 主标题
        brls::Box*   m_row1        = nullptr;
        brls::Box*   m_badgeBox    = nullptr; ///< 徽标背景框
        brls::Label* m_badgeLabel  = nullptr; ///< 徽标文字
        brls::Label* m_titleLabel  = nullptr; ///< 主标题

        // Row2：时间文字
        brls::Label* m_subLabel    = nullptr;

        // Row3：游玩时长（SAVE_STATE 模式下 GONE）
        brls::Label* m_playLabel   = nullptr;

        void _initLayout();

        /// 获取平台徽标背景色
        static NVGcolor _getBadgeColor(PlatformBadgeColor color);

        void onFocusGained() override;
        void onFocusLost()   override;
    };

} // namespace beiklive
