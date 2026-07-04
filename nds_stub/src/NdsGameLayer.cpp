#include "nds_stub/NdsGameLayer.hpp"

#include <algorithm>

#include "../../third_party/ArcDelta_melonDS/src/GPU.h"
#include "../../third_party/ArcDelta_melonDS/src/GPU2D_Deko.h"
#include "../../third_party/ArcDelta_melonDS/src/frontend/switch/Gfx.h"

namespace beiklive::nds_stub {

void NdsGameLayer::init(GPU2D::DekoRenderer* renderer)
{
    m_renderer = renderer;
    for (int front = 0; front < 2; ++front)
    {
        for (int screen = 0; screen < 2; ++screen)
        {
            m_framebufferTextures[front][screen] =
                Gfx::TextureCreateExternal(m_renderer->GetFramebufferTextureWidth(),
                                           m_renderer->GetFramebufferTextureHeight(),
                                           m_renderer->GetFramebuffer(front, screen));
        }
    }
}

void NdsGameLayer::deinit()
{
    for (int front = 0; front < 2; ++front)
    {
        for (int screen = 0; screen < 2; ++screen)
        {
            if (m_framebufferTextures[front][screen])
            {
                Gfx::TextureDelete(m_framebufferTextures[front][screen]);
                m_framebufferTextures[front][screen] = 0;
            }
        }
    }
    m_renderer = nullptr;
}

RectF NdsGameLayer::topRect() const
{
    constexpr float scale = 2.0f;
    return {kScreenWidth * 0.5f - kDsWidth * scale - 22.0f,
            kScreenHeight * 0.5f - kDsHeight * scale * 0.5f,
            kDsWidth * scale,
            kDsHeight * scale};
}

RectF NdsGameLayer::bottomRect() const
{
    constexpr float scale = 2.0f;
    return {kScreenWidth * 0.5f + 22.0f,
            kScreenHeight * 0.5f - kDsHeight * scale * 0.5f,
            kDsWidth * scale,
            kDsHeight * scale};
}

RectF NdsGameLayer::touchRect() const
{
    return m_screensSwapped ? topRect() : bottomRect();
}

bool NdsGameLayer::readTouch(u16& outX, u16& outY) const
{
    HidTouchScreenState state {};
    if (!hidGetTouchScreenStates(&state, 1) || state.count == 0)
        return false;

    const RectF rect = touchRect();
    const float sx = static_cast<float>(state.touches[0].x);
    const float sy = static_cast<float>(state.touches[0].y);
    if (sx < rect.x || sx >= rect.x + rect.w ||
        sy < rect.y || sy >= rect.y + rect.h)
        return false;

    outX = static_cast<u16>(std::clamp((sx - rect.x) * kDsWidth / rect.w, 0.0f, 255.0f));
    outY = static_cast<u16>(std::clamp((sy - rect.y) * kDsHeight / rect.h, 0.0f, 191.0f));
    return true;
}

void NdsGameLayer::drawScreens() const
{
    if (!m_renderer)
        return;

    const RectF top = topRect();
    const RectF bottom = bottomRect();
    const int topScreen = m_screensSwapped ? 1 : 0;
    const int bottomScreen = m_screensSwapped ? 0 : 1;
    const float srcWidth = static_cast<float>(m_renderer->GetFramebufferWidth());
    const float srcHeight = static_cast<float>(m_renderer->GetFramebufferHeight());

    Gfx::SetSampler((m_linearFiltering ? Gfx::sampler_Linear : Gfx::sampler_Nearest) |
                    Gfx::sampler_ClampToEdge);
    if (m_waitForFramebufferReady)
        Gfx::WaitForFenceReady(m_renderer->FramebufferReady[GPU::FrontBuffer]);
    Gfx::DrawRectangle(m_framebufferTextures[GPU::FrontBuffer][topScreen],
                       {top.x, top.y},
                       {top.w, top.h},
                       {0.0f, 0.0f},
                       {srcWidth, srcHeight},
                       {1.0f, 1.0f, 1.0f, 1.0f});
    Gfx::DrawRectangle(m_framebufferTextures[GPU::FrontBuffer][bottomScreen],
                       {bottom.x, bottom.y},
                       {bottom.w, bottom.h},
                       {0.0f, 0.0f},
                       {srcWidth, srcHeight},
                       {1.0f, 1.0f, 1.0f, 1.0f});
    Gfx::SignalFence(m_renderer->FramebufferPresented[GPU::FrontBuffer]);
}

} // namespace beiklive::nds_stub
