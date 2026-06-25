#include "emulator/melonds/MelonDSInput.h"

#include "NDS.h"
#include "core/common.h"

#include <algorithm>

namespace beiklive::melonds {

namespace {
constexpr uint32_t kNdsKeyA      = 1u << 0;
constexpr uint32_t kNdsKeyB      = 1u << 1;
constexpr uint32_t kNdsKeySelect = 1u << 2;
constexpr uint32_t kNdsKeyStart  = 1u << 3;
constexpr uint32_t kNdsKeyRight  = 1u << 4;
constexpr uint32_t kNdsKeyLeft   = 1u << 5;
constexpr uint32_t kNdsKeyUp     = 1u << 6;
constexpr uint32_t kNdsKeyDown   = 1u << 7;
constexpr uint32_t kNdsKeyR      = 1u << 8;
constexpr uint32_t kNdsKeyL      = 1u << 9;
constexpr uint32_t kNdsKeyX      = 1u << 10;
constexpr uint32_t kNdsKeyY      = 1u << 11;

uint32_t dsBitForRetroId(unsigned id)
{
    switch (id)
    {
    case 0:  return kNdsKeyB;
    case 1:  return kNdsKeyY;
    case 2:  return kNdsKeySelect;
    case 3:  return kNdsKeyStart;
    case 4:  return kNdsKeyUp;
    case 5:  return kNdsKeyDown;
    case 6:  return kNdsKeyLeft;
    case 7:  return kNdsKeyRight;
    case 8:  return kNdsKeyA;
    case 9:  return kNdsKeyX;
    case 10: return kNdsKeyL;
    case 11: return kNdsKeyR;
    default: return 0;
    }
}
}

void MelonDSInput::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dsKeyMask = 0x0FFF;
    m_touchDown = false;
    m_touchX = 0;
    m_touchY = 0;
}

void MelonDSInput::SetButton(unsigned id, bool pressed)
{
    const uint32_t bit = dsBitForRetroId(id);
    if (!bit)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (pressed)
        m_dsKeyMask &= ~bit;
    else
        m_dsKeyMask |= bit;
}

void MelonDSInput::SetButtonsFromMask(uint32_t mask)
{
    for (unsigned i = 0; i < 16; ++i)
        SetButton(i, (mask >> i) & 1u);
}

void MelonDSInput::SetTouch(int x, int y, bool down)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_touchDown = down;
    m_touchX = std::clamp(x, 0, 255);
    m_touchY = std::clamp(y, 0, 191);
}

void MelonDSInput::Apply(melonDS::NDS& nds) const
{
    uint32_t keyMask;
    bool touchDown;
    int touchX;
    int touchY;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        keyMask = m_dsKeyMask;
        touchDown = m_touchDown;
        touchX = m_touchX;
        touchY = m_touchY;
    }

    nds.SetKeyMask(keyMask);
    if (touchDown)
        nds.TouchScreen(static_cast<melonDS::u16>(touchX), static_cast<melonDS::u16>(touchY));
    else
        nds.ReleaseScreen();
}

} // namespace beiklive::melonds
