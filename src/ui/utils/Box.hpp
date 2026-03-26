#pragma once

#include <borealis.hpp>
#include "Header.hpp"

namespace beiklive
{
    class HeaderBar; // forward declaration to break circular dependency

    class Box : public brls::Box
    {
    public:
        Box();
        Box(brls::Axis flexDirection);
        ~Box();
    
        void showHeader(bool show);
        void showFooter(bool show);
        void showBackground(bool show);
        void showShader(bool show);
        brls::Box* getContentBox() { return contentBox; }

    private:
        // 背景层
        void setupBackgroundLayer();
        brls::Image* backgroundLayer = nullptr;
        // Shader层
        void setupShaderLayer();
        brls::Rectangle* shaderLayer = nullptr;
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
    };

}