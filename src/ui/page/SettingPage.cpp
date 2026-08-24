#include "ui/page/SettingPage.hpp"
#include "ui/page/FileListPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "ui/utils/UiHelper.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "ui/utils/GradientFocus.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/widget/VideoBackgroundView.hpp"

#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_selector.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/header.hpp>
#include "ui/widget/DetailCell.hpp"
#include <borealis/views/scrolling_frame.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/applet_frame.hpp>

#include "core/Tools.hpp"
#include "core/Translation.hpp"
#include "core/SteamGridDb.hpp"
#include "core/ThreadPool.hpp"
#include "core/constexpr.h"
#include "game/audio/AudioManager.hpp"
#include "game/control/InputMappingDefaults.hpp"
#include "game/retro/LibretroLoader.hpp"

#include <chrono>
#include <array>
#include <atomic>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>
#include <sstream>
#include <iomanip>
#include <memory>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace beiklive
{

// ─────────────────────────────────────────────────────────────────────────────
//  配置读写辅助函数（基于 src 的 ConfigManager 宏）
// ─────────────────────────────────────────────────────────────────────────────

static bool cfgGetBool(const std::string &key, bool def)
{
    return GET_SETTING_KEY_INT(key, def ? 1 : 0) != 0;
}

static void cfgSetBool(const std::string &key, bool val)
{
    SET_SETTING_KEY_INT(key, val ? 1 : 0);
}

static int cfgGetInt(const std::string &key, int def)
{
    return GET_SETTING_KEY_INT(key, def);
}

static void cfgSetInt(const std::string &key, int val)
{
    SET_SETTING_KEY_INT(key, val);
}

static std::string cfgGetStr(const std::string &key, const std::string &def)
{
    return GET_SETTING_KEY_STR(key, def);
}

static void cfgSetStr(const std::string &key, const std::string &val)
{
    SET_SETTING_KEY_STR(key, val);
}

static std::string pointerHotkeyHint(const char* key)
{
    if (std::strcmp(key, "hotkey.pointer_mode.pad") == 0)
        return L("启用或关闭右摇杆控制的 NDS 触控指针");
    if (std::strcmp(key, "hotkey.pointer_click.pad") == 0)
        return L("按住时在指针当前位置触摸 NDS 下屏");
    if (std::strcmp(key, "hotkey.swap_screens.pad") == 0)
        return L("交换 NDS 上屏与下屏的显示位置");
    if (std::strcmp(key, "hotkey.mic_input.pad") == 0)
        return L("按住时向 NDS 模拟麦克风输入吹气白噪声");
    return L("可绑定单键或双键组合");
}

// ─────────────────────────────────────────────────────────────────────────────
//  布局辅助函数（已移至 ui/utils/UiHelper.hpp）
// ─────────────────────────────────────────────────────────────────────────────
class SettingsSectionHeaderView final : public brls::View
{
public:
    explicit SettingsSectionHeaderView(std::string title)
        : m_title(std::move(title))
    {
        setHeight(58.f);
        setFocusable(false);
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        (void)style;
        (void)ctx;
        if (m_font < 0)
            m_font = brls::Application::getDefaultFont();
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x + 2.f, y + 17.f, 4.f, 24.f, 2.f);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 225));
        nvgFill(vg);
        nvgFontFaceId(vg, m_font);
        nvgFontSize(vg, 20.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, uiTextPrimary(0.92f));
        nvgText(vg, x + 18.f, y + 29.f, m_title.c_str(), nullptr);
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 18.f, y + 52.f);
        nvgLineTo(vg, x + w - 6.f, y + 52.f);
        nvgStrokeColor(vg, uiDivider());
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
    }

private:
    std::string m_title;
    int m_font = -1;
};

static brls::Label* makeHint(const std::string& text)
{
    auto* label = new brls::Label();
    label->setText(text);
    label->setFontSize(14.f);
    label->setTextColor(uiTextSecondary(0.80f));
    label->setMarginTop(4.f);
    label->setMarginBottom(10.f);
    label->setMarginLeft(16.f);
    label->setMarginRight(16.f);
    label->setSingleLine(false);
    label->setFocusable(false);
    return label;
}

static SettingsSectionHeaderView* makeHeader(const std::string& title)
{
    return new SettingsSectionHeaderView(title);
}

static brls::Box* makeContentBox()
{
    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(12.f, 24.f, 24.f, 24.f);
    return box;
}

static brls::ScrollingFrame* makeScrollTab()
{
    auto* scroll = new brls::ScrollingFrame();
    scroll->setGrow(1.f);
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    scroll->setScrollingIndicatorVisible(false);
    scroll->setFocusable(false);
    return scroll;
}

static int findIndex(const std::vector<std::string> &options,
                     const std::string &val, int defaultIdx = 0)
{
    for (int i = 0; i < (int)options.size(); ++i)
        if (options[i] == val)
            return i;
    return defaultIdx;
}

// ─────────────────────────────────────────────────────────────────────────────
//  KeyCaptureView（按键捕获全屏页）
// ─────────────────────────────────────────────────────────────────────────────

struct CapPadKey
{
    const char *name;
    brls::ControllerButton btn;
};

static const CapPadKey k_capPadKeys[] = {
    {"PAD_LT", brls::BUTTON_LT}, 
    {"PAD_LB", brls::BUTTON_LB}, 
    {"PAD_LSB", brls::BUTTON_LSB},
    {"PAD_UP", brls::BUTTON_UP}, 
    {"PAD_RIGHT", brls::BUTTON_RIGHT},
    {"PAD_DOWN", brls::BUTTON_DOWN}, 
    {"PAD_LEFT", brls::BUTTON_LEFT},
    {"PAD_BACK", brls::BUTTON_BACK}, 
    {"PAD_START", brls::BUTTON_START},
    {"PAD_RSB", brls::BUTTON_RSB}, 
    {"PAD_Y", brls::BUTTON_Y},
    {"PAD_B", brls::BUTTON_B}, 
    {"PAD_A", brls::BUTTON_A}, 
    {"PAD_X", brls::BUTTON_X},
    {"PAD_RB", brls::BUTTON_RB}, 
    {"PAD_RT", brls::BUTTON_RT},
};
static constexpr int k_capPadKeyCount =
    static_cast<int>(sizeof(k_capPadKeys) / sizeof(k_capPadKeys[0]));

static constexpr int k_capMaxKeys = 2; ///< 组合键最大按键数

struct CapKbdKey
{
    brls::BrlsKeyboardScancode scancode;
    const char* name;
};

static const CapKbdKey k_capKbdKeys[] = {
    // 字母
    { brls::BRLS_KBD_KEY_A, "A" }, { brls::BRLS_KBD_KEY_B, "B" },
    { brls::BRLS_KBD_KEY_C, "C" }, { brls::BRLS_KBD_KEY_D, "D" },
    { brls::BRLS_KBD_KEY_E, "E" }, { brls::BRLS_KBD_KEY_F, "F" },
    { brls::BRLS_KBD_KEY_G, "G" }, { brls::BRLS_KBD_KEY_H, "H" },
    { brls::BRLS_KBD_KEY_I, "I" }, { brls::BRLS_KBD_KEY_J, "J" },
    { brls::BRLS_KBD_KEY_K, "K" }, { brls::BRLS_KBD_KEY_L, "L" },
    { brls::BRLS_KBD_KEY_M, "M" }, { brls::BRLS_KBD_KEY_N, "N" },
    { brls::BRLS_KBD_KEY_O, "O" }, { brls::BRLS_KBD_KEY_P, "P" },
    { brls::BRLS_KBD_KEY_Q, "Q" }, { brls::BRLS_KBD_KEY_R, "R" },
    { brls::BRLS_KBD_KEY_S, "S" }, { brls::BRLS_KBD_KEY_T, "T" },
    { brls::BRLS_KBD_KEY_U, "U" }, { brls::BRLS_KBD_KEY_V, "V" },
    { brls::BRLS_KBD_KEY_W, "W" }, { brls::BRLS_KBD_KEY_X, "X" },
    { brls::BRLS_KBD_KEY_Y, "Y" }, { brls::BRLS_KBD_KEY_Z, "Z" },
    // 数字
    { brls::BRLS_KBD_KEY_0, "0" }, { brls::BRLS_KBD_KEY_1, "1" },
    { brls::BRLS_KBD_KEY_2, "2" }, { brls::BRLS_KBD_KEY_3, "3" },
    { brls::BRLS_KBD_KEY_4, "4" }, { brls::BRLS_KBD_KEY_5, "5" },
    { brls::BRLS_KBD_KEY_6, "6" }, { brls::BRLS_KBD_KEY_7, "7" },
    { brls::BRLS_KBD_KEY_8, "8" }, { brls::BRLS_KBD_KEY_9, "9" },
    // 功能键
    { brls::BRLS_KBD_KEY_F1,  "F1"  }, { brls::BRLS_KBD_KEY_F2,  "F2"  },
    { brls::BRLS_KBD_KEY_F3,  "F3"  }, { brls::BRLS_KBD_KEY_F4,  "F4"  },
    { brls::BRLS_KBD_KEY_F5,  "F5"  }, { brls::BRLS_KBD_KEY_F6,  "F6"  },
    { brls::BRLS_KBD_KEY_F7,  "F7"  }, { brls::BRLS_KBD_KEY_F8,  "F8"  },
    { brls::BRLS_KBD_KEY_F9,  "F9"  }, { brls::BRLS_KBD_KEY_F10, "F10" },
    { brls::BRLS_KBD_KEY_F11, "F11" }, { brls::BRLS_KBD_KEY_F12, "F12" },
    // 特殊键
    { brls::BRLS_KBD_KEY_SPACE,     "Space"     },
    { brls::BRLS_KBD_KEY_ENTER,     "Enter"     },
    { brls::BRLS_KBD_KEY_TAB,       "Tab"       },
    { brls::BRLS_KBD_KEY_ESCAPE,    "Esc"       },
    { brls::BRLS_KBD_KEY_BACKSPACE, "Backspace" },
    { brls::BRLS_KBD_KEY_DELETE,    "Del"       },
    { brls::BRLS_KBD_KEY_UP,        "Up"        },
    { brls::BRLS_KBD_KEY_DOWN,      "Down"      },
    { brls::BRLS_KBD_KEY_LEFT,      "Left"      },
    { brls::BRLS_KBD_KEY_RIGHT,     "Right"     },
    // 修饰键
    { brls::BRLS_KBD_KEY_LEFT_SHIFT,    "Shift"   },
    { brls::BRLS_KBD_KEY_RIGHT_SHIFT,   "Shift"   },
    { brls::BRLS_KBD_KEY_LEFT_CONTROL,  "Ctrl"    },
    { brls::BRLS_KBD_KEY_RIGHT_CONTROL, "Ctrl"    },
    { brls::BRLS_KBD_KEY_LEFT_ALT,      "Alt"     },
    { brls::BRLS_KBD_KEY_RIGHT_ALT,     "Alt"     },
};
static constexpr int k_capKbdKeyCount =
    static_cast<int>(sizeof(k_capKbdKeys) / sizeof(k_capKbdKeys[0]));

static std::string captureUtf8(char32_t codepoint)
{
    std::string result;
    if (codepoint <= 0x7f)
        result.push_back(static_cast<char>(codepoint));
    else if (codepoint <= 0x7ff)
    {
        result.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else
    {
        result.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    return result;
}

class KeyCaptureView : public beiklive::Box
{
public:
    explicit KeyCaptureView(std::function<void(const std::string &)> onDone, float countdownSecs = 3.0f)
        : m_onDone(std::move(onDone)), m_countdownSeconds(countdownSecs)
    {
        this->showFooter(false);
        this->showHeader(false);
        

        this->setFocusable(true);
        this->getContentBox()->setAxis(brls::Axis::COLUMN);
        this->getContentBox()->setAlignItems(brls::AlignItems::CENTER);
        this->getContentBox()->setJustifyContent(brls::JustifyContent::CENTER);
        this->getContentBox()->setGrow(1.0f);

        // ── 圆角卡片 ──
        auto* card = new brls::Box(brls::Axis::COLUMN);
        card->setFocusable(false);
        card->setCornerRadius(16.f);
        card->setBackgroundColor(nvgRGBA(30, 30, 35, 200));
        card->setShadowType(brls::ShadowType::GENERIC);
        card->setShadowVisibility(true);
        card->setAlignItems(brls::AlignItems::CENTER);
        card->setPadding(40.f, 60.f, 40.f, 60.f);
        card->setWidth(560.f);

        // 图标区
        auto* iconLabel = new brls::Label();
        iconLabel->setText("\uE041"); // gamepad icon (if font supports)
        iconLabel->setFontSize(36.f);
        iconLabel->setTextColor(nvgRGB(79, 193, 255));
        iconLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        iconLabel->setMarginBottom(12.f);
        iconLabel->setFocusable(false);
        card->addView(iconLabel);

        // 标题
        auto* titleLabel = new brls::Label();
        titleLabel->setText(L("按键捕获"));
        titleLabel->setFontSize(26.f);
        titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        titleLabel->setMarginBottom(6.f);
        titleLabel->setFocusable(false);
        card->addView(titleLabel);

        // 分隔线
        auto* div = new brls::Rectangle(nvgRGBA(79, 193, 255, 80));
        div->setWidth(80.f);
        div->setHeight(2.f);
        div->setMarginBottom(20.f);
        card->addView(div);

        // 提示文字
        m_promptLabel = new brls::Label();
        m_promptLabel->setText(L("按下要绑定的按键(支持组合键)"));
        m_promptLabel->setFontSize(17.f);
        m_promptLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        m_promptLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_promptLabel->setMarginBottom(16.f);
        m_promptLabel->setFocusable(false);
        card->addView(m_promptLabel);

        // 捕获的按键显示
        m_keyLabel = new brls::Label();
        m_keyLabel->setText("...");
        m_keyLabel->setFontSize(30.f);
        m_keyLabel->setTextColor(nvgRGBA(79, 193, 255,50));
        m_keyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_keyLabel->setMarginBottom(20.f);
        m_keyLabel->setFocusable(false);
        card->addView(m_keyLabel);

        // 进度条区域
        auto* barRow = new brls::Box(brls::Axis::ROW);
        barRow->setFocusable(false);
        barRow->setAlignItems(brls::AlignItems::CENTER);
        barRow->setJustifyContent(brls::JustifyContent::CENTER);
        barRow->setMarginBottom(8.f);

        m_progressBar = new brls::Rectangle(nvgRGBA(79, 193, 255, 50));
        m_progressBar->setWidth(240.f);
        m_progressBar->setHeight(6.f);
        m_progressBar->setCornerRadius(3.f);
        m_progressBar->setFocusable(false);
        barRow->addView(m_progressBar);

        card->addView(barRow);

        // 倒计时文字
        m_countdownLabel = new brls::Label();
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << m_countdownSeconds << L(" 秒");
        m_countdownLabel->setText(oss.str());
        m_countdownLabel->setFontSize(16.f);
        m_countdownLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        m_countdownLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_countdownLabel->setMarginBottom(14.f);
        m_countdownLabel->setFocusable(false);
        card->addView(m_countdownLabel);

        // 提示
        m_hintLabel = new brls::Label();
        m_hintLabel->setText(L("松开所有按键以开始捕获  |  最多 2 个按键"));
        m_hintLabel->setFontSize(14.f);
        m_hintLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        m_hintLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_hintLabel->setVisibility(brls::Visibility::INVISIBLE);
        m_hintLabel->setFocusable(false);
        card->addView(m_hintLabel);

        this->getContentBox()->addView(card);
        card->setVisibility(brls::Visibility::GONE);

        m_startTime = std::chrono::steady_clock::now();
        m_visualStartTime = m_startTime;
        m_lastVisualFrame = m_startTime;

        // 消费所有手柄导航键，防止触发父视图操作或提前关闭页面
        static const brls::ControllerButton k_swallowBtns[] = {
            brls::BUTTON_A, brls::BUTTON_B, brls::BUTTON_X, brls::BUTTON_Y,
            brls::BUTTON_LB, brls::BUTTON_RB, brls::BUTTON_LT, brls::BUTTON_RT,
            brls::BUTTON_LSB, brls::BUTTON_RSB,
            brls::BUTTON_UP, brls::BUTTON_DOWN, brls::BUTTON_LEFT, brls::BUTTON_RIGHT,
            brls::BUTTON_NAV_UP, brls::BUTTON_NAV_DOWN,
            brls::BUTTON_NAV_LEFT, brls::BUTTON_NAV_RIGHT,
            brls::BUTTON_START, brls::BUTTON_BACK,
        };
        for (auto btn : k_swallowBtns)
        {
            registerAction("", btn,
                           [this, btn](brls::View *) -> bool
                           {
                               if (!m_done && !m_waitingForRelease)
                                   captureGamepadButton(btn);
                               return true;
                           },
                           /*hidden=*/true);
        }

        // 注册键盘按键捕获
        for (int i = 0; i < k_capKbdKeyCount; ++i)
        {
            auto key = k_capKbdKeys[i].scancode;
            registerAction(brls::BrlsKeyCombination(key),
                           [this, key](brls::View *) -> bool
                           {
                               if (!m_done && !m_waitingForRelease)
                                   captureKeyboardKey(key);
                               return true;
                           },
                           /*allowRepeating=*/false);
        }
    }

    void draw(NVGcontext *vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext *ctx) override
    {
        // 半透明深色背景
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 180));
        nvgFill(vg);

        if (!m_done)
        {
            if (m_waitingForRelease)
            {
                checkAllReleased();
                m_startTime = std::chrono::steady_clock::now();
                m_visualPrompt = L("松开所有已按下的按键");
                m_visualHint = L("松开后开始捕获，最多可同时绑定 2 个按键");
                m_barProgress = 1.f;
                m_barColor = nvgRGB(255, 183, 77);
                m_promptLabel->setText(L("松开所有已按下的按键..."));
                m_promptLabel->setTextColor(nvgRGB(255, 183, 77));
                m_hintLabel->setVisibility(brls::Visibility::VISIBLE);
            }
            else
            {
                m_visualPrompt = L("按下要绑定的按键");
                m_visualHint = L("支持手柄、摇杆方向、键盘和双键组合");
                m_promptLabel->setText(L("按下要绑定的按键..."));
                m_promptLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
                m_hintLabel->setVisibility(brls::Visibility::GONE);

                // 轮询摇杆方向
                _pollSticks();

                auto now        = std::chrono::steady_clock::now();
                float elapsed   = std::chrono::duration<float>(now - m_startTime).count();
                float remaining = m_countdownSeconds - elapsed;

                if (remaining <= 0.0f)
                {
                    finish(m_captured);
                }
                else
                {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(2) << remaining << L(" 秒后自动确认");
                    m_countdownLabel->setText(oss.str());

                    // 更新进度条宽度
                    float barProgress = remaining / m_countdownSeconds;
                    m_barProgress = std::max(0.f, std::min(1.f, barProgress));
                    float barWidth = 240.f * barProgress;
                    m_progressBar->setWidth(barWidth);
                    if (barProgress < 0.3f)
                    {
                        m_barColor = nvgRGB(255, 82, 82);
                        m_progressBar->setColor(nvgRGB(255, 82, 82));  // 红色警告
                    }
                    else if (barProgress < 0.6f)
                    {
                        m_barColor = nvgRGB(255, 183, 77);
                        m_progressBar->setColor(nvgRGB(255, 183, 77)); // 橙色
                    }
                    else
                    {
                        m_barColor = nvgRGB(79, 193, 255);
                        m_progressBar->setColor(nvgRGB(79, 193, 255)); // 蓝色
                    }
                }
            }
        }
        brls::Box::draw(vg, x, y, w, h, style, ctx);
        _drawCaptureOverlay(vg, x, y, w, h);
        if (!m_done)
            invalidate();
    }

private:
    std::function<void(const std::string &)> m_onDone;
    float m_countdownSeconds = 3.0f;
    brls::Label *m_promptLabel    = nullptr;
    brls::Label *m_keyLabel       = nullptr;
    brls::Label *m_countdownLabel = nullptr;
    brls::Label *m_hintLabel      = nullptr;
    brls::Rectangle *m_progressBar = nullptr;
    std::chrono::steady_clock::time_point m_startTime;
    std::chrono::steady_clock::time_point m_visualStartTime;
    std::chrono::steady_clock::time_point m_lastVisualFrame;
    bool m_done              = false;
    bool m_waitingForRelease = true;
    std::vector<std::string> m_capturedKeys;
    std::string m_captured;
    std::string m_visualPrompt = L("松开所有已按下的按键");
    std::string m_visualHint = L("松开后开始捕获，最多可同时绑定 2 个按键");
    float m_barProgress = 1.f;
    float m_keyPulse = 0.f;
    NVGcolor m_barColor = nvgRGB(79, 193, 255);
    int m_captureFont = -1;
    int m_captureSwitchFont = -1;

    void captureGamepadButton(brls::ControllerButton btn)
    {
        const char *name = nullptr;
        for (int i = 0; i < k_capPadKeyCount; ++i)
            if (k_capPadKeys[i].btn == btn) { name = k_capPadKeys[i].name; break; }
        if (!name)
            return;

        if (std::find(m_capturedKeys.begin(), m_capturedKeys.end(), name) != m_capturedKeys.end())
            return;

        if (static_cast<int>(m_capturedKeys.size()) >= k_capMaxKeys)
            return;

        m_capturedKeys.push_back(name);
        m_captured = buildCombo(m_capturedKeys);
        m_keyPulse = 1.f;
        m_keyLabel->setText(m_captured);
        // 捕获到第一个按键后重置倒计时
        m_startTime = std::chrono::steady_clock::now();
    }

    void captureKeyboardKey(brls::BrlsKeyboardScancode key)
    {
        const char* name = nullptr;
        for (int i = 0; i < k_capKbdKeyCount; ++i)
            if (k_capKbdKeys[i].scancode == key) { name = k_capKbdKeys[i].name; break; }
        if (!name) return;

        if (std::find(m_capturedKeys.begin(), m_capturedKeys.end(), name) != m_capturedKeys.end())
            return;

        if (static_cast<int>(m_capturedKeys.size()) >= k_capMaxKeys)
            return;

        m_capturedKeys.push_back(name);
        m_captured = buildCombo(m_capturedKeys);
        m_keyPulse = 1.f;
        m_keyLabel->setText(m_captured);
        m_startTime = std::chrono::steady_clock::now();
    }

    // ── 摇杆捕获 ──
    struct StickDir {
        const char* name;
        int         axis;
        bool        positive;  // true=正方向, false=负方向
    };
    static const StickDir k_stickDirs[];
    static constexpr int  k_stickDirCount = 8;

    bool m_stickPrevActive[k_stickDirCount] = {};

    void _pollSticks()
    {
        auto state = brls::Application::getControllerState();
        for (int i = 0; i < k_stickDirCount; ++i)
        {
            float val = (k_stickDirs[i].axis < static_cast<int>(brls::_AXES_MAX))
                ? state.axes[k_stickDirs[i].axis] : 0.f;
            bool active = k_stickDirs[i].positive ? (val > 0.5f) : (val < -0.5f);
            if (active && !m_stickPrevActive[i])
                _captureStick(k_stickDirs[i].name);
            m_stickPrevActive[i] = active;
        }
    }

    void _captureStick(const char* name)
    {
        if (!name) return;
        if (std::find(m_capturedKeys.begin(), m_capturedKeys.end(), name) != m_capturedKeys.end())
            return;
        if (static_cast<int>(m_capturedKeys.size()) >= k_capMaxKeys)
            return;
        m_capturedKeys.push_back(name);
        m_captured = buildCombo(m_capturedKeys);
        m_keyPulse = 1.f;
        m_keyLabel->setText(m_captured);
        m_startTime = std::chrono::steady_clock::now();
    }

    void checkAllReleased()
    {
        // 检查手柄
        auto state = brls::Application::getControllerState();
        for (int i = 0; i < k_capPadKeyCount; ++i)
        {
            int idx = static_cast<int>(k_capPadKeys[i].btn);
            if (idx >= 0 && idx < static_cast<int>(brls::_BUTTON_MAX) && state.buttons[idx])
                return;
        }
        // 检查键盘
        auto* im = brls::Application::getPlatform()->getInputManager();
        if (im)
        {
            for (int i = 0; i < k_capKbdKeyCount; ++i)
            {
                if (im->getKeyboardKeyState(k_capKbdKeys[i].scancode))
                    return;
            }
        }
        m_waitingForRelease = false;
    }

    static std::string buildCombo(const std::vector<std::string> &keys)
    {
        std::string result;
        for (const auto &k : keys)
        {
            if (!result.empty())
                result += " + ";
            result += k;
        }
        return result;
    }

    struct CaptureVisual
    {
        std::string glyph;
        std::string suffix;
        bool switchGlyph = false;
    };

    CaptureVisual captureVisualFor(const std::string& token) const
    {
        for (int i = 0; i < k_capPadKeyCount; ++i)
        {
            if (token != k_capPadKeys[i].name)
                continue;
            if (token == "PAD_LT") return {captureUtf8(0xE0A6), "", true};
            if (token == "PAD_RT") return {captureUtf8(0xE0A7), "", true};
            return {brls::Hint::getKeyIcon(k_capPadKeys[i].btn), "", true};
        }
        if (token == "PAD_LEFTSTICKUP") return {captureUtf8(0xE0C1), "↑", true};
        if (token == "PAD_LEFTSTICKDOWN") return {captureUtf8(0xE0C1), "↓", true};
        if (token == "PAD_LEFTSTICKLEFT") return {captureUtf8(0xE0C1), "←", true};
        if (token == "PAD_LEFTSTICKRIGHT") return {captureUtf8(0xE0C1), "→", true};
        if (token == "PAD_RIGHTSTICKUP") return {captureUtf8(0xE0C2), "↑", true};
        if (token == "PAD_RIGHTSTICKDOWN") return {captureUtf8(0xE0C2), "↓", true};
        if (token == "PAD_RIGHTSTICKLEFT") return {captureUtf8(0xE0C2), "←", true};
        if (token == "PAD_RIGHTSTICKRIGHT") return {captureUtf8(0xE0C2), "→", true};
        return {token, "", false};
    }

    void drawCaptureChip(NVGcontext* vg, const CaptureVisual& visual,
                         float centerX, float centerY, float scale)
    {
        nvgFontFaceId(vg, visual.switchGlyph ? m_captureSwitchFont : m_captureFont);
        nvgFontSize(vg, (visual.switchGlyph ? 34.f : 22.f) * scale);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, visual.glyph.c_str(), nullptr, bounds);
        float suffixBounds[4]{};
        if (!visual.suffix.empty())
        {
            nvgFontFaceId(vg, m_captureFont);
            nvgFontSize(vg, 23.f * scale);
            nvgTextBounds(vg, 0.f, 0.f, visual.suffix.c_str(), nullptr, suffixBounds);
        }
        const float chipW = std::max(58.f, bounds[2] - bounds[0]
            + (visual.suffix.empty() ? 28.f : suffixBounds[2] - suffixBounds[0] + 38.f));
        const float chipH = 54.f;
        const float chipX = centerX - chipW * 0.5f;
        const float chipY = centerY - chipH * 0.5f;
        const NVGpaint shadow = nvgBoxGradient(vg, chipX + 4.f, chipY + 5.f,
            chipW, chipH, 27.f, 5.f, nvgRGBA(0, 0, 0, 78), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, chipX - 3.f, chipY - 3.f, chipW + 14.f, chipH + 15.f);
        nvgRoundedRect(vg, chipX, chipY, chipW, chipH, 27.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, chipX, chipY, chipW, chipH, 27.f);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 35));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, chipX + 1.f, chipY + 1.f, chipW - 2.f, chipH - 2.f, 26.f);
        nvgStrokeColor(vg, nvgRGBA(119, 211, 255, 150));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
        nvgFontFaceId(vg, visual.switchGlyph ? m_captureSwitchFont : m_captureFont);
        nvgFontSize(vg, (visual.switchGlyph ? 34.f : 22.f) * scale);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 255));
        const float glyphX = centerX - (visual.suffix.empty() ? 0.f : 9.f);
        nvgText(vg, glyphX, centerY, visual.glyph.c_str(), nullptr);
        if (!visual.suffix.empty())
        {
            nvgFontFaceId(vg, m_captureFont);
            nvgFontSize(vg, 23.f * scale);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgText(vg, glyphX + 15.f, centerY, visual.suffix.c_str(), nullptr);
        }
    }

    void _drawCaptureOverlay(NVGcontext* vg, float x, float y, float w, float h)
    {
        if (m_captureFont < 0)
            m_captureFont = brls::Application::getDefaultFont();
        if (m_captureSwitchFont < 0)
            m_captureSwitchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastVisualFrame).count();
        m_lastVisualFrame = now;
        if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
        m_keyPulse = std::max(0.f, m_keyPulse - dt * 4.8f);
        const float elapsed = std::chrono::duration<float>(now - m_visualStartTime).count();
        const float entranceRaw = std::max(0.f, std::min(1.f, elapsed * 5.2f));
        const float entrance = 1.f - std::pow(1.f - entranceRaw, 3.f);

        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, static_cast<unsigned char>(215.f * entranceRaw)));
        nvgFill(vg);

        const float panelW = std::min(680.f, w - 100.f);
        const float panelH = 408.f;
        const float panelX = x + (w - panelW) * 0.5f;
        const float panelY = y + (h - panelH) * 0.5f + (1.f - entrance) * 46.f;
        nvgSave(vg);
        nvgGlobalAlpha(vg, entranceRaw);
        nvgTranslate(vg, x + w * 0.5f, y + h * 0.5f);
        const float panelScale = 0.90f + entrance * 0.10f;
        nvgScale(vg, panelScale, panelScale);
        nvgTranslate(vg, -(x + w * 0.5f), -(y + h * 0.5f));

        const NVGpaint shadow = nvgBoxGradient(vg, panelX + 6.f, panelY + 8.f,
            panelW, panelH, 10.f, 7.f, nvgRGBA(0, 0, 0, 135), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, panelX - 4.f, panelY - 4.f, panelW + 20.f, panelH + 22.f);
        nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 10.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, panelX, panelY, panelW, panelH, 10.f);
        nvgFillColor(vg, getUiThemeMode() == UiThemeMode::Light
            ? nvgRGBA(248, 250, 252, 250) : nvgRGBA(24, 31, 43, 250));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, panelX + 1.f, panelY + 1.f, panelW - 2.f, panelH - 2.f, 9.f);
        nvgStrokeColor(vg, uiDivider(1.15f));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);

        nvgFontFaceId(vg, m_captureFont);
        nvgFontSize(vg, 27.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, uiTextPrimary());
        nvgText(vg, panelX + 34.f, panelY + 42.f, L("按键捕获").c_str(), nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, uiTextSecondary(0.72f));
        nvgText(vg, panelX + 34.f, panelY + 70.f, L("当前绑定会在倒计时结束后确认").c_str(), nullptr);
        nvgBeginPath(vg);
        nvgMoveTo(vg, panelX + 30.f, panelY + 92.f);
        nvgLineTo(vg, panelX + panelW - 30.f, panelY + 92.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 34));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        nvgFontSize(vg, 19.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, m_waitingForRelease
            ? nvgRGBA(180, 83, 9, 240) : uiTextPrimary(0.92f));
        nvgText(vg, panelX + panelW * 0.5f, panelY + 126.f, m_visualPrompt.c_str(), nullptr);

        const float chipY = panelY + 205.f;
        if (m_capturedKeys.empty())
        {
            nvgFontSize(vg, 42.f);
            nvgFillColor(vg, nvgRGBA(119, 211, 255, 65));
            nvgText(vg, panelX + panelW * 0.5f, chipY, "...", nullptr);
        }
        else
        {
            const float pulseScale = 1.f + 0.10f * std::sin(m_keyPulse * 3.1415926f);
            const float gap = 84.f;
            const float firstX = panelX + panelW * 0.5f
                - (static_cast<float>(m_capturedKeys.size()) - 1.f) * gap * 0.5f;
            for (size_t i = 0; i < m_capturedKeys.size(); ++i)
            {
                drawCaptureChip(vg, captureVisualFor(m_capturedKeys[i]),
                    firstX + static_cast<float>(i) * gap, chipY, pulseScale);
                if (i + 1 < m_capturedKeys.size())
                {
                    nvgFontFaceId(vg, m_captureFont);
                    nvgFontSize(vg, 23.f);
                    nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                    nvgFillColor(vg, uiTextSecondary(0.72f));
                    nvgText(vg, firstX + (static_cast<float>(i) + 0.5f) * gap,
                        chipY, "+", nullptr);
                }
            }
        }

        const float trackX = panelX + 74.f;
        const float trackY = panelY + 282.f;
        const float trackW = panelW - 148.f;
        nvgBeginPath(vg);
        nvgRoundedRect(vg, trackX, trackY, trackW, 7.f, 3.5f);
        nvgFillColor(vg, nvgRGBA(255, 255, 255, 24));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, trackX, trackY, trackW * m_barProgress, 7.f, 3.5f);
        nvgFillColor(vg, m_barColor);
        nvgFill(vg);
        nvgFontFaceId(vg, m_captureFont);
        nvgFontSize(vg, 15.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, uiTextSecondary(0.72f));
        nvgText(vg, panelX + panelW * 0.5f, panelY + 320.f, m_visualHint.c_str(), nullptr);
        nvgFontSize(vg, 14.f);
        nvgFillColor(vg, uiTextMuted(0.70f));
        nvgText(vg, panelX + panelW * 0.5f, panelY + 354.f,
            L("保持按键组合，松开后等待自动确认").c_str(), nullptr);
        nvgRestore(vg);
    }

    void finish(const std::string &result)
    {
        if (m_done)
            return;
        m_done = true;
        if (!result.empty())
        {
            m_keyLabel->setText(result);
            m_keyLabel->setTextColor(nvgRGB(79, 193, 255));
            m_countdownLabel->setText(L("已确认"));
            m_countdownLabel->setTextColor(nvgRGB(129, 199, 132));
            m_progressBar->setWidth(240.f);
            m_progressBar->setColor(nvgRGB(129, 199, 132));
        }
        if (m_onDone)
            m_onDone(result);
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
    }
};

const KeyCaptureView::StickDir KeyCaptureView::k_stickDirs[] = {
    {"PAD_LEFTSTICKUP",    static_cast<int>(brls::LEFT_Y),  false},
    {"PAD_LEFTSTICKDOWN",  static_cast<int>(brls::LEFT_Y),  true },
    {"PAD_LEFTSTICKLEFT",  static_cast<int>(brls::LEFT_X),  false},
    {"PAD_LEFTSTICKRIGHT", static_cast<int>(brls::LEFT_X),  true },
    {"PAD_RIGHTSTICKUP",   static_cast<int>(brls::RIGHT_Y), false},
    {"PAD_RIGHTSTICKDOWN", static_cast<int>(brls::RIGHT_Y), true },
    {"PAD_RIGHTSTICKLEFT", static_cast<int>(brls::RIGHT_X), false},
    {"PAD_RIGHTSTICKRIGHT",static_cast<int>(brls::RIGHT_X), true },
};

/// 推入全屏按键捕获页
static void openKeyCapture(std::function<void(const std::string &)> onDone)
{
    auto *content = new KeyCaptureView(std::move(onDone));
    auto *frame   = new brls::AppletFrame(content);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);
}

class SettingsFooterBar final : public brls::View
{
public:
    SettingsFooterBar()
    {
        setHeight(58.f);
        setFocusable(false);
    }

    void setContentMode(bool contentMode)
    {
        m_contentMode = contentMode;
        invalidate();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        (void)style;
        (void)ctx;
        if (m_font < 0)
            m_font = brls::Application::getDefaultFont();
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        float cursor = x + w - 32.f;
        _drawHint(vg, brls::BUTTON_B, m_contentMode ? L("返回分类").c_str() : L("返回").c_str(),
                  cursor, y + h * 0.5f);
        _drawHint(vg, brls::BUTTON_A, m_contentMode ? L("修改").c_str() : L("进入").c_str(),
                  cursor, y + h * 0.5f);
    }

private:
    int m_font = -1;
    int m_switchFont = -1;
    bool m_contentMode = false;

    void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                   const char* label, float& cursor, float y)
    {
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgFontFaceId(vg, m_font);
        nvgFontSize(vg, 18.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
        cursor -= bounds[2] - bounds[0] + 43.f;
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, uiIconPrimary(0.96f));
        nvgText(vg, cursor + 13.f, y, glyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_font);
        nvgFontSize(vg, 18.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, uiTextPrimary(0.88f));
        nvgText(vg, cursor + 30.f, y, label, nullptr);
        cursor -= 16.f;
    }
};

class SettingsCategoryBar final : public brls::View
{
public:
    SettingsCategoryBar(std::vector<std::string> labels,
                        std::function<void(int, int)> onSwitch,
                        std::function<void()> onEnter,
                        std::function<void()> onBack)
        : m_labels(std::move(labels))
        , m_onSwitch(std::move(onSwitch))
        , m_onEnter(std::move(onEnter))
        , m_onBack(std::move(onBack))
    {
        setHeight(104.f);
        setFocusable(true);
        HIDE_BRLS_HIGHLIGHT(this);
        setCustomNavigationRoute(brls::FocusDirection::UP, this);
        setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);
        auto previous = [this](brls::View*) -> bool { _switch(-1); return true; };
        auto next = [this](brls::View*) -> bool { _switch(1); return true; };
        registerAction("", brls::BUTTON_LEFT, previous, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_RIGHT, next, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, previous, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, next, true, true, brls::SOUND_NONE);
        registerAction(L("进入"), brls::BUTTON_A, [this](brls::View*) -> bool {
            if (m_onEnter) m_onEnter();
            return true;
        }, false, false, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_DOWN, [this](brls::View*) -> bool {
            if (m_onEnter) m_onEnter();
            return true;
        }, false, true, brls::SOUND_NONE);
        registerAction(L("返回"), brls::BUTTON_B, [this](brls::View*) -> bool {
            if (m_onBack) m_onBack();
            return true;
        }, false, false, brls::SOUND_NONE);
        m_lastFrame = std::chrono::steady_clock::now();
    }

    int selectedIndex() const { return m_selected; }
    void switchRelative(int direction) { _switch(direction); }

    void frame(brls::FrameContext* ctx) override
    {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrame).count();
        m_lastFrame = now;
        if (dt <= 0.f || dt > 0.25f) dt = 0.016f;
        m_time += dt;
        m_entrance = std::min(1.f, m_entrance + dt * 3.f);
        m_transition = std::min(1.f, m_transition + dt * 5.f);
        invalidate();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        (void)style;
        (void)ctx;
        if (m_font < 0)
            m_font = brls::Application::getDefaultFont();
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
        const float entrance = 1.f - std::pow(1.f - m_entrance, 3.f);
        nvgSave(vg);
        nvgGlobalAlpha(vg, std::max(0.f, std::min(1.f, m_entrance)));
        nvgTranslate(vg, 0.f, -(1.f - entrance) * 54.f);
        nvgFontFaceId(vg, m_font);
        nvgFontSize(vg, 27.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 36.f, y + 43.f, L("设置").c_str(), nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, nvgRGBA(210, 216, 226, 180));
        nvgText(vg, x + 36.f, y + 72.f, L("模拟器、画面、声音与输入配置").c_str(), nullptr);

        const float centerY = y + 45.f;
        const float startX = x + 250.f;
        const float availableW = std::max(120.f, w - 286.f);
        const int count = static_cast<int>(m_labels.size());
        const float segmentW = availableW / static_cast<float>(count);
        constexpr float selectorH = 42.f;
        const float eased = 1.f - std::pow(1.f - m_transition, 3.f);
        for (int index = 0; index < count; ++index)
        {
            const bool selected = index == m_selected;
            const float labelX = startX + segmentW * (static_cast<float>(index) + 0.5f);
            const float selectorW = std::min(132.f, segmentW - 8.f);
            const float prominence = selected ? (0.78f + eased * 0.22f) : 0.f;
            if (selected)
            {
                const float sx = labelX - selectorW * 0.5f;
                const float sy = centerY - selectorH * 0.5f;
                const NVGpaint shadow = nvgBoxGradient(
                    vg, sx + 3.f, sy + 3.f, selectorW, selectorH, 21.f, 5.f,
                    nvgRGBA(0, 0, 0, static_cast<unsigned char>(72.f * prominence)),
                    nvgRGBA(0, 0, 0, 0));
                nvgBeginPath(vg);
                nvgRect(vg, sx - 2.f, sy - 2.f, selectorW + 10.f, selectorH + 10.f);
                nvgRoundedRect(vg, sx, sy, selectorW, selectorH, 21.f);
                nvgPathWinding(vg, NVG_HOLE);
                nvgFillPaint(vg, shadow);
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, sx, sy, selectorW, selectorH, 21.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(22.f + 22.f * prominence)));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, sx + 1.f, sy + 1.f,
                               selectorW - 2.f, selectorH - 2.f, 20.f);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255,
                    static_cast<unsigned char>(70.f + 65.f * prominence)));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
            }
            nvgFontFaceId(vg, m_font);
            nvgFontSize(vg, selected ? 20.f : 17.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected
                ? nvgRGBA(255, 255, 255, 255)
                : nvgRGBA(205, 212, 223, 165));
            nvgText(vg, labelX, centerY,
                    m_labels[static_cast<size_t>(index)].c_str(), nullptr);
        }
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 36.f, y + 94.f);
        nvgLineTo(vg, x + w - 36.f, y + 94.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 46));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgRestore(vg);
    }

private:
    std::vector<std::string> m_labels;
    std::function<void(int, int)> m_onSwitch;
    std::function<void()> m_onEnter;
    std::function<void()> m_onBack;
    int m_selected = 0;
    int m_direction = 1;
    int m_font = -1;
    int m_switchFont = -1;
    float m_time = 0.f;
    float m_entrance = 0.f;
    float m_transition = 1.f;
    std::chrono::steady_clock::time_point m_lastFrame;

    void _switch(int direction)
    {
        if (m_transition < 0.72f || m_labels.size() <= 1) return;
        const int count = static_cast<int>(m_labels.size());
        m_selected = (m_selected + (direction < 0 ? -1 : 1) + count) % count;
        m_direction = direction < 0 ? -1 : 1;
        m_transition = 0.f;
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
        if (m_onSwitch) m_onSwitch(m_selected, m_direction);
    }
};

class ModernSettingFrame final : public brls::Box
{
public:
    ModernSettingFrame(std::vector<std::string> labels, std::function<void()> onBack)
        : brls::Box(brls::Axis::COLUMN)
        , m_onBack(std::move(onBack))
    {
        setGrow(1.f);
        setWidthPercentage(100.f);
        setFocusable(false);
        m_bar = new SettingsCategoryBar(
            std::move(labels),
            [this](int index, int direction) { _showTab(index, direction); },
            [this]() { _enterContent(); },
            [this]() { _close(); });
        addView(m_bar);
        m_content = new brls::Box(brls::Axis::COLUMN);
        m_content->setGrow(1.f);
        m_content->setWidthPercentage(100.f);
        m_content->setPadding(4.f, 26.f, 2.f, 26.f);
        m_content->setFocusable(false);
        addView(m_content);
        m_footer = new SettingsFooterBar();
        addView(m_footer);
    }

    void addTab(brls::View* view)
    {
        const int index = static_cast<int>(m_views.size());
        class ContentSurface final : public brls::Box
        {
        public:
            ContentSurface() : brls::Box(brls::Axis::COLUMN)
            {
                setGrow(1.f);
                setWidthPercentage(100.f);
                setPadding(8.f, 10.f, 8.f, 10.f);
                setBackground(brls::ViewBackground::NONE);
            }

            void draw(NVGcontext* vg, float x, float y, float w, float h,
                      brls::Style style, brls::FrameContext* ctx) override
            {
                const NVGpaint shadow = nvgBoxGradient(
                    vg, x + 5.f, y + 6.f, w, h, 8.f, 5.f,
                    nvgRGBA(0, 0, 0, 74), nvgRGBA(0, 0, 0, 0));
                nvgBeginPath(vg);
                nvgRect(vg, x - 3.f, y - 3.f, w + 16.f, h + 17.f);
                nvgRoundedRect(vg, x, y, w, h, 8.f);
                nvgPathWinding(vg, NVG_HOLE);
                nvgFillPaint(vg, shadow);
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, x, y, w, h, 8.f);
                nvgFillColor(vg, nvgRGBA(255, 255, 255, 7));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, x + 1.f, y + 1.f, w - 2.f, h - 2.f, 7.f);
                nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 42));
                nvgStrokeWidth(vg, 1.5f);
                nvgStroke(vg);
                brls::Box::draw(vg, x, y, w, h, style, ctx);
            }
        };

        auto* surface = new ContentSurface();
        view->setGrow(1.f);
        view->setWidthPercentage(100.f);
        surface->addView(view);
        surface->setVisibility(index == 0 ? brls::Visibility::VISIBLE
                                          : brls::Visibility::GONE);
        surface->registerAction(L("返回分类"), brls::BUTTON_B,
            [this](brls::View*) -> bool {
                _leaveContent();
                return true;
            }, false, false, brls::SOUND_BACK);
        m_views.push_back(surface);
        m_content->addView(surface);
    }

    void finish()
    {
        brls::sync([this]() { brls::Application::giveFocus(m_bar); });
    }

private:
    SettingsCategoryBar* m_bar = nullptr;
    SettingsFooterBar* m_footer = nullptr;
    brls::Box* m_content = nullptr;
    std::vector<brls::View*> m_views;
    std::function<void()> m_onBack;
    int m_selected = 0;
    bool m_contentMode = false;
    bool m_closing = false;

    void _showTab(int index, int direction)
    {
        if (index < 0 || index >= static_cast<int>(m_views.size()) || index == m_selected)
            return;
        m_views[static_cast<size_t>(m_selected)]->setVisibility(brls::Visibility::GONE);
        m_selected = index;
        auto* view = m_views[static_cast<size_t>(m_selected)];
        AnimationHelper::slideInFromRight(view, static_cast<float>(direction) * 82.f, 220);
        AnimationHelper::fadeIn(view, 190);
        if (m_contentMode)
        {
            brls::sync([view]() {
                brls::View* target = view->getDefaultFocus();
                brls::Application::giveFocus(target ? target : view);
            });
        }
    }

    void _enterContent()
    {
        if (m_closing || m_views.empty()) return;
        m_contentMode = true;
        m_footer->setContentMode(true);
        auto* view = m_views[static_cast<size_t>(m_selected)];
        brls::View* target = view->getDefaultFocus();
        brls::Application::giveFocus(target ? target : view);
    }

    void _leaveContent()
    {
        m_contentMode = false;
        m_footer->setContentMode(false);
        brls::Application::giveFocus(m_bar);
    }

    void _close()
    {
        if (m_closing) return;
        m_closing = true;
        brls::Application::blockInputs();
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
        AnimationHelper::fadeOut(this, 220, false, [this]() {
            brls::Application::unblockInputs();
            if (m_onBack) m_onBack();
        });
    }
};

namespace
{
enum class NanoSettingKind
{
    Section,
    Toggle,
    Selector,
    Action,
    Platform,
    Binding,
};

struct NanoSettingItem
{
    NanoSettingKind kind = NanoSettingKind::Action;
    std::string title;
    std::string hint;
    char32_t icon = beiklive::material::SETTINGS;
    std::function<std::string()> value;
    std::function<void()> activate;
    std::vector<std::string> options;
    std::function<int()> selectedOption;
    std::function<void(int)> applyOption;
    std::function<bool()> reset;
    std::string configKey;
    std::string defaultBinding = "none";
    std::string platformPrefix;
    bool nds = false;
};

struct NanoSettingsHost
{
    std::function<void(bool)> showShader;
    std::function<void(GradientTheme)> setGradientTheme;
    std::function<void()> applyUiTheme;
    std::function<void(bool)> showBackground;
    std::function<void(const std::string&)> setBackgroundImage;
    std::function<void()> close;
};

static std::string settingIconUtf8(char32_t codepoint)
{
    std::string result;
    if (codepoint <= 0x7f)
        result.push_back(static_cast<char>(codepoint));
    else if (codepoint <= 0x7ff)
    {
        result.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else if (codepoint <= 0xffff)
    {
        result.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else
    {
        result.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        result.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    return result;
}

static float settingClamp(float value)
{
    return std::max(0.f, std::min(1.f, value));
}

static float settingSmooth(float value)
{
    value = settingClamp(value);
    return value * value * (3.f - 2.f * value);
}

static float settingBack(float value)
{
    value = settingClamp(value);
    constexpr float c1 = 1.25f;
    constexpr float c3 = c1 + 1.f;
    const float shifted = value - 1.f;
    return 1.f + c3 * shifted * shifted * shifted + c1 * shifted * shifted;
}

static unsigned char settingAlpha(float value)
{
    return static_cast<unsigned char>(255.f * settingClamp(value));
}

static bool settingUsesLightSurface()
{
    return beiklive::getUiThemeMode() == beiklive::UiThemeMode::Light;
}

static NVGcolor settingPanelColor(float alpha = 0.96f)
{
    return settingUsesLightSurface()
        ? nvgRGBA(248, 250, 252, settingAlpha(alpha))
        : nvgRGBA(24, 31, 43, settingAlpha(alpha));
}

static NVGcolor settingPanelSubtle(float alpha = 0.08f)
{
    return settingUsesLightSurface()
        ? nvgRGBA(15, 23, 42, settingAlpha(alpha * 0.42f))
        : nvgRGBA(255, 255, 255, settingAlpha(alpha));
}

static NVGcolor settingBorder(float alpha = 0.24f)
{
    return settingUsesLightSurface()
        ? nvgRGBA(15, 23, 42, settingAlpha(alpha))
        : nvgRGBA(255, 255, 255, settingAlpha(alpha));
}

static NVGcolor settingText(float alpha = 1.f) { return beiklive::uiTextPrimary(alpha); }
static NVGcolor settingSecondary(float alpha = 1.f) { return beiklive::uiTextSecondary(alpha); }
static NVGcolor settingMuted(float alpha = 1.f) { return beiklive::uiTextMuted(alpha); }

static std::vector<std::string> splitSettingText(const std::string& text, char separator)
{
    std::vector<std::string> result;
    std::istringstream stream(text);
    std::string part;
    while (std::getline(stream, part, separator))
    {
        const auto first = std::find_if_not(part.begin(), part.end(), [](unsigned char c) { return std::isspace(c); });
        const auto last = std::find_if_not(part.rbegin(), part.rend(), [](unsigned char c) { return std::isspace(c); }).base();
        if (first < last)
            result.emplace_back(first, last);
    }
    return result;
}

class NanoSettingsCanvas final : public brls::View
{
public:
    explicit NanoSettingsCanvas(NanoSettingsHost host)
        : m_host(std::move(host))
    {
        setFocusable(true);
        setGrow(1.f);
        setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(this);
        setCustomNavigationRoute(brls::FocusDirection::UP, this);
        setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
        setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
        setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);
        _buildSettings();

        auto up = [this](brls::View*) -> bool { _move(-1); return true; };
        auto down = [this](brls::View*) -> bool { _move(1); return true; };
        auto left = [this](brls::View*) -> bool { _adjust(-1); return true; };
        auto right = [this](brls::View*) -> bool { _adjust(1); return true; };
        auto previousCategory = [this](brls::View*) -> bool { _switchCategory(-1); return true; };
        auto nextCategory = [this](brls::View*) -> bool { _switchCategory(1); return true; };
        registerAction("", brls::BUTTON_NAV_UP, up, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_DOWN, down, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_LEFT, left, true, true, brls::SOUND_NONE);
        registerAction("", brls::BUTTON_NAV_RIGHT, right, true, true, brls::SOUND_NONE);
        registerAction(L("上一类"), brls::BUTTON_LB, previousCategory, true, false, brls::SOUND_NONE);
        registerAction(L("下一类"), brls::BUTTON_RB, nextCategory, true, false, brls::SOUND_NONE);
        registerAction(L("选择"), brls::BUTTON_A, [this](brls::View*) -> bool {
            _activate();
            return true;
        }, false, false, brls::SOUND_NONE);
        registerAction(L("清除"), brls::BUTTON_X, [this](brls::View*) -> bool {
            _clearBinding();
            return true;
        }, false, false, brls::SOUND_NONE);
        registerAction(L("恢复默认"), brls::BUTTON_BACK, [this](brls::View*) -> bool {
            _resetCoreSetting();
            return true;
        }, false, false, brls::SOUND_NONE);
        registerAction(L("返回"), brls::BUTTON_B, [this](brls::View*) -> bool {
            _back();
            return true;
        }, false, false, brls::SOUND_NONE);
        m_lastFrame = std::chrono::steady_clock::now();
        brls::sync([this]() { brls::Application::giveFocus(this); });
    }

    ~NanoSettingsCanvas() override
    {
        m_aliveToken->store(false);
    }

    void frame(brls::FrameContext* ctx) override
    {
        brls::View::frame(ctx);
        const auto now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrame).count();
        m_lastFrame = now;
        if (dt <= 0.f || dt > 0.25f)
            dt = 0.016f;
        m_time += dt;
        if (m_closing)
        {
            m_pageEntrance = std::max(0.f, m_pageEntrance - dt * 4.6f);
            if (m_pageEntrance <= 0.f && !m_closeQueued)
            {
                m_closeQueued = true;
                const auto close = m_host.close;
                brls::sync([close]() { if (close) close(); });
            }
        }
        else
        {
            m_pageEntrance = std::min(1.f, m_pageEntrance + dt * 3.7f);
            m_contentEntrance = std::min(1.f, m_contentEntrance + dt * 6.5f);
        }
        m_categoryMotion = std::min(1.f, m_categoryMotion + dt * 7.f);
        m_overlayMotion += ((m_selectorOpen ? 1.f : 0.f) - m_overlayMotion)
            * std::min(1.f, dt * 14.f);
        m_steamOverlayMotion += ((m_steamDialog != SteamDialog::None ? 1.f : 0.f)
            - m_steamOverlayMotion) * std::min(1.f, dt * 14.f);
        m_pressMotion = std::max(0.f, m_pressMotion - dt * 5.6f);
        float& targetScroll = _activeTargetScroll();
        float& scroll = _activeScroll();
        scroll += (targetScroll - scroll) * std::min(1.f, dt * 15.f);
        invalidate();
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        (void)style;
        (void)ctx;
        _ensureFonts();
        const float page = settingBack(m_pageEntrance);
        const float alpha = settingSmooth(m_pageEntrance);
        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        _drawHeader(vg, x, y - (1.f - page) * 62.f, w);
        nvgSave(vg);
        const float scale = 0.965f + page * 0.035f;
        nvgTranslate(vg, x + w * 0.5f, y + h * 0.54f + (1.f - page) * 28.f);
        nvgScale(vg, scale, scale);
        nvgTranslate(vg, -(x + w * 0.5f), -(y + h * 0.54f));
        _drawContent(vg, x + 36.f, y + 116.f, w - 72.f, h - 184.f);
        nvgRestore(vg);
        _drawFooter(vg, x, y, w, h);
        nvgRestore(vg);
        if (m_overlayMotion > 0.002f)
            _drawSelector(vg, x, y, w, h);
        if (m_steamOverlayMotion > 0.002f)
            _drawSteamDialog(vg, x, y, w, h);
    }

private:
    enum class SteamDialog {
        None, ApiInfo, ApiModify, ConfirmCacheClear, Working, Result
    };
    struct Rect { float x = 0.f; float y = 0.f; float w = 0.f; float h = 0.f; };
    struct Category { std::string title; char32_t icon; std::vector<NanoSettingItem> items; };

    NanoSettingsHost m_host;
    std::array<Category, 6> m_categories{{
        {L("模拟器"), 0xE322, {}}, {L("按键"), 0xE30F, {}}, {L("游戏"), 0xE338, {}},
        {L("显示"), 0xE333, {}}, {L("声音"), 0xE050, {}}, {L("调试"), 0xE868, {}}
    }};
    std::vector<NanoSettingItem> m_mappingItems;
    std::vector<NanoSettingItem> m_coreItems;
    int m_category = 0;
    std::array<int, 6> m_focus{{0, 0, 0, 0, 0, 0}};
    std::array<float, 6> m_scroll{{0.f, 0.f, 0.f, 0.f, 0.f, 0.f}};
    std::array<float, 6> m_targetScroll{{0.f, 0.f, 0.f, 0.f, 0.f, 0.f}};
    int m_mappingFocus = 0;
    float m_mappingScroll = 0.f;
    float m_mappingTargetScroll = 0.f;
    bool m_inMapping = false;
    bool m_inCore = false;
    int m_coreFocus = 0;
    float m_coreScroll = 0.f;
    float m_coreTargetScroll = 0.f;
    std::string m_coreTitle;
    std::string m_mappingTitle;
    std::string m_mappingPrefix;
    bool m_mappingNds = false;
    bool m_selectorOpen = false;
    SteamDialog m_steamDialog = SteamDialog::None;
    int m_steamDialogChoice = 0;
    std::string m_steamDialogTitle;
    std::string m_steamDialogMessage;
    bool m_steamDialogSuccess = false;
    float m_steamOverlayMotion = 0.f;
    std::shared_ptr<std::atomic<bool>> m_aliveToken =
        std::make_shared<std::atomic<bool>>(true);
    std::string m_selectorTitle;
    std::vector<std::string> m_selectorOptions;
    int m_selectorIndex = 0;
    std::function<void(int)> m_selectorApply;
    int m_defaultFont = -1;
    int m_materialFont = -1;
    int m_switchFont = -1;
    float m_time = 0.f;
    float m_pageEntrance = 0.f;
    float m_contentEntrance = 1.f;
    float m_categoryMotion = 1.f;
    float m_overlayMotion = 0.f;
    float m_pressMotion = 0.f;
    int m_pressedItem = -1;
    bool m_pressedInMapping = false;
    int m_categoryDirection = 1;
    bool m_closing = false;
    bool m_closeQueued = false;
    std::chrono::steady_clock::time_point m_lastFrame;
    std::chrono::steady_clock::time_point m_lastMoveAction{};

    void _ensureFonts()
    {
        if (m_defaultFont < 0)
            m_defaultFont = brls::Application::getDefaultFont();
        if (m_materialFont < 0)
            m_materialFont = brls::Application::getFont(brls::FONT_MATERIAL_ICONS);
        if (m_switchFont < 0)
            m_switchFont = brls::Application::getFont(brls::FONT_SWITCH_ICONS);
    }

    static NanoSettingItem _section(const std::string& title)
    {
        NanoSettingItem item;
        item.kind = NanoSettingKind::Section;
        item.title = title;
        return item;
    }

    static NanoSettingItem _toggle(const std::string& title, const std::string& hint,
                                   char32_t icon, std::function<bool()> get,
                                   std::function<void(bool)> set,
                                   std::string configKey = {})
    {
        NanoSettingItem item;
        item.kind = NanoSettingKind::Toggle;
        item.title = title;
        item.hint = hint;
        item.icon = icon;
        // value 保持中文原值用于 _drawItem 的开关判断；显示时由渲染层 L() 翻译。
        item.value = [get]() { return get() ? L("开启") : L("关闭"); };
        item.activate = [get, set]() { set(!get()); };
        if (!configKey.empty())
            item.reset = [configKey = std::move(configKey)]() {
                return beiklive::SettingManager
                    && beiklive::SettingManager->ResetToDefault(configKey)
                    && beiklive::SettingManager->Save();
            };
        return item;
    }

    static NanoSettingItem _selector(const std::string& title, const std::string& hint,
                                     char32_t icon, std::vector<std::string> options,
                                     std::function<int()> get,
                                     std::function<void(int)> set,
                                     std::string configKey = {})
    {
        NanoSettingItem item;
        item.kind = NanoSettingKind::Selector;
        item.title = title;
        item.hint = hint;
        item.icon = icon;
        item.options = std::move(options);
        item.selectedOption = std::move(get);
        item.applyOption = std::move(set);
        item.value = [options = item.options, selected = item.selectedOption]() {
            const int index = selected ? selected() : 0;
            return index >= 0 && index < static_cast<int>(options.size())
                ? options[static_cast<size_t>(index)] : L("未设置");
        };
        if (!configKey.empty())
            item.reset = [configKey = std::move(configKey)]() {
                return beiklive::SettingManager
                    && beiklive::SettingManager->ResetToDefault(configKey)
                    && beiklive::SettingManager->Save();
            };
        return item;
    }

    static NanoSettingItem _action(const std::string& title, const std::string& hint,
                                   char32_t icon, std::function<std::string()> value,
                                   std::function<void()> action)
    {
        NanoSettingItem item;
        item.kind = NanoSettingKind::Action;
        item.title = title;
        item.hint = hint;
        item.icon = icon;
        item.value = std::move(value);
        item.activate = std::move(action);
        return item;
    }

    void _buildSettings()
    {
        using namespace beiklive::SettingKey;
        cfgSetBool("input.joystick.enabled", true);
        cfgSetBool("input.joystick.diagonal", true);
        auto& emulator = m_categories[0].items;
        emulator.push_back(_section(L("核心设置")));
        auto addCore = [this, &emulator](const std::string& title,
                                         const std::string& coreName,
                                         char32_t icon,
                                         std::function<void()> open) {
            emulator.push_back(_action(title, L("配置 ") + coreName + L(" 核心参数"), icon,
                [coreName]() { return coreName + "  >"; }, std::move(open)));
        };
        addCore(L("GBA 核心"), "mGBA", 0xE30F, [this]() { _openMgbaCore(); });
        addCore(L("NDS 核心（melonDS）"), "melonDS", 0xE322, [this]() { _openMelonDsCore(); });
        addCore(L("3DS 核心"), "Azahar", 0xE30F, [this]() { _openThreeDsCore(); });
        addCore(L("FC/NES 核心"), "Nestopia", 0xE333,
                [this]() { _openLibretroCore(L("Nestopia 核心设置"), CoreType::Nestopia); });
        LibretroLoader::discoverCoreOptions(CoreType::Fceumm, beiklive::SettingManager);
        if (!LibretroLoader::coreOptions(CoreType::Fceumm).empty()) {
            addCore(L("FC/NES 核心"), "FCEUmm", 0xE333,
                    [this]() { _openLibretroCore(L("FCEUmm 核心设置"), CoreType::Fceumm); });
        }
        addCore(L("SFC 核心"), "Snes9x", 0xE338,
                [this]() { _openLibretroCore(L("Snes9x 核心设置"), CoreType::Snes9x); });
        addCore(L("SFC 核心"), "Snes9x 2005", 0xE338,
                [this]() { _openLibretroCore(L("Snes9x 2005 核心设置"), CoreType::Snes9x2005); });
        addCore(L("GB/GBC 核心"), "GameBattle", 0xE30F,
                [this]() { _openLibretroCore(L("GameBattle 核心设置"), CoreType::Gambatte); });
        addCore(L("MD 核心"), "Genesis Plus GX", 0xE338,
                [this]() { _openGenesisCore(); });
        addCore(L("Arcade 核心"), "FBNeo", 0xE30F,
                [this]() { _openFbneoCore(); });
        addCore(L("Dreamcast 核心"), "Flycast", 0xE30F,
                [this]() { _openFlycastCore(); });
        addCore(L("PSP 核心"), "PPSSPP", 0xE30F,
                [this]() { _openPpssppCore(); });
        addCore(L("PS1 核心"), "DuckStation", 0xE30F,
                [this]() { _openDuckStationCore(); });
        addCore(L("Saturn 核心"), "YabaSanshiro", 0xE30F,
                [this]() { _openYabaSanshiroCore(); });
        addCore(L("GC / Wii 核心"), "Dolphin", 0xE30F,
                [this]() { _openDolphinCore(); });

        emulator.push_back(_section(L("存档与封面")));
        emulator.push_back(_selector(L("SRAM 存档目录"), L("选择 SRAM 与 ROM 同目录或模拟器统一目录"), beiklive::material::STORAGE,
            {L("ROM 所在目录"), L("模拟器目录")},
            []() { return cfgGetStr("save.sramDir", "").empty() ? 0 : 1; },
            [](int i) { cfgSetStr("save.sramDir", i == 0 ? "" : beiklive::path::savePath()); }));
        const std::vector<std::string> slots = {
            L("关闭"), L("档位0"), L("档位1"), L("档位2"), L("档位3"), L("档位4"), L("档位5"), L("档位6"), L("档位7"), L("档位8"), L("档位9")};
        emulator.push_back(_selector(L("自动保存游戏状态"), L("启动游戏后使用的自动存档档位"), beiklive::material::SAVE, slots,
            []() { return std::clamp(cfgGetInt("save.autoSaveState", 0), 0, 10); },
            [](int i) { cfgSetInt("save.autoSaveState", i); }));
        const std::vector<std::string> intervalLabels = {L("关闭"), L("1 分钟"), L("3 分钟"), L("5 分钟"), L("10 分钟")};
        const std::vector<int> intervalValues = {0, 60, 180, 300, 600};
        emulator.push_back(_selector(L("自动保存间隔"), L("定时创建即时存档，降低意外丢失进度的风险"), 0xE425, intervalLabels,
            [intervalValues]() { const int cur = cfgGetInt("save.autoSaveInterval", 0); for (int i = 0; i < 5; ++i) if (intervalValues[i] == cur) return i; return 0; },
            [intervalValues](int i) { if (i >= 0 && i < 5) cfgSetInt("save.autoSaveInterval", intervalValues[i]); }));
        emulator.push_back(_selector(L("启动时自动加载"), L("进入游戏时自动读取指定即时存档"), beiklive::material::RESTORE, slots,
            []() { return std::clamp(cfgGetInt("save.autoLoadState0", 0), 0, 10); },
            [](int i) { cfgSetInt("save.autoLoadState0", i); }));
        emulator.push_back(_selector(L("退出游戏时自动保存"), L("关闭游戏时写入指定即时存档"), beiklive::material::BACKUP, slots,
            []() { return std::clamp(cfgGetInt("save.autoSaveOnExit", 0), 0, 10); },
            [](int i) { cfgSetInt("save.autoSaveOnExit", i); }));
        emulator.push_back(_toggle(L("使用存档截图作为封面"), L("使用即时存档 0 的截图，且不会覆盖自定义封面"), beiklive::material::PHOTO_LIBRARY,
            []() { return cfgGetBool(KEY_UI_USE_SAVESTATE_THUMB, false); },
            [](bool v) { cfgSetBool(KEY_UI_USE_SAVESTATE_THUMB, v); }));

        emulator.push_back(_section(L("界面")));
        emulator.push_back(_selector(L("颜色主题"), L("控制原生控件及自绘界面的深色或浅色基调"), 0xE3B7,
            {L("深色"), L("浅色")},
            []() { return cfgGetStr(KEY_UI_THEME, "dark") == "light" ? 1 : 0; },
            [this](int i) {
                cfgSetStr(KEY_UI_THEME, i == 1 ? "light" : "dark");
                if (m_host.applyUiTheme) m_host.applyUiTheme();
                invalidate();
            }));
        emulator.push_back(_toggle(L("动态渐变背景"), L("显示与主页一致的动态背景"), 0xE3B7,
            []() { return cfgGetBool(KEY_UI_SHOW_SHADER, false); },
            [this](bool v) { cfgSetBool(KEY_UI_SHOW_SHADER, v); if (m_host.showShader) m_host.showShader(v); }));
        const std::vector<std::string> themeLabels = {L("深夜蓝"), L("柠檬黄"), L("牛油果绿"), L("草莓红"), L("海洋蓝"), L("樱花粉"), L("VSCode黑"), L("极光青"), L("皇家紫"), L("日落橙"), L("石墨灰"), L("云雾白")};
        const std::vector<std::string> themeValues = {"Midnight", "LemonYellow", "AvocadoGreen", "StrawberryRed", "OceanBlue", "SakuraPink", "VscodeBlack", "AuroraTeal", "RoyalPurple", "SunsetOrange", "Graphite", "CloudWhite"};
        emulator.push_back(_selector(L("渐变主题"), L("切换后立即应用到当前页面"), 0xE40A, themeLabels,
            [themeValues]() { return findIndex(themeValues, cfgGetStr(KEY_UI_GRADIENT_THEME, "VscodeBlack"), 6); },
            [this, themeValues](int i) {
                if (i < 0 || i >= static_cast<int>(themeValues.size())) return;
                cfgSetStr(KEY_UI_GRADIENT_THEME, themeValues[i]);
                if (!m_host.setGradientTheme) return;
                m_host.setGradientTheme(gradientThemeFromId(themeValues[i]));
            }));
        emulator.push_back(_selector(L("主页布局(重启后生效)"), L("选择首页的布局样式，重启应用后生效"), 0xE8A1,
            {L("Switch 布局"), L("IISU 布局")},
            []() {
                const int cur = cfgGetInt("theme", (int)beiklive::enums::ThemeLayout::SWITCH_THEME);
                if (cur == (int)beiklive::enums::ThemeLayout::IISU_THEME) return 1;
                return 0;
            },
            [](int i) {
                cfgSetInt("theme", i == 1
                    ? (int)beiklive::enums::ThemeLayout::IISU_THEME
                    : (int)beiklive::enums::ThemeLayout::SWITCH_THEME);
            }));
        emulator.push_back(_toggle(L("启用背景图片"), L("在动态背景上显示自定义 PNG、GIF图片 或 MP4视频"), beiklive::material::IMAGE,
            []() { return cfgGetBool(KEY_UI_SHOW_BG_IMAGE, false); },
            [this](bool v) { cfgSetBool(KEY_UI_SHOW_BG_IMAGE, v); if (m_host.showBackground) m_host.showBackground(v); }));
        emulator.push_back(_action(L("背景图片路径"), L("从文件浏览器选择 PNG、GIF图片 或 MP4视频(视频要求小于128MB，编码H264 最高720p 帧数不要高于60fps,30fps最流畅)"), beiklive::material::IMAGE,
            []() { const auto path = cfgGetStr(KEY_UI_BG_IMAGE_PATH, ""); return path.empty() ? L("未设置") : beiklive::tools::getFileName(path); },
            [this]() {
                const std::filesystem::path current(cfgGetStr(KEY_UI_BG_IMAGE_PATH, ""));
                beiklive::openFilePicker({"png", "gif", "mp4"}, [this](const std::string& path) {
                    cfgSetStr(KEY_UI_BG_IMAGE_PATH, path);
                    if (m_host.setBackgroundImage) m_host.setBackgroundImage(path);
                    invalidate();
                }, current.parent_path().string(), current.filename().string());
            }));
        emulator.push_back(_selector(L("GIF播放速度"), L("影响 GIF 背景播放速度；切换后立即生效"), 0xE8D5,
            {"0.5x", "0.75x", "1.0x", "1.25x", "1.5x", "2.0x"},
            []() {
                static constexpr float speeds[] = {0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f};
                const float current = GET_SETTING_KEY_FLOAT(KEY_UI_BG_GIF_SPEED, 1.f);
                int best = 2;
                float distance = std::fabs(current - speeds[best]);
                for (int i = 0; i < 6; ++i) {
                    const float candidate = std::fabs(current - speeds[i]);
                    if (candidate < distance) { best = i; distance = candidate; }
                }
                return best;
            },
            [](int index) {
                static constexpr float speeds[] = {0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f};
                if (index >= 0 && index < 6)
                    SET_SETTING_KEY_FLOAT(KEY_UI_BG_GIF_SPEED, speeds[index]);
            }));
        const std::vector<int> videoFrameRateValues = {30, 35, 40, 45, 50, 55, 60};
        emulator.push_back(_selector(L("MP4播放帧率"), L("限制 MP4 背景纹理更新频率，通过跳帧降低 CPU/GPU 占用；不改变播放速度"), 0xE8D5,
            {"30 FPS", "35 FPS", "40 FPS", "45 FPS", "50 FPS", "55 FPS", "60 FPS"},
            [videoFrameRateValues]() {
                const int current = cfgGetInt(KEY_UI_BG_VIDEO_FRAME_RATE, 30);
                int best = 0;
                int distance = std::abs(current - videoFrameRateValues[best]);
                for (int i = 1; i < static_cast<int>(videoFrameRateValues.size()); ++i) {
                    const int candidate = std::abs(current - videoFrameRateValues[i]);
                    if (candidate < distance) { best = i; distance = candidate; }
                }
                return best;
            },
            [videoFrameRateValues](int index) {
                if (index >= 0 && index < static_cast<int>(videoFrameRateValues.size()))
                    cfgSetInt(KEY_UI_BG_VIDEO_FRAME_RATE, videoFrameRateValues[index]);
            }));
        emulator.push_back(_toggle(L("文件列表滚动动画"), L("关闭后文件列表会直接跳转"), 0xE8D5,
            []() { return cfgGetBool(KEY_FILE_LIST_SCROLL_ANIM, true); },
            [](bool v) { cfgSetBool(KEY_FILE_LIST_SCROLL_ANIM, v); }));
        emulator.push_back(_selector(L("游戏库标题字号"), L("调整网格卡片标题的字号"), 0xE245,
            {L("正常"), "大", L("超大")},
            []() { return std::clamp(cfgGetInt(KEY_UI_LIBRARY_TITLE_SIZE, 0), 0, 2); },
            [](int i) { cfgSetInt(KEY_UI_LIBRARY_TITLE_SIZE, i); }));

        emulator.push_back(_section(L("语言 / Language")));
        emulator.push_back(_selector(L("语言 / Language"), L("重启后生效 / Takes effect after restart"), 0xE873,
            {L("简体中文"), "English", "日本語"},
            []() {
                std::string lang = cfgGetStr(KEY_UI_LANGUAGE, "zh-CN");
                if (lang == "en-US" || lang == "en") return 1;
                if (lang == "ja-JP" || lang == "ja") return 2;
                return 0;
            },
            [](int i) { cfgSetStr(KEY_UI_LANGUAGE, i == 1 ? "en-US" : i == 2 ? "ja-JP" : "zh-CN"); }));

        emulator.push_back(_section("SteamGridDB"));
        emulator.push_back(_action(
            "SteamGridDB API Key",
            L("用于在线搜索游戏封面；密钥保存在 GBAStation/SteamGirdDB/api"),
            0xE0DA,
            []() { return beiklive::steamgriddb::hasApiKey() ? L("已输入  >") : "空  >"; },
            [this]() { _openSteamApiDialog(); }));
        emulator.push_back(_action(
            L("清空 SteamGridDB 缓存"),
            L("清理本地 API 查询结果和已下载的预览图片"),
            0xE872,
            []() { return std::string(L("清理  >")); },
            [this]() { _beginSteamCacheClear(); }));

        auto& key = m_categories[1].items;
        key.push_back(_section(L("游戏平台")));
        struct Platform { std::string name; std::string prefix; std::string hint; bool nds; };
        static const Platform platforms[] = {
            {L("GBA 按键映射"), "", L("Game Boy Advance 游戏"), false},
            {L("GBC 按键映射"), "gbc.", L("Game Boy Color 游戏"), false},
            {L("GB 按键映射"), "gb.", L("Game Boy 游戏"), false},
            {L("FC/NES 按键映射"), "nes.", L("Nintendo Entertainment System 游戏"), false},
            {L("SFC 按键映射"), "sfc.", L("Super Famicom 游戏"), false},
            {L("NDS 按键映射"), "nds.", L("Nintendo DS 游戏与触摸指针热键"), true},
            {L("3DS 按键映射"), "3ds.", L("Nintendo 3DS 游戏与双摇杆控制"), false},
            {L("MD 按键映射"), "md.", L("Mega Drive 六键手柄与 Mode 键"), false},
            {L("Arcade 按键映射"), "arcade.", L("外部街机核心按键与热键"), false},
            {L("DC 按键映射"), "dc.", L("Dreamcast 外部核心按键与热键"), false},
            {L("PSP 按键映射"), "psp.", L("PPSSPP 外部核心按键与热键"), false},
        };
        for (const auto& platform : platforms)
        {
            NanoSettingItem item;
            item.kind = NanoSettingKind::Platform;
            item.title = platform.name;
            item.hint = platform.hint;
            item.icon = 0xE30F;
            item.value = []() { return std::string(L("进入配置")); };
            item.platformPrefix = platform.prefix;
            item.nds = platform.nds;
            key.push_back(std::move(item));
        }

        auto& game = m_categories[2].items;
        game.push_back(_section(L("快进")));
        game.push_back(_toggle(L("启用快进"), L("允许使用快进热键改变模拟速度"), 0xE01F,
            []() { return cfgGetBool("fastforward.enabled", true); }, [](bool v) { cfgSetBool("fastforward.enabled", v); }));
        game.push_back(_selector(L("快进触发模式"), L("按住：松开即停止；切换：再次按下才停止"), 0xE043,
            {L("按住"), L("切换")}, []() { return cfgGetStr("fastforward.mode", "hold") == "toggle" ? 1 : 0; },
            [](int i) { cfgSetStr("fastforward.mode", i == 1 ? "toggle" : "hold"); }));
        const std::vector<std::string> multiplierLabels = {L("0.1倍"), L("0.5倍"), L("1倍"), L("1.25倍"), L("1.5倍"), L("1.75倍"), L("2倍"), L("3倍"), L("4倍"), L("5倍"), L("6倍"), L("7倍"), L("8倍"), L("9倍"), L("10倍")};
        const std::vector<float> multiplierValues = {0.1f, 0.5f, 1.f, 1.25f, 1.5f, 1.75f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f, 9.f, 10.f};
        game.push_back(_selector(L("快进倍率"), L("低于 1 倍可用于慢动作"), 0xE8E5, multiplierLabels,
            [multiplierValues]() { const float cur = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 2.f); for (int i = 0; i < static_cast<int>(multiplierValues.size()); ++i) if (std::fabs(cur - multiplierValues[i]) < 0.001f) return i; return 6; },
            [multiplierValues](int i) { if (i >= 0 && i < static_cast<int>(multiplierValues.size())) SET_SETTING_KEY_FLOAT("fastforward.multiplier", multiplierValues[i]); }));
        game.push_back(_toggle(L("快进时静音"), L("避免高速播放产生刺耳音频"), 0xE04F,
            []() { return cfgGetBool("fastforward.mute", true); }, [](bool v) { cfgSetBool("fastforward.mute", v); }));
        game.push_back(_section(L("倒带")));
        game.push_back(_toggle(L("启用倒带"), L("运行时保存历史状态以便回退"), 0xE166,
            []() { return cfgGetBool("rewind.enabled", false); }, [](bool v) { cfgSetBool("rewind.enabled", v); }));
        game.push_back(_selector(L("倒带触发模式"), L("按住或切换倒带状态"), 0xE043,
            {L("按住"), L("切换")}, []() { return cfgGetStr("rewind.mode", "hold") == "toggle" ? 1 : 0; },
            [](int i) { cfgSetStr("rewind.mode", i == 1 ? "toggle" : "hold"); }));
        game.push_back(_toggle(L("倒带时静音"), L("回退期间停止播放反向音频"), 0xE04F,
            []() { return cfgGetBool("rewind.mute", false); }, [](bool v) { cfgSetBool("rewind.mute", v); }));
        const std::vector<int> stepValues = {1, 2, 4, 8};
        game.push_back(_selector(L("倒带步进"), L("步进越小，回退控制越精细"), 0xE5D5,
            {L("1 帧"), L("2 帧"), L("4 帧"), L("8 帧")},
            [stepValues]() { const int cur = cfgGetInt("rewind.step", 2); for (int i = 0; i < 4; ++i) if (stepValues[i] == cur) return i; return 1; },
            [stepValues](int i) { if (i >= 0 && i < 4) cfgSetInt("rewind.step", stepValues[i]); }));
        game.push_back(_toggle(L("可视化倒带界面"), L("显示历史缩略图时间轴"), beiklive::material::PHOTO_LIBRARY,
            []() { return cfgGetBool(KEY_REWIND_SHOW_UI, false); }, [](bool v) { cfgSetBool(KEY_REWIND_SHOW_UI, v); }));
        const std::vector<int> rewindIntervals = {1, 2, 4, 8, 16, 60, 120};
        game.push_back(_selector(L("状态保存间隔"), L("间隔越短越精确，但内存与 CPU 占用越高"), 0xE425,
            {L("每帧"), L("每2帧"), L("每4帧"), L("每8帧"), L("每16帧"), L("每60帧（约1秒）"), L("每120帧（约2秒）")},
            [rewindIntervals]() { const int cur = cfgGetInt(KEY_REWIND_SAVE_INTERVAL, 1); for (int i = 0; i < 7; ++i) if (rewindIntervals[i] == cur) return i; return 0; },
            [rewindIntervals](int i) { if (i >= 0 && i < 7) cfgSetInt(KEY_REWIND_SAVE_INTERVAL, rewindIntervals[i]); }));
        const std::vector<int> bufferValues = {60, 120, 600, 1800};
        game.push_back(_selector(L("最大倒带缓存"), L("缓存越大，可回退的时间越长"), 0xE1DB,
            {L("60（约1秒）"), L("120（约2秒）"), L("600（约10秒）"), L("1800（约30秒）")},
            [bufferValues]() { const int cur = cfgGetInt(KEY_REWIND_BUFFER_SIZE, 600); for (int i = 0; i < 4; ++i) if (bufferValues[i] == cur) return i; return 2; },
            [bufferValues](int i) { if (i >= 0 && i < 4) cfgSetInt(KEY_REWIND_BUFFER_SIZE, bufferValues[i]); }));
        game.push_back(_selector(L("缩略图压缩策略"), L("最近邻速度优先，双线性画面更平滑"), beiklive::material::IMAGE,
            {L("最近邻（速度优先）"), L("双线性（质量优先）")},
            []() { return std::clamp(cfgGetInt(KEY_REWIND_THUMB_COMPRESSION, 0), 0, 1); },
            [](int i) { cfgSetInt(KEY_REWIND_THUMB_COMPRESSION, i); }));

        _buildDisplaySettings();
        _buildAudioSettings();
        _buildDebugSettings();
        _normalizeFocus();
    }

    void _buildDisplaySettings()
    {
        using namespace beiklive::SettingKey;
        auto& display = m_categories[3].items;
        display.push_back(_section(L("画面")));
        const std::vector<std::string> modeLabels = {L("按比例（Fit）"), L("拉伸（Fill）"), L("原始（Original）"), "4:3", L("整数倍"), L("自定义")};
        const std::vector<std::string> modeValues = {"fit", "fill", "original", "four_three", "integer", "custom"};
        display.push_back(_selector(L("画面模式"), L("控制游戏画面的缩放与宽高比"), 0xE8A1, modeLabels,
            [modeValues]() { return findIndex(modeValues, cfgGetStr("display.mode", "original"), 2); },
            [modeValues](int i) { if (i >= 0 && i < 6) cfgSetStr("display.mode", modeValues[i]); }));
        const std::vector<int> scaleValues = {0, 1, 2, 3, 4, 5};
        display.push_back(_selector(L("整数倍缩放"), L("画面模式为整数倍时生效"), 0xE8FF,
            {L("自动"), L("1倍"), L("2倍"), L("3倍"), L("4倍"), L("5倍")},
            [scaleValues]() { const int cur = cfgGetInt("display.integer_scale_mult", 0); for (int i = 0; i < 6; ++i) if (scaleValues[i] == cur) return i; return 0; },
            [scaleValues](int i) { if (i >= 0 && i < 6) cfgSetInt("display.integer_scale_mult", scaleValues[i]); }));
        display.push_back(_selector(L("纹理过滤"), L("像素风格更锐利，平滑模式边缘更柔和"), 0xE3F4,
            {L("像素风格（Nearest）"), L("平滑（Linear）")},
            []() { return cfgGetStr("display.filter", "nearest") == "linear" ? 1 : 0; },
            [](int i) { cfgSetStr("display.filter", i == 1 ? "linear" : "nearest"); }));
        display.push_back(_toggle(L("显示快进覆盖层"), L("快进时显示状态提示"), 0xE01F,
            []() { return cfgGetBool("display.showFfOverlay", true); }, [](bool v) { cfgSetBool("display.showFfOverlay", v); }));
        display.push_back(_toggle(L("显示倒带覆盖层"), L("倒带时显示状态提示"), 0xE166,
            []() { return cfgGetBool("display.showRewindOverlay", true); }, [](bool v) { cfgSetBool("display.showRewindOverlay", v); }));
        display.push_back(_toggle(L("显示静音覆盖层"), L("静音时显示状态提示"), 0xE04F,
            []() { return cfgGetBool("display.showMuteOverlay", true); }, [](bool v) { cfgSetBool("display.showMuteOverlay", v); }));
        display.push_back(_toggle(L("显示 FPS 覆盖层"), L("在游戏画面上显示实时帧率"), 0xE8E5,
            []() { return cfgGetBool("display.showFps", false); }, [](bool v) { cfgSetBool("display.showFps", v); }));

        const std::array<std::pair<const char*, const char*>, 9> platforms{{
            {"GBA", KEY_DISPLAY_OVERLAY_GBA_PATH}, {"GBC", KEY_DISPLAY_OVERLAY_GBC_PATH},
            {"GB", KEY_DISPLAY_OVERLAY_GB_PATH}, {"FC", KEY_DISPLAY_OVERLAY_NES_PATH},
            {"SFC", KEY_DISPLAY_OVERLAY_SNES_PATH}, {"MD", KEY_DISPLAY_OVERLAY_GENESIS_PATH},
            {"Arcade", KEY_DISPLAY_OVERLAY_ARCADE_PATH}, {"DC", KEY_DISPLAY_OVERLAY_DC_PATH},
            {"PSP", KEY_DISPLAY_OVERLAY_PSP_PATH}}};
        display.push_back(_section(L("默认遮罩")));
        for (const auto& platform : platforms)
        {
            const std::string key = platform.second;
            display.push_back(_action(std::string(platform.first) + L(" 遮罩"), L("导入新游戏时自动套用"), beiklive::material::IMAGE,
                [key]() { const auto path = cfgGetStr(key, ""); return path.empty() ? L("未设置") : beiklive::tools::getFileName(path); },
                [this, key]() { _pickFile(key, {"png"}); }));
        }
        const std::array<std::pair<const char*, const char*>, 9> shaders{{
            {"GBA", KEY_DISPLAY_SHADER_GBA_PATH}, {"GBC", KEY_DISPLAY_SHADER_GBC_PATH},
            {"GB", KEY_DISPLAY_SHADER_GB_PATH}, {"FC", KEY_DISPLAY_SHADER_NES_PATH},
            {"SFC", KEY_DISPLAY_SHADER_SNES_PATH}, {"MD", KEY_DISPLAY_SHADER_GENESIS_PATH},
            {"Arcade", KEY_DISPLAY_SHADER_ARCADE_PATH}, {"DC", KEY_DISPLAY_SHADER_DC_PATH},
            {"PSP", KEY_DISPLAY_SHADER_PSP_PATH}}};
        display.push_back(_section(L("默认着色器")));
        for (const auto& platform : shaders)
        {
            const std::string key = platform.second;
            display.push_back(_action(std::string(platform.first) + L(" 着色器"), L("导入新游戏时自动套用"), 0xE3B7,
                [key]() { const auto path = cfgGetStr(key, ""); return path.empty() ? L("未设置") : beiklive::tools::getFileName(path); },
                [this, key]() { _pickFile(key, {"glslp", "glsl"}); }));
        }
    }

    void _buildAudioSettings()
    {
        using namespace beiklive::SettingKey;
        auto& audio = m_categories[4].items;
        audio.push_back(_section(L("音频输出")));
        const std::vector<int> masterVolumeValues = {0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
        std::vector<std::string> masterVolumeLabels;
        for (int v : masterVolumeValues) {
            masterVolumeLabels.push_back(v == 0 ? L("静音") : std::to_string(v) + "%");
        }
        audio.push_back(_selector(L("主音量"), L("全局音量，同时影响游戏音频与界面音效"), 0xE425,
            masterVolumeLabels,
            [masterVolumeValues]() { const int cur = cfgGetInt(KEY_AUDIO_MASTER_VOLUME, 100); for (int i = 0; i < static_cast<int>(masterVolumeValues.size()); ++i) if (masterVolumeValues[i] == cur) return i; return 10; },
            [masterVolumeValues](int i) {
                if (i < 0 || i >= static_cast<int>(masterVolumeValues.size())) return;
                const int v = masterVolumeValues[i];
                cfgSetInt(KEY_AUDIO_MASTER_VOLUME, v);
                beiklive::AudioManager::instance().setMasterVolume(
                    static_cast<float>(v) / 100.0f);
            }, KEY_AUDIO_MASTER_VOLUME));
        audio.push_back(_toggle(L("按钮音效"), L("播放界面导航和确认音效"), 0xE050,
            []() { return cfgGetBool("audio.buttonSfx", true); }, [](bool v) { cfgSetBool("audio.buttonSfx", v); }));
        const std::vector<int> volumeValues = {0, 25, 50, 75, 100};
        const std::vector<std::string> volumeLabels = {L("静音"), "25%", "50%", "75%", "100%"};
        audio.push_back(_selector(L("按键音效音量"), L("调整界面导航和确认音效的音量"), 0xE050,
            volumeLabels,
            [volumeValues]() { const int cur = cfgGetInt(KEY_AUDIO_BUTTON_SFX_VOLUME, 100); for (int i = 0; i < static_cast<int>(volumeValues.size()); ++i) if (volumeValues[i] == cur) return i; return 4; },
            [volumeValues](int i) { if (i >= 0 && i < static_cast<int>(volumeValues.size())) cfgSetInt(KEY_AUDIO_BUTTON_SFX_VOLUME, volumeValues[i]); }));
        const std::vector<int> targetValues = {60, 90, 120, 160};
        audio.push_back(_selector(L("目标缓冲延迟"), L("越低反馈越快，越高越不容易断音"), 0xE425,
            {"60 ms", "90 ms", "120 ms", "160 ms"},
            [targetValues]() { const int cur = cfgGetInt(KEY_AUDIO_TARGET_LATENCY_MS, 90); for (int i = 0; i < 4; ++i) if (targetValues[i] == cur) return i; return 1; },
            [targetValues](int i) { if (i >= 0 && i < 4) cfgSetInt(KEY_AUDIO_TARGET_LATENCY_MS, targetValues[i]); }));
        const std::vector<int> maxValues = {120, 180, 240, 320};
        audio.push_back(_selector(L("最大缓冲延迟"), L("超过该延迟会丢弃旧音频，避免声音落后画面"), 0xE8B5,
            {"120 ms", "180 ms", "240 ms", "320 ms"},
            [maxValues]() { const int cur = cfgGetInt(KEY_AUDIO_MAX_LATENCY_MS, 180); for (int i = 0; i < 4; ++i) if (maxValues[i] == cur) return i; return 1; },
            [maxValues](int i) { if (i >= 0 && i < 4) cfgSetInt(KEY_AUDIO_MAX_LATENCY_MS, maxValues[i]); }));
        const std::vector<float> syncValues = {0.f, 0.008f, 0.015f, 0.025f};
        audio.push_back(_selector(L("音画同步修正"), L("根据缓冲量微调模拟节奏，减少爆音和长期漂移"), 0xE8D5,
            {L("关闭"), L("柔和"), L("标准"), "强"},
            [syncValues]() { const float cur = GET_SETTING_KEY_FLOAT(KEY_AUDIO_SYNC_STRENGTH, 0.015f); int best = 0; float delta = std::fabs(cur - syncValues[0]); for (int i = 1; i < 4; ++i) { const float d = std::fabs(cur - syncValues[i]); if (d < delta) { best = i; delta = d; } } return best; },
            [syncValues](int i) { if (i >= 0 && i < 4) SET_SETTING_KEY_FLOAT(KEY_AUDIO_SYNC_STRENGTH, syncValues[i]); }));
        const std::vector<int> fadeValues = {0, 4, 6, 10};
        audio.push_back(_selector(L("切换淡入淡出"), L("暂停、静音和读档时降低咔哒声"), 0xE8D5,
            {L("关闭"), "4 ms", "6 ms", "10 ms"},
            [fadeValues]() { const int cur = cfgGetInt(KEY_AUDIO_TRANSITION_FADE_MS, 6); for (int i = 0; i < 4; ++i) if (fadeValues[i] == cur) return i; return 2; },
            [fadeValues](int i) { if (i >= 0 && i < 4) cfgSetInt(KEY_AUDIO_TRANSITION_FADE_MS, fadeValues[i]); }));
    }

    void _buildDebugSettings()
    {
        using namespace beiklive::SettingKey;
        auto& debug = m_categories[5].items;
        debug.push_back(_section(L("日志")));
        const std::vector<std::string> logLabels = {L("调试（debug）"), L("信息（info）"), L("警告（warning）"), L("错误（error）")};
        const std::vector<std::string> logValues = {"debug", "info", "warning", "error"};
        debug.push_back(_selector(L("日志级别"), L("控制写入日志的最低消息等级"), beiklive::material::DESCRIPTION, logLabels,
            [logValues]() { return findIndex(logValues, cfgGetStr(KEY_DEBUG_LOG_LEVEL, "info"), 1); },
            [logValues](int i) {
                if (i < 0 || i >= 4) return;
                cfgSetStr(KEY_DEBUG_LOG_LEVEL, logValues[i]);
                static const brls::LogLevel levels[] = {brls::LogLevel::LOG_DEBUG, brls::LogLevel::LOG_INFO, brls::LogLevel::LOG_WARNING, brls::LogLevel::LOG_ERROR};
                brls::Logger::setLogLevel(levels[i]);
            }));
        debug.push_back(_toggle(L("输出日志到文件"), L("将运行日志追加写入日志文件"), beiklive::material::SAVE,
            []() { return cfgGetBool(KEY_DEBUG_LOG_FILE, false); },
            [](bool v) {
                cfgSetBool(KEY_DEBUG_LOG_FILE, v);
                static FILE* logFile = nullptr;
                if (v)
                {
                    if (logFile) { std::fclose(logFile); logFile = nullptr; }
                    logFile = std::fopen(beiklive::path::logFilePath().c_str(), "a");
                    if (logFile) brls::Logger::setLogOutput(logFile);
                }
                else
                {
                    brls::Logger::setLogOutput(nullptr);
                    if (logFile) { std::fclose(logFile); logFile = nullptr; }
                }
            }));
        debug.push_back(_toggle(L("调试信息覆盖层"), L("在屏幕上叠加帧率和帧时间等性能数据"), 0xE868,
            []() { return cfgGetBool(KEY_DEBUG_LOG_OVERLAY, false); },
            [](bool v) { cfgSetBool(KEY_DEBUG_LOG_OVERLAY, v); brls::Application::enableDebuggingView(v); }));
    }

    void _pickFile(const std::string& key, const std::vector<std::string>& extensions)
    {
        const std::filesystem::path current(cfgGetStr(key, ""));
        beiklive::openFilePicker(extensions, [this, key](const std::string& path) {
            cfgSetStr(key, path);
            invalidate();
        }, current.parent_path().string(), current.filename().string());
    }

    static std::string _coreCategoryTitle(const std::string& category)
    {
        if (category == "system") return L("系统");
        if (category == "video") return L("视频");
        if (category == "audio") return L("音频");
        if (category == "input") return L("输入");
        if (category == "hacks") return L("性能与兼容性");
        if (category == "lightgun") return L("光枪");
        if (category == "mapping") return L("按键与映射");
        if (category == "advanced_av") return L("高级音视频");
        return L("其他");
    }

    static std::string _coreOptionTitle(const std::string& key,
                                        const std::string& fallback)
    {
        static const std::pair<const char*, const char*> titles[] = {
            {"snes9x_region", "主机地区（需重新载入核心）"},
            {"snes9x_aspect", "首选画面比例"},
            {"snes9x_overscan", "裁剪过扫描区域"},
            {"snes9x_hires_blend", "高分辨率画面混合"},
            {"snes9x_blargg", "Blargg NTSC 滤镜"},
            {"snes9x_audio_interpolation", "音频插值"},
            {"snes9x_up_down_allowed", "允许相反方向同时输入"},
            {"snes9x_overclock_superfx", "SuperFX 超频"},
            {"snes9x_overclock_cycles", "减少游戏掉速（实验性）"},
            {"snes9x_reduce_sprite_flicker", "减少精灵闪烁（实验性）"},
            {"snes9x_randomize_memory", "启动时随机化内存（实验性）"},
            {"snes9x_block_invalid_vram_access", "阻止无效显存访问"},
            {"snes9x_echo_buffer_hack", "回声缓冲兼容修正"},
            {"snes9x_show_lightgun_settings", "显示光枪设置"},
            {"snes9x_lightgun_mode", "光枪输入方式"},
            {"snes9x_superscope_reverse_buttons", "交换 Super Scope 扳机按键"},
            {"snes9x_superscope_crosshair", "Super Scope 准星大小"},
            {"snes9x_superscope_color", "Super Scope 准星颜色"},
            {"snes9x_justifier1_crosshair", "Justifier 1 准星大小"},
            {"snes9x_justifier1_color", "Justifier 1 准星颜色"},
            {"snes9x_justifier2_crosshair", "Justifier 2 准星大小"},
            {"snes9x_justifier2_color", "Justifier 2 准星颜色"},
            {"snes9x_rifle_crosshair", "M.A.C.S. 光枪准星大小"},
            {"snes9x_rifle_color", "M.A.C.S. 光枪准星颜色"},
            {"snes9x_show_advanced_av_settings", "显示高级音视频设置"},
            {"snes9x_gfx_clip", "启用图形裁剪窗口"},
            {"snes9x_gfx_transp", "启用透明效果"},
            {"snes9x_2005_region", "主机地区（需重新启动）"},
            {"snes9x_2005_frameskip", "跳帧"},
            {"snes9x_2005_frameskip_threshold", "跳帧阈值"},
            {"snes9x_2005_low_pass_filter", "低通音频滤波"},
            {"snes9x_2005_low_pass_range", "低通滤波强度"},
            {"snes9x_2005_overclock_cycles", "减少游戏掉速（实验性）"},
            {"snes9x_2005_reduce_sprite_flicker", "减少精灵闪烁（实验性）"},
        };
        for (const auto& item : titles)
            if (key == item.first) return item.second;
        if (key.rfind("snes9x_layer_", 0) == 0) {
            const std::string layer = key.substr(std::string("snes9x_layer_").size());
            return layer == "5" ? L("显示精灵层") : L("显示背景层 ") + layer;
        }
        if (key.rfind("snes9x_sndchan_volume_", 0) == 0)
            return L("声道 ") + key.substr(std::string("snes9x_sndchan_volume_").size()) + L(" 音量");
        return fallback.empty() ? key : fallback;
    }

    static std::string _coreOptionDescription(const std::string& key,
                                              const std::string& fallback)
    {
        static const std::pair<const char*, const char*> descriptions[] = {
            {"snes9x_region", "指定主机使用 NTSC 或 PAL 制式；选择错误会影响游戏速度。"},
            {"snes9x_aspect", "选择核心建议的显示比例。"},
            {"snes9x_overscan", "裁掉电视通常不会显示的画面顶部和底部边缘。"},
            {"snes9x_hires_blend", "在 512 像素高分辨率模式下混合相邻像素。"},
            {"snes9x_blargg", "模拟不同类型的 NTSC 电视信号。"},
            {"snes9x_audio_interpolation", "选择声音重采样方式；高斯插值最接近原机音色。"},
            {"snes9x_up_down_allowed", "允许左右或上下方向同时按下，部分游戏可能出现异常。"},
            {"snes9x_overclock_superfx", "调整 SuperFX 协处理器频率，过高可能造成时序错误。"},
            {"snes9x_overclock_cycles", "提高主 CPU 运行速度以减少掉速，可能降低兼容性。"},
            {"snes9x_reduce_sprite_flicker", "提高同屏精灵数量上限，可能导致显示异常。"},
            {"snes9x_randomize_memory", "启动时随机填充系统内存，适用于依赖未初始化内存的游戏。"},
            {"snes9x_block_invalid_vram_access", "部分自制游戏或 ROM 修改版需要关闭此项。"},
            {"snes9x_echo_buffer_hack", "仅为使用旧版 Addmusic 的 ROM 修改版开启。"},
            {"snes9x_show_lightgun_settings", "显示 Super Scope、Justifier 等光枪配置。"},
            {"snes9x_lightgun_mode", "选择使用光枪指针或触摸屏输入。"},
            {"snes9x_superscope_reverse_buttons", "交换 Super Scope 的射击与光标按键。"},
            {"snes9x_show_advanced_av_settings", "显示图层、透明效果和独立声道音量等设置。"},
            {"snes9x_2005_region", "指定主机使用 NTSC 或 PAL 制式；选择错误会影响游戏速度。"},
            {"snes9x_2005_frameskip", "性能不足时跳过部分画面，减少声音卡顿。"},
            {"snes9x_2005_frameskip_threshold", "手动跳帧时，音频缓冲低于此比例便跳过画面。"},
            {"snes9x_2005_low_pass_filter", "削弱高频杂音并模拟原机偏厚的音色。"},
            {"snes9x_2005_low_pass_range", "数值越高，低通滤波效果越明显。"},
            {"snes9x_2005_overclock_cycles", "提高主 CPU 运行速度以减少掉速，可能降低兼容性。"},
            {"snes9x_2005_reduce_sprite_flicker", "提高同屏精灵数量上限以减少闪烁。"},
        };
        for (const auto& item : descriptions)
            if (key == item.first) return item.second;
        if (key.find("crosshair") != std::string::npos) return L("设置屏幕准星大小。");
        if (key.find("_color") != std::string::npos) return L("设置屏幕准星颜色。");
        if (key.rfind("snes9x_layer_", 0) == 0) return L("控制该画面图层是否显示。");
        if (key.rfind("snes9x_sndchan_volume_", 0) == 0) return L("调整该声音通道的输出音量。");
        return fallback;
    }

    static std::string _coreOptionValue(const std::string& raw,
                                        const std::string& sourceLabel)
    {
        const std::string source = sourceLabel.empty() ? raw : sourceLabel;
        if (std::any_of(source.begin(), source.end(), [](unsigned char c) { return c >= 0x80; }))
            return source;
        std::string lower = source;
        std::transform(lower.begin(), lower.end(), lower.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        static const std::pair<const char*, const char*> values[] = {
            {"enabled", "开启"}, {"disabled", "关闭"}, {"on", "开启"}, {"off", "关闭"},
            {"yes", "是"}, {"no", "否"}, {"true", "开启"}, {"false", "关闭"},
            {"auto", "自动"}, {"manual", "手动"}, {"default", "默认"}, {"none", "无"},
            {"light", "轻度"}, {"compatible", "兼容"}, {"max", "最大"},
            {"uncorrected", "未校正"}, {"4:3 (preserved)", "4:3（保持比例）"},
            {"merge", "合并"}, {"blur", "模糊"}, {"monochrome", "黑白"},
            {"rf", "射频"}, {"composite", "复合视频"}, {"gaussian", "高斯"},
            {"cubic", "三次插值"}, {"linear", "线性"}, {"light gun", "光枪"},
            {"lightgun", "光枪"}, {"touchscreen", "触摸屏"},
            {"white", "白色"}, {"red", "红色"}, {"orange", "橙色"},
            {"yellow", "黄色"}, {"green", "绿色"}, {"cyan", "青色"},
            {"sky", "天蓝色"}, {"blue", "蓝色"}, {"violet", "紫罗兰色"},
            {"pink", "粉色"}, {"purple", "紫色"}, {"black", "黑色"},
            {"25% grey", "25% 灰色"}, {"50% grey", "50% 灰色"}, {"75% grey", "75% 灰色"},
        };
        for (const auto& item : values)
            if (lower == item.first) return item.second;
        const std::string blend = " (blend)";
        if (lower.size() > blend.size() && lower.compare(lower.size() - blend.size(), blend.size(), blend) == 0)
            return _coreOptionValue(raw, source.substr(0, source.size() - blend.size())) + L("（混合）");
        return source;
    }

    void _appendRawOption(const LibretroLoader::CoreOptionDefinition& option)
    {
        if (option.values.empty()) return;
        const std::string key = "core." + option.key;
        const std::string fallback = option.defaultValue;
        const std::string title = _coreOptionTitle(option.key, option.title);
        const std::string description = _coreOptionDescription(option.key, option.description);
        auto lower = [](std::string value) {
            std::transform(value.begin(), value.end(), value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return value;
        };
        int disabledIndex = -1;
        int enabledIndex = -1;
        for (size_t i = 0; i < option.values.size(); ++i) {
            const std::string value = lower(option.values[i].value);
            if (value == "disabled" || value == "off" || value == "no" || value == "false")
                disabledIndex = static_cast<int>(i);
            else if (value == "enabled" || value == "on" || value == "yes" || value == "true")
                enabledIndex = static_cast<int>(i);
        }
        const bool toggle = option.values.size() == 2 && disabledIndex >= 0 && enabledIndex >= 0;
        if (toggle) {
            const std::string disabled = option.values[static_cast<size_t>(disabledIndex)].value;
            const std::string enabled = option.values[static_cast<size_t>(enabledIndex)].value;
            m_coreItems.push_back(_toggle(title, description,
                beiklive::material::SETTINGS,
                [key, enabled, fallback]() { return cfgGetStr(key, fallback) == enabled; },
                [key, disabled, enabled](bool value) { cfgSetStr(key, value ? enabled : disabled); }, key));
            return;
        }
        std::vector<std::string> raw;
        std::vector<std::string> labels;
        for (const auto& value : option.values) {
            raw.push_back(value.value);
            labels.push_back(_coreOptionValue(value.value, value.label));
        }
        m_coreItems.push_back(_selector(title, description,
            beiklive::material::SETTINGS, labels,
            [key, raw, fallback]() { return findIndex(raw, cfgGetStr(key, fallback)); },
            [key, raw](int index) {
                if (index >= 0 && index < static_cast<int>(raw.size()))
                    cfgSetStr(key, raw[static_cast<size_t>(index)]);
            }, key));
    }

    void _finishCorePage(const std::string& title)
    {
        m_coreTitle = title;
        m_coreFocus = _firstFocusable(m_coreItems);
        m_coreScroll = m_coreTargetScroll = 0.f;
        m_inCore = true;
        m_contentEntrance = 0.f;
    }

    void _openMappingPage(const std::string& title, const std::string& prefix, bool nds)
    {
        m_mappingTitle = title;
        m_mappingPrefix = prefix;
        m_mappingNds = nds;
        _buildMappingItems(m_mappingPrefix, m_mappingNds);
        m_inMapping = true;
        m_contentEntrance = 0.f;
        invalidate();
    }

    void _appendExternalCorePaths(const std::string& coreName,
                                  const std::string& pathKey,
                                  const std::string& defaultPath,
                                  const std::string& returnKey)
    {
        m_coreItems.push_back(_action(
            coreName + L(" NRO 路径"), L("链式调用时启动的外部核心 Stub"), beiklive::material::DESCRIPTION,
            [pathKey, defaultPath]() {
                const auto p = cfgGetStr(pathKey, defaultPath);
                return p.empty() ? L("未设置  >") : beiklive::tools::getFileName(p) + "  >";
            },
            [this, pathKey]() { _pickFile(pathKey, {"nro"}); }));
        m_coreItems.push_back(_action(
            L("返回主程序路径"), L("外部核心退出游戏后返回的 GBAStation NRO"), beiklive::material::DESCRIPTION,
            [returnKey]() {
                const auto p = cfgGetStr(returnKey, "sdmc:/switch/GBAStation.nro");
                return p.empty() ? L("未设置  >") : beiklive::tools::getFileName(p) + "  >";
            },
            [this, returnKey]() { _pickFile(returnKey, {"nro"}); }));
    }

    void _appendExternalDisplaySettings(const std::string& prefix,
                                        const std::vector<std::string>& sizeValues,
                                        const std::vector<std::string>& sizeLabels,
                                        const std::string& defaultSize)
    {
        const std::vector<std::string> modeValues = {"Display", "Integer"};
        m_coreItems.push_back(_selector(
            L("画面模式"), L("Display 为按比例填充，Integer 为整数缩放"), 0xE8FF,
            {L("屏幕适配"), L("整数缩放")},
            [prefix, modeValues]() {
                return findIndex(modeValues, cfgGetStr("core." + prefix + ".display_mode", "Display"));
            },
            [prefix, modeValues](int i) {
                if (i >= 0 && i < static_cast<int>(modeValues.size()))
                    cfgSetStr("core." + prefix + ".display_mode", modeValues[static_cast<size_t>(i)]);
            }, "core." + prefix + ".display_mode"));
        m_coreItems.push_back(_selector(
            L("画面尺寸"), L("外部核心启动和菜单会读取同一配置"), 0xE8FF,
            sizeLabels,
            [prefix, sizeValues, defaultSize]() {
                return findIndex(sizeValues, cfgGetStr("core." + prefix + ".display_size", defaultSize));
            },
            [prefix, sizeValues](int i) {
                if (i >= 0 && i < static_cast<int>(sizeValues.size()))
                    cfgSetStr("core." + prefix + ".display_size", sizeValues[static_cast<size_t>(i)]);
            }, "core." + prefix + ".display_size"));
    }

    struct ExternalCoreOption {
        const char* category;
        const char* key;
        const char* title;
        const char* defaultValue;
    };

    static std::string _externalOptionDescription(const ExternalCoreOption& option)
    {
        const std::string key = option.key;
        if (key.find("resolution") != std::string::npos) return L("提高内部渲染精度；较高倍率会增加 GPU 负载。");
        if (key.find("frameskip") != std::string::npos || key.find("frame_skip") != std::string::npos) return L("在性能不足时减少绘制的画面数量以保持运行速度。");
        if (key.find("texture") != std::string::npos || key.find("filter") != std::string::npos) return L("控制纹理的采样、过滤或替换方式。");
        if (key.find("anisotropic") != std::string::npos) return L("改善倾斜角度下的纹理清晰度。");
        if (key.find("widescreen") != std::string::npos) return L("为支持的游戏启用宽屏画面修正。");
        if (key.find("vmu") != std::string::npos) return L("配置 Dreamcast VMU 记忆卡及其屏幕显示。");
        if (key.find("lightgun") != std::string::npos || key.find("crosshair") != std::string::npos) return L("配置光枪输入和准星显示。");
        if (key.find("deadzone") != std::string::npos) return L("过滤摇杆或扳机的轻微抖动。");
        if (key.find("language") != std::string::npos) return L("设置模拟主机向游戏报告的系统语言。");
        if (key.find("region") != std::string::npos || key.find("broadcast") != std::string::npos) return L("设置模拟主机的地区和视频制式。");
        if (key.find("bios") != std::string::npos) return L("控制 BIOS 启动流程与兼容性行为。");
        if (key.find("renderer") != std::string::npos || key.find("rendering") != std::string::npos) return L("控制图形后端及渲染工作方式。");
        if (key.find("cpu") != std::string::npos || key.find("clock") != std::string::npos) return L("调整模拟 CPU 的执行方式或运行频率。");
        if (key.find("cheat") != std::string::npos) return L("控制游戏金手指功能。");
        if (key.find("network") != std::string::npos || key.find("bba") != std::string::npos || key.find("upnp") != std::string::npos) return L("控制网络与宽带适配器功能。");
        if (key.find("audio") != std::string::npos || key.find("dsp") != std::string::npos) return L("控制声音模拟和音频输出行为。");
        if (key.find("input") != std::string::npos || key.find("analog") != std::string::npos || key.find("trigger") != std::string::npos) return L("控制模拟器接收和解释手柄输入的方式。");
        return L("调整该核心功能的兼容性、画面或运行行为。");
    }

    static std::vector<std::string> _externalOptionValues(const ExternalCoreOption& option)
    {
        const std::string key = option.key;
        const std::string value = option.defaultValue;
        if (value == "enabled" || value == "disabled") return {"disabled", "enabled"};
        if (key == "ppsspp_cpu_core") return {"JIT", "IR JIT", "Interpreter"};
        if (key == "ppsspp_io_timing_method") return {"Fast", "Host", "Simulate UMD delays", "Simulate UMD slow reading speed"};
        if (key == "ppsspp_psp_model") return {"psp_1000", "psp_2000_3000"};
        if (key == "ppsspp_button_preference") return {"Cross", "Circle"};
        if (key == "ppsspp_internal_resolution") return {"480x272", "960x544", "1440x816", "1920x1088", "2400x1360", "2880x1632"};
        if (key == "ppsspp_rendering_mode") return {"buffered", "nonbuffered"};
        if (key == "ppsspp_texture_filtering") return {"Auto", "Nearest", "Linear", "Auto max quality"};
        if (key == "ppsspp_texture_anisotropic_filtering") return {"Off", "2x", "4x", "8x", "16x"};
        if (key == "ppsspp_lower_resolution_for_effects") return {"Off", "Safe", "Balanced", "Aggressive"};
        if (key == "ppsspp_texture_scaling_type") return {"xbrz", "hybrid", "bicubic", "hybrid_bicubic"};
        if (key == "ppsspp_texture_scaling_level") return {"disabled", "2x", "3x", "4x", "5x"};
        if (key == "ppsspp_frameskip") return {"0", "1", "2", "3", "4", "5"};
        if (key == "ppsspp_inflight_frames") return {"No buffer", "Up to 1", "Up to 2"};
        if (key == "ppsspp_analog_deadzone") return {"0.00", "0.10", "0.15", "0.20", "0.25", "0.30"};
        if (key == "ppsspp_analog_sensitivity") return {"0.80", "0.90", "1.00", "1.10", "1.20", "1.30"};
        if (key == "ppsspp_language") return {"Automatic", "English", "Japanese", "French", "Spanish", "German", "Italian", "Korean", "Chinese Traditional", "Chinese Simplified"};
        if (key == "reicast_renderer") return {"vulkan"};
        if (key == "reicast_internal_resolution") return {"640x480", "960x720", "1280x960", "1920x1440"};
        if (key == "reicast_region") return {"USA", "Japan", "Europe"};
        if (key == "reicast_language") return {"English", "Japanese", "French", "German", "Italian", "Spanish", "Chinese", "Korean"};
        if (key == "reicast_cable_type") return {"TV (Composite)", "TV (RGB)", "VGA"};
        if (key == "reicast_broadcast") return {"NTSC", "PAL", "PAL-M", "PAL-N"};
        if (key == "reicast_screen_rotation") return {"horizontal", "vertical"};
        if (key == "reicast_alpha_sorting") return {"per-strip", "per-triangle (normal)", "per-pixel"};
        if (key == "reicast_oit_abuffer_size") return {"128MB", "256MB", "512MB", "1024MB"};
        if (key == "reicast_oit_layers") return {"8", "16", "32", "64"};
        if (key == "reicast_anisotropic_filtering") return {"1", "2", "4", "8", "16"};
        if (key == "reicast_texture_filtering") return {"0", "1", "2", "3", "4"};
        if (key == "reicast_texupscale") return {"1", "2", "3", "4", "5", "6"};
        if (key == "reicast_texupscale_max_filtered_texture_size") return {"64", "128", "256", "512", "1024"};
        if (key == "reicast_sh4clock") return {"100", "150", "180", "200", "220", "240"};
        if (key.find("deadzone") != std::string::npos) return {"0%", "5%", "10%", "15%", "20%", "25%", "30%"};
        if (key == "reicast_lightgun_crosshair_size_scaling") return {"50%", "75%", "100%", "125%", "150%"};
        if (key.find("device_port") != std::string::npos) return {"None", "VMU", "Purupuru", "Microphone"};
        if (key == "reicast_per_content_vmus") return {"All VMUs", "Per-Game VMUs"};
        if (key.find("screen_position") != std::string::npos) return {"Upper Left", "Upper Right", "Lower Left", "Lower Right"};
        if (key.find("screen_size_mult") != std::string::npos) return {"1x", "2x", "3x", "4x"};
        if (key.find("screen_opacity") != std::string::npos) return {"25%", "50%", "75%", "100%"};
        if (key == "fbneo-cpu-speed-adjust") return {"50%", "75%", "100%", "125%", "150%", "200%"};
        if (key == "fbneo-analog-speed") return {"50%", "75%", "100%", "125%", "150%"};
        if (key == "fbneo-frameskip") return {"0", "1", "2", "3", "4", "5"};
        if (key == "fbneo-resolution") return {"640x480", "800x600", "1024x768", "1280x960", "1600x1200", "1920x1440"};
        if (key == "fbneo-vertical-mode") return {"disabled", "enabled", "alternate", "TATE", "TATE alternate"};
        if (key == "fbneo-frameskip-type") return {"disabled", "Fixed", "Auto", "Manual"};
        if (key == "fbneo-fixed-frameskip") return {"0", "1", "2", "3", "4", "5"};
        if (key == "fbneo-frameskip-manual-threshold") return {"15", "18", "21", "24", "27", "30", "33", "36", "39", "42", "45", "48", "51", "54", "57", "60"};
        if (key == "fbneo-diagnostic-input") return {"None", "Hold Start", "Start + A + B", "Hold Start + A + B", "Start + L + R", "Hold Start + L + R", "Hold Select", "Select + A + B", "Select + L + R"};
        if (key == "fbneo-samplerate") return {"44100", "48000"};
        if (key == "fbneo-sample-interpolation") return {"disabled", "2-point 1st order", "4-point 3rd order"};
        if (key == "fbneo-fm-interpolation") return {"disabled", "4-point 3rd order"};
        if (key == "fbneo-socd") return {"0", "1", "2", "3", "4", "5", "6"};
        if (key == "fbneo-lightgun-crosshair-emulation") return {"hide with lightgun device", "always hide", "always show"};
        if (key == "fbneo-neogeo-mode") return {"DIPSWITCH", "MVS_EUR", "MVS_USA", "MVS_JAP", "AES_EUR", "AES_JAP", "UNIBIOS"};
        if (key == "fbneo-memcard-mode") return {"disabled", "shared", "per-game"};
        return {value};
    }

    // External cores deliberately keep their own JSONC files.  The launcher is
    // the common source of truth, so every option is mirrored in config.cfg and
    // copied into the JSONC file by the core before it boots.
    void _appendExternalOptions(const std::string& prefix,
                                const std::vector<ExternalCoreOption>& options)
    {
        std::string category;
        for (const auto& option : options) {
            if (category != option.category) {
                category = option.category;
                m_coreItems.push_back(_section(category));
            }
            const std::string configKey = "core." + prefix + "." + option.key;
            const std::string fallback = option.defaultValue;
            if (beiklive::SettingManager)
                beiklive::SettingManager->SetDefault(configKey, ConfigValue(fallback));
            const std::vector<std::string> values = _externalOptionValues(option);
            std::vector<std::string> labels;
            labels.reserve(values.size());
            for (const auto& value : values)
                labels.push_back(_coreOptionValue(value, value));
            m_coreItems.push_back(_selector(
                option.title, _externalOptionDescription(option), beiklive::material::SETTINGS, labels,
                [configKey, values, fallback]() { return findIndex(values, cfgGetStr(configKey, fallback)); },
                [configKey, values](int index) {
                    if (index >= 0 && index < static_cast<int>(values.size()))
                        cfgSetStr(configKey, values[static_cast<size_t>(index)]);
                }, configKey));
        }
    }

    void _openLibretroCore(const std::string& title, CoreType type)
    {
        m_coreItems.clear();
        LibretroLoader::discoverCoreOptions(type, beiklive::SettingManager);
        std::string category;
        for (const auto& option : LibretroLoader::coreOptions(type)) {
            if (option.category != category) {
                category = option.category;
                m_coreItems.push_back(_section(_coreCategoryTitle(category)));
            }
            _appendRawOption(option);
        }
        _finishCorePage(title);
    }

    void _openMgbaCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("系统")));
        const std::vector<std::string> modelValues = {"Autodetect", "Game Boy", "Super Game Boy", "Game Boy Color", "Game Boy Advance"};
        const std::vector<std::string> modelLabels = {L("自动识别"), "Game Boy", "Super Game Boy", "Game Boy Color", "Game Boy Advance"};
        m_coreItems.push_back(_selector(L("GB 机型"), L("根据 ROM 头自动判断机型"), 0xE30F, modelLabels,
            [modelValues]() { return findIndex(modelValues, cfgGetStr("core.mgba_gb_model", "Autodetect")); },
            [modelValues](int i) { if (i >= 0 && i < (int)modelValues.size()) cfgSetStr("core.mgba_gb_model", modelValues[i]); }, "core.mgba_gb_model"));
        m_coreItems.push_back(_toggle(L("使用 BIOS"), L("使用真实 BIOS 启动"), 0xE86F,
            []() { return cfgGetStr("core.mgba_use_bios", "ON") == "ON"; }, [](bool v) { cfgSetStr("core.mgba_use_bios", v ? "ON" : "OFF"); }, "core.mgba_use_bios"));
        m_coreItems.push_back(_toggle(L("跳过 BIOS 动画"), L("直接进入游戏"), 0xE044,
            []() { return cfgGetStr("core.mgba_skip_bios", "OFF") == "ON"; }, [](bool v) { cfgSetStr("core.mgba_skip_bios", v ? "ON" : "OFF"); }, "core.mgba_skip_bios"));
        const std::vector<std::string> rtcLabels = {L("持久化 RTC"), L("跟随系统时间")}, rtcValues = {"persist", "system"};
        m_coreItems.push_back(_selector(L("RTC 时钟模式"), L("游戏实时时钟来源"), 0xE8B5, rtcLabels,
            [rtcValues]() { return findIndex(rtcValues, cfgGetStr("core.mgba_rtc_mode", "persist")); },
            [rtcValues](int i) { if (i >= 0 && i < 2) cfgSetStr("core.mgba_rtc_mode", rtcValues[i]); }, "core.mgba_rtc_mode"));
        m_coreItems.push_back(_section(L("视频")));
        const auto colors = beiklive::GetGbColorPresets();
        m_coreItems.push_back(_selector(L("GB 配色"), L("GB 单色游戏的调色板"), 0xE40A, colors,
            [colors]() { return findIndex(colors, cfgGetStr("core.mgba_gb_colors", "Grayscale")); },
            [colors](int i) { if (i >= 0 && i < (int)colors.size()) cfgSetStr("core.mgba_gb_colors", colors[i]); }, "core.mgba_gb_colors"));
        m_coreItems.push_back(_toggle(L("SGB 边框"), L("显示 Super Game Boy 边框"), 0xE3F4,
            []() { return cfgGetStr("core.mgba_sgb_borders", "OFF") == "ON"; }, [](bool v) { cfgSetStr("core.mgba_sgb_borders", v ? "ON" : "OFF"); }, "core.mgba_sgb_borders"));
        const std::vector<std::string> frames = {"0","1","2","3","4","5","6","7","8","9","10"};
        m_coreItems.push_back(_selector(L("跳帧"), L("降低渲染负载"), 0xE8D5, frames,
            [frames]() { return findIndex(frames, cfgGetStr("core.mgba_frameskip", "0")); },
            [frames](int i) { if (i >= 0 && i < (int)frames.size()) cfgSetStr("core.mgba_frameskip", frames[i]); }, "core.mgba_frameskip"));
        m_coreItems.push_back(_section(L("音频")));
        m_coreItems.push_back(_toggle(L("低通滤波"), L("模拟硬件音色"), 0xE050,
            []() { return cfgGetStr("core.mgba_audio_low_pass_filter", "disabled") == "enabled"; }, [](bool v) { cfgSetStr("core.mgba_audio_low_pass_filter", v ? "enabled" : "disabled"); }, "core.mgba_audio_low_pass_filter"));
        const std::vector<std::string> ranges = {"20","40","60","80","100"};
        m_coreItems.push_back(_selector(L("滤波强度"), L("低通滤波截止范围"), 0xE8E5, {"20%","40%","60%","80%","100%"},
            [ranges]() { return findIndex(ranges, cfgGetStr("core.mgba_audio_low_pass_range", "60"), 2); },
            [ranges](int i) { if (i >= 0 && i < 5) cfgSetStr("core.mgba_audio_low_pass_range", ranges[i]); }, "core.mgba_audio_low_pass_range"));
        m_coreItems.push_back(_section(L("输入与性能")));
        const std::vector<std::string> idleValues = {"Remove Known", "Detect and Remove", "Don't Remove"};
        const std::vector<std::string> idleLabels = {L("移除已知空闲循环"), L("检测并移除"), L("不移除")};
        m_coreItems.push_back(_selector(L("空闲循环优化"), L("降低无意义 CPU 占用"), 0xE8EF, idleLabels,
            [idleValues]() { return findIndex(idleValues, cfgGetStr("core.mgba_idle_optimization", "Remove Known")); },
            [idleValues](int i) { if (i >= 0 && i < 3) cfgSetStr("core.mgba_idle_optimization", idleValues[i]); }, "core.mgba_idle_optimization"));
        m_coreItems.push_back(_toggle(L("允许相反方向"), L("允许左右或上下同时按下"), 0xE5D5,
            []() { return cfgGetStr("core.mgba_allow_opposing_directions", "no") == "yes"; }, [](bool v) { cfgSetStr("core.mgba_allow_opposing_directions", v ? "yes" : "no"); }, "core.mgba_allow_opposing_directions"));
        m_coreItems.push_back(_toggle(L("强制 GBP 振动"), L("模拟 Game Boy Player 振动"), 0xE8B8,
            []() { return cfgGetStr("core.mgba_force_gbp", "OFF") == "ON"; }, [](bool v) { cfgSetStr("core.mgba_force_gbp", v ? "ON" : "OFF"); }, "core.mgba_force_gbp"));
        const std::vector<std::string> solar = {"0","1","2","3","4","5","6","7","8","9","10"};
        m_coreItems.push_back(_selector(L("太阳传感器"), L("Boktai 等游戏使用"), 0xE3B0, solar,
            [solar]() { return findIndex(solar, cfgGetStr("core.mgba_solar_sensor_level", "5"), 5); },
            [solar](int i) { if (i >= 0 && i < 11) cfgSetStr("core.mgba_solar_sensor_level", solar[i]); }, "core.mgba_solar_sensor_level"));
        _finishCorePage(L("mGBA 核心设置"));
    }

    void _openMelonDsCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("BIOS、固件与启动")));
        auto file = [this](const std::string& title, const std::string& key, const std::vector<std::string>& ext) {
            m_coreItems.push_back(_action(title, L("选择 melonDS 使用的系统文件"), beiklive::material::DESCRIPTION,
                [key]() { const auto p = cfgGetStr(key, ""); return p.empty() ? L("默认路径  >") : beiklive::tools::getFileName(p) + "  >"; },
                [this, key, ext]() { _pickFile(key, ext); }));
        };
        file("ARM9 BIOS", "core.melonds_bios9_path", {"bin","rom"});
        file("ARM7 BIOS", "core.melonds_bios7_path", {"bin","rom"});
        file(L("固件"), "core.melonds_firmware_path", {"bin"});
        m_coreItems.push_back(_selector(L("启动方式"), L("可直接进入游戏，或先进入 Nintendo DS 固件菜单"), 0xE044,
            {L("直接进入游戏"), L("先进入固件菜单")},
            []() { return cfgGetBool("core.melonds_direct_boot", true) ? 0 : 1; },
            [](int i) { cfgSetBool("core.melonds_direct_boot", i == 0); }, "core.melonds_direct_boot"));
        const std::vector<std::string> firmwareLanguages = {
            L("跟随固件"), L("日语"), L("英语"), L("法语"), L("德语"), L("意大利语"), L("西班牙语"), L("简体中文")};
        m_coreItems.push_back(_selector(L("固件语言"), L("仅覆盖运行时语言，不会修改固件文件"), 0xE8C4,
            firmwareLanguages,
            []() { return std::clamp(cfgGetInt("core.melonds_firmware_language", -1) + 1, 0, 7); },
            [](int i) { cfgSetInt("core.melonds_firmware_language", std::clamp(i, 0, 7) - 1); }, "core.melonds_firmware_language"));
        m_coreItems.push_back(_section(L("JIT 与性能")));
        m_coreItems.push_back(_toggle(L("启用 JIT"), L("显著提升模拟性能"), 0xE8EF,
            []() { return cfgGetBool("core.melonds_jit_enabled", true); }, [](bool v) { cfgSetBool("core.melonds_jit_enabled", v); }, "core.melonds_jit_enabled"));
        const std::vector<std::string> blocks = {"8","16","32","64"};
        m_coreItems.push_back(_selector(L("JIT 最大块"), L("较大值性能更高但兼容性可能下降"), 0xE1B1, blocks,
            [blocks]() { return findIndex(blocks, cfgGetStr("core.melonds_jit_block_size", "32"), 2); },
            [blocks](int i) { if (i >= 0 && i < 4) cfgSetStr("core.melonds_jit_block_size", blocks[i]); }, "core.melonds_jit_block_size"));
        m_coreItems.push_back(_toggle("分支优化", "优化 JIT 分支", 0xE8EF, []() { return cfgGetBool("core.melonds_jit_branch", true); }, [](bool v) { cfgSetBool("core.melonds_jit_branch", v); }, "core.melonds_jit_branch"));
        m_coreItems.push_back(_toggle("常量优化", "优化 JIT 常量访问", 0xE8EF, []() { return cfgGetBool("core.melonds_jit_literal", true); }, [](bool v) { cfgSetBool("core.melonds_jit_literal", v); }, "core.melonds_jit_literal"));
        m_coreItems.push_back(_toggle("快速内存", "提高内存访问速度", 0xE8EF, []() { return cfgGetBool("core.melonds_jit_fast_memory", true); }, [](bool v) { cfgSetBool("core.melonds_jit_fast_memory", v); }, "core.melonds_jit_fast_memory"));
        m_coreItems.push_back(_section(L("视频")));
        m_coreItems.push_back(_toggle(L("线程渲染"), L("将软件渲染工作放到独立线程"), 0xE8D5,
            []() { return cfgGetBool("core.melonds_threaded_renderer", true); }, [](bool v) { cfgSetBool("core.melonds_threaded_renderer", v); }, "core.melonds_threaded_renderer"));
        const std::vector<std::string> scales = {"1","2","3","4"};
        m_coreItems.push_back(_selector(L("内部渲染倍率"), L("提高 3D 画面的内部清晰度"), 0xE8FF, {"1x","2x","3x","4x"},
            [scales]() { return findIndex(scales, cfgGetStr("core.melonds_render_scale", "1")); },
            [scales](int i) { if (i >= 0 && i < 4) cfgSetStr("core.melonds_render_scale", scales[i]); }, "core.melonds_render_scale"));
        m_coreItems.push_back(_toggle(L("改进多边形"), L("提高 3D 多边形边缘精度"), 0xE3F4,
            []() { return cfgGetBool("core.melonds_better_polygons", false); }, [](bool v) { cfgSetBool("core.melonds_better_polygons", v); }, "core.melonds_better_polygons"));
        m_coreItems.push_back(_section(L("存储与网络")));
        m_coreItems.push_back(_toggle("启用 DLDI", "启用虚拟 SD 卡访问", beiklive::material::STORAGE, []() { return cfgGetBool("core.melonds_dldi_enabled", false); }, [](bool v) { cfgSetBool("core.melonds_dldi_enabled", v); }, "core.melonds_dldi_enabled"));
        file(L("DLDI SD 镜像"), "core.melonds_dldi_path", {"img","bin"});
        m_coreItems.push_back(_toggle("随机 MAC 地址", "每次启动生成随机无线地址", beiklive::material::WIFI, []() { return cfgGetBool("core.melonds_randomize_mac", false); }, [](bool v) { cfgSetBool("core.melonds_randomize_mac", v); }, "core.melonds_randomize_mac"));
        _finishCorePage(L("melonDS 核心设置"));
    }

    void _openFbneoCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("外部核心")));
        _appendExternalCorePaths(
            "FBNeo", "arcade.externalNro.path", "/GBAStation/core/GBAStationFBNeoStub.nro",
            "arcade.externalNro.returnPath");

        // m_coreItems.push_back(_section(L("画面")));
        // _appendExternalDisplaySettings(
        //     "fbneo",
        //     {"Auto", "4:3", "16:9", "Stretch", "Original", "1x", "2x"},
        //     {L("自动"), "4:3", "16:9", L("拉伸"), L("原始"), "1x", "2x"},
        //     "Auto");
        const std::vector<std::string> shaderValues = {"None", "xBRZ", "Eagle", "CrtEasyMode"};
        m_coreItems.push_back(_selector(
            L("着色器"), L("外部街机核心的画面滤镜"), 0xE40A,
            {L("关闭"), "xBRZ", "Eagle", "CRT EasyMode"},
            [shaderValues]() {
                return findIndex(shaderValues, cfgGetStr("core.fbneo.shader_type", "None"));
            },
            [shaderValues](int i) {
                if (i >= 0 && i < static_cast<int>(shaderValues.size()))
                    cfgSetStr("core.fbneo.shader_type", shaderValues[static_cast<size_t>(i)]);
            }, "core.fbneo.shader_type"));

        _appendExternalOptions("fbneo", {
            {"视频", "fbneo-allow-depth-32", "32 位色深", "enabled"},
            {"视频", "fbneo-vertical-mode", "竖屏游戏方向", "disabled"},
            {"视频", "fbneo-force-60hz", "强制 60Hz", "disabled"},
            {"视频", "fbneo-resolution", "矢量游戏分辨率", "640x480"},
            {"性能", "fbneo-frameskip-type", "跳帧方式", "disabled"},
            {"性能", "fbneo-fixed-frameskip", "固定跳帧", "0"},
            {"性能", "fbneo-frameskip-manual-threshold", "手动跳帧阈值", "33"},
            {"性能", "fbneo-cpu-speed-adjust", "CPU 时钟", "100%"},
            {"街机系统", "fbneo-diagnostic-input", "诊断输入", "Hold Start"},
            {"街机系统", "fbneo-hiscores", "高分记录", "enabled"},
            {"街机系统", "fbneo-allow-patched-romsets", "允许补丁 ROM", "enabled"},
            {"音频", "fbneo-samplerate", "采样率", "48000"},
            {"音频", "fbneo-sample-interpolation", "采样插值", "4-point 3rd order"},
            {"音频", "fbneo-fm-interpolation", "FM 插值", "4-point 3rd order"},
            {"音频", "fbneo-lowpass-filter", "低通滤波", "disabled"},
            {"输入", "fbneo-analog-speed", "模拟摇杆速度", "100%"},
            {"输入", "fbneo-socd", "相反方向处理", "3"},
            {"输入", "fbneo-lightgun-crosshair-emulation", "光枪准星", "hide with lightgun device"},
            {"Neo Geo", "fbneo-neogeo-mode", "Neo Geo BIOS 模式", "DIPSWITCH"},
            {"Neo Geo", "fbneo-memcard-mode", "Neo Geo 记忆卡", "disabled"},
        });

        m_coreItems.push_back(_section(L("按键")));
        m_coreItems.push_back(_action(
            L("Arcade 按键映射"), L("配置 FBNeo 外部核心使用的 config.cfg 映射"), 0xE30F,
            []() { return std::string(L("进入配置  >")); },
            [this]() { _openMappingPage(L("Arcade 按键映射"), "arcade.", false); }));
        _finishCorePage(L("FBNeo 核心设置"));
    }

    void _openFlycastCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("外部核心")));
        _appendExternalCorePaths(
            "Flycast", "dc.externalNro.path", "/GBAStation/core/GBAStationFlycastStub.nro",
            "dc.externalNro.returnPath");

        m_coreItems.push_back(_section(L("画面")));
        _appendExternalDisplaySettings(
            "flycast",
            {"4:3", "16:9", "Stretch", "Original", "1x", "2x", "Auto"},
            {"4:3", "16:9", L("拉伸"), L("原始"), "1x", "2x", L("自动")},
            "4:3");

        _appendExternalOptions("flycast", {
            {"系统与 BIOS", "reicast_renderer", "渲染器", "vulkan"},
            {"系统与 BIOS", "reicast_region", "主机地区", "USA"},
            {"系统与 BIOS", "reicast_language", "系统语言", "English"},
            {"系统与 BIOS", "reicast_hle_bios", "HLE BIOS", "disabled"},
            {"系统与 BIOS", "reicast_boot_to_bios", "启动至 BIOS", "disabled"},
            {"系统与 BIOS", "reicast_enable_dsp", "启用 DSP", "enabled"},
            {"系统与 BIOS", "reicast_allow_service_buttons", "服务按键", "disabled"},
            {"系统与 BIOS", "reicast_force_freeplay", "强制免费游戏", "enabled"},
            {"系统与 BIOS", "reicast_emulate_bba", "模拟宽带适配器", "disabled"},
            {"系统与 BIOS", "reicast_upnp", "UPnP", "enabled"},
            {"视频与渲染", "reicast_internal_resolution", "内部渲染分辨率", "640x480"},
            {"视频与渲染", "reicast_screen_rotation", "屏幕旋转", "horizontal"},
            {"视频与渲染", "reicast_alpha_sorting", "透明排序", "per-triangle (normal)"},
            {"视频与渲染", "reicast_oit_abuffer_size", "OIT 缓冲区", "512MB"},
            {"视频与渲染", "reicast_oit_layers", "OIT 图层数", "32"},
            {"视频与渲染", "reicast_emulate_framebuffer", "模拟帧缓冲", "disabled"},
            {"视频与渲染", "reicast_enable_rttb", "渲染至纹理缓冲", "disabled"},
            {"视频与渲染", "reicast_mipmapping", "Mipmapping", "enabled"},
            {"视频与渲染", "reicast_fog", "雾化效果", "enabled"},
            {"视频与渲染", "reicast_volume_modifier_enable", "体积修饰器", "enabled"},
            {"视频与渲染", "reicast_anisotropic_filtering", "各向异性过滤", "4"},
            {"视频与渲染", "reicast_texture_filtering", "纹理过滤", "0"},
            {"性能", "reicast_delay_frame_swapping", "延迟交换帧", "enabled"},
            {"性能", "reicast_detect_vsync_swap_interval", "检测垂直同步", "disabled"},
            {"性能", "reicast_pvr2_filtering", "PVR2 过滤", "disabled"},
            {"性能", "reicast_texupscale", "纹理放大", "1"},
            {"性能", "reicast_texupscale_max_filtered_texture_size", "纹理放大最大尺寸", "256"},
            {"性能", "reicast_native_depth_interpolation", "原生深度插值", "disabled"},
            {"性能", "reicast_fix_upscale_bleeding_edge", "修复放大边缘渗色", "enabled"},
            {"性能", "reicast_threaded_rendering", "线程渲染", "enabled"},
            {"性能", "reicast_auto_skip_frame", "自动跳帧", "disabled"},
            {"性能", "reicast_frame_skipping", "跳帧", "disabled"},
            {"游戏兼容", "reicast_widescreen_cheats", "宽屏金手指", "disabled"},
            {"游戏兼容", "reicast_widescreen_hack", "宽屏补丁", "disabled"},
            {"游戏兼容", "reicast_gdrom_fast_loading", "GD-ROM 快速读取", "disabled"},
            {"游戏兼容", "reicast_dc_32mb_mod", "32MB 内存扩展", "disabled"},
            {"游戏兼容", "reicast_sh4clock", "SH4 时钟", "200"},
            {"纹理", "reicast_custom_textures", "自定义纹理", "disabled"},
            {"纹理", "reicast_dump_textures", "导出纹理", "disabled"},
            {"输入与网络", "reicast_analog_stick_deadzone", "摇杆死区", "15%"},
            {"输入与网络", "reicast_trigger_deadzone", "扳机死区", "0%"},
            {"输入与网络", "reicast_digital_triggers", "数字扳机", "disabled"},
            {"输入与网络", "reicast_network_output", "网络输出", "disabled"},
            {"光枪", "reicast_show_lightgun_settings", "显示光枪设置", "disabled"},
            {"光枪", "reicast_lightgun_crosshair_size_scaling", "光枪准星大小", "100%"},
            {"光枪", "reicast_lightgun1_crosshair", "光枪 1 准星", "disabled"},
            {"光枪", "reicast_lightgun2_crosshair", "光枪 2 准星", "disabled"},
            {"光枪", "reicast_lightgun3_crosshair", "光枪 3 准星", "disabled"},
            {"光枪", "reicast_lightgun4_crosshair", "光枪 4 准星", "disabled"},
            {"VMU 与外设", "reicast_device_port1_slot1", "端口 1 插槽 1", "VMU"},
            {"VMU 与外设", "reicast_device_port1_slot2", "端口 1 插槽 2", "Purupuru"},
            {"VMU 与外设", "reicast_device_port2_slot1", "端口 2 插槽 1", "VMU"},
            {"VMU 与外设", "reicast_device_port2_slot2", "端口 2 插槽 2", "Purupuru"},
            {"VMU 与外设", "reicast_device_port3_slot1", "端口 3 插槽 1", "VMU"},
            {"VMU 与外设", "reicast_device_port3_slot2", "端口 3 插槽 2", "Purupuru"},
            {"VMU 与外设", "reicast_device_port4_slot1", "端口 4 插槽 1", "VMU"},
            {"VMU 与外设", "reicast_device_port4_slot2", "端口 4 插槽 2", "Purupuru"},
            {"VMU 与外设", "reicast_per_content_vmus", "每游戏独立 VMU", "All VMUs"},
            {"VMU 与外设", "reicast_vmu_sound", "VMU 声音", "disabled"},
            {"VMU 屏幕", "reicast_show_vmu_screen_settings", "显示 VMU 屏幕设置", "disabled"},
            {"VMU 屏幕", "reicast_vmu1_screen_display", "VMU 1 屏幕", "disabled"},
            {"VMU 屏幕", "reicast_vmu1_screen_position", "VMU 1 位置", "Upper Left"},
            {"VMU 屏幕", "reicast_vmu1_screen_size_mult", "VMU 1 尺寸", "1x"},
            {"VMU 屏幕", "reicast_vmu1_pixel_on_color", "VMU 1 亮点颜色", "DEFAULT_ON 00"},
            {"VMU 屏幕", "reicast_vmu1_pixel_off_color", "VMU 1 暗点颜色", "DEFAULT_OFF 01"},
            {"VMU 屏幕", "reicast_vmu1_screen_opacity", "VMU 1 不透明度", "100%"},
            {"VMU 屏幕", "reicast_vmu2_screen_display", "VMU 2 屏幕", "disabled"},
            {"VMU 屏幕", "reicast_vmu2_screen_position", "VMU 2 位置", "Upper Right"},
            {"VMU 屏幕", "reicast_vmu2_screen_size_mult", "VMU 2 尺寸", "1x"},
            {"VMU 屏幕", "reicast_vmu2_pixel_on_color", "VMU 2 亮点颜色", "DEFAULT_ON 00"},
            {"VMU 屏幕", "reicast_vmu2_pixel_off_color", "VMU 2 暗点颜色", "DEFAULT_OFF 01"},
            {"VMU 屏幕", "reicast_vmu2_screen_opacity", "VMU 2 不透明度", "100%"},
            {"VMU 屏幕", "reicast_vmu3_screen_display", "VMU 3 屏幕", "disabled"},
            {"VMU 屏幕", "reicast_vmu3_screen_position", "VMU 3 位置", "Lower Left"},
            {"VMU 屏幕", "reicast_vmu3_screen_size_mult", "VMU 3 尺寸", "1x"},
            {"VMU 屏幕", "reicast_vmu3_pixel_on_color", "VMU 3 亮点颜色", "DEFAULT_ON 00"},
            {"VMU 屏幕", "reicast_vmu3_pixel_off_color", "VMU 3 暗点颜色", "DEFAULT_OFF 01"},
            {"VMU 屏幕", "reicast_vmu3_screen_opacity", "VMU 3 不透明度", "100%"},
            {"VMU 屏幕", "reicast_vmu4_screen_display", "VMU 4 屏幕", "disabled"},
            {"VMU 屏幕", "reicast_vmu4_screen_position", "VMU 4 位置", "Lower Right"},
            {"VMU 屏幕", "reicast_vmu4_screen_size_mult", "VMU 4 尺寸", "1x"},
            {"VMU 屏幕", "reicast_vmu4_pixel_on_color", "VMU 4 亮点颜色", "DEFAULT_ON 00"},
            {"VMU 屏幕", "reicast_vmu4_pixel_off_color", "VMU 4 暗点颜色", "DEFAULT_OFF 01"},
            {"VMU 屏幕", "reicast_vmu4_screen_opacity", "VMU 4 不透明度", "100%"},
        });

        m_coreItems.push_back(_section(L("按键")));
        m_coreItems.push_back(_action(
            L("DC 按键映射"), L("配置 Flycast 外部核心使用的 config.cfg 映射"), 0xE30F,
            []() { return std::string(L("进入配置  >")); },
            [this]() { _openMappingPage(L("DC 按键映射"), "dc.", false); }));
        _finishCorePage(L("Flycast 核心设置"));
    }

    void _openPpssppCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("外部核心")));
        _appendExternalCorePaths(
            "PPSSPP", "psp.externalNro.path", "/GBAStation/core/GBAStationPPSSPPStub.nro",
            "psp.externalNro.returnPath");

        m_coreItems.push_back(_section(L("性能与画面")));
        _appendExternalDisplaySettings(
            "ppsspp",
            {"16:9", "4:3", "Stretch", "Original", "1x", "2x", "3x", "4x", "Auto"},
            {"16:9", "4:3", L("拉伸"), L("原始"), "1x", "2x", "3x", "4x", L("自动")},
            "16:9");
        const std::vector<std::string> resolutionValues = {"1", "2", "3", "4"};
        m_coreItems.push_back(_selector(
            L("渲染分辨率"), L("提高 3D 画面内部清晰度，倍率越高负载越大"), 0xE8FF,
            {"1x PSP", "2x PSP", "3x PSP", "4x PSP"},
            [resolutionValues]() {
                return findIndex(resolutionValues, cfgGetStr("core.ppsspp.rendering_resolution", "1"));
            },
            [resolutionValues](int i) {
                if (i >= 0 && i < static_cast<int>(resolutionValues.size()))
                    cfgSetStr("core.ppsspp.rendering_resolution", resolutionValues[static_cast<size_t>(i)]);
            }, "core.ppsspp.rendering_resolution"));
        const std::vector<std::string> frameskipValues = {"0", "1", "2", "3", "4", "5"};
        m_coreItems.push_back(_selector(
            L("跳帧"), L("降低渲染压力；0 表示关闭"), 0xE8D5,
            {L("关闭"), "1", "2", "3", "4", "5"},
            [frameskipValues]() {
                return findIndex(frameskipValues, cfgGetStr("core.ppsspp.frameskip", "0"));
            },
            [frameskipValues](int i) {
                if (i >= 0 && i < static_cast<int>(frameskipValues.size()))
                    cfgSetStr("core.ppsspp.frameskip", frameskipValues[static_cast<size_t>(i)]);
            }, "core.ppsspp.frameskip"));
        m_coreItems.push_back(_toggle(
            L("自动跳帧"), L("低帧率时自动跳过部分画面"), 0xE8D5,
            []() { return cfgGetBool("core.ppsspp.auto_frameskip", false); },
            [](bool v) { cfgSetBool("core.ppsspp.auto_frameskip", v); }, "core.ppsspp.auto_frameskip"));
        m_coreItems.push_back(_toggle(
            L("快速内存"), L("启用 PPSSPP Fast Memory，速度更快但可能影响少数游戏"), 0xE8EF,
            []() { return cfgGetBool("core.ppsspp.fast_memory", true); },
            [](bool v) { cfgSetBool("core.ppsspp.fast_memory", v); }, "core.ppsspp.fast_memory"));
        m_coreItems.push_back(_toggle(
            L("I/O 独立线程"), L("将部分文件读取放到独立线程，减少卡顿"), 0xE8D5,
            []() { return cfgGetBool("core.ppsspp.io_thread", true); },
            [](bool v) { cfgSetBool("core.ppsspp.io_thread", v); }, "core.ppsspp.io_thread"));

        _appendExternalOptions("ppsspp", {
            {"CPU 与性能", "ppsspp_cpu_core", "CPU 后端", "JIT"},
            {"CPU 与性能", "ppsspp_fast_memory", "快速内存", "enabled"},
            {"CPU 与性能", "ppsspp_ignore_bad_memory_access", "忽略错误内存访问", "enabled"},
            {"CPU 与性能", "ppsspp_io_timing_method", "I/O 时序", "Fast"},
            {"CPU 与性能", "ppsspp_force_lag_sync", "强制延迟同步", "disabled"},
            {"CPU 与性能", "ppsspp_locked_cpu_speed", "锁定 CPU 时钟", "0"},
            {"CPU 与性能", "ppsspp_cache_iso", "缓存 ISO", "disabled"},
            {"系统", "ppsspp_cheats", "启用金手指", "disabled"},
            {"系统", "ppsspp_psp_model", "PSP 机型", "psp_2000_3000"},
            {"系统", "ppsspp_button_preference", "确认键偏好", "Cross"},
            {"系统", "ppsspp_language", "PSP 系统语言", "Automatic"},
            {"系统", "ppsspp_memstick_inserted", "插入记忆棒", "enabled"},
            {"视频与渲染", "ppsspp_internal_resolution", "内部渲染分辨率", "480x272"},
            {"视频与渲染", "ppsspp_software_rendering", "软件渲染", "disabled"},
            {"视频与渲染", "ppsspp_rendering_mode", "渲染模式", "buffered"},
            {"视频与渲染", "ppsspp_gpu_hardware_transform", "硬件变换", "enabled"},
            {"视频与渲染", "ppsspp_skip_buffer_effects", "跳过缓冲效果", "disabled"},
            {"视频与渲染", "ppsspp_frameskip", "跳帧", "0"},
            {"视频与渲染", "ppsspp_auto_frameskip", "自动跳帧", "disabled"},
            {"视频与渲染", "ppsspp_frame_duplication", "重复帧", "disabled"},
            {"视频与渲染", "ppsspp_detect_vsync_swap_interval", "检测垂直同步", "disabled"},
            {"视频与渲染", "ppsspp_inflight_frames", "在途帧数", "Up to 2"},
            {"纹理", "ppsspp_texture_filtering", "纹理过滤", "Auto"},
            {"纹理", "ppsspp_texture_anisotropic_filtering", "各向异性过滤", "Off"},
            {"纹理", "ppsspp_lower_resolution_for_effects", "降低特效分辨率", "Off"},
            {"纹理", "ppsspp_texture_deposterize", "去色带", "disabled"},
            {"纹理", "ppsspp_texture_scaling_type", "纹理缩放算法", "xbrz"},
            {"纹理", "ppsspp_texture_scaling_level", "纹理缩放倍率", "1"},
            {"纹理", "ppsspp_texture_replacement", "纹理替换", "enabled"},
            {"输入", "ppsspp_analog_is_circular", "圆形模拟摇杆", "disabled"},
            {"输入", "ppsspp_analog_deadzone", "摇杆死区", "0.15"},
            {"输入", "ppsspp_analog_sensitivity", "摇杆灵敏度", "1.10"},
            {"显示", "ppsspp_cropto16x9", "裁切至 16:9", "disabled"},
            {"高级", "ppsspp_block_transfer_gpu", "GPU 块传输", "enabled"},
            {"高级", "ppsspp_disable_range_culling", "禁用范围剔除", "disabled"},
        });

        m_coreItems.push_back(_section(L("按键")));
        m_coreItems.push_back(_action(
            L("PSP 按键映射"), L("配置 PPSSPP 外部核心使用的 config.cfg 映射"), 0xE30F,
            []() { return std::string(L("进入配置  >")); },
            [this]() { _openMappingPage(L("PSP 按键映射"), "psp.", false); }));
        _finishCorePage(L("PPSSPP 核心设置"));
    }

    void _openDuckStationCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("外部核心")));
        _appendExternalCorePaths(
            "DuckStation", "ps1.externalNro.path", "/GBAStation/core/GBAStationDuckStationStub.nro",
            "ps1.externalNro.returnPath");

        m_coreItems.push_back(_section(L("画面与启动")));
        const std::vector<std::string> resolutionValues = {"1", "2", "3", "4"};
        m_coreItems.push_back(_selector(
            L("内部渲染分辨率"), L("倍率越高画面越清晰，但性能开销也越大"), 0xE8FF,
            {"1x", "2x", "3x", "4x"},
            [resolutionValues]() {
                return findIndex(resolutionValues, std::to_string(cfgGetInt("ps1.resolutionScale", 1)));
            },
            [resolutionValues](int i) {
                if (i >= 0 && i < static_cast<int>(resolutionValues.size()))
                    cfgSetInt("ps1.resolutionScale", std::stoi(resolutionValues[static_cast<size_t>(i)]));
            }, "ps1.resolutionScale"));
        const std::vector<std::string> aspectValues = {
            "Auto (Game Native)", "4:3", "16:9", "Stretch To Fill"};
        m_coreItems.push_back(_selector(
            L("画面比例"), L("按游戏原始比例显示，或选择固定拉伸比例"), 0xE3F4,
            {L("自动（游戏原始比例）"), "4:3", "16:9", L("拉伸填满")},
            [aspectValues]() { return findIndex(aspectValues, cfgGetStr("ps1.aspectRatio", "Auto (Game Native)")); },
            [aspectValues](int i) {
                if (i >= 0 && i < static_cast<int>(aspectValues.size()))
                    cfgSetStr("ps1.aspectRatio", aspectValues[static_cast<size_t>(i)]);
            }, "ps1.aspectRatio"));
        m_coreItems.push_back(_toggle(
            L("快速启动"), L("跳过 PlayStation BIOS 动画，关闭可获得更接近原机的启动过程"), 0xE8B5,
            []() { return cfgGetBool("ps1.fastBoot", true); },
            [](bool value) { cfgSetBool("ps1.fastBoot", value); }, "ps1.fastBoot"));

        m_coreItems.push_back(_section(L("按键")));
        m_coreItems.push_back(_action(
            L("PS1 按键映射"), L("配置 DuckStation 外部核心使用的 config.cfg 映射"), 0xE30F,
            []() { return std::string(L("进入配置  >")); },
            [this]() { _openMappingPage(L("PS1 按键映射"), "ps1.", false); }));
        _finishCorePage(L("DuckStation 核心设置"));
    }

    void _openYabaSanshiroCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("外部核心")));
        _appendExternalCorePaths(
            "YabaSanshiro", "saturn.externalNro.path", "/GBAStation/core/GBAStationYabaSanshiroStub.nro",
            "saturn.externalNro.returnPath");
        m_coreItems.push_back(_section(L("系统")));
        m_coreItems.push_back(_toggle(
            L("使用 HLE BIOS"), L("没有 Saturn BIOS 文件时使用内置高层模拟，兼容性较低"), 0xE8B5,
            []() { return cfgGetBool("core.saturn.emulated_bios", false); },
            [](bool value) { cfgSetBool("core.saturn.emulated_bios", value); }, "core.saturn.emulated_bios"));
        m_coreItems.push_back(_selector(
            L("渲染分辨率"), L("原生分辨率最稳定；更高倍率会增加 GPU 负担"), 0xE8FF,
            {L("原生"), "4x", "2x", L("原始输出")},
            []() { return std::clamp(cfgGetInt("core.saturn.resolution_mode", 0), 0, 3); },
            [](int value) { cfgSetInt("core.saturn.resolution_mode", std::clamp(value, 0, 3)); }, "core.saturn.resolution_mode"));
        m_coreItems.push_back(_section(L("按键")));
        m_coreItems.push_back(_action(
            L("Saturn 按键映射"), L("Switch A/B/X/Y/L/R/ZL/ZR 对应 Saturn 六键手柄"), 0xE30F,
            []() { return std::string(L("进入配置  >")); },
            [this]() { _openMappingPage(L("Saturn 按键映射"), "saturn.", false); }));
        _finishCorePage(L("YabaSanshiro 核心设置"));
    }

    void _openDolphinCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("外部核心")));
        _appendExternalCorePaths(
            "Dolphin", "dolphin.externalNro.path", "/GBAStation/core/GBAStationDolphinStub.nro",
            "dolphin.externalNro.returnPath");
        m_coreItems.push_back(_section(L("GameCube / Wii")));
        m_coreItems.push_back(_toggle(
            L("宽屏"), L("为支持的 GameCube 和 Wii 游戏启用宽屏补丁"), 0xE3F4,
            []() { return cfgGetStr("core.dolphin.dolphin_widescreen", "enabled") == "enabled"; },
            [](bool value) { cfgSetStr("core.dolphin.dolphin_widescreen", value ? "enabled" : "disabled"); },
            "core.dolphin.dolphin_widescreen"));
        m_coreItems.push_back(_toggle(
            L("手柄震动"), L("启用 GameCube / Wii Classic Controller 震动"), 0xE8B8,
            []() { return cfgGetStr("core.dolphin.dolphin_enable_rumble", "enabled") == "enabled"; },
            [](bool value) { cfgSetStr("core.dolphin.dolphin_enable_rumble", value ? "enabled" : "disabled"); },
            "core.dolphin.dolphin_enable_rumble"));
        m_coreItems.push_back(_selector(
            L("Wii 控制器"), L("第一阶段使用 Classic Controller 模拟，不启用体感"), 0xE30F,
            {L("Classic Controller")}, []() { return 0; }, [](int) {}, "core.dolphin.dolphin_wiimote1_mode"));
        m_coreItems.push_back(_section(L("按键")));
        m_coreItems.push_back(_action(
            L("GC / Wii 按键映射"), L("Dolphin 会根据游戏自动选择 GameCube 或 Wii Classic Controller"), 0xE30F,
            []() { return std::string(L("进入配置  >")); },
            [this]() { _openMappingPage(L("GC / Wii 按键映射"), "dolphin.", false); }));
        _finishCorePage(L("Dolphin 核心设置"));
    }

    void _openGenesisCore()
    {
        m_coreItems.clear();
        m_coreItems.push_back(_section(L("系统与兼容性")));
        const std::vector<std::string> regionValues = {"auto", "ntsc-u", "pal", "ntsc-j"};
        m_coreItems.push_back(_selector(
            L("主机区域"), L("自动识别失败时可强制游戏区域，重新启动游戏后生效"), 0xE8B5,
            {L("自动识别"), L("NTSC-U（美版）"), L("PAL（欧版）"), L("NTSC-J（日版）")},
            [regionValues]() {
                return findIndex(regionValues, cfgGetStr("core.genesis.region", "auto"));
            },
            [regionValues](int i) {
                if (i >= 0 && i < static_cast<int>(regionValues.size()))
                    cfgSetStr("core.genesis.region", regionValues[static_cast<size_t>(i)]);
            }, "core.genesis.region"));
        m_coreItems.push_back(_selector(
            L("手柄类型"), L("六键模式支持 X、Y、Z 与 Mode，重新启动游戏后生效"), 0xE30F,
            {L("三键手柄"), L("六键手柄")},
            []() { return cfgGetInt("core.genesis.pad_buttons", 6) == 3 ? 0 : 1; },
            [](int i) { cfgSetInt("core.genesis.pad_buttons", i == 0 ? 3 : 6); }, "core.genesis.pad_buttons"));
        m_coreItems.push_back(_toggle(
            L("移除精灵限制"), L("减少横向精灵过多时的闪烁，可能改变原始硬件表现"), 0xE8EF,
            []() { return cfgGetStr("core.genesis.no_sprite_limit", "disabled") == "enabled"; },
            [](bool v) { cfgSetStr("core.genesis.no_sprite_limit", v ? "enabled" : "disabled"); }, "core.genesis.no_sprite_limit"));

        m_coreItems.push_back(_section(L("音频")));
        m_coreItems.push_back(_toggle(
            L("低通滤波"), L("模拟 Mega Drive 原机的柔和音色"), 0xE050,
            []() { return cfgGetStr("core.genesis.low_pass", "enabled") == "enabled"; },
            [](bool v) { cfgSetStr("core.genesis.low_pass", v ? "enabled" : "disabled"); }, "core.genesis.low_pass"));
        const std::vector<int> lowPassValues = {20, 40, 60, 80, 100};
        m_coreItems.push_back(_selector(
            L("低通滤波强度"), L("数值越高，保留的高频越多"), 0xE8E5,
            {"20%", "40%", "60%", "80%", "100%"},
            [lowPassValues]() {
                const int current = cfgGetInt("core.genesis.low_pass_range", 60);
                for (int i = 0; i < static_cast<int>(lowPassValues.size()); ++i)
                    if (lowPassValues[static_cast<size_t>(i)] == current) return i;
                return 2;
            },
            [lowPassValues](int i) {
                if (i >= 0 && i < static_cast<int>(lowPassValues.size()))
                    cfgSetInt("core.genesis.low_pass_range", lowPassValues[static_cast<size_t>(i)]);
            }, "core.genesis.low_pass_range"));
        m_coreItems.push_back(_toggle(
            L("高质量 FM 重采样"), L("提高 YM2612 音频质量，低性能设备可关闭"), 0xE050,
            []() { return cfgGetStr("core.genesis.hq_fm", "enabled") == "enabled"; },
            [](bool v) { cfgSetStr("core.genesis.hq_fm", v ? "enabled" : "disabled"); }, "core.genesis.hq_fm"));
        m_coreItems.push_back(_toggle(
            L("高质量 PSG 重采样"), L("提高 PSG 方波音频质量，低性能设备可关闭"), 0xE050,
            []() { return cfgGetStr("core.genesis.hq_psg", "enabled") == "enabled"; },
            [](bool v) { cfgSetStr("core.genesis.hq_psg", v ? "enabled" : "disabled"); }, "core.genesis.hq_psg"));
        m_coreItems.push_back(_toggle(
            L("单声道输出"), L("将左右声道混合为单声道"), 0xE04F,
            []() { return cfgGetStr("core.genesis.mono", "disabled") == "enabled"; },
            [](bool v) { cfgSetStr("core.genesis.mono", v ? "enabled" : "disabled"); }, "core.genesis.mono"));
        _finishCorePage(L("Genesis Plus GX 核心设置"));
    }

    void _openThreeDsTextInput(const std::string& title, const std::string& key,
                               const std::string& hint, int maxLength)
    {
        auto* ime = brls::Application::getPlatform()->getImeManager();
        if (!ime) {
            brls::Application::notify(L("当前平台不支持文本输入"));
            return;
        }
        ime->openForText([this, key](std::string text) {
            cfgSetStr(key, text);
            invalidate();
        }, title, hint, maxLength, cfgGetStr(key, ""),
            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
    }

    void _openThreeDsCore()
    {
        const auto key = [](const char* name) { return std::string("core.azahar.") + name; };

        m_coreItems.clear();
        m_coreItems.push_back(_section(L("系统")));
        m_coreItems.push_back(_selector(L("主机型号"), L("New 3DS 兼容更多游戏，Old 3DS 更接近旧机型"), 0xE30F,
            {"Old 3DS", "New 3DS"},
            [key]() { return cfgGetBool(key("new_3ds"), true) ? 1 : 0; },
            [key](int i) { cfgSetBool(key("new_3ds"), i == 1); }, key("new_3ds")));
        std::vector<int> cpuValues;
        std::vector<std::string> cpuLabels;
        cpuValues.reserve(159);
        cpuLabels.reserve(159);
        for (int value = 10; value <= 800; value += 5) {
            cpuValues.push_back(value);
            cpuLabels.push_back(std::to_string(value) + "%");
        }
        m_coreItems.push_back(_selector(L("CPU 时钟"), L("提高可改善部分游戏速度，也可能引入兼容性问题"), 0xE8EF,
            cpuLabels,
            [key, cpuValues]() {
                const int cur = cfgGetInt(key("cpu_clock"), 100);
                const int normalized = std::clamp(((cur + 2) / 5) * 5, 10, 800);
                return (normalized - 10) / 5;
            },
            [key, cpuValues](int i) {
                if (i >= 0 && i < static_cast<int>(cpuValues.size()))
                    cfgSetInt(key("cpu_clock"), cpuValues[static_cast<size_t>(i)]);
            }, key("cpu_clock")));
        m_coreItems.push_back(_toggle(
            L("打开菜单时暂停"), L("打开游戏内菜单时暂停模拟并静音"),
            0xE034,
            [key]() { return cfgGetBool(key("pause_when_menu_open"), true); },
            [key](bool v) { cfgSetBool(key("pause_when_menu_open"), v); }, key("pause_when_menu_open")));
        const std::vector<std::string> regionValues = {"auto", "japan", "usa", "europe", "australia", "china", "korea", "taiwan"};
        m_coreItems.push_back(_selector(L("系统区域"), L("覆盖 3DS 系统区域，自动适合大多数游戏"), 0xE8C4,
            {L("自动"), L("日本"), L("美国"), L("欧洲"), L("澳大利亚"), L("中国"), L("韩国"), L("台湾")},
            [key, regionValues]() { return findIndex(regionValues, cfgGetStr(key("region"), "auto")); },
            [key, regionValues](int i) { if (i >= 0 && i < static_cast<int>(regionValues.size())) cfgSetStr(key("region"), regionValues[static_cast<size_t>(i)]); }, key("region")));
        const std::vector<std::string> languageValues = {"", "japanese", "english", "french", "german", "italian", "spanish", "simplified_chinese", "korean", "dutch", "portuguese", "russian", "traditional_chinese"};
        m_coreItems.push_back(_selector(L("系统语言"), L("写入 3DS CFG 存档；留空表示不覆盖"), 0xE8C4,
            {L("不覆盖"), L("日语"), L("英语"), L("法语"), L("德语"), L("意大利语"), L("西班牙语"), L("简体中文"), L("韩语"), L("荷兰语"), L("葡萄牙语"), L("俄语"), L("繁体中文")},
            [key, languageValues]() { return findIndex(languageValues, cfgGetStr(key("language"), "")); },
            [key, languageValues](int i) { if (i >= 0 && i < static_cast<int>(languageValues.size())) cfgSetStr(key("language"), languageValues[static_cast<size_t>(i)]); }, key("language")));
        m_coreItems.push_back(_action(L("用户名"), L("写入 3DS CFG 存档，最多 10 个 UTF-16 字符"), 0xE7FD,
            [key]() { const auto name = cfgGetStr(key("username"), ""); return name.empty() ? L("不覆盖") : name; },
            [this, key]() { _openThreeDsTextInput(L("3DS 用户名"), key("username"), L("留空表示不覆盖"), 32); }));
        m_coreItems.push_back(_toggle(L("虚拟 SD 卡"), L("允许 3DS 软件访问模拟 SD 数据"), beiklive::material::STORAGE,
            [key]() { return cfgGetBool(key("use_virtual_sd"), true); },
            [key](bool v) { cfgSetBool(key("use_virtual_sd"), v); }, key("use_virtual_sd")));

        m_coreItems.push_back(_section(L("图形与性能")));
        m_coreItems.push_back(_toggle("CPU JIT", L("启用 CPU 动态重编译，通常必须开启以保证性能"), 0xE8EF,
            [key]() { return cfgGetBool(key("use_cpu_jit"), true); },
            [key](bool v) { cfgSetBool(key("use_cpu_jit"), v); }, key("use_cpu_jit")));
        // const std::vector<int> resValues = {0, 1, 2, 3, 4};
        // m_coreItems.push_back(_selector("内部分辨率", "0 为自动；更高倍率更清晰但负载更高", 0xE8FF,
        //     {"自动", "1x", "2x", "3x", "4x"},
        //     [key, resValues]() {
        //         const int cur = cfgGetInt(key("upscale"), 1);
        //         for (int i = 0; i < static_cast<int>(resValues.size()); ++i)
        //             if (resValues[static_cast<size_t>(i)] == cur) return i;
        //         return 1;
        //     },
        //     [key, resValues](int i) { if (i >= 0 && i < static_cast<int>(resValues.size())) cfgSetInt(key("upscale"), resValues[static_cast<size_t>(i)]); }));
        m_coreItems.push_back(_toggle(L("硬件着色器"), L("使用 GPU 模拟 3DS 着色器，通常性能更好"), 0xE3B7,
            [key]() { return cfgGetBool(key("use_hw_shader"), true); },
            [key](bool v) { cfgSetBool(key("use_hw_shader"), v); }, key("use_hw_shader")));
        m_coreItems.push_back(_toggle("Shader JIT", L("启用着色器 JIT 编译"), 0xE8EF,
            [key]() { return cfgGetBool(key("use_shader_jit"), true); },
            [key](bool v) { cfgSetBool(key("use_shader_jit"), v); }, key("use_shader_jit")));
        m_coreItems.push_back(_toggle(L("精确乘法"), L("提高着色器精度，可能降低性能"), 0xE3F4,
            [key]() { return cfgGetBool(key("accurate_mul"), true); },
            [key](bool v) { cfgSetBool(key("accurate_mul"), v); }, key("accurate_mul")));
        m_coreItems.push_back(_toggle(
            L("磁盘 Shader 缓存"), L("保存编译结果以减少后续卡顿"),
            beiklive::material::SAVE,
            [key]() { return cfgGetBool(key("disk_shader_cache"), true); },
            [key](bool v) { cfgSetBool(key("disk_shader_cache"), v); }, key("disk_shader_cache")));
        m_coreItems.push_back(_toggle(
            L("异步 GPU 模拟"), L("实验性并行 GPU 路径；默认关闭以保证稳定性能"),
            0xE8D5,
            [key]() { return cfgGetBool(key("async_gpu"), false); },
            [key](bool v) { cfgSetBool(key("async_gpu"), v); }, key("async_gpu")));
        m_coreItems.push_back(_toggle(
            L("严格 GPU 同步"), L("异步 GPU 开启时强制同步，用于兼容性排查"),
            0xE8D5,
            [key]() { return cfgGetBool(key("strict_gpu_sync"), false); },
            [key](bool v) { cfgSetBool(key("strict_gpu_sync"), v); }, key("strict_gpu_sync")));
        m_coreItems.push_back(_toggle(
            L("异步 Shader 编译"), L("减少运行时卡顿，少数游戏可能闪烁"), 0xE8D5,
            [key]() { return cfgGetBool(key("async_shaders"), true); },
            [key](bool v) { cfgSetBool(key("async_shaders"), v); }, key("async_shaders")));
        m_coreItems.push_back(_toggle(
            L("显示着色器编译文字"), L("在画面上显示后台着色器编译进度"), 0xE8D5,
            [key]() {
              return cfgGetBool(key("show_shader_compile_notice"), true);
            },
            [key](bool v) {
              cfgSetBool(key("show_shader_compile_notice"), v);
            }, key("show_shader_compile_notice")));
        m_coreItems.push_back(_toggle(
            L("异步呈现"), L("允许渲染和呈现解耦，通常可改善流畅度"), 0xE8D5,
            [key]() { return cfgGetBool(key("async_presentation"), true); },
            [key](bool v) { cfgSetBool(key("async_presentation"), v); }, key("async_presentation")));
        m_coreItems.push_back(_toggle(
            L("SPIR-V Shader 生成"), L("Vulkan 后端使用 SPIR-V 着色器路径"), 0xE3B7,
            [key]() { return cfgGetBool(key("spirv_shader_gen"), true); },
            [key](bool v) { cfgSetBool(key("spirv_shader_gen"), v); }, key("spirv_shader_gen")));
        m_coreItems.push_back(_toggle(
            L("禁用 SPIR-V 优化器"), L("保持当前 Switch NVK 更稳定的默认路径"),
            0xE868,
            [key]() {
              return cfgGetBool(key("disable_spirv_optimizer"), true);
            },
            [key](bool v) { cfgSetBool(key("disable_spirv_optimizer"), v); }, key("disable_spirv_optimizer")));
        m_coreItems.push_back(_toggle(L("垂直同步"), L("同步显示刷新，减少撕裂"), 0xE8B5,
            [key]() { return cfgGetBool(key("vsync"), true); },
            [key](bool v) { cfgSetBool(key("vsync"), v); }, key("vsync")));
        const std::vector<float> frameLimitValues = {0.f, 50.f, 75.f, 100.f, 150.f, 200.f};
        m_coreItems.push_back(_selector(L("帧率限制"), L("100% 为正常速度，0 表示不限制"), 0xE8E5,
            {L("不限制"), "50%", "75%", "100%", "150%", "200%"},
            [key, frameLimitValues]() {
                const float cur = GET_SETTING_KEY_FLOAT(key("frame_limit"), 100.f);
                int best = 3;
                float delta = std::fabs(cur - frameLimitValues[3]);
                for (int i = 0; i < static_cast<int>(frameLimitValues.size()); ++i) {
                    const float d = std::fabs(cur - frameLimitValues[static_cast<size_t>(i)]);
                    if (d < delta) { best = i; delta = d; }
                }
                return best;
            },
            [key, frameLimitValues](int i) { if (i >= 0 && i < static_cast<int>(frameLimitValues.size())) SET_SETTING_KEY_FLOAT(key("frame_limit"), frameLimitValues[static_cast<size_t>(i)]); }, key("frame_limit")));
        m_coreItems.push_back(_toggle(L("模拟 3DS GPU 时序"), L("更精确但更慢，调试兼容性时使用"), 0xE868,
            [key]() { return cfgGetBool(key("simulate_3ds_gpu_timings"), false); },
            [key](bool v) { cfgSetBool(key("simulate_3ds_gpu_timings"), v); }, key("simulate_3ds_gpu_timings")));
        m_coreItems.push_back(_toggle(L("禁用右眼渲染"), L("关闭立体 3D 的右眼画面以节省性能"), 0xE8A1,
            [key]() { return cfgGetBool(key("disable_right_eye"), true); },
            [key](bool v) { cfgSetBool(key("disable_right_eye"), v); }, key("disable_right_eye")));

        m_coreItems.push_back(_section(L("视频流")));
        m_coreItems.push_back(_toggle(
            L("视频 CPU 节流加速"), L("实验性：播放过场视频时临时降低 3DS Core Clock，可能改善部分游戏视频卡顿"),
            0xE8EF,
            [key]() { return cfgGetBool(key("movie_cpu_throttle"), true); },
            [key](bool v) { cfgSetBool(key("movie_cpu_throttle"), v); }, key("movie_cpu_throttle")));
        const std::vector<int> movieClockValues = {10, 25, 40, 50, 75, 100};
        m_coreItems.push_back(_selector(L("视频节流时钟"), L("视频加速开启时使用的 3DS Core Clock 百分比"),
            0xE8E5,
            {"10%", "25%", "40%", "50%", "75%", "100%"},
            [key, movieClockValues]() {
                const int cur = cfgGetInt(key("movie_throttle_clock"), 50);
                int best = 3;
                int delta = std::abs(cur - movieClockValues[3]);
                for (int i = 0; i < static_cast<int>(movieClockValues.size()); ++i) {
                    const int d = std::abs(cur - movieClockValues[static_cast<size_t>(i)]);
                    if (d < delta) {
                        best = i;
                        delta = d;
                    }
                }
                return best;
            },
            [key, movieClockValues](int i) {
                if (i >= 0 && i < static_cast<int>(movieClockValues.size()))
                    cfgSetInt(key("movie_throttle_clock"), movieClockValues[static_cast<size_t>(i)]);
            }, key("movie_throttle_clock")));

        m_coreItems.push_back(_section(L("纹理")));
        const std::vector<std::string> filterValues = {"none", "anime4k", "bicubic", "scaleforce", "xbrz", "mmpx"};
        m_coreItems.push_back(_selector(L("纹理滤镜"), L("对游戏纹理做放大滤镜处理"), 0xE3F4,
            {L("关闭"), "Anime4K", "Bicubic", "ScaleForce", "xBRZ", "MMPX"},
            [key, filterValues]() { return findIndex(filterValues, cfgGetStr(key("texture_filter"), "none")); },
            [key, filterValues](int i) { if (i >= 0 && i < static_cast<int>(filterValues.size())) cfgSetStr(key("texture_filter"), filterValues[static_cast<size_t>(i)]); }, key("texture_filter")));
        const std::vector<std::string> samplingValues = {"game", "nearest", "linear"};
        m_coreItems.push_back(_selector(L("纹理采样"), L("控制纹理采样方式"), 0xE8A1,
            {L("跟随游戏"), "Nearest", "Linear"},
            [key, samplingValues]() { return findIndex(samplingValues, cfgGetStr(key("texture_sampling"), "game")); },
            [key, samplingValues](int i) { if (i >= 0 && i < static_cast<int>(samplingValues.size())) cfgSetStr(key("texture_sampling"), samplingValues[static_cast<size_t>(i)]); }, key("texture_sampling")));
        m_coreItems.push_back(_toggle(L("自定义纹理"), L("加载用户替换纹理包"), beiklive::material::IMAGE,
            [key]() { return cfgGetBool(key("custom_textures"), false); },
            [key](bool v) { cfgSetBool(key("custom_textures"), v); }, key("custom_textures")));
        m_coreItems.push_back(_toggle(L("导出纹理"), L("运行时将游戏纹理导出到用户目录"), beiklive::material::SAVE,
            [key]() { return cfgGetBool(key("dump_textures"), false); },
            [key](bool v) { cfgSetBool(key("dump_textures"), v); }, key("dump_textures")));

        // m_coreItems.push_back(_section("屏幕布局"));
        // const std::vector<std::string> layoutValues = {"default", "single", "large", "large_inverted", "side", "hybrid", "hybrid_inverted"};
        // m_coreItems.push_back(_selector("屏幕布局", "选择上下屏排列方式", 0xE8A1,
        //     {"默认", "单屏", "大屏", "大屏反向", "左右并排", "混合", "混合反向"},
        //     [key, layoutValues]() { return findIndex(layoutValues, cfgGetStr(key("layout"), "default")); },
        //     [key, layoutValues](int i) { if (i >= 0 && i < static_cast<int>(layoutValues.size())) cfgSetStr(key("layout"), layoutValues[static_cast<size_t>(i)]); }));
        // const std::vector<std::string> posValues = {"top_right", "middle_right", "bottom_right", "top_left", "middle_left", "bottom_left", "above_large", "below_large"};
        // m_coreItems.push_back(_selector("小屏位置", "大屏/混合布局下的小屏位置", 0xE8A1,
        //     {"右上", "右中", "右下", "左上", "左中", "左下", "大屏上方", "大屏下方"},
        //     [key, posValues]() { return findIndex(posValues, cfgGetStr(key("small_screen_position"), "bottom_right"), 2); },
        //     [key, posValues](int i) { if (i >= 0 && i < static_cast<int>(posValues.size())) cfgSetStr(key("small_screen_position"), posValues[static_cast<size_t>(i)]); }));
        // const std::vector<std::string> orientationValues = {"horizontal", "vertical", "horizontal_inverted", "vertical_inverted"};
        // m_coreItems.push_back(_selector("屏幕方向", "横屏、竖屏及反向模式", 0xE8D5,
        //     {"横屏", "竖屏", "横屏反向", "竖屏反向"},
        //     [key, orientationValues]() { return findIndex(orientationValues, cfgGetStr(key("display_orientation"), "horizontal")); },
        //     [key, orientationValues](int i) { if (i >= 0 && i < static_cast<int>(orientationValues.size())) cfgSetStr(key("display_orientation"), orientationValues[static_cast<size_t>(i)]); }));
        // const std::vector<std::string> sizeValues = {"default", "stretch", "original", "integer"};
        // m_coreItems.push_back(_selector("显示尺寸", "默认、拉伸或整数缩放", 0xE8FF,
        //     {"默认", "拉伸", "原始", "整数倍"},
        //     [key, sizeValues]() { return findIndex(sizeValues, cfgGetStr(key("display_size"), "default")); },
        //     [key, sizeValues](int i) { if (i >= 0 && i < static_cast<int>(sizeValues.size())) cfgSetStr(key("display_size"), sizeValues[static_cast<size_t>(i)]); }));
        // m_coreItems.push_back(_toggle("交换上下屏", "将上屏与下屏互换", 0xE8D5,
        //     [key]() { return cfgGetBool(key("swap_screens"), false); },
        //     [key](bool v) { cfgSetBool(key("swap_screens"), v); }));
        // const std::vector<float> proportionValues = {2.f, 3.f, 4.f, 6.f, 8.f, 12.f, 16.f};
        // m_coreItems.push_back(_selector("大屏比例", "大屏布局中主屏相对副屏的比例", 0xE8FF,
        //     {"2x", "3x", "4x", "6x", "8x", "12x", "16x"},
        //     [key, proportionValues]() {
        //         const float cur = GET_SETTING_KEY_FLOAT(key("large_screen_proportion"), 4.f);
        //         int best = 2;
        //         float delta = std::fabs(cur - proportionValues[2]);
        //         for (int i = 0; i < static_cast<int>(proportionValues.size()); ++i) {
        //             const float d = std::fabs(cur - proportionValues[static_cast<size_t>(i)]);
        //             if (d < delta) { best = i; delta = d; }
        //         }
        //         return best;
        //     },
        //     [key, proportionValues](int i) { if (i >= 0 && i < static_cast<int>(proportionValues.size())) SET_SETTING_KEY_FLOAT(key("large_screen_proportion"), proportionValues[static_cast<size_t>(i)]); }));

        m_coreItems.push_back(_section(L("音频与输入")));
        const std::vector<std::string> audioValues = {"hle", "lle", "lle_multithreaded"};
        m_coreItems.push_back(_selector(L("音频模拟"), L("HLE 性能更好；LLE 更接近硬件"), 0xE050,
            {"HLE", "LLE", L("LLE 多线程")},
            [key, audioValues]() { return findIndex(audioValues, cfgGetStr(key("audio_emulation"), "hle")); },
            [key, audioValues](int i) { if (i >= 0 && i < static_cast<int>(audioValues.size())) cfgSetStr(key("audio_emulation"), audioValues[static_cast<size_t>(i)]); }, key("audio_emulation")));
        m_coreItems.push_back(_toggle(L("音频拉伸"), L("通过拉伸音频减少爆音，可能增加延迟"), 0xE8B5,
            [key]() { return cfgGetBool(key("audio_stretching"), false); },
            [key](bool v) { cfgSetBool(key("audio_stretching"), v); }, key("audio_stretching")));
        m_coreItems.push_back(_toggle(L("实时音频"), L("优先保持低延迟音频输出"), 0xE8B5,
            [key]() { return cfgGetBool(key("realtime_audio"), true); },
            [key](bool v) { cfgSetBool(key("realtime_audio"), v); }, key("realtime_audio")));
        const std::vector<std::string> micValues = {"null", "auto", "static_noise"};
        m_coreItems.push_back(_selector(L("麦克风输入"), L("3DS 麦克风模拟来源"), 0xE050,
            {L("关闭"), L("自动"), L("静态噪声")},
            [key, micValues]() { return findIndex(micValues, cfgGetStr(key("input_type"), "null")); },
            [key, micValues](int i) { if (i >= 0 && i < static_cast<int>(micValues.size())) cfgSetStr(key("input_type"), micValues[static_cast<size_t>(i)]); }, key("input_type")));

        m_coreItems.push_back(_section(L("调试")));
        m_coreItems.push_back(_toggle(L("渲染调试上下文"), L("启用 Vulkan/OpenGL 调试上下文，普通游玩建议关闭"), 0xE868,
            [key]() { return cfgGetBool(key("renderer_debug"), false); },
            [key](bool v) { cfgSetBool(key("renderer_debug"), v); }, key("renderer_debug")));
        m_coreItems.push_back(_toggle(L("导出命令缓冲"), L("输出底层渲染命令数据，调试时使用"), 0xE868,
            [key]() { return cfgGetBool(key("dump_command_buffers"), false); },
            [key](bool v) { cfgSetBool(key("dump_command_buffers"), v); }, key("dump_command_buffers")));

        _finishCorePage(L("Azahar 3DS 核心设置"));
    }

    void _openSteamApiDialog()
    {
        m_steamDialogChoice = 0;
        if (beiklive::steamgriddb::hasApiKey()) {
            m_steamDialog = SteamDialog::ApiModify;
            m_steamDialogTitle = "SteamGridDB API Key";
            m_steamDialogMessage =
                L("当前已经保存 API Key。是否重新输入并验证新的密钥？");
        } else {
            m_steamDialog = SteamDialog::ApiInfo;
            m_steamDialogTitle = L("获取 SteamGridDB API Key");
            m_steamDialogMessage =
                L("请登录 steamgriddb.com，进入个人设置中的 API 页面，创建并复制 API Key。\n")
                + L("密钥只保存在本机，不会写入游戏数据库。");
        }
    }

    void _openSteamApiIme()
    {
        auto* ime = brls::Application::getPlatform()->getImeManager();
        if (!ime) {
            m_steamDialog = SteamDialog::Result;
            m_steamDialogTitle = L("无法输入");
            m_steamDialogMessage = L("当前平台未提供文本输入服务。");
            m_steamDialogSuccess = false;
            return;
        }
        const std::string current = beiklive::steamgriddb::loadApiKey();
        ime->openForText([this, alive = m_aliveToken](std::string key) {
            if (!alive->load() || key.empty()) return;
            std::string error;
            if (!beiklive::steamgriddb::saveApiKey(key, &error)) {
                m_steamDialog = SteamDialog::Result;
                m_steamDialogTitle = L("保存失败");
                m_steamDialogMessage = error.empty() ? L("无法保存 API Key") : error;
                m_steamDialogSuccess = false;
                return;
            }
            _beginSteamApiValidation(key);
        }, "SteamGridDB API Key", L("请输入 API Key"), 256, current,
            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
    }

    void _beginSteamApiValidation(std::string key)
    {
        m_steamDialog = SteamDialog::Working;
        m_steamDialogTitle = L("正在验证 API Key");
        m_steamDialogMessage = L("正在连接 SteamGridDB 并尝试获取搜索数据...");
        auto alive = m_aliveToken;
        ThreadPool::instance().enqueuePriority([
            this, alive, key = std::move(key)]() {
            auto result = beiklive::steamgriddb::validateApiKey(key);
            brls::sync([this, alive, result = std::move(result)]() mutable {
                if (!alive->load()) return;
                m_steamDialog = SteamDialog::Result;
                m_steamDialogChoice = 0;
                m_steamDialogSuccess = result.ok;
                if (result.ok) {
                    m_steamDialogTitle = L("验证成功");
                    m_steamDialogMessage =
                        L("API Key 可以正常访问 SteamGridDB，封面搜索功能已经可用。");
                } else {
                    m_steamDialogTitle = result.networkError
                        ? L("网络异常") : L("API Key 无效");
                    m_steamDialogMessage = result.error.empty()
                        ? L("无法验证 API Key，请重新输入。") : result.error;
                }
                invalidate();
            });
        });
    }

    void _beginSteamCacheClear()
    {
        m_steamDialog = SteamDialog::ConfirmCacheClear;
        m_steamDialogChoice = 1;
        m_steamDialogTitle = L("确认清空缓存");
            m_steamDialogMessage =
                L("将删除 SteamGridDB 搜索结果和图片预览缓存。\n")
                + L("已经保存到游戏目录中的封面不会被删除。");
        m_steamDialogSuccess = false;
    }

    void _performSteamCacheClear()
    {
        m_steamDialog = SteamDialog::Working;
        m_steamDialogTitle = L("正在清空缓存");
        m_steamDialogMessage =
            L("正在删除 GBAStation/SteamGirdDB/cache 中的本地缓存...");
        auto alive = m_aliveToken;
        ThreadPool::instance().enqueuePriority([this, alive]() {
            std::string error;
            const bool ok = beiklive::steamgriddb::clearCache(&error);
            brls::sync([this, alive, ok, error = std::move(error)]() {
                if (!alive->load()) return;
                m_steamDialog = SteamDialog::Result;
                m_steamDialogChoice = 0;
                m_steamDialogSuccess = ok;
                m_steamDialogTitle = ok ? L("缓存已清空") : L("清理失败");
                m_steamDialogMessage = ok
                    ? L("SteamGridDB API 结果和图片预览缓存已经清理。")
                    : (error.empty() ? L("无法清理缓存目录。") : error);
                invalidate();
            });
        });
    }

    bool _handleSteamDialogActivate()
    {
        if (m_steamDialog == SteamDialog::None) return false;
        if (m_steamDialog == SteamDialog::Working) return true;
        if (m_steamDialog == SteamDialog::Result) {
            m_steamDialog = SteamDialog::None;
            return true;
        }
        if (m_steamDialog == SteamDialog::ConfirmCacheClear) {
            if (m_steamDialogChoice == 0)
                _performSteamCacheClear();
            else
                m_steamDialog = SteamDialog::None;
        } else if (m_steamDialogChoice == 0) {
            _openSteamApiIme();
        } else {
            m_steamDialog = SteamDialog::None;
        }
        return true;
    }

    void _buildMappingItems(const std::string& prefix, bool nds)
    {
        m_mappingItems.clear();

        if (prefix == "arcade.")
        {
            struct ArcadeBinding
            {
                std::string label;
                std::string suffix;
                std::string defaultValue;
            };
            static const ArcadeBinding arcadeBindings[] = {
                {L("方向 上"), "up", "PAD_UP"},
                {L("方向 下"), "down", "PAD_DOWN"},
                {L("方向 左"), "left", "PAD_LEFT"},
                {L("方向 右"), "right", "PAD_RIGHT"},
                {L("街机按钮 1"), "a", "PAD_A"},
                {L("街机按钮 2"), "b", "PAD_B"},
                {L("街机按钮 3"), "x", "PAD_X"},
                {L("街机按钮 4"), "y", "PAD_Y"},
                {L("街机按钮 5"), "l", "PAD_LB"},
                {L("街机按钮 6"), "r", "PAD_RB"},
                {L("街机按钮 7"), "l2", "PAD_LT"},
                {L("街机按钮 8"), "r2", "PAD_RT"},
                {L("投币"), "select", "PAD_BACK"},
                {L("开始"), "start", "PAD_START"},
            };

            m_mappingItems.push_back(_section(L("Arcade 游戏按键")));
            for (const auto& binding : arcadeBindings)
            {
                _addBinding(binding.label, L("直接映射到外部街机核心输入"),
                            beiklive::input_mapping::makeHandleKey(prefix, binding.suffix),
                            binding.defaultValue);
            }
            m_mappingItems.push_back(_section(L("Arcade 功能键")));
            _addBinding(L("打开菜单"), L("可绑定单键或双键组合"),
                        beiklive::input_mapping::makeKey(prefix, "hotkey.menu.pad"),
                        "PAD_LT+PAD_RT");
            _addBinding(L("快进"), L("可绑定单键或双键组合"),
                        beiklive::input_mapping::makeKey(prefix, "handle.fastforward"),
                        "PAD_LSB");
            _addBinding(L("倒带"), L("可绑定单键或双键组合"),
                        beiklive::input_mapping::makeKey(prefix, "handle.rewind"),
                        "PAD_RSB");
            // _addBinding(L("快速保存"), L("可绑定单键或双键组合"),
            //             beiklive::input_mapping::makeKey(prefix, "hotkey.quicksave.pad"),
            //             "none");
            // _addBinding(L("快速读取"), L("可绑定单键或双键组合"),
            //             beiklive::input_mapping::makeKey(prefix, "hotkey.quickload.pad"),
            //             "none");
            m_mappingFocus = _firstFocusable(m_mappingItems);
            m_mappingScroll = m_mappingTargetScroll = 0.f;
            return;
        }

        const unsigned mask = beiklive::input_mapping::platformMaskForPrefix(prefix);
        m_mappingItems.push_back(_section(L("游戏按键")));
        for (const auto& entry : beiklive::input_mapping::kGameButtonDefaults)
        {
            if ((entry.platformMask & mask) == 0) continue;
            _addBinding(beiklive::input_mapping::gameButtonLabelForPrefix(prefix, entry),
                        L("游戏内对应按键"),
                        beiklive::input_mapping::makeHandleKey(prefix, entry.suffix),
                        beiklive::input_mapping::defaultHandleValueForPrefix(
                            prefix, entry.suffix, entry.defaultValue));
        }
        m_mappingItems.push_back(_section(L("功能热键")));
        for (const auto& entry : beiklive::input_mapping::kHotkeyDefaults)
        {
            if (!beiklive::input_mapping::showsHotkeyForPrefix(prefix, entry, nds))
                continue;
            _addBinding(entry.label, L("可绑定单键或双键组合"), beiklive::input_mapping::makeKey(prefix, entry.key), entry.defaultValue);
        }
        const bool pointerHotkeys = nds || prefix == "3ds.";
        if (pointerHotkeys)
        {
            m_mappingItems.push_back(_section(nds ? L("NDS 指针与屏幕") : L("3DS 指针与屏幕")));
            for (const auto& entry : beiklive::input_mapping::kPointerHotkeys)
            {
                if ((nds && entry.hiddenOnNds) ||
                    (prefix == "3ds." && entry.hiddenOnThreeDs))
                    continue;
                _addBinding(entry.label, pointerHotkeyHint(entry.key),
                            beiklive::input_mapping::makeKey(prefix, entry.key),
                            entry.defaultValue);
            }
        }
        if (nds)
        {
            m_mappingItems.push_back(_section(L("NDS 特殊功能")));
            for (const auto& entry : beiklive::input_mapping::kNdsSpecialHotkeys)
            {
                _addBinding(entry.label, pointerHotkeyHint(entry.key),
                            beiklive::input_mapping::makeKey(prefix, entry.key),
                            entry.defaultValue);
            }
        }
        if (beiklive::input_mapping::showsTurboBindingsForPrefix(prefix))
        {
            m_mappingItems.push_back(_section(L("连发")));
            _addBinding(prefix == "md." ? L("MD C 连发") : L("A 连发"),
                        prefix == "md." ? L("按住时自动重复触发 MD C") : L("按住时自动重复触发 A"),
                        beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboAKey),
                        beiklive::input_mapping::kTurboADefault);
            _addBinding(prefix == "md." ? L("MD B 连发") : L("B 连发"),
                        prefix == "md." ? L("按住时自动重复触发 MD B") : L("按住时自动重复触发 B"),
                        beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboBKey),
                        beiklive::input_mapping::kTurboBDefault);
            const std::vector<float> rates = {1.f, 5.f, 10.f, 15.f, 30.f};
            m_mappingItems.push_back(_selector(L("连发速度"), L("每秒自动触发次数"), 0xE8E5,
                {L("每秒1次"), L("每秒5次"), L("每秒10次"), L("每秒15次"), L("每秒30次")},
                [rates]() { const float cur = GET_SETTING_KEY_FLOAT("turbo.rate", 10.f); for (int i = 0; i < 5; ++i) if (std::fabs(cur - rates[i]) < 0.01f) return i; return 2; },
                [rates](int i) { if (i >= 0 && i < 5) SET_SETTING_KEY_FLOAT("turbo.rate", rates[i]); }));
        }
        m_mappingFocus = _firstFocusable(m_mappingItems);
        m_mappingScroll = m_mappingTargetScroll = 0.f;
    }

    void _addBinding(const std::string& title, const std::string& hint,
                     const std::string& key, const std::string& defaultValue)
    {
        NanoSettingItem item;
        item.kind = NanoSettingKind::Binding;
        item.title = title;
        item.hint = hint;
        item.icon = 0xE30F;
        item.configKey = key;
        item.defaultBinding = defaultValue;
        item.value = [key, defaultValue]() { return cfgGetStr(key, defaultValue); };
        m_mappingItems.push_back(std::move(item));
    }

    std::vector<NanoSettingItem>& _activeItems()
    {
        if (m_inMapping) return m_mappingItems;
        if (m_inCore) return m_coreItems;
        return m_categories[static_cast<size_t>(m_category)].items;
    }

    int& _activeFocus()
    {
        if (m_inMapping) return m_mappingFocus;
        if (m_inCore) return m_coreFocus;
        return m_focus[static_cast<size_t>(m_category)];
    }

    float& _activeScroll()
    {
        if (m_inMapping) return m_mappingScroll;
        if (m_inCore) return m_coreScroll;
        return m_scroll[static_cast<size_t>(m_category)];
    }

    float& _activeTargetScroll()
    {
        if (m_inMapping) return m_mappingTargetScroll;
        if (m_inCore) return m_coreTargetScroll;
        return m_targetScroll[static_cast<size_t>(m_category)];
    }

    static int _firstFocusable(const std::vector<NanoSettingItem>& items)
    {
        for (int i = 0; i < static_cast<int>(items.size()); ++i)
            if (items[static_cast<size_t>(i)].kind != NanoSettingKind::Section)
                return i;
        return 0;
    }

    void _normalizeFocus()
    {
        for (size_t i = 0; i < m_categories.size(); ++i)
            m_focus[i] = _firstFocusable(m_categories[i].items);
    }

    void _move(int direction)
    {
        if (m_closing) return;
        if (m_steamDialog != SteamDialog::None) {
            if (m_steamDialog == SteamDialog::ApiInfo ||
                m_steamDialog == SteamDialog::ApiModify ||
                m_steamDialog == SteamDialog::ConfirmCacheClear) {
                m_steamDialogChoice = (m_steamDialogChoice +
                    (direction < 0 ? -1 : 1) + 2) % 2;
                brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
            }
            return;
        }
        const auto now = std::chrono::steady_clock::now();
        if (m_lastMoveAction.time_since_epoch().count() != 0
            && std::chrono::duration<float>(now - m_lastMoveAction).count() < 0.045f)
            return;
        m_lastMoveAction = now;
        if (m_selectorOpen)
        {
            const int count = static_cast<int>(m_selectorOptions.size());
            if (count > 0)
                m_selectorIndex = (m_selectorIndex + (direction < 0 ? -1 : 1) + count) % count;
            brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
            return;
        }
        auto& items = _activeItems();
        if (items.empty()) return;
        int index = _activeFocus();
        for (int attempt = 0; attempt < static_cast<int>(items.size()); ++attempt)
        {
            index = (index + (direction < 0 ? -1 : 1) + static_cast<int>(items.size())) % static_cast<int>(items.size());
            if (items[static_cast<size_t>(index)].kind != NanoSettingKind::Section)
            {
                _activeFocus() = index;
                _ensureFocusedVisible();
                brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
                break;
            }
        }
    }

    void _adjust(int direction)
    {
        if (m_steamDialog == SteamDialog::ApiInfo ||
            m_steamDialog == SteamDialog::ApiModify ||
            m_steamDialog == SteamDialog::ConfirmCacheClear) {
            m_steamDialogChoice = (m_steamDialogChoice +
                (direction < 0 ? -1 : 1) + 2) % 2;
            brls::Application::getAudioPlayer()->play(
                brls::SOUND_FOCUS_CHANGE);
            invalidate();
        }
    }

    void _switchCategory(int direction)
    {
        if (m_closing || m_selectorOpen || m_steamDialog != SteamDialog::None ||
            m_categoryMotion < 0.68f) return;
        if (m_inMapping)
        {
            m_inMapping = false;
            m_mappingItems.clear();
        }
        if (m_inCore) { m_inCore = false; m_coreItems.clear(); }
        m_category = (m_category + (direction < 0 ? -1 : 1) + static_cast<int>(m_categories.size())) % static_cast<int>(m_categories.size());
        m_categoryDirection = direction < 0 ? -1 : 1;
        m_categoryMotion = 0.f;
        m_contentEntrance = 0.f;
        _ensureFocusedVisible();
        brls::Application::getAudioPlayer()->play(brls::SOUND_FOCUS_CHANGE);
    }

    void _activate()
    {
        if (m_closing) return;
        if (_handleSteamDialogActivate()) {
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
            invalidate();
            return;
        }
        if (m_selectorOpen)
        {
            if (m_selectorApply) m_selectorApply(m_selectorIndex);
            m_selectorOpen = false;
            brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
            return;
        }
        auto& items = _activeItems();
        if (items.empty()) return;
        const int activeIndex = _activeFocus();
        auto& item = items[static_cast<size_t>(activeIndex)];
        m_pressedItem = activeIndex;
        m_pressedInMapping = m_inMapping;
        m_pressMotion = 1.f;
        switch (item.kind)
        {
        case NanoSettingKind::Toggle:
        case NanoSettingKind::Action:
            if (item.activate) item.activate();
            break;
        case NanoSettingKind::Selector:
            m_selectorTitle = item.title;
            m_selectorOptions = item.options;
            m_selectorIndex = item.selectedOption ? item.selectedOption() : 0;
            m_selectorIndex = std::clamp(m_selectorIndex, 0, std::max(0, static_cast<int>(m_selectorOptions.size()) - 1));
            m_selectorApply = item.applyOption;
            m_selectorOpen = true;
            break;
        case NanoSettingKind::Platform:
            m_mappingTitle = item.title;
            m_mappingPrefix = item.platformPrefix;
            m_mappingNds = item.nds;
            _buildMappingItems(m_mappingPrefix, m_mappingNds);
            m_inMapping = true;
            m_contentEntrance = 0.f;
            break;
        case NanoSettingKind::Binding:
        {
            const std::string key = item.configKey;
            openKeyCapture([this, key](const std::string& captured) {
                if (captured.empty()) return;
                std::string current = cfgGetStr(key, "none");
                if (current.empty() || current == "none") current = captured;
                else
                {
                    bool exists = false;
                    for (const auto& alternative : splitSettingText(current, '|'))
                        if (alternative == captured) { exists = true; break; }
                    if (!exists) current += "|" + captured;
                }
                cfgSetStr(key, current);
                invalidate();
            });
            break;
        }
        default:
            break;
        }
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        invalidate();
    }

    void _clearBinding()
    {
        if (!m_inMapping || m_selectorOpen || m_mappingItems.empty()) return;
        auto& item = m_mappingItems[static_cast<size_t>(m_mappingFocus)];
        if (item.kind != NanoSettingKind::Binding) return;
        cfgSetStr(item.configKey, "none");
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        invalidate();
    }

    void _resetCoreSetting()
    {
        if (!m_inCore || m_selectorOpen || m_coreItems.empty()) return;
        auto& item = m_coreItems[static_cast<size_t>(m_coreFocus)];
        if (!item.reset || !item.reset()) return;
        brls::Application::getAudioPlayer()->play(brls::SOUND_CLICK);
        invalidate();
    }

    void _back()
    {
        if (m_closing) return;
        if (m_steamDialog != SteamDialog::None) {
            if (m_steamDialog != SteamDialog::Working)
                m_steamDialog = SteamDialog::None;
            brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            return;
        }
        if (m_selectorOpen)
        {
            m_selectorOpen = false;
            brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            return;
        }
        if (m_inMapping)
        {
            m_inMapping = false;
            m_contentEntrance = 0.f;
            brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            return;
        }
        if (m_inCore)
        {
            m_inCore = false;
            m_coreItems.clear();
            m_contentEntrance = 0.f;
            brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
            return;
        }
        m_closing = true;
        brls::Application::getAudioPlayer()->play(brls::SOUND_BACK);
    }

    float _itemHeight(const NanoSettingItem& item) const
    {
        return item.kind == NanoSettingKind::Section ? 47.f : 76.f;
    }

    float _focusOffset() const
    {
        const auto& items = const_cast<NanoSettingsCanvas*>(this)->_activeItems();
        const int focus = m_inMapping ? m_mappingFocus : (m_inCore ? m_coreFocus : m_focus[static_cast<size_t>(m_category)]);
        float offset = 0.f;
        for (int i = 0; i < focus && i < static_cast<int>(items.size()); ++i)
            offset += _itemHeight(items[static_cast<size_t>(i)]) + 8.f;
        return offset;
    }

    float _contentHeight() const
    {
        const auto& items = const_cast<NanoSettingsCanvas*>(this)->_activeItems();
        float height = 18.f;
        for (const auto& item : items)
            height += _itemHeight(item) + 8.f;
        return height;
    }

    void _ensureFocusedVisible()
    {
        constexpr float viewport = 470.f;
        const float top = _focusOffset();
        const auto& items = const_cast<NanoSettingsCanvas*>(this)->_activeItems();
        const int focus = m_inMapping ? m_mappingFocus : (m_inCore ? m_coreFocus : m_focus[static_cast<size_t>(m_category)]);
        const float height = items.empty() ? 0.f : _itemHeight(items[static_cast<size_t>(focus)]);
        float& target = _activeTargetScroll();
        if (top < target + 18.f) target = std::max(0.f, top - 18.f);
        if (top + height > target + viewport - 18.f) target = top + height - viewport + 18.f;
        target = std::clamp(target, 0.f, std::max(0.f, _contentHeight() - viewport));
    }

    void _drawExternalShadow(NVGcontext* vg, const Rect& r, float radius, float alpha = 1.f)
    {
        const NVGpaint shadow = nvgBoxGradient(vg, r.x + 5.f, r.y + 6.f, r.w, r.h, radius, 5.f,
            nvgRGBA(0, 0, 0, settingAlpha(0.28f * alpha)), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, r.x - 3.f, r.y - 3.f, r.w + 16.f, r.h + 17.f);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, shadow);
        nvgFill(vg);
    }

    void _drawPanel(NVGcontext* vg, const Rect& r, float radius = 8.f, float fill = 0.035f)
    {
        _drawExternalShadow(vg, r, radius, 0.85f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x, r.y, r.w, r.h, radius);
        nvgFillColor(vg, settingPanelSubtle(fill));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, r.x + 1.f, r.y + 1.f, r.w - 2.f, r.h - 2.f, std::max(1.f, radius - 1.f));
        nvgStrokeColor(vg, settingBorder(0.18f));
        nvgStrokeWidth(vg, 1.5f);
        nvgStroke(vg);
    }

    void _drawHeader(NVGcontext* vg, float x, float y, float w)
    {
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 27.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, GET_THEME_COLOR("brls/text"));
        nvgText(vg, x + 36.f, y + 42.f, L("设置").c_str(), nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, settingSecondary(0.72f));
        const std::string subtitle = m_inMapping ? m_mappingTitle :
            (m_inCore ? m_coreTitle : L("模拟器、输入、游戏与系统选项"));
        nvgText(vg, x + 36.f, y + 70.f, subtitle.c_str(), nullptr);

        const float startX = x + 238.f;
        const float available = std::max(300.f, w - 274.f);
        const float segment = available / static_cast<float>(m_categories.size());
        const float centerY = y + 48.f;
        for (int i = 0; i < static_cast<int>(m_categories.size()); ++i)
        {
            const bool selected = i == m_category;
            const float centerX = startX + segment * (static_cast<float>(i) + 0.5f);
            const float pillW = std::min(142.f, segment - 7.f);
            const Rect pill{centerX - pillW * 0.5f, centerY - 22.f, pillW, 44.f};
            if (selected)
            {
                _drawExternalShadow(vg, pill, 22.f, 0.8f);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, pill.x, pill.y, pill.w, pill.h, 22.f);
                nvgFillColor(vg, settingPanelSubtle(0.14f));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, pill.x + 1.f, pill.y + 1.f, pill.w - 2.f, pill.h - 2.f, 21.f);
                nvgStrokeColor(vg, settingBorder(0.42f));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
            }
            const std::string icon = settingIconUtf8(m_categories[static_cast<size_t>(i)].icon);
            nvgFontFaceId(vg, m_materialFont);
            nvgFontSize(vg, selected ? 23.f : 20.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected ? settingText() : settingMuted(0.72f));
            nvgText(vg, centerX - 29.f, centerY, icon.c_str(), nullptr);
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, selected ? 21.f : 19.f);
            nvgText(vg, centerX + 13.f, centerY, m_categories[static_cast<size_t>(i)].title.c_str(), nullptr);
        }
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 36.f, y + 98.f);
        nvgLineTo(vg, x + w - 36.f, y + 98.f);
        nvgStrokeColor(vg, settingBorder(0.22f));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
    }

    void _drawContent(NVGcontext* vg, float x, float y, float w, float h)
    {
        auto& items = _activeItems();
        const int focus = _activeFocus();
        const float transition = settingBack(m_contentEntrance);
        const float categoryEase = settingSmooth(m_categoryMotion);
        const float slide = (1.f - categoryEase) * static_cast<float>(m_categoryDirection) * 82.f;
        float cursorY = y + 16.f - _activeScroll();
        nvgSave(vg);
        nvgIntersectScissor(vg, x + 2.f, y + 2.f, w - 4.f, h - 4.f);
        nvgGlobalAlpha(vg, settingSmooth(m_contentEntrance));
        nvgTranslate(vg, slide + (1.f - transition) * 30.f, 0.f);
        for (int i = 0; i < static_cast<int>(items.size()); ++i)
        {
            const auto& item = items[static_cast<size_t>(i)];
            const float itemH = _itemHeight(item);
            if (cursorY + itemH >= y - 20.f && cursorY <= y + h + 20.f)
            {
                if (item.kind == NanoSettingKind::Section)
                    _drawSection(vg, item, x + 24.f, cursorY, w - 48.f, itemH);
                else
                    _drawItem(vg, item, i, i == focus, x + 18.f, cursorY, w - 36.f, itemH);
            }
            cursorY += itemH + 8.f;
        }
        nvgRestore(vg);

        const float contentH = _contentHeight();
        if (contentH > h)
        {
            const float trackH = h - 32.f;
            const float thumbH = std::max(42.f, trackH * h / contentH);
            const float maxScroll = std::max(1.f, contentH - h);
            const float thumbY = y + 16.f + (_activeScroll() / maxScroll) * (trackH - thumbH);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + w - 8.f, thumbY, 3.f, thumbH, 1.5f);
        nvgFillColor(vg, settingMuted(0.60f));
            nvgFill(vg);
        }
    }

    void _drawSection(NVGcontext* vg, const NanoSettingItem& item,
                      float x, float y, float w, float h)
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y + 13.f, 4.f, 23.f, 2.f);
        nvgFillColor(vg, nvgRGBA(79, 193, 255, 230));
        nvgFill(vg);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 19.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, settingText(0.90f));
        nvgText(vg, x + 16.f, y + h * 0.5f, L(item.title).c_str(), nullptr);
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + 132.f, y + h * 0.5f);
        nvgLineTo(vg, x + w, y + h * 0.5f);
        nvgStrokeColor(vg, settingBorder(0.16f));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
    }

    void _drawItem(NVGcontext* vg, const NanoSettingItem& item, int itemIndex, bool focused,
                   float x, float y, float w, float h)
    {
        const bool pressed = m_pressMotion > 0.f && m_pressedItem == itemIndex
            && m_pressedInMapping == m_inMapping;
        const float pressScale = pressed ? 1.f - 0.035f * settingSmooth(m_pressMotion) : 1.f;
        nvgSave(vg);
        if (pressed)
        {
            nvgTranslate(vg, x + w * 0.5f, y + h * 0.5f);
            nvgScale(vg, pressScale, pressScale);
            nvgTranslate(vg, -(x + w * 0.5f), -(y + h * 0.5f));
        }
        _drawExternalShadow(vg, {x, y, w, h}, 7.f, focused ? 0.82f : 0.48f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y, w, h, 7.f);
        nvgFillColor(vg, focused
            ? nvgRGBA(79, 193, 255, pressed ? 42 : 25)
            : settingPanelSubtle(pressed ? 0.10f : 0.045f));
        nvgFill(vg);
        if (focused)
        {
            beiklive::ui::drawGradientFocusBorder(vg, x, y, w, h, 7.f, 3.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_time));
        }
        else
        {
            nvgBeginPath(vg);
            nvgRoundedRect(vg, x + 1.f, y + 1.f, w - 2.f, h - 2.f, 6.f);
            nvgStrokeColor(vg, settingBorder(0.14f));
            nvgStrokeWidth(vg, 1.f);
            nvgStroke(vg);
        }
        const Rect iconBox{x + 15.f, y + 14.f, 48.f, 48.f};
        nvgBeginPath(vg);
        nvgRoundedRect(vg, iconBox.x, iconBox.y, iconBox.w, iconBox.h, 7.f);
        nvgFillColor(vg, focused ? nvgRGBA(79, 193, 255, 38) : settingPanelSubtle(0.08f));
        nvgFill(vg);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 25.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, focused ? nvgRGBA(0, 102, 204, 255) : settingMuted(0.86f));
        const std::string icon = settingIconUtf8(item.icon);
        nvgText(vg, iconBox.x + iconBox.w * 0.5f, iconBox.y + iconBox.h * 0.5f, icon.c_str(), nullptr);

        nvgFontFaceId(vg, m_defaultFont);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFontSize(vg, focused ? 19.f : 18.f);
        nvgFillColor(vg, focused ? settingText() : settingText(0.86f));
        nvgText(vg, x + 79.f, y + (item.hint.empty() ? h * 0.5f : 28.f),
                L(item.title).c_str(), nullptr);
        if (!item.hint.empty())
        {
            nvgFontSize(vg, 13.5f);
            nvgFillColor(vg, settingSecondary(focused ? 0.82f : 0.64f));
            nvgText(vg, x + 79.f, y + 51.f, L(item.hint).c_str(), nullptr);
        }

        const std::string value = item.value ? item.value() : std::string();
        if (item.kind == NanoSettingKind::Binding)
            _drawBinding(vg, value, x + w - 24.f, y + h * 0.5f, focused);
        else if (item.kind == NanoSettingKind::Toggle)
            _drawToggle(vg, value == L("开启"), x + w - 78.f, y + h * 0.5f, focused);
        else
        {
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, focused ? 20.f : 18.f);
            nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, focused ? nvgRGBA(0, 102, 204, 255) : settingSecondary(0.88f));
            nvgText(vg, x + w - 39.f, y + h * 0.5f, L(value).c_str(), nullptr);
            nvgFontSize(vg, 22.f);
            nvgText(vg, x + w - 17.f, y + h * 0.5f, ">", nullptr);
        }
        nvgRestore(vg);
    }

    void _drawToggle(NVGcontext* vg, bool enabled, float x, float y, bool focused)
    {
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y - 14.f, 54.f, 28.f, 14.f);
        const NVGpaint toggleShadow = nvgBoxGradient(vg, x + 3.f, y - 10.f,
            54.f, 28.f, 14.f, 4.f, nvgRGBA(0, 0, 0, 58), nvgRGBA(0, 0, 0, 0));
        nvgBeginPath(vg);
        nvgRect(vg, x - 2.f, y - 17.f, 63.f, 38.f);
        nvgRoundedRect(vg, x, y - 14.f, 54.f, 28.f, 14.f);
        nvgPathWinding(vg, NVG_HOLE);
        nvgFillPaint(vg, toggleShadow);
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, x, y - 14.f, 54.f, 28.f, 14.f);
        nvgFillColor(vg, enabled ? nvgRGBA(0, 102, 204, focused ? 220 : 175) : settingPanelSubtle(0.14f));
        nvgFill(vg);
        const float knobX = enabled ? x + 40.f : x + 14.f;
        nvgBeginPath(vg);
        nvgCircle(vg, knobX, y, 10.f);
        nvgFillColor(vg, enabled ? nvgRGBA(255, 255, 255, 255) : settingMuted(0.82f));
        nvgFill(vg);
    }

    struct BindingVisual
    {
        std::string glyph;
        std::string suffix;
        bool switchGlyph = false;
    };

    BindingVisual _bindingToken(const std::string& token) const
    {
        struct ButtonToken { const char* token; brls::ControllerButton button; };
        static const ButtonToken buttons[] = {
            {"PAD_A", brls::BUTTON_A}, {"PAD_B", brls::BUTTON_B}, {"PAD_X", brls::BUTTON_X}, {"PAD_Y", brls::BUTTON_Y},
            {"PAD_LB", brls::BUTTON_LB}, {"PAD_RB", brls::BUTTON_RB},
            {"PAD_LSB", brls::BUTTON_LSB}, {"PAD_RSB", brls::BUTTON_RSB}, {"PAD_START", brls::BUTTON_START}, {"PAD_BACK", brls::BUTTON_BACK},
            {"PAD_UP", brls::BUTTON_UP}, {"PAD_DOWN", brls::BUTTON_DOWN}, {"PAD_LEFT", brls::BUTTON_LEFT}, {"PAD_RIGHT", brls::BUTTON_RIGHT},
        };
        for (const auto& button : buttons)
            if (token == button.token)
                return {brls::Hint::getKeyIcon(button.button), "", true};
        if (token == "PAD_LT") return {settingIconUtf8(0xE0A6), "", true};
        if (token == "PAD_RT") return {settingIconUtf8(0xE0A7), "", true};
        if (token == "PAD_LEFTSTICKUP") return {settingIconUtf8(0xE0C1), "↑", true};
        if (token == "PAD_LEFTSTICKDOWN") return {settingIconUtf8(0xE0C1), "↓", true};
        if (token == "PAD_LEFTSTICKLEFT") return {settingIconUtf8(0xE0C1), "←", true};
        if (token == "PAD_LEFTSTICKRIGHT") return {settingIconUtf8(0xE0C1), "→", true};
        if (token == "PAD_RIGHTSTICKUP") return {settingIconUtf8(0xE0C2), "↑", true};
        if (token == "PAD_RIGHTSTICKDOWN") return {settingIconUtf8(0xE0C2), "↓", true};
        if (token == "PAD_RIGHTSTICKLEFT") return {settingIconUtf8(0xE0C2), "←", true};
        if (token == "PAD_RIGHTSTICKRIGHT") return {settingIconUtf8(0xE0C2), "→", true};
        return {token, "", false};
    }

    void _drawBinding(NVGcontext* vg, const std::string& binding,
                      float right, float centerY, bool focused)
    {
        if (binding.empty() || binding == "none")
        {
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 15.f);
            nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, settingMuted(0.72f));
            nvgText(vg, right, centerY, L("未绑定").c_str(), nullptr);
            return;
        }
        const auto alternatives = splitSettingText(binding, '|');
        float cursor = right;
        for (auto alt = alternatives.rbegin(); alt != alternatives.rend(); ++alt)
        {
            const auto tokens = splitSettingText(*alt, '+');
            for (auto token = tokens.rbegin(); token != tokens.rend(); ++token)
            {
                const auto visual = _bindingToken(*token);
                nvgFontFaceId(vg, visual.switchGlyph ? m_switchFont : m_defaultFont);
                nvgFontSize(vg, visual.switchGlyph ? 24.f : 14.f);
                float bounds[4]{};
                nvgTextBounds(vg, 0.f, 0.f, visual.glyph.c_str(), nullptr, bounds);
                float suffixBounds[4]{};
                if (!visual.suffix.empty())
                {
                    nvgFontFaceId(vg, m_defaultFont);
                    nvgFontSize(vg, 18.f);
                    nvgTextBounds(vg, 0.f, 0.f, visual.suffix.c_str(), nullptr, suffixBounds);
                }
                const float chipW = std::max(34.f, bounds[2] - bounds[0]
                    + (visual.suffix.empty() ? 18.f : suffixBounds[2] - suffixBounds[0] + 25.f));
                cursor -= chipW;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, cursor, centerY - 17.f, chipW, 34.f, 17.f);
                nvgFillColor(vg, focused ? nvgRGBA(79, 193, 255, 42) : nvgRGBA(255, 255, 255, 18));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRoundedRect(vg, cursor + 1.f, centerY - 16.f, chipW - 2.f, 32.f, 16.f);
                nvgStrokeColor(vg, focused ? nvgRGBA(119, 211, 255, 150) : nvgRGBA(255, 255, 255, 42));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
                nvgFontFaceId(vg, visual.switchGlyph ? m_switchFont : m_defaultFont);
                nvgFontSize(vg, visual.switchGlyph ? 24.f : 14.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, focused ? settingText() : settingText(0.82f));
                const float glyphCenter = cursor + chipW * 0.5f
                    - (visual.suffix.empty() ? 0.f : 8.f);
                nvgText(vg, glyphCenter, centerY, visual.glyph.c_str(), nullptr);
                if (!visual.suffix.empty())
                {
                    nvgFontFaceId(vg, m_defaultFont);
                    nvgFontSize(vg, 18.f);
                    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
                    nvgText(vg, glyphCenter + 11.f, centerY, visual.suffix.c_str(), nullptr);
                }
                cursor -= 7.f;
            }
            if (alt + 1 != alternatives.rend())
            {
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 13.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, settingMuted(0.72f));
                cursor -= 17.f;
                nvgText(vg, cursor + 8.f, centerY, L("或").c_str(), nullptr);
                cursor -= 5.f;
            }
            if (cursor < right - 390.f) break;
        }
    }

    void _drawFooter(NVGcontext* vg, float x, float y, float w, float h)
    {
        float cursor = x + w - 32.f;
        const float hintY = y + h - 29.f;
        _drawHint(vg, brls::BUTTON_B,
                  m_selectorOpen ? L("取消").c_str() : (m_inMapping ? L("返回平台").c_str() : (m_inCore ? L("返回核心").c_str() : L("返回").c_str())),
                  cursor, hintY);
        if (m_inMapping && !m_selectorOpen && !m_mappingItems.empty()
            && m_mappingItems[static_cast<size_t>(m_mappingFocus)].kind == NanoSettingKind::Binding)
            _drawHint(vg, brls::BUTTON_X, L("清除绑定").c_str(), cursor, hintY);
        if (m_inCore && !m_selectorOpen && !m_coreItems.empty()
            && m_coreItems[static_cast<size_t>(m_coreFocus)].reset)
            _drawHint(vg, brls::BUTTON_BACK, L("恢复默认").c_str(), cursor, hintY);
        _drawHint(vg, brls::BUTTON_A, m_selectorOpen ? L("确认").c_str() : L("选择").c_str(), cursor, hintY);
        if (!m_inMapping && !m_inCore && !m_selectorOpen)
        {
            _drawHint(vg, brls::BUTTON_RB, L("下一类").c_str(), cursor, hintY);
            _drawHint(vg, brls::BUTTON_LB, L("上一类").c_str(), cursor, hintY);
        }
    }

    void _drawHint(NVGcontext* vg, brls::ControllerButton button,
                   const char* label, float& cursor, float y)
    {
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 17.f);
        float bounds[4]{};
        nvgTextBounds(vg, 0.f, 0.f, label, nullptr, bounds);
        const float width = bounds[2] - bounds[0];
        cursor -= width + 42.f;
        nvgFontFaceId(vg, m_switchFont);
        nvgFontSize(vg, 24.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, settingText(0.94f));
        const std::string glyph = brls::Hint::getKeyIcon(button);
        nvgText(vg, cursor + 13.f, y, glyph.c_str(), nullptr);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 17.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, settingText(0.88f));
        nvgText(vg, cursor + 29.f, y, label, nullptr);
        cursor -= 13.f;
    }

    void _drawSelector(NVGcontext* vg, float x, float y, float w, float h)
    {
        const float eased = settingBack(m_overlayMotion);
        nvgSave(vg);
        nvgGlobalAlpha(vg, settingSmooth(m_overlayMotion));
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 205));
        nvgFill(vg);
        const int count = static_cast<int>(m_selectorOptions.size());
        const int visible = std::min(7, std::max(1, count));
        const float panelW = std::min(620.f, w - 120.f);
        const float panelH = 104.f + visible * 54.f;
        const Rect panel{x + (w - panelW) * 0.5f,
                         y + (h - panelH) * 0.5f + (1.f - eased) * 42.f,
                         panelW, panelH};
        nvgTranslate(vg, x + w * 0.5f, y + h * 0.5f);
        const float scale = 0.90f + eased * 0.10f;
        nvgScale(vg, scale, scale);
        nvgTranslate(vg, -(x + w * 0.5f), -(y + h * 0.5f));
        _drawPanel(vg, panel, 8.f, 0.10f);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 23.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, settingText());
        nvgText(vg, panel.x + 28.f, panel.y + 36.f, L(m_selectorTitle).c_str(), nullptr);
        nvgFontSize(vg, 17.f);
        nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, settingSecondary(0.86f));
        const std::string counter = count > 0 ? std::to_string(m_selectorIndex + 1) + " / " + std::to_string(count) : "0 / 0";
        nvgText(vg, panel.x + panel.w - 28.f, panel.y + 36.f, counter.c_str(), nullptr);
        int first = std::max(0, m_selectorIndex - visible / 2);
        first = std::min(first, std::max(0, count - visible));
        for (int row = 0; row < visible && first + row < count; ++row)
        {
            const int index = first + row;
            const float rowY = panel.y + 72.f + row * 54.f;
            const bool selected = index == m_selectorIndex;
            const Rect optionRect{panel.x + 18.f, rowY, panel.w - 36.f, 46.f};
            _drawExternalShadow(vg, optionRect, 7.f, selected ? 0.70f : 0.30f);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, optionRect.x, optionRect.y, optionRect.w, optionRect.h, 7.f);
            nvgFillColor(vg, selected ? nvgRGBA(79, 193, 255, 34) : nvgRGBA(255, 255, 255, 5));
            nvgFill(vg);
            if (selected)
            {
                beiklive::ui::drawGradientFocusBorder(vg, panel.x + 18.f, rowY, panel.w - 36.f, 46.f,
                    7.f, 3.f, 1.f, beiklive::ui::gradientFocusAnimationOffset(m_time));
            }
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, selected ? 19.f : 17.f);
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, selected ? settingText() : settingMuted(0.82f));
            nvgText(vg, panel.x + 40.f, rowY + 23.f, L(m_selectorOptions[static_cast<size_t>(index)]).c_str(), nullptr);
            if (selected)
            {
                const std::string check = settingIconUtf8(0xE5CA);
                nvgFontFaceId(vg, m_materialFont);
                nvgFontSize(vg, 22.f);
                nvgTextAlign(vg, NVG_ALIGN_RIGHT | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, nvgRGBA(119, 211, 255, 255));
                nvgText(vg, panel.x + panel.w - 40.f, rowY + 23.f, check.c_str(), nullptr);
            }
        }
        nvgRestore(vg);
    }

    void _drawSteamDialog(NVGcontext* vg, float x, float y, float w, float h)
    {
        const float alpha = settingSmooth(m_steamOverlayMotion);
        const float eased = settingBack(m_steamOverlayMotion);
        nvgSave(vg);
        nvgGlobalAlpha(vg, alpha);
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 214));
        nvgFill(vg);
        const float panelW = 680.f;
        const float panelH = 360.f;
        const Rect panel{x + (w - panelW) * 0.5f,
                         y + (h - panelH) * 0.5f + (1.f - eased) * 38.f,
                         panelW, panelH};
        _drawExternalShadow(vg, panel, 18.f, 0.75f);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, panel.x, panel.y, panel.w, panel.h, 18.f);
        nvgFillColor(vg, settingPanelColor(0.98f));
        nvgFill(vg);
        nvgStrokeColor(vg, settingBorder(0.32f));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);

        char32_t iconCode = 0xE0DA;
        NVGcolor accent = nvgRGBA(92, 193, 255, 245);
        if (m_steamDialog == SteamDialog::Working) iconCode = 0xE863;
        else if (m_steamDialog == SteamDialog::ConfirmCacheClear) {
            iconCode = 0xE872;
            accent = nvgRGBA(255, 181, 92, 245);
        }
        else if (m_steamDialog == SteamDialog::Result) {
            iconCode = m_steamDialogSuccess ? 0xE86C : 0xE000;
            accent = m_steamDialogSuccess
                ? nvgRGBA(111, 207, 151, 245)
                : nvgRGBA(255, 111, 145, 245);
        }
        nvgBeginPath(vg);
        nvgRoundedRect(vg, panel.x + 30.f, panel.y + 28.f, 68.f, 68.f, 13.f);
        nvgFillColor(vg, nvgRGBA(accent.r * 255, accent.g * 255,
                                 accent.b * 255, 32));
        nvgFill(vg);
        nvgFontFaceId(vg, m_materialFont);
        nvgFontSize(vg, 40.f);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, accent);
        const std::string icon = settingIconUtf8(iconCode);
        nvgSave(vg);
        if (m_steamDialog == SteamDialog::Working) {
            const float cx = panel.x + 64.f;
            const float cy = panel.y + 62.f;
            nvgTranslate(vg, cx, cy);
            nvgRotate(vg, m_time * 2.8f);
            nvgTranslate(vg, -cx, -cy);
        }
        nvgText(vg, panel.x + 64.f, panel.y + 62.f, icon.c_str(), nullptr);
        nvgRestore(vg);

        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 26.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, settingText(0.96f));
        nvgText(vg, panel.x + 120.f, panel.y + 50.f,
                m_steamDialogTitle.c_str(), nullptr);
        nvgFontSize(vg, 15.f);
        nvgFillColor(vg, settingSecondary(0.78f));
        nvgText(vg, panel.x + 120.f, panel.y + 78.f,
                L("SteamGridDB 在线封面服务").c_str(), nullptr);
        nvgBeginPath(vg);
        nvgMoveTo(vg, panel.x + 30.f, panel.y + 116.f);
        nvgLineTo(vg, panel.x + panel.w - 30.f, panel.y + 116.f);
        nvgStrokeColor(vg, nvgRGBA(255, 255, 255, 28));
        nvgStrokeWidth(vg, 1.f);
        nvgStroke(vg);
        nvgFontFaceId(vg, m_defaultFont);
        nvgFontSize(vg, 18.f);
        nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(vg, settingSecondary(0.90f));
        nvgTextBox(vg, panel.x + 34.f, panel.y + 140.f,
                   panel.w - 68.f, m_steamDialogMessage.c_str(), nullptr);

        if (m_steamDialog == SteamDialog::ApiInfo ||
            m_steamDialog == SteamDialog::ApiModify ||
            m_steamDialog == SteamDialog::ConfirmCacheClear) {
            const std::string labels[2] = {
                m_steamDialog == SteamDialog::ConfirmCacheClear
                    ? L("确认清理")
                    : (m_steamDialog == SteamDialog::ApiInfo
                        ? L("我要输入") : L("修改密钥")),
                L("取消")};
            for (int i = 0; i < 2; ++i) {
                const float buttonW = 210.f;
                const float buttonX = panel.x + panel.w * 0.5f - 220.f + i * 230.f;
                const float buttonY = panel.y + panel.h - 76.f;
                const bool focused = i == m_steamDialogChoice;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, buttonX, buttonY, buttonW, 52.f, 10.f);
                nvgFillColor(vg, focused ? settingPanelSubtle(0.14f) : settingPanelSubtle(0.045f));
                nvgFill(vg);
                nvgStrokeColor(vg, settingBorder(focused ? 0.42f : 0.18f));
                nvgStrokeWidth(vg, 1.f);
                nvgStroke(vg);
                if (focused)
                    beiklive::ui::drawGradientFocusBorder(
                        vg, buttonX, buttonY, buttonW, 52.f, 10.f, 3.f, 1.f,
                        beiklive::ui::gradientFocusAnimationOffset(m_time));
                nvgFontFaceId(vg, m_defaultFont);
                nvgFontSize(vg, 20.f);
                nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
                nvgFillColor(vg, settingText(0.94f));
                nvgText(vg, buttonX + buttonW * 0.5f, buttonY + 26.f,
                        labels[i].c_str(), nullptr);
            }
        } else if (m_steamDialog == SteamDialog::Result) {
            const float buttonW = 220.f;
            const float buttonX = panel.x + (panel.w - buttonW) * 0.5f;
            const float buttonY = panel.y + panel.h - 76.f;
            nvgBeginPath(vg);
            nvgRoundedRect(vg, buttonX, buttonY, buttonW, 52.f, 10.f);
            nvgFillColor(vg, nvgRGBA(255, 255, 255, 34));
            nvgFill(vg);
            beiklive::ui::drawGradientFocusBorder(
                vg, buttonX, buttonY, buttonW, 52.f, 10.f, 3.f, 1.f,
                beiklive::ui::gradientFocusAnimationOffset(m_time));
            nvgFontFaceId(vg, m_defaultFont);
            nvgFontSize(vg, 20.f);
            nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, settingText(0.94f));
            nvgText(vg, buttonX + buttonW * 0.5f, buttonY + 26.f,
                    L("知道了").c_str(), nullptr);
        }
        nvgRestore(vg);
    }
};
}

// ─────────────────────────────────────────────────────────────────────────────
//  共享常量
// ─────────────────────────────────────────────────────────────────────────────

using namespace beiklive::SettingKey;

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 模拟器设置
// ─────────────────────────────────────────────────────────────────────────────

brls::View *SettingPage::buildUITab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── GBA/GBC 核心设置 ──────────────────────────────────────────────────────
    box->addView(makeHeader(L("mgba 核心设置")));

    {
        std::vector<std::string> gbModels = {
            "Autodetect", "Game Boy", "Super Game Boy", "Game Boy Color", "Game Boy Advance"};
        std::string curModel = cfgGetStr("core.mgba_gb_model", "Autodetect");
        auto *gbModelCell    = new brls::SelectorCell();
        gbModelCell->init(L("GB 机型"), gbModels, findIndex(gbModels, curModel),
                          [gbModels](int idx) { if (idx >= 0 && idx < 5) cfgSetStr("core.mgba_gb_model", gbModels[idx]); });
        box->addView(gbModelCell);
        box->addView(makeHint(L("Autodetect: 根据 ROM 头自动检测硬件型号")));
    }

    auto *biosCell = new brls::BooleanCell();
    biosCell->init(L("使用 BIOS"), cfgGetStr("core.mgba_use_bios", "ON") == "ON",
                   [](bool v) { cfgSetStr("core.mgba_use_bios", v ? "ON" : "OFF"); });
    box->addView(biosCell);
    box->addView(makeHint(L("开启 BIOS 后，之前在非 BIOS 模式下保存的即时存档可能会失效")));

    auto *skipBiosCell = new brls::BooleanCell();
    skipBiosCell->init(L("跳过 BIOS 动画"),
                       cfgGetStr("core.mgba_skip_bios", "OFF") == "ON",
                       [](bool v) { cfgSetStr("core.mgba_skip_bios", v ? "ON" : "OFF"); });
    box->addView(skipBiosCell);

    box->addView(makeHint(L("BIOS 文件请放入 GBAStation/bios 目录下（gba_bios.bin）")));

    {
        auto& gbColors = beiklive::GetGbColorPresets();
        std::string curGbColor = cfgGetStr("core.mgba_gb_colors", "Grayscale");
        auto *gbColorCell      = new brls::SelectorCell();
        gbColorCell->init(L("GB 配色"), gbColors, findIndex(gbColors, curGbColor),
                          [&gbColors](int idx) { if (idx >= 0 && idx < (int)gbColors.size()) cfgSetStr("core.mgba_gb_colors", gbColors[idx]); });
        box->addView(gbColorCell);
        box->addView(makeHint(L("为 GB/GBC 单色游戏着色，不影响 GBA 游戏")));
    }

    auto *sgbBorderCell = new brls::BooleanCell();
    sgbBorderCell->init(L("SGB 边框"), cfgGetStr("core.mgba_sgb_borders", "OFF") == "ON",
                        [](bool v) { cfgSetStr("core.mgba_sgb_borders", v ? "ON" : "OFF"); });
    box->addView(sgbBorderCell);
    box->addView(makeHint(L("如果GB GBC游戏显示在左上角，请关闭此选项")));

    {
        std::vector<std::string> rtcModes = {L("持久化 RTC"), L("跟随当前系统时间")};
        std::vector<std::string> rtcModeIds = {"persist", "system"};
        std::string curRtcMode = cfgGetStr("core.mgba_rtc_mode", "persist");
        auto* rtcModeCell = new brls::SelectorCell();
        rtcModeCell->init(L("RTC 时钟模式"), rtcModes, findIndex(rtcModeIds, curRtcMode),
                          [rtcModeIds](int idx) {
                              if (idx >= 0 && idx < static_cast<int>(rtcModeIds.size()))
                                  cfgSetStr("core.mgba_rtc_mode", rtcModeIds[idx]);
                          });
        box->addView(rtcModeCell);
        box->addView(makeHint(L("持久化 RTC：保留游戏内部时钟进度；跟随当前系统时间：每次启动时按当前设备时间校准")));
    }

    // ── 存档设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("存档设置")));

    {
        std::vector<std::string> saveDirs = {L("ROM 所在目录"), L("模拟器目录")};
        std::string curSram = cfgGetStr("save.sramDir", "");
        auto *sramDirCell = new brls::SelectorCell();
        sramDirCell->init(L("SRAM 存档目录"), saveDirs, curSram.empty() ? 0 : 1,
                          [](int idx) { cfgSetStr("save.sramDir", idx == 0 ? "" : beiklive::path::savePath()); });
        box->addView(sramDirCell);
    }

    auto *autoSaveCell = new brls::SelectorCell();
    {
        std::vector<std::string> slotOpts = {L("关闭"), L("档位0"), L("档位1"), L("档位2"), L("档位3"), L("档位4"), L("档位5"), L("档位6"), L("档位7"), L("档位8"), L("档位9")};
        int curSlot = GET_SETTING_KEY_INT("save.autoSaveState", 0);
        if (curSlot < 0 || curSlot > 10) curSlot = 0;
        autoSaveCell->init(L("自动保存游戏状态"), slotOpts, curSlot,
                           [](int i) { SET_SETTING_KEY_INT("save.autoSaveState", i); });
    }
    box->addView(autoSaveCell);

    {
        std::vector<std::string> intervals = {L("关闭"), L("1 分钟"), L("3 分钟"), L("5 分钟"), L("10 分钟")};
        static const int intervalVals[] = {0, 60, 180, 300, 600};
        int curVal = GET_SETTING_KEY_INT("save.autoSaveInterval", 0);
        int idx = 0;
        for (int i = 0; i < 5; ++i) if (intervalVals[i] == curVal) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("自动保存间隔"), intervals, idx,
                   [](int i) { if (i >= 0 && i < 5) SET_SETTING_KEY_INT("save.autoSaveInterval", intervalVals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("定时自动创建即时存档，防止意外丢失进度")));
    }

    auto *autoLoadCell = new brls::SelectorCell();
    {
        std::vector<std::string> slotOpts = {L("关闭"), L("档位0"), L("档位1"), L("档位2"), L("档位3"), L("档位4"), L("档位5"), L("档位6"), L("档位7"), L("档位8"), L("档位9")};
        int curSlot = GET_SETTING_KEY_INT("save.autoLoadState0", 0);
        if (curSlot < 0 || curSlot > 10) curSlot = 0;
        autoLoadCell->init(L("启动时自动加载"), slotOpts, curSlot,
                           [](int i) { SET_SETTING_KEY_INT("save.autoLoadState0", i); });
    }
    box->addView(autoLoadCell);

    {
        std::vector<std::string> exitSlotOpts = {L("关闭"), L("档位0"), L("档位1"), L("档位2"), L("档位3"), L("档位4"), L("档位5"), L("档位6"), L("档位7"), L("档位8"), L("档位9")};
        int curExitSlot = GET_SETTING_KEY_INT("save.autoSaveOnExit", 0);
        if (curExitSlot < 0 || curExitSlot > 10) curExitSlot = 0;
        auto *exitSaveCell = new brls::SelectorCell();
        exitSaveCell->init(L("退出游戏时自动保存"), exitSlotOpts, curExitSlot,
                           [](int i) { SET_SETTING_KEY_INT("save.autoSaveOnExit", i); });
        box->addView(exitSaveCell);
    }

    // ── 封面设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("封面设置")));

    {
        auto *thumbCell = new brls::BooleanCell();
        thumbCell->init(L("使用存档截图作为封面"),
                       cfgGetBool(beiklive::SettingKey::KEY_UI_USE_SAVESTATE_THUMB, false),
                       [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_UI_USE_SAVESTATE_THUMB, v); });
        box->addView(thumbCell);
        box->addView(makeHint(L("使用即时存档0截图作为封面，已自定义封面的游戏不覆盖")));
    }



    // ── 模拟器UI ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("模拟器UI")));

    {
        auto *shaderCell = new brls::BooleanCell();
        shaderCell->init(L("启用动态渐变背景"),
                        cfgGetBool(beiklive::SettingKey::KEY_UI_SHOW_SHADER, false),
                        [this](bool v) {
                            cfgSetBool(beiklive::SettingKey::KEY_UI_SHOW_SHADER, v);
                            this->showShader(v);
                        });
        box->addView(shaderCell);
    }

    {
        std::vector<std::string> themes = {L("深夜蓝"), L("柠檬黄"), L("牛油果绿"), L("草莓红"), L("海洋蓝"), L("樱花粉"), L("VSCode黑"), L("极光青"), L("皇家紫"), L("日落橙"), L("石墨灰"), L("云雾白")};
        std::vector<std::string> themeIds = {"Midnight", "LemonYellow", "AvocadoGreen", "StrawberryRed",
                                              "OceanBlue", "SakuraPink", "VscodeBlack", "AuroraTeal", "RoyalPurple", "SunsetOrange", "Graphite", "CloudWhite"};
        std::string curTheme = cfgGetStr(beiklive::SettingKey::KEY_UI_GRADIENT_THEME, "VscodeBlack");
        int curIdx = findIndex(themeIds, curTheme, 6);
        auto *themeCell = new brls::SelectorCell();
        themeCell->init(L("渐变主题"), themes, curIdx,
                       [this, themeIds](int idx) {
                           if (idx >= 0 && idx < (int)themeIds.size()) {
                               cfgSetStr(beiklive::SettingKey::KEY_UI_GRADIENT_THEME, themeIds[idx]);
                               this->setGradientTheme(gradientThemeFromId(themeIds[idx]));
                           }
                       });
        box->addView(themeCell);
    }

    // ── 背景图片设置 ──
    {
        auto *bgSwitch = new brls::BooleanCell();
        bgSwitch->init(L("启用背景图片"),
                      cfgGetBool(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, false),
                      [this](bool v) {
                          cfgSetBool(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, v);
                          this->showBackground(v);
                      });
        box->addView(bgSwitch);

        auto *bgPathCell = new brls::DetailCell();
        bgPathCell->setText(L("背景图片路径"));
        std::string curPath = cfgGetStr(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, "");
        bgPathCell->setDetailText(curPath.empty() ? L("未设置") : beiklive::tools::getFileName(curPath));
        bgPathCell->registerAction(L("选择"), brls::BUTTON_A,
            [this, bgPathCell](brls::View*) -> bool {
                std::filesystem::path currentPath(cfgGetStr(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, ""));
                beiklive::openFilePicker({"png", "gif", "mp4"},
                    [this, bgPathCell](const std::string& path) {
                        cfgSetStr(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, path);
                        bgPathCell->setDetailText(beiklive::tools::getFileName(path));
                        this->setBackgroundImage(path, true);
                    },
                    currentPath.parent_path().string(),
                    currentPath.filename().string());
                return true;
            });
        box->addView(bgPathCell);

        auto *gifSpeedCell = new brls::SelectorCell();
        static constexpr float gifSpeeds[] = {0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f};
        const float currentGifSpeed = GET_SETTING_KEY_FLOAT(
            beiklive::SettingKey::KEY_UI_BG_GIF_SPEED, 1.f);
        int gifSpeedIndex = 2;
        float gifSpeedDistance = std::fabs(currentGifSpeed - gifSpeeds[gifSpeedIndex]);
        for (int i = 0; i < 6; ++i) {
            const float distance = std::fabs(currentGifSpeed - gifSpeeds[i]);
            if (distance < gifSpeedDistance) { gifSpeedIndex = i; gifSpeedDistance = distance; }
        }
        gifSpeedCell->init(L("GIF播放速度"),
                           {"0.5x", "0.75x", "1.0x", "1.25x", "1.5x", "2.0x"},
                           gifSpeedIndex,
                           [](int index) {
                               if (index >= 0 && index < 6)
                                   SET_SETTING_KEY_FLOAT(
                                       beiklive::SettingKey::KEY_UI_BG_GIF_SPEED,
                                       gifSpeeds[index]);
                           });
        box->addView(gifSpeedCell);

        auto *videoFrameRateCell = new brls::SelectorCell();
        static constexpr int videoFrameRates[] = {30, 35, 40, 45, 50, 55, 60};
        const int currentVideoFrameRate = cfgGetInt(
            beiklive::SettingKey::KEY_UI_BG_VIDEO_FRAME_RATE, 30);
        int videoFrameRateIndex = 0;
        int videoFrameRateDistance = std::abs(currentVideoFrameRate - videoFrameRates[0]);
        for (int i = 1; i < 7; ++i) {
            const int distance = std::abs(currentVideoFrameRate - videoFrameRates[i]);
            if (distance < videoFrameRateDistance) {
                videoFrameRateIndex = i;
                videoFrameRateDistance = distance;
            }
        }
        videoFrameRateCell->init(L("MP4播放帧率"),
                                 {"30 FPS", "35 FPS", "40 FPS", "45 FPS", "50 FPS", "55 FPS", "60 FPS"},
                                 videoFrameRateIndex,
                                 [](int index) {
                                     if (index >= 0 && index < 7)
                                         cfgSetInt(beiklive::SettingKey::KEY_UI_BG_VIDEO_FRAME_RATE,
                                                   videoFrameRates[index]);
                                 });
        box->addView(videoFrameRateCell);

    }

    // 文件列表滚动动画
    {
        auto *scrollAnimCell = new brls::BooleanCell();
        scrollAnimCell->init(L("文件列表滚动动画"),
                            cfgGetBool(beiklive::SettingKey::KEY_FILE_LIST_SCROLL_ANIM, true),
                            [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_FILE_LIST_SCROLL_ANIM, v); });
        box->addView(scrollAnimCell);
        box->addView(makeHint(L("启用文件浏览器的平滑滚动效果，关闭后列表直接跳转")));
    }

    {
        auto* titleSizeCell = new brls::SelectorCell();
        titleSizeCell->init(L("游戏库标题字号"),
            {L("正常"), "大", L("超大")},
            cfgGetInt(beiklive::SettingKey::KEY_UI_LIBRARY_TITLE_SIZE, 0),
            [](int sel) { cfgSetInt(beiklive::SettingKey::KEY_UI_LIBRARY_TITLE_SIZE, sel); });
        box->addView(titleSizeCell);
    }
    box->addView(makeHint(L("设置游戏库网格列表中游戏标题的显示字号")));

    scroll->setContentView(box);
    auto *container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 游戏设置
// ─────────────────────────────────────────────────────────────────────────────

brls::View *SettingPage::buildGameTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── 快进设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("快进设置")));

    auto *ffEnabledCell = new brls::BooleanCell();
    ffEnabledCell->init("启用快进", cfgGetBool("fastforward.enabled", true),
                        [](bool v) { cfgSetBool("fastforward.enabled", v); });
    box->addView(ffEnabledCell);

    {
        std::vector<std::string> modes = {L("按住"), L("切换")};
        std::string curMode = cfgGetStr("fastforward.mode", "hold");
        auto *cell = new brls::SelectorCell();
        cell->init(L("触发模式"), modes, curMode == "toggle" ? 1 : 0,
                   [](int idx) { cfgSetStr("fastforward.mode", idx == 1 ? "toggle" : "hold"); });
        box->addView(cell);
        box->addView(makeHint(L("按住：长按快进键触发  |  切换：按一次永久保持")));
    }

    {
        std::vector<std::string> multis = {L("0.1倍"), L("0.5倍"), L("1倍"), L("1.25倍"), L("1.5倍"), L("1.75倍"), L("2倍"), L("3倍"), L("4倍"), L("5倍"), L("6倍"), L("7倍"), L("8倍"), L("9倍"), L("10倍")};
        static const float multiVals[] = {0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
        float curMulti = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 2.0f);
        int idx = 6;
        for (int i = 0; i < 15; ++i) if (multiVals[i] == curMulti) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("快进倍率"), multis, idx,
                   [](int i) { if (i >= 0 && i < 15) SET_SETTING_KEY_FLOAT("fastforward.multiplier", multiVals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("小于1倍时可实现慢动作效果，大于1倍用于快进加速")));
    }

    auto *ffMuteCell = new brls::BooleanCell();
    ffMuteCell->init("快进时静音", cfgGetBool("fastforward.mute", true),
                     [](bool v) { cfgSetBool("fastforward.mute", v); });
    box->addView(ffMuteCell);

    // ── 倒带设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("倒带设置")));

    auto *rewEnabledCell = new brls::BooleanCell();
    rewEnabledCell->init("启用倒带", cfgGetBool("rewind.enabled", false),
                         [](bool v) { cfgSetBool("rewind.enabled", v); });
    box->addView(rewEnabledCell);

    {
        std::vector<std::string> modes = {L("按住"), L("切换")};
        std::string curMode = cfgGetStr("rewind.mode", "hold");
        auto *cell = new brls::SelectorCell();
        cell->init(L("触发模式"), modes, curMode == "toggle" ? 1 : 0,
                   [](int idx) { cfgSetStr("rewind.mode", idx == 1 ? "toggle" : "hold"); });
        box->addView(cell);
        box->addView(makeHint(L("按住：长按倒带键触发  |  切换：按一次永久保持")));
    }

    auto *rewMuteCell = new brls::BooleanCell();
    rewMuteCell->init("倒带时静音", cfgGetBool("rewind.mute", false),
                      [](bool v) { cfgSetBool("rewind.mute", v); });
    box->addView(rewMuteCell);

    {
        std::vector<std::string> stepOpts = {L("1 帧"), L("2 帧"), L("4 帧"), L("8 帧")};
        static const int stepVals[] = {1, 2, 4, 8};
        int curStep = GET_SETTING_KEY_INT("rewind.step", 2);
        int idx = 1;
        for (int i = 0; i < 4; ++i) if (stepVals[i] == curStep) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("倒带步进"), stepOpts, idx,
                   [](int i) { if (i >= 0 && i < 4) SET_SETTING_KEY_INT("rewind.step", stepVals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("每次倒带操作回退的帧数，步进越小控制越精细")));
    }

    {
        bool showUI = cfgGetBool(beiklive::SettingKey::KEY_REWIND_SHOW_UI, false);
        auto *showUiCell = new brls::BooleanCell();
        showUiCell->init(L("显示可视化倒带界面"), showUI,
                         [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_REWIND_SHOW_UI, v); });
        box->addView(showUiCell);
    }

    {
        std::vector<std::string> intervalOpts = {L("每帧"), L("每2帧"), L("每4帧"), L("每8帧"), L("每16帧"), L("每60帧(~1秒)"), L("每120帧(~2秒)")};
        static const int intervalVals[] = {1, 2, 4, 8, 16, 60, 120};
        static const int intervalCount = 7;
        int curInterval = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SAVE_INTERVAL, 1);
        int curIdx = 0;
        for (int i = 0; i < intervalCount; ++i) if (intervalVals[i] == curInterval) { curIdx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("倒带保存间隔"), intervalOpts, curIdx,
                   [](int i) { if (i >= 0 && i < intervalCount) SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SAVE_INTERVAL, intervalVals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("间隔越短精度越高但内存占用越大")));
    }

    {
        std::vector<std::string> bufferOpts = {L("60 (~1秒)"), L("120 (~2秒)"), L("600 (~10秒)"), L("1800 (~30秒)")};
        static const int bufferVals[] = {60, 120, 600, 1800};
        int curBuffer = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_BUFFER_SIZE, 600);
        int curIdx = 2;
        for (int i = 0; i < 4; ++i) if (bufferVals[i] == curBuffer) { curIdx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("最大倒带缓存"), bufferOpts, curIdx,
                   [](int i) { if (i >= 0 && i < 4) SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_BUFFER_SIZE, bufferVals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("缓冲帧越多可回退时间越长，但内存占用成倍增加")));
    }

    {
        int curCompression = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_THUMB_COMPRESSION, 0);
        std::vector<std::string> compressionOpts = {L("最近邻（速度优先）"), L("双线性（质量优先）")};
        auto *cell = new brls::SelectorCell();
        cell->init(L("缩略图压缩策略"), compressionOpts, curCompression,
                   [](int idx) { if (idx >= 0 && idx <= 1) SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_THUMB_COMPRESSION, idx); });
        box->addView(cell);
        box->addView(makeHint(L("最近邻速度更快但锯齿明显，双线性更平滑但性能略低")));
    }

    scroll->setContentView(box);
    auto *container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 显示设置
// ─────────────────────────────────────────────────────────────────────────────

brls::View *SettingPage::buildDisplayTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── 画面显示 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("画面显示")));

    {
        std::vector<std::string> modes = {L("按比例 (Fit)"), L("拉伸 (Fill)"), L("原始 (Original)"), "4:3", L("整数倍 (Integer)"), L("自定义 (Custom)")};
        std::string curMode = cfgGetStr("display.mode", "original");
        std::vector<std::string> modeIds = {"fit", "fill", "original", "four_three", "integer", "custom"};
        int idx = findIndex(modeIds, curMode);
        auto *cell = new brls::SelectorCell();
        cell->init(L("画面模式"), modes, idx,
                   [](int i) {
                       static const char* vals[] = {"fit", "fill", "original", "four_three", "integer", "custom"};
                       if (i >= 0 && i < 6) cfgSetStr("display.mode", vals[i]);
                   });
        box->addView(cell);
        box->addView(makeHint(L("画面缩放模式：Fit=保持比例最大化 Fill=拉伸填满 4:3=按窗口高度等比换算宽度")));
    }

    {
        std::vector<std::string> scales = {L("自动"), L("1倍"), L("2倍"), L("3倍"), L("4倍"), L("5倍")};
        static const int scaleVals[] = {0, 1, 2, 3, 4, 5};
        int curScale = GET_SETTING_KEY_INT("display.integer_scale_mult", 0);
        int idx = 0;
        for (int i = 0; i < 6; ++i) if (scaleVals[i] == curScale) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("整数倍缩放"), scales, idx,
                   [](int i) { if (i >= 0 && i < 6) SET_SETTING_KEY_INT("display.integer_scale_mult", scaleVals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("画面模式为整数倍时生效，自动=取最大整数倍")));
    }

    {
        std::vector<std::string> filters = {L("像素风格 (Nearest)"), L("平滑 (Linear)")};
        std::string curFilter = cfgGetStr("display.filter", "nearest");
        int idx = (curFilter == "linear") ? 1 : 0;
        auto *cell = new brls::SelectorCell();
        cell->init(L("纹理过滤"), filters, idx,
                   [](int i) { cfgSetStr("display.filter", i == 1 ? "linear" : "nearest"); });
        box->addView(cell);
        box->addView(makeHint(L("Nearest 像素点阵风格（锐利）| Linear 平滑柔和（模糊）")));
    }

    auto *ffOverlayCell = new brls::BooleanCell();
    ffOverlayCell->init("显示快进覆盖层", cfgGetBool("display.showFfOverlay", true),
                         [](bool v) { cfgSetBool("display.showFfOverlay", v); });
    box->addView(ffOverlayCell);

    auto *rewOverlayCell = new brls::BooleanCell();
    rewOverlayCell->init("显示倒带覆盖层", cfgGetBool("display.showRewindOverlay", true),
                          [](bool v) { cfgSetBool("display.showRewindOverlay", v); });
    box->addView(rewOverlayCell);

    auto *muteOverlayCell = new brls::BooleanCell();
    muteOverlayCell->init("显示静音覆盖层", cfgGetBool("display.showMuteOverlay", true),
                           [](bool v) { cfgSetBool("display.showMuteOverlay", v); });
    box->addView(muteOverlayCell);

    {
        auto *fpsCell = new brls::BooleanCell();
        fpsCell->init("显示 FPS 覆盖层", cfgGetBool("display.showFps", false),
                       [](bool v) { cfgSetBool("display.showFps", v); });
        box->addView(fpsCell);
    }

    // ── 遮罩设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("遮罩设置")));
    box->addView(makeHint(L("以下设置会在导入新游戏时自动套用")));

    auto makeOverlayPathCell = [&](const std::string &cfgKey, const std::string &labelText) {
        auto *cell = new brls::DetailCell();
        cell->setText(labelText);
        std::string cur = cfgGetStr(cfgKey, "");
        cell->setDetailText(cur.empty() ? L("未设置") : beiklive::tools::getFileName(cur));
        cell->registerAction(L(" 选择"), brls::BUTTON_A,
            [cell, cfgKey](brls::View *) {
                std::filesystem::path currentPath(cfgGetStr(cfgKey, ""));
                openFilePicker({"png"}, [cell, cfgKey](const std::string &path) {
                    cfgSetStr(cfgKey, path);
                    cell->setDetailText(beiklive::tools::getFileName(path));
                }, currentPath.parent_path().string(), currentPath.filename().string());
                return true;
            }, false, false, brls::SOUND_CLICK);
        return cell;
    };

    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBA_PATH, "GBA 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBC_PATH, "GBC 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GB_PATH,  "GB 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_NES_PATH,  "FC 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_SNES_PATH, "SFC 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GENESIS_PATH, "MD 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_ARCADE_PATH, "Arcade 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_DC_PATH, "DC 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_PSP_PATH, "PSP 遮罩"));

    // ── 着色器设置 ────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("着色器设置")));
    box->addView(makeHint(L("以下设置会在导入新游戏时自动套用")));

    auto makeShaderPathCell = [&](const std::string &cfgKey, const std::string &labelText) {
        auto *cell = new brls::DetailCell();
        cell->setText(labelText);
        std::string cur = cfgGetStr(cfgKey, "");
        cell->setDetailText(cur.empty() ? L("未设置") : beiklive::tools::getFileName(cur));
        cell->registerAction(L("选择"), brls::BUTTON_A,
            [cell, cfgKey](brls::View *) {
                std::filesystem::path currentPath(cfgGetStr(cfgKey, ""));
                openFilePicker({"glslp", "glsl"}, [cell, cfgKey](const std::string &path) {
                    cfgSetStr(cfgKey, path);
                    cell->setDetailText(beiklive::tools::getFileName(path));
                }, currentPath.parent_path().string(), currentPath.filename().string());
                return true;
            }, false, false, brls::SOUND_CLICK);
        return cell;
    };

    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_GBA_PATH, "GBA 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_GBC_PATH, "GBC 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_GB_PATH,  "GB 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_NES_PATH,  "FC 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_SNES_PATH, "SFC 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_GENESIS_PATH, "MD 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_ARCADE_PATH, "Arcade 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_DC_PATH, "DC 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_PSP_PATH, "PSP 着色器"));

    scroll->setContentView(box);
    auto *container = new brls::Box(brls::Axis::COLUMN);
    container->setWidthPercentage(100.f);
    container->setGrow(1.0f);
    container->addView(scroll);
    return container;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 声音设置
// ─────────────────────────────────────────────────────────────────────────────

brls::View *SettingPage::buildAudioTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── 音频设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("音频设置")));

    auto *sfxCell = new brls::BooleanCell();
    sfxCell->init("按钮音效", cfgGetBool("audio.buttonSfx", true),
                   [](bool v) { cfgSetBool("audio.buttonSfx", v); });
    box->addView(sfxCell);

    {
        std::vector<std::string> opts = {L("静音"), "25%", "50%", "75%", "100%"};
        static const int vals[] = {0, 25, 50, 75, 100};
        int cur = cfgGetInt(beiklive::SettingKey::KEY_AUDIO_BUTTON_SFX_VOLUME, 100);
        int idx = 4;
        for (int i = 0; i < 5; ++i) if (vals[i] == cur) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("按键音效音量"), opts, idx,
                   [](int i) { if (i >= 0 && i < 5) cfgSetInt(beiklive::SettingKey::KEY_AUDIO_BUTTON_SFX_VOLUME, vals[i]); });
        box->addView(cell);
    }

    {
        std::vector<std::string> opts = {"60 ms", "90 ms", "120 ms", "160 ms"};
        static const int vals[] = {60, 90, 120, 160};
        int cur = cfgGetInt(beiklive::SettingKey::KEY_AUDIO_TARGET_LATENCY_MS, 90);
        int idx = 1;
        for (int i = 0; i < 4; ++i) if (vals[i] == cur) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("目标缓冲延迟"), opts, idx,
                   [](int i) { if (i >= 0 && i < 4) cfgSetInt(beiklive::SettingKey::KEY_AUDIO_TARGET_LATENCY_MS, vals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("越低操作反馈越快，越高越不容易断音")));
    }

    {
        std::vector<std::string> opts = {"120 ms", "180 ms", "240 ms", "320 ms"};
        static const int vals[] = {120, 180, 240, 320};
        int cur = cfgGetInt(beiklive::SettingKey::KEY_AUDIO_MAX_LATENCY_MS, 180);
        int idx = 1;
        for (int i = 0; i < 4; ++i) if (vals[i] == cur) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("最大缓冲延迟"), opts, idx,
                   [](int i) { if (i >= 0 && i < 4) cfgSetInt(beiklive::SettingKey::KEY_AUDIO_MAX_LATENCY_MS, vals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("超过该延迟会丢弃旧音频，避免声音落后画面")));
    }

    {
        std::vector<std::string> opts = {L("关闭"), L("柔和"), L("标准"), "强"};
        static const float vals[] = {0.0f, 0.008f, 0.015f, 0.025f};
        float cur = GET_SETTING_KEY_FLOAT(beiklive::SettingKey::KEY_AUDIO_SYNC_STRENGTH, 0.015f);
        int idx = 2;
        float best = std::fabs(cur - vals[2]);
        for (int i = 0; i < 4; ++i) {
            float diff = std::fabs(cur - vals[i]);
            if (diff < best) { best = diff; idx = i; }
        }
        auto *cell = new brls::SelectorCell();
        cell->init(L("音画同步修正"), opts, idx,
                   [](int i) { if (i >= 0 && i < 4) SET_SETTING_KEY_FLOAT(beiklive::SettingKey::KEY_AUDIO_SYNC_STRENGTH, vals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("根据音频缓冲量微调模拟节奏，减少爆音和长期漂移")));
    }

    {
        std::vector<std::string> opts = {L("关闭"), "4 ms", "6 ms", "10 ms"};
        static const int vals[] = {0, 4, 6, 10};
        int cur = cfgGetInt(beiklive::SettingKey::KEY_AUDIO_TRANSITION_FADE_MS, 6);
        int idx = 2;
        for (int i = 0; i < 4; ++i) if (vals[i] == cur) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("切换淡入淡出"), opts, idx,
                   [](int i) { if (i >= 0 && i < 4) cfgSetInt(beiklive::SettingKey::KEY_AUDIO_TRANSITION_FADE_MS, vals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("暂停、静音、读档等状态切换时降低咔哒声")));
    }

    {
        std::vector<std::string> lpfOpts = {L("关闭"), L("开启")};
        std::string curLpf = cfgGetStr("core.mgba_audio_low_pass_filter", "disabled");
        auto *cell = new brls::SelectorCell();
        cell->init(L("低通滤波器"), lpfOpts, curLpf == "enabled" ? 1 : 0,
                   [](int idx) { cfgSetStr("core.mgba_audio_low_pass_filter", idx == 1 ? "enabled" : "disabled"); });
        box->addView(cell);
        box->addView(makeHint(L("模拟 GBA 硬件低通滤波，降低高频噪音")));
    }

    {
        std::vector<std::string> rangeOpts = {"20%", "40%", "60%", "80%", "100%"};
        static const char* rangeVals[] = {"20", "40", "60", "80", "100"};
        std::string curRange = cfgGetStr("core.mgba_audio_low_pass_range", "60");
        int idx = 2;
        for (int i = 0; i < 5; ++i) if (rangeVals[i] == curRange) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init(L("低通滤波截止频率"), rangeOpts, idx,
                   [](int i) { if (i >= 0 && i < 5) cfgSetStr("core.mgba_audio_low_pass_range", rangeVals[i]); });
        box->addView(cell);
        box->addView(makeHint(L("数值越低高频削减越多，音色越沉闷")));
    }

    scroll->setContentView(box);
    auto *container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->setWidthPercentage(100.f);
    container->addView(scroll);
    return container;
}

namespace
{
    void registerKeyBindActions(beiklive::DetailCell* cell, const std::string& cfgKey)
    {
        cell->registerAction(L("确认"), brls::BUTTON_A,
            [cell, cfgKey](brls::View*) {
                openKeyCapture([cell, cfgKey](const std::string& r) {
                    if (r.empty()) return;
                    std::string cur = cfgGetStr(cfgKey, "none");
                    if (cur.empty() || cur == "none") {
                        cur = r;
                    } else {
                        bool exists = false;
                        std::istringstream iss(cur);
                        std::string tok;
                        while (std::getline(iss, tok, '|')) {
                            if (tok == r) { exists = true; break; }
                        }
                        if (!exists) cur += "|" + r;
                    }
                    cfgSetStr(cfgKey, cur);
                    cell->setRightText(cur);
                });
                return true;
            }, false, false, brls::SOUND_CLICK);
        cell->registerAction(L("清除绑定"), brls::BUTTON_X,
            [cell, cfgKey](brls::View*) {
                cfgSetStr(cfgKey, "none");
                cell->setRightText("none");
                return true;
            }, false, false, brls::SOUND_CLICK);
    }

    brls::Box* makeKeyBindListContainer()
    {
        auto* box = new brls::Box(brls::Axis::COLUMN);
        box->setPadding(10.f, 10.f, 10.f, 10.f);
        box->setCornerRadius(10.f);
        box->setBorderThickness(1.f);
        box->setBorderColor(nvgRGBA(255, 255, 255, 50));
        return box;
    }

    brls::View* buildKeyBindPlatformContent(const std::string& prefix, bool nds)
    {
        auto* scroll = makeScrollTab();
        auto* box = makeContentBox();
        const unsigned platformMask = beiklive::input_mapping::platformMaskForPrefix(prefix);

        box->addView(makeHeader(L("游戏按键映射（手柄）")));
        auto* mapcontainer = makeKeyBindListContainer();
        for (const auto& entry : beiklive::input_mapping::kGameButtonDefaults)
        {
            if ((entry.platformMask & platformMask) == 0)
                continue;
            std::string cfgKey = beiklive::input_mapping::makeHandleKey(prefix, entry.suffix);
            auto* cell = new beiklive::DetailCell();
            cell->setLeftTextSize(18.f);
            cell->setLeftText(beiklive::input_mapping::gameButtonLabelForPrefix(prefix, entry));
            cell->setRightText(cfgGetStr(
                cfgKey,
                beiklive::input_mapping::defaultHandleValueForPrefix(
                    prefix, entry.suffix, entry.defaultValue)));
            registerKeyBindActions(cell, cfgKey);
            mapcontainer->addView(cell);
        }
        box->addView(mapcontainer);

        box->addView(makeHeader(L("功能热键绑定")));
        for (const auto& entry : beiklive::input_mapping::kHotkeyDefaults)
        {
            if (!beiklive::input_mapping::showsHotkeyForPrefix(prefix, entry, nds))
                continue;
            std::string cfgKey = beiklive::input_mapping::makeKey(prefix, entry.key);
            auto* cell = new beiklive::DetailCell();
            cell->setLeftText(std::string(entry.label));
            cell->setRightText(cfgGetStr(cfgKey, entry.defaultValue));
            registerKeyBindActions(cell, cfgKey);
            box->addView(cell);
        }
        const bool pointerHotkeys = nds || prefix == "3ds.";
        if (pointerHotkeys)
        {
            for (const auto& entry : beiklive::input_mapping::kPointerHotkeys)
            {
                if ((nds && entry.hiddenOnNds) ||
                    (prefix == "3ds." && entry.hiddenOnThreeDs))
                    continue;
                std::string cfgKey = beiklive::input_mapping::makeKey(prefix, entry.key);
                auto* cell = new beiklive::DetailCell();
                cell->setLeftText(std::string(entry.label));
                cell->setRightText(cfgGetStr(cfgKey, entry.defaultValue));
                registerKeyBindActions(cell, cfgKey);
                box->addView(cell);
                box->addView(makeHint(pointerHotkeyHint(entry.key)));
            }
        }
        if (nds)
        {
            box->addView(makeHeader(L("NDS 特殊功能")));
            for (const auto& entry : beiklive::input_mapping::kNdsSpecialHotkeys)
            {
                std::string cfgKey = beiklive::input_mapping::makeKey(prefix, entry.key);
                auto* cell = new beiklive::DetailCell();
                cell->setLeftText(std::string(entry.label));
                cell->setRightText(cfgGetStr(cfgKey, entry.defaultValue));
                registerKeyBindActions(cell, cfgKey);
                box->addView(cell);
                box->addView(makeHint(pointerHotkeyHint(entry.key)));
            }
        }

        if (beiklive::input_mapping::showsTurboBindingsForPrefix(prefix))
        {
            box->addView(makeHeader(L("连发按键绑定")));
            {
                std::string cfgKey = beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboAKey);
                auto* cell = new beiklive::DetailCell();
                cell->setLeftText(L("A 连发"));
                cell->setRightText(cfgGetStr(cfgKey, beiklive::input_mapping::kTurboADefault));
                registerKeyBindActions(cell, cfgKey);
                box->addView(cell);
            }
            {
                std::string cfgKey = beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboBKey);
                auto* cell = new beiklive::DetailCell();
                cell->setLeftText(L("B 连发"));
                cell->setRightText(cfgGetStr(cfgKey, beiklive::input_mapping::kTurboBDefault));
                registerKeyBindActions(cell, cfgKey);
                box->addView(cell);
            }
            {
                std::vector<std::string> rates = {L("每秒1次"), L("每秒5次"), L("每秒10次"), L("每秒15次"), L("每秒30次")};
                static const float rateVals[] = {1.0f, 5.0f, 10.0f, 15.0f, 30.0f};
                float curRate = GET_SETTING_KEY_FLOAT("turbo.rate", 10.0f);
                int idx = 2;
                for (int i = 0; i < 5; ++i)
                    if (rateVals[i] == curRate) { idx = i; break; }
                auto* rateCell = new brls::SelectorCell();
                rateCell->init(L("连发速度"), rates, idx,
                               [](int i) {
                                   if (i >= 0 && i < 5)
                                       SET_SETTING_KEY_FLOAT("turbo.rate", rateVals[i]);
                               });
                box->addView(rateCell);
                box->addView(makeHint(L("按住连发按键时每秒触发的次数，次数越高反应越快")));
            }
        }

        scroll->setContentView(box);
        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setGrow(1.0f);
        container->setWidthPercentage(100.f);
        container->addView(scroll);
        return container;
    }

    void openKeyBindPlatformPage(beiklive::Box* parent,  const std::string& title, const std::string& prefix, bool nds)
    {
        auto* page = new beiklive::Box();
        page->showHeader(true);
        page->getHeader()->setTitle(title);
        page->showFooter(true);
        page->registerAction(L("返回"), brls::BUTTON_B, [page](brls::View*) {
            beiklive::popActivity(page);
            return true;
        });
        page->getContentBox()->addView(buildKeyBindPlatformContent(prefix, nds));
        auto* frame = new brls::AppletFrame(page);
        HIDE_BRLS_BAR(frame);
        beiklive::pushActivity(frame, parent, page);
    }
}

brls::View *SettingPage::buildKeyBindTab()
{
    auto* scroll = makeScrollTab();
    auto* box = makeContentBox();

    struct PlatformEntry
    {
        std::string label;
        std::string prefix;
        bool nds;
    };
    static const PlatformEntry platforms[] = {
        {L("映射GBA游戏"), "", false},
        {L("映射GBC游戏"), "gbc.", false},
        {L("映射GB游戏"), "gb.", false},
        {L("映射NES游戏"), "nes.", false},
        {L("映射SFC游戏"), "sfc.", false},
        {L("映射NDS游戏"), "nds.", true},
        {L("映射MD游戏"), "md.", false},
        {L("映射Arcade游戏"), "arcade.", false},
        {L("映射DC游戏"), "dc.", false},
        {L("映射PSP游戏"), "psp.", false},
    };

    for (const auto& platform : platforms)
    {
        auto* cell = new beiklive::DetailCell();
        cell->setLeftText(platform.label);
        cell->setRightText(">");
        cell->registerClickAction([this, platform](brls::View*) -> bool {
            openKeyBindPlatformPage(this, platform.label, platform.prefix, platform.nds);
            return true;
        });
        box->addView(cell);
    }

    scroll->setContentView(box);
    auto* container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->setWidthPercentage(100.f);
    container->addView(scroll);
    return container;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 调试工具
// ─────────────────────────────────────────────────────────────────────────────

brls::View *SettingPage::buildDebugTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── 日志 ──────────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("日志")));

    {
        static const char *logLevelIds[]        = {"debug", "info", "warning", "error"};
        std::vector<std::string> logLevels      = {
            L("调试 (debug)"), L("信息 (info)"), L("警告 (warning)"), L("错误 (error)")};
        std::string curLevel                     = cfgGetStr(KEY_DEBUG_LOG_LEVEL, "info");
        int levelIdx                             = 1;
        for (int i = 0; i < 4; ++i)
            if (curLevel == logLevelIds[i]) { levelIdx = i; break; }
        auto *logLevelCell = new brls::SelectorCell();
        logLevelCell->init(L("日志级别"), logLevels, levelIdx,
                           [](int idx)
                           {
                               if (idx >= 0 && idx < 4)
                               {
                                   cfgSetStr(KEY_DEBUG_LOG_LEVEL, logLevelIds[idx]);
                                   static const brls::LogLevel lvMap[] = {
                                       brls::LogLevel::LOG_DEBUG,
                                       brls::LogLevel::LOG_INFO,
                                       brls::LogLevel::LOG_WARNING,
                                       brls::LogLevel::LOG_ERROR,
                                   };
                                   brls::Logger::setLogLevel(lvMap[idx]);
                               }
                           });
        box->addView(logLevelCell);
    }

    auto *logFileCell = new brls::BooleanCell();
    logFileCell->init(L("输出日志到文件"),
                      cfgGetBool(KEY_DEBUG_LOG_FILE, false),
                      [](bool v)
                      {
                          cfgSetBool(KEY_DEBUG_LOG_FILE, v);
                          static FILE *s_logFile = nullptr;
                          if (v)
                          {
                              if (s_logFile) { std::fclose(s_logFile); s_logFile = nullptr; }
                              s_logFile = std::fopen(beiklive::path::logFilePath().c_str(), "a");
                              if (s_logFile)
                                  brls::Logger::setLogOutput(s_logFile);
                          }
                          else
                          {
                              brls::Logger::setLogOutput(nullptr);
                              if (s_logFile) { std::fclose(s_logFile); s_logFile = nullptr; }
                          }
                      });
    box->addView(logFileCell);

    auto *logOverlayCell = new brls::BooleanCell();
    logOverlayCell->init(L("显示调试信息覆盖层"),
                         cfgGetBool(KEY_DEBUG_LOG_OVERLAY, false),
                         [](bool v)
                         {
                             cfgSetBool(KEY_DEBUG_LOG_OVERLAY, v);
                             brls::Application::enableDebuggingView(v);
                         });
    box->addView(logOverlayCell);
    box->addView(makeHint(L("在屏幕上方叠加显示帧率、帧时间等性能数据")));

    // ── 核心调试 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader(L("核心调试选项")));

    {
        std::vector<std::string> idleOpts = {"Remove Known", "Detect and Remove", "Don't Remove"};
        std::string curIdle = cfgGetStr("core.mgba_idle_optimization", "Remove Known");
        auto *idleCell = new brls::SelectorCell();
        idleCell->init(L("空闲优化"), idleOpts, findIndex(idleOpts, curIdle),
                       [idleOpts](int idx) { if (idx >= 0 && idx < 3) cfgSetStr("core.mgba_idle_optimization", idleOpts[idx]); });
        box->addView(idleCell);
        box->addView(makeHint(L("减少无意义循环的 CPU 占用，大部分情况下使用 Remove Known")));
    }

    {
        auto *cell = new brls::BooleanCell();
        cell->init(L("允许同时按下反方向"), cfgGetStr("core.mgba_allow_opposing_directions", "no") == "yes",
                   [](bool v) { cfgSetStr("core.mgba_allow_opposing_directions", v ? "yes" : "no"); });
        box->addView(cell);
        box->addView(makeHint(L("允许同时按下左+右或上+下方向键")));
    }

    {
        auto *cell = new brls::BooleanCell();
        cell->init(L("强制 GBP 振动"), cfgGetStr("core.mgba_force_gbp", "OFF") == "ON",
                   [](bool v) { cfgSetStr("core.mgba_force_gbp", v ? "ON" : "OFF"); });
        box->addView(cell);
        box->addView(makeHint(L("强制模拟 Game Boy Player 振动外设效果")));
    }


    scroll->setContentView(box);
    auto *container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->setWidthPercentage(100.f);
    container->addView(scroll);

    return container;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SettingPage
// ─────────────────────────────────────────────────────────────────────────────

SettingPage::SettingPage()
{
    this->showHeader(false);
    this->showFooter(false);
    this->setFocusable(false);
    init();
}

SettingPage::~SettingPage()
{
}

void SettingPage::init()
{
    NanoSettingsHost host;
    host.showShader = [this](bool visible) { this->showShader(visible); };
    host.setGradientTheme = [this](GradientTheme theme) { this->setGradientTheme(theme); };
    host.applyUiTheme = [this]() {
        ApplyUiTheme();
        this->invalidate();
    };
    host.showBackground = [this](bool visible) { this->showBackground(visible); };
    host.setBackgroundImage = [this](const std::string& path) { this->setBackgroundImage(path, true); };
    host.close = [this]() { beiklive::popActivity(this, false); };
    auto* canvas = new NanoSettingsCanvas(std::move(host));
    m_settingsFrame = canvas;
    this->getContentBox()->addView(canvas);
}

} // namespace beiklive
