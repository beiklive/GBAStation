#include "AnimationHelper.hpp"

namespace beiklive
{
    // ============================================================
    // 内部辅助结构：封装单个 Animatable 和目标视图，实现自动生命周期管理。
    //
    // 分配在堆上。在 endCallback 中执行 delete this。
    //
    // 安全性分析：
    //   borealis Ticking::stop(finished) 执行流程：
    //     1. 从 runningTickings 中移除 this（Animatable 指针）
    //     2. 将 running 置为 false
    //     3. 调用 endCallback(finished)  ← delete this 在此处执行
    //     4. 返回到 updateTickings()     ← 不再访问已删除的指针
    //   updateTickings() 在调用 stop() 之前已将所有 tickings 复制到局部 vector，
    //   因此 delete this 之后，循环继续访问的是局部 copy，不会解引用已删除的指针。
    //   此模式依赖于当前 borealis 实现（library/lib/core/time.cpp）的执行顺序。
    // ============================================================
    struct FadeAnim
    {
        brls::View*           view;
        brls::Animatable      anim{ 0.0f };
        float                 targetAlpha;
        bool                  goneAfter;
        std::function<void()> onComplete;

        /// 启动淡变动画
        void start(float fromAlpha, float toAlpha, int durationMs,
                   brls::EasingFunction easing = brls::EasingFunction::quadraticOut)
        {
            targetAlpha = toAlpha;
            anim.reset(fromAlpha);
            anim.addStep(toAlpha, durationMs, easing);
            anim.setTickCallback([this]() {
                view->setAlpha(anim.getValue());
            });
            anim.setEndCallback([this](bool) {
                view->setAlpha(targetAlpha);
                if (goneAfter) view->setVisibility(brls::Visibility::GONE);
                if (onComplete) onComplete();
                delete this; // 参见上方安全性分析
            });
            anim.start();
        }
    };

    struct SlideAnim
    {
        brls::View*           view;
        brls::Animatable      anim{ 0.0f };
        float                 toY;
        bool                  goneAfter;
        std::function<void()> onComplete;

        /// 启动平移动画
        void start(float fromY, float targetY, int durationMs,
                   brls::EasingFunction easing = brls::EasingFunction::quadraticOut)
        {
            toY = targetY;
            anim.reset(fromY);
            anim.addStep(targetY, durationMs, easing);
            anim.setTickCallback([this]() {
                view->setTranslationY(anim.getValue());
            });
            anim.setEndCallback([this](bool) {
                view->setTranslationY(toY);
                if (goneAfter) view->setVisibility(brls::Visibility::GONE);
                if (onComplete) onComplete();
                delete this; // 参见 FadeAnim 的安全性分析
            });
            anim.start();
        }
    };

    // ============================================================
    // fadeIn – 淡入动画
    // ============================================================
    void AnimationHelper::fadeIn(brls::View* view, int durationMs,
                                 std::function<void()> onComplete)
    {
        if (!view) return;

        // 先设置为可见（从 GONE 恢复时 YG 布局节点需要重新激活）
        view->setVisibility(brls::Visibility::VISIBLE);
        view->setAlpha(0.0f);

        auto* fa       = new FadeAnim();
        fa->view       = view;
        fa->goneAfter  = false;
        fa->onComplete = std::move(onComplete);
        fa->start(0.0f, 1.0f, durationMs, brls::EasingFunction::backOut);
    }

    // ============================================================
    // fadeOut – 淡出动画
    // ============================================================
    void AnimationHelper::fadeOut(brls::View* view, int durationMs,
                                  bool goneAfter,
                                  std::function<void()> onComplete)
    {
        if (!view) return;

        auto* fa       = new FadeAnim();
        fa->view       = view;
        fa->goneAfter  = goneAfter;
        fa->onComplete = std::move(onComplete);
        fa->start(1.0f, 0.0f, durationMs, brls::EasingFunction::backIn);
    }

    // ============================================================
    // slideInFromBottom – 从底部滑入
    // ============================================================
    void AnimationHelper::slideInFromBottom(brls::View* view, float distance,
                                            int durationMs,
                                            std::function<void()> onComplete)
    {
        if (!view) return;

        view->setVisibility(brls::Visibility::VISIBLE);
        view->setAlpha(1.0f);
        view->setTranslationY(distance);

        auto* sa       = new SlideAnim();
        sa->view       = view;
        sa->goneAfter  = false;
        sa->onComplete = std::move(onComplete);
        sa->start(distance, 0.0f, durationMs, brls::EasingFunction::backOut);
    }

    // ============================================================
    // slideOutToBottom – 滑出到底部
    // ============================================================
    void AnimationHelper::slideOutToBottom(brls::View* view, float distance,
                                           int durationMs, bool goneAfter,
                                           std::function<void()> onComplete)
    {
        if (!view) return;

        auto* sa       = new SlideAnim();
        sa->view       = view;
        sa->goneAfter  = goneAfter;
        sa->onComplete = std::move(onComplete);
        sa->start(0.0f, distance, durationMs, brls::EasingFunction::backIn);
    }

    // ============================================================
    // pushActivity – 推入 Activity（封装 brls::Application::pushActivity）
    // ============================================================
    void AnimationHelper::pushActivity(brls::Activity* activity,
                                       brls::TransitionAnimation animation)
    {
        brls::Application::pushActivity(activity, animation);
    }

    // ============================================================
    // popActivity – 弹出当前 Activity（封装 brls::Application::popActivity）
    // ============================================================
    void AnimationHelper::popActivity(brls::TransitionAnimation animation,
                                      std::function<void()> cb,
                                      bool freeView)
    {
        brls::Application::popActivity(animation, cb, freeView);
    }

} // namespace beiklive
