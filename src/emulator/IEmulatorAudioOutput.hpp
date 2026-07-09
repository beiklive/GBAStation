#pragma once

namespace beiklive {

class IEmulatorAudioOutput {
public:
    virtual ~IEmulatorAudioOutput() = default;
    virtual bool HandlesAudioOutput() const = 0;
    virtual void SetAudioOutputEnabled(bool enabled) = 0;
    virtual void SetAudioOutputSpeed(float speed) { (void)speed; }
    virtual void FlushAudioOutput() = 0;
};

} // namespace beiklive
