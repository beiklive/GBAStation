#include "three_ds_stub/ThreeDSInput.hpp"

#include <algorithm>
#include <cmath>
#include <memory>

#include "common/param_package.h"
#include "common/settings.h"
#include "common/vector_math.h"
#include "core/frontend/input.h"
#include "three_ds_stub/ThreeDSLog.hpp"

namespace beiklive::three_ds_stub {
namespace {

ThreeDSInput* active_input = nullptr;

class SwitchButton final : public Input::ButtonDevice {
public:
    explicit SwitchButton(std::uint64_t mask) : mask_(mask) {}

    bool GetStatus() const override {
        return active_input && active_input->Button(mask_);
    }

private:
    std::uint64_t mask_;
};

class SwitchButtonFactory final : public Input::Factory<Input::ButtonDevice> {
public:
    std::unique_ptr<Input::ButtonDevice> Create(const Common::ParamPackage& params) override {
        return std::make_unique<SwitchButton>(
            std::stoull(params.Get("button", std::string{"0"})));
    }
};

class SwitchAnalog final : public Input::AnalogDevice {
public:
    explicit SwitchAnalog(int stick) : stick_(stick) {}

    std::tuple<float, float> GetStatus() const override {
        return active_input ? active_input->Analog(stick_) : std::tuple<float, float>{};
    }

private:
    int stick_;
};

class SwitchAnalogFactory final : public Input::Factory<Input::AnalogDevice> {
public:
    std::unique_ptr<Input::AnalogDevice> Create(const Common::ParamPackage& params) override {
        return std::make_unique<SwitchAnalog>(params.Get("stick", 0));
    }
};

class SwitchMotion final : public Input::MotionDevice {
public:
    std::tuple<Common::Vec3<float>, Common::Vec3<float>> GetStatus() const override {
        return {{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, 0.0f}};
    }
};

class SwitchMotionFactory final : public Input::Factory<Input::MotionDevice> {
public:
    std::unique_ptr<Input::MotionDevice> Create(const Common::ParamPackage&) override {
        return std::make_unique<SwitchMotion>();
    }
};

std::string ButtonParam(std::uint64_t button) {
    return Common::ParamPackage{{"engine", "switch"}, {"button", std::to_string(button)}}.Serialize();
}

std::string AnalogParam(int stick) {
    return Common::ParamPackage{{"engine", "switch"}, {"stick", std::to_string(stick)}}.Serialize();
}

void ConfigureAzaharBindings() {
    auto& profile = Settings::values.current_input_profile;
    profile.buttons.fill("engine:null");
    profile.analogs.fill("engine:null");

    // Switch and 3DS both use Nintendo's A-right, B-bottom, X-top, Y-left layout.
    profile.buttons[Settings::NativeButton::A] = ButtonParam(HidNpadButton_A);
    profile.buttons[Settings::NativeButton::B] = ButtonParam(HidNpadButton_B);
    profile.buttons[Settings::NativeButton::X] = ButtonParam(HidNpadButton_X);
    profile.buttons[Settings::NativeButton::Y] = ButtonParam(HidNpadButton_Y);
    profile.buttons[Settings::NativeButton::Up] = ButtonParam(HidNpadButton_Up);
    profile.buttons[Settings::NativeButton::Down] = ButtonParam(HidNpadButton_Down);
    profile.buttons[Settings::NativeButton::Left] = ButtonParam(HidNpadButton_Left);
    profile.buttons[Settings::NativeButton::Right] = ButtonParam(HidNpadButton_Right);
    profile.buttons[Settings::NativeButton::L] = ButtonParam(HidNpadButton_L);
    profile.buttons[Settings::NativeButton::R] = ButtonParam(HidNpadButton_R);
    profile.buttons[Settings::NativeButton::ZL] = ButtonParam(HidNpadButton_ZL);
    profile.buttons[Settings::NativeButton::ZR] = ButtonParam(HidNpadButton_ZR);
    profile.buttons[Settings::NativeButton::Start] = ButtonParam(HidNpadButton_Plus);
    profile.buttons[Settings::NativeButton::Select] = ButtonParam(HidNpadButton_Minus);
    profile.analogs[Settings::NativeAnalog::CirclePad] = AnalogParam(0);
    profile.analogs[Settings::NativeAnalog::CStick] = AnalogParam(1);
    profile.motion_device = "engine:switch";
    profile.touch_device = "engine:emu_window";
    profile.controller_touch_device.clear();
}

} // namespace

bool ThreeDSInput::Init() {
    if (initialized_) {
        return true;
    }

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&pad_);
    hidInitializeTouchScreen();

    active_input = this;
    Input::RegisterFactory<Input::ButtonDevice>("switch", std::make_shared<SwitchButtonFactory>());
    Input::RegisterFactory<Input::AnalogDevice>("switch", std::make_shared<SwitchAnalogFactory>());
    Input::RegisterFactory<Input::MotionDevice>("switch", std::make_shared<SwitchMotionFactory>());
    ConfigureAzaharBindings();
    initialized_ = true;
    return true;
}

void ThreeDSInput::Shutdown() {
    if (!initialized_) {
        return;
    }
    Input::UnregisterFactory<Input::ButtonDevice>("switch");
    Input::UnregisterFactory<Input::AnalogDevice>("switch");
    Input::UnregisterFactory<Input::MotionDevice>("switch");
    if (active_input == this) {
        active_input = nullptr;
    }
    initialized_ = false;
}

void ThreeDSInput::Poll() {
    if (!initialized_) {
        return;
    }

    padUpdate(&pad_);
    const std::uint64_t held = padGetButtons(&pad_);
    exit_requested_ = exit_requested_ ||
                      ((held & (HidNpadButton_ZL | HidNpadButton_ZR | HidNpadButton_Plus)) ==
                       (HidNpadButton_ZL | HidNpadButton_ZR | HidNpadButton_Plus));

    HidTouchScreenState state{};
    if (hidGetTouchScreenStates(&state, 1) > 0 && state.count > 0) {
        touch_.pressed = true;
        touch_.x = std::min<unsigned>(1279, state.touches[0].x);
        touch_.y = std::min<unsigned>(719, state.touches[0].y);
    } else {
        touch_ = {};
    }
}

bool ThreeDSInput::ExitRequested() const {
    return exit_requested_;
}

ThreeDSTouchState ThreeDSInput::TouchState() const {
    return touch_;
}

bool ThreeDSInput::Button(std::uint64_t mask) const {
    return initialized_ && (padGetButtons(&pad_) & mask) != 0;
}

std::tuple<float, float> ThreeDSInput::Analog(int stick) const {
    if (!initialized_) {
        return {};
    }
    const HidAnalogStickState state =
        padGetStickPos(&pad_, stick == 0 ? 0 : 1);
    constexpr float scale = 1.0f / 32768.0f;
    float x = std::clamp(static_cast<float>(state.x) * scale, -1.0f, 1.0f);
    float y = std::clamp(static_cast<float>(state.y) * scale, -1.0f, 1.0f);
    const float length = std::sqrt(x * x + y * y);
    if (length > 1.0f) {
        x /= length;
        y /= length;
    }
    return {x, y};
}

} // namespace beiklive::three_ds_stub
