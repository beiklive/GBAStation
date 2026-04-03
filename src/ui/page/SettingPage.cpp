#include "ui/page/SettingPage.hpp"
#include "ui/page/FileListPage.hpp"

#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/cells/cell_selector.hpp>
#include <borealis/views/cells/cell_detail.hpp>
#include <borealis/views/header.hpp>
#include <borealis/views/scrolling_frame.hpp>
#include <borealis/views/label.hpp>
#include <borealis/views/applet_frame.hpp>

#include "core/Tools.hpp"
#include "core/constexpr.h"

#include <chrono>
#include <sstream>
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

static std::string cfgGetStr(const std::string &key, const std::string &def)
{
    return GET_SETTING_KEY_STR(key, def);
}

static void cfgSetStr(const std::string &key, const std::string &val)
{
    SET_SETTING_KEY_STR(key, val);
}

// ─────────────────────────────────────────────────────────────────────────────
//  布局辅助函数
// ─────────────────────────────────────────────────────────────────────────────

static brls::ScrollingFrame *makeScrollTab()
{
    auto *scroll = new brls::ScrollingFrame();
    scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
    return scroll;
}

static brls::Box *makeContentBox()
{
    auto *box = new brls::Box(brls::Axis::COLUMN);
    box->setPadding(20.f, 40.f, 30.f, 40.f);
    return box;
}

static brls::Header *makeHeader(const std::string &title)
{
    auto *h = new brls::Header();
    h->setTitle(title);
    return h;
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
//  路径选取辅助：推入一个仅显示指定扩展名文件的 FileListPage Activity
//  @param extensions  允许的文件扩展名列表（不含点号，如 {"png"}）
//  @param onSelected  选中文件时的回调，参数为完整路径
//  @param startPath   初始目录（空时从驱动器列表开始）
// ─────────────────────────────────────────────────────────────────────────────

static void openFilePicker(const std::vector<std::string> &extensions,
                           std::function<void(const std::string &)> onSelected,
                           const std::string &startPath = "")
{
    auto *flPage = new beiklive::FileListPage();
    flPage->setFliter(beiklive::enums::FilterMode::Whitelist, extensions);
    flPage->onFileSelected = [onSelected](beiklive::DirListData item)
    {
        // 仅处理实际文件（非目录）
        if (item.itemType != beiklive::enums::FileType::DRIVE &&
            item.itemType != beiklive::enums::FileType::DIRECTORY)
        {
            onSelected(item.fullPath);
            brls::Application::popActivity();
        }
    };
    if (!startPath.empty())
        flPage->setPath(startPath);

    auto *container = new brls::Box(brls::Axis::COLUMN);
    container->setGrow(1.0f);
    container->addView(flPage);
    container->registerAction("关闭"_i18n, brls::BUTTON_START,
                              [](brls::View *) { brls::Application::popActivity(); return true; });

    auto *frame = new brls::AppletFrame(container);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    frame->setBackground(brls::ViewBackground::NONE);
    brls::Application::pushActivity(new brls::Activity(frame));

    if (startPath.empty())
        flPage->showDriveList();
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
    {"LT", brls::BUTTON_LT}, {"LB", brls::BUTTON_LB}, {"LSB", brls::BUTTON_LSB},
    {"UP", brls::BUTTON_UP}, {"RIGHT", brls::BUTTON_RIGHT},
    {"DOWN", brls::BUTTON_DOWN}, {"LEFT", brls::BUTTON_LEFT},
    {"BACK", brls::BUTTON_BACK}, {"START", brls::BUTTON_START},
    {"RSB", brls::BUTTON_RSB}, {"Y", brls::BUTTON_Y},
    {"B", brls::BUTTON_B}, {"A", brls::BUTTON_A}, {"X", brls::BUTTON_X},
    {"RB", brls::BUTTON_RB}, {"RT", brls::BUTTON_RT},
};
static constexpr int k_capPadKeyCount =
    static_cast<int>(sizeof(k_capPadKeys) / sizeof(k_capPadKeys[0]));

static constexpr int k_capMaxKeys = 2; ///< 组合键最大按键数

/// 按键捕获全屏页，5 秒倒计时结束后调用 onDone(result)。
class KeyCaptureView : public brls::Box
{
public:
    explicit KeyCaptureView(std::function<void(const std::string &)> onDone)
        : m_onDone(std::move(onDone))
    {
        setFocusable(true);
        setAxis(brls::Axis::COLUMN);
        setAlignItems(brls::AlignItems::CENTER);
        setJustifyContent(brls::JustifyContent::CENTER);
        setGrow(1.0f);

        m_promptLabel = new brls::Label();
        m_promptLabel->setText("按下要绑定的按键...");
        m_promptLabel->setFontSize(24.f);
        m_promptLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_promptLabel);

        auto *spacer1 = new brls::Padding();
        spacer1->setHeight(30.f);
        addView(spacer1);

        m_keyLabel = new brls::Label();
        m_keyLabel->setText("等待按键...");
        m_keyLabel->setFontSize(48.f);
        m_keyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_keyLabel);

        auto *spacer2 = new brls::Padding();
        spacer2->setHeight(30.f);
        addView(spacer2);

        m_countdownLabel = new brls::Label();
        m_countdownLabel->setText("5 秒");
        m_countdownLabel->setFontSize(22.f);
        m_countdownLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        addView(m_countdownLabel);

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
    }

    void draw(NVGcontext *vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext *ctx) override
    {
        // 半透明深色背景
        nvgBeginPath(vg);
        nvgRect(vg, x, y, w, h);
        nvgFillColor(vg, nvgRGBA(0, 0, 0, 200));
        nvgFill(vg);

        if (!m_done)
        {
            if (m_waitingForRelease)
            {
                checkGamepadRelease();
                m_startTime = std::chrono::steady_clock::now();
            }
            else
            {
                auto now     = std::chrono::steady_clock::now();
                float elapsed   = std::chrono::duration<float>(now - m_startTime).count();
                float remaining = 5.0f - elapsed;

                if (remaining <= 0.0f)
                {
                    finish(m_captured);
                }
                else
                {
                    int secs = static_cast<int>(std::ceil(remaining));
                    m_countdownLabel->setText(std::to_string(secs) + " 秒");
                }
            }
        }
        brls::Box::draw(vg, x, y, w, h, style, ctx);
        if (!m_done)
            invalidate();
    }

private:
    std::function<void(const std::string &)> m_onDone;
    brls::Label *m_promptLabel    = nullptr;
    brls::Label *m_keyLabel       = nullptr;
    brls::Label *m_countdownLabel = nullptr;
    std::chrono::steady_clock::time_point m_startTime;
    bool m_done              = false;
    bool m_waitingForRelease = true; ///< 开始捕获前需全部松开
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
    }

    void checkGamepadRelease()
    {
        auto state = brls::Application::getControllerState();
        for (int i = 0; i < k_capPadKeyCount; ++i)
        {
            int idx = static_cast<int>(k_capPadKeys[i].btn);
            if (idx >= 0 && idx < static_cast<int>(brls::_BUTTON_MAX) && state.buttons[idx])
                return;
        }
        m_waitingForRelease = false;
    }

    static std::string buildCombo(const std::vector<std::string> &keys)
    {
        std::string result;
        for (const auto &k : keys)
        {
            if (!result.empty())
                result += "+";
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
            m_keyLabel->setText(result);
        if (m_onDone)
            m_onDone(result);
        brls::Application::popActivity(brls::TransitionAnimation::NONE);
    }
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
//  Tab: 界面设置
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame *SettingPage::buildUITab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── 存档设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("存档设置"));

    {
        std::vector<std::string> saveDirs = {"ROM 所在目录", "模拟器目录"};

        auto *sramDirCell = new brls::SelectorCell();
        sramDirCell->init("SRAM 存档目录", saveDirs,
                          cfgGetStr("save.sramDir", "").empty() ? 0 : 1,
                          [](int idx)
                          {
                              if (idx == 0)
                                  cfgSetStr("save.sramDir", "");
                              else
                                  cfgSetStr("save.sramDir",
                                            beiklive::path::savePath());
                          });
        box->addView(sramDirCell);
    }

    // ── 金手指设置 ────────────────────────────────────────────────────────────
    box->addView(makeHeader("金手指设置"));

    {
        std::vector<std::string> cheatDirs = {"ROM 所在目录", "模拟器目录"};

        auto *cheatDirCell = new brls::SelectorCell();
        cheatDirCell->init("金手指目录", cheatDirs,
                           cfgGetStr("cheat.dir", "").empty() ? 0 : 1,
                           [](int idx)
                           {
                               if (idx == 0)
                                   cfgSetStr("cheat.dir", "");
                               else
                                   cfgSetStr("cheat.dir", beiklive::path::cheatPath());
                           });
        box->addView(cheatDirCell);
    }

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 游戏设置
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame *SettingPage::buildGameTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── GBA/GBC 游戏 ──────────────────────────────────────────────────────────
    box->addView(makeHeader("GBA/GBC 核心设置"));

    {
        std::vector<std::string> gbModels = {
            "Autodetect", "Game Boy", "Super Game Boy", "Game Boy Color", "Game Boy Advance"};
        std::string curModel = cfgGetStr("core.mgba_gb_model", "Autodetect");
        auto *gbModelCell    = new brls::SelectorCell();
        gbModelCell->init("GB 机型", gbModels, findIndex(gbModels, curModel),
                          [gbModels](int idx)
                          {
                              if (idx >= 0 && idx < (int)gbModels.size())
                                  cfgSetStr("core.mgba_gb_model", gbModels[idx]);
                          });
        box->addView(gbModelCell);
    }

    auto *biosCell = new brls::BooleanCell();
    biosCell->init("使用 BIOS",
                   cfgGetStr("core.mgba_use_bios", "ON") == "ON",
                   [](bool v) { cfgSetStr("core.mgba_use_bios", v ? "ON" : "OFF"); });
    box->addView(biosCell);

    auto *skipBiosCell = new brls::BooleanCell();
    skipBiosCell->init("跳过 BIOS 动画",
                       cfgGetStr("core.mgba_skip_bios", "OFF") == "ON",
                       [](bool v) { cfgSetStr("core.mgba_skip_bios", v ? "ON" : "OFF"); });
    box->addView(skipBiosCell);

    {
        std::vector<std::string> gbColors = {
            "Grayscale", "Honey", "Lime", "Grapefruit", "Game Boy", "Burnt Orange",
            "Mystic Blue", "Motocross Pink", "Gaiden Pink", "Blues", "Dark Knight",
            "Solarized Gold"};
        std::string curGbColor = cfgGetStr("core.mgba_gb_colors", "Grayscale");
        auto *gbColorCell      = new brls::SelectorCell();
        gbColorCell->init("GB 配色", gbColors, findIndex(gbColors, curGbColor),
                          [gbColors](int idx)
                          {
                              if (idx >= 0 && idx < (int)gbColors.size())
                                  cfgSetStr("core.mgba_gb_colors", gbColors[idx]);
                          });
        box->addView(gbColorCell);
    }

    {
        std::vector<std::string> idleOpts = {
            "Remove Known", "Detect and Remove", "Don't Remove"};
        std::string curIdle = cfgGetStr("core.mgba_idle_optimization", "Remove Known");
        auto *idleCell      = new brls::SelectorCell();
        idleCell->init("空闲优化", idleOpts, findIndex(idleOpts, curIdle),
                       [idleOpts](int idx)
                       {
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

brls::ScrollingFrame *SettingPage::buildDisplayTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // ── 遮罩设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("遮罩"));

    // 构建遮罩路径选取 DetailCell 的辅助 lambda
    auto makeOverlayPathCell = [&](const std::string &cfgKey,
                                   const std::string &labelText)
    {
        auto *cell = new brls::DetailCell();
        cell->setText(labelText);
        std::string cur = cfgGetStr(cfgKey, "");
        cell->setDetailText(cur.empty() ? "未设置" : beiklive::tools::getFileName(cur));
        cell->registerAction("确认"_i18n, brls::BUTTON_A,
                             [cell, cfgKey](brls::View *)
                             {
                                 std::string dir = cfgGetStr(cfgKey, "");
                                 auto pos        = dir.rfind('/');
#ifdef _WIN32
                                 auto posW = dir.rfind('\\');
                                 if (posW != std::string::npos &&
                                     (pos == std::string::npos || posW > pos))
                                     pos = posW;
#endif
                                 if (pos != std::string::npos)
                                     dir = dir.substr(0, pos);
                                 else
                                     dir = "";
                                 openFilePicker({"png"},
                                               [cell, cfgKey](const std::string &path)
                                               {
                                                   cfgSetStr(cfgKey, path);
                                                   cell->setDetailText(
                                                       beiklive::tools::getFileName(path));
                                               },
                                               dir);
                                 return true;
                             },
                             false, false, brls::SOUND_CLICK);
        return cell;
    };

    box->addView(makeOverlayPathCell(KEY_DISPLAY_OVERLAY_GBA_PATH, "GBA 遮罩路径"));
    box->addView(makeOverlayPathCell(KEY_DISPLAY_OVERLAY_GBC_PATH, "GBC 遮罩路径"));
    box->addView(makeOverlayPathCell(KEY_DISPLAY_OVERLAY_GB_PATH, "GB 遮罩路径"));

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 声音设置
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame *SettingPage::buildAudioTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    box->addView(makeHeader("游戏音频"));

    {
        std::vector<std::string> lpfOpts = {"关闭 (disabled)", "开启 (enabled)"};
        std::string curLpf               = cfgGetStr("core.mgba_audio_low_pass_filter", "disabled");
        auto *lpfCell                    = new brls::SelectorCell();
        lpfCell->init("低通滤波器", lpfOpts, (curLpf == "enabled") ? 1 : 0,
                      [](int idx)
                      {
                          cfgSetStr("core.mgba_audio_low_pass_filter",
                                    idx == 1 ? "enabled" : "disabled");
                      });
        box->addView(lpfCell);
    }

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 按键绑定
// ─────────────────────────────────────────────────────────────────────────────

struct GameBtnEntry
{
    const char *label;
    const char *suffix;
};

static const GameBtnEntry k_gameBtns[] = {
    {"A 键", "a"},       {"B 键", "b"},     {"X 键", "x"},      {"Y 键", "y"},
    {"上", "up"},        {"下", "down"},     {"左", "left"},     {"右", "right"},
    {"L1", "l"},         {"R1", "r"},        {"L2", "l2"},       {"R2", "r2"},
    {"START", "start"},  {"SELECT", "select"},
    {"左摇杆上", "lstick_up"},   {"左摇杆下", "lstick_down"},
    {"左摇杆左", "lstick_left"}, {"左摇杆右", "lstick_right"},
    {"右摇杆上", "rstick_up"},   {"右摇杆下", "rstick_down"},
    {"右摇杆左", "rstick_left"}, {"右摇杆右", "rstick_right"},
};
static constexpr int k_gameBtnCount =
    static_cast<int>(sizeof(k_gameBtns) / sizeof(k_gameBtns[0]));

// 热键配置项（key=配置键, label=显示名称）
struct HotkeyEntry
{
    const char *cfgKey;
    const char *label;
};
static const HotkeyEntry k_hotkeys[] = {
    {"handle.fastforward",    "快进"},
    {"handle.rewind",         "倒带"},
    {"hotkey.quicksave.pad",  "快速保存"},
    {"hotkey.quickload.pad",  "快速读取"},
    {"hotkey.menu.pad",       "打开菜单"},
    {"hotkey.mute.pad",       "静音"},
};
static constexpr int k_hotkeyCount =
    static_cast<int>(sizeof(k_hotkeys) / sizeof(k_hotkeys[0]));

brls::ScrollingFrame *SettingPage::buildKeyBindTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

    // 辅助函数：为 DetailCell 注册 A（追加 combo）和 X（清空）动作。
    auto registerKeyBindActions = [](brls::DetailCell* cell, const std::string& cfgKey)
    {
        cell->registerAction("确认"_i18n, brls::BUTTON_A,
            [cell, cfgKey](brls::View*) {
                openKeyCapture([cell, cfgKey](const std::string& r) {
                    if (r.empty()) return;
                    // 追加新 combo（逗号分隔），去重后写回配置
                    std::string cur = cfgGetStr(cfgKey, "none");
                    if (cur.empty() || cur == "none") {
                        cur = r;
                    } else {
                        // 检查是否已存在此 combo
                        bool exists = false;
                        std::istringstream iss(cur);
                        std::string tok;
                        while (std::getline(iss, tok, ',')) {
                            if (tok == r) { exists = true; break; }
                        }
                        if (!exists) cur += "," + r;
                    }
                    cfgSetStr(cfgKey, cur);
                    cell->setDetailText(cur);
                });
                return true;
            }, false, false, brls::SOUND_CLICK);
        cell->registerAction("清除绑定", brls::BUTTON_X,
            [cell, cfgKey](brls::View*) {
                cfgSetStr(cfgKey, "none");
                cell->setDetailText("none");
                return true;
            }, false, false, brls::SOUND_CLICK);
    };

    // ── 游戏按键 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("游戏按键映射（手柄）"));

    for (int i = 0; i < k_gameBtnCount; ++i)
    {
        std::string cfgKey = std::string("handle.") + k_gameBtns[i].suffix;
        auto *cell         = new brls::DetailCell();
        cell->setText(k_gameBtns[i].label);
        cell->setDetailText(cfgGetStr(cfgKey, "none"));
        registerKeyBindActions(cell, cfgKey);
        box->addView(cell);
    }

    // ── 功能热键 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("功能热键绑定"));

    for (int i = 0; i < k_hotkeyCount; ++i)
    {
        std::string cfgKey = k_hotkeys[i].cfgKey;
        auto *cell         = new brls::DetailCell();
        cell->setText(std::string(k_hotkeys[i].label) + "（手柄）");
        cell->setDetailText(cfgGetStr(cfgKey, "none"));
        registerKeyBindActions(cell, cfgKey);
        box->addView(cell);
    }

    // ── 摇杆设置 ──────────────────────────────────────────────────────────────
    box->addView(makeHeader("摇杆设置"));

    auto *joystickCell = new brls::BooleanCell();
    joystickCell->init("启用左摇杆方向键输入",
                       cfgGetBool("input.joystick.enabled", true),
                       [](bool v) { cfgSetBool("input.joystick.enabled", v); });
    box->addView(joystickCell);

    auto *diagonalCell = new brls::BooleanCell();
    diagonalCell->init("允许斜向输入（同时触发 X 和 Y 方向）",
                       cfgGetBool("input.joystick.diagonal", true),
                       [](bool v) { cfgSetBool("input.joystick.diagonal", v); });
    box->addView(diagonalCell);

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tab: 调试工具
// ─────────────────────────────────────────────────────────────────────────────

brls::ScrollingFrame *SettingPage::buildDebugTab()
{
    auto *scroll = makeScrollTab();
    auto *box    = makeContentBox();

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

    scroll->setContentView(box);
    return scroll;
}

// ─────────────────────────────────────────────────────────────────────────────
//  SettingPage
// ─────────────────────────────────────────────────────────────────────────────

SettingPage::SettingPage()
{
    brls::sync([this]()
    {
        this->showHeader(false);
        this->showFooter(false);
        this->showBackground(false);

        m_tabframe = new brls::TabFrame();
        m_tabframe->setGrow(1.0f);
        init();
        this->getContentBox()->addView(m_tabframe);
    });
}

SettingPage::~SettingPage()
{
}

void SettingPage::init()
{
    m_tabframe->addTab("界面",  [this]() { return buildUITab(); });
    m_tabframe->addTab("游戏",  [this]() { return buildGameTab(); });
    m_tabframe->addTab("画面",  [this]() { return buildDisplayTab(); });
    m_tabframe->addTab("音频",  [this]() { return buildAudioTab(); });
    m_tabframe->addTab("按键",  [this]() { return buildKeyBindTab(); });
    m_tabframe->addTab("调试",  [this]() { return buildDebugTab(); });
}

} // namespace beiklive
