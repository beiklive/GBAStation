#ifndef GAME_DATABASE_HPP
#define GAME_DATABASE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <mutex>

#include "enums.h"
#include "constexpr.h"


namespace beiklive
{

    // JSON 转换函数声明
    void to_json(nlohmann::json &j, const GameEntry &entry);
    void from_json(const nlohmann::json &j, GameEntry &entry);

    // 游戏数据库管理类
    class GameDatabase
    {
    public:
        // 构造函数：可指定自动保存模式与文件路径
        // autoSaveMode: 0 = 手动保存, 1 = 每次变更立即保存, 2 = 定时保存（间隔 intervalSeconds）
        // autoSaveInterval: 当 mode=2 时，定时保存的间隔（秒）
        GameDatabase(const std::string &filepath = "", int autoSaveMode = 0, int autoSaveInterval = 5);
        ~GameDatabase();

        // 禁止拷贝和移动
        GameDatabase(const GameDatabase &) = delete;
        GameDatabase &operator=(const GameDatabase &) = delete;

        // 插入或更新（线程安全）
        void upsert(const GameEntry &entry);

        // 根据 crc32 删除（线程安全）
        bool removeByCrc32(int crc32);

        // 根据 path 删除（线程安全）
        bool removeByPath(const std::string &path);

        // 根据 crc32 查询（线程安全）
        std::optional<GameEntry> findByCrc32(int crc32) const;

        // 根据 path 查询（线程安全）
        std::optional<GameEntry> findByPath(const std::string &path) const;

        // 获取所有条目（线程安全，返回副本）
        std::vector<GameEntry> getAll() const;

        // 导出为 JSON（线程安全）
        nlohmann::json toJson() const;

        // 从 JSON 加载（会清空现有数据，线程安全）
        void fromJson(const nlohmann::json &j);

        // 清空所有数据（线程安全）
        void clear();

        // 保存到文件（线程安全）
        bool saveToFile(const std::string &filepath) const;

        // 从文件加载（线程安全）
        bool loadFromFile(const std::string &filepath);

        // ── 按平台分文件的读写接口 ─────────────────────────────────────────────

        /// 设置数据库目录（平台分文件存储时使用）
        void setDbDir(const std::string &dir);

        /// 从目录中按平台加载所有数据库文件并合并，若平台文件不存在则回退加载主文件。
        /// 此函数会清空现有数据。
        bool loadFromDir(const std::string &dir);

        /// 将数据按平台分组，分别保存到目录下的平台数据库文件
        bool saveToDir(const std::string &dir) const;

        // ── 通用字段访问接口（基于 JSON 中间层，方便新增字段无需修改调用代码）──

        /// 通过 crc32 设置游戏条目的某个字段（支持 GameEntry 中的所有 JSON 字段名）
        bool set(int crc32, const std::string &key, const nlohmann::json &value);

        /// 通过文件路径设置游戏条目的某个字段
        bool set(const std::string &path, const std::string &key, const nlohmann::json &value);

        /// 通过 crc32 获取游戏条目的某个字段，不存在则返回 defaultValue
        nlohmann::json get(int crc32, const std::string &key, const nlohmann::json &defaultValue = nullptr) const;

        /// 通过文件路径获取游戏条目的某个字段，不存在则返回 defaultValue
        nlohmann::json get(const std::string &path, const std::string &key, const nlohmann::json &defaultValue = nullptr) const;

        /// 通过 crc32 设置字段默认值：仅当该字段在 JSON 中不存在或为 null 时才写入
        bool setDefault(int crc32, const std::string &key, const nlohmann::json &defaultValue);

        /// 通过文件路径设置字段默认值：仅当该字段在 JSON 中不存在或为 null 时才写入
        bool setDefault(const std::string &path, const std::string &key, const nlohmann::json &defaultValue);

        // ─────────────────────────────────────────────────────────────────────

        // 手动触发保存（如果启用了自动保存，也会更新脏标记）
        // 同时保存主文件（filepath_）和平台分文件（dbDir_ 下各平台文件）
        bool flush();

        // 设置自动保存模式（运行时动态修改）
        void setAutoSaveMode(int mode, int intervalSeconds = 5);

        // 设置数据库文件路径
        void setFilePath(const std::string &filepath);

        // 获取最近玩的游戏列表，按 lastPlayed 降序排序，返回前 count 个条目
        std::vector<GameEntry> getRecentPlayed(int count) const;

        // 获取指定平台的所有游戏条目（返回副本）
        std::vector<GameEntry> getByPlatform(beiklive::enums::EmuPlatform platform) const;

    private:
        // 内部非线程安全的操作，调用时需持有写锁
        void doUpsert(const GameEntry &entry);
        bool doRemoveByCrc32(int crc32);
        bool doRemoveByPath(const std::string &path);
        std::optional<GameEntry> doFindByCrc32(int crc32) const;
        std::optional<GameEntry> doFindByPath(const std::string &path) const;
        void doClear();

        /// 内部：根据平台 int 值返回平台数据库文件名
        static std::string getPlatformFileName(int platform);

        // 标记数据已修改，并触发自动保存（如果需要）
        void markDirtyAndAutoSave();

        // 数据存储
        std::vector<GameEntry> data_;
        std::unordered_map<int, size_t> crc32Index_;
        std::unordered_map<std::string, size_t> pathIndex_;


        // 自动保存相关
        std::string filepath_;          ///< 合并主文件路径
        std::string dbDir_;             ///< 数据库目录（用于按平台分文件存储）
        int autoSaveMode_;     // 0: manual, 1: immediate, 2: periodic
        int autoSaveInterval_; // seconds (for mode 2)
        bool dirty_;           // 是否有未保存的修改
        mutable std::recursive_mutex m_mutex;  ///< 保护所有数据访问
    };

}
#endif // GAME_DATABASE_HPP