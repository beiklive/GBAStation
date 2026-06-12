#include "Box.hpp"
#include "Header.hpp"
#include "core/common.h"
#include <filesystem>

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

    void Box::setBackgroundImage(const std::string& path)
    {
        if(backgroundLayer && !path.empty())
            backgroundLayer->setImageFromFileForce(path);
    }

    void Box::showShader(bool show)
    {
        if(shaderLayer)
            shaderLayer->setVisibility(show ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void Box::setGradientTheme(GradientTheme theme)
    {
        if(shaderLayer)
            shaderLayer->setGradientTheme(theme);
    }

    void Box::animaShow(std::function<void()> onStart)
    {
        if (!contentBox) return;

        if (onStart)
            onStart();

        m_animScale.stop();
        m_animOffsetX.stop();
        m_animState = AnimState::Showing;

        contentBox->setVisibility(brls::Visibility::VISIBLE);

        // 阶段1：从右侧外移入 (200ms)
        m_animScale.reset(0.9f);
        m_animOffsetX.reset(300.0f);
        m_animOffsetX.addStep(0.0f, 200, tweeny::easing::enumerated::cubicOut);

        m_animOffsetX.setEndCallback([this](bool finished) {
            if (!finished || m_animState != AnimState::Showing || !contentBox) return;

            // 阶段2：从小缩放到原本大小 (150ms)
            m_animScale.reset(0.9f);
            m_animScale.addStep(1.0f, 150, tweeny::easing::enumerated::backOut);

            m_animScale.setEndCallback([this](bool) {
                if (!contentBox) return;
                m_animState = AnimState::None;
            });

            m_animScale.start();
        });

        m_animOffsetX.start();
    }

    void Box::animaHide(std::function<void()> onComplete)
    {
        if (!contentBox) return;

        auto onCompletePtr = std::make_shared<std::function<void()>>(std::move(onComplete));

        m_animScale.stop();
        m_animOffsetX.stop();
        m_animState = AnimState::Hiding;

        // 阶段1：缩小到 0.9x (150ms)
        m_animScale.reset(1.0f);
        m_animScale.addStep(0.9f, 150, tweeny::easing::enumerated::cubicIn);
        m_animOffsetX.reset(0.0f);

        m_animScale.setEndCallback([this, onCompletePtr](bool finished) {
            if (!finished || m_animState != AnimState::Hiding || !contentBox) return;

            // 阶段2：向左滑出屏幕 (200ms)
            m_animOffsetX.reset(0.0f);
            m_animOffsetX.addStep(-contentBox->getWidth() - 50.0f, 200,
                                  tweeny::easing::enumerated::cubicIn);

            m_animOffsetX.setEndCallback([this, onCompletePtr](bool) {
                if (!contentBox) return;
                contentBox->setVisibility(brls::Visibility::GONE);
                m_animState = AnimState::None;
                if (*onCompletePtr)
                    (*onCompletePtr)();
            });

            m_animOffsetX.start();
        });

        m_animScale.start();
    }

    void Box::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);

        if (m_animState != AnimState::None && contentBox)
            this->invalidate();
    }

    void Box::draw(NVGcontext* vg, float x, float y, float w, float h,
                   brls::Style style, brls::FrameContext* ctx)
    {
        if (m_animState != AnimState::None && contentBox) {
            float sx = m_animScale;
            float sy = m_animScale;
            float tx = m_animOffsetX;

            brls::Rect frame = contentBox->getFrame();
            float cx = frame.origin.x + frame.size.width * 0.5f;
            float cy = frame.origin.y + frame.size.height * 0.5f;

            nvgSave(vg);
            nvgTranslate(vg, cx, cy);
            nvgTranslate(vg, tx, 0.0f);
            nvgScale(vg, sx, sy);
            nvgTranslate(vg, -cx, -cy);
            brls::Box::draw(vg, x, y, w, h, style, ctx);
            nvgRestore(vg);
        } else {
            brls::Box::draw(vg, x, y, w, h, style, ctx);
        }
    }

    void Box::setupBackgroundLayer()
    {
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

        backgroundLayer->setImageFromFile(BK_RES("img/bg2.png")); // 默认背景图
        // 读取配置的背景图（如果有的话）
        std::string bgPath = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, "");
        if (!bgPath.empty() && std::filesystem::exists(bgPath))
            backgroundLayer->setImageFromFile(bgPath);
        // 应用所有背景设置（可见性、图片、XMB着色器与颜色）
        this->addView(backgroundLayer);
        bool showBg = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 0) != 0;
        showBackground(showBg);
    }

    void Box::setupShaderLayer()
    {
        #undef ABSOLUTE
        shaderLayer = new beiklive::DynamicBackgroundBox();
        shaderLayer->setFocusable(false);
        shaderLayer->setPositionType(brls::PositionType::ABSOLUTE);
        shaderLayer->setPositionTop(0);
        shaderLayer->setPositionLeft(0);
        shaderLayer->setWidthPercentage(100);
        shaderLayer->setHeightPercentage(100);
        this->addView(shaderLayer);
        // 主题始终应用（不受可见性影响）
        std::string themeStr = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_UI_GRADIENT_THEME, "VscodeBlack");
        if (themeStr == "Midnight")           shaderLayer->setGradientTheme(GradientTheme::Midnight);
        else if (themeStr == "LemonYellow")   shaderLayer->setGradientTheme(GradientTheme::LemonYellow);
        else if (themeStr == "AvocadoGreen")  shaderLayer->setGradientTheme(GradientTheme::AvocadoGreen);
        else if (themeStr == "StrawberryRed") shaderLayer->setGradientTheme(GradientTheme::StrawberryRed);
        else if (themeStr == "OceanBlue")     shaderLayer->setGradientTheme(GradientTheme::OceanBlue);
        else if (themeStr == "SakuraPink")    shaderLayer->setGradientTheme(GradientTheme::SakuraPink);
        else                                   shaderLayer->setGradientTheme(GradientTheme::VscodeBlack);
        // 根据配置决定初始可见性
        bool enable = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_SHADER, 1) != 0;
        shaderLayer->setVisibility(enable ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void Box::setupMainBox()
    {

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
        header = new beiklive::HeaderBar();
        header->setTitle("");
        mainBox->addView(header);
    }

    void Box::setupFooter()
    {

        bottomBar = new brls::BottomBar();
        bottomBar->setWidthPercentage(100);
        mainBox->addView(bottomBar);
    }
}
