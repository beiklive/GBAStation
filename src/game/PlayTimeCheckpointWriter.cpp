#include "PlayTimeCheckpointWriter.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace beiklive
{
    PlayTimeCheckpointWriter& PlayTimeCheckpointWriter::instance()
    {
        static PlayTimeCheckpointWriter writer;
        return writer;
    }

    PlayTimeCheckpointWriter::PlayTimeCheckpointWriter()
        : m_worker(&PlayTimeCheckpointWriter::workerLoop, this)
    {
    }

    PlayTimeCheckpointWriter::~PlayTimeCheckpointWriter()
    {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_cv.notify_all();
        if (m_worker.joinable())
            m_worker.join();
    }

    uint64_t PlayTimeCheckpointWriter::queueLocked(const std::string& path, int seconds)
    {
        auto& entry = m_entries[path];
        const int normalizedSeconds = std::max(0, seconds);
        if (entry.requestedVersion > 0 && entry.seconds >= normalizedSeconds &&
            (entry.completedVersion < entry.requestedVersion ||
             entry.successfulVersion >= entry.requestedVersion))
            return entry.requestedVersion;

        entry.seconds = std::max(entry.seconds, normalizedSeconds);
        return ++entry.requestedVersion;
    }

    void PlayTimeCheckpointWriter::submit(const std::string& path, int seconds)
    {
        if (path.empty())
            return;

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopping)
                return;
            queueLocked(path, seconds);
        }
        m_cv.notify_one();
    }

    bool PlayTimeCheckpointWriter::flush(const std::string& path, int seconds)
    {
        if (path.empty())
            return false;

        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_stopping)
            return false;

        const uint64_t version = queueLocked(path, seconds);
        m_cv.notify_one();
        m_cv.wait(lock, [&] {
            const auto it = m_entries.find(path);
            return m_stopping ||
                   (it != m_entries.end() && it->second.completedVersion >= version);
        });

        const auto it = m_entries.find(path);
        return it != m_entries.end() && it->second.successfulVersion >= version;
    }

    void PlayTimeCheckpointWriter::forget(const std::string& path)
    {
        if (path.empty())
            return;

        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&] {
            const auto it = m_entries.find(path);
            return it == m_entries.end() ||
                   (!it->second.writing &&
                    it->second.completedVersion >= it->second.requestedVersion);
        });
        m_entries.erase(path);
    }

    bool PlayTimeCheckpointWriter::hasPendingLocked() const
    {
        for (const auto& [path, entry] : m_entries)
        {
            (void)path;
            if (!entry.writing && entry.completedVersion < entry.requestedVersion)
                return true;
        }
        return false;
    }

    void PlayTimeCheckpointWriter::workerLoop()
    {
        while (true)
        {
            std::string path;
            int seconds = 0;
            uint64_t version = 0;

            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [&] { return m_stopping || hasPendingLocked(); });

                if (!hasPendingLocked())
                {
                    if (m_stopping)
                        return;
                    continue;
                }

                for (auto& [candidatePath, entry] : m_entries)
                {
                    if (entry.writing || entry.completedVersion >= entry.requestedVersion)
                        continue;
                    path = candidatePath;
                    seconds = entry.seconds;
                    version = entry.requestedVersion;
                    entry.writing = true;
                    break;
                }
            }

            const bool success = writeCheckpoint(path, seconds);

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                const auto it = m_entries.find(path);
                if (it != m_entries.end())
                {
                    it->second.writing = false;
                    it->second.completedVersion = std::max(it->second.completedVersion, version);
                    if (success)
                        it->second.successfulVersion = std::max(it->second.successfulVersion, version);
                }
            }
            m_cv.notify_all();
        }
    }

    bool PlayTimeCheckpointWriter::writeCheckpoint(const std::string& path, int seconds)
    {
        namespace fs = std::filesystem;

        std::error_code ec;
        const fs::path target(path);
        const fs::path temp(path + ".tmp");
        if (!target.parent_path().empty())
            fs::create_directories(target.parent_path(), ec);

        {
            std::ofstream file(temp, std::ios::trunc);
            if (!file)
                return false;
            file << std::max(0, seconds);
            file.close();
            if (!file)
            {
                fs::remove(temp, ec);
                return false;
            }
        }

        ec.clear();
        fs::rename(temp, target, ec);
        if (!ec)
            return true;

        ec.clear();
        fs::copy_file(temp, target, fs::copy_options::overwrite_existing, ec);
        if (!ec)
        {
            std::error_code removeEc;
            fs::remove(temp, removeEc);
            return true;
        }

        std::error_code removeEc;
        ec.clear();
        fs::remove(temp, removeEc);
        return false;
    }
}
