#include "Box.hpp"
#include "Header.hpp"

namespace beiklive
{
    Box::Box() : brls::Box()
    {
        setupBackgroundLayer();
        setupShaderLayer();
        setupMainBox();
        setupHeader();
        setupContentBox();
        setupFooter();
        brls::Logger::info("Box initialized");
    }

    Box::Box(brls::Axis flexDirection) : brls::Box(flexDirection)
    {
        setupBackgroundLayer();
        setupShaderLayer();
        setupMainBox();
        setupHeader();
        setupContentBox();
        setupFooter();
        brls::Logger::info("Box initialized");

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

        backgroundLayer->setImageFromFile("resources\\img\\bg2.png"); // 默认背景图，用户设置的背景会覆盖它
        // 应用所有背景设置（可见性、图片、XMB着色器与颜色）
        this->addView(backgroundLayer);
        // showBackground(false); // 默认隐藏背景
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


        // HIDE_BRLS_BACKGROUND(mainBox);
        this->addView(mainBox);

    }

    void Box::setupContentBox()
    {
        if(contentBox) return; // already setup

        contentBox = new brls::Box(brls::Axis::COLUMN);
        contentBox->setFocusable(false);
        contentBox->setPositionType(brls::PositionType::RELATIVE);
        contentBox->setGrow(1.0f);
        contentBox->setMarginRight(GET_STYLE("brls/applet_frame/padding_sides"));
        contentBox->setMarginLeft(GET_STYLE("brls/applet_frame/padding_sides"));
        // contentBox->setPaddingRight(GET_STYLE("brls/applet_frame/header_padding_sides"));
        // contentBox->setPaddingLeft(GET_STYLE("brls/applet_frame/header_padding_sides"));

        // HIDE_BRLS_BACKGROUND(contentBox);
        mainBox->addView(contentBox);
    }

    void Box::setupHeader()
    {
        if(header) return; // already setup
        header = new beiklive::HeaderBar();
        // HIDE_BRLS_BACKGROUND(header);

        header->setTitle("");
        mainBox->addView(header);
    }

    void Box::setupFooter()
    {
        if(bottomBar) return; // already setup

        #undef ABSOLUTE
        bottomBar = new brls::BottomBar();
        bottomBar->setWidthPercentage(100);
        // HIDE_BRLS_BACKGROUND(bottomBar);
        mainBox->addView(bottomBar);
    }
}