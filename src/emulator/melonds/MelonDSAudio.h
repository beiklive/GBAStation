#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>

namespace beiklive::melonds {

class MelonDSAudio {
public:
    static constexpr size_t kCapacitySamples = 16384;

    void Reset();
    void Push(const int16_t* samples, size_t count);
    bool Drain(std::vector<int16_t>& out);

private:
    std::vector<int16_t> m_ring = std::vector<int16_t>(kCapacitySamples);
    size_t m_readPos = 0;
    size_t m_writePos = 0;
    size_t m_available = 0;
    std::mutex m_mutex;
};

} // namespace beiklive::melonds
