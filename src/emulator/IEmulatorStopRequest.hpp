#pragma once

namespace beiklive {

struct IEmulatorStopRequest {
    virtual ~IEmulatorStopRequest() = default;
    virtual void RequestStop() = 0;
};

} // namespace beiklive
