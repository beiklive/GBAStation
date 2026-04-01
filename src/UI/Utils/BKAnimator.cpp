#include "UI/Utils/BKAnimator.hpp"

namespace beiklive {

void BKAnimator::showView(brls::View* view,
                          std::function<void()> cb,
                          int durationMs)
{
    if (!view) return;

    // 先确保视图可见（GONE 状态下 show() 无效果）
    view->setVisibility(brls::Visibility::VISIBLE);

    // 以淡入动画从 alpha=0 渐变到 alpha=1
    view->show([cb]() {
        if (cb) cb();
    }, true, static_cast<float>(durationMs));
}

void BKAnimator::hideView(brls::View* view,
                          std::function<void()> cb,
                          int durationMs)
{
    if (!view) return;

    // 以淡出动画从 alpha=1 渐变到 alpha=0，动画完成后设置 GONE 并执行回调
    view->hide([view, cb]() {
        view->setVisibility(brls::Visibility::GONE);
        if (cb) cb();
    }, true, static_cast<float>(durationMs));
}

void BKAnimator::pushActivity(brls::Activity* activity)
{
    if (!activity) return;
    // 使用滑动动画推入：新页面从右侧滑入，旧页面向左退出
    brls::Application::pushActivity(activity, brls::TransitionAnimation::SLIDE_LEFT);
}

void BKAnimator::popActivity(std::function<void()> cb)
{
    // 使用淡出动画弹出当前 Activity
    brls::Application::popActivity(brls::TransitionAnimation::FADE, cb ? std::move(cb) : []() {});
}

} // namespace beiklive
