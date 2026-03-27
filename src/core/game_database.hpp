#ifndef GAME_DATABASE_HPP
#define GAME_DATABASE_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

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

        // 修改游玩统计（线程安全）
        bool updatePlayStats(int crc32, int newPlayCount, int newPlayTime, const std::string &newLastPlayed);

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

        // 手动触发保存（如果启用了自动保存，也会更新脏标记）
        bool flush();

        // 设置自动保存模式（运行时动态修改）
        void setAutoSaveMode(int mode, int intervalSeconds = 5);

        // 设置数据库文件路径
        void setFilePath(const std::string &filepath);

        // 获取最近玩的游戏列表，按 lastPlayed 降序排序，返回前 count 个条目
        std::vector<GameEntry> getRecentPlayed(int count) const;

    private:
        // 内部非线程安全的操作，调用时需持有写锁
        void doUpsert(const GameEntry &entry);
        bool doRemoveByCrc32(int crc32);
        bool doRemoveByPath(const std::string &path);
        std::optional<GameEntry> doFindByCrc32(int crc32) const;
        std::optional<GameEntry> doFindByPath(const std::string &path) const;
        bool doUpdatePlayStats(int crc32, int newPlayCount, int newPlayTime, const std::string &newLastPlayed);
        void doClear();

        // 标记数据已修改，并触发自动保存（如果需要）
        void markDirtyAndAutoSave();

        // 数据存储
        std::vector<GameEntry> data_;
        std::unordered_map<int, size_t> crc32Index_;
        std::unordered_map<std::string, size_t> pathIndex_;


        // 自动保存相关
        std::string filepath_;
        int autoSaveMode_;     // 0: manual, 1: immediate, 2: periodic
        int autoSaveInterval_; // seconds (for mode 2)
        bool dirty_;           // 是否有未保存的修改
    };

}
#endif // GAME_DATABASE_HPP