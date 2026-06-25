#include "ui/page/SettingPage.hpp"
#include "ui/page/FileListPage.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "ui/utils/UiHelper.hpp"

#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_selector.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/header.hpp>
#include "ui/widget/DetailCell.hpp"
#include <borealis/views/scrolling_frame.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/applet_frame.hpp>

#include "core/Tools.hpp"
#include "core/constexpr.h"
#include "game/control/InputMappingDefaults.hpp"

#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <string>
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

// ─────────────────────────────────────────────────────────────────────────────
//  布局辅助函数（已移至 ui/utils/UiHelper.hpp）
// ─────────────────────────────────────────────────────────────────────────────
using beiklive::ui::makeHint;
using beiklive::ui::makeHeader;
using beiklive::ui::makeContentBox;
using beiklive::ui::makeScrollTab;

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
        titleLabel->setText("按键捕获");
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
        m_promptLabel->setText("按下要绑定的按键(支持组合键)");
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
        oss << std::fixed << std::setprecision(2) << m_countdownSeconds << " 秒";
        m_countdownLabel->setText(oss.str());
        m_countdownLabel->setFontSize(16.f);
        m_countdownLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        m_countdownLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_countdownLabel->setMarginBottom(14.f);
        m_countdownLabel->setFocusable(false);
        card->addView(m_countdownLabel);

        // 提示
        m_hintLabel = new brls::Label();
        m_hintLabel->setText("松开所有按键以开始捕获  |  最多 2 个按键");
        m_hintLabel->setFontSize(14.f);
        m_hintLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        m_hintLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_hintLabel->setVisibility(brls::Visibility::INVISIBLE);
        m_hintLabel->setFocusable(false);
        card->addView(m_hintLabel);

        this->getContentBox()->addView(card);

        m_startTime = std::chrono::steady_clock::now();

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
                m_promptLabel->setText("松开所有已按下的按键...");
                m_promptLabel->setTextColor(nvgRGB(255, 183, 77));
                m_hintLabel->setVisibility(brls::Visibility::VISIBLE);
            }
            else
            {
                m_promptLabel->setText("按下要绑定的按键...");
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
                    oss << std::fixed << std::setprecision(2) << remaining << " 秒后自动确认";
                    m_countdownLabel->setText(oss.str());

                    // 更新进度条宽度
                    float barProgress = remaining / m_countdownSeconds;
                    float barWidth = 240.f * barProgress;
                    m_progressBar->setWidth(barWidth);
                    if (barProgress < 0.3f)
                        m_progressBar->setColor(nvgRGB(255, 82, 82));  // 红色警告
                    else if (barProgress < 0.6f)
                        m_progressBar->setColor(nvgRGB(255, 183, 77)); // 橙色
                    else
                        m_progressBar->setColor(nvgRGB(79, 193, 255)); // 蓝色
                }
            }
        }
        brls::Box::draw(vg, x, y, w, h, style, ctx);
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
    bool m_done              = false;
    bool m_waitingForRelease = true;
    std::vector<std::string> m_capturedKeys;
    std::string m_captured;

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

    void finish(const std::string &result)
    {
        if (m_done)
            return;
        m_done = true;
        if (!result.empty())
        {
            m_keyLabel->setText(result);
            m_keyLabel->setTextColor(nvgRGB(79, 193, 255));
            m_countdownLabel->setText("已确认");
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
    box->addView(makeHeader("GBA/GBC 核心设置"));

    {
        std::vector<std::string> gbModels = {
            "Autodetect", "Game Boy", "Super Game Boy", "Game Boy Color", "Game Boy Advance"};
        std::string curModel = cfgGetStr("core.mgba_gb_model", "Autodetect");
        auto *gbModelCell    = new brls::SelectorCell();
        gbModelCell->init("GB 机型", gbModels, findIndex(gbModels, curModel),
                          [gbModels](int idx) { if (idx >= 0 && idx < 5) cfgSetStr("core.mgba_gb_model", gbModels[idx]); });
        box->addView(gbModelCell);
        box->addView(makeHint("Autodetect: 根据 ROM 头自动检测硬件型号"));
    }

    auto *biosCell = new brls::BooleanCell();
    biosCell->init("使用 BIOS", cfgGetStr("core.mgba_use_bios", "ON") == "ON",
                   [](bool v) { cfgSetStr("core.mgba_use_bios", v ? "ON" : "OFF"); });
    box->addView(biosCell);
    box->addView(makeHint("开启 BIOS 后，之前在非 BIOS 模式下保存的即时存档可能会失效"));

    auto *skipBiosCell = new brls::BooleanCell();
    skipBiosCell->init("跳过 BIOS 动画",
                       cfgGetStr("core.mgba_skip_bios", "OFF") == "ON",
                       [](bool v) { cfgSetStr("core.mgba_skip_bios", v ? "ON" : "OFF"); });
    box->addView(skipBiosCell);

    box->addView(makeHint("BIOS 文件请放入 GBAStation/bios 目录下（gba_bios.bin）"));

    {
        auto& gbColors = beiklive::GetGbColorPresets();
        std::string curGbColor = cfgGetStr("core.mgba_gb_colors", "Grayscale");
        auto *gbColorCell      = new brls::SelectorCell();
        gbColorCell->init("GB 配色", gbColors, findIndex(gbColors, curGbColor),
                          [&gbColors](int idx) { if (idx >= 0 && idx < (int)gbColors.size()) cfgSetStr("core.mgba_gb_colors", gbColors[idx]); });
        box->addView(gbColorCell);
        box->addView(makeHint("为 GB/GBC 单色游戏着色，不影响 GBA 游戏"));
    }

    {
        std::vector<std::string> rtcModes = {"持久化 RTC", "跟随当前系统时间"};
        std::vector<std::string> rtcModeIds = {"persist", "system"};
        std::string curRtcMode = cfgGetStr("core.mgba_rtc_mode", "persist");
        auto* rtcModeCell = new brls::SelectorCell();
        rtcModeCell->init("RTC 时钟模式", rtcModes, findIndex(rtcModeIds, curRtcMode),
                          [rtcModeIds](int idx) {
                              if (idx >= 0 && idx < static_cast<int>(rtcModeIds.size()))
                                  cfgSetStr("core.mgba_rtc_mode", rtcModeIds[idx]);
                          });
        box->addView(rtcModeCell);
        box->addView(makeHint("持久化 RTC：保留游戏内部时钟进度；跟随当前系统时间：每次启动时按当前设备时间校准"));
    }

    // ── 存档设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("存档设置"));

    {
        std::vector<std::string> saveDirs = {"ROM 所在目录", "模拟器目录"};
        std::string curSram = cfgGetStr("save.sramDir", "");
        auto *sramDirCell = new brls::SelectorCell();
        sramDirCell->init("SRAM 存档目录", saveDirs, curSram.empty() ? 0 : 1,
                          [](int idx) { cfgSetStr("save.sramDir", idx == 0 ? "" : beiklive::path::savePath()); });
        box->addView(sramDirCell);
    }

    auto *autoSaveCell = new brls::SelectorCell();
    {
        std::vector<std::string> slotOpts = {"关闭", "档位0", "档位1", "档位2", "档位3", "档位4", "档位5", "档位6", "档位7", "档位8", "档位9"};
        int curSlot = GET_SETTING_KEY_INT("save.autoSaveState", 0);
        if (curSlot < 0 || curSlot > 10) curSlot = 0;
        autoSaveCell->init("自动保存游戏状态", slotOpts, curSlot,
                           [](int i) { SET_SETTING_KEY_INT("save.autoSaveState", i); });
    }
    box->addView(autoSaveCell);

    {
        std::vector<std::string> intervals = {"关闭", "1 分钟", "3 分钟", "5 分钟", "10 分钟"};
        static const int intervalVals[] = {0, 60, 180, 300, 600};
        int curVal = GET_SETTING_KEY_INT("save.autoSaveInterval", 0);
        int idx = 0;
        for (int i = 0; i < 5; ++i) if (intervalVals[i] == curVal) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("自动保存间隔", intervals, idx,
                   [](int i) { if (i >= 0 && i < 5) SET_SETTING_KEY_INT("save.autoSaveInterval", intervalVals[i]); });
        box->addView(cell);
        box->addView(makeHint("定时自动创建即时存档，防止意外丢失进度"));
    }

    auto *autoLoadCell = new brls::SelectorCell();
    {
        std::vector<std::string> slotOpts = {"关闭", "档位0", "档位1", "档位2", "档位3", "档位4", "档位5", "档位6", "档位7", "档位8", "档位9"};
        int curSlot = GET_SETTING_KEY_INT("save.autoLoadState0", 0);
        if (curSlot < 0 || curSlot > 10) curSlot = 0;
        autoLoadCell->init("启动时自动加载", slotOpts, curSlot,
                           [](int i) { SET_SETTING_KEY_INT("save.autoLoadState0", i); });
    }
    box->addView(autoLoadCell);

    {
        std::vector<std::string> exitSlotOpts = {"关闭", "档位0", "档位1", "档位2", "档位3", "档位4", "档位5", "档位6", "档位7", "档位8", "档位9"};
        int curExitSlot = GET_SETTING_KEY_INT("save.autoSaveOnExit", 0);
        if (curExitSlot < 0 || curExitSlot > 10) curExitSlot = 0;
        auto *exitSaveCell = new brls::SelectorCell();
        exitSaveCell->init("退出游戏时自动保存", exitSlotOpts, curExitSlot,
                           [](int i) { SET_SETTING_KEY_INT("save.autoSaveOnExit", i); });
        box->addView(exitSaveCell);
    }

    // ── 封面设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("封面设置"));

    {
        auto *thumbCell = new brls::BooleanCell();
        thumbCell->init("使用存档截图作为封面",
                       cfgGetBool(beiklive::SettingKey::KEY_UI_USE_SAVESTATE_THUMB, false),
                       [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_UI_USE_SAVESTATE_THUMB, v); });
        box->addView(thumbCell);
        box->addView(makeHint("使用即时存档0截图作为封面，已自定义封面的游戏不覆盖"));
    }



    // ── 模拟器UI ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("模拟器UI"));

    {
        auto *shaderCell = new brls::BooleanCell();
        shaderCell->init("启用动态渐变背景",
                        cfgGetBool(beiklive::SettingKey::KEY_UI_SHOW_SHADER, false),
                        [this](bool v) {
                            cfgSetBool(beiklive::SettingKey::KEY_UI_SHOW_SHADER, v);
                            this->showShader(v);
                        });
        box->addView(shaderCell);
    }

    {
        std::vector<std::string> themes = {"深夜蓝", "柠檬黄", "牛油果绿", "草莓红", "海洋蓝", "樱花粉", "VSCode黑"};
        std::vector<std::string> themeIds = {"Midnight", "LemonYellow", "AvocadoGreen", "StrawberryRed",
                                              "OceanBlue", "SakuraPink", "VscodeBlack"};
        std::string curTheme = cfgGetStr(beiklive::SettingKey::KEY_UI_GRADIENT_THEME, "VscodeBlack");
        int curIdx = findIndex(themeIds, curTheme, 6);
        auto *themeCell = new brls::SelectorCell();
        themeCell->init("渐变主题", themes, curIdx,
                       [this, themeIds](int idx) {
                           if (idx >= 0 && idx < (int)themeIds.size()) {
                               cfgSetStr(beiklive::SettingKey::KEY_UI_GRADIENT_THEME, themeIds[idx]);
                               if (themeIds[idx] == "Midnight")      this->setGradientTheme(GradientTheme::Midnight);
                               else if (themeIds[idx] == "LemonYellow")  this->setGradientTheme(GradientTheme::LemonYellow);
                               else if (themeIds[idx] == "AvocadoGreen") this->setGradientTheme(GradientTheme::AvocadoGreen);
                               else if (themeIds[idx] == "StrawberryRed") this->setGradientTheme(GradientTheme::StrawberryRed);
                               else if (themeIds[idx] == "OceanBlue")    this->setGradientTheme(GradientTheme::OceanBlue);
                               else if (themeIds[idx] == "SakuraPink")   this->setGradientTheme(GradientTheme::SakuraPink);
                               else if (themeIds[idx] == "VscodeBlack")  this->setGradientTheme(GradientTheme::VscodeBlack);
                           }
                       });
        box->addView(themeCell);
    }

    // ── 背景图片设置 ──
    {
        auto *bgSwitch = new brls::BooleanCell();
        bgSwitch->init("启用背景图片",
                      cfgGetBool(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, false),
                      [this](bool v) {
                          cfgSetBool(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, v);
                          this->showBackground(v);
                      });
        box->addView(bgSwitch);

        auto *bgPathCell = new brls::DetailCell();
        bgPathCell->setText("背景图片路径");
        std::string curPath = cfgGetStr(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, "");
        bgPathCell->setDetailText(curPath.empty() ? "未设置" : beiklive::tools::getFileName(curPath));
        bgPathCell->registerAction("选择", brls::BUTTON_A,
            [this, bgPathCell](brls::View*) -> bool {
                std::string dir = cfgGetStr(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, "");
                auto pos = dir.rfind('/');
#ifdef _WIN32
                auto posW = dir.rfind('\\');
                if (posW != std::string::npos && (pos == std::string::npos || posW > pos)) pos = posW;
#endif
                if (pos != std::string::npos) dir = dir.substr(0, pos); else dir = "";
                beiklive::openFilePicker({"png"},
                    [this, bgPathCell](const std::string& path) {
                        cfgSetStr(beiklive::SettingKey::KEY_UI_BG_IMAGE_PATH, path);
                        bgPathCell->setDetailText(beiklive::tools::getFileName(path));
                        this->setBackgroundImage(path);
                    },
                    dir);
                return true;
            });
        box->addView(bgPathCell);
    }

    // 文件列表滚动动画
    {
        auto *scrollAnimCell = new brls::BooleanCell();
        scrollAnimCell->init("文件列表滚动动画",
                            cfgGetBool(beiklive::SettingKey::KEY_FILE_LIST_SCROLL_ANIM, true),
                            [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_FILE_LIST_SCROLL_ANIM, v); });
        box->addView(scrollAnimCell);
        box->addView(makeHint("启用文件浏览器的平滑滚动效果，关闭后列表直接跳转"));
    }

    {
        auto* titleSizeCell = new brls::SelectorCell();
        titleSizeCell->init("游戏库标题字号",
            {"正常", "大", "超大"},
            cfgGetInt(beiklive::SettingKey::KEY_UI_LIBRARY_TITLE_SIZE, 0),
            [](int sel) { cfgSetInt(beiklive::SettingKey::KEY_UI_LIBRARY_TITLE_SIZE, sel); });
        box->addView(titleSizeCell);
    }
    box->addView(makeHint("设置游戏库网格列表中游戏标题的显示字号"));

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
    box->addView(makeHeader("快进设置"));

    auto *ffEnabledCell = new brls::BooleanCell();
    ffEnabledCell->init("启用快进", cfgGetBool("fastforward.enabled", true),
                        [](bool v) { cfgSetBool("fastforward.enabled", v); });
    box->addView(ffEnabledCell);

    {
        std::vector<std::string> modes = {"按住", "切换"};
        std::string curMode = cfgGetStr("fastforward.mode", "hold");
        auto *cell = new brls::SelectorCell();
        cell->init("触发模式", modes, curMode == "toggle" ? 1 : 0,
                   [](int idx) { cfgSetStr("fastforward.mode", idx == 1 ? "toggle" : "hold"); });
        box->addView(cell);
        box->addView(makeHint("按住：长按快进键触发  |  切换：按一次永久保持"));
    }

    {
        std::vector<std::string> multis = {"0.1倍", "0.5倍", "1倍", "1.25倍", "1.5倍", "1.75倍", "2倍", "3倍", "4倍", "5倍", "6倍", "7倍", "8倍", "9倍", "10倍"};
        static const float multiVals[] = {0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
        float curMulti = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
        int idx = 8;
        for (int i = 0; i < 15; ++i) if (multiVals[i] == curMulti) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("快进倍率", multis, idx,
                   [](int i) { if (i >= 0 && i < 15) SET_SETTING_KEY_FLOAT("fastforward.multiplier", multiVals[i]); });
        box->addView(cell);
        box->addView(makeHint("小于1倍时可实现慢动作效果，大于1倍用于快进加速"));
    }

    auto *ffMuteCell = new brls::BooleanCell();
    ffMuteCell->init("快进时静音", cfgGetBool("fastforward.mute", true),
                     [](bool v) { cfgSetBool("fastforward.mute", v); });
    box->addView(ffMuteCell);

    // ── 倒带设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("倒带设置"));

    auto *rewEnabledCell = new brls::BooleanCell();
    rewEnabledCell->init("启用倒带", cfgGetBool("rewind.enabled", false),
                         [](bool v) { cfgSetBool("rewind.enabled", v); });
    box->addView(rewEnabledCell);

    {
        std::vector<std::string> modes = {"按住", "切换"};
        std::string curMode = cfgGetStr("rewind.mode", "hold");
        auto *cell = new brls::SelectorCell();
        cell->init("触发模式", modes, curMode == "toggle" ? 1 : 0,
                   [](int idx) { cfgSetStr("rewind.mode", idx == 1 ? "toggle" : "hold"); });
        box->addView(cell);
        box->addView(makeHint("按住：长按倒带键触发  |  切换：按一次永久保持"));
    }

    auto *rewMuteCell = new brls::BooleanCell();
    rewMuteCell->init("倒带时静音", cfgGetBool("rewind.mute", false),
                      [](bool v) { cfgSetBool("rewind.mute", v); });
    box->addView(rewMuteCell);

    {
        std::vector<std::string> stepOpts = {"1 帧", "2 帧", "4 帧", "8 帧"};
        static const int stepVals[] = {1, 2, 4, 8};
        int curStep = GET_SETTING_KEY_INT("rewind.step", 2);
        int idx = 1;
        for (int i = 0; i < 4; ++i) if (stepVals[i] == curStep) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("倒带步进", stepOpts, idx,
                   [](int i) { if (i >= 0 && i < 4) SET_SETTING_KEY_INT("rewind.step", stepVals[i]); });
        box->addView(cell);
        box->addView(makeHint("每次倒带操作回退的帧数，步进越小控制越精细"));
    }

    {
        bool showUI = cfgGetBool(beiklive::SettingKey::KEY_REWIND_SHOW_UI, false);
        auto *showUiCell = new brls::BooleanCell();
        showUiCell->init("显示可视化倒带界面", showUI,
                         [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_REWIND_SHOW_UI, v); });
        box->addView(showUiCell);
    }

    {
        std::vector<std::string> intervalOpts = {"每帧", "每2帧", "每4帧", "每8帧", "每16帧", "每60帧(~1秒)", "每120帧(~2秒)"};
        static const int intervalVals[] = {1, 2, 4, 8, 16, 60, 120};
        static const int intervalCount = 7;
        int curInterval = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SAVE_INTERVAL, 1);
        int curIdx = 0;
        for (int i = 0; i < intervalCount; ++i) if (intervalVals[i] == curInterval) { curIdx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("倒带保存间隔", intervalOpts, curIdx,
                   [](int i) { if (i >= 0 && i < intervalCount) SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_SAVE_INTERVAL, intervalVals[i]); });
        box->addView(cell);
        box->addView(makeHint("间隔越短精度越高但内存占用越大"));
    }

    {
        std::vector<std::string> bufferOpts = {"60 (~1秒)", "120 (~2秒)", "600 (~10秒)", "1800 (~30秒)"};
        static const int bufferVals[] = {60, 120, 600, 1800};
        int curBuffer = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_BUFFER_SIZE, 600);
        int curIdx = 2;
        for (int i = 0; i < 4; ++i) if (bufferVals[i] == curBuffer) { curIdx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("最大倒带缓存", bufferOpts, curIdx,
                   [](int i) { if (i >= 0 && i < 4) SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_BUFFER_SIZE, bufferVals[i]); });
        box->addView(cell);
        box->addView(makeHint("缓冲帧越多可回退时间越长，但内存占用成倍增加"));
    }

    {
        int curCompression = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_THUMB_COMPRESSION, 0);
        std::vector<std::string> compressionOpts = {"最近邻（速度优先）", "双线性（质量优先）"};
        auto *cell = new brls::SelectorCell();
        cell->init("缩略图压缩策略", compressionOpts, curCompression,
                   [](int idx) { if (idx >= 0 && idx <= 1) SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_REWIND_THUMB_COMPRESSION, idx); });
        box->addView(cell);
        box->addView(makeHint("最近邻速度更快但锯齿明显，双线性更平滑但性能略低"));
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
    box->addView(makeHeader("画面显示"));

    {
        std::vector<std::string> modes = {"按比例 (Fit)", "拉伸 (Fill)", "原始 (Original)", "4:3", "整数倍 (Integer)", "自定义 (Custom)"};
        std::string curMode = cfgGetStr("display.mode", "original");
        std::vector<std::string> modeIds = {"fit", "fill", "original", "four_three", "integer", "custom"};
        int idx = findIndex(modeIds, curMode);
        auto *cell = new brls::SelectorCell();
        cell->init("画面模式", modes, idx,
                   [](int i) {
                       static const char* vals[] = {"fit", "fill", "original", "four_three", "integer", "custom"};
                       if (i >= 0 && i < 6) cfgSetStr("display.mode", vals[i]);
                   });
        box->addView(cell);
        box->addView(makeHint("画面缩放模式：Fit=保持比例最大化 Fill=拉伸填满 4:3=按窗口高度等比换算宽度"));
    }

    {
        std::vector<std::string> scales = {"自动", "1倍", "2倍", "3倍", "4倍", "5倍"};
        static const int scaleVals[] = {0, 1, 2, 3, 4, 5};
        int curScale = GET_SETTING_KEY_INT("display.integer_scale_mult", 0);
        int idx = 0;
        for (int i = 0; i < 6; ++i) if (scaleVals[i] == curScale) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("整数倍缩放", scales, idx,
                   [](int i) { if (i >= 0 && i < 6) SET_SETTING_KEY_INT("display.integer_scale_mult", scaleVals[i]); });
        box->addView(cell);
        box->addView(makeHint("画面模式为整数倍时生效，自动=取最大整数倍"));
    }

    {
        std::vector<std::string> filters = {"像素风格 (Nearest)", "平滑 (Linear)"};
        std::string curFilter = cfgGetStr("display.filter", "nearest");
        int idx = (curFilter == "linear") ? 1 : 0;
        auto *cell = new brls::SelectorCell();
        cell->init("纹理过滤", filters, idx,
                   [](int i) { cfgSetStr("display.filter", i == 1 ? "linear" : "nearest"); });
        box->addView(cell);
        box->addView(makeHint("Nearest 像素点阵风格（锐利）| Linear 平滑柔和（模糊）"));
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
    box->addView(makeHeader("遮罩设置"));

    auto *overlayEnabledCell = new brls::BooleanCell();
    overlayEnabledCell->init("启用遮罩", cfgGetBool(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_ENABLED, false),
                             [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_ENABLED, v); });
    box->addView(overlayEnabledCell);

    auto makeOverlayPathCell = [&](const std::string &cfgKey, const std::string &labelText) {
        auto *cell = new brls::DetailCell();
        cell->setText(labelText);
        std::string cur = cfgGetStr(cfgKey, "");
        cell->setDetailText(cur.empty() ? "未设置" : beiklive::tools::getFileName(cur));
        cell->registerAction(" 选择", brls::BUTTON_A,
            [cell, cfgKey](brls::View *) {
                std::string dir = cfgGetStr(cfgKey, "");
                auto pos = dir.rfind('/');
#ifdef _WIN32
                auto posW = dir.rfind('\\');
                if (posW != std::string::npos && (pos == std::string::npos || posW > pos)) pos = posW;
#endif
                if (pos != std::string::npos) dir = dir.substr(0, pos); else dir = "";
                openFilePicker({"png"}, [cell, cfgKey](const std::string &path) {
                    cfgSetStr(cfgKey, path);
                    cell->setDetailText(beiklive::tools::getFileName(path));
                }, dir);
                return true;
            }, false, false, brls::SOUND_CLICK);
        return cell;
    };

    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBA_PATH, "GBA 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBC_PATH, "GBC 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GB_PATH,  "GB 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_NES_PATH,  "FC 遮罩"));
    box->addView(makeOverlayPathCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_SNES_PATH, "SFC 遮罩"));

    // ── 着色器设置 ────────────────────────────────────────────────────────────
    box->addView(makeHeader("着色器设置"));

    auto *shaderEnabledCell = new brls::BooleanCell();
    shaderEnabledCell->init("启用着色器", cfgGetBool(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, false),
                            [](bool v) { cfgSetBool(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, v); });
    box->addView(shaderEnabledCell);

    auto makeShaderPathCell = [&](const std::string &cfgKey, const std::string &labelText) {
        auto *cell = new brls::DetailCell();
        cell->setText(labelText);
        std::string cur = cfgGetStr(cfgKey, "");
        cell->setDetailText(cur.empty() ? "未设置" : beiklive::tools::getFileName(cur));
        cell->registerAction("选择", brls::BUTTON_A,
            [cell, cfgKey](brls::View *) {
                std::string dir = cfgGetStr(cfgKey, "");
                auto pos = dir.rfind('/');
#ifdef _WIN32
                auto posW = dir.rfind('\\');
                if (posW != std::string::npos && (pos == std::string::npos || posW > pos)) pos = posW;
#endif
                if (pos != std::string::npos) dir = dir.substr(0, pos); else dir = "";
                openFilePicker({"glslp", "glsl"}, [cell, cfgKey](const std::string &path) {
                    cfgSetStr(cfgKey, path);
                    cell->setDetailText(beiklive::tools::getFileName(path));
                }, dir);
                return true;
            }, false, false, brls::SOUND_CLICK);
        return cell;
    };

    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_GBA_PATH, "GBA 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_GBC_PATH, "GBC 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_GB_PATH,  "GB 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_NES_PATH,  "FC 着色器"));
    box->addView(makeShaderPathCell(beiklive::SettingKey::KEY_DISPLAY_SHADER_SNES_PATH, "SFC 着色器"));

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
    box->addView(makeHeader("音频设置"));

    auto *sfxCell = new brls::BooleanCell();
    sfxCell->init("按钮音效", cfgGetBool("audio.buttonSfx", true),
                   [](bool v) { cfgSetBool("audio.buttonSfx", v); });
    box->addView(sfxCell);

    {
        std::vector<std::string> opts = {"60 ms", "90 ms", "120 ms", "160 ms"};
        static const int vals[] = {60, 90, 120, 160};
        int cur = cfgGetInt(beiklive::SettingKey::KEY_AUDIO_TARGET_LATENCY_MS, 90);
        int idx = 1;
        for (int i = 0; i < 4; ++i) if (vals[i] == cur) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("目标缓冲延迟", opts, idx,
                   [](int i) { if (i >= 0 && i < 4) cfgSetInt(beiklive::SettingKey::KEY_AUDIO_TARGET_LATENCY_MS, vals[i]); });
        box->addView(cell);
        box->addView(makeHint("越低操作反馈越快，越高越不容易断音"));
    }

    {
        std::vector<std::string> opts = {"120 ms", "180 ms", "240 ms", "320 ms"};
        static const int vals[] = {120, 180, 240, 320};
        int cur = cfgGetInt(beiklive::SettingKey::KEY_AUDIO_MAX_LATENCY_MS, 180);
        int idx = 1;
        for (int i = 0; i < 4; ++i) if (vals[i] == cur) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("最大缓冲延迟", opts, idx,
                   [](int i) { if (i >= 0 && i < 4) cfgSetInt(beiklive::SettingKey::KEY_AUDIO_MAX_LATENCY_MS, vals[i]); });
        box->addView(cell);
        box->addView(makeHint("超过该延迟会丢弃旧音频，避免声音落后画面"));
    }

    {
        std::vector<std::string> opts = {"关闭", "柔和", "标准", "强"};
        static const float vals[] = {0.0f, 0.008f, 0.015f, 0.025f};
        float cur = GET_SETTING_KEY_FLOAT(beiklive::SettingKey::KEY_AUDIO_SYNC_STRENGTH, 0.015f);
        int idx = 2;
        float best = std::fabs(cur - vals[2]);
        for (int i = 0; i < 4; ++i) {
            float diff = std::fabs(cur - vals[i]);
            if (diff < best) { best = diff; idx = i; }
        }
        auto *cell = new brls::SelectorCell();
        cell->init("音画同步修正", opts, idx,
                   [](int i) { if (i >= 0 && i < 4) SET_SETTING_KEY_FLOAT(beiklive::SettingKey::KEY_AUDIO_SYNC_STRENGTH, vals[i]); });
        box->addView(cell);
        box->addView(makeHint("根据音频缓冲量微调模拟节奏，减少爆音和长期漂移"));
    }

    {
        std::vector<std::string> opts = {"关闭", "4 ms", "6 ms", "10 ms"};
        static const int vals[] = {0, 4, 6, 10};
        int cur = cfgGetInt(beiklive::SettingKey::KEY_AUDIO_TRANSITION_FADE_MS, 6);
        int idx = 2;
        for (int i = 0; i < 4; ++i) if (vals[i] == cur) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("切换淡入淡出", opts, idx,
                   [](int i) { if (i >= 0 && i < 4) cfgSetInt(beiklive::SettingKey::KEY_AUDIO_TRANSITION_FADE_MS, vals[i]); });
        box->addView(cell);
        box->addView(makeHint("暂停、静音、读档等状态切换时降低咔哒声"));
    }

    {
        std::vector<std::string> lpfOpts = {"关闭", "开启"};
        std::string curLpf = cfgGetStr("core.mgba_audio_low_pass_filter", "disabled");
        auto *cell = new brls::SelectorCell();
        cell->init("低通滤波器", lpfOpts, curLpf == "enabled" ? 1 : 0,
                   [](int idx) { cfgSetStr("core.mgba_audio_low_pass_filter", idx == 1 ? "enabled" : "disabled"); });
        box->addView(cell);
        box->addView(makeHint("模拟 GBA 硬件低通滤波，降低高频噪音"));
    }

    {
        std::vector<std::string> rangeOpts = {"20%", "40%", "60%", "80%", "100%"};
        static const char* rangeVals[] = {"20", "40", "60", "80", "100"};
        std::string curRange = cfgGetStr("core.mgba_audio_low_pass_range", "60");
        int idx = 2;
        for (int i = 0; i < 5; ++i) if (rangeVals[i] == curRange) { idx = i; break; }
        auto *cell = new brls::SelectorCell();
        cell->init("低通滤波截止频率", rangeOpts, idx,
                   [](int i) { if (i >= 0 && i < 5) cfgSetStr("core.mgba_audio_low_pass_range", rangeVals[i]); });
        box->addView(cell);
        box->addView(makeHint("数值越低高频削减越多，音色越沉闷"));
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
        cell->registerAction("确认"_i18n, brls::BUTTON_A,
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
        cell->registerAction("清除绑定", brls::BUTTON_X,
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

        box->addView(makeHeader("游戏按键映射（手柄）"));
        auto* mapcontainer = makeKeyBindListContainer();
        for (const auto& entry : beiklive::input_mapping::kGameButtonDefaults)
        {
            if ((entry.platformMask & platformMask) == 0)
                continue;
            std::string cfgKey = beiklive::input_mapping::makeHandleKey(prefix, entry.suffix);
            auto* cell = new beiklive::DetailCell();
            cell->setLeftTextSize(18.f);
            cell->setLeftText(entry.label);
            cell->setRightText(cfgGetStr(cfgKey, entry.defaultValue));
            registerKeyBindActions(cell, cfgKey);
            mapcontainer->addView(cell);
        }
        box->addView(mapcontainer);

        box->addView(makeHeader("功能热键绑定"));
        for (const auto& entry : beiklive::input_mapping::kHotkeyDefaults)
        {
            if (nds && entry.hiddenOnNds)
                continue;
            std::string cfgKey = beiklive::input_mapping::makeKey(prefix, entry.key);
            auto* cell = new beiklive::DetailCell();
            cell->setLeftText(std::string(entry.label));
            cell->setRightText(cfgGetStr(cfgKey, entry.defaultValue));
            registerKeyBindActions(cell, cfgKey);
            box->addView(cell);
        }
        if (nds)
        {
            for (const auto& entry : beiklive::input_mapping::kNdsPointerHotkeys)
            {
                std::string cfgKey = beiklive::input_mapping::makeKey(prefix, entry.key);
                auto* cell = new beiklive::DetailCell();
                cell->setLeftText(std::string(entry.label));
                cell->setRightText(cfgGetStr(cfgKey, entry.defaultValue));
                registerKeyBindActions(cell, cfgKey);
                box->addView(cell);
            }
            box->addView(makeHint("切换为指针模式后使用右摇杆控制指针"));
        }

        box->addView(makeHeader("连发按键绑定"));
        {
            std::string cfgKey = beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboAKey);
            auto* cell = new beiklive::DetailCell();
            cell->setLeftText("A 连发");
            cell->setRightText(cfgGetStr(cfgKey, beiklive::input_mapping::kTurboADefault));
            registerKeyBindActions(cell, cfgKey);
            box->addView(cell);
        }
        {
            std::string cfgKey = beiklive::input_mapping::makeKey(prefix, beiklive::input_mapping::kTurboBKey);
            auto* cell = new beiklive::DetailCell();
            cell->setLeftText("B 连发");
            cell->setRightText(cfgGetStr(cfgKey, beiklive::input_mapping::kTurboBDefault));
            registerKeyBindActions(cell, cfgKey);
            box->addView(cell);
        }
        {
            std::vector<std::string> rates = {"每秒1次", "每秒5次", "每秒10次", "每秒15次", "每秒30次"};
            static const float rateVals[] = {1.0f, 5.0f, 10.0f, 15.0f, 30.0f};
            float curRate = GET_SETTING_KEY_FLOAT("turbo.rate", 10.0f);
            int idx = 2;
            for (int i = 0; i < 5; ++i)
                if (rateVals[i] == curRate) { idx = i; break; }
            auto* rateCell = new brls::SelectorCell();
            rateCell->init("连发速度", rates, idx,
                           [](int i) {
                               if (i >= 0 && i < 5)
                                   SET_SETTING_KEY_FLOAT("turbo.rate", rateVals[i]);
                           });
            box->addView(rateCell);
            box->addView(makeHint("按住连发按键时每秒触发的次数，次数越高反应越快"));
        }

        box->addView(makeHeader("摇杆设置"));
        auto* joystickCell = new brls::BooleanCell();
        joystickCell->init("启用左摇杆方向键输入",
                           cfgGetBool("input.joystick.enabled", true),
                           [](bool v) { cfgSetBool("input.joystick.enabled", v); });
        box->addView(joystickCell);

        auto* diagonalCell = new brls::BooleanCell();
        diagonalCell->init("允许斜向输入（同时触发 X 和 Y 方向）",
                           cfgGetBool("input.joystick.diagonal", true),
                           [](bool v) { cfgSetBool("input.joystick.diagonal", v); });
        box->addView(diagonalCell);

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
        page->registerAction("返回", brls::BUTTON_B, [page](brls::View*) {
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
        const char* label;
        const char* prefix;
        bool nds;
    };
    static const PlatformEntry platforms[] = {
        {"映射GBA/GBC/GB游戏", "", false},
        {"映射NES游戏", "nes.", false},
        {"映射SFC游戏", "sfc.", false},
        {"映射NDS游戏", "nds.", true},
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
    box->addView(makeHeader("日志"));

    {
        static const char *logLevelIds[]        = {"debug", "info", "warning", "error"};
        std::vector<std::string> logLevels      = {
            "调试 (debug)", "信息 (info)", "警告 (warning)", "错误 (error)"};
        std::string curLevel                     = cfgGetStr(KEY_DEBUG_LOG_LEVEL, "info");
        int levelIdx                             = 1;
        for (int i = 0; i < 4; ++i)
            if (curLevel == logLevelIds[i]) { levelIdx = i; break; }
        auto *logLevelCell = new brls::SelectorCell();
        logLevelCell->init("日志级别", logLevels, levelIdx,
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
    logFileCell->init("输出日志到文件",
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
    logOverlayCell->init("显示调试信息覆盖层",
                         cfgGetBool(KEY_DEBUG_LOG_OVERLAY, false),
                         [](bool v)
                         {
                             cfgSetBool(KEY_DEBUG_LOG_OVERLAY, v);
                             brls::Application::enableDebuggingView(v);
                         });
    box->addView(logOverlayCell);
    box->addView(makeHint("在屏幕上方叠加显示帧率、帧时间等性能数据"));

    // ── 核心调试 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("核心调试选项"));

    {
        std::vector<std::string> idleOpts = {"Remove Known", "Detect and Remove", "Don't Remove"};
        std::string curIdle = cfgGetStr("core.mgba_idle_optimization", "Remove Known");
        auto *idleCell = new brls::SelectorCell();
        idleCell->init("空闲优化", idleOpts, findIndex(idleOpts, curIdle),
                       [idleOpts](int idx) { if (idx >= 0 && idx < 3) cfgSetStr("core.mgba_idle_optimization", idleOpts[idx]); });
        box->addView(idleCell);
        box->addView(makeHint("减少无意义循环的 CPU 占用，大部分情况下使用 Remove Known"));
    }

    {
        auto *cell = new brls::BooleanCell();
        cell->init("允许同时按下反方向", cfgGetStr("core.mgba_allow_opposing_directions", "no") == "yes",
                   [](bool v) { cfgSetStr("core.mgba_allow_opposing_directions", v ? "yes" : "no"); });
        box->addView(cell);
        box->addView(makeHint("允许同时按下左+右或上+下方向键"));
    }

    {
        auto *cell = new brls::BooleanCell();
        cell->init("超级 GB 边框", cfgGetStr("core.mgba_sgb_borders", "ON") == "ON",
                   [](bool v) { cfgSetStr("core.mgba_sgb_borders", v ? "ON" : "OFF"); });
        box->addView(cell);
        box->addView(makeHint("为 Super Game Boy 游戏绘制专属边框图案"));
    }

    {
        auto *cell = new brls::BooleanCell();
        cell->init("强制 GBP 振动", cfgGetStr("core.mgba_force_gbp", "OFF") == "ON",
                   [](bool v) { cfgSetStr("core.mgba_force_gbp", v ? "ON" : "OFF"); });
        box->addView(cell);
        box->addView(makeHint("强制模拟 Game Boy Player 振动外设效果"));
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
        this->showHeader(true);
        this->getHeader()->setTitle("设置");
        this->showFooter(true);
        this->registerAction("返回", brls::BUTTON_B, [this](brls::View*) { 
            beiklive::popActivity(this);
            return true;
        });
        m_tabframe = new beiklive::TabFrame();
        this->getContentBox()->addView(m_tabframe);
        init();
}

SettingPage::~SettingPage()
{
}

void SettingPage::init()
{
    m_tabframe->addTab(
        "模拟器",  
        BK_RES("img/ui/setting/emu.png"), 
        nullptr, 
        nullptr, 
        nullptr,  
        buildUITab()
    );
    m_tabframe->addDivider();
    m_tabframe->addTab("按键",   
        BK_RES("img/ui/setting/control.png"), 
        nullptr, 
        nullptr, 
        nullptr,  
        buildKeyBindTab());
    m_tabframe->addTab("游戏",   
        BK_RES("img/ui/setting/game.png"), 
        nullptr, 
        nullptr, 
        nullptr,  
        buildGameTab());
    m_tabframe->addTab("显示",   
        BK_RES("img/ui/setting/display.png"), 
        nullptr, 
        nullptr, 
        nullptr,  
        buildDisplayTab());
    m_tabframe->addTab("声音",   
        BK_RES("img/ui/setting/sound.png"), 
        nullptr, 
        nullptr, 
        nullptr,  
        buildAudioTab());
    m_tabframe->addDivider();
    m_tabframe->addTab("调试",   
        BK_RES("img/ui/setting/debug.png"), 
        nullptr, 
        nullptr, 
        nullptr,      
        buildDebugTab());

    m_tabframe->addFinish();
}

} // namespace beiklive
