#include "input/MelonDSInput.hpp"
#include "MelonDSInstance.hpp"

namespace beiklive::melonds
{

MelonDSInput::MelonDSInput(MelonDSInstance& instance)
    : m_instance(instance)
{
}

void MelonDSInput::pressButton(unsigned id)
{
    if (id >= kNdsButtonCount) return;

    unsigned bit = id;
    if (id >= 8) bit += 8;

    m_keyMask &= ~(1u << bit);
    m_instance.SetKeyMask(m_keyMask);
}

void MelonDSInput::releaseButton(unsigned id)
{
    if (id >= kNdsButtonCount) return;

    unsigned bit = id;
    if (id >= 8) bit += 8;

    m_keyMask |= (1u << bit);
    m_instance.SetKeyMask(m_keyMask);
}

void MelonDSInput::setButtonState(unsigned id, bool pressed)
{
    if (pressed)
        pressButton(id);
    else
        releaseButton(id);
}

void MelonDSInput::touchScreen(uint16_t x, uint16_t y)
{
    m_touchDown = true;
    m_instance.TouchScreen(x, y);
}

void MelonDSInput::releaseTouch()
{
    m_touchDown = false;
    m_instance.ReleaseScreen();
}

} // namespace beiklive::melonds
