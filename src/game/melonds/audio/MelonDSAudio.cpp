#include "audio/MelonDSAudio.hpp"
#include "MelonDSInstance.hpp"

#include <cstring>

namespace beiklive::melonds
{

MelonDSAudio::MelonDSAudio(MelonDSInstance& instance)
    : m_instance(instance)
{
}

int MelonDSAudio::readSamples(int16_t* buffer, int maxSamples)
{
    if (m_muted || !buffer || maxSamples <= 0)
        return 0;

    return m_instance.ReadAudio(buffer, maxSamples);
}

void MelonDSAudio::mute(bool muted)
{
    m_muted = muted;
}

} // namespace beiklive::melonds
