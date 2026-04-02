#pragma once

#include <borealis.hpp>
#include <atomic>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <deque>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include "constexpr.h" // 一些常量定义
#include "enums.h"     // 枚举类型定义

#include "game_database.hpp"
#include "ConfigManager.hpp"
#include "json.hpp" // JSON 处理库


using namespace brls::literals; // for _i18n
using json = nlohmann::json;
namespace beiklive // 全局变量
{
    extern beiklive::ConfigManager *SettingManager;     // 全局配置管理器实例
    extern beiklive::ConfigManager *NameMappingManager; // 全局名称映射管理器实例
    extern beiklive::GameDatabase *GameDB; // 全局游戏数据库实例

}



namespace beiklive // 全局功能函数
{


    /// 根据画面模式计算游戏帧在视图区域内的绘制矩形
    inline DisplayRect computeDisplayRect(ScreenMode mode,
                                          float viewX, float viewY,
                                          float viewW, float viewH,
                                          unsigned gameW, unsigned gameH,
                                          float customScale = 1.0f,
                                          float xOffset = 0.0f,
                                          float yOffset = 0.0f)
    {
        DisplayRect r;
        const float gw = static_cast<float>(gameW);
        const float gh = static_cast<float>(gameH);
        if (gw <= 0.f || gh <= 0.f) return r;

        float scale = 1.0f;
        switch (mode)
        {
            case ScreenMode::Fit:
                scale = std::min(viewW / gw, viewH / gh);
                break;
            case ScreenMode::Fill:
                r.w = viewW;
                r.h = viewH;
                r.x = viewX + xOffset;
                r.y = viewY + yOffset;
                return r;
            case ScreenMode::IntegerScale:
            {
                float s = std::min(viewW / gw, viewH / gh);
                scale = std::max(1.0f, std::floor(s));
                break;
            }
            case ScreenMode::FreeScale:
                scale = customScale;
                break;
        }

        r.w = gw * scale;
        r.h = gh * scale;
        r.x = viewX + (viewW - r.w) * 0.5f + xOffset;
        r.y = viewY + (viewH - r.h) * 0.5f + yOffset;
        return r;
    }


     inline std::string GetCorePath(int platform){
        switch (platform)
        {
        case (int)beiklive::enums::EmuPlatform::EmuGBA:
        case (int)beiklive::enums::EmuPlatform::EmuGBC:
        case (int)beiklive::enums::EmuPlatform::EmuGB:
            #if defined(__SWITCH__)
                return "";  // switch平台直接静态链接
            #elif defined(_WIN32)
                return  std::string("mgba_libretro.dll");
            #endif
        default:
            return "";
        }
    }


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

namespace beiklive // 函数声明
{

    void ConfigureInit();  // 配置系统初始化，确保目录存在并加载配置文件
    void RegisterStyles(); // 注册全局样式
    void RegisterThemes(); // 注册全局主题色

    std::vector<CheatEntry> parseChtFile(const std::string& path); // 解析 .cht 金手指文件，返回金手指条目列表
    bool saveChtFile(const std::string& path, const std::vector<CheatEntry>& entries); // 将金手指列表以 .cht 格式写入文件


    int GetGamePixelHeight(int platform); // 获取游戏的原始像素高度（如 GBA 为 160）
    int GetGamePixelWidth(int platform);  // 获取游戏的原始像素宽度（如 GBA 为 240）


} // namespace beiklive
