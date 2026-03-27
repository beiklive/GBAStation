
#include "game_database.hpp"
#include <fstream>
#include <filesystem>
#include <stdexcept>

namespace beiklive
{

    // JSON 转换函数实现
    void to_json(nlohmann::json &j, const GameEntry &entry)
    {
        j = nlohmann::json{
            {"path", entry.path},
            {"title", entry.title},
            {"logoPath", entry.logoPath},
            {"playCount", entry.playCount},
            {"playTime", entry.playTime},
            {"platform", entry.platform},
            {"lastPlayed", entry.lastPlayed},
            {"crc32", entry.crc32}};
    }

    void from_json(const nlohmann::json &j, GameEntry &entry)
    {
        j.at("path").get_to(entry.path);
        j.at("title").get_to(entry.title);
        j.at("logoPath").get_to(entry.logoPath);
        j.at("playCount").get_to(entry.playCount);
        j.at("playTime").get_to(entry.playTime);
        j.at("platform").get_to(entry.platform);
        j.at("lastPlayed").get_to(entry.lastPlayed);
        j.at("crc32").get_to(entry.crc32);
    }

    // GameDatabase 实现
    GameDatabase::GameDatabase(const std::string &filepath, int autoSaveMode, int autoSaveInterval)
        : filepath_(filepath), autoSaveMode_(autoSaveMode), autoSaveInterval_(autoSaveInterval),
          dirty_(false), stopAutoSave_(false)
    {
        if (autoSaveMode_ == 2)
        {
            autoSaveThread_ = std::thread(&GameDatabase::autoSaveWorker, this);
        }
    }

    GameDatabase::~GameDatabase()
    {
        if (autoSaveMode_ == 2)
        {
            {
                std::lock_guard<std::mutex> lock(cvMutex_);
                stopAutoSave_ = true;
            }
            cv_.notify_one();
            if (autoSaveThread_.joinable())
            {
                autoSaveThread_.join();
            }
        }
        // 析构前尝试保存最后一次修改（如果启用了自动保存且文件路径有效）
        if (!filepath_.empty() && autoSaveMode_ != 0)
        {
            saveToFile(filepath_);
        }
    }

    void GameDatabase::upsert(const GameEntry &entry)
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        doUpsert(entry);
        markDirtyAndAutoSave();
    }

    bool GameDatabase::removeByCrc32(int crc32)
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        bool result = doRemoveByCrc32(crc32);
        if (result)
            markDirtyAndAutoSave();
        return result;
    }

    bool GameDatabase::removeByPath(const std::string &path)
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        bool result = doRemoveByPath(path);
        if (result)
            markDirtyAndAutoSave();
        return result;
    }

    std::optional<GameEntry> GameDatabase::findByCrc32(int crc32) const
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        return doFindByCrc32(crc32);
    }

    std::optional<GameEntry> GameDatabase::findByPath(const std::string &path) const
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        return doFindByPath(path);
    }

    bool GameDatabase::updatePlayStats(int crc32, int newPlayCount, int newPlayTime, const std::string &newLastPlayed)
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        bool result = doUpdatePlayStats(crc32, newPlayCount, newPlayTime, newLastPlayed);
        if (result)
            markDirtyAndAutoSave();
        return result;
    }

    std::vector<GameEntry> GameDatabase::getAll() const
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        return data_;
    }

    nlohmann::json GameDatabase::toJson() const
    {
        std::shared_lock<std::shared_mutex> lock(rwMutex_);
        nlohmann::json j = nlohmann::json::array();
        for (const auto &entry : data_)
        {
            j.push_back(entry);
        }
        return j;
    }

    void GameDatabase::fromJson(const nlohmann::json &j)
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        doClear();
        if (!j.is_array())
            throw std::invalid_argument("JSON must be an array");
        for (const auto &item : j)
        {
            GameEntry entry = item.get<GameEntry>();
            doUpsert(entry);
        }
        markDirtyAndAutoSave();
    }

    void GameDatabase::clear()
    {
        std::unique_lock<std::shared_mutex> lock(rwMutex_);
        doClear();
        markDirtyAndAutoSave();
    }

    bool GameDatabase::saveToFile(const std::string &filepath) const
    {
        // 先复制数据
        std::vector<GameEntry> snapshot;
        {
            std::shared_lock<std::shared_mutex> lock(rwMutex_);
            snapshot = data_;
        }
        // 在锁外进行 JSON 序列化和文件写入
        try
        {
            nlohmann::json j = nlohmann::json::array();
            for (const auto &entry : snapshot)
            {
                j.push_back(entry);
            }
            std::ofstream file(filepath);
            if (!file.is_open())
                return false;
            file << j.dump(4);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool GameDatabase::loadFromFile(const std::string &filepath)
    {
        try
        {
            std::ifstream file(filepath);
            if (!file.is_open())
                return false;
            nlohmann::json j;
            file >> j;
            fromJson(j);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    bool GameDatabase::flush()
    {
        if (filepath_.empty())
            return false;
        bool saved = saveToFile(filepath_);
        if (saved)
        {
            std::lock_guard<std::mutex> lock(dirtyMutex_);
            dirty_ = false;
        }
        return saved;
    }

    void GameDatabase::setAutoSaveMode(int mode, int intervalSeconds)
    {
        if (autoSaveMode_ == 2)
        {
            {
                std::lock_guard<std::mutex> lock(cvMutex_);
                stopAutoSave_ = true;
            }
            cv_.notify_one();
            if (autoSaveThread_.joinable())
            {
                autoSaveThread_.join();
            }
        }
        autoSaveMode_ = mode;
        autoSaveInterval_ = intervalSeconds;
        if (autoSaveMode_ == 2)
        {
            stopAutoSave_ = false;
            autoSaveThread_ = std::thread(&GameDatabase::autoSaveWorker, this);
        }
    }

    void GameDatabase::setFilePath(const std::string &filepath)
    {
        std::lock_guard<std::mutex> lock(fileMutex_);
        filepath_ = filepath;
    }

    // 私有成员函数实现
    void GameDatabase::doUpsert(const GameEntry &entry)
    {
        auto it = crc32Index_.find(entry.crc32);
        if (it != crc32Index_.end())
        {
            // 更新已有条目
            data_[it->second] = entry;
            if (data_[it->second].path != entry.path)
            {
                pathIndex_.erase(data_[it->second].path);
                pathIndex_[entry.path] = it->second;
            }
        }
        else
        {
            data_.push_back(entry);
            size_t idx = data_.size() - 1;
            crc32Index_[entry.crc32] = idx;
            pathIndex_[entry.path] = idx;
        }
    }

    bool GameDatabase::doRemoveByCrc32(int crc32)
    {
        auto it = crc32Index_.find(crc32);
        if (it == crc32Index_.end())
            return false;
        size_t idx = it->second;
        pathIndex_.erase(data_[idx].path);
        crc32Index_.erase(it);
        if (idx != data_.size() - 1)
        {
            data_[idx] = std::move(data_.back());
            const auto &moved = data_[idx];
            crc32Index_[moved.crc32] = idx;
            pathIndex_[moved.path] = idx;
        }
        data_.pop_back();
        return true;
    }

    bool GameDatabase::doRemoveByPath(const std::string &path)
    {
        auto it = pathIndex_.find(path);
        if (it == pathIndex_.end())
            return false;
        size_t idx = it->second;
        crc32Index_.erase(data_[idx].crc32);
        pathIndex_.erase(it);
        if (idx != data_.size() - 1)
        {
            data_[idx] = std::move(data_.back());
            const auto &moved = data_[idx];
            crc32Index_[moved.crc32] = idx;
            pathIndex_[moved.path] = idx;
        }
        data_.pop_back();
        return true;
    }

    std::optional<GameEntry> GameDatabase::doFindByCrc32(int crc32) const
    {
        auto it = crc32Index_.find(crc32);
        if (it == crc32Index_.end())
            return std::nullopt;
        return data_[it->second];
    }

    std::optional<GameEntry> GameDatabase::doFindByPath(const std::string &path) const
    {
        auto it = pathIndex_.find(path);
        if (it == pathIndex_.end())
            return std::nullopt;
        return data_[it->second];
    }

    bool GameDatabase::doUpdatePlayStats(int crc32, int newPlayCount, int newPlayTime, const std::string &newLastPlayed)
    {
        auto it = crc32Index_.find(crc32);
        if (it == crc32Index_.end())
            return false;
        auto &entry = data_[it->second];
        entry.playCount = newPlayCount;
        entry.playTime = newPlayTime;
        entry.lastPlayed = newLastPlayed;
        return true;
    }

    void GameDatabase::doClear()
    {
        data_.clear();
        crc32Index_.clear();
        pathIndex_.clear();
    }

    void GameDatabase::markDirtyAndAutoSave()
    {
        if (autoSaveMode_ == 0)
            return;
        {
            std::lock_guard<std::mutex> lock(dirtyMutex_);
            dirty_ = true;
        }
        if (autoSaveMode_ == 1)
        {
            flush();
        }
        else if (autoSaveMode_ == 2)
        {
            cv_.notify_one();
        }
    }

    void GameDatabase::autoSaveWorker()
    {
        std::unique_lock<std::mutex> lock(cvMutex_);
        while (!stopAutoSave_)
        {
            cv_.wait_for(lock, std::chrono::seconds(autoSaveInterval_), [this]
                         { return stopAutoSave_ || (dirty_ && !filepath_.empty()); });
            if (stopAutoSave_)
                break;
            if (dirty_ && !filepath_.empty())
            {
                lock.unlock();
                flush();
                lock.lock();
            }
        }
    }

}