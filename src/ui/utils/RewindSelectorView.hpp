#pragma once

#include "core/common.h"
#include "ui/utils/GameView.hpp"

#include <borealis/views/h_scrolling_frame.hpp>
#include <functional>
#include <vector>
#include <cstdint>

namespace beiklive
{
    /// 倒带缩略图卡片 – 在 HScrollingFrame 内显示单帧缩略图
    class RewindThumbItem : public brls::Box
    {
    public:
        static constexpr float ITEM_W = 160.f; ///< 卡片宽度（像素）
        static constexpr float ITEM_H = 130.f; ///< 卡片高度（像素）

        /// @param frameIndex  对应 m_rewindBuffer 中的帧索引
        /// @param thumb       RGB565 缩略图数据（可能为空）
        RewindThumbItem(int frameIndex, const std::vector<uint16_t>& thumb);
        ~RewindThumbItem();

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        int getFrameIndex() const { return m_frameIndex; }

        /// 点击回调（由 RewindSelectorView 注入）
        std::function<void(int frameIndex)> onItemClicked;

    private:
        int  m_frameIndex;
        int  m_nvgImage    = 0;    ///< NanoVG 图像句柄（0 表示无缩略图）
        bool m_imgCreated  = false;

        /// RGBA8888 像素数据（由 RGB565 转换，首次 draw 时转为 NVG 图像）
        std::vector<uint8_t> m_rgbaData;

        brls::Label* m_indexLabel  = nullptr; ///< 帧序号标签（底部）
        brls::Label* m_noImgLabel  = nullptr; ///< 无缩略图占位标签（居中）

        void _createNvgImage(NVGcontext* vg); ///< 首次 draw 时创建 NVG 图像

        void onFocusGained() override;
        void onFocusLost()   override;
    };

    // =========================================================================

    /// 可视化倒带选择界面
    ///
    /// 以横向滚动卡片列表展示倒带历史缩略图，供用户选择要恢复的历史时刻。
    /// 由 GamePage 创建并注入 GameView 引用，当倒带键按下且"显示倒带界面"设置
    /// 开启时，从底部弹出并获得焦点；用户选中后恢复对应状态并关闭界面。
    class RewindSelectorView : public brls::Box
    {
    public:
        RewindSelectorView();
        ~RewindSelectorView() = default;

        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        /// 打开倒带界面：用传入的缩略图快照重建卡片列表
        /// @param frames  (帧索引, RGB565缩略图) 对列表（最新帧在前）
        void openWithFrames(std::vector<std::pair<int, std::vector<uint16_t>>> frames);

        /// 将焦点设置到最右侧卡片（最新帧）
        void focusNewest();

        /// 设置"恢复帧"回调（由 GamePage 注入，参数为 m_rewindBuffer 帧索引）
        void setOnFrameSelected(std::function<void(int)> cb) { m_onFrameSelected = std::move(cb); }

        /// 设置"关闭"回调（由 GamePage 注入，关闭时调用）
        void setOnClose(std::function<void()> cb) { m_onClose = std::move(cb); }

    private:
        brls::Box*            m_panel       = nullptr; ///< 半透明背景面板
        brls::Box*            m_titleRow    = nullptr; ///< 标题行容器
        brls::Label*          m_titleLabel  = nullptr; ///< "可视化倒带" 标题
        brls::Label*          m_hintLabel   = nullptr; ///< 操作提示文字
        brls::HScrollingFrame* m_scrollFrame = nullptr; ///< 横向滚动帧容器
        brls::Box*            m_itemBox     = nullptr; ///< 卡片列表容器（ROW 方向）

        std::vector<RewindThumbItem*> m_items; ///< 当前显示的缩略图卡片列表

        std::function<void(int)> m_onFrameSelected;
        std::function<void()>    m_onClose;

        void _initLayout();
        void _clearItems();
    };

} // namespace beiklive
