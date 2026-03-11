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

};
