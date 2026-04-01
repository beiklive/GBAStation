#pragma once

#include <borealis.hpp>
#include <functional>

namespace beiklive {

/// 视图与Activity动画工具类。
///
/// 封装视图淡入/淡出动画和 Activity 切换动画，
/// 用于统一管理所有UI出场和入场效果。
///
/// 典型用法：
/// @code
///   // 淡入显示视图
///   BKAnimator::showView(menuView);
///
///   // 淡出后设置 GONE
///   BKAnimator::hideView(menuView, [=]() { /* 完成后回调 */ });
///
///   // 带滑动动画推入 Activity
///   BKAnimator::pushActivity(new brls::Activity(frame));
///
///   // 带淡出动画弹出 Activity
///   BKAnimator::popActivity();
/// @endcode
class BKAnimator {
public:
    /// 淡入显示视图。
    /// 先将视图可见性设为 VISIBLE，再以淡入动画从透明渐变到不透明。
    /// @param view        目标视图（不得为空）
    /// @param cb          动画完成后的回调（可为空）
    /// @param durationMs  动画时长（毫秒），默认 200ms
    static void showView(brls::View* view,
                         std::function<void()> cb = {},
                         int durationMs = 200);

    /// 淡出隐藏视图。
    /// 先以淡出动画从不透明渐变到透明，动画完成后将视图可见性设为 GONE。
    /// @param view        目标视图（不得为空）
    /// @param cb          动画完成且视图已隐藏后的回调（可为空）
    /// @param durationMs  动画时长（毫秒），默认 200ms
    static void hideView(brls::View* view,
                         std::function<void()> cb = {},
                         int durationMs = 200);

    /// 带滑动入场动画推入 Activity。
    /// 新 Activity 从右侧滑入，旧 Activity 向左淡出。
    /// @param activity  要推入的 Activity（不得为空）
    static void pushActivity(brls::Activity* activity);

    /// 带淡出动画弹出当前 Activity。
    /// @param cb  Activity 完成弹出后的回调（可为空）
    static void popActivity(std::function<void()> cb = {});
};

} // namespace beiklive
