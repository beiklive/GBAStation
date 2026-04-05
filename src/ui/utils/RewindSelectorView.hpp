#pragma once

#include "core/common.h"
#include "ui/utils/GameView.hpp"

#include <borealis/views/h_scrolling_frame.hpp>
#include <functional>
#include <vector>

namespace beiklive
{
    /// 单个倒带缩略图卡片（在 RewindSelectorView 的 HScrollingFrame 内）
    class RewindThumbCard : public brls::Box
    {
    public:
        /// @param thumbData  RGB565 缩略图数据（宽×高个 uint16_t）
        /// @param frameIndex 该帧在采样结果中的索引
        /// @param timeHint   大致时间提示字符串（如 "-2s"）
        RewindThumbCard(const std::vector<uint16_t>& thumbData, int frameIndex, const std::string& timeHint);
        ~RewindThumbCard();

        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;
        void onFocusGained() override;
        void onFocusLost() override;

        int  getFrameIndex() const { return m_frameIndex; }
        bool hasThumbnail()  const { return m_hasThumb; }

    private:
        int                    m_frameIndex = 0;
        std::string            m_timeHint;
        bool                   m_hasThumb  = false;
        bool                   m_focused   = false;

        std::vector<uint8_t>   m_rgbaData;  ///< RGBA8888 数据（从 RGB565 转换而来，用于 nvgCreateImageRGBA）
        int                    m_nvgImage  = -1; ///< NVG 图像句柄（懒创建，首次 draw 时上传）
    };

    /// 可视化倒带选择界面
    ///
    /// 显示一行水平可滚动的缩略图卡片，代表倒带缓冲区的历史帧。
    /// 由 GamePage 以绝对定位方式添加到视图树，初始隐藏（GONE），
    /// 倒带开始时通过动画滑入底部。
    class RewindSelectorView : public brls::Box
    {
    public:
        RewindSelectorView(GameView* gameView);
        ~RewindSelectorView() = default;

        void draw(NVGcontext* vg, float x, float y, float width, float height,
                  brls::Style style, brls::FrameContext* ctx) override;

        /// 刷新缩略图列表（从 GameView 采样最新的倒带帧）
        void refreshThumbnails();

        /// 设置"选择某帧恢复"的回调（参数为帧在采样结果中的索引）
        void setOnSelectFrame(std::function<void(int)> cb) { m_onSelectFrame = std::move(cb); }

    private:
        GameView*              m_gameView    = nullptr; ///< 关联的游戏视图（用于采样帧）
        brls::HScrollingFrame* m_scrollFrame = nullptr; ///< 水平滚动容器
        brls::Box*             m_cardBox     = nullptr; ///< 卡片容器（行方向）

        std::function<void(int)> m_onSelectFrame; ///< 选帧回调

        void _buildHeader();
        void _buildScrollArea();
    };

} // namespace beiklive
