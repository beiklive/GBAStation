#pragma once

#include <borealis.hpp>

#include "constexpr.h" // 一些常量定义
#include "enums.h"     // 枚举类型定义

#include "Utils/ConfigManager.hpp"


using namespace brls::literals; // for _i18n

namespace beiklive // 全局变量
{
    extern beiklive::ConfigManager *SettingManager;     // 全局配置管理器实例
    extern beiklive::ConfigManager *NameMappingManager; // 全局名称映射管理器实例
}
namespace beiklive // 全局功能函数
{
    inline std::string res_path(const std::string &path)
    {
#ifdef __SWITCH__
        return "romfs:/" + path;
#else
        return "./resources/" + path;
#endif
    }

    /// 检查是否存在某个键
    inline bool hasKey(beiklive::ConfigManager *manager, const std::string &key)
    {
        if (!manager)
            return false;
        return manager->Contains(key);
    }

    /// 读取 SettingManager 中某个游戏字段的字符串值。
    inline std::string getKeyStr(beiklive::ConfigManager *manager, const std::string &key, const std::string &def = "")
    {
        if (!manager)
            return def;
        auto v = manager->Get(key);
        if (!v)
            return def;
        if (auto s = v->AsString())
            return *s;
        return def;
    }

    /// 设置 SettingManager 中某个游戏字段并保存。
    inline void setKeyStr(beiklive::ConfigManager *manager, const std::string &key, const std::string &val)
    {
        if (!manager)
            return;
        manager->Set(key, beiklive::ConfigValue(val));
        manager->Save();
    }

    /// 读取 SettingManager 中某个游戏字段的整数值。
    inline int getKeyInt(beiklive::ConfigManager *manager, const std::string &key, int def = 0)
    {
        if (!manager)
            return def;
        auto v = manager->Get(key);
        if (!v)
            return def;
        if (auto i = v->AsInt())
            return *i;
        return def;
    }

    /// 设置 SettingManager 中某个游戏整数字段并保存。
    inline void setKeyInt(beiklive::ConfigManager *manager, const std::string &key, int val)
    {
        if (!manager)
            return;
        manager->Set(key, beiklive::ConfigValue(val));
        manager->Save();
    }

    /// 读取 SettingManager 中某个游戏字段的浮点值。
    inline float getKeyFloat(beiklive::ConfigManager *manager, const std::string &key, float def = 0.0f)
    {
        if (!manager)
            return def;
        auto v = manager->Get(key);
        if (!v)
            return def;
        if (auto f = v->AsFloat())
            return *f;
        if (auto i = v->AsInt())
            return static_cast<float>(*i);
        return def;
    }

    /// 设置 SettingManager 中某个游戏浮点字段并保存。
    inline void setKeyFloat(beiklive::ConfigManager *manager, const std::string &key, float val)
    {
        if (!manager)
            return;
        manager->Set(key, beiklive::ConfigValue(val));
        manager->Save();
    }

#define BK_RES(path) beiklive::res_path(path)
#define CHECK_KEY(key) beiklive::hasKey(beiklive::SettingManager, key)
#define GET_SETTING_KEY_STR(key, def) beiklive::getKeyStr(beiklive::SettingManager, key, def)
#define SET_SETTING_KEY_STR(key, val) beiklive::setKeyStr(beiklive::SettingManager, key, val)
#define GET_SETTING_KEY_INT(key, def) beiklive::getKeyInt(beiklive::SettingManager, key, def)
#define SET_SETTING_KEY_INT(key, val) beiklive::setKeyInt(beiklive::SettingManager, key, val)
#define GET_SETTING_KEY_FLOAT(key, def) beiklive::getKeyFloat(beiklive::SettingManager, key, def)
#define SET_SETTING_KEY_FLOAT(key, val) beiklive::setKeyFloat(beiklive::SettingManager, key, val)

#define GET_MAPPING_KEY_STR(key, def) beiklive::getKeyStr(beiklive::NameMappingManager, key, def)
#define SET_MAPPING_KEY_STR(key, val) beiklive::setKeyStr(beiklive::NameMappingManager, key, val)
#define GET_MAPPING_KEY_INT(key, def) beiklive::getKeyInt(beiklive::NameMappingManager, key, def)
#define SET_MAPPING_KEY_INT(key, val) beiklive::setKeyInt(beiklive::NameMappingManager, key, val)
#define GET_MAPPING_KEY_FLOAT(key, def) beiklive::getKeyFloat(beiklive::NameMappingManager, key, def)
#define SET_MAPPING_KEY_FLOAT(key, val) beiklive::setKeyFloat(beiklive::NameMappingManager, key, val)

#define HIDE_BRLS_HIGHLIGHT(view)                 \
    do                                            \
    {                                             \
        (view)->setHideHighlightBackground(true); \
        (view)->setHideHighlightBorder(true);     \
        (view)->setHideClickAnimation(true);      \
    } while (0)

#define HIDE_BRLS_BACKGROUND(view)                         \
    do                                                     \
    {                                                      \
        (view)->setBackground(brls::ViewBackground::NONE); \
    } while (0)

#define HIDE_BRLS_BAR(frame)                                  \
    do                                                        \
    {                                                         \
        (frame)->setHeaderVisibility(brls::Visibility::GONE); \
        (frame)->setFooterVisibility(brls::Visibility::GONE); \
    } while (0)

#define ADD_STYLE(name, value) \
    brls::Application::getStyle().addMetric(name, value)
#define ADD_THEME_COLOR(name, color) \
    brls::Application::getTheme().addColor(name, color)
#define GET_STYLE(name) \
    brls::Application::getStyle().getMetric(name)
#define GET_THEME_COLOR(name) \
    brls::Application::getTheme().getColor(name)

}

namespace beiklive // 结构体
{

    // 游戏条目结构体，包含游戏路径、显示标题、封面路径等字段, 用于在游戏列表中显示和管理游戏信息
    struct GameEntry
    {
        std::string path;       // 游戏文件路径
        std::string title;      // 显示标题（默认为映射名）
        std::string logoPath;   // 游戏封面图片路径
        int playCount = 0;      // 玩过的次数
        int playTime = 0;       // 玩过的总时间（单位：秒）
        std::string lastPlayed; // 上次玩的时间(时间戳字符串)
    };

    typedef std::vector<GameEntry> GameList; // 游戏列表类型定义

    // 列表元素数据
    struct ListItem
    {
        std::string text;     // 标题
        std::string subText;  // 子标题
        std::string iconPath; // 图标路径
        std::string data;     // 额外数据（如游戏路径）
    };

    typedef std::vector<ListItem> ListItemList; // 列表数据类型定义


    struct DirListData
    {
        std::string fileName; // 文件名（不含路径）
        std::string fullPath; // 完整路径
        std::string iconPath; // 图标路径
        beiklive::enums::FileType itemType; // 文件类型
        std::string fileSize;  // 文件大小（字节），目录为0
        size_t childCount;      // 子项数量，仅目录有效，文件为0
    };

} // namespace beiklive

namespace beiklive // 函数声明
{

    void ConfigureInit(); // 配置系统初始化，确保目录存在并加载配置文件
    void RegisterStyles(); // 注册全局样式
    void RegisterThemes(); // 注册全局主题色

} // namespace beiklive
