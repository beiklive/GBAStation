#pragma once

#include <cstdint>

namespace beiklive::melonds
{

class MelonDSInstance;

class MelonDSVideo
{
public:
    explicit MelonDSVideo(MelonDSInstance& instance);

    const uint32_t* topBuffer() const;
    const uint32_t* bottomBuffer() const;

    void captureFrame(uint32_t* out, unsigned outWidth, unsigned outHeight) const;

    static constexpr unsigned kScreenWidth = 256;
    static constexpr unsigned kScreenHeight = 192;

private:
    MelonDSInstance& m_instance;
};

} // namespace beiklive::melonds
