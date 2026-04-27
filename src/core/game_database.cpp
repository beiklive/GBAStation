#include "game_database.hpp"
#include <fstream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

namespace beiklive
{
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
            {"crc32", entry.crc32},
            {"savePath", entry.savePath},
            {"screenShotPath", entry.screenShotPath},
            {"cheatPath", entry.cheatPath},
            {"overlayPath", entry.overlayPath},
            {"shaderPath", entry.shaderPath},
            {"overlayEnabled", entry.overlayEnabled},
            {"shaderEnabled", entry.shaderEnabled},
            {"displayMode", entry.displayMode},
            {"integerAspectRatio", entry.integerAspectRatio},
            {"customScale", entry.customScale},
            {"customOffsetX", entry.customOffsetX},
            {"customOffsetY", entry.customOffsetY},
            {"shaderParaNames", entry.shaderParaNames},
            {"shaderParaValues", entry.shaderParaValues}};
    }

    void from_json(const nlohmann::json &j, GameEntry &entry)
    {
        // 统一使用 value() 并提供默认值，兼容新旧数据
        entry.path = j.value("path", "");
        entry.title = j.value("title", "");
        entry.logoPath = j.value("logoPath", "");
        entry.playCount = j.value("playCount", 0);
        entry.playTime = j.value("playTime", 0);
        entry.platform = j.value("platform", (int)beiklive::enums::EmuPlatform::NONE);
        entry.lastPlayed = j.value("lastPlayed", "");
        entry.crc32 = j.value("crc32", 0);
        entry.savePath = j.value("savePath", "");
        entry.screenShotPath = j.value("screenShotPath", "");
        entry.cheatPath = j.value("cheatPath", "");
        entry.overlayPath = j.value("overlayPath", "");
        entry.shaderPath = j.value("shaderPath", "");
        entry.overlayEnabled = j.value("overlayEnabled", false);
        entry.shaderEnabled = j.value("shaderEnabled", false);
        entry.displayMode = j.value("displayMode", 0);
        entry.integerAspectRatio = j.value("integerAspectRatio", 1.0f);
        entry.customScale = j.value("customScale", 1.0f);
        entry.customOffsetX = j.value("customOffsetX", 0.0f);
        entry.customOffsetY = j.value("customOffsetY", 0.0f);
        entry.shaderParaNames = j.value("shaderParaNames", std::vector<std::string>());
        entry.shaderParaValues = j.value("shaderParaValues", std::vector<float>());
    }

    // ==================== GameDatabase 实现（单线程版） ====================
    GameDatabase::GameDatabase(const std::string &filepath, int autoSaveMode, int autoSaveInterval)
        : filepath_(filepath), autoSaveMode_(autoSaveMode),
          autoSaveInterval_(autoSaveInterval), dirty_(false)
    {
        // 自动保存不再使用后台线程，仅根据模式决定行为
        if (!filepath_.empty())
            loadFromFile(filepath_);
    }

    GameDatabase::~GameDatabase()
    {
        // 析构时若自动保存模式非手动，则保存一次
        if (autoSaveMode_ != 0)
        {
            if (!dbDir_.empty())
                saveToDir(dbDir_);
            if (!filepath_.empty())
                saveToFile(filepath_);
        }
    }

    void GameDatabase::upsert(const GameEntry &entry)
    {
        doUpsert(entry);
        markDirtyAndAutoSave();
    }

    bool GameDatabase::removeByCrc32(int crc32)
    {
        bool result = doRemoveByCrc32(crc32);
        if (result)
            markDirtyAndAutoSave();
        return result;
    }

    bool GameDatabase::removeByPath(const std::string &path)
    {
        bool result = doRemoveByPath(path);
        if (result)
            markDirtyAndAutoSave();
        return result;
    }

    std::optional<GameEntry> GameDatabase::findByCrc32(int crc32) const
    {
        return doFindByCrc32(crc32);
    }

    std::optional<GameEntry> GameDatabase::findByPath(const std::string &path) const
    {
        return doFindByPath(path);
    }

    std::vector<GameEntry> GameDatabase::getAll() const
    {
        return data_;
    }

    nlohmann::json GameDatabase::toJson() const
    {
        nlohmann::json j = nlohmann::json::array();
        for (const auto &entry : data_)
        {
            nlohmann::json item;
            to_json(item, entry);
            j.push_back(item);
        }
        return j;
    }

    void GameDatabase::fromJson(const nlohmann::json &j)
    {
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
        doClear();
        markDirtyAndAutoSave();
    }

    bool GameDatabase::saveToFile(const std::string &filepath) const
    {
        try
        {
            nlohmann::json j = toJson();
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

    // ── 按平台分文件接口实现 ────────────────────────────────────────────────

    void GameDatabase::setDbDir(const std::string &dir)
    {
        dbDir_ = dir;
    }

    /*static*/ std::string GameDatabase::getPlatformFileName(int platform)
    {
        switch (platform)
        {
        case (int)beiklive::enums::EmuPlatform::EmuGBA: return beiklive::path::DATA_BASE_FILE_GBA;
        case (int)beiklive::enums::EmuPlatform::EmuGBC: return beiklive::path::DATA_BASE_FILE_GBC;
        case (int)beiklive::enums::EmuPlatform::EmuGB:  return beiklive::path::DATA_BASE_FILE_GB;
        default: return beiklive::path::DATA_BASE_FILE;
        }
    }

    bool GameDatabase::loadFromDir(const std::string &dir)
    {
        doClear();
        dbDir_ = dir;

        // 尝试逐平台加载数据库文件并合并
        const int platforms[] = {
            (int)beiklive::enums::EmuPlatform::EmuGBA,
            (int)beiklive::enums::EmuPlatform::EmuGBC,
            (int)beiklive::enums::EmuPlatform::EmuGB,
        };

        bool anyPlatformLoaded = false;
        for (int platform : platforms)
        {
            std::string filePath = dir + beiklive::path::SPLIT_CHAR + getPlatformFileName(platform);
            try
            {
                std::ifstream file(filePath);
                if (!file.is_open())
                    continue;
                nlohmann::json j;
                file >> j;
                if (!j.is_array())
                    continue;
                for (const auto &item : j)
                {
                    GameEntry entry = item.get<GameEntry>();
                    doUpsert(entry);
                }
                anyPlatformLoaded = true;
            }
            catch (const std::exception &e)
            {
                brls::Logger::warning("GameDatabase: 加载平台文件 {} 失败: {}", filePath, e.what());
            }
            catch (...)
            {
                brls::Logger::warning("GameDatabase: 加载平台文件 {} 时发生未知异常", filePath);
            }
        }

        // 向后兼容：若平台文件均不存在，则尝试加载旧版合并主文件并迁移
        if (!anyPlatformLoaded && !filepath_.empty())
        {
            try
            {
                std::ifstream file(filepath_);
                if (file.is_open())
                {
                    nlohmann::json j;
                    file >> j;
                    if (j.is_array())
                    {
                        for (const auto &item : j)
                        {
                            GameEntry entry = item.get<GameEntry>();
                            doUpsert(entry);
                        }
                        // 迁移：将旧版数据按平台分别保存到各平台文件
                        saveToDir(dir);
                    }
                }
            }
            catch (const std::exception &e)
            {
                brls::Logger::warning("GameDatabase: 迁移旧版数据库文件失败: {}", e.what());
            }
            catch (...)
            {
                brls::Logger::warning("GameDatabase: 迁移旧版数据库文件时发生未知异常");
            }
        }

        dirty_ = false;
        return true;
    }

    bool GameDatabase::saveToDir(const std::string &dir) const
    {
        // 按平台分组
        std::unordered_map<int, nlohmann::json> platformData;
        for (const auto &entry : data_)
        {
            nlohmann::json item;
            to_json(item, entry);
            if (!platformData.count(entry.platform))
                platformData[entry.platform] = nlohmann::json::array();
            platformData[entry.platform].push_back(item);
        }

        bool allOk = true;
        for (auto &[platform, j] : platformData)
        {
            std::string filePath = dir + beiklive::path::SPLIT_CHAR + getPlatformFileName(platform);
            try
            {
                std::ofstream file(filePath);
                if (!file.is_open())
                {
                    allOk = false;
                    continue;
                }
                file << j.dump(4);
            }
            catch (const std::exception &e)
            {
                brls::Logger::warning("GameDatabase: 保存平台文件 {} 失败: {}", filePath, e.what());
                allOk = false;
            }
            catch (...)
            {
                brls::Logger::warning("GameDatabase: 保存平台文件 {} 时发生未知异常", filePath);
                allOk = false;
            }
        }
        return allOk;
    }

    // ── 通用字段访问接口实现 ──────────────────────────────────────────────────

    bool GameDatabase::set(int crc32, const std::string &key, const nlohmann::json &value)
    {
        auto it = crc32Index_.find(crc32);
        if (it == crc32Index_.end())
            return false;
        GameEntry &entry = data_[it->second];
        nlohmann::json j;
        to_json(j, entry);
        j[key] = value;
        try
        {
            from_json(j, entry);
        }
        catch (...)
        {
            return false;
        }
        markDirtyAndAutoSave();
        return true;
    }

    bool GameDatabase::set(const std::string &path, const std::string &key, const nlohmann::json &value)
    {
        auto it = pathIndex_.find(path);
        if (it == pathIndex_.end())
            return false;
        GameEntry &entry = data_[it->second];
        nlohmann::json j;
        to_json(j, entry);
        j[key] = value;
        try
        {
            from_json(j, entry);
        }
        catch (...)
        {
            return false;
        }
        markDirtyAndAutoSave();
        return true;
    }

    nlohmann::json GameDatabase::get(int crc32, const std::string &key, const nlohmann::json &defaultValue) const
    {
        auto it = crc32Index_.find(crc32);
        if (it == crc32Index_.end())
            return defaultValue;
        nlohmann::json j;
        to_json(j, data_[it->second]);
        return j.value(key, defaultValue);
    }

    nlohmann::json GameDatabase::get(const std::string &path, const std::string &key, const nlohmann::json &defaultValue) const
    {
        auto it = pathIndex_.find(path);
        if (it == pathIndex_.end())
            return defaultValue;
        nlohmann::json j;
        to_json(j, data_[it->second]);
        return j.value(key, defaultValue);
    }

    bool GameDatabase::setDefault(int crc32, const std::string &key, const nlohmann::json &defaultValue)
    {
        auto it = crc32Index_.find(crc32);
        if (it == crc32Index_.end())
            return false;
        GameEntry &entry = data_[it->second];
        nlohmann::json j;
        to_json(j, entry);
        // 仅在字段不存在或为 null 时才写入默认值，或字段长度为0
        if (!j.contains(key) || j[key].is_null() || (j[key].is_string() && j[key].get<std::string>().empty()))
        {
            j[key] = defaultValue;
            try
            {
                from_json(j, entry);
            }
            catch (...)
            {
                return false;
            }
            markDirtyAndAutoSave();
        }
        return true;
    }

    bool GameDatabase::setDefault(const std::string &path, const std::string &key, const nlohmann::json &defaultValue)
    {
        auto it = pathIndex_.find(path);
        if (it == pathIndex_.end())
            return false;
        GameEntry &entry = data_[it->second];
        nlohmann::json j;
        to_json(j, entry);
        if (!j.contains(key) || j[key].is_null())
        {
            j[key] = defaultValue;
            try
            {
                from_json(j, entry);
            }
            catch (...)
            {
                return false;
            }
            markDirtyAndAutoSave();
        }
        return true;
    }

    // ─────────────────────────────────────────────────────────────────────────

    bool GameDatabase::flush()
    {
        bool ok = true;
        // 保存合并主文件（向后兼容）
        if (!filepath_.empty())
        {
            bool saved = saveToFile(filepath_);
            ok = ok && saved;
        }
        // 同时按平台分文件保存
        if (!dbDir_.empty())
        {
            bool saved = saveToDir(dbDir_);
            ok = ok && saved;
        }
        if (ok)
            dirty_ = false;
        return ok;
    }

    void GameDatabase::setAutoSaveMode(int mode, int intervalSeconds)
    {
        autoSaveMode_ = mode;
        autoSaveInterval_ = intervalSeconds;
        // 注意：此版本不再支持后台自动保存，仅手动模式或立即保存模式有效
    }

    void GameDatabase::setFilePath(const std::string &filepath)
    {
        filepath_ = filepath;
    }

    std::vector<GameEntry> GameDatabase::getRecentPlayed(int count) const {
        // 复制所有数据
        std::vector<GameEntry> result = data_;
        
        // 按 lastPlayed 降序排序（字符串格式 "yy-mm-dd hh-mm-ss" 直接比较即可）
        std::sort(result.begin(), result.end(),
                [](const GameEntry& a, const GameEntry& b) {
                    return a.lastPlayed > b.lastPlayed; // 降序，最近的在前面
                });
        
        // 取前 count 条，若不足则全部返回
        if (result.size() > static_cast<size_t>(count)) {
            result.resize(count);
        }
        return result;
    }
    // ==================== 公开接口 ====================
    // 获取指定平台的游戏列表（返回副本）
    std::vector<GameEntry> GameDatabase::getByPlatform(beiklive::enums::EmuPlatform platform) const
    {
        std::vector<GameEntry> result;
        int platformInt = static_cast<int>(platform);
        for (const auto& entry : data_)
        {
            if (entry.platform == platformInt)
                result.push_back(entry);
        }
        return result;
    }

    // ==================== 私有实现（无锁） ====================
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
        dirty_ = true;
        // 立即保存模式（mode=1）或定时模式（mode=2）都直接调用 flush
        // 注意：定时模式在单线程版本中无后台线程，因此也立即保存
        flush();
    }
} // namespace beiklive