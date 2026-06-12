#pragma once

#include <borealis.hpp>
#include <borealis/core/animation.hpp>
#include "Header.hpp"
#include "DynamicBackgroundBox.hpp"

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
        void showBackground(bool show);
        void setBackgroundImage(const std::string& path);
        void showShader(bool show);
        void setGradientTheme(GradientTheme theme);
        brls::Box* getContentBox() { return contentBox; }

        void animaShow(std::function<void()> onStart = nullptr);
        void animaHide(std::function<void()> onComplete = nullptr);

        void frame(brls::FrameContext* ctx) override;
        void draw(NVGcontext* vg, float x, float y, float w, float h,
                  brls::Style style, brls::FrameContext* ctx) override;


    private:
        // 背景层
        void setupBackgroundLayer();
        brls::Image* backgroundLayer = nullptr;
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
        brls::Animatable m_animScale{1.0f};
        brls::Animatable m_animOffsetX{0.0f};
        brls::Animatable m_animHeaderY{0.0f};
        brls::Animatable m_animFooterY{0.0f};

        static constexpr int32_t ANIM_DUR_SLIDE     = 200;
        static constexpr int32_t ANIM_DUR_SCALE     = 150;
        static constexpr int32_t ANIM_DUR_HFADE     = 200;
        static constexpr long    ANIM_DELAY_PHASE   = 1;
        static constexpr long    ANIM_DELAY_ENDPAUSE = 1;
    };

}
