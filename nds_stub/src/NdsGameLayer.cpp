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
                Gfx::TextureCreateExternal(kDsWidth, kDsHeight, m_renderer->GetFramebuffer(front, screen));
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

bool NdsGameLayer::readTouch(u16& outX, u16& outY) const
{
    HidTouchScreenState state {};
    if (!hidGetTouchScreenStates(&state, 1) || state.count == 0)
        return false;

    const RectF rect = bottomRect();
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

    Gfx::SetSampler(Gfx::sampler_Linear | Gfx::sampler_ClampToEdge);
    if (m_waitForFramebufferReady)
        Gfx::WaitForFenceReady(m_renderer->FramebufferReady[GPU::FrontBuffer]);
    Gfx::DrawRectangle(m_framebufferTextures[GPU::FrontBuffer][0],
                       {top.x, top.y},
                       {top.w, top.h},
                       {0.0f, 0.0f},
                       {static_cast<float>(kDsWidth), static_cast<float>(kDsHeight)},
                       {1.0f, 1.0f, 1.0f, 1.0f});
    Gfx::DrawRectangle(m_framebufferTextures[GPU::FrontBuffer][1],
                       {bottom.x, bottom.y},
                       {bottom.w, bottom.h},
                       {0.0f, 0.0f},
                       {static_cast<float>(kDsWidth), static_cast<float>(kDsHeight)},
                       {1.0f, 1.0f, 1.0f, 1.0f});
    Gfx::SignalFence(m_renderer->FramebufferPresented[GPU::FrontBuffer]);
}

} // namespace beiklive::nds_stub
