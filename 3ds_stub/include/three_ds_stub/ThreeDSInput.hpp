#pragma once

#include <cstdint>
#include <tuple>

#include "three_ds_stub/ThreeDSSwitch.hpp"

namespace beiklive::three_ds_stub {

struct ThreeDSTouchState {
    bool pressed = false;
    unsigned x = 0;
    unsigned y = 0;
};

class ThreeDSInput {
public:
    bool Init();
    void Shutdown();
    void Poll();

    [[nodiscard]] bool ExitRequested() const;
    [[nodiscard]] ThreeDSTouchState TouchState() const;

    [[nodiscard]] bool Button(std::uint64_t mask) const;
    [[nodiscard]] std::tuple<float, float> Analog(int stick) const;

private:
    PadState pad_{};
    ThreeDSTouchState touch_{};
    bool initialized_ = false;
    bool exit_requested_ = false;
};

} // namespace beiklive::three_ds_stub
