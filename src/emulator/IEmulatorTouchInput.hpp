#pragma once

namespace beiklive {

struct IEmulatorTouchInput {
    virtual ~IEmulatorTouchInput() = default;
    virtual void SetTouch(int x, int y, bool down) = 0;
};

} // namespace beiklive
