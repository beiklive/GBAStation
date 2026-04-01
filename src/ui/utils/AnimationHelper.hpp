#pragma once

#include <borealis.hpp>
#include <functional>

namespace beiklive
{
    /// UI 动画辅助类
    ///
    /// 提供 View 显隐动画（淡入/淡出）和平移动画（从底部滑入/滑出），
    /// 以及 Activity push/pop 时的过渡动画封装。
    ///
    /// 所有动画均基于 brls::Animatable（Ticking 系统），在 UI 线程中运行。
    /// 调用任意方法前，当前线程必须是 borealis 主线程（或通过 brls::sync 调用）。
    class AnimationHelper
    {
    public:
        // ---- View 显隐动画 ---------------------------------------------------

        /// 淡入动画：设置 visibility 为 VISIBLE，alpha 从 0 渐变至 1。
        /// @param view         目标视图
        /// @param durationMs   动画时长（毫秒，默认 200ms）
        /// @param onComplete   动画完成后的回调（可为空）
        static void fadeIn(brls::View* view, int durationMs = 200,
                           std::function<void()> onComplete = {});

        /// 淡出动画：alpha 从 1 渐变至 0，动画完成后可选将 visibility 设为 GONE。
        /// @param view         目标视图
        /// @param durationMs   动画时长（毫秒，默认 200ms）
        /// @param goneAfter    动画完成后是否设为 GONE（默认 true）
        /// @param onComplete   动画完成后的回调（可为空）
        static void fadeOut(brls::View* view, int durationMs = 200,
                            bool goneAfter = true,
                            std::function<void()> onComplete = {});

        // ---- View 平移动画 ---------------------------------------------------

        /// 从底部滑入动画：先设置 visibility 为 VISIBLE，再将 translateY 从 +distance 滑至 0。
        /// @param view         目标视图
        /// @param distance     初始偏移量（像素，默认 60px）
        /// @param durationMs   动画时长（毫秒，默认 250ms）
        /// @param onComplete   动画完成后的回调（可为空）
        static void slideInFromBottom(brls::View* view, float distance = 60.f,
                                      int durationMs = 250,
                                      std::function<void()> onComplete = {});

        /// 滑出到底部动画：将 translateY 从 0 滑至 +distance，完成后可选设为 GONE。
        /// @param view         目标视图
        /// @param distance     终止偏移量（像素，默认 60px）
        /// @param durationMs   动画时长（毫秒，默认 250ms）
        /// @param goneAfter    动画完成后是否设为 GONE（默认 true）
        /// @param onComplete   动画完成后的回调（可为空）
        static void slideOutToBottom(brls::View* view, float distance = 60.f,
                                     int durationMs = 250, bool goneAfter = true,
                                     std::function<void()> onComplete = {});

        // ---- Activity 过渡动画 -----------------------------------------------

        /// 推入 Activity（带过渡动画，默认淡入）。
        /// @param activity     要推入的 Activity（所有权由 borealis 管理）
        /// @param animation    过渡动画类型（默认 FADE）
        static void pushActivity(brls::Activity* activity,
                                 brls::TransitionAnimation animation = brls::TransitionAnimation::FADE);

        /// 弹出当前 Activity（带过渡动画，默认淡出）。
        /// @param animation    过渡动画类型（默认 FADE）
        /// @param cb           弹出完成后的回调
        /// @param freeView     是否释放弹出的 Activity 内存（默认 true）
        static void popActivity(brls::TransitionAnimation animation = brls::TransitionAnimation::FADE,
                                std::function<void()> cb = {},
                                bool freeView = true);
    };

} // namespace beiklive
