#include "Box.hpp"
#include "Header.hpp"
#include "core/common.h"
#include <algorithm>
#include <cctype>
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
        if (show)
            ensureBackgroundImageLoaded();
        if (backgroundLayer)
            backgroundLayer->setVisibility(show && !backgroundIsGif && !backgroundIsVideo
                ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
        if (backgroundGifLayer)
            backgroundGifLayer->setVisibility(show && backgroundIsGif && backgroundGifLayer->isLoaded()
                ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
        if (backgroundVideoLayer)
            backgroundVideoLayer->setVisibility(show && backgroundIsVideo && backgroundVideoLayer->isLoaded()
                ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    static bool isGifPath(const std::string& path)
    {
        const std::filesystem::path file(path);
        std::string extension = file.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension == ".gif";
    }

    static bool isMp4Path(const std::string& path)
    {
        const std::filesystem::path file(path);
        std::string extension = file.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension == ".mp4";
    }

    void Box::setBackgroundImage(const std::string& path)
    {
        if (!backgroundLayer || path.empty())
            return;

        backgroundIsGif = isGifPath(path) && backgroundGifLayer && backgroundGifLayer->load(path);
        backgroundIsVideo = !backgroundIsGif && isMp4Path(path) && backgroundVideoLayer &&
            backgroundVideoLayer->load(path);
        const bool show = GET_SETTING_KEY_INT(
            beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 0) != 0;
        if (backgroundIsGif) {
            backgroundLayer->setVisibility(brls::Visibility::GONE);
            if (backgroundVideoLayer) {
                backgroundVideoLayer->clear();
                backgroundVideoLayer->setVisibility(brls::Visibility::GONE);
            }
            backgroundGifLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                   : brls::Visibility::GONE);
        } else if (backgroundIsVideo) {
            backgroundLayer->setVisibility(brls::Visibility::GONE);
            if (backgroundGifLayer) {
                backgroundGifLayer->clear();
                backgroundGifLayer->setVisibility(brls::Visibility::GONE);
            }
            backgroundVideoLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                     : brls::Visibility::GONE);
        } else {
            if (backgroundGifLayer)
                backgroundGifLayer->clear();
            if (backgroundVideoLayer)
                backgroundVideoLayer->clear();
            backgroundLayer->setImageFromFileForce(path);
            backgroundLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                : brls::Visibility::GONE);
        }
        backgroundImageLoaded = true;
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

        brls::Application::blockInputs();

        m_animOffsetX.stop();
        m_animHeaderY.stop();
        m_animFooterY.stop();
        m_animState = AnimState::Showing;

        contentBox->setVisibility(brls::Visibility::VISIBLE);

        // 顶栏/底栏初始保持在屏幕外
        if (header && header->getVisibility() == brls::Visibility::VISIBLE)
            m_animHeaderY.reset(-header->getHeight());
        if (bottomBar && bottomBar->getVisibility() == brls::Visibility::VISIBLE)
            m_animFooterY.reset(bottomBar->getHeight());

        // 阶段1：contentBox 从左侧滑入
        m_animOffsetX.reset(1280.0f);
        m_animOffsetX.addStep(0.0f, ANIM_DUR_SLIDE, tweeny::easing::enumerated::cubicOut);

        m_animOffsetX.setEndCallback([this](bool finished) {
            if (!finished || m_animState != AnimState::Showing || !contentBox) return;

            brls::delay(ANIM_DELAY_PHASE, [this]() {
                ASYNC_RETAIN
                if (m_animState != AnimState::Showing || !contentBox) { ASYNC_RELEASE; return; }

                // 阶段2：顶栏/底栏归位
                bool hasVisibleHeader = header && header->getVisibility() == brls::Visibility::VISIBLE;
                bool hasVisibleFooter = bottomBar && bottomBar->getVisibility() == brls::Visibility::VISIBLE;
                if (hasVisibleHeader) {
                    m_animHeaderY.reset(-header->getHeight());
                    m_animHeaderY.addStep(0.0f, ANIM_DUR_HFADE, tweeny::easing::enumerated::cubicOut);
                    m_animHeaderY.setEndCallback([this](bool) {
                        brls::delay(ANIM_DELAY_ENDPAUSE, [this]() {
                            ASYNC_RETAIN
                            brls::Application::unblockInputs();
                            m_animState = AnimState::None;
                            ASYNC_RELEASE
                        });
                    });
                    m_animHeaderY.start();
                }
                if (hasVisibleFooter) {
                    m_animFooterY.reset(bottomBar->getHeight());
                    m_animFooterY.addStep(0.0f, ANIM_DUR_HFADE, tweeny::easing::enumerated::cubicOut);
                    if (!hasVisibleHeader) {
                        m_animFooterY.setEndCallback([this](bool) {
                            brls::delay(ANIM_DELAY_ENDPAUSE, [this]() {
                                ASYNC_RETAIN
                                brls::Application::unblockInputs();
                                m_animState = AnimState::None;
                                ASYNC_RELEASE
                            });
                        });
                    }
                    m_animFooterY.start();
                }
                if (!hasVisibleHeader && !hasVisibleFooter) {
                    brls::delay(ANIM_DELAY_ENDPAUSE, [this]() {
                        ASYNC_RETAIN
                        brls::Application::unblockInputs();
                        m_animState = AnimState::None;
                        ASYNC_RELEASE
                    });
                }
                ASYNC_RELEASE
            });
        });

        m_animOffsetX.start();
    }

    void Box::animaHide(std::function<void()> onComplete)
    {
        if (!contentBox) return;

        brls::Application::blockInputs();

        auto onCompletePtr = std::make_shared<std::function<void()>>(std::move(onComplete));

        m_animOffsetX.stop();
        m_animHeaderY.stop();
        m_animFooterY.stop();
        m_animState = AnimState::Hiding;

        m_animOffsetX.reset(0.0f);

        auto startSlide = [this, onCompletePtr]() {
            // 阶段2：向左滑出屏幕
            m_animOffsetX.reset(0.0f);
            m_animOffsetX.addStep(-contentBox->getWidth() - 50.0f, ANIM_DUR_SLIDE,
                                  tweeny::easing::enumerated::cubicIn);

            m_animOffsetX.setEndCallback([this, onCompletePtr](bool) {
                if (!contentBox) return;
                brls::delay(ANIM_DELAY_ENDPAUSE, [this, onCompletePtr]() {
                    ASYNC_RETAIN
                    if (!contentBox) { ASYNC_RELEASE; return; }
                    brls::Application::unblockInputs();
                    m_animState = AnimState::None;
                    if (*onCompletePtr)
                        (*onCompletePtr)();
                    ASYNC_RELEASE
                });
            });

            m_animOffsetX.start();
        };

        // 阶段1：顶栏向上移出，底栏向下移出
        bool hasVisibleHeader = header && header->getVisibility() == brls::Visibility::VISIBLE;
        bool hasVisibleFooter = bottomBar && bottomBar->getVisibility() == brls::Visibility::VISIBLE;
        if (hasVisibleHeader) {
            m_animHeaderY.reset(0.0f);
            m_animHeaderY.addStep(-header->getHeight(), ANIM_DUR_HFADE,
                                  tweeny::easing::enumerated::cubicIn);
            m_animHeaderY.setEndCallback([this, onCompletePtr](bool) {
                brls::delay(ANIM_DELAY_PHASE, [this, onCompletePtr]() {
                    ASYNC_RETAIN
                    if (m_animState != AnimState::Hiding || !contentBox) { ASYNC_RELEASE; return; }

                    m_animOffsetX.reset(0.0f);
                    m_animOffsetX.addStep(-contentBox->getWidth() - 50.0f, ANIM_DUR_SLIDE,
                                          tweeny::easing::enumerated::cubicIn);

                    m_animOffsetX.setEndCallback([this, onCompletePtr](bool) {
                        if (!contentBox) return;
                        brls::delay(ANIM_DELAY_ENDPAUSE, [this, onCompletePtr]() {
                            ASYNC_RETAIN
                            if (!contentBox) { ASYNC_RELEASE; return; }
                            brls::Application::unblockInputs();
                            m_animState = AnimState::None;
                            if (*onCompletePtr)
                                (*onCompletePtr)();
                            ASYNC_RELEASE
                        });
                    });

                    m_animOffsetX.start();
                    ASYNC_RELEASE
                });
            });
            m_animHeaderY.start();
        }
        if (hasVisibleFooter) {
            m_animFooterY.reset(0.0f);
            m_animFooterY.addStep(bottomBar->getHeight(), ANIM_DUR_HFADE,
                                  tweeny::easing::enumerated::cubicIn);
            if (!hasVisibleHeader) {
                m_animFooterY.setEndCallback([this, onCompletePtr](bool) {
                    brls::delay(ANIM_DELAY_PHASE, [this, onCompletePtr]() {
                        ASYNC_RETAIN
                        if (m_animState != AnimState::Hiding || !contentBox) { ASYNC_RELEASE; return; }

                        m_animOffsetX.reset(0.0f);
                        m_animOffsetX.addStep(-contentBox->getWidth() - 50.0f, ANIM_DUR_SLIDE,
                                              tweeny::easing::enumerated::cubicIn);

                        m_animOffsetX.setEndCallback([this, onCompletePtr](bool) {
                            if (!contentBox) return;
                            brls::delay(ANIM_DELAY_ENDPAUSE, [this, onCompletePtr]() {
                                ASYNC_RETAIN
                                if (!contentBox) { ASYNC_RELEASE; return; }
                                brls::Application::unblockInputs();
                                m_animState = AnimState::None;
                                if (*onCompletePtr)
                                    (*onCompletePtr)();
                                ASYNC_RELEASE
                            });
                        });

                        m_animOffsetX.start();
                        ASYNC_RELEASE
                    });
                });
            }
            m_animFooterY.start();
        }
        if (!hasVisibleHeader && !hasVisibleFooter) {
            startSlide();
        }
    }

    void Box::frame(brls::FrameContext* ctx)
    {
        brls::Box::frame(ctx);

        if (m_animState != AnimState::None && contentBox) {
            contentBox->setTranslationX(m_animOffsetX);
            if (header && header->getVisibility() == brls::Visibility::VISIBLE) {
                header->setTranslationY(m_animHeaderY);
                float h = header->getHeight();
                if (h > 0)
                    header->setAlpha(1.0f - std::abs((float)m_animHeaderY) / h);
            }
            if (bottomBar && bottomBar->getVisibility() == brls::Visibility::VISIBLE) {
                bottomBar->setTranslationY(m_animFooterY);
                float h = bottomBar->getHeight();
                if (h > 0)
                    bottomBar->setAlpha(1.0f - std::abs((float)m_animFooterY) / h);
            }
            this->invalidate();
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

        this->addView(backgroundLayer);
        backgroundGifLayer = new beiklive::GifBackgroundView();
        backgroundGifLayer->setFocusable(false);
        backgroundGifLayer->setPositionType(brls::PositionType::ABSOLUTE);
        backgroundGifLayer->setPositionTop(0);
        backgroundGifLayer->setPositionLeft(0);
        backgroundGifLayer->setWidthPercentage(100);
        backgroundGifLayer->setHeightPercentage(100);
        backgroundGifLayer->setVisibility(brls::Visibility::GONE);
        this->addView(backgroundGifLayer);
        backgroundVideoLayer = new beiklive::VideoBackgroundView();
        backgroundVideoLayer->setFocusable(false);
        backgroundVideoLayer->setPositionType(brls::PositionType::ABSOLUTE);
        backgroundVideoLayer->setPositionTop(0);
        backgroundVideoLayer->setPositionLeft(0);
        backgroundVideoLayer->setWidthPercentage(100);
        backgroundVideoLayer->setHeightPercentage(100);
        backgroundVideoLayer->setVisibility(brls::Visibility::GONE);
        this->addView(backgroundVideoLayer);

        bool showBg = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 0) != 0;
        showBackground(showBg);
    }

    void Box::ensureBackgroundImageLoaded()
    {
        if (!backgroundLayer || backgroundImageLoaded)
            return;

        const std::string bgPath = GET_SETTING_KEY_STR(
            beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, "");
        if (!bgPath.empty() && std::filesystem::exists(bgPath))
            setBackgroundImage(bgPath);
        else {
            backgroundLayer->setImageFromFile(BK_RES("img/bg2.png"));
            backgroundIsGif = false;
            backgroundIsVideo = false;
            backgroundLayer->setVisibility(brls::Visibility::VISIBLE);
        }
        backgroundImageLoaded = true;
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
        // contentBox->setMarginRight(GET_STYLE("brls/applet_frame/padding_sides"));
        // contentBox->setMarginLeft(GET_STYLE("brls/applet_frame/padding_sides"));
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
