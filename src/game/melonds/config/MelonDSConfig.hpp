#pragma once

namespace beiklive::melonds
{

struct MelonDSConfig
{
    int rendererType = 0;
    int audioLatency = 2;
    int screenLayout = 0;
    int frameSkip = 0;
    bool threadedRendering = false;
    bool jitEnabled = true;
    int resolutionScale = 1;

    enum ScreenLayout
    {
        Vertical = 0,
        Horizontal = 1,
        Single = 2,
        Hybrid = 3
    };

    enum RendererType
    {
        Software = 0,
        OpenGL = 1
    };
};

} // namespace beiklive::melonds
