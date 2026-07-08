#include "game_database.hpp"
#include "common.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

namespace beiklive
{
    namespace
    {
        bool writeJsonFileSafely(const std::string& filePath, const nlohmann::json& json)
        {
            const fs::path target(filePath);
            const fs::path tmp = target.string() + ".tmp";

            std::ofstream file(tmp, std::ios::binary | std::ios::trunc);
            if (!file.is_open())
                return false;

            file << json.dump(4);
            file.flush();
            if (!file)
            {
                file.close();
                std::error_code removeEc;
                fs::remove(tmp, removeEc);
                return false;
            }
            file.close();

            std::error_code ec;
            fs::rename(tmp, target, ec);
            if (!ec)
                return true;

            // Some platforms do not replace an existing target with rename().
            const fs::path backup = target.string() + ".bak";
            fs::remove(backup, ec);
            ec.clear();
            if (fs::exists(target, ec))
            {
                ec.clear();
                fs::rename(target, backup, ec);
                if (ec)
                {
                    std::error_code removeEc;
                    fs::remove(tmp, removeEc);
                    return false;
                }
            }

            ec.clear();
            fs::rename(tmp, target, ec);
            if (!ec)
            {
                std::error_code removeEc;
                fs::remove(backup, removeEc);
                return true;
            }

            std::error_code restoreEc;
            if (fs::exists(backup, restoreEc))
                fs::rename(backup, target, restoreEc);
            std::error_code removeEc;
            fs::remove(tmp, removeEc);
            return false;
        }

        std::vector<std::string> fallbackNdsShaderTypes()
        {
            return {"RetroArch_dot"};
        }

        std::string lowerAscii(std::string value)
        {
            std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });
            return value;
        }

        std::string trimCopy(std::string value)
        {
            value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](unsigned char c) {
                return std::isspace(c) == 0;
            }));
            value.erase(std::find_if(value.rbegin(), value.rend(), [](unsigned char c) {
                return std::isspace(c) == 0;
            }).base(), value.end());
            return value;
        }

        bool startsWith(const std::string& value, const char* prefix)
        {
            const std::string p(prefix);
            return value.size() >= p.size() &&
                   std::equal(p.begin(), p.end(), value.begin());
        }

        std::string canonicalNdsShaderStem(std::string value)
        {
            value = trimCopy(lowerAscii(std::move(value)));
            std::string out;
            out.reserve(value.size());
            bool lastWasDash = true;
            for (unsigned char c : value)
            {
                if (std::isalnum(c) != 0)
                {
                    out.push_back(static_cast<char>(c));
                    lastWasDash = false;
                    continue;
                }
                if (!lastWasDash)
                {
                    out.push_back('-');
                    lastWasDash = true;
                }
            }
            while (!out.empty() && out.back() == '-')
                out.pop_back();
            return out;
        }

        std::string ndsShaderMatchKey(const std::string& type)
        {
            if (startsWith(type, "DraStic_"))
                return "drastic-" + canonicalNdsShaderStem(type.substr(8));
            if (startsWith(type, "drastic-"))
                return canonicalNdsShaderStem(type);
            if (startsWith(type, "RetroArch_"))
                return "retroarch-" + canonicalNdsShaderStem(type.substr(10));
            return canonicalNdsShaderStem(type);
        }

        std::string oldRetroArchNdsShaderName(const std::string& type)
        {
            if (type == "dot")
                return "RetroArch_dot";
            if (type == "dot-clear")
                return "RetroArch_dot-clear";
            if (type == "xbrz-freescale")
                return "RetroArch_xbrz-freescale";
            if (type == "lcd-grid-v2-nds-color")
                return "RetroArch_lcd-grid-v2-nds-color";
            return {};
        }

        bool isUnsupportedNdsShaderType(const std::string& type)
        {
            const std::string key = ndsShaderMatchKey(type);
            if (!startsWith(key, "drastic-"))
                return false;

            return key.find("cartoon") != std::string::npos ||
                   key.find("fxaa") != std::string::npos ||
                   key.find("smaa") != std::string::npos ||
                   key.find("nataa") != std::string::npos ||
                   key.find("aacolor") != std::string::npos ||
                   key.find("aa2") != std::string::npos ||
                   key == "drastic-aa";
        }

        std::vector<std::string> loadNdsShaderTypes()
        {
            std::ifstream in(beiklive::res_path("config/nds_shaders.json"));
            if (!in)
                return fallbackNdsShaderTypes();

            auto parsed = nlohmann::json::parse(in, nullptr, false);
            if (parsed.is_discarded() || !parsed.is_array())
                return fallbackNdsShaderTypes();

            std::vector<std::string> result;
            for (const auto& item : parsed)
            {
                if (!item.is_string())
                    continue;
                const std::string value = item.get<std::string>();
                if (value.empty() ||
                    isUnsupportedNdsShaderType(value) ||
                    std::find(result.begin(), result.end(), value) != result.end())
                {
                    continue;
                }
                result.push_back(value);
            }

            if (std::find(result.begin(), result.end(), "RetroArch_dot") == result.end())
                result.insert(result.begin(), "RetroArch_dot");
            return result.empty() ? fallbackNdsShaderTypes() : result;
        }

        const std::vector<std::string>& ndsShaderTypes()
        {
            static const std::vector<std::string> types = loadNdsShaderTypes();
            return types;
        }

        std::string normalizeNdsShaderType(const std::string& type)
        {
            const auto& types = ndsShaderTypes();
            if (std::find(types.begin(), types.end(), type) != types.end())
                return type;

            const std::string oldRetro = oldRetroArchNdsShaderName(type);
            if (!oldRetro.empty() &&
                std::find(types.begin(), types.end(), oldRetro) != types.end())
            {
                return oldRetro;
            }

            const std::string key = ndsShaderMatchKey(type);
            for (const auto& candidate : types)
            {
                if (ndsShaderMatchKey(candidate) == key)
                    return candidate;
            }
            return "RetroArch_dot";
        }
    }

    /// 确保字符串为合法 UTF-8（剔除非法字节）
    static std::string sanitizeUtf8(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size();)
        {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c <= 0x7F) {
                out.push_back(static_cast<char>(c)); ++i;
            } else if (c >= 0xC2 && c <= 0xDF && i + 1 < s.size() &&
                       (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80) {
                out.push_back(s[i]); out.push_back(s[i+1]); i += 2;
            } else if (c >= 0xE0 && c <= 0xEF && i + 2 < s.size() &&
                       (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80 &&
                       (static_cast<unsigned char>(s[i+2]) & 0xC0) == 0x80) {
                out.push_back(s[i]); out.push_back(s[i+1]); out.push_back(s[i+2]); i += 3;
            } else if (c >= 0xF0 && c <= 0xF4 && i + 3 < s.size() &&
                       (static_cast<unsigned char>(s[i+1]) & 0xC0) == 0x80 &&
                       (static_cast<unsigned char>(s[i+2]) & 0xC0) == 0x80 &&
                       (static_cast<unsigned char>(s[i+3]) & 0xC0) == 0x80) {
                out.push_back(s[i]); out.push_back(s[i+1]);
                out.push_back(s[i+2]); out.push_back(s[i+3]); i += 4;
            } else {
                ++i; // 跳过非法字节
            }
        }
        return out;
    }

    void to_json(nlohmann::json &j, const GameEntry &entry)
    {
        const bool isNds = entry.platform == (int)beiklive::enums::EmuPlatform::EmuNDS;
        const std::string ndsShaderType = normalizeNdsShaderType(entry.NdsShaderType);
        j = nlohmann::json{
            {"path", sanitizeUtf8(entry.path)},
            {"title", sanitizeUtf8(entry.title)},
            {"logoPath", sanitizeUtf8(entry.logoPath)},
            {"playCount", entry.playCount},
            {"playTime", entry.playTime},
            {"platform", entry.platform},
            {"core", sanitizeUtf8(entry.core)},
            {"lastPlayed", sanitizeUtf8(entry.lastPlayed)},
            {"crc32", entry.crc32},
            {"favourite", entry.favourite},
            {"savePath", sanitizeUtf8(entry.savePath)},
            {"screenShotPath", sanitizeUtf8(entry.screenShotPath)},
            {"cheatPath", sanitizeUtf8(entry.cheatPath)},
            {"overlayPath", sanitizeUtf8(entry.overlayPath)},
            {"shaderPath", sanitizeUtf8(entry.shaderPath)},
            {"overlayEnabled", entry.overlayEnabled},
            {"shaderEnabled", entry.shaderEnabled},
            {"displayMode", entry.displayMode},
            {"integerAspectRatio", entry.integerAspectRatio},
            {"customScale", entry.customScale},
            {"customOffsetX", entry.customOffsetX},
            {"customOffsetY", entry.customOffsetY},
            {"ndsTopScale", entry.ndsTopScale},
            {"ndsTopOffsetX", entry.ndsTopOffsetX},
            {"ndsTopOffsetY", entry.ndsTopOffsetY},
            {"ndsBottomScale", entry.ndsBottomScale},
            {"ndsBottomOffsetX", entry.ndsBottomOffsetX},
            {"ndsBottomOffsetY", entry.ndsBottomOffsetY},
            {"ndsScreenLayout", sanitizeUtf8(entry.ndsScreenLayout)},
            {"ndsScreenOrientation", sanitizeUtf8(entry.ndsScreenOrientation)},
            {"ndsIntegerScale", entry.ndsIntegerScale},
            {"ndsScreenGap", entry.ndsScreenGap},
            {"ndsInternalResolution", entry.ndsInternalResolution},
            {"NdsShaderType", sanitizeUtf8(ndsShaderType)},
            {"MgbaShaderType", sanitizeUtf8(entry.MgbaShaderType)},
            {"shaderParaPath", sanitizeUtf8(isNds ? ndsShaderType : entry.shaderParaPath)},
            {"shaderParaNames", isNds ? std::vector<std::string>() : entry.shaderParaNames},
            {"shaderParaValues", isNds ? std::vector<float>() : entry.shaderParaValues}};
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
        entry.core = beiklive::NormalizeCoreId(entry.platform, j.value("core", ""));
        entry.lastPlayed = j.value("lastPlayed", "");
        entry.crc32 = j.value("crc32", 0);
        entry.favourite = j.value("favourite", false);
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
        entry.ndsTopScale = j.value("ndsTopScale", 1.0f);
        entry.ndsTopOffsetX = j.value("ndsTopOffsetX", 0.0f);
        entry.ndsTopOffsetY = j.value("ndsTopOffsetY", 0.0f);
        entry.ndsBottomScale = j.value("ndsBottomScale", 1.0f);
        entry.ndsBottomOffsetX = j.value("ndsBottomOffsetX", 0.0f);
        entry.ndsBottomOffsetY = j.value("ndsBottomOffsetY", 0.0f);
        entry.ndsScreenLayout = j.value("ndsScreenLayout", "priority_top");
        entry.ndsScreenOrientation = j.value("ndsScreenOrientation", "0");
        entry.ndsIntegerScale = j.value("ndsIntegerScale", true);
        entry.ndsScreenGap = std::clamp(j.value("ndsScreenGap", 0), -256, 256);
        entry.ndsInternalResolution = std::clamp(j.value("ndsInternalResolution", 1), 1, 4);
        entry.NdsShaderType = j.value("NdsShaderType", "");
        entry.MgbaShaderType = j.value("MgbaShaderType", "");
        if (entry.MgbaShaderType.empty())
            entry.MgbaShaderType = j.value("shaderType", "");
        if (entry.MgbaShaderType.empty())
            entry.MgbaShaderType = "RetroArch_dot";
        entry.shaderParaPath = j.value("shaderParaPath", "");
        entry.shaderParaNames = j.value("shaderParaNames", std::vector<std::string>());
        entry.shaderParaValues = j.value("shaderParaValues", std::vector<float>());
        if (entry.platform == (int)beiklive::enums::EmuPlatform::EmuNDS)
        {
            if (entry.NdsShaderType.empty())
                entry.NdsShaderType = entry.shaderParaPath.empty() ? "RetroArch_dot" : entry.shaderParaPath;
            entry.NdsShaderType = normalizeNdsShaderType(entry.NdsShaderType);
            entry.shaderParaPath = entry.NdsShaderType;
            entry.shaderParaNames.clear();
            entry.shaderParaValues.clear();
        }
    }

    // ==================== GameDatabase 实现（单线程版） ====================
    GameDatabase::GameDatabase(int autoSaveMode, int autoSaveInterval)
        : autoSaveMode_(autoSaveMode),
          autoSaveInterval_(autoSaveInterval), dirty_(false)
    {
    }

    GameDatabase::~GameDatabase()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (autoSaveMode_ != 0 && !dbDir_.empty())
            saveToDir(dbDir_);
    }

    void GameDatabase::upsert(const GameEntry &entry)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        doUpsert(entry);
        markDirtyAndAutoSave();
    }

    void GameDatabase::upsertByPath(const GameEntry &entry)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        doUpsertByPath(entry);
        markDirtyAndAutoSave();
    }

    bool GameDatabase::removeByCrc32(int crc32)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        bool result = doRemoveByCrc32(crc32);
        if (result)
            markDirtyAndAutoSave();
        return result;
    }

    bool GameDatabase::removeByPath(const std::string &path)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        bool result = doRemoveByPath(path);
        if (result)
            markDirtyAndAutoSave();
        return result;
    }

    std::optional<GameEntry> GameDatabase::findByCrc32(int crc32) const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return doFindByCrc32(crc32);
    }

    std::optional<GameEntry> GameDatabase::findByPath(const std::string &path) const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return doFindByPath(path);
    }

    std::vector<GameEntry> GameDatabase::getAll() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        return data_;
    }

    nlohmann::json GameDatabase::toJson() const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        doClear();
        markDirtyAndAutoSave();
    }

    void GameDatabase::clearAll()
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        doClear();
        dirty_ = false;
        if (!dbDir_.empty()) {
            const int platforms[] = {
                (int)beiklive::enums::EmuPlatform::EmuGBA,
                (int)beiklive::enums::EmuPlatform::EmuGBC,
                (int)beiklive::enums::EmuPlatform::EmuGB,
                (int)beiklive::enums::EmuPlatform::EmuNES,
                (int)beiklive::enums::EmuPlatform::EmuSNES,
                (int)beiklive::enums::EmuPlatform::EmuNDS,
            };
            std::error_code ec;
            for (int p : platforms)
                std::filesystem::remove(dbDir_ + beiklive::path::SPLIT_CHAR + getPlatformFileName(p), ec);
        }
    }


    std::string GameDatabase::getPlatformFileName(int platform)
    {
        switch (platform)
        {
        case (int)beiklive::enums::EmuPlatform::EmuGBA: return beiklive::path::DATA_BASE_FILE_GBA;
        case (int)beiklive::enums::EmuPlatform::EmuGBC: return beiklive::path::DATA_BASE_FILE_GBC;
        case (int)beiklive::enums::EmuPlatform::EmuGB:  return beiklive::path::DATA_BASE_FILE_GB;
        case (int)beiklive::enums::EmuPlatform::EmuNES: return beiklive::path::DATA_BASE_FILE_NES;
        case (int)beiklive::enums::EmuPlatform::EmuSNES: return beiklive::path::DATA_BASE_FILE_SNES;
        case (int)beiklive::enums::EmuPlatform::EmuNDS: return beiklive::path::DATA_BASE_FILE_NDS;
        default: return beiklive::path::DATA_BASE_FILE;
        }
    }

    bool GameDatabase::loadFromDir(const std::string &dir)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        doClear();
        dbDir_ = dir;

        const int platforms[] = {
            (int)beiklive::enums::EmuPlatform::EmuGBA,
            (int)beiklive::enums::EmuPlatform::EmuGBC,
            (int)beiklive::enums::EmuPlatform::EmuGB,
            (int)beiklive::enums::EmuPlatform::EmuNES,
            (int)beiklive::enums::EmuPlatform::EmuSNES,
            (int)beiklive::enums::EmuPlatform::EmuNDS,
        };

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
                    doUpsertByPath(entry);
                }
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

        dirty_ = false;
        return true;
    }

    bool GameDatabase::saveToDir(const std::string &dir) const
    {
        // 确保目录存在
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        // 按平台分组。即使某个平台当前没有条目，也写出合法空数组，避免
        // 异常退出后平台 JSON 缺失或保留过期内容。
        std::unordered_map<int, nlohmann::json> platformData;
        const int platforms[] = {
            (int)beiklive::enums::EmuPlatform::EmuGBA,
            (int)beiklive::enums::EmuPlatform::EmuGBC,
            (int)beiklive::enums::EmuPlatform::EmuGB,
            (int)beiklive::enums::EmuPlatform::EmuNES,
            (int)beiklive::enums::EmuPlatform::EmuSNES,
            (int)beiklive::enums::EmuPlatform::EmuNDS,
        };
        for (int platform : platforms)
            platformData[platform] = nlohmann::json::array();

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
                if (!writeJsonFileSafely(filePath, j))
                {
                    brls::Logger::warning("GameDatabase: 安全保存平台文件失败: {}", filePath);
                    allOk = false;
                }
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
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = crc32Index_.find(crc32);
        if (it == crc32Index_.end())
            return defaultValue;
        nlohmann::json j;
        to_json(j, data_[it->second]);
        return j.value(key, defaultValue);
    }

    nlohmann::json GameDatabase::get(const std::string &path, const std::string &key, const nlohmann::json &defaultValue) const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        auto it = pathIndex_.find(path);
        if (it == pathIndex_.end())
            return defaultValue;
        nlohmann::json j;
        to_json(j, data_[it->second]);
        return j.value(key, defaultValue);
    }

    bool GameDatabase::setDefault(int crc32, const std::string &key, const nlohmann::json &defaultValue)
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        if (dbDir_.empty())
            return false;
        bool ok = saveToDir(dbDir_);
        if (ok)
        dirty_ = false;
        return true;
    }

    std::vector<GameEntry> GameDatabase::getRecentPlayed(int count) const {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
        std::vector<GameEntry> result = data_;
        std::sort(result.begin(), result.end(),
                [](const GameEntry& a, const GameEntry& b) {
                    return a.lastPlayed > b.lastPlayed;
                });
        if (result.size() > static_cast<size_t>(count)) {
            result.resize(count);
        }
        return result;
    }

    std::vector<GameEntry> GameDatabase::getByPlatform(beiklive::enums::EmuPlatform platform) const
    {
        std::lock_guard<std::recursive_mutex> lock(m_mutex);
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
    void GameDatabase::doUpsertByPath(const GameEntry &entry)
    {
        auto it = pathIndex_.find(entry.path);
        if (it != pathIndex_.end())
        {
            int oldCrc32 = data_[it->second].crc32;
            data_[it->second] = entry;
            if (oldCrc32 != entry.crc32)
            {
                if (oldCrc32 != 0)
                    crc32Index_.erase(oldCrc32);
                if (entry.crc32 != 0)
                    crc32Index_[entry.crc32] = it->second;
            }
        }
        else
        {
            data_.push_back(entry);
            size_t idx = data_.size() - 1;
            if (entry.crc32 != 0)
                crc32Index_[entry.crc32] = idx;
            pathIndex_[entry.path] = idx;
        }
    }



    void GameDatabase::doUpsert(const GameEntry &entry)
    {
        auto it = entry.crc32 != 0 ? crc32Index_.find(entry.crc32) : crc32Index_.end();
        if (it != crc32Index_.end())
        {
            // 更新已有条目：先保存旧路径再赋值，否则旧路径信息被覆盖丢失
            std::string oldPath = data_[it->second].path;
            data_[it->second] = entry;
            if (oldPath != entry.path)
            {
                pathIndex_.erase(oldPath);
                pathIndex_[entry.path] = it->second;
            }
        }
        else
        {
            data_.push_back(entry);
            size_t idx = data_.size() - 1;
            if (entry.crc32 != 0)
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
            if (moved.crc32 != 0)
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
        if (data_[idx].crc32 != 0)
            crc32Index_.erase(data_[idx].crc32);
        pathIndex_.erase(it);
        if (idx != data_.size() - 1)
        {
            data_[idx] = std::move(data_.back());
            const auto &moved = data_[idx];
            if (moved.crc32 != 0)
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
