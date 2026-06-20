#pragma once

#include <cstdint>
#include <mutex>

namespace melonDS {
class NDS;
}

namespace beiklive::melonds {

class MelonDSInput {
public:
    void Reset();
    void SetButton(unsigned id, bool pressed);
    void SetButtonsFromMask(uint32_t mask);
    void SetTouch(int x, int y, bool down);
    void Apply(melonDS::NDS& nds) const;

private:
    uint32_t m_dsKeyMask = 0x0FFF;
    bool m_touchDown = false;
    int m_touchX = 0;
    int m_touchY = 0;
    mutable std::mutex m_mutex;
};

} // namespace beiklive::melonds
