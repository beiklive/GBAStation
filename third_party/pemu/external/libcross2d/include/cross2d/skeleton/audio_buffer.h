#ifndef AUDIO_BUFFER_H
#define AUDIO_BUFFER_H

#include <cstring>

#if __cplusplus >= 201103L

#include <cstdint>

#else

#include <stdint.h>

#endif

namespace c2d {
    class SampleBuffer {
    public:
        SampleBuffer() {
            m_buffer_size = 0;
            m_buffer = nullptr;
        }

        explicit SampleBuffer(int num_samples) {
            m_buffer_size = num_samples;
            m_buffer = new int16_t[m_buffer_size];
            clear();
        }

        ~SampleBuffer() {
            delete[] m_buffer;
            m_buffer = nullptr;
        }

        inline void clear() {
            if (m_buffer == nullptr) return;
            m_start = 0;
            m_size = 0;
            memset(m_buffer, 0, m_buffer_size * 2);
        }

        inline bool pull(int16_t *dst, int num_samples) {
            if (space_filled() < num_samples)
                return false;

            memcpy(dst, m_buffer + m_start, min(num_samples, m_buffer_size - m_start) * 2);

            if (num_samples > (m_buffer_size - m_start))
                memcpy(dst + (m_buffer_size - m_start), m_buffer, (num_samples - (m_buffer_size - m_start)) * 2);

            m_start = (m_start + num_samples) % m_buffer_size;
            m_size -= num_samples;

            return true;
        }

        inline bool push(int16_t *src, int num_samples) {
            if (space_empty() < num_samples) return false;

            int end = m_start + m_size;
            if (end > m_buffer_size) end -= m_buffer_size;

            // if src is null, assume SampleBuffer was filled manually
            if (src) {
                int first_write_size = min(num_samples, m_buffer_size - end);
                memcpy(m_buffer + end, src, first_write_size * 2);
                if (num_samples > first_write_size)
                    memcpy(m_buffer, src + first_write_size, (num_samples - first_write_size) * 2);
            }

            m_size += num_samples;

            return true;
        }

        inline int space() const {
            return m_buffer_size;
        }

        inline int space_empty() const {
            return m_buffer_size - m_size;
        }

        inline int space_filled() const {
            return m_size;
        }

        void resize(int num_samples) {
            if (m_buffer) delete[] m_buffer;
            m_buffer_size = num_samples;
            m_buffer = new int16_t[m_buffer_size];
            clear();
        }

    private:
        int m_size = 0;
        int m_start = 0;
        int16_t *m_buffer;
        int m_buffer_size;

        static inline int min(int a, int b) {
            return ((a) < (b) ? (a) : (b));
        }
    };
}

#endif /* AUDIO_BUFFER_H */
