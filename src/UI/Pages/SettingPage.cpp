#include "UI/Pages/SettingPage.hpp"
#include "UI/Pages/FileListPage.hpp"

#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_selector.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/scrolling_frame.hpp>
#include <borealis/views/dialog.hpp>
#include <borealis/views/label.hpp>

#include <chrono>
#include <string>
#include <vector>
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
//  Config helpers (declared in common.hpp / implemented in common.cpp)
// ─────────────────────────────────────────────────────────────────────────────

using beiklive::cfgGetBool;
using beiklive::cfgGetStr;
using beiklive::cfgGetFloat;
using beiklive::cfgGetInt;
using beiklive::cfgSetStr;
using beiklive::cfgSetBool;

// ─────────────────────────────────────────────────────────────────────────────
//  Layout helpers
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
//  KeyCaptureView
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

/// Content view embedded inside a key-capture Dialog.
/// Polls for keyboard / gamepad input every frame (via draw()) and fires
/// onDone(result) when a key is detected or the 5-second countdown expires.
class KeyCaptureView : public brls::Box
{
public:
    explicit KeyCaptureView(bool isKeyboard,
                            std::function<void(const std::string&)> onDone)
        : m_isKeyboard(isKeyboard), m_onDone(std::move(onDone))
    {
        setAxis(brls::Axis::COLUMN);
        setAlignItems(brls::AlignItems::CENTER);
        setPadding(30.f, 60.f, 30.f, 60.f);
        setWidth(500.f);

        m_promptLabel = new brls::Label();
        m_promptLabel->setText(isKeyboard
            ? "beiklive/settings/keybind/press_kbd"_i18n
            : "beiklive/settings/keybind/press_pad"_i18n);
        m_promptLabel->setFontSize(20.f);
        m_promptLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_promptLabel);

        auto* spacer1 = new brls::Padding();
        spacer1->setHeight(16.f);
        addView(spacer1);

        m_keyLabel = new brls::Label();
        m_keyLabel->setText("beiklive/settings/keybind/waiting"_i18n);
        m_keyLabel->setFontSize(36.f);
        m_keyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_keyLabel);

        auto* spacer2 = new brls::Padding();
        spacer2->setHeight(20.f);
        addView(spacer2);

        m_countdownLabel = new brls::Label();
        m_countdownLabel->setText("5" + std::string("beiklive/settings/keybind/countdown_suffix"_i18n));
        m_countdownLabel->setFontSize(18.f);
        m_countdownLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_countdownLabel);

        m_startTime = std::chrono::steady_clock::now();

        // For gamepad capture mode: register borealis action handlers for every
        // capturable button.  This achieves two things:
        //  1. The press is CONSUMED by this view (returns true), so it never
        //     propagates to the parent DetailCell.  Without this, the same
        //     button press that closes the dialog would immediately re-open it
        //     (e.g., pressing A to map it re-triggers the cell's A action).
        //  2. The mapping result is set through the borealis action system,
        //     which fires once per press, not every frame like draw()-polling.
        if (!isKeyboard) {
            for (int i = 0; i < k_capPadKeyCount; ++i) {
                brls::ControllerButton btn = k_capPadKeys[i].btn;
                registerAction("", btn,
                    [this](brls::View*) -> bool {
                        if (!m_done && !m_waitingForRelease) {
                            // Collect all currently held buttons to support combos.
                            auto state = brls::Application::getControllerState();
                            std::string combo;
                            for (int j = 0; j < k_capPadKeyCount; ++j) {
                                int idx = static_cast<int>(k_capPadKeys[j].btn);
                                if (idx >= 0 && idx < static_cast<int>(brls::_BUTTON_MAX) &&
                                    state.buttons[idx]) {
                                    if (!combo.empty()) combo += "+";
                                    combo += k_capPadKeys[j].name;
                                }
                            }
                            if (!combo.empty())
                                finish(combo);
                        }
                        return true; // always consume – never propagate to parent
                    },
                    /*hidden=*/true);
            }
        }
    }

    void setDialog(brls::Dialog* dlg) { m_dialog = dlg; }

    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override
    {
        if (!m_done)
        {
            // Wait for all buttons/keys to be released before starting capture.
            // This prevents the button that opened the dialog from being
            // immediately captured as the new binding.
            if (m_waitingForRelease)
            {
                if (m_isKeyboard) checkKeyboardRelease();
                else              checkGamepadRelease();
                // Reset the countdown while waiting for release so the user
                // gets the full 5 seconds once input capture actually starts.
                m_startTime = std::chrono::steady_clock::now();
            }
            else
            {
                auto now = std::chrono::steady_clock::now();
                float elapsed = std::chrono::duration<float>(now - m_startTime).count();
                float remaining = 5.0f - elapsed;

                if (remaining <= 0.0f)
                {
                    finish("");
                }
                else
                {
                    int secs = static_cast<int>(std::ceil(remaining));
                    m_countdownLabel->setText(std::to_string(secs) + "beiklive/settings/keybind/countdown_suffix"_i18n);
                    if (m_isKeyboard) pollKeyboard();
                    else              pollGamepad();
                }
            }
        }
        brls::Box::draw(vg, x, y, w, h, style, ctx);
        if (!m_done) invalidate();
    }

private:
    bool m_isKeyboard;
    std::function<void(const std::string&)> m_onDone;
    brls::Dialog* m_dialog = nullptr;
    brls::Label* m_promptLabel    = nullptr;
    brls::Label* m_keyLabel       = nullptr;
    brls::Label* m_countdownLabel = nullptr;
    std::chrono::steady_clock::time_point m_startTime;
    bool m_done = false;
    /// True while waiting for all buttons/keys to be released before capture starts.
    bool m_waitingForRelease = true;

    void finish(const std::string& result)
    {
        if (m_done) return;
        m_done = true;
        if (!result.empty()) m_keyLabel->setText(result);
        if (m_onDone) m_onDone(result);
        if (m_dialog) m_dialog->close();
    }

    /// Returns true when all tracked keys are released; clears m_waitingForRelease.
    void checkKeyboardRelease()
    {
#ifndef __SWITCH__
        auto* platform = brls::Application::getPlatform();
        auto* im = platform ? platform->getInputManager() : nullptr;
        if (!im) { m_waitingForRelease = false; return; }

        for (int i = 0; i < k_capKbdKeyCount; ++i)
        {
            if (im->getKeyboardKeyState(k_capKbdKeys[i].sc))
                return; // still pressed
        }
        // Check modifier keys too
        if (im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_CONTROL)  ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_CONTROL) ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_SHIFT)    ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_SHIFT)   ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_LEFT_ALT)      ||
            im->getKeyboardKeyState(brls::BRLS_KBD_KEY_RIGHT_ALT))
            return; // modifier still pressed
        m_waitingForRelease = false;
#else
        m_waitingForRelease = false;
#endif
    }

    void checkGamepadRelease()
    {
        auto state = brls::Application::getControllerState();
        for (int i = 0; i < k_capPadKeyCount; ++i)
        {
            int idx = static_cast<int>(k_capPadKeys[i].btn);
            if (idx >= 0 && idx < static_cast<int>(brls::_BUTTON_MAX) &&
                state.buttons[idx])
                return; // still pressed
        }
        m_waitingForRelease = false;
    }

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

        for (int i = 0; i < k_capKbdKeyCount; ++i)
        {
            if (im->getKeyboardKeyState(k_capKbdKeys[i].sc))
            {
                std::string combo;
                if (ctrl)  combo += "CTRL+";
                if (shift) combo += "SHIFT+";
                if (alt)   combo += "ALT+";
                combo += k_capKbdKeys[i].name;
                finish(combo);
                return;
            }
        }
        // Show live modifier hints while waiting
        if (ctrl || shift || alt)
        {
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

    void pollGamepad()
    {
        auto state = brls::Application::getControllerState();
        std::string hint;
        for (int i = 0; i < k_capPadKeyCount; ++i)
        {
            int idx = static_cast<int>(k_capPadKeys[i].btn);
            if (idx >= 0 && idx < static_cast<int>(brls::_BUTTON_MAX) &&
                state.buttons[idx])
            {
                if (!hint.empty()) hint += "+";
                hint += k_capPadKeys[i].name;
            }
        }
        if (!hint.empty())
            m_keyLabel->setText(hint + "beiklive/settings/keybind/combo_more"_i18n);
        else
            m_keyLabel->setText("beiklive/settings/keybind/waiting"_i18n);
    }
};

static void openKeyCapture(bool isKeyboard,
                            std::function<void(const std::string&)> onDone)
{
    auto* content = new KeyCaptureView(isKeyboard, std::move(onDone));
    auto* dlg = new brls::Dialog(content);
    dlg->setCancelable(true);
    content->setDialog(dlg);
    dlg->open();
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
        flPage->setFilter({"png", "gif"}, FileListPage::FilterMode::Whitelist);
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

    std::vector<std::string> holdModes = { "按住", "切换" };

    // ── 加速 ──────────────────────────────────────────────────────────────────
    box->addView(makeHeader("加速（最大速度依赖机器性能）"));

    auto* ffEnableCell = new brls::BooleanCell();
    ffEnableCell->init("启用加速",
                       cfgGetBool("fastforward.enabled", true),
                       [](bool v){ cfgSetBool("fastforward.enabled", v); });
    box->addView(ffEnableCell);

    {
        std::string ffModeStr = cfgGetStr("fastforward.mode", "hold");
        auto* ffModeCell = new brls::SelectorCell();
        ffModeCell->init("按键模式", holdModes,
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
        ffMultCell->init("加速倍率", ffRateLabels, ffMultIdx,
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
    box->addView(makeHeader("倒带"));

    auto* rewindEnCell = new brls::BooleanCell();
    rewindEnCell->init("启用倒带",
                       cfgGetBool("rewind.enabled", false),
                       [](bool v){ cfgSetBool("rewind.enabled", v); });
    box->addView(rewindEnCell);

    {
        std::string rewModeStr = cfgGetStr("rewind.mode", "hold");
        auto* rewModeCell = new brls::SelectorCell();
        rewModeCell->init("按键模式", holdModes,
                          (rewModeStr == "toggle") ? 1 : 0,
            [](int idx){ cfgSetStr("rewind.mode", idx == 1 ? "toggle" : "hold"); });
        box->addView(rewModeCell);
    }

    {
        std::vector<std::string> bufSizeLabels = {
            "300 帧 (5秒)","600 帧 (10秒)","1200 帧 (20秒)","3600 帧 (60秒)"
        };
        int curBuf = cfgGetInt("rewind.bufferSize", 3600);
        int bufIdx = 3;
        for (int i = 0; i < 4; ++i)
            if (curBuf == k_bufSizeInts[i]) { bufIdx = i; break; }
        auto* bufCell = new brls::SelectorCell();
        bufCell->init("倒带缓存数量", bufSizeLabels, bufIdx,
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
        stepCell->init("倒带步数", rewSteps, stepIdx,
            [](int idx){
                if (SettingManager) {
                    SettingManager->Set("rewind.step", beiklive::ConfigValue(idx + 1));
                    SettingManager->Save();
                }
            });
        box->addView(stepCell);
    }

    // ── GBA/GBC 游戏 ──────────────────────────────────────────────────────────
    box->addView(makeHeader("GBA/GBC 游戏"));

    {
        std::vector<std::string> gbModels = {
            "Autodetect","Game Boy","Super Game Boy","Game Boy Color","Game Boy Advance"
        };
        std::string curModel = cfgGetStr("core.mgba_gb_model","Autodetect");
        auto* gbModelCell = new brls::SelectorCell();
        gbModelCell->init("GB 机型", gbModels, findIndex(gbModels, curModel),
            [gbModels](int idx){
                if (idx >= 0 && idx < (int)gbModels.size())
                    cfgSetStr("core.mgba_gb_model", gbModels[idx]);
            });
        box->addView(gbModelCell);
    }

    auto* biosCell = new brls::BooleanCell();
    biosCell->init("使用 BIOS",
                   cfgGetStr("core.mgba_use_bios","ON") == "ON",
                   [](bool v){ cfgSetStr("core.mgba_use_bios", v ? "ON" : "OFF"); });
    box->addView(biosCell);

    auto* skipBiosCell = new brls::BooleanCell();
    skipBiosCell->init("跳过 BIOS 画面",
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
        gbColorCell->init("GB 调色板", gbColors, findIndex(gbColors, curGbColor),
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
        idleCell->init("空闲优化", idleOpts, findIndex(idleOpts, curIdle),
            [idleOpts](int idx){
                if (idx >= 0 && idx < (int)idleOpts.size())
                    cfgSetStr("core.mgba_idle_optimization", idleOpts[idx]);
            });
        box->addView(idleCell);
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

    box->addView(makeHeader("画面显示（在游戏中调整更方便）"));

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
        dispModeCell->init("显示模式", dispModes, dispModeIdx,
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
        intScaleCell->init("整数倍缩放倍率（整数倍模式下生效）", intScaleLabels, multIdx,
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
        filterCell->init("纹理过滤", filters, (curFilter == "linear") ? 1 : 0,
            [](int idx){ cfgSetStr("display.filter", idx == 1 ? "linear" : "nearest"); });
        box->addView(filterCell);
    }

    box->addView(makeHeader("状态显示"));

    auto* showFpsCell = new brls::BooleanCell();
    showFpsCell->init("显示帧率 (FPS)",
                      cfgGetBool("display.showFps", false),
                      [](bool v){ cfgSetBool("display.showFps", v); });
    box->addView(showFpsCell);

    auto* showFfCell = new brls::BooleanCell();
    showFfCell->init("显示快进提示",
                     cfgGetBool("display.showFfOverlay", true),
                     [](bool v){ cfgSetBool("display.showFfOverlay", v); });
    box->addView(showFfCell);

    auto* showRewCell = new brls::BooleanCell();
    showRewCell->init("显示倒带提示",
                      cfgGetBool("display.showRewindOverlay", true),
                      [](bool v){ cfgSetBool("display.showRewindOverlay", v); });
    box->addView(showRewCell);

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

    box->addView(makeHeader("模拟器声音"));

    auto* sfxCell = new brls::BooleanCell();
    sfxCell->init("模拟器按键音效",
                  cfgGetBool(KEY_AUDIO_BUTTON_SFX, false),
                  [](bool v){ cfgSetBool(KEY_AUDIO_BUTTON_SFX, v); });
    box->addView(sfxCell);

    box->addView(makeHeader("游戏声音"));

    auto* ffMuteCell = new brls::BooleanCell();
    ffMuteCell->init("快进时静音",
                     cfgGetBool("fastforward.mute", true),
                     [](bool v){ cfgSetBool("fastforward.mute", v); });
    box->addView(ffMuteCell);

    auto* rewMuteCell = new brls::BooleanCell();
    rewMuteCell->init("倒带时静音",
                      cfgGetBool("rewind.mute", false),
                      [](bool v){ cfgSetBool("rewind.mute", v); });
    box->addView(rewMuteCell);

    {
        std::vector<std::string> lpfOpts = { "关闭 (disabled)","开启 (enabled)" };
        std::string curLpf = cfgGetStr("core.mgba_audio_low_pass_filter","disabled");
        auto* lpfCell = new brls::SelectorCell();
        lpfCell->init("低通滤波器", lpfOpts, (curLpf == "enabled") ? 1 : 0,
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

#ifndef __SWITCH__
    // ── 键盘 ──────────────────────────────────────────────────────────────────
    box->addView(makeHeader("beiklive/settings/keybind/header_kbd"_i18n));

    for (int i = 0; i < k_gameBtnCount; ++i)
    {
        std::string cfgKey = std::string("keyboard.") + k_gameBtns[i].suffix;
        auto* cell = new brls::DetailCell();
        cell->setText(k_gameBtns[i].label);
        cell->setDetailText(cfgGetStr(cfgKey, "none"));
        std::string captureKey = cfgKey;
        cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [cell, captureKey](brls::View*) {
                openKeyCapture(true, [cell, captureKey](const std::string& r) {
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
        std::string kbdKey = InputMappingConfig::hotkeyKbdConfigKey(hk);
        std::string label  = std::string(InputMappingConfig::hotkeyDisplayName(hk))
                             + "beiklive/settings/keybind/kbd_suffix"_i18n;
        auto* cell = new brls::DetailCell();
        cell->setText(label);
        cell->setDetailText(cfgGetStr(kbdKey, "none"));
        std::string captureKey = kbdKey;
        cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [cell, captureKey](brls::View*) {
                openKeyCapture(true, [cell, captureKey](const std::string& r) {
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
#endif

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

    box->addView(makeHeader("日志设置"));

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
        logLevelCell->init("日志等级", logLevels, levelIdx,
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
    logFileCell->init("日志文件写入",
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
    logOverlayCell->init("日志浮窗",
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