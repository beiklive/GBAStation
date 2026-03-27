#pragma once
#include <SQLiteCpp/SQLiteCpp.h>
#include <memory>
#include <mutex>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>
#include <SQLiteCpp/Exception.h>
#include "constexpr.h"
namespace beiklive
{

    class DB
    {
    public:
        // 设置数据库路径（必须在首次 get() 之前调用）
        static void setPath(const std::string &path)
        {
            std::lock_guard<std::mutex> lock(mutex());
            dbPath() = path;
        }

        // 获取数据库实例
        static SQLite::Database &get()
        {
            std::lock_guard<std::mutex> lock(mutex());

            if (!instance())
            {
                std::string path = dbPath().empty() ? defaultPath() : dbPath();

                instance() = std::make_unique<SQLite::Database>(
                    path,
                    SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

                init(*instance());
            }

            return *instance();
        }

    private:
        static std::unique_ptr<SQLite::Database> &instance()
        {
            static std::unique_ptr<SQLite::Database> inst;
            return inst;
        }

        static std::string &dbPath()
        {
            static std::string path;
            return path;
        }

        static std::mutex &mutex()
        {
            static std::mutex m;
            return m;
        }

        static std::string defaultPath()
        {
            return beiklive::path::databaseFilePath(); // 默认当前目录
        }

        static void init(SQLite::Database &db)
        {
            // 高性能配置
            db.exec("PRAGMA journal_mode=WAL;");
            db.exec("PRAGMA synchronous=NORMAL;");
            db.exec("PRAGMA temp_store=MEMORY;");
        }


    };

} // namespace beiklive
