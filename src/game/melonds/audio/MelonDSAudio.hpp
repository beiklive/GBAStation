#pragma once

#include <cstdint>

namespace beiklive::melonds
{

class MelonDSInstance;

class MelonDSAudio
{
public:
    explicit MelonDSAudio(MelonDSInstance& instance);

    int readSamples(int16_t* buffer, int maxSamples);

    void mute(bool muted);

    bool isMuted() const { return m_muted; }

private:
    MelonDSInstance& m_instance;
    bool m_muted = false;
};

} // namespace beiklive::melonds
