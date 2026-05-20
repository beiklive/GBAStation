#pragma once

#include <cstdint>

namespace beiklive::melonds
{

class MelonDSInstance;

class MelonDSInput
{
public:
    explicit MelonDSInput(MelonDSInstance& instance);

    void pressButton(unsigned id);
    void releaseButton(unsigned id);
    void setButtonState(unsigned id, bool pressed);

    void touchScreen(uint16_t x, uint16_t y);
    void releaseTouch();

    static constexpr unsigned kNdsButtonCount = 16;

    enum NDSButton
    {
        BTN_A       = 0,
        BTN_B       = 1,
        BTN_SELECT  = 2,
        BTN_START   = 3,
        BTN_RIGHT   = 4,
        BTN_LEFT    = 5,
        BTN_UP      = 6,
        BTN_DOWN    = 7,
        BTN_R       = 8,
        BTN_L       = 9,
        BTN_X       = 10,
        BTN_Y       = 11,
        BTN_TOUCH   = 12,
        BTN_LID     = 13,
        BTN_DEBUG   = 14,
        BTN_PEN_DOWN = 15
    };

private:
    MelonDSInstance& m_instance;
    uint32_t m_keyMask = 0x03FF03FF;
    bool m_touchDown = false;
};

} // namespace beiklive::melonds
