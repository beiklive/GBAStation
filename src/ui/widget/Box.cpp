#include "Box.hpp"
#include "Header.hpp"
#include "core/common.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace beiklive
{
    namespace
    {
        // Background media is an application-wide choice. Individual Boxes
        // are short-lived page views and must never resurrect a pending
        // decoder after another page has selected a different file.
        std::string& selectedBackgroundPath()
        {
            // Do not construct mutable background state before main().  The
            // path is only needed once a Box has started applying UI settings.
            static std::string path;
            return path;
        }
    }

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
        backgroundVisibleRequested = show;
        if (show) {
            // Returning from an emulator page resumes the shared MP4 worker.
            VideoBackgroundView::setSharedPlaybackPaused(false);
            ensureBackgroundImageLoaded();
        }
        if (!show) {
            if (backgroundGifLayer && backgroundGifLayer->getVisibility() == brls::Visibility::VISIBLE) {
                backgroundGifHidePending = true;
                backgroundGifFade.stop();
                backgroundGifFade.reset(1.0f);
                backgroundGifFade.addStep(0.0f, 250, tweeny::easing::enumerated::cubicOut);
                backgroundGifFade.start();
            }
            if (backgroundVideoLayer && backgroundVideoLayer->getVisibility() == brls::Visibility::VISIBLE) {
                backgroundVideoHidePending = true;
                backgroundVideoFade.stop();
                backgroundVideoFade.reset(1.0f);
                backgroundVideoFade.addStep(0.0f, 250, tweeny::easing::enumerated::cubicOut);
                backgroundVideoFade.start();
            }
        } else {
            backgroundGifHidePending = false;
            backgroundVideoHidePending = false;
        }
        if (backgroundLayer)
            backgroundLayer->setVisibility(show && !backgroundIsGif &&
                (!backgroundIsVideo || !backgroundVideoLoadPending ||
                 !backgroundVideoLayer || !backgroundVideoLayer->isLoaded())
                ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
        if (backgroundGifLayer)
            backgroundGifLayer->setVisibility((show || backgroundGifHidePending) && backgroundIsGif && backgroundGifLayer->isLoaded()
                ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
        if (backgroundVideoLayer)
            // The transparent video view must receive frame() calls while it
            // is waiting for its first decoded frame, otherwise the texture
            // creation and the fade-in each wait for the other forever.
            backgroundVideoLayer->setVisibility((show || backgroundVideoHidePending) && backgroundIsVideo &&
                (!backgroundVideoLoadPending || backgroundVideoLayer->isLoaded())
                ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
    }

    void Box::suspendBackgroundPlayback(bool suspend)
    {
        VideoBackgroundView::setSharedPlaybackPaused(suspend);
        VideoBackgroundView::setSharedAudioSuspended(suspend);
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

    void Box::setBackgroundImage(const std::string& path, bool activateVideo)
    {
        if (!backgroundLayer || path.empty())
            return;

        // Only Settings passes activateVideo=true. Normal page restoration
        // attaches to that choice; at cold start there is no owner yet.
        auto& selectedPath = selectedBackgroundPath();
        if (activateVideo || selectedPath.empty())
            selectedPath = path;

        // A different dynamic background must leave the screen gracefully.
        // Retain the current view until its opacity reaches zero, then apply
        // the new source (which starts at zero and performs its own fade-in).
        const bool pathChanged = (backgroundIsGif || backgroundIsVideo) &&
            ((!backgroundIsVideo && (!isGifPath(path) || !backgroundGifLayer ||
                                     backgroundGifLayer->path() != path)) ||
             (backgroundIsVideo && backgroundVideoPath != path));
        if (!backgroundApplyingTransition && pathChanged) {
            backgroundTransitionPending = true;
            backgroundTransitionPath = path;
            backgroundTransitionActivateVideo = activateVideo;
            // Switch FFmpeg backgrounds are intentionally serialized.  The
            // platform can terminate while two decoder threads concurrently
            // probe/read separate MP4 files from SD, so the replacement is
            // started only after this outgoing layer has faded out and its
            // decoder has been stopped at transition commit.
            if (backgroundIsGif && backgroundGifLayer) {
                backgroundGifHidePending = true;
                backgroundGifFade.stop();
                backgroundGifFade.reset(backgroundGifLayer->getAlpha());
                backgroundGifFade.addStep(0.0f, 250, tweeny::easing::enumerated::cubicOut);
                backgroundGifFade.start();
            }
            if (backgroundIsVideo && backgroundVideoLayer) {
                backgroundVideoHidePending = true;
                backgroundVideoFade.stop();
                backgroundVideoFade.reset(backgroundVideoLayer->getAlpha());
                backgroundVideoFade.addStep(0.0f, 250, tweeny::easing::enumerated::cubicOut);
                backgroundVideoFade.start();
            }
            return;
        }

        const bool cachedGif = isGifPath(path) && GifBackgroundView::hasCachedAnimation(path);
        // A path chosen in Settings is an explicit user action, not startup
        // restoration. Start its GIF decode now so the page does not remain
        // empty until a later navigation causes another frame/update.
        backgroundIsGif = isGifPath(path) && backgroundGifLayer &&
            backgroundGifLayer->load(path, activateVideo);
        const bool requestedVideo = !backgroundIsGif && isMp4Path(path);
        backgroundIsVideo = requestedVideo && backgroundVideoLayer &&
            !path.empty() && std::filesystem::exists(path);
        if (requestedVideo && !backgroundIsVideo)
            brls::Logger::warning("MP4 background path is unavailable: '{}'", path);
        const bool show = backgroundVisibleRequested &&
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 0) != 0;
        if (backgroundIsGif) {
            backgroundVideoLoadPending = false;
            backgroundVideoFadeStarted = false;
            backgroundVideoPath.clear();
            VideoBackgroundView::clearCachedVideo();
            backgroundLayer->setVisibility(brls::Visibility::GONE);
            if (backgroundVideoLayer) {
                backgroundVideoLayer->clear();
                backgroundVideoLayer->setVisibility(brls::Visibility::GONE);
            }
            // Reattaching a page preserves the existing shared GIF without a
            // flash; a deliberate replacement always starts transparently.
            backgroundGifFadeStarted = cachedGif && backgroundGifLayer->isLoaded() &&
                !backgroundApplyingTransition;
            backgroundGifHidePending = false;
            backgroundGifFade.stop();
            backgroundGifFade.reset(backgroundGifFadeStarted ? 1.0f : 0.0f);
            backgroundGifLayer->setAlpha(backgroundGifFadeStarted ? 1.0f : 0.0f);
            backgroundGifLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                   : brls::Visibility::GONE);
        } else if (backgroundIsVideo) {
            // onResume reapplies settings after dialogs close. Do not tear
            // down the same deferred player and restart its two-second timer
            // on every resume, or it can be postponed indefinitely.
            const bool samePendingVideo = backgroundVideoLoadPending &&
                backgroundVideoPath == path;
            const bool sameLoadedVideo = !backgroundVideoLoadPending &&
                backgroundVideoPath == path && backgroundVideoLayer->isLoaded() &&
                backgroundVideoLayer->isCurrentCachedVideo(path);
            if (samePendingVideo || sameLoadedVideo) {
                backgroundLayer->setVisibility(show && !sameLoadedVideo
                                                    ? brls::Visibility::VISIBLE
                                                    : brls::Visibility::GONE);
                backgroundVideoLayer->setVisibility(show && sameLoadedVideo
                                                        ? brls::Visibility::VISIBLE
                                                        : brls::Visibility::GONE);
                return;
            }
            const bool cachedVideo = VideoBackgroundView::hasCachedVideo(path);
            if (backgroundGifLayer) {
                backgroundGifLayer->clear();
                backgroundGifLayer->setVisibility(brls::Visibility::GONE);
            }
            GifBackgroundView::clearCachedAnimation();
            // Do not enter FFmpeg while the startup scene is still being
            // assembled.  The first frame will be requested from frame().
            backgroundVideoLayer->clear();
            backgroundVideoLayer->setVisibility(brls::Visibility::GONE);
            VideoBackgroundView::keepCachedVideo(path);
            backgroundVideoPath = path;
            if (cachedVideo) {
                // Binding a process-global player performs no filesystem I/O
                // and keeps its timeline/texture exactly where the previous
                // page left them.
                backgroundVideoLoadPending = false;
                // A preloaded replacement may still be decoding. Keep the
                // fallback until a real first frame is available; otherwise
                // begin the normal fade on the next UI frame.
                backgroundVideoFadeStarted = false;
                backgroundVideoFade.stop();
                backgroundVideoFade.reset(0.0f);
                if (!backgroundVideoLayer->load(path)) {
                    backgroundIsVideo = false;
                    backgroundVideoFadeStarted = false;
                    backgroundLayer->setImageFromFile(BK_RES("img/bg2.png"));
                    backgroundLayer->setVisibility(brls::Visibility::VISIBLE);
                } else if (backgroundVideoLayer->isLoaded()) {
                    backgroundVideoFadeStarted = true;
                    backgroundVideoFade.reset(1.0f);
                    backgroundLayer->setVisibility(brls::Visibility::GONE);
                    backgroundVideoLayer->setAlpha(1.0f);
                    backgroundVideoLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                             : brls::Visibility::GONE);
                    brls::Logger::info("MP4: attached ready shared background '{}'", path);
                } else {
                    backgroundLayer->setImageFromFile(BK_RES("img/bg2.png"));
                    backgroundLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                        : brls::Visibility::GONE);
                    backgroundVideoLayer->setAlpha(0.0f);
                    backgroundVideoLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                             : brls::Visibility::GONE);
                    brls::Logger::info("MP4: attached preloading background '{}'", path);
                }
                backgroundImageLoaded = true;
                return;
            }
            // Only restored MP4 files are delayed. A file picked from the
            // Settings page is an explicit user action and should begin
            // producing its first frame immediately.
            backgroundVideoLoadPending = true;
            backgroundVideoFadeStarted = false;
            backgroundVideoFade.stop();
            backgroundVideoFade.reset(0.0f);
            // Keep a static fallback underneath until the decoder supplies a
            // real frame.  This is used for both a newly selected video and a
            // persisted one restored at launch.
            backgroundLayer->setImageFromFile(BK_RES("img/bg2.png"));
            backgroundLayer->setVisibility(show ? brls::Visibility::VISIBLE
                                                : brls::Visibility::GONE);
            backgroundVideoLoadAfter = std::chrono::steady_clock::now() +
                std::chrono::milliseconds(activateVideo ? 0 : 2000);
            brls::Logger::info("MP4 background {} deferred load scheduled: '{}'",
                               activateVideo ? "selected" : "restored", backgroundVideoPath);
        } else {
            backgroundVideoLoadPending = false;
            backgroundVideoFadeStarted = false;
            backgroundVideoPath.clear();
            backgroundVideoFade.stop();
            VideoBackgroundView::clearCachedVideo();
            if (backgroundGifLayer)
                backgroundGifLayer->clear();
            if (backgroundVideoLayer)
                backgroundVideoLayer->clear();
            if (requestedVideo)
                backgroundLayer->setImageFromFile(BK_RES("img/bg2.png"));
            else
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

        // GIF decoding happens off the UI thread. Once its first frame is
        // uploaded, make a previously-hidden background layer visible without
        // requiring the user to toggle the setting or reopen the page.
        const bool showBackgroundImage = backgroundVisibleRequested &&
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 0) != 0;
        if (backgroundIsGif && backgroundGifLayer && backgroundGifLayer->isLoaded()) {
            // A replacement must finish fading the old dynamic layer out
            // before the normal loaded/fade-in path is allowed to run.  The
            // previous ordering made this branch unreachable for an already
            // loaded GIF, leaving the transition pending forever.
            if (backgroundGifHidePending) {
                backgroundGifLayer->setAlpha(backgroundGifFade.getValue());
                if (backgroundGifFade.getValue() <= 0.001f) {
                    backgroundGifHidePending = false;
                    backgroundGifFadeStarted = false;
                    backgroundGifLayer->setVisibility(brls::Visibility::GONE);
                }
                invalidate();
            } else if (showBackgroundImage) {
                if (!backgroundGifFadeStarted) {
                    backgroundGifFadeStarted = true;
                    backgroundGifFade.stop();
                    backgroundGifFade.reset(0.0f);
                    backgroundGifFade.addStep(1.0f, 350, tweeny::easing::enumerated::cubicOut);
                    backgroundGifFade.start();
                    backgroundGifLayer->setVisibility(brls::Visibility::VISIBLE);
                }
                backgroundGifLayer->setAlpha(backgroundGifFade.getValue());
                invalidate();
            }
        }
        if (backgroundTransitionPending) {
            const bool gifFinished = !backgroundIsGif || !backgroundGifHidePending;
            const bool videoFinished = !backgroundIsVideo || !backgroundVideoHidePending;
            if (gifFinished && videoFinished) {
                const std::string nextPath = backgroundTransitionPath;
                const bool nextActivateVideo = backgroundTransitionActivateVideo;
                backgroundTransitionPending = false;
                backgroundTransitionPath.clear();
                brls::Logger::info("Background transition committed: '{}'", nextPath);
                backgroundApplyingTransition = true;
                setBackgroundImage(nextPath, nextActivateVideo);
                backgroundApplyingTransition = false;
            }
        }
        if (backgroundIsVideo && backgroundVideoLayer) {
            if (showBackgroundImage && backgroundVideoLoadPending &&
                !backgroundVideoPath.empty() &&
                backgroundVideoPath == selectedBackgroundPath() &&
                std::filesystem::exists(backgroundVideoPath) &&
                std::chrono::steady_clock::now() >= backgroundVideoLoadAfter) {
                backgroundVideoLoadPending = false;
                brls::Logger::info("Starting deferred MP4 background load: {}", backgroundVideoPath);
                if (!backgroundVideoLayer->load(backgroundVideoPath)) {
                    backgroundIsVideo = false;
                    backgroundLayer->setImageFromFile(BK_RES("img/bg2.png"));
                    backgroundLayer->setVisibility(brls::Visibility::VISIBLE);
                } else {
                    backgroundVideoLayer->setAlpha(0.0f);
                    backgroundVideoLayer->setVisibility(brls::Visibility::VISIBLE);
                }
            } else if (backgroundVideoLoadPending &&
                       (backgroundVideoPath.empty() ||
                        !std::filesystem::exists(backgroundVideoPath))) {
                backgroundVideoLoadPending = false;
                backgroundIsVideo = false;
                brls::Logger::warning("Deferred MP4 background path vanished before loading: '{}'",
                                      backgroundVideoPath);
                backgroundLayer->setImageFromFile(BK_RES("img/bg2.png"));
                backgroundLayer->setVisibility(brls::Visibility::VISIBLE);
            }

            // Check the outgoing fade before the normal loaded path.  An
            // active MP4 is necessarily loaded, so testing the latter first
            // prevents backgroundVideoHidePending from ever completing and
            // strands a preloaded replacement in the cache.
            if (backgroundVideoHidePending) {
                backgroundVideoLayer->setAlpha(backgroundVideoFade.getValue());
                if (backgroundVideoFade.getValue() <= 0.001f) {
                    backgroundVideoHidePending = false;
                    backgroundVideoFadeStarted = false;
                    backgroundVideoLayer->setVisibility(brls::Visibility::GONE);
                }
                invalidate();
            } else if (showBackgroundImage && !backgroundVideoLoadPending &&
                       backgroundVideoLayer->isLoaded()) {
                if (!backgroundVideoFadeStarted) {
                    backgroundVideoFadeStarted = true;
                    backgroundVideoFade.reset(0.0f);
                    backgroundVideoFade.addStep(1.0f, 350,
                                                tweeny::easing::enumerated::cubicOut);
                    backgroundVideoFade.start();
                    backgroundLayer->setVisibility(brls::Visibility::GONE);
                    backgroundVideoLayer->setVisibility(brls::Visibility::VISIBLE);
                }
                backgroundVideoLayer->setAlpha(backgroundVideoFade.getValue());
                invalidate();
            }
        }

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
        const std::string themeStr = GET_SETTING_KEY_STR(
            beiklive::SettingKey::KEY_UI_GRADIENT_THEME, "VscodeBlack");
        shaderLayer->setGradientTheme(gradientThemeFromId(themeStr));
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
