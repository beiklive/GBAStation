//
// Created by cpasjuste on 12/12/16.
//

#include <string>
#include "cross2d/c2d.h"

using namespace c2d;

Audio::Audio(int rate, int samples, C2DAudioCallback cb) {
    mutex = new C2DMutex();

    m_sample_rate = rate;
    m_samples = samples;
    m_samples_size = m_samples * channels * (int) sizeof(int16_t);

    m_buffer = new SampleBuffer();
    m_buffer->resize(m_samples * channels * 3);

    callback = cb;
    available = true;

    printf("Audio::Audio: rate = %i, samples = %i, samples size = %i\n",
           rate, samples, m_samples_size);
}

void Audio::play(const void *data, int samples, SyncMode syncMode) {
    if (available) {
        if (paused) {
            pause(0);
        }

        if (callback) {
            return;
        }

        if (syncMode == LowLatency) {
            lock();
            int queued = getSampleBufferQueued();
            unlock();
            while (queued >= getSamplesSize()) {
                if (!available) return;
                if (c2d_renderer) c2d_renderer->delayUs(1);
                lock();
                queued = getSampleBufferQueued();
                unlock();
            }
        } else if (syncMode == Safe) {
            //printf("play: samples: %i, queued: %i, available: %i\n",
            //     samples * channels, getSampleBufferQueued(), getSampleBufferAvailable());
            lock();
            int space = getSampleBufferAvailable();
            unlock();
            while (space < samples * channels) {
                //printf("play (delay): samples: %i, queued: %i\n", getSamples() * channels, getSampleBufferQueued());
                if (!available) return;
                if (c2d_renderer) c2d_renderer->delayUs(1);
                lock();
                space = getSampleBufferAvailable();
                unlock();
            }
            //printf("play (done): queued: %i\n", getSampleBufferQueued());
        }

        lock();
        m_buffer->push((int16_t *) data, samples * channels);
        unlock();
    }
}

void Audio::reset() {
    lock();
    m_buffer->clear();
    unlock();
    paused = false;
}

void Audio::pause(int pause) {
    paused = pause;
    if (paused) {
        lock();
        m_buffer->clear();
        unlock();
    }
}

int Audio::getSampleRate() {
    return m_sample_rate;
}

int Audio::getChannels() {
    return channels;
}

int Audio::getSamples() {
    return m_samples;
}

int Audio::getSamplesSize() {
    return m_samples_size;
}

SampleBuffer *Audio::getSampleBuffer() {
    return m_buffer;
}

int Audio::getSampleBufferQueued() {
    return m_buffer->space_filled();
}

int Audio::getSampleBufferCapacity() {
    return m_buffer->space();
}

int Audio::getSampleBufferAvailable() {
    return m_buffer->space_empty();
}

int Audio::toSamples(int rate, float fps) {
    return (int) ((float) rate / fps);
}

bool Audio::isAvailable() {
    return available;
}

bool Audio::lock() {
    return mutex ? mutex->lock() : false;
}

bool Audio::unlock() {
    return mutex ? mutex->unlock() : false;
}

Audio::~Audio() {
    available = false;

    if (m_buffer) {
        delete (m_buffer);
    }

    if (mutex) {
        delete (mutex);
    }
}
