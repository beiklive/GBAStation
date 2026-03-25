#pragma once

#include <borealis.hpp>

#include "constexpr.h" // 一些常量定义
#include "enums.h"     // 枚举类型定义


#include "Utils/ConfigManager.hpp"

#include "UI/utils/Box.hpp"




using namespace brls::literals; // for _i18n


namespace beiklive
{
    extern beiklive::ConfigManager* SettingManager; // 全局配置管理器实例
    extern beiklive::ConfigManager* NameMappingManager; // 全局名称映射管理器实例


/// 读取 SettingManager 中某个游戏字段的字符串值。
inline std::string getKeyStr( beiklive::ConfigManager* manager, const std::string& key, const std::string& def = "")
{
    if (!manager) return def;
    auto v = manager->Get(key);
    if (!v) return def;
    if (auto s = v->AsString()) return *s;
    return def;
}

/// 设置 SettingManager 中某个游戏字段并保存。
inline void setKeyStr( beiklive::ConfigManager* manager, const std::string& key, const std::string& val)
{
    if (!manager) return;
    manager->Set(key, beiklive::ConfigValue(val));
    manager->Save();
}

/// 读取 SettingManager 中某个游戏字段的整数值。
inline int getKeyInt( beiklive::ConfigManager* manager, const std::string& key, int def = 0)
{
    if (!manager) return def;
    auto v = manager->Get(key);
    if (!v) return def;
    if (auto i = v->AsInt()) return *i;
    return def;
}

/// 设置 SettingManager 中某个游戏整数字段并保存。
inline void setKeyInt( beiklive::ConfigManager* manager, const std::string& key, int val)
{
    if (!manager) return;
    manager->Set(key, beiklive::ConfigValue(val));
    manager->Save();
}

/// 读取 SettingManager 中某个游戏字段的浮点值。
inline float getKeyFloat( beiklive::ConfigManager* manager, const std::string& key, float def = 0.0f)
{
    if (!manager) return def;
    auto v = manager->Get(key);
    if (!v) return def;
    if (auto f = v->AsFloat()) return *f;
    if (auto i = v->AsInt())   return static_cast<float>(*i);
    return def;
}

/// 设置 SettingManager 中某个游戏浮点字段并保存。
inline void setKeyFloat( beiklive::ConfigManager* manager, const std::string& key, float val)
{
    if (!manager) return;
    manager->Set(key, beiklive::ConfigValue(val));
    manager->Save();
}


#define GET_SETTING_KEY_STR(key, def)       getKeyStr(SettingManager, key, def)
#define SET_SETTING_KEY_STR(key, val)       setKeyStr(SettingManager, key, val)
#define GET_SETTING_KEY_INT(key, def)       getKeyInt(SettingManager, key, def)
#define SET_SETTING_KEY_INT(key, val)       setKeyInt(SettingManager, key, val)
#define GET_SETTING_KEY_FLOAT(key, def)     getKeyFloat(SettingManager, key, def)
#define SET_SETTING_KEY_FLOAT(key, val)     setKeyFloat(SettingManager, key, val)

#define GET_MAPPING_KEY_STR(key, def)       getKeyStr(NameMappingManager, key, def)
#define SET_MAPPING_KEY_STR(key, val)       setKeyStr(NameMappingManager, key, val)
#define GET_MAPPING_KEY_INT(key, def)       getKeyInt(NameMappingManager, key, def)
#define SET_MAPPING_KEY_INT(key, val)       setKeyInt(NameMappingManager, key, val)
#define GET_MAPPING_KEY_FLOAT(key, def)     getKeyFloat(NameMappingManager, key, def)
#define SET_MAPPING_KEY_FLOAT(key, val)     setKeyFloat(NameMappingManager, key, val)


#define HIDE_BRLS_BAR(frame) \
    do { \
        (frame)->setHeaderVisibility(brls::Visibility::GONE); \
        (frame)->setFooterVisibility(brls::Visibility::GONE); \
    } while(0)

#define ADD_STYLE(name, value) \
    brls::Application::getStyle().addMetric(name, value)
#define ADD_THEME_COLOR(name, color) \
    brls::Application::getTheme().addColor(name, color)
#define GET_STYLE(name) \
    brls::Application::getStyle().getMetric(name)
#define GET_THEME_COLOR(name) \
    brls::Application::getTheme().getColor(name)



void ConfigureInit();
void RegisterStyles();
void RegisterThemes();




} // namespace beiklive

