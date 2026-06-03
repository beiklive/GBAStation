#pragma once

#include <borealis.hpp>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unordered_set>
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
namespace beiklive // 全局变量 动态背景
{
    struct FloatingIcon
    {
        float x;
        float y;
        float speedX;
        float speedY;
        float size;
        float rotation;
        float rotateSpeed;
        float alpha;
        int symbolIndex;
    };

    extern std::vector<FloatingIcon> g_backgroundIcons;
    extern float g_backgroundLastTime;
    extern std::unordered_set<std::string> g_forceRefreshPaths;

    enum class GradientTheme
    {
        Midnight,      // 默认深夜蓝
        LemonYellow,   // 柠檬黄
        AvocadoGreen,  // 牛油果绿
        StrawberryRed, // 草莓红
        OceanBlue,     // 海洋蓝
        SakuraPink,    // 樱花粉
        VscodeBlack,     // VSCode 黑
    };

    extern GradientTheme g_gradientTheme;

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
                                          float yOffset = 0.0f,
                                          int integerScaleMult = 0)
    {
        DisplayRect r;
        const float gw = static_cast<float>(gameW);
        const float gh = static_cast<float>(gameH);
        if (gw <= 0.f || gh <= 0.f) return r;

        float scale = 1.0f;
        bool useOffsets = false;
        switch (mode)
        {
            case ScreenMode::Fit:
                scale = std::min(viewW / gw, viewH / gh);
                break;
            case ScreenMode::Fill:
                r.w = viewW;
                r.h = viewH;
                r.x = viewX;
                r.y = viewY;
                return r;
            case ScreenMode::IntegerScale:
                if (integerScaleMult > 0)
                    scale = static_cast<float>(integerScaleMult);
                else {
                    float s = std::min(viewW / gw, viewH / gh);
                    scale = std::max(1.0f, std::floor(s));
                }
                break;
            case ScreenMode::FreeScale:
                scale = customScale;
                useOffsets = true;
                break;
        }

        r.w = gw * scale;
        r.h = gh * scale;
        r.x = viewX + (viewW - r.w) * 0.5f + (useOffsets ? xOffset : 0.f);
        r.y = viewY + (viewH - r.h) * 0.5f + (useOffsets ? yOffset : 0.f);
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
            #elif defined(__APPLE__)
                return  beiklive::path::corePath() + beiklive::path::SPLIT_CHAR + std::string("mgba_libretro.dylib");
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

// 禁用左右导航
#define DISABLE_LR_NAVIGATION(panel) \
    do                                \
    {                                 \
        (panel)->setCustomNavigationRoute(brls::FocusDirection::RIGHT, panel);  \
        (panel)->setCustomNavigationRoute(brls::FocusDirection::LEFT, panel);   \
    } while (0)

// 设置上下循环导航
#define UP_DOWN_NAVIGATION(firstTab, lastTab) \
    do                                            \
    {                                             \
        (firstTab)->setCustomNavigationRoute(brls::FocusDirection::UP, lastTab);   \
        (lastTab)->setCustomNavigationRoute(brls::FocusDirection::DOWN, firstTab); \
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
    void InitBackgroundIcons(); // 初始化全局背景图标
    void UpdateBackgroundIcons(float dt, float width, float height); // 更新全局背景图标
    void GetGradientColors(NVGcolor& top, NVGcolor& bottom); // 根据全局主题获取渐变色
    void RegisterStyles(); // 注册全局样式
    void RegisterThemes(); // 注册全局主题色

    std::vector<CheatEntry> parseChtFile(const std::string& path); // 解析 .cht 金手指文件，返回金手指条目列表
    bool saveChtFile(const std::string& path, const std::vector<CheatEntry>& entries); // 将金手指列表以 .cht 格式写入文件


    int GetGamePixelHeight(int platform); // 获取游戏的原始像素高度（如 GBA 为 160）
    int GetGamePixelWidth(int platform);  // 获取游戏的原始像素宽度（如 GBA 为 240）

    std::string GetGameLogoLayerPath(int platform); // 获取平台默认封面图路径（如 GBA 的默认封面图）

    // GB/GBC 配色预设列表
    inline const std::vector<std::string>& GetGbColorPresets()
    {
        static const std::vector<std::string> presets = {
            "Grayscale", "DMG Green", "GB Pocket", "GB Light",
            "GBC Brown ↑", "GBC Red ↑A", "GBC Dark Brown ↑B",
            "GBC Pale Yellow ↓", "GBC Orange ↓A", "GBC Yellow ↓B",
            "GBC Blue ←", "GBC Dark Blue ←A", "GBC Gray ←B",
            "GBC Green →", "GBC Dark Green →A", "GBC Reverse →B",
            "SGB 1-A", "SGB 1-B", "SGB 1-C", "SGB 1-D", "SGB 1-E", "SGB 1-F", "SGB 1-G", "SGB 1-H",
            "SGB 2-A", "SGB 2-B", "SGB 2-C", "SGB 2-D", "SGB 2-E", "SGB 2-F", "SGB 2-G", "SGB 2-H",
            "SGB 3-A", "SGB 3-B", "SGB 3-C", "SGB 3-D", "SGB 3-E", "SGB 3-F", "SGB 3-G", "SGB 3-H",
            "SGB 4-A", "SGB 4-B", "SGB 4-C", "SGB 4-D", "SGB 4-E", "SGB 4-F", "SGB 4-G", "SGB 4-H"
        };
        return presets;
    }
} // namespace beiklive
