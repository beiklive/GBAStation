#include "FlashGameRun.hpp"
#include "game/flash/flashnx_bridge.h"
#include "game/flash/FlashKeymap.hpp"

namespace beiklive::flash {

FlashGameRun::~FlashGameRun()
{
    Cleanup();
}

bool FlashGameRun::SetupGame(beiklive::GameEntry entry)
{
    m_gameEntry = std::move(entry);
    m_ready = false;

    FlashKeymap::initForSwf(m_gameEntry.path);

    ruffle_set_swf_path(m_gameEntry.path.c_str());

    if (ruffle_init() != 0) {
        brls::Logger::error("FlashGameRun: ruffle_init 失败");
        return false;
    }

    m_ready = true;
    brls::Logger::info("FlashGameRun: 初始化完成 {}", m_gameEntry.path);
    return true;
}

void FlashGameRun::Cleanup()
{
    if (m_ready) {
        ruffle_shutdown();
        FlashKeymap::reset();
    }
    m_ready = false;
}

void FlashGameRun::RenderFrame(uint64_t dt_us)
{
    if (m_ready)
        ruffle_render_frame_dt(dt_us);
}

void FlashGameRun::HandleKey(int code, bool down)
{
    if (m_ready)
        ruffle_handle_key(code, down);
}

void FlashGameRun::HandleMouseMove(int x, int y)
{
    if (m_ready)
        ruffle_handle_mouse_move(x, y);
}

void FlashGameRun::HandleMouseButton(bool down)
{
    if (m_ready)
        ruffle_handle_mouse_button(down);
}

void FlashGameRun::Restart()
{
    if (m_ready)
        ruffle_restart();
}

} // namespace beiklive::flash
