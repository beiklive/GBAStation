#pragma once

#include <borealis.hpp>
#include <borealis/core/animation.hpp>
#include <chrono>
#include "Header.hpp"
#include "DynamicBackgroundBox.hpp"
#include "GifBackgroundView.hpp"
#include "VideoBackgroundView.hpp"

namespace beiklive
{
    class HeaderBar; // forward declaration to break circular dependency

    class Box : public brls::Box
    {
    public:
        Box();
        Box(brls::Axis flexDirection);
        ~Box();
    
        beiklive::HeaderBar* getHeader() { return header; }
        brls::BottomBar* getBottomBar() { return bottomBar; }
        void showHeader(bool show);
        void showFooter(bool show);
        void hideFooterLine(){ bottomBar->hideLineTop(); }
        void showBackground(bool show);
        void setBackgroundImage(const std::string& path, bool activateVideo = false);
        void suspendBackgroundPlayback(bool suspend);
        void showShader(bool show);
        void setGradientTheme(GradientTheme theme);
        brls::Box* getContentBox() { return contentBox; }

        void animaShow(std::function<void()> onStart = nullptr);
        void animaHide(std::function<void()> onComplete = nullptr);
        virtual void onActivityResume() {}

        void frame(brls::FrameContext* ctx) override;


    private:
        // 背景层
        void setupBackgroundLayer();
        void setupReadabilityLayer();
        void ensureBackgroundImageLoaded();
        brls::Image* backgroundLayer = nullptr;
        beiklive::GifBackgroundView* backgroundGifLayer = nullptr;
        beiklive::VideoBackgroundView* backgroundVideoLayer = nullptr;
        brls::Rectangle* readabilityLayer = nullptr;
        bool backgroundImageLoaded = false;
        bool backgroundIsGif = false;
        bool backgroundIsVideo = false;
        // MP4 is deliberately opened only after the first screen has been
        // visible for a while.  Some files cause FFmpeg to perform expensive
        // stream probing, which must never happen during application startup.
        bool backgroundVideoLoadPending = false;
        bool backgroundVideoFadeStarted = false;
        bool backgroundGifFadeStarted = false;
        bool backgroundVideoHidePending = false;
        bool backgroundGifHidePending = false;
        bool backgroundTransitionPending = false;
        bool backgroundApplyingTransition = false;
        std::string backgroundTransitionPath;
        bool backgroundTransitionActivateVideo = false;
        std::string backgroundVideoPath;
        std::chrono::steady_clock::time_point backgroundVideoLoadAfter{};
        brls::Animatable backgroundVideoFade{0.0f};
        brls::Animatable backgroundGifFade{0.0f};
        // Shader层
        void setupShaderLayer();
        beiklive::DynamicBackgroundBox* shaderLayer = nullptr;
        brls::Box *mainBox = nullptr;
        brls::Box *contentBox = nullptr; // 内容层，页头和页脚之间的部分
        void setupMainBox();
        void setupContentBox();
        // 页头
        void setupHeader();
        beiklive::HeaderBar* header = nullptr;
        // 页脚
        void setupFooter();
        brls::BottomBar* bottomBar = nullptr;

        // 内容层动画状态
        enum class AnimState { None, Hiding, Showing };
        AnimState m_animState = AnimState::None;
        brls::Animatable m_animOffsetX{0.0f};
        brls::Animatable m_animHeaderY{0.0f};
        brls::Animatable m_animFooterY{0.0f};

        static constexpr int32_t ANIM_DUR_SLIDE     = 200;
        static constexpr int32_t ANIM_DUR_HFADE     = 200;
        static constexpr long    ANIM_DELAY_PHASE   = 1;
        static constexpr long    ANIM_DELAY_ENDPAUSE = 1;
    };

}
