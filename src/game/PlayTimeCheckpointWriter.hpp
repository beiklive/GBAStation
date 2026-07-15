#pragma once

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>

namespace beiklive
{
    class PlayTimeCheckpointWriter
    {
    public:
        static PlayTimeCheckpointWriter& instance();

        void submit(const std::string& path, int seconds);
        bool flush(const std::string& path, int seconds);
        void forget(const std::string& path);

        ~PlayTimeCheckpointWriter();

        PlayTimeCheckpointWriter(const PlayTimeCheckpointWriter&) = delete;
        PlayTimeCheckpointWriter& operator=(const PlayTimeCheckpointWriter&) = delete;

    private:
        struct Entry
        {
            int seconds = 0;
            uint64_t requestedVersion = 0;
            uint64_t completedVersion = 0;
            uint64_t successfulVersion = 0;
            bool writing = false;
        };

        PlayTimeCheckpointWriter();

        uint64_t queueLocked(const std::string& path, int seconds);
        bool hasPendingLocked() const;
        void workerLoop();
        static bool writeCheckpoint(const std::string& path, int seconds);

        std::mutex m_mutex;
        std::condition_variable m_cv;
        std::unordered_map<std::string, Entry> m_entries;
        std::thread m_worker;
        bool m_stopping = false;
    };
}
