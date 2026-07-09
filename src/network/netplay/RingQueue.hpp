#pragma once

#include <array>
#include <cstddef>
#include <mutex>
#include <optional>

namespace beiklive::netplay
{

template <typename T, size_t Capacity>
class RingQueue
{
public:
    bool push(const T& value)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_size >= Capacity)
            return false;

        m_items[m_tail] = value;
        m_tail = (m_tail + 1) % Capacity;
        ++m_size;
        return true;
    }

    std::optional<T> pop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_size == 0)
            return std::nullopt;

        T value = m_items[m_head];
        m_head = (m_head + 1) % Capacity;
        --m_size;
        return value;
    }

    size_t size() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_size;
    }

    void clear()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

private:
    mutable std::mutex m_mutex;
    std::array<T, Capacity> m_items{};
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_size = 0;
};

} // namespace beiklive::netplay
