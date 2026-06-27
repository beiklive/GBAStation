#pragma once

namespace beiklive {

class IEmulatorVideoFrameMode {
public:
    virtual ~IEmulatorVideoFrameMode() = default;

    /// 控制核心是否需要保留高开销的加速帧读回。
    /// false 时允许核心退回到低成本的原生分辨率 CPU 帧缓存，
    /// 供截图/兜底显示等路径使用。
    virtual void SetAcceleratedFrameReadbackEnabled(bool enabled) = 0;
};

} // namespace beiklive
