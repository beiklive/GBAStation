#include "common.hpp"

namespace beiklive {
    void swallow(brls::View* v, brls::ControllerButton btn)
    {
        v->registerAction("", btn, [](brls::View*) { return true; },
                          /*hidden=*/true);
    }


    void CheckGLSupport()
    {
        #ifdef BOREALIS_USE_OPENGL
        #  ifdef USE_GLES3
            brls::Logger::info("Using OpenGL ES 3 backend");
        #  elif defined(USE_GLES2)
            brls::Logger::info("Using OpenGL ES 2 backend");
        #  elif defined(USE_GL2)
            brls::Logger::info("Using OpenGL 2 backend");
        #  else
            brls::Logger::info("Using OpenGL 3 backend");
        #  endif
        #else
            brls::Logger::info("Using non-OpenGL backend");
        #endif
    }

    void InsertBackground(brls::Box *view)
    {
        #undef ABSOLUTE // avoid conflict with brls::PositionType::ABSOLUTE

        auto *m_bgImage = new beiklive::UI::ProImage();
        m_bgImage->setFocusable(false);
        m_bgImage->setPositionType(brls::PositionType::ABSOLUTE);
        m_bgImage->setPositionTop(0);
        m_bgImage->setPositionLeft(0);
        m_bgImage->setWidthPercentage(100);
        m_bgImage->setHeightPercentage(100);
        m_bgImage->setScalingType(brls::ImageScalingType::FIT);
        m_bgImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        m_bgImage->setShaderAnimation(beiklive::UI::ShaderAnimationType::PSP_XMB_RIPPLE);
        view->addView(m_bgImage);
    }
};
