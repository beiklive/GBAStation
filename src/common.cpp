#include "common.hpp"
#include <set>

namespace beiklive {

// ─────────────────────────────────────────────────────────────────────────────
//  XMB background registry
// ─────────────────────────────────────────────────────────────────────────────

/// All ProImage instances created by InsertBackground() are tracked here
/// so that ApplyXmbColorToAll() can update them when the color setting changes.
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
        ApplyXmbColor(img);
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

// Returns display names (in order matching k_xmbColorPresets)
static const char* k_xmbColorNames[] = {
    "深蓝", "紫色", "绿色", "橙色", "红色", "青色", "黑色", "原版"
};

/// Apply the XMB colour preset stored in SettingManager to @a img.
/// Falls back to "blue" if the key is absent or unknown.
void ApplyXmbColor(beiklive::UI::ProImage* img)
{
    if (!img) return;
    std::string preset = "blue";
    if (SettingManager) {
        auto v = SettingManager->Get("UI.pspxmb.color");
        if (v) {
            if (auto s = v->AsString()) preset = *s;
        }
    }
    for (int i = 0; i < k_xmbColorPresetCount; ++i) {
        if (preset == k_xmbColorPresets[i].name) {
            img->setXmbBgColor(k_xmbColorPresets[i].r,
                               k_xmbColorPresets[i].g,
                               k_xmbColorPresets[i].b);
            return;
        }
    }
    // fallback to "blue"
    img->setXmbBgColor(0.05f, 0.10f, 0.25f);
}

    void swallow(brls::View* v, brls::ControllerButton btn)
    {
        v->registerAction("", btn, [](brls::View*) { return true; },
                          /*hidden=*/true);
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
        m_bgImage->setShaderAnimation(beiklive::UI::ShaderAnimationType::PSP_XMB_RIPPLE);
        ApplyXmbColor(m_bgImage);
        RegisterXmbBackground(m_bgImage);
        view->addView(m_bgImage);
    }
};
