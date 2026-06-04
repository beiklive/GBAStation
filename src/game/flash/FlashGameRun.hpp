#pragma once

#include "core/common.h"
#include "core/Tools.hpp"
#include "core/GameSignal.hpp"

#include <string>
#include <cstdint>

namespace beiklive::flash {

class FlashGameRun {
public:
    FlashGameRun() = default;
    ~FlashGameRun();

    bool SetupGame(beiklive::GameEntry entry);
    void Cleanup();

    void RenderFrame(uint64_t dt_us);

    void HandleKey(int code, bool down);
    void HandleMouseMove(int x, int y);
    void HandleMouseButton(bool down);

    void Restart();

    bool IsReady() const { return m_ready; }

    const beiklive::GameEntry& gameEntry() const { return m_gameEntry; }

private:
    beiklive::GameEntry m_gameEntry;
    bool m_ready = false;
};

} // namespace beiklive::flash
