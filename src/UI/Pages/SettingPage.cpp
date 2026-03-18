#include "UI/Pages/SettingPage.hpp"
#include "UI/Pages/FileListPage.hpp"

#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_selector.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/scrolling_frame.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/applet_frame.hpp>

#include <chrono>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  配置辅助函数（声明于 common.hpp）
// ─────────────────────────────────────────────────────────────────────────────

using beiklive::cfgGetBool;
using beiklive::cfgGetStr;
using beiklive::cfgGetFloat;
using beiklive::cfgGetInt;
using beiklive::cfgSetStr;
using beiklive::cfgSetBool;

// ─────────────────────────────────────────────────────────────────────────────
//  布局辅助函数
// ─────────────────────────────────────────────────────────────────────────────

static brls::ScrollingFrame* makeScrollTab()
{
    auto* scroll = new brls::ScrollingFrame();
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    return scroll;
}

static brls::Box* makeContentBox()
{
    auto* box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(20.f, 40.f, 30.f, 40.f);
    return box;
}

static brls::Header* makeHeader(const std::string& title)
{
    auto* h = new brls::Header();
    h->setTitle(title);
    return h;
}

static int findIndex(const std::vector<std::string>& options,
                     const std::string& val, int defaultIdx = 0)
{
    for (int i = 0; i < (int)options.size(); ++i)
        if (options[i] == val) return i;
    return defaultIdx;
}

// ─────────────────────────────────────────────────────────────────────────────
//  KeyCaptureView（按键捕获全屏页）
// ─────────────────────────────────────────────────────────────────────────────

struct CapKbdKey { const char* name; brls::BrlsKeyboardScancode sc; };
static const CapKbdKey k_capKbdKeys[] = {
    {"A",brls::BRLS_KBD_KEY_A},{"B",brls::BRLS_KBD_KEY_B},
    {"C",brls::BRLS_KBD_KEY_C},{"D",brls::BRLS_KBD_KEY_D},
    {"E",brls::BRLS_KBD_KEY_E},{"F",brls::BRLS_KBD_KEY_F},
    {"G",brls::BRLS_KBD_KEY_G},{"H",brls::BRLS_KBD_KEY_H},
    {"I",brls::BRLS_KBD_KEY_I},{"J",brls::BRLS_KBD_KEY_J},
    {"K",brls::BRLS_KBD_KEY_K},{"L",brls::BRLS_KBD_KEY_L},
    {"M",brls::BRLS_KBD_KEY_M},{"N",brls::BRLS_KBD_KEY_N},
    {"O",brls::BRLS_KBD_KEY_O},{"P",brls::BRLS_KBD_KEY_P},
    {"Q",brls::BRLS_KBD_KEY_Q},{"R",brls::BRLS_KBD_KEY_R},
    {"S",brls::BRLS_KBD_KEY_S},{"T",brls::BRLS_KBD_KEY_T},
    {"U",brls::BRLS_KBD_KEY_U},{"V",brls::BRLS_KBD_KEY_V},
    {"W",brls::BRLS_KBD_KEY_W},{"X",brls::BRLS_KBD_KEY_X},
    {"Y",brls::BRLS_KBD_KEY_Y},{"Z",brls::BRLS_KBD_KEY_Z},
    {"0",brls::BRLS_KBD_KEY_0},{"1",brls::BRLS_KBD_KEY_1},
    {"2",brls::BRLS_KBD_KEY_2},{"3",brls::BRLS_KBD_KEY_3},
    {"4",brls::BRLS_KBD_KEY_4},{"5",brls::BRLS_KBD_KEY_5},
    {"6",brls::BRLS_KBD_KEY_6},{"7",brls::BRLS_KBD_KEY_7},
    {"8",brls::BRLS_KBD_KEY_8},{"9",brls::BRLS_KBD_KEY_9},
    {"SPACE",brls::BRLS_KBD_KEY_SPACE},
    {"ENTER",brls::BRLS_KBD_KEY_ENTER},
    {"BACKSPACE",brls::BRLS_KBD_KEY_BACKSPACE},
    {"TAB",brls::BRLS_KBD_KEY_TAB},
    {"ESC",brls::BRLS_KBD_KEY_ESCAPE},
    {"GRAVE",brls::BRLS_KBD_KEY_GRAVE_ACCENT},
    {"UP",brls::BRLS_KBD_KEY_UP},{"DOWN",brls::BRLS_KBD_KEY_DOWN},
    {"LEFT",brls::BRLS_KBD_KEY_LEFT},{"RIGHT",brls::BRLS_KBD_KEY_RIGHT},
    {"F1",brls::BRLS_KBD_KEY_F1},{"F2",brls::BRLS_KBD_KEY_F2},
    {"F3",brls::BRLS_KBD_KEY_F3},{"F4",brls::BRLS_KBD_KEY_F4},
    {"F5",brls::BRLS_KBD_KEY_F5},{"F6",brls::BRLS_KBD_KEY_F6},
    {"F7",brls::BRLS_KBD_KEY_F7},{"F8",brls::BRLS_KBD_KEY_F8},
    {"F9",brls::BRLS_KBD_KEY_F9},{"F10",brls::BRLS_KBD_KEY_F10},
    {"F11",brls::BRLS_KBD_KEY_F11},{"F12",brls::BRLS_KBD_KEY_F12},
};
static constexpr int k_capKbdKeyCount =
    static_cast<int>(sizeof(k_capKbdKeys) / sizeof(k_capKbdKeys[0]));

struct CapPadKey { const char* name; brls::ControllerButton btn; };
static const CapPadKey k_capPadKeys[] = {
    {"LT",brls::BUTTON_LT},{"LB",brls::BUTTON_LB},{"LSB",brls::BUTTON_LSB},
    {"UP",brls::BUTTON_UP},{"RIGHT",brls::BUTTON_RIGHT},
    {"DOWN",brls::BUTTON_DOWN},{"LEFT",brls::BUTTON_LEFT},
    {"BACK",brls::BUTTON_BACK},{"START",brls::BUTTON_START},
    {"RSB",brls::BUTTON_RSB},{"Y",brls::BUTTON_Y},
    {"B",brls::BUTTON_B},{"A",brls::BUTTON_A},{"X",brls::BUTTON_X},
    {"RB",brls::BUTTON_RB},{"RT",brls::BUTTON_RT},
};
static constexpr int k_capPadKeyCount =
    static_cast<int>(sizeof(k_capPadKeys) / sizeof(k_capPadKeys[0]));

static constexpr int k_capMaxKeys = 2; ///< 组合键最大按键数

/// 按键捕获全屏页，以 Activity 形式推入。
/// 每帧通过 draw() 轮询键盘/手柄输入，检测到按键或 5 秒倒计时结束后调用 onDone(result)。
/// 所有 borealis 导航操作均被消费，避免干扰捕获过程。
class KeyCaptureView : public brls::Box
{
public:
    explicit KeyCaptureView(bool isKeyboard,
                            std::function<void(const std::string&)> onDone)
        : m_isKeyboard(isKeyboard), m_onDone(std::move(onDone))
    {
        setFocusable(true);
        setAxis(brls::Axis::COLUMN);
        setAlignItems(brls::AlignItems::CENTER);
        setJustifyContent(brls::JustifyContent::CENTER);
        setGrow(1.0f);

        m_promptLabel = new brls::Label();
        m_promptLabel->setText(isKeyboard
            ? "beiklive/settings/keybind/press_kbd"_i18n
            : "beiklive/settings/keybind/press_pad"_i18n);
        m_promptLabel->setFontSize(24.f);
        m_promptLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_promptLabel);

        auto* spacer1 = new brls::Padding();
        spacer1->setHeight(30.f);
        addView(spacer1);

        m_keyLabel = new brls::Label();
        m_keyLabel->setText("beiklive/settings/keybind/waiting"_i18n);
        m_keyLabel->setFontSize(48.f);
        m_keyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_keyLabel);

        auto* spacer2 = new brls::Padding();
        spacer2->setHeight(30.f);
        addView(spacer2);

        m_countdownLabel = new brls::Label();
        m_countdownLabel->setText("5" + std::string("beiklive/settings/keybind/countdown_suffix"_i18n));
        m_countdownLabel->setFontSize(22.f);
        m_countdownLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_countdownLabel);

        m_startTime = std::chrono::steady_clock::now();

        // 消费所有手柄导航键，防止触发父视图操作或提前关闭页面。
        // 手柄模式下，每次按键也会同时捕获绑定。
        static const brls::ControllerButton k_swallowBtns[] = {
            brls::BUTTON_A, brls::BUTTON_B, brls::BUTTON_X, brls::BUTTON_Y,
            brls::BUTTON_LB, brls::BUTTON_RB, brls::BUTTON_LT, brls::BUTTON_RT,
            brls::BUTTON_LSB, brls::BUTTON_RSB,
            brls::BUTTON_UP, brls::BUTTON_DOWN, brls::BUTTON_LEFT, brls::BUTTON_RIGHT,
            brls::BUTTON_NAV_UP, brls::BUTTON_NAV_DOWN,
            brls::BUTTON_NAV_LEFT, brls::BUTTON_NAV_RIGHT,
            brls::BUTTON_START, brls::BUTTON_BACK,
        };
        for (auto btn : k_swallowBtns) {
            registerAction("", btn,
                [this, btn](brls::View*) -> bool {
                    if (!m_done && !m_waitingForRelease && !m_isKeyboard) {
                        captureGamepadButton(btn);
                    }
                    return true; // always consume
                },
                /*hidden=*/true);
        }
    }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        // 半透明深色背景，覆盖整个页面
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 200));
        nvgFill(vg);

        if (!m_done)
        {
            if (m_waitingForRelease)
            {
                if (m_isKeyboard) checkKeyboardRelease();
                else              checkGamepadRelease();
                // 等待按键释放时重置倒计时，确保捕获开始后用户有完整 5 秒
                m_startTime = std::chrono::steady_clock::now();
            }
            else
            {
                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(now - m_startTime).count();
                float remaining = 5.0f - elapsed;

                if (remaining <= 0.0f)
                {
                    // 倒计时结束：使用已收集到的按键（可能为空）
                    finish(m_captured);
                }
                else
                {
                    int secs = static_cast<int>(std::ceil(remaining));
                    m_countdownLabel->setText(std::to_string(secs) +
                        "beiklive/settings/keybind/countdown_suffix"_i18n);
                    if (m_isKeyboard) pollKeyboard();
                    // 手柄捕获通过 registerAction 回调处理
                }
            }
        }
        brls::Box::draw(vg, x, y, w, h, style, ctx);
        if (!m_done) invalidate();
    }

private:
    bool m_isKeyboard;
    std::function<void(const std::string&)> m_onDone;
    brls::Label* m_promptLabel    = nullptr;
    brls::Label* m_keyLabel       = nullptr;
    brls::Label* m_countdownLabel = nullptr;
    std::chrono::steady_clock::time_point m_startTime;
    bool m_done = false;
    /// 等待按键释放标志（开始捕获前需全部松开）
    bool m_waitingForRelease = true;
    /// 已捕获按键名称列表（有序，去重，上限 k_capMaxKeys）
    std::vector<std::string> m_capturedKeys;
    /// 由 m_capturedKeys 构建的最终组合字符串
    std::string m_captured;

    // ── Gamepad capture (via action callback) ──────────────────────────────

    void captureGamepadButton(brls::ControllerButton btn)
    {
        // 查找按键名称
        const char* name = nullptr;
        for (int i = 0; i < k_capPadKeyCount; ++i) {
            if (k_capPadKeys[i].btn == btn) { name = k_capPadKeys[i].name; break; }
        }
        if (!name) return;

        // 忽略重复按键
        if (std::find(m_capturedKeys.begin(), m_capturedKeys.end(), name) != m_capturedKeys.end())
            return;

        // 达到上限则不再添加
        if (static_cast<int>(m_capturedKeys.size()) >= k_capMaxKeys)
            return;

        m_capturedKeys.push_back(name);
        m_captured = buildCombo(m_capturedKeys);
        m_keyLabel->setText(m_captured);

        // 不在此处调用 finish()，等待 5 秒倒计时结束以便用户输入组合键（第二个按钮）。
    }

    // ── Keyboard capture (polled every frame) ──────────────────────────────

    void pollKeyboard()
    {
#ifndef __SWITCH__
        auto* platform = brls::Application::getPlatform();
        auto* im = platform ? platform->getInputManager() : nullptr;
        if (!im) return;

        bool ctrl  = im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_CONTROL) ||
                     im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_CONTROL);
        bool shift = im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_SHIFT) ||
                     im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_SHIFT);
        bool alt   = im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_ALT) ||
                     im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_ALT);

        // 收集当前所有按下的非修饰键
        std::vector<std::string> pressed;
        for (int i = 0; i < k_capKbdKeyCount; ++i) {
            if (im->getKeyboardKeyState(k_capKbdKeys[i].sc))
                pressed.push_back(k_capKbdKeys[i].name);
        }

        if (!pressed.empty()) {
            // 构建新捕获集：修饰键在前，普通键在后，去重，上限 k_capMaxKeys
            std::vector<std::string> newKeys = m_capturedKeys;
            for (const auto& key : pressed) {
                // Duplicate check
                if (std::find(newKeys.begin(), newKeys.end(), key) != newKeys.end()) continue;
                if (static_cast<int>(newKeys.size()) >= k_capMaxKeys) break;
                newKeys.push_back(key);
            }

            // Prepend modifier prefix to the combo string
            std::string combo;
            if (ctrl)  combo += "CTRL+";
            if (shift) combo += "SHIFT+";
            if (alt)   combo += "ALT+";
            combo += buildCombo(newKeys);

            m_capturedKeys = newKeys;
            m_captured     = combo;
            m_keyLabel->setText(combo);
            // Do NOT call finish() here – wait for the 5-second countdown to
            // expire so the user has time to enter a combo (second key).
        }
        else if (ctrl || shift || alt)
        {
            // Show live modifier hint while waiting for a regular key
            std::string mod;
            if (ctrl)  mod += "CTRL+";
            if (shift) mod += "SHIFT+";
            if (alt)   mod += "ALT+";
            mod += "beiklive/settings/keybind/combo_more"_i18n;
            m_keyLabel->setText(mod);
        }
        else
        {
            m_keyLabel->setText("beiklive/settings/keybind/waiting"_i18n);
        }
#endif
    }

    // ── Release detection ────────────────────────────────────────────────────

    void checkKeyboardRelease()
    {
#ifndef __SWITCH__
        auto* platform = brls::Application::getPlatform();
        auto* im = platform ? platform->getInputManager() : nullptr;
        if (!im) { m_waitingForRelease = false; return; }

        for (int i = 0; i < k_capKbdKeyCount; ++i)
            if (im->getKeyboardKeyState(k_capKbdKeys[i].sc)) return;

        if (im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_CONTROL)  ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_CONTROL) ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_SHIFT)    ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_SHIFT)   ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_ALT)      ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_ALT))
            return;
        m_waitingForRelease = false;
#else
        m_waitingForRelease = false;
#endif
    }

    void checkGamepadRelease()
    {
        auto state = brls::Application::getControllerState();
        for (int i = 0; i < k_capPadKeyCount; ++i) {
            int idx = static_cast<int>(k_capPadKeys[i].btn);
            if (idx >= 0 && idx < static_cast<int>(brls::_BUTTON_MAX) &&
                state.buttons[idx])
                return;
        }
        m_waitingForRelease = false;
    }

    // ── Helpers ──────────────────────────────────────────────────────────────

    static std::string buildCombo(const std::vector<std::string>& keys)
    {
        std::string result;
        for (const auto& k : keys) {
            if (!result.empty()) result += "+";
            result += k;
        }
        return result;
    }

    void finish(const std::string& result)
    {
        if (m_done) return;
        m_done = true;
        if (!result.empty()) m_keyLabel->setText(result);
        if (m_onDone) m_onDone(result);
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
    }
};

/// Push a full-screen key-capture page as a new Activity.
/// Clears all borealis navigation actions for the duration of the capture.
static void openKeyCapture(bool isKeyboard,
                            std::function<void(const std::string&)> onDone)
{
    auto* content = new KeyCaptureView(isKeyboard, std::move(onDone));
    auto* frame = new brls::AppletFrame(content);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame),
                                    brls::TransitionAnimation::NONE);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Colour presets (must match common.cpp k_xmbColorPresets order)
// ─────────────────────────────────────────────────────────────────────────────

static const char* k_xmbColorIds[]    = { "blue","purple","green","orange","red","cyan","black","original" };
static const char* k_xmbColorLabels[] = { "深蓝","紫色","绿色","橙色","红色","青色","黑色","原版" };
static constexpr int k_xmbColorCount  = 8;

// ─────────────────────────────────────────────────────────────────────────────
//  Shared constants
// ─────────────────────────────────────────────────────────────────────────────

static const float  ffRateVals[]     = { 2.0f, 3.0f, 4.0f, 6.0f, 8.0f };
static const int    k_bufSizeInts[]  = { 300, 600, 1200, 3600 };

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 界面设置
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame* SettingPage::buildUITab()
{
    auto* scroll = makeScrollTab();
    auto* box    = makeContentBox();

    // ── 背景图片 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/ui/header_bg"_i18n));

    auto* showBgCell = new brls::BooleanCell();
    showBgCell->init("beiklive/settings/ui/show_bg"_i18n,
                     cfgGetBool(KEY_UI_SHOW_BG_IMAGE, false),
                     [](bool v){ cfgSetBool(KEY_UI_SHOW_BG_IMAGE, v); beiklive::ApplyXmbColorToAll(); });
    box->addView(showBgCell);

    auto* bgPathCell = new brls::DetailCell();
    bgPathCell->setText("beiklive/settings/ui/bg_path"_i18n);
    bgPathCell->setDetailText(beiklive::string::extractFileName(cfgGetStr(KEY_UI_BG_IMAGE_PATH, "beiklive/settings/ui/bg_not_set"_i18n)));
    bgPathCell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A, [bgPathCell](brls::View*) {
        auto* flPage = new FileListPage();
        flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
        flPage->setDefaultFileCallback([bgPathCell](const FileListItem& item) {
            cfgSetStr(KEY_UI_BG_IMAGE_PATH, item.fullPath);
            bgPathCell->setDetailText(beiklive::string::extractFileName(item.fullPath));
            beiklive::ApplyXmbColorToAll();

            
            brls::Application::popActivity();
        });
        // Start at directory of current path, or root
        std::string startPath = cfgGetStr(KEY_UI_BG_IMAGE_PATH, "");
        if (!startPath.empty()) {
            auto pos = startPath.rfind('/');
#ifdef _WIN32
            auto posW = startPath.rfind('\\');
            if (posW != std::string::npos && (pos == std::string::npos || posW > pos))
                pos = posW;
#endif
            if (pos != std::string::npos) startPath = startPath.substr(0, pos);
        }
        if (startPath.empty()) startPath = "/";
        flPage->navigateTo(startPath);

        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setGrow(1.0f);
        container->addView(flPage);
        container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START, [](brls::View*) {
            brls::Application::popActivity();
            return true;
        });
        auto* frame = new brls::AppletFrame(container);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
        return true;
    }, false, false, brls::SOUND_CLICK);
    box->addView(bgPathCell);

    // ── 背景图片模糊 ──────────────────────────────────────────────────────────
    auto* bgBlurCell = new brls::BooleanCell();
    bgBlurCell->init("beiklive/settings/ui/bg_blur"_i18n,
                     cfgGetBool(KEY_UI_BG_BLUR_ENABLED, false),
                     [](bool v){ cfgSetBool(KEY_UI_BG_BLUR_ENABLED, v); beiklive::ApplyXmbColorToAll(); });
    box->addView(bgBlurCell);

    {
        static const float k_blurRadii[] = { 8.0f, 10.0f, 12.0f, 14.0f, 16.0f, 18.0f, 20.0f };
        static constexpr int k_blurRadiiCount = static_cast<int>(sizeof(k_blurRadii) / sizeof(k_blurRadii[0]));
        std::vector<std::string> blurLabels = { "8","10","12","14","16","18","20" };
        float curRadius = cfgGetFloat(KEY_UI_BG_BLUR_RADIUS, 12.0f);
        int blurIdx = 2; // default to 12
        for (int i = 0; i < k_blurRadiiCount; ++i)
            if (std::fabs(curRadius - k_blurRadii[i]) < 0.01f) { blurIdx = i; break; }
        auto* blurRadiusCell = new brls::SelectorCell();
        blurRadiusCell->init("beiklive/settings/ui/bg_blur_level"_i18n, blurLabels, blurIdx,
            [](int idx){
                if (idx >= 0 && idx < k_blurRadiiCount && SettingManager) {
                    SettingManager->Set(KEY_UI_BG_BLUR_RADIUS,
                                        beiklive::ConfigValue(k_blurRadii[idx]));
                    SettingManager->Save();
                    beiklive::ApplyXmbColorToAll();
                }
            });
        box->addView(blurRadiusCell);
    }

    // ── PSP XMB 风格背景 ──────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/ui/header_xmb"_i18n));

    auto* showXmbCell = new brls::BooleanCell();
    showXmbCell->init("beiklive/settings/ui/show_xmb"_i18n,
                      cfgGetBool(KEY_UI_SHOW_XMB_BG, false),
                      [](bool v){ cfgSetBool(KEY_UI_SHOW_XMB_BG, v); beiklive::ApplyXmbColorToAll(); });
    box->addView(showXmbCell);

    {
        std::vector<std::string> colorLabels(k_xmbColorLabels,
                                              k_xmbColorLabels + k_xmbColorCount);
        std::string curId = cfgGetStr(KEY_UI_PSPXMB_COLOR, "blue");
        int curIdx = 0;
        for (int i = 0; i < k_xmbColorCount; ++i)
            if (curId == k_xmbColorIds[i]) { curIdx = i; break; }

        auto* xmbColorCell = new brls::SelectorCell();
        xmbColorCell->init("beiklive/settings/ui/xmb_color"_i18n, colorLabels, curIdx,
            [](int idx){
                if (idx >= 0 && idx < k_xmbColorCount) {
                    cfgSetStr(KEY_UI_PSPXMB_COLOR, k_xmbColorIds[idx]);
                    beiklive::ApplyXmbColorToAll();
                }
            });
        box->addView(xmbColorCell);
    }

    // ── 存档设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/game/header_save"_i18n));

    {
        std::vector<std::string> saveDirs = {
            "beiklive/settings/game/cheat_loc_rom"_i18n,
            "beiklive/settings/game/cheat_loc_emu"_i18n
        };

        auto* autoSaveStateCell = new brls::BooleanCell();
        autoSaveStateCell->init("beiklive/settings/game/auto_save_state"_i18n,
                                cfgGetBool("save.autoSaveState", false),
                                [](bool v){ cfgSetBool("save.autoSaveState", v); });
        box->addView(autoSaveStateCell);

        {
            static const int k_autoSaveIntervals[] = { 0, 60, 180, 300, 600 };
            static constexpr int k_autoSaveIntervalCount = 5;
            std::vector<std::string> intervalLabels = {
                "beiklive/settings/game/auto_save_interval_off"_i18n,
                "beiklive/settings/game/auto_save_interval_1min"_i18n,
                "beiklive/settings/game/auto_save_interval_3min"_i18n,
                "beiklive/settings/game/auto_save_interval_5min"_i18n,
                "beiklive/settings/game/auto_save_interval_10min"_i18n,
            };
            int curInterval = cfgGetInt("save.autoSaveInterval", 0);
            int intervalIdx = 0;
            for (int i = 0; i < k_autoSaveIntervalCount; ++i)
                if (curInterval == k_autoSaveIntervals[i]) { intervalIdx = i; break; }
            auto* autoSaveIntervalCell = new brls::SelectorCell();
            autoSaveIntervalCell->init("beiklive/settings/game/auto_save_interval"_i18n,
                intervalLabels, intervalIdx,
                [](int idx){
                    if (idx >= 0 && idx < k_autoSaveIntervalCount && SettingManager) {
                        SettingManager->Set("save.autoSaveInterval",
                                            beiklive::ConfigValue(k_autoSaveIntervals[idx]));
                        SettingManager->Save();
                    }
                });
            box->addView(autoSaveIntervalCell);
        }

        auto* autoLoadState0Cell = new brls::BooleanCell();
        autoLoadState0Cell->init("beiklive/settings/game/auto_load_state0"_i18n,
                                 cfgGetBool("save.autoLoadState0", false),
                                 [](bool v){ cfgSetBool("save.autoLoadState0", v); });
        box->addView(autoLoadState0Cell);

        {
            auto* sramDirCell = new brls::SelectorCell();
            sramDirCell->init(
                "beiklive/settings/game/sram_dir"_i18n, saveDirs,
                cfgGetStr("save.sramDir", "").empty() ? 0 : 1,
                [](int idx) {
                    if (idx == 0) cfgSetStr("save.sramDir", "");
                    else cfgSetStr("save.sramDir", BK_APP_ROOT_DIR + std::string("saves"));
                });
            box->addView(sramDirCell);
        }

        {
            auto* stateDirCell = new brls::SelectorCell();
            stateDirCell->init(
                "beiklive/settings/game/state_dir"_i18n, saveDirs,
                cfgGetStr("save.stateDir", "").empty() ? 0 : 1,
                [](int idx) {
                    if (idx == 0) cfgSetStr("save.stateDir", "");
                    else cfgSetStr("save.stateDir", BK_APP_ROOT_DIR + std::string("saves"));
                });
            box->addView(stateDirCell);
        }
    }

    // ── 截图设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/game/header_screenshot"_i18n));

    {
        std::vector<std::string> screenshotDirs = {
            "beiklive/settings/game/screenshot_dir_rom"_i18n,
            "beiklive/settings/game/screenshot_dir_albums"_i18n,
        };
        auto* screenshotDirCell = new brls::SelectorCell();
        screenshotDirCell->init(
            "beiklive/settings/game/screenshot_dir"_i18n, screenshotDirs,
            cfgGetInt("screenshot.dir", 0),
            [](int idx) {
                if (SettingManager) {
                    SettingManager->Set("screenshot.dir", beiklive::ConfigValue(idx));
                    SettingManager->Save();
                }
            });
        box->addView(screenshotDirCell);
    }

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 游戏设置
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame* SettingPage::buildGameTab()
{
    auto* scroll = makeScrollTab();
    auto* box    = makeContentBox();

    std::vector<std::string> holdModes = {
        "beiklive/settings/game/mode_hold"_i18n,
        "beiklive/settings/game/mode_toggle"_i18n
    };

    // ── 加速 ──────────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/game/header_ff"_i18n));

    auto* ffEnableCell = new brls::BooleanCell();
    ffEnableCell->init("beiklive/settings/game/ff_enable"_i18n,
                       cfgGetBool("fastforward.enabled", true),
                       [](bool v){ cfgSetBool("fastforward.enabled", v); });
    box->addView(ffEnableCell);

    {
        std::string ffModeStr = cfgGetStr("fastforward.mode", "hold");
        auto* ffModeCell = new brls::SelectorCell();
        ffModeCell->init("beiklive/settings/game/key_mode"_i18n, holdModes,
                         (ffModeStr == "toggle") ? 1 : 0,
            [](int idx){ cfgSetStr("fastforward.mode", idx == 1 ? "toggle" : "hold"); });
        box->addView(ffModeCell);
    }

    {
        std::vector<std::string> ffRateLabels = { "2x","3x","4x","6x","8x" };
        float curMult = cfgGetFloat("fastforward.multiplier", 4.0f);
        int ffMultIdx = 2;
        for (int i = 0; i < 5; ++i)
            if (std::fabs(curMult - ffRateVals[i]) < 0.01f) { ffMultIdx = i; break; }
        auto* ffMultCell = new brls::SelectorCell();
        ffMultCell->init("beiklive/settings/game/ff_rate"_i18n, ffRateLabels, ffMultIdx,
            [](int idx){
                if (idx >= 0 && idx < 5 && SettingManager) {
                    SettingManager->Set("fastforward.multiplier",
                                        beiklive::ConfigValue(ffRateVals[idx]));
                    SettingManager->Save();
                }
            });
        box->addView(ffMultCell);
    }

    // ── 倒带 ──────────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/game/header_rewind"_i18n));

    auto* rewindEnCell = new brls::BooleanCell();
    rewindEnCell->init("beiklive/settings/game/rewind_enable"_i18n,
                       cfgGetBool("rewind.enabled", false),
                       [](bool v){ cfgSetBool("rewind.enabled", v); });
    box->addView(rewindEnCell);

    {
        std::string rewModeStr = cfgGetStr("rewind.mode", "hold");
        auto* rewModeCell = new brls::SelectorCell();
        rewModeCell->init("beiklive/settings/game/key_mode"_i18n, holdModes,
                          (rewModeStr == "toggle") ? 1 : 0,
            [](int idx){ cfgSetStr("rewind.mode", idx == 1 ? "toggle" : "hold"); });
        box->addView(rewModeCell);
    }

    {
        std::vector<std::string> bufSizeLabels = {
            "beiklive/settings/game/rewind_buf_300"_i18n,
            "beiklive/settings/game/rewind_buf_600"_i18n,
            "beiklive/settings/game/rewind_buf_1200"_i18n,
            "beiklive/settings/game/rewind_buf_3600"_i18n
        };
        int curBuf = cfgGetInt("rewind.bufferSize", 3600);
        int bufIdx = 3;
        for (int i = 0; i < 4; ++i)
            if (curBuf == k_bufSizeInts[i]) { bufIdx = i; break; }
        auto* bufCell = new brls::SelectorCell();
        bufCell->init("beiklive/settings/game/rewind_buf_size"_i18n, bufSizeLabels, bufIdx,
            [](int idx){
                if (idx >= 0 && idx < 4 && SettingManager) {
                    SettingManager->Set("rewind.bufferSize",
                                        beiklive::ConfigValue(k_bufSizeInts[idx]));
                    SettingManager->Save();
                }
            });
        box->addView(bufCell);
    }

    {
        std::vector<std::string> rewSteps = { "1","2","3","4","5" };
        int curStep = cfgGetInt("rewind.step", 2);
        int stepIdx = (curStep >= 1 && curStep <= 5) ? curStep - 1 : 1;
        auto* stepCell = new brls::SelectorCell();
        stepCell->init("beiklive/settings/game/rewind_step"_i18n, rewSteps, stepIdx,
            [](int idx){
                if (SettingManager) {
                    SettingManager->Set("rewind.step", beiklive::ConfigValue(idx + 1));
                    SettingManager->Save();
                }
            });
        box->addView(stepCell);
    }

    // ── GBA/GBC 游戏 ──────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/game/header_gba"_i18n));

    {
        std::vector<std::string> gbModels = {
            "Autodetect","Game Boy","Super Game Boy","Game Boy Color","Game Boy Advance"
        };
        std::string curModel = cfgGetStr("core.mgba_gb_model","Autodetect");
        auto* gbModelCell = new brls::SelectorCell();
        gbModelCell->init("beiklive/settings/game/gb_model"_i18n, gbModels, findIndex(gbModels, curModel),
            [gbModels](int idx){
                if (idx >= 0 && idx < (int)gbModels.size())
                    cfgSetStr("core.mgba_gb_model", gbModels[idx]);
            });
        box->addView(gbModelCell);
    }

    auto* biosCell = new brls::BooleanCell();
    biosCell->init("beiklive/settings/game/use_bios"_i18n,
                   cfgGetStr("core.mgba_use_bios","ON") == "ON",
                   [](bool v){ cfgSetStr("core.mgba_use_bios", v ? "ON" : "OFF"); });
    box->addView(biosCell);

    auto* skipBiosCell = new brls::BooleanCell();
    skipBiosCell->init("beiklive/settings/game/skip_bios"_i18n,
                       cfgGetStr("core.mgba_skip_bios","OFF") == "ON",
                       [](bool v){ cfgSetStr("core.mgba_skip_bios", v ? "ON" : "OFF"); });
    box->addView(skipBiosCell);

    {
        std::vector<std::string> gbColors = {
            "Grayscale","Honey","Lime","Grapefruit","Game Boy","Burnt Orange",
            "Mystic Blue","Motocross Pink","Gaiden Pink","Blues","Dark Knight","Solarized Gold"
        };
        std::string curGbColor = cfgGetStr("core.mgba_gb_colors","Grayscale");
        auto* gbColorCell = new brls::SelectorCell();
        gbColorCell->init("beiklive/settings/game/gb_colors"_i18n, gbColors, findIndex(gbColors, curGbColor),
            [gbColors](int idx){
                if (idx >= 0 && idx < (int)gbColors.size())
                    cfgSetStr("core.mgba_gb_colors", gbColors[idx]);
            });
        box->addView(gbColorCell);
    }

    {
        std::vector<std::string> idleOpts = {
            "Remove Known","Detect and Remove","Don't Remove"
        };
        std::string curIdle = cfgGetStr("core.mgba_idle_optimization","Remove Known");
        auto* idleCell = new brls::SelectorCell();
        idleCell->init("beiklive/settings/game/idle_opt"_i18n, idleOpts, findIndex(idleOpts, curIdle),
            [idleOpts](int idx){
                if (idx >= 0 && idx < (int)idleOpts.size())
                    cfgSetStr("core.mgba_idle_optimization", idleOpts[idx]);
            });
        box->addView(idleCell);
    }

    // ── 金手指设置 ────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/game/header_cheat"_i18n));

    std::vector<std::string> saveDirs = {
        "beiklive/settings/game/cheat_loc_rom"_i18n,
        "beiklive/settings/game/cheat_loc_emu"_i18n
    };

    auto* cheatEnableCell = new brls::BooleanCell();
    cheatEnableCell->init("beiklive/settings/game/cheat_enable"_i18n,
                          cfgGetBool("cheat.enabled", false),
                          [](bool v){ cfgSetBool("cheat.enabled", v); });
    box->addView(cheatEnableCell);

    {
        auto* cheatDirCell = new brls::SelectorCell();
        cheatDirCell->init(
            "beiklive/settings/game/cheat_dir"_i18n,
            saveDirs,
            cfgGetStr("cheat.dir", "").empty() ? 0 : 1,
            [](int idx) {
                if (idx == 0) {
                    cfgSetStr("cheat.dir", "");
                } else if (idx == 1) {
                    cfgSetStr("cheat.dir", BK_APP_ROOT_DIR + std::string("cheats"));
                }
            }
        );
        box->addView(cheatDirCell);
    }


    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 画面设置
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame* SettingPage::buildDisplayTab()
{
    auto* scroll = makeScrollTab();
    auto* box    = makeContentBox();

    box->addView(makeHeader("beiklive/settings/display/header_video"_i18n));

    {
        std::vector<std::string> dispModes = {
            "适应 (Fit)","填充 (Fill)","原始 (Original)","整数倍 (Integer)","自定义 (Custom)"
        };
        static const char* dispModeIds[] = { "fit","fill","original","integer","custom" };
        std::string curMode = cfgGetStr("display.mode","original");
        int dispModeIdx = 2;
        for (int i = 0; i < 5; ++i)
            if (curMode == dispModeIds[i]) { dispModeIdx = i; break; }
        auto* dispModeCell = new brls::SelectorCell();
        dispModeCell->init("beiklive/settings/display/mode"_i18n, dispModes, dispModeIdx,
            [](int idx){ if (idx >= 0 && idx < 5) cfgSetStr("display.mode", dispModeIds[idx]); });
        box->addView(dispModeCell);
    }

    // ── 整数倍缩放倍率（仅在整数倍模式下生效） ────────────────────────────
    {
        // 0 = 自动最大整数倍; 1–6 = 固定倍率
        static const int k_intScaleVals[] = { 0, 1, 2, 3, 4, 5, 6 };
        static const char* k_intScaleLabels[] = {
            "自动 (Auto)", "1x", "2x", "3x", "4x", "5x", "6x"
        };
        static constexpr int k_intScaleCount = 7;
        int curMult = cfgGetInt("display.integer_scale_mult", 0);
        int multIdx = 0;
        for (int i = 0; i < k_intScaleCount; ++i)
            if (curMult == k_intScaleVals[i]) { multIdx = i; break; }
        std::vector<std::string> intScaleLabels(k_intScaleLabels,
                                                 k_intScaleLabels + k_intScaleCount);
        auto* intScaleCell = new brls::SelectorCell();
        intScaleCell->init("beiklive/settings/display/int_scale"_i18n, intScaleLabels, multIdx,
            [](int idx){
                if (idx >= 0 && idx < k_intScaleCount && SettingManager) {
                    SettingManager->Set("display.integer_scale_mult",
                                        beiklive::ConfigValue(k_intScaleVals[idx]));
                    SettingManager->Save();
                }
            });
        box->addView(intScaleCell);
    }

    {
        std::vector<std::string> filters = { "最近邻 (Nearest)","双线性 (Linear)" };
        std::string curFilter = cfgGetStr("display.filter","nearest");
        auto* filterCell = new brls::SelectorCell();
        filterCell->init("beiklive/settings/display/filter"_i18n, filters, (curFilter == "linear") ? 1 : 0,
            [](int idx){ cfgSetStr("display.filter", idx == 1 ? "linear" : "nearest"); });
        box->addView(filterCell);
    }

    box->addView(makeHeader("beiklive/settings/display/header_status"_i18n));

    auto* showFpsCell = new brls::BooleanCell();
    showFpsCell->init("beiklive/settings/display/show_fps"_i18n,
                      cfgGetBool("display.showFps", false),
                      [](bool v){ cfgSetBool("display.showFps", v); });
    box->addView(showFpsCell);

    auto* showFfCell = new brls::BooleanCell();
    showFfCell->init("beiklive/settings/display/show_ff"_i18n,
                     cfgGetBool("display.showFfOverlay", true),
                     [](bool v){ cfgSetBool("display.showFfOverlay", v); });
    box->addView(showFfCell);

    auto* showRewCell = new brls::BooleanCell();
    showRewCell->init("beiklive/settings/display/show_rewind"_i18n,
                      cfgGetBool("display.showRewindOverlay", true),
                      [](bool v){ cfgSetBool("display.showRewindOverlay", v); });
    box->addView(showRewCell);

    auto* showMuteCell = new brls::BooleanCell();
    showMuteCell->init("beiklive/settings/display/show_mute"_i18n,
                       cfgGetBool("display.showMuteOverlay", true),
                       [](bool v){ cfgSetBool("display.showMuteOverlay", v); });
    box->addView(showMuteCell);

    // ── 遮罩设置 ────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/display/header_overlay"_i18n));

    auto* overlayEnCell = new brls::BooleanCell();
    overlayEnCell->init("beiklive/settings/display/overlay_enable"_i18n,
                        cfgGetBool(KEY_DISPLAY_OVERLAY_ENABLED, false),
                        [](bool v){ cfgSetBool(KEY_DISPLAY_OVERLAY_ENABLED, v); });
    box->addView(overlayEnCell);

    // Helper: build a PNG path picker DetailCell for a given config key and i18n label.
    auto makeOverlayPathCell = [&](const std::string& cfgKey, const std::string& labelKey) {
        auto* cell = new brls::DetailCell();
        cell->setText(labelKey);
        cell->setDetailText(beiklive::string::extractFileName(
            cfgGetStr(cfgKey, "beiklive/settings/display/overlay_not_set"_i18n)));
        cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [cell, cfgKey](brls::View*) {
                auto* flPage = new FileListPage();
                flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
                flPage->setDefaultFileCallback([cell, cfgKey](const FileListItem& item) {
                    cfgSetStr(cfgKey, item.fullPath);
                    cell->setDetailText(beiklive::string::extractFileName(item.fullPath));
                    brls::Application::popActivity();
                });
                std::string startPath = beiklive::string::extractDirPath(cfgGetStr(cfgKey, ""));
                if (startPath.empty()) startPath = "/";
                flPage->navigateTo(startPath);
                auto* container = new brls::Box(brls::Axis::COLUMN);
                container->setGrow(1.0f);
                container->addView(flPage);
                container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                    [](brls::View*) { brls::Application::popActivity(); return true; });
                auto* frame = new brls::AppletFrame(container);
                frame->setHeaderVisibility(brls::Visibility::GONE);
                frame->setFooterVisibility(brls::Visibility::GONE);
                frame->setBackground(brls::ViewBackground::NONE);
                brls::Application::pushActivity(new brls::Activity(frame));
                return true;
            }, false, false, brls::SOUND_CLICK);
        return cell;
    };

    box->addView(makeOverlayPathCell(KEY_DISPLAY_OVERLAY_GBA_PATH,
                                    "beiklive/settings/display/overlay_gba_path"_i18n));
    box->addView(makeOverlayPathCell(KEY_DISPLAY_OVERLAY_GBC_PATH,
                                    "beiklive/settings/display/overlay_gbc_path"_i18n));

    // ── 着色器设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/display/header_shader"_i18n));

    // Helper: 构建着色器路径选择 DetailCell
    auto makeShaderPathCell = [&](const std::string& enableKey,
                                   const std::string& pathKey,
                                   const std::string& enableLabel,
                                   const std::string& pathLabel) {
        // 着色器开关
        auto* enCell = new brls::BooleanCell();
        enCell->init(enableLabel, cfgGetBool(enableKey, false),
                     [enableKey](bool v){ cfgSetBool(enableKey, v); });
        box->addView(enCell);

        // 着色器路径
        auto* pathCell = new brls::DetailCell();
        pathCell->setText(pathLabel);
        std::string cur = cfgGetStr(pathKey, "");
        pathCell->setDetailText(cur.empty()
            ? "beiklive/settings/display/overlay_not_set"_i18n
            : beiklive::string::extractFileName(cur));
        pathCell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [pathCell, pathKey](brls::View*) {
                auto* flPage = new FileListPage();
                flPage->setFilter({"glslp", "glsl"}, FileListPage::FilterMode::Whitelist);
                flPage->setDefaultFileCallback([pathCell, pathKey](const FileListItem& item) {
                    cfgSetStr(pathKey, item.fullPath);
                    pathCell->setDetailText(beiklive::string::extractFileName(item.fullPath));
                    brls::Application::popActivity();
                });
                std::string startPath = beiklive::string::extractDirPath(cfgGetStr(pathKey, ""));
                if (startPath.empty()) startPath = "/";
                flPage->navigateTo(startPath);
                auto* container = new brls::Box(brls::Axis::COLUMN);
                container->setGrow(1.0f);
                container->addView(flPage);
                container->registerAction("beiklive/hints/close"_i18n, brls::BUTTON_START,
                    [](brls::View*) { brls::Application::popActivity(); return true; });
                auto* frame = new brls::AppletFrame(container);
                frame->setHeaderVisibility(brls::Visibility::GONE);
                frame->setFooterVisibility(brls::Visibility::GONE);
                frame->setBackground(brls::ViewBackground::NONE);
                brls::Application::pushActivity(new brls::Activity(frame));
                return true;
            }, false, false, brls::SOUND_CLICK);
        box->addView(pathCell);
    };

    // GBA 着色器
    makeShaderPathCell(KEY_DISPLAY_SHADER_GBA_ENABLED, KEY_DISPLAY_SHADER_GBA_PATH,
                       "beiklive/settings/display/shader_gba_enable"_i18n,
                       "beiklive/settings/display/shader_gba_path"_i18n);

    // GBC 着色器
    makeShaderPathCell(KEY_DISPLAY_SHADER_GBC_ENABLED, KEY_DISPLAY_SHADER_GBC_PATH,
                       "beiklive/settings/display/shader_gbc_enable"_i18n,
                       "beiklive/settings/display/shader_gbc_path"_i18n);

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 声音设置
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame* SettingPage::buildAudioTab()
{
    auto* scroll = makeScrollTab();
    auto* box    = makeContentBox();

    box->addView(makeHeader("beiklive/settings/audio/header_game"_i18n));

    auto* ffMuteCell = new brls::BooleanCell();
    ffMuteCell->init("beiklive/settings/audio/ff_mute"_i18n,
                     cfgGetBool("fastforward.mute", true),
                     [](bool v){ cfgSetBool("fastforward.mute", v); });
    box->addView(ffMuteCell);

    auto* rewMuteCell = new brls::BooleanCell();
    rewMuteCell->init("beiklive/settings/audio/rewind_mute"_i18n,
                      cfgGetBool("rewind.mute", false),
                      [](bool v){ cfgSetBool("rewind.mute", v); });
    box->addView(rewMuteCell);

    {
        std::vector<std::string> lpfOpts = { "关闭 (disabled)","开启 (enabled)" };
        std::string curLpf = cfgGetStr("core.mgba_audio_low_pass_filter","disabled");
        auto* lpfCell = new brls::SelectorCell();
        lpfCell->init("beiklive/settings/audio/lpf"_i18n, lpfOpts, (curLpf == "enabled") ? 1 : 0,
            [](int idx){
                cfgSetStr("core.mgba_audio_low_pass_filter",
                          idx == 1 ? "enabled" : "disabled");
            });
        box->addView(lpfCell);
    }

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 按键预设
// ─────────────────────────────────────────────────────────────────────────────

struct GameBtnEntry { const char* label; const char* suffix; };
static const GameBtnEntry k_gameBtns[] = {
    {"A 键","a"},  {"B 键","b"},   {"X 键","x"},  {"Y 键","y"},
    {"上","up"},   {"下","down"},  {"左","left"}, {"右","right"},
    {"L1","l"},    {"R1","r"},     {"L2","l2"},   {"R2","r2"},
    {"START","start"}, {"SELECT","select"},
};
static constexpr int k_gameBtnCount =
    static_cast<int>(sizeof(k_gameBtns) / sizeof(k_gameBtns[0]));

using beiklive::InputMappingConfig;

brls::ScrollingFrame* SettingPage::buildKeyBindTab()
{
    auto* scroll = makeScrollTab();
    auto* box    = makeContentBox();

    // ── 手柄 ──────────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/keybind/header_pad"_i18n));

    for (int i = 0; i < k_gameBtnCount; ++i)
    {
        std::string cfgKey = std::string("handle.") + k_gameBtns[i].suffix;
        auto* cell = new brls::DetailCell();
        cell->setText(k_gameBtns[i].label);
        cell->setDetailText(cfgGetStr(cfgKey, "none"));
        std::string captureKey = cfgKey;
        cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [cell, captureKey](brls::View*) {
                openKeyCapture(false, [cell, captureKey](const std::string& r) {
                    if (!r.empty()) { cfgSetStr(captureKey, r); cell->setDetailText(r); }
                });
                return true;
            }, false, false, brls::SOUND_CLICK);
        cell->registerAction("beiklive/hints/clear_binding"_i18n, brls::BUTTON_X,
            [cell, captureKey](brls::View*) {
                cfgSetStr(captureKey, "none");
                cell->setDetailText("none");
                return true;
            }, false, false, brls::SOUND_CLICK);
        box->addView(cell);
    }

    for (int i = 0; i < static_cast<int>(InputMappingConfig::Hotkey::_Count); ++i)
    {
        auto hk = static_cast<InputMappingConfig::Hotkey>(i);
        // 跳过：打开金手指菜单、打开着色器菜单、退出游戏（这三个不暴露给用户配置）
        if (hk == InputMappingConfig::Hotkey::OpenCheatMenu  ||
            hk == InputMappingConfig::Hotkey::OpenShaderMenu ||
            hk == InputMappingConfig::Hotkey::ExitGame)
            continue;
        std::string padKey = InputMappingConfig::hotkeyPadConfigKey(hk);
        std::string label  = std::string(InputMappingConfig::hotkeyDisplayName(hk))
                             + "beiklive/settings/keybind/pad_suffix"_i18n;
        auto* cell = new brls::DetailCell();
        cell->setText(label);
        cell->setDetailText(cfgGetStr(padKey, "none"));
        std::string captureKey = padKey;
        cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [cell, captureKey](brls::View*) {
                openKeyCapture(false, [cell, captureKey](const std::string& r) {
                    if (!r.empty()) { cfgSetStr(captureKey, r); cell->setDetailText(r); }
                });
                return true;
            }, false, false, brls::SOUND_CLICK);
        cell->registerAction("beiklive/hints/clear_binding"_i18n, brls::BUTTON_X,
            [cell, captureKey](brls::View*) {
                cfgSetStr(captureKey, "none");
                cell->setDetailText("none");
                return true;
            }, false, false, brls::SOUND_CLICK);
        box->addView(cell);
    }


    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 调试工具
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame* SettingPage::buildDebugTab()
{
    auto* scroll = makeScrollTab();
    auto* box    = makeContentBox();

    box->addView(makeHeader("beiklive/settings/debug/header_log"_i18n));

    {
        static const char* logLevelIds[] = { "debug","info","warning","error" };
        std::vector<std::string> logLevels = {
            "调试 (debug)","信息 (info)","警告 (warning)","错误 (error)"
        };
        std::string curLevel = cfgGetStr(KEY_DEBUG_LOG_LEVEL, "info");
        int levelIdx = 1;
        for (int i = 0; i < 4; ++i)
            if (curLevel == logLevelIds[i]) { levelIdx = i; break; }
        auto* logLevelCell = new brls::SelectorCell();
        logLevelCell->init("beiklive/settings/debug/log_level"_i18n, logLevels, levelIdx,
            [](int idx){
                if (idx >= 0 && idx < 4) {
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

    auto* logFileCell = new brls::BooleanCell();
    logFileCell->init("beiklive/settings/debug/log_file"_i18n,
                      cfgGetBool(KEY_DEBUG_LOG_FILE, false),
                      [](bool v){
                          cfgSetBool(KEY_DEBUG_LOG_FILE, v);
                          // Static tracking: close previous log file before opening a new one
                          static FILE* s_logFile = nullptr;
                          if (v) {
                              if (s_logFile) { std::fclose(s_logFile); s_logFile = nullptr; }
                              s_logFile = std::fopen(BK_APP_LOG_FILE, "a");
                              if (s_logFile) brls::Logger::setLogOutput(s_logFile);
                          } else {
                              brls::Logger::setLogOutput(nullptr);
                              if (s_logFile) { std::fclose(s_logFile); s_logFile = nullptr; }
                          }
                      });
    box->addView(logFileCell);

    auto* logOverlayCell = new brls::BooleanCell();
    logOverlayCell->init("beiklive/settings/debug/log_overlay"_i18n,
                         cfgGetBool(KEY_DEBUG_LOG_OVERLAY, false),
                         [](bool v){
                             cfgSetBool(KEY_DEBUG_LOG_OVERLAY, v);
                             brls::Application::enableDebuggingView(v);
                         });
    box->addView(logOverlayCell);

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SettingPage
// ─────────────────────────────────────────────────────────────────────────────

SettingPage::SettingPage()
{
    // this->setAlignItems(brls::AlignItems::CENTER);
    this->setAxis(brls::Axis::COLUMN);
    this->setWidth(brls::View::AUTO);

    m_tabframe = new brls::TabFrame();
    m_tabframe->setGrow(1.0f);
    Init();
    addView(m_tabframe);
    addView(new brls::BottomBar());
}

SettingPage::~SettingPage()
{
}

void SettingPage::Init()
{
    m_tabframe->addTab("beiklive/settings/tab/ui"_i18n,      [this]() { return buildUITab(); });
    m_tabframe->addTab("beiklive/settings/tab/game"_i18n,    [this]() { return buildGameTab(); });
    m_tabframe->addTab("beiklive/settings/tab/display"_i18n, [this]() { return buildDisplayTab(); });
    m_tabframe->addTab("beiklive/settings/tab/audio"_i18n,   [this]() { return buildAudioTab(); });
    m_tabframe->addTab("beiklive/settings/tab/keybind"_i18n, [this]() { return buildKeyBindTab(); });
    m_tabframe->addTab("beiklive/settings/tab/debug"_i18n,   [this]() { return buildDebugTab(); });
}