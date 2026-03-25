#include "Box.hpp"
#include "Header.hpp"

namespace beiklive
{
    Box::Box() : brls::Box()
    {
        setupMainBox();
        setupBackgroundLayer();
        setupShaderLayer();
        setupHeader();
        setupFooter();
    }

    Box::Box(brls::Axis flexDirection) : brls::Box(flexDirection)
    {
        setupMainBox();
        setupBackgroundLayer();
        setupShaderLayer();
        setupHeader();
        setupFooter();
    }

    Box::~Box()
    {

    }

    void Box::showHeader(bool show)
    {
        if(header)
            header->setVisibility(show ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void Box::showFooter(bool show)
    {
        if(bottomBar)
            bottomBar->setVisibility(show ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void Box::showBackground(bool show)
    {
        if(backgroundLayer)
            backgroundLayer->setVisibility(show ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void Box::showShader(bool show)
    {
        if(shaderLayer)
            shaderLayer->setVisibility(show ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }


    void Box::setupBackgroundLayer()
    {
        if(backgroundLayer) return; // already setup
        #undef ABSOLUTE
        backgroundLayer = new brls::Image();
        backgroundLayer->setFocusable(false);
        backgroundLayer->setPositionType(brls::PositionType::ABSOLUTE);
        backgroundLayer->setPositionTop(0);
        backgroundLayer->setPositionLeft(0);
        backgroundLayer->setWidthPercentage(100);
        backgroundLayer->setHeightPercentage(100);
        backgroundLayer->setScalingType(brls::ImageScalingType::FIT);
        backgroundLayer->setInterpolation(brls::ImageInterpolation::LINEAR);
        // 应用所有背景设置（可见性、图片、XMB着色器与颜色）
        this->addView(backgroundLayer);
        showBackground(false); // 默认隐藏背景
    }

    void Box::setupShaderLayer()
    {
        // #undef ABSOLUTE
        // shaderLayer = new brls::Rectangle();
        // shaderLayer->setFocusable(false);
        // shaderLayer->setPositionType(brls::PositionType::ABSOLUTE);
        // shaderLayer->setPositionTop(0);
        // shaderLayer->setPositionLeft(0);
        // shaderLayer->setWidthPercentage(100);
        // shaderLayer->setHeightPercentage(100);
        // this->addView(shaderLayer);
    }

    void Box::setupMainBox()
    {
        if(mainBox) return; // already setup

        mainBox = new brls::Box(brls::Axis::COLUMN);
        mainBox->setFocusable(false);
        mainBox->setPositionType(brls::PositionType::RELATIVE);
        mainBox->setWidthPercentage(100);
        mainBox->setHeightPercentage(100);
        this->addView(mainBox);
    }

    void Box::setupHeader()
    {
        if(header) return; // already setup
        header = new beiklive::HeaderBar();

        header->setTitle("");
        mainBox->addView(header);
    }

    void Box::setupFooter()
    {
        if(bottomBar) return; // already setup

        #undef ABSOLUTE
        bottomBar = new brls::BottomBar();
        bottomBar->setPositionType(brls::PositionType::ABSOLUTE);
        bottomBar->setPositionBottom(0);
        bottomBar->setPositionLeft(0);
        bottomBar->setWidthPercentage(100);
        mainBox->addView(bottomBar);
    }
}