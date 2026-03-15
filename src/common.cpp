#include "common.hpp"
#include <borealis/core/cache_helper.hpp>
#include <set>

/// 每当最近游戏队列变化时置为 true，StartPageView 每帧检查此标志并刷新游戏列表
bool g_recentGamesDirty = false;

namespace beiklive {

// ─────────────────────────────────────────────────────────────────────────────
//  XMB 背景注册表
// ─────────────────────────────────────────────────────────────────────────────

/// 所有由 InsertBackground() 创建的 ProImage 实例均在此跟踪，
/// 以便颜色设置变化时 ApplyXmbColorToAll() 统一更新
static std::set<beiklive::UI::ProImage*> s_xmbBackgrounds;

void RegisterXmbBackground(beiklive::UI::ProImage* img)
{
    if (img) s_xmbBackgrounds.insert(img);
}

void UnregisterXmbBackground(beiklive::UI::ProImage* img)
{
    if (img) s_xmbBackgrounds.erase(img);
}

void ApplyXmbColorToAll()
{
    for (auto* img : s_xmbBackgrounds)
        ApplyXmbBg(img);
}

// ─────────────────────────────────────────────────────────────────────────────
//  XMB colour preset table
// ─────────────────────────────────────────────────────────────────────────────

struct XmbColorPreset { const char* name; float r, g, b; };

static const XmbColorPreset k_xmbColorPresets[] = {
    { "blue",     0.05f, 0.10f, 0.25f },   // 深蓝
    { "purple",   0.15f, 0.05f, 0.30f },   // 紫色
    { "green",    0.05f, 0.20f, 0.10f },   // 绿色
    { "orange",   0.30f, 0.15f, 0.05f },   // 橙色
    { "red",      0.25f, 0.05f, 0.05f },   // 红色
    { "cyan",     0.05f, 0.20f, 0.25f },   // 青色
    { "black",    0.02f, 0.02f, 0.05f },   // 黑色
    { "original", 0.74f, 0.86f, 0.14f },   // 原版
};
static constexpr int k_xmbColorPresetCount =
    static_cast<int>(sizeof(k_xmbColorPresets) / sizeof(k_xmbColorPresets[0]));

// 返回各预设的显示名称（顺序与 k_xmbColorPresets 一致）
static const char* k_xmbColorNames[] = {
    "深蓝", "紫色", "绿色", "橙色", "红色", "青色", "黑色", "原版"
};

/// 将 SettingManager 中存储的 XMB 颜色预设应用到 img，未知预设回退为 "blue"
void ApplyXmbColor(beiklive::UI::ProImage* img)
{
    if (!img) return;
    std::string preset = cfgGetStr("UI.pspxmb.color", "blue");
    for (int i = 0; i < k_xmbColorPresetCount; ++i) {
        if (preset == k_xmbColorPresets[i].name) {
            img->setXmbBgColor(k_xmbColorPresets[i].r,
                               k_xmbColorPresets[i].g,
                               k_xmbColorPresets[i].b);
            return;
        }
    }
    // 回退为 "blue"
    img->setXmbBgColor(0.05f, 0.10f, 0.25f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  配置读写辅助函数（声明于 common.hpp，实现于 common.cpp）
// ─────────────────────────────────────────────────────────────────────────────

bool cfgGetBool(const std::string& key, bool def)
{
    if (!SettingManager) return def;
    auto v = SettingManager->Get(key);
    if (!v) {
        // brls::Logger::debug("cfgGetBool [{}]: missing, returning default={}", key, def);
        return def;
    }
    bool result = def;
    if (auto s = v->AsString())      result = (*s == "true" || *s == "1" || *s == "yes");
    else if (auto i = v->AsInt())    result = (*i != 0);
    else                             result = def;
    // brls::Logger::debug("cfgGetBool [{}] = {}", key, result);
    return result;
}

std::string cfgGetStr(const std::string& key, const std::string& def)
{
    if (!SettingManager) return def;
    auto v = SettingManager->Get(key);
    if (!v) {
        // brls::Logger::debug("cfgGetStr [{}]: missing, returning default", key);
        return def;
    }
    if (auto s = v->AsString()) {
        // brls::Logger::debug("cfgGetStr [{}] = '{}'", key, *s);
        return *s;
    }
    return def;
}

float cfgGetFloat(const std::string& key, float def)
{
    if (!SettingManager) return def;
    auto v = SettingManager->Get(key);
    if (!v) {
        // brls::Logger::debug("cfgGetFloat [{}]: missing, returning default={}", key, def);
        return def;
    }
    if (auto f = v->AsFloat()) {
        // brls::Logger::debug("cfgGetFloat [{}] = {}", key, *f);
        return *f;
    }
    if (auto i = v->AsInt()) {
        float result = static_cast<float>(*i);
        // brls::Logger::debug("cfgGetFloat [{}] = {} (from int)", key, result);
        return result;
    }
    return def;
}

int cfgGetInt(const std::string& key, int def)
{
    if (!SettingManager) return def;
    auto v = SettingManager->Get(key);
    if (!v) {
        // brls::Logger::debug("cfgGetInt [{}]: missing, returning default={}", key, def);
        return def;
    }
    if (auto i = v->AsInt()) {
        // brls::Logger::debug("cfgGetInt [{}] = {}", key, *i);
        return *i;
    }
    if (auto f = v->AsFloat()) {
        int result = static_cast<int>(*f);
        // brls::Logger::debug("cfgGetInt [{}] = {} (from float)", key, result);
        return result;
    }
    return def;
}

void cfgSetStr(const std::string& key, const std::string& val)
{
    // brls::Logger::debug("cfgSetStr [{}] = '{}'", key, val);
    if (SettingManager) {
        SettingManager->Set(key, beiklive::ConfigValue(val));
        SettingManager->Save();
    }
}

void cfgSetBool(const std::string& key, bool val)
{
    // brls::Logger::debug("cfgSetBool [{}] = {}", key, val);
    cfgSetStr(key, val ? "true" : "false");
}

/// 根据 SettingManager 配置将所有背景设置应用到 img：
///  - UI.showXmbBg       → 启用/禁用 PSP XMB 波纹着色器
///  - UI.pspxmb.color    → XMB 背景颜色
///  - UI.showBgImage + UI.bgImagePath → 背景图片
///  - UI.bgBlurEnabled / UI.bgBlurRadius → Kawase 模糊
///  - 两项均禁用时设为 GONE
void ApplyXmbBg(beiklive::UI::ProImage* img)
{
    if (!img) return;

    bool showXmb = cfgGetBool("UI.showXmbBg",    false);
    bool showBg  = cfgGetBool("UI.showBgImage",   false);
    std::string bgPath = cfgGetStr("UI.bgImagePath", "");

    // 两项功能均未启用时隐藏背景控件
    if (!showXmb && !showBg) {
        img->setVisibility(brls::Visibility::GONE);
        return;
    }

    img->setVisibility(brls::Visibility::VISIBLE);

    // 先绘制背景图片，XMB 着色器叠加其上
    if (showBg && !bgPath.empty()) {
        img->setImageFromFile(bgPath);
    } else {
        // 背景图片已禁用或路径未设置：清除纹理使其立即消失
        img->clear();
    }

    // 应用模糊设置
    bool blurEnabled = cfgGetBool("UI.bgBlurEnabled", false);
    float blurRadius = cfgGetFloat("UI.bgBlurRadius",  12.0f);
    img->setBlurEnabled(blurEnabled);
    img->setBlurRadius(blurRadius);

    // 应用 XMB 着色器动画与颜色
    if (showXmb) {
        img->setShaderAnimation(beiklive::UI::ShaderAnimationType::PSP_XMB_RIPPLE);
        ApplyXmbColor(img);
    } else {
        img->setShaderAnimation(beiklive::UI::ShaderAnimationType::NONE);
    }
}

    void swallow(brls::View* v, brls::ControllerButton btn)
    {
        v->registerAction("", btn, [](brls::View*) { return true; },
                          /*hidden=*/true);
    }

    void clearUIImageCache()
    {
        // 释放所有缓存的文件字节
        beiklive::UI::ImageFileCache::instance().clear();
        // 标记所有 brls TextureCache 条目为脏，确保后续加载从磁盘重新读取而非使用旧 GPU 纹理句柄
        brls::TextureCache::instance().cache.markAllDirty();
    }


    void CheckGLSupport()
    {
        #ifdef BOREALIS_USE_OPENGL
        #  ifdef USE_GLES3
            brls::Logger::info("Using OpenGL ES 3 backend");
        #  elif defined(USE_GLES2)
            brls::Logger::info("Using OpenGL ES 2 backend");
        #  elif defined(USE_GL2)
            brls::Logger::info("Using OpenGL 2 backend");
        #  else
            brls::Logger::info("Using OpenGL 3 backend");
        #  endif
        #else
            brls::Logger::info("Using non-OpenGL backend");
        #endif
    }

    void InsertBackground(brls::Box *view)
    {
        #undef ABSOLUTE // avoid conflict with brls::PositionType::ABSOLUTE

        auto *m_bgImage = new beiklive::UI::ProImage();
        m_bgImage->setFocusable(false);
        m_bgImage->setPositionType(brls::PositionType::ABSOLUTE);
        m_bgImage->setPositionTop(0);
        m_bgImage->setPositionLeft(0);
        m_bgImage->setWidthPercentage(100);
        m_bgImage->setHeightPercentage(100);
        m_bgImage->setScalingType(brls::ImageScalingType::FIT);
        m_bgImage->setInterpolation(brls::ImageInterpolation::LINEAR);
        // 应用所有背景设置（可见性、图片、XMB着色器与颜色）
        ApplyXmbBg(m_bgImage);
        RegisterXmbBackground(m_bgImage);
        view->addView(m_bgImage);
    }
};
