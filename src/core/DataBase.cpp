#include "DataBase.hpp"

namespace beiklive
{
    DataBase::DataBase(const std::string& dbPath)
    {
        try
        {
            m_db = new SQLite::Database(dbPath, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
            // 可以在这里执行一些初始化操作，如创建表等
        }
        catch (const std::exception& e)
        {
            m_db = nullptr;
        }
    }

    DataBase::~DataBase()
    {
        if (m_db)
        {
            delete m_db;
            m_db = nullptr;
        }
    }

    // 其他数据库操作方法的实现可以在这里添加，如查询、插入、更新等



} // namespace beiklive
