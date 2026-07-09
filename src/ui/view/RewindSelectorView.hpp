#pragma once

#include "core/common.h"
#include "ui/view/GameViewBase.hpp"

#include <borealis.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <vector>

namespace beiklive
{
    /// NanoVG 自绘的可视化倒带选择界面。
    ///
    /// 缩略图按时间顺序排列：最旧帧在左，最新帧在右；打开时默认选中最新帧。
    class RewindSelectorView : public brls::Box
    {
    public:
        RewindSelectorView();
        ~RewindSelectorView() override;

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;

        /// 打开倒带界面：用传入的缩略图快照重建自绘列表。
        void openWithFrames(std::vector<RewindThumbSnapshot> frames);

        /// 设置"恢复帧"回调（由 GamePage 注入，参数为倒带缓冲区帧索引）。
        void setOnFrameSelected(std::function<void(int)> cb) { m_onFrameSelected = std::move(cb); }

        /// 设置"预览帧"回调（选中项变化时调用，参数为倒带缓冲区帧索引）。
        void setOnFrameFocused(std::function<void(int)> cb) { m_onFrameFocused = std::move(cb); }

        /// 设置"关闭"回调（由 GamePage 注入，关闭时调用）。
        void setOnClose(std::function<void()> cb) { m_onClose = std::move(cb); }

        /// 自绘面板自己接收焦点和输入。
        brls::View* getDefaultFocus() override;
        brls::View* getNextFocus(brls::FocusDirection direction, brls::View* currentView) override;

    private:
        struct ThumbItem
        {
            int frameIndex = 0;
            int secondsAgo = 0;
            int nvgImage = 0;
            bool imageCreated = false;
            unsigned width = 0;
            unsigned height = 0;
            std::vector<std::uint8_t> rgba;
        };

        std::vector<ThumbItem> m_items;
        int m_selected = 0;
        int m_font = -1;

        bool m_prevLeft = false;
        bool m_prevRight = false;
        float m_holdLeftTime = 0.0f;
        float m_holdRightTime = 0.0f;
        float m_holdLeftRepeat = 0.0f;
        float m_holdRightRepeat = 0.0f;
        std::chrono::steady_clock::time_point m_lastFrameTime;

        std::function<void(int)> m_onFrameSelected;
        std::function<void(int)> m_onFrameFocused;
        std::function<void()> m_onClose;

        void _clearItems(NVGcontext* vg = nullptr);
        void _captureInputState();
        void _moveSelection(int delta);
        void _notifyFrameFocused();
        void _selectCurrent();
        void _close();
        void _createImage(NVGcontext* vg, ThumbItem& item);
        void _drawText(NVGcontext* vg, float x, float y, float size, NVGcolor color,
                       int align, const char* text) const;
        void _drawFocusBorder(NVGcontext* vg, float x, float y, float w, float h, float alpha) const;
    };

} // namespace beiklive
