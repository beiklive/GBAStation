#pragma once
#include <SQLiteCpp/SQLiteCpp.h>

namespace beiklive
{
    class DataBase
    {
    public:
        DataBase(const std::string& dbPath);
        ~DataBase();

        // 其他数据库操作方法，如查询、插入、更新等

    private:
        SQLite::Database* m_db = nullptr; // SQLiteCpp 数据库对象指针
    };


} // namespace beiklive
