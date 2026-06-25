#include "emulator/melonds/MelonDSAudio.h"

#include <algorithm>

namespace beiklive::melonds {

void MelonDSAudio::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_readPos = 0;
    m_writePos = 0;
    m_available = 0;
}

void MelonDSAudio::Push(const int16_t* samples, size_t count)
{
    if (!samples || count == 0)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    if (count >= m_ring.size())
    {
        samples += count - m_ring.size();
        count = m_ring.size();
        m_readPos = 0;
        m_writePos = 0;
        m_available = 0;
    }

    if (m_available + count > m_ring.size())
    {
        const size_t drop = (m_available + count) - m_ring.size();
        m_readPos = (m_readPos + drop) % m_ring.size();
        m_available -= drop;
    }

    for (size_t i = 0; i < count; ++i)
    {
        m_ring[m_writePos] = samples[i];
        m_writePos = (m_writePos + 1) % m_ring.size();
    }
    m_available += count;
}

bool MelonDSAudio::Drain(std::vector<int16_t>& out)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_available == 0)
    {
        out.clear();
        return false;
    }

    out.resize(m_available);
    for (size_t i = 0; i < out.size(); ++i)
    {
        out[i] = m_ring[m_readPos];
        m_readPos = (m_readPos + 1) % m_ring.size();
    }
    m_available = 0;
    return true;
}

} // namespace beiklive::melonds
