#include "GameMenuView.hpp"
#include "core/Tools.hpp"
#include "core/cheat/CheatSystem.hpp"
#include "emulator/mgba_native/MgbaCheatSystem.hpp"
#include "game/control/GameInputManager.hpp"
#include "ui/widget/HintsBar.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "ui/utils/UiHelper.hpp"
#include <algorithm>
#include <filesystem>
#include "borealis/core/cache_helper.hpp"
#include <borealis/views/dialog.hpp>
#include <borealis/views/dropdown.hpp>
#include <borealis/views/cells/cell_bool.hpp>
#include <borealis/views/rectangle.hpp>
#include <cctype>
#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace beiklive
{
    namespace
    {
        bool isMgbaNativePlatform(int platform)
        {
            return beiklive::mgba_native::cheats::IsMgbaPlatform(platform);
        }

        bool isMgbaGbaPlatform(int platform)
        {
            return platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA);
        }

        std::string mgbaCheatCodeTypeLabel(const CheatEntry& cheat)
        {
            std::string codeType = beiklive::mgba_native::cheats::NormalizeCodeType(cheat.codeType);
            if (codeType.empty())
                codeType = beiklive::mgba_native::cheats::DetectCodeType(cheat.code);
            if (codeType.empty())
                codeType = "Auto";
            return codeType;
        }

        std::string cheatRowText(const CheatEntry& cheat, bool showMgbaCodeType)
        {
            if (!showMgbaCodeType || cheat.code.empty())
                return cheat.desc;
            return cheat.desc + " [" + mgbaCheatCodeTypeLabel(cheat) + "]";
        }

        struct NesButtonBindInfo
        {
            const char* label;
            const char* suffix;
            const char* fallback;
        };

        void setDefaultIfMissing(const std::string& key, const std::string& value)
        {
            if (beiklive::SettingManager && !beiklive::SettingManager->Contains(key))
                SET_SETTING_KEY_STR(key.c_str(), value);
        }

        void initNesPlayerDefaults()
        {
            const std::string prefixes[] = {"nes.p1.", "nes.p2."};
            for (const auto& prefix : prefixes)
            {
                setDefaultIfMissing(prefix + "handle.up", "PAD_UP|PAD_LEFTSTICKUP");
                setDefaultIfMissing(prefix + "handle.down", "PAD_DOWN|PAD_LEFTSTICKDOWN");
                setDefaultIfMissing(prefix + "handle.left", "PAD_LEFT|PAD_LEFTSTICKLEFT");
                setDefaultIfMissing(prefix + "handle.right", "PAD_RIGHT|PAD_LEFTSTICKRIGHT");
                setDefaultIfMissing(prefix + "handle.a", "PAD_A");
                setDefaultIfMissing(prefix + "handle.b", "PAD_B");
                setDefaultIfMissing(prefix + "handle.start", "PAD_START");
                setDefaultIfMissing(prefix + "handle.select", "PAD_BACK");
                setDefaultIfMissing(prefix + "handle.fastforward", "none");
                setDefaultIfMissing(prefix + "handle.rewind", "none");
            }
            setDefaultIfMissing("nes.p1.handle.menu", "PAD_LB");
            setDefaultIfMissing("nes.p2.handle.menu", "PAD_RB");
        }

        constexpr NesButtonBindInfo kNesButtonBinds[] = {
            {"上", "up", "PAD_UP|PAD_LEFTSTICKUP"},
            {"下", "down", "PAD_DOWN|PAD_LEFTSTICKDOWN"},
            {"左", "left", "PAD_LEFT|PAD_LEFTSTICKLEFT"},
            {"右", "right", "PAD_RIGHT|PAD_LEFTSTICKRIGHT"},
            {"A", "a", "PAD_A"},
            {"B", "b", "PAD_B"},
            {"start键", "start", "PAD_START"},
            {"select键", "select", "PAD_BACK"},
            {"模拟器菜单", "menu", "PAD_LB"},
            {"快进", "fastforward", "none"},
            {"倒带", "rewind", "none"},
        };

        struct MenuCapturePadKey
        {
            const char* name;
            brls::ControllerButton btn;
        };

        constexpr MenuCapturePadKey kMenuCapturePadKeys[] = {
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

        class MenuKeyCaptureView : public beiklive::Box
        {
        public:
            explicit MenuKeyCaptureView(std::function<void(const std::string&)> onDone,
                                        float countdownSecs = 2.0f)
                : m_onDone(std::move(onDone)), m_countdownSeconds(countdownSecs)
            {
                showFooter(false);
                showHeader(false);
                setFocusable(true);
                getContentBox()->setAxis(brls::Axis::COLUMN);
                getContentBox()->setAlignItems(brls::AlignItems::CENTER);
                getContentBox()->setJustifyContent(brls::JustifyContent::CENTER);
                getContentBox()->setGrow(1.f);

                auto* card = new brls::Box(brls::Axis::COLUMN);
                card->setFocusable(false);
                card->setCornerRadius(16.f);
                card->setBackgroundColor(nvgRGBA(30, 30, 35, 220));
                card->setShadowType(brls::ShadowType::GENERIC);
                card->setShadowVisibility(true);
                card->setAlignItems(brls::AlignItems::CENTER);
                card->setPadding(38.f, 58.f, 38.f, 58.f);
                card->setWidth(540.f);

                auto* title = new brls::Label();
                title->setText("按键监听");
                title->setFontSize(26.f);
                title->setTextColor(GET_THEME_COLOR("brls/text"));
                title->setHorizontalAlign(brls::HorizontalAlign::CENTER);
                title->setMarginBottom(14.f);
                title->setFocusable(false);
                card->addView(title);

                m_promptLabel = new brls::Label();
                m_promptLabel->setText("松开所有按键以开始捕获");
                m_promptLabel->setFontSize(17.f);
                m_promptLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
                m_promptLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
                m_promptLabel->setMarginBottom(16.f);
                m_promptLabel->setFocusable(false);
                card->addView(m_promptLabel);

                m_keyLabel = new brls::Label();
                m_keyLabel->setText("...");
                m_keyLabel->setFontSize(30.f);
                m_keyLabel->setTextColor(nvgRGB(79, 193, 255));
                m_keyLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
                m_keyLabel->setMarginBottom(18.f);
                m_keyLabel->setFocusable(false);
                card->addView(m_keyLabel);

                auto* barRow = new brls::Box(brls::Axis::ROW);
                barRow->setFocusable(false);
                barRow->setAlignItems(brls::AlignItems::CENTER);
                barRow->setJustifyContent(brls::JustifyContent::CENTER);
                barRow->setMarginBottom(8.f);
                m_progressBar = new brls::Rectangle(nvgRGBA(79, 193, 255, 80));
                m_progressBar->setWidth(240.f);
                m_progressBar->setHeight(6.f);
                m_progressBar->setCornerRadius(3.f);
                m_progressBar->setFocusable(false);
                barRow->addView(m_progressBar);
                card->addView(barRow);

                m_countdownLabel = new brls::Label();
                m_countdownLabel->setText("最多 2 个按键");
                m_countdownLabel->setFontSize(14.f);
                m_countdownLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
                m_countdownLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
                m_countdownLabel->setFocusable(false);
                card->addView(m_countdownLabel);

                getContentBox()->addView(card);
                m_startTime = std::chrono::steady_clock::now();

                for (const auto& key : kMenuCapturePadKeys)
                {
                    registerAction("", key.btn,
                        [this, btn = key.btn](brls::View*) -> bool {
                            if (!m_done && !m_waitingForRelease)
                                captureGamepadButton(btn);
                            return true;
                        },
                        true);
                }
            }

            void draw(NVGcontext* vg, float x, float y, float w, float h,
                      brls::Style style, brls::FrameContext* ctx) override
            {
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
                    }
                    else
                    {
                        m_promptLabel->setText("按下要绑定的按键...");
                        pollSticks();
                        const auto now = std::chrono::steady_clock::now();
                        const float elapsed = std::chrono::duration<float>(now - m_startTime).count();
                        const float remaining = m_countdownSeconds - elapsed;
                        if (remaining <= 0.f)
                        {
                            finish(m_captured);
                        }
                        else
                        {
                            std::ostringstream oss;
                            oss << std::fixed << std::setprecision(2) << remaining << " 秒后确认";
                            m_countdownLabel->setText(oss.str());
                            m_progressBar->setWidth(240.f * (remaining / m_countdownSeconds));
                        }
                    }
                }
                beiklive::Box::draw(vg, x, y, w, h, style, ctx);
                if (!m_done)
                    invalidate();
            }

        private:
            std::function<void(const std::string&)> m_onDone;
            float m_countdownSeconds = 2.f;
            brls::Label* m_promptLabel = nullptr;
            brls::Label* m_keyLabel = nullptr;
            brls::Label* m_countdownLabel = nullptr;
            brls::Rectangle* m_progressBar = nullptr;
            std::chrono::steady_clock::time_point m_startTime;
            bool m_done = false;
            bool m_waitingForRelease = true;
            std::vector<std::string> m_capturedKeys;
            std::string m_captured;

            struct StickDir
            {
                const char* name;
                int axis;
                bool positive;
            };
            static const StickDir k_stickDirs[];
            static constexpr int k_stickDirCount = 8;
            bool m_stickPrevActive[k_stickDirCount] = {};

            void captureGamepadButton(brls::ControllerButton btn)
            {
                const char* name = nullptr;
                for (const auto& key : kMenuCapturePadKeys)
                    if (key.btn == btn) { name = key.name; break; }
                if (!name)
                    return;
                if (std::find(m_capturedKeys.begin(), m_capturedKeys.end(), name) != m_capturedKeys.end())
                    return;
                if (m_capturedKeys.size() >= 2)
                    return;
                m_capturedKeys.push_back(name);
                m_captured = buildCombo();
                m_keyLabel->setText(m_captured);
                m_startTime = std::chrono::steady_clock::now();
            }

            void pollSticks()
            {
                auto state = brls::Application::getControllerState();
                for (int i = 0; i < k_stickDirCount; ++i)
                {
                    const float val = k_stickDirs[i].axis < static_cast<int>(brls::_AXES_MAX)
                        ? state.axes[k_stickDirs[i].axis]
                        : 0.f;
                    const bool active = k_stickDirs[i].positive ? val > 0.5f : val < -0.5f;
                    if (active && !m_stickPrevActive[i])
                        captureStick(k_stickDirs[i].name);
                    m_stickPrevActive[i] = active;
                }
            }

            void captureStick(const char* name)
            {
                if (!name)
                    return;
                if (std::find(m_capturedKeys.begin(), m_capturedKeys.end(), name) != m_capturedKeys.end())
                    return;
                if (m_capturedKeys.size() >= 2)
                    return;
                m_capturedKeys.push_back(name);
                m_captured = buildCombo();
                m_keyLabel->setText(m_captured);
                m_startTime = std::chrono::steady_clock::now();
            }

            void checkAllReleased()
            {
                auto state = brls::Application::getControllerState();
                for (const auto& key : kMenuCapturePadKeys)
                {
                    const int idx = static_cast<int>(key.btn);
                    if (idx >= 0 && idx < static_cast<int>(brls::_BUTTON_MAX) && state.buttons[idx])
                        return;
                }
                for (int i = 0; i < k_stickDirCount; ++i)
                {
                    const float val = k_stickDirs[i].axis < static_cast<int>(brls::_AXES_MAX)
                        ? state.axes[k_stickDirs[i].axis]
                        : 0.f;
                    if (std::abs(val) > 0.5f)
                        return;
                }
                m_waitingForRelease = false;
            }

            std::string buildCombo() const
            {
                std::string result;
                for (const auto& key : m_capturedKeys)
                {
                    if (!result.empty())
                        result += "+";
                    result += key;
                }
                return result;
            }

            void finish(const std::string& result)
            {
                if (m_done)
                    return;
                m_done = true;
                if (m_onDone)
                    m_onDone(result);
                brls::delay(300, []() {
                    brls::Application::popActivity(brls::TransitionAnimation::FADE);
                });
            }
        };

        const MenuKeyCaptureView::StickDir MenuKeyCaptureView::k_stickDirs[] = {
            {"PAD_LEFTSTICKUP",     static_cast<int>(brls::LEFT_Y),  false},
            {"PAD_LEFTSTICKDOWN",   static_cast<int>(brls::LEFT_Y),  true },
            {"PAD_LEFTSTICKLEFT",   static_cast<int>(brls::LEFT_X),  false},
            {"PAD_LEFTSTICKRIGHT",  static_cast<int>(brls::LEFT_X),  true },
            {"PAD_RIGHTSTICKUP",    static_cast<int>(brls::RIGHT_Y), false},
            {"PAD_RIGHTSTICKDOWN",  static_cast<int>(brls::RIGHT_Y), true },
            {"PAD_RIGHTSTICKLEFT",  static_cast<int>(brls::RIGHT_X), false},
            {"PAD_RIGHTSTICKRIGHT", static_cast<int>(brls::RIGHT_X), true },
        };

    }

    // 金手指格式转换：只统一行/段分隔符，不破坏核心原生格式。
    // 例如 GBA VBA 的 ":"、GB Game Genie 的 "-" 需要原样交给核心解析。
    std::string convertCheatCode(const std::string& input) {
        std::string result;
        bool lastWasSeparator = true;

        for (unsigned char c : input) {
            const bool separator = std::isspace(c) || c == '+' || c == ',' || c == ';';
            if (separator) {
                if (!result.empty() && !lastWasSeparator)
                    result += '+';
                lastWasSeparator = true;
                continue;
            }
            if (c == '"' || c == '\'')
                continue;
            result += static_cast<char>(c);
            lastWasSeparator = false;
        }

        while (!result.empty() && result.back() == '+')
            result.pop_back();

        return result;
    }
    using beiklive::ui::makeHint;

    GameMenuView::GameMenuView(beiklive::GameEntry gameData)
        : m_gameEntry(std::move(gameData))
    {
        _initLayout();
    }

    GameMenuView::~GameMenuView()
    {
    }

    void GameMenuView::addCoreDisplaySettingView(brls::View* view)
    {
        if (m_coreDisplaySettingsBox && view)
            m_coreDisplaySettingsBox->addView(view);
    }

    void GameMenuView::_initLayout()
    {
        this->setFocusable(false);
        this->setAxis(brls::Axis::COLUMN);
        HIDE_BRLS_HIGHLIGHT(this);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 240));
        this->setWidthPercentage(100.f);
        this->setHeightPercentage(100.f);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setAlignItems(brls::AlignItems::CENTER);

        // 居中面板
        m_panel = new beiklive::TabFrame();
        HIDE_BRLS_HIGHLIGHT(m_panel);

        // BK_RES("img/ui/menu/" + iconPath)
        // 标题
        this->getHeader()->setTitle("游戏菜单");
        // ── 创建 6 个菜单按钮 ──────────────────────────────────────────────
        this->showBackground(false);
        // 1. 返回游戏（无面板）
        m_panel->addTab(
            "返回游戏",
            BK_RES("img/ui/menu/back.png"),
            [this]()
            {
                if (m_onResume)
                    m_onResume();
            });

        m_panel->registerAction("返回", brls::BUTTON_B, [this](brls::View *) -> bool
                                {
            brls::sync([this]() {
                _clearGridItemsFocus();
                if (m_onResume) m_onResume();
            });
            return true; });
        // 2. 保存状态（绑定保存状态面板）
        m_savePanel = _createSaveStatePanel();
        m_panel->addTab(
            "保存状态",
            BK_RES("img/ui/menu/save.png"),
            nullptr,
            nullptr,
            nullptr,
            m_savePanel,
            m_saveGrid->getItemView(0) // 默认聚焦第一个槽位
        );

        // 3. 读取状态（绑定读取状态面板）
        m_loadPanel = _createLoadStatePanel();
        m_panel->addTab(
            "读取状态",
            BK_RES("img/ui/menu/load.png"),
            nullptr,
            nullptr,
            nullptr,
            m_loadPanel,
            m_loadGrid->getItemView(0) // 默认聚焦第一个槽位
        );

        // 4. 金手指设置
        auto *cheatPanel = _createCheatPanel();
        m_panel->addTab(
            "金手指设置",
            BK_RES("img/ui/menu/cheat.png"),
            nullptr, nullptr, nullptr,
            cheatPanel);

        // 5. 画面设置
        auto *displayPanel = _createDisplayPanel();
        m_panel->addTab(
            "画面设置",
            BK_RES("img/ui/menu/display.png"),
            nullptr, nullptr, nullptr,
            displayPanel);

        if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNES))
        {
            auto* controllerPanel = _createControllerPanel();
            m_panel->addTab(
                "手柄",
                BK_RES("img/ui/setting/control.png"),
                nullptr, nullptr, nullptr,
                controllerPanel);
        }

        // 插入分割线
        m_panel->addDivider();

        // TODO 添加重置游戏（重启游戏）功能
        m_panel->addTab(
            "重置游戏",
            BK_RES("img/ui/menu/reset.png"),
            [this]()
            {
                if (m_onReset)
                    m_onReset();
            });

        // 6. 退出游戏（无面板）
        m_panel->addTab(
            "退出游戏",
            BK_RES("img/ui/menu/exit.png"),
            [this]()
            {
                if (m_onExit)
                    m_onExit();
            });

        m_panel->addFinish();
        this->getContentBox()->addView(m_panel);
    }

    // ============================================================
    // slotName 已移至 beiklive::tools::slotName
    std::string GameMenuView::_slotName(int slot)
    {
        return beiklive::tools::slotName(slot);
    }

    // ============================================================
    // _createSaveStatePanel
    // ============================================================

    brls::View *GameMenuView::_createSaveStatePanel()
    {
        auto *wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(wrapper);

        auto *titleLabel = new brls::Label();
        titleLabel->setText("保存状态");
        titleLabel->setFontSize(18.f);
        titleLabel->setMarginBottom(8.f);
        titleLabel->setMarginTop(8.f);
        titleLabel->setMarginLeft(18.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        auto *grid = new beiklive::GridBox(2);
        grid->setGrow(1.f);
        m_saveGrid = grid;

        m_saveItems.clear();
        for (int slot = 0; slot < 10; ++slot)
        {
            auto *item = new beiklive::GridItem(GridItemMode::SAVE_STATE, slot);
            item->setEmpty(_slotName(slot));
            m_saveItems.push_back(item);

            beiklive::GridItem *captItem = item;
            grid->addItem([captItem]() -> brls::View *
                          { return captItem; });
        }
        grid->commit();
        // GridBox 的 onItemClicked 触发确认对话框
        grid->onItemClicked = [this](int slot)
        {
            // auto* dialog = new brls::Dialog("确认保存到" + _slotName(slot) + "？");
            // dialog->addButton("取消", []() {});
            // dialog->addButton("确认", [this, slot]() {
            if (m_saveStateCallback)
                m_saveStateCallback(slot);

            brls::sync([this]()
                       {
                    if (m_onResume) m_onResume(); });

            // dialog->open();
        };
        grid->onItemFocused = [this, grid](int slot)
        {
            grid->setItemIndex(slot);
        };

        grid->registerAction("删除该档位", brls::BUTTON_X, [this, grid](brls::View *view) -> bool
        {
            int slot = grid->getItemIndex();
            if (slot < 0 || slot >= static_cast<int>(m_loadItems.size()))
                return true;
            
            auto* dialog = new brls::Dialog("确定要删除档位" + _slotName(slot) + "?");
            dialog->addButton("取消", []() {});
            dialog->addButton("确认", [this, slot]() {
                if (m_deleteStateCallback)
                    m_deleteStateCallback(slot);
            });
            dialog->open();
            return true; 
        });

        wrapper->addView(grid);
        return wrapper;
    }

    // ============================================================
    // _createLoadStatePanel
    // ============================================================

    brls::View *GameMenuView::_createLoadStatePanel()
    {
        auto *wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(wrapper);

        auto *titleLabel = new brls::Label();
        titleLabel->setText("读取状态");
        titleLabel->setFontSize(18.f);
        titleLabel->setMarginBottom(8.f);
        titleLabel->setMarginTop(8.f);
        titleLabel->setMarginLeft(18.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);

        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        auto *grid = new beiklive::GridBox(2);
        grid->setGrow(1.f);
        m_loadGrid = grid;

        m_loadItems.clear();
        for (int slot = 0; slot < 10; ++slot)
        {
            auto *item = new beiklive::GridItem(GridItemMode::SAVE_STATE, slot);
            item->setEmpty(_slotName(slot));
            m_loadItems.push_back(item);

            beiklive::GridItem *captItem = item;
            grid->addItem([captItem]() -> brls::View *
                          { return captItem; });
        }
        grid->commit();

        grid->onItemClicked = [this](int slot)
        {
            // auto* dialog = new brls::Dialog("确认从" + _slotName(slot) + "读取？");
            // dialog->addButton("取消", []() {});
            // dialog->addButton("确认", [this, slot]() {
            if (m_loadStateCallback)
                m_loadStateCallback(slot);
            brls::sync([this]()
                       {
                    if (m_onResume) m_onResume(); });
            // });
            // dialog->open();
        };
        grid->onItemFocused = [this, grid](int slot)
        {
            grid->setItemIndex(slot);
        };

        grid->registerAction("删除该档位", brls::BUTTON_X, [this, grid](brls::View *view) -> bool
        {
            int slot = grid->getItemIndex();
            if (slot < 0 || slot >= static_cast<int>(m_saveItems.size()))
                return true;
            
            auto* dialog = new brls::Dialog("确定要删除档位" + _slotName(slot) + "?");
            dialog->addButton("取消", []() {});
            dialog->addButton("确认", [this, slot]() {
                if (m_deleteStateCallback)
                    m_deleteStateCallback(slot);
            });
            dialog->open();
            return true; 
        });
        wrapper->addView(grid);
        return wrapper;
    }

    void GameMenuView::_clearGridItemsFocus()
    {
        for (auto *item : m_saveItems)
        {
            if (item)
                item->setFocusable(false);
        }
        for (auto *item : m_loadItems)
        {
            if (item)
                item->setFocusable(false);
        }
    }

    // ============================================================
    // _refreshStatePanel – 异步扫描存档并更新 GridItem 显示
    // ============================================================

    void GameMenuView::_refreshStatePanel(bool isSave)
    {
        if (!m_stateInfoCallback)
            return;

        auto infoCallback = m_stateInfoCallback;

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, infoCallback, isSave]()
                    {
            std::vector<StateSlotInfo> infos;
            infos.reserve(10);
            for (int slot = 0; slot < 10; ++slot)
                infos.push_back(infoCallback(slot));

            std::vector<std::string> refreshPaths;
            refreshPaths.reserve(infos.size());
            for (auto& info : infos)
                if (info.exists && !info.thumbPath.empty())
                    refreshPaths.push_back(info.thumbPath);

            // 将 ASYNC_RELEASE 移入 brls::sync 回调内部，确保在 UI 线程执行时
            // 检查视图是否已销毁，避免 View 析构与 brls::sync 投递之间的竞态条件。
            brls::sync([ASYNC_TOKEN, infos = std::move(infos), refreshPaths = std::move(refreshPaths), isSave]() {
                ASYNC_RELEASE
                for (const auto& path : refreshPaths)
                    g_forceRefreshPaths.insert(path);

                auto& items = isSave ? m_saveItems : m_loadItems;
                for (int slot = 0; slot < 10 && slot < static_cast<int>(items.size()); ++slot)
                {
                    auto* item = items[slot];
                    if (!item) continue;
                    item->setFocusable(true);
                    const auto& info = infos[slot];
                    item->setDataLoaded();
                    item->setTitle(_slotName(slot));
                    if (info.exists)
                    {
                        item->setSubText(info.timeStr.empty() ? "时间未知" : info.timeStr);
                        if (!info.thumbPath.empty())
                        {
                            item->setImagePath(info.thumbPath);
                        }
                    }
                    // else
                    // {
                    //     // item->setEmpty(_slotName(slot));
                    //     item->setDataLoaded();
                    //     item->setTitle(_slotName(slot));
                    //     item->setImagePath(BK_RES("img/ui/menu/empty.png"));
                    // }
                }
            }); });
    }

    void GameMenuView::refreshSlotState(int slot)
    {
        if (slot < 0 || slot >= 10 || !m_stateInfoCallback) return;

        StateSlotInfo info = m_stateInfoCallback(slot);
        if (info.exists && !info.thumbPath.empty())
            g_forceRefreshPaths.insert(info.thumbPath);

        for (auto* items : {&m_saveItems, &m_loadItems}) {
            if (slot >= static_cast<int>(items->size())) continue;
            auto* item = (*items)[slot];
            if (!item) continue;

            item->setFocusable(true);
            item->setDataLoaded();
            item->setTitle(_slotName(slot));
            if (info.exists) {
                item->setSubText(info.timeStr.empty() ? "时间未知" : info.timeStr);
                if (!info.thumbPath.empty())
                    item->setImagePath(info.thumbPath);
            } else {
                item->setImagePath(BK_RES("img/ui/menu/empty.png"));
                item->setSubTextEmpty();
            }
        }
    }

    void GameMenuView::draw(NVGcontext *vg, float x, float y, float w, float h,
                            brls::Style style, brls::FrameContext *ctx)
    {
        Box::draw(vg, x, y, w, h, style, ctx);
    }

    void GameMenuView::onShow()
    {
        _refreshStatePanel(true);
        _refreshStatePanel(false);
        m_panel->onShow();
    }

    std::vector<std::string> GameMenuView::_controllerOptions() const
    {
        int count = GameInputManager::instance().getControllerCount();
        if (count <= 0)
            count = 1;
        std::vector<std::string> options;
        options.reserve(static_cast<size_t>(count));
        for (int i = 0; i < count; ++i)
            options.push_back("手柄 " + std::to_string(i));
        return options;
    }

    brls::View* GameMenuView::_createControllerPanel()
    {
        initNesPlayerDefaults();

        auto* scroll = beiklive::ui::makeScrollTab();
        auto* box = beiklive::ui::makeContentBox();

        box->addView(beiklive::ui::makeHeader("FC 双手柄"));
        auto* enabledCell = new brls::BooleanCell();
        enabledCell->init("开启双打",
                          GameInputManager::instance().isNesDualPlayerEnabled(),
                          [](bool v) {
                              GameInputManager::instance().setNesDualPlayerEnabled(v);
                          });
        box->addView(enabledCell);
        box->addView(beiklive::ui::makeHint("开启双打后按键映射会变为下方的 P1/P2 设置，此设置不会保存，每次打开游戏都要设置一次"));

        auto* testBtn = new beiklive::DetailCell();
        testBtn->setLeftText("测试手柄序号");
        testBtn->setRightText("开始");
        testBtn->registerClickAction([this](brls::View*) -> bool {
            _notifyPressedController();
            return true;
        });
        box->addView(testBtn);
        box->addView(beiklive::ui::makeHint("多手柄按下时会提示手柄序号，用于下方的手柄分配"));

        auto* playersRow = new brls::Box(brls::Axis::ROW);
        playersRow->setWidthPercentage(100.f);
        playersRow->setFocusable(false);
        playersRow->setAlignItems(brls::AlignItems::FLEX_START);
        playersRow->setMarginTop(12.f);
        auto* leftBox = _createNesPlayerBox(0);
        playersRow->addView(leftBox);
        auto* RightBox = _createNesPlayerBox(1);
        playersRow->addView(RightBox);
        box->addView(playersRow);

        scroll->setContentView(box);
        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setVisibility(brls::Visibility::GONE);
        container->setGrow(1.f);
        container->setWidthPercentage(100.f);
        container->addView(scroll);
        return container;
    }

    brls::View* GameMenuView::_createNesPlayerBox(int player)
    {
        const std::string playerName = player == 0 ? "P1" : "P2";
        const std::string prefix = player == 0 ? "nes.p1." : "nes.p2.";
        const int defaultController = player == 0 ? 0 : 1;
        auto* box = new brls::Box(brls::Axis::COLUMN);
        box->setGrow(1.f);
        box->setFocusable(false);
        box->setCornerRadius(8.f);
        box->setBorderThickness(1.f);
        box->setBorderColor(nvgRGBA(255, 255, 255, 50));
        box->setPadding(12.f, 12.f, 12.f, 12.f);
        box->setMarginRight(player == 0 ? 10.f : 0.f);
        box->setMarginLeft(player == 1 ? 10.f : 0.f);

        auto* title = new brls::Label();
        title->setText(playerName);
        title->setFontSize(20.f);
        title->setTextColor(GET_THEME_COLOR("brls/text"));
        title->setMarginBottom(8.f);
        title->setFocusable(false);
        box->addView(title);

        auto* selector = new beiklive::DetailCell();
        selector->setLeftText(playerName + " 手柄");
        auto refreshSelectorText = [this, selector, prefix, defaultController]() {
            auto options = _controllerOptions();
            int selected = GET_SETTING_KEY_INT((prefix + "controller").c_str(), defaultController);
            if (selected < 0 || selected >= static_cast<int>(options.size()))
                selected = 0;
            selector->setRightText(options[selected]);
        };
        refreshSelectorText();
        selector->registerClickAction([this, selector, prefix, playerName, defaultController, refreshSelectorText](brls::View*) -> bool {
            auto options = _controllerOptions();
            int selected = GET_SETTING_KEY_INT((prefix + "controller").c_str(), defaultController);
            if (selected < 0 || selected >= static_cast<int>(options.size()))
                selected = 0;
            auto* dropdown = new brls::Dropdown(
                playerName + " 手柄",
                options,
                [selector, prefix, options](int idx) {
                    if (idx < 0 || idx >= static_cast<int>(options.size()))
                        return;
                    SET_SETTING_KEY_INT((prefix + "controller").c_str(), idx);
                    selector->setRightText(options[idx]);
                },
                selected,
                [refreshSelectorText](int) {
                    refreshSelectorText();
                });
            brls::Application::pushActivity(new brls::Activity(dropdown));
            return true;
        });
        box->addView(selector);

        for (const auto& bind : kNesButtonBinds)
        {
            const std::string cfgKey = prefix + "handle." + bind.suffix;
            std::string fallback = bind.fallback;
            if (std::string(bind.suffix) == "menu")
                fallback = player == 0 ? "PAD_LB" : "PAD_RB";
            auto* cell = new beiklive::DetailCell();
            cell->setLeftText(bind.label);
            cell->setRightText(GET_SETTING_KEY_STR(cfgKey.c_str(), fallback));
            cell->registerClickAction([this, cell, cfgKey](brls::View*) -> bool {
                _openNesKeyCapture(cell, cfgKey);
                return true;
            });
            cell->registerAction("清除绑定", brls::BUTTON_X,
                [cell, cfgKey](brls::View*) -> bool {
                    SET_SETTING_KEY_STR(cfgKey.c_str(), "none");
                    cell->setRightText("none");
                    return true;
                }, false, false, brls::SOUND_CLICK);
            box->addView(cell);
        }

        return box;
    }

    void GameMenuView::_openNesKeyCapture(beiklive::DetailCell* cell, const std::string& cfgKey)
    {
        auto* content = new MenuKeyCaptureView([cell, cfgKey](const std::string& result) {
            if (result.empty())
                return;

            std::string value = GET_SETTING_KEY_STR(cfgKey.c_str(), "none");
            if (value.empty() || value == "none")
            {
                value = result;
            }
            else
            {
                bool exists = false;
                std::istringstream iss(value);
                std::string combo;
                while (std::getline(iss, combo, '|'))
                {
                    if (combo == result)
                    {
                        exists = true;
                        break;
                    }
                }
                if (!exists)
                    value += "|" + result;
            }

            SET_SETTING_KEY_STR(cfgKey.c_str(), value);
            if (cell)
                cell->setRightText(value);
        });
        auto* frame = new brls::AppletFrame(content);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame),
                                        brls::TransitionAnimation::FADE);
    }

    void GameMenuView::_notifyPressedController()
    {
        auto& input = GameInputManager::instance();
        input.setInputEnabled(true);
        input.handleInput();

        int count = input.getControllerCount();
        if (count <= 0)
            count = 1;
        count = std::min(count, GAMEPADS_MAX);

        for (int i = 0; i < count; ++i)
        {
            if (input.getControllerButtonMask(i) != 0)
            {
                brls::Application::notify("触发此按钮的是手柄  " + std::to_string(i));
                return;
            }
        }
        brls::Application::notify("未检测到手柄按键");
    }

    // ============================================================
    // _createCheatPanel
    // ============================================================
    brls::View *GameMenuView::_createCheatPanel()
    {
        // 子界面容器
        auto *wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        wrapper->setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(wrapper);
        // wrapper->setWireframeEnabled(true);

        // 顶栏
        auto *topRow = new brls::Box(brls::Axis::ROW);
        topRow->setFocusable(false);
        topRow->setAlignItems(brls::AlignItems::CENTER);
        topRow->setCornerRadius(10.f);
        topRow->setBorderThickness(1.f);
        topRow->setBorderColor(nvgRGBA(255, 255, 255, 50));
        topRow->setPadding(12.f, 16.f, 8.f, 16.f);
        topRow->setHeight(80.f);
        topRow->setWidthPercentage(100.f);
        topRow->setMarginBottom(10.f);
        // topRow->setWireframeEnabled(true);

        auto* imagefile = new brls::Image();
        imagefile->setWidth(60.f);
        imagefile->setHeight(60.f);
        imagefile->setImageFromFile(BK_RES("img/ui/menu/cheat.png"));
        imagefile->setMarginLeft(5.f);
        imagefile->setMarginRight(10.f);
        imagefile->setScalingType(brls::ImageScalingType::FIT);
        imagefile->setInterpolation(brls::ImageInterpolation::LINEAR);
        topRow->addView(imagefile);

        {

        auto* filenameBox = new brls::Box(brls::Axis::COLUMN);
        filenameBox->setHeightPercentage(100.f);
        filenameBox->setWidth(250.f);
        filenameBox->setFocusable(false);
        filenameBox->setPaddingRight(3.f);
        filenameBox->setMarginRight(3.f);
        filenameBox->setAlignItems(brls::AlignItems::CENTER);

        auto *titleLabel = new brls::Label();
        titleLabel->setText("当前金手指文件");
        titleLabel->setFontSize(13.f);
        titleLabel->setWidth(240.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        titleLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        titleLabel->setMarginRight(10.f);
        titleLabel->setMarginBottom(10.f);
        titleLabel->setMarginTop(10.f);
        titleLabel->setFocusable(false);

        filenameBox->addView(titleLabel);

        cheatPathLabel = new brls::Label();
        cheatPathLabel->setText(beiklive::tools::getFileName(m_gameEntry.cheatPath));
        cheatPathLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        cheatPathLabel->setWidth(240.f);
        cheatPathLabel->setHeight(20.f);
        cheatPathLabel->setFontSize(18.f);
        cheatPathLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        cheatPathLabel->setMarginRight(10.f);
        cheatPathLabel->setFocusable(false);
        cheatPathLabel->setSingleLine(true);
        cheatPathLabel->setAnimated(true);
        cheatPathLabel->setAutoAnimate(true);
        filenameBox->addView(cheatPathLabel);

        filenameBox->setLineRight(1.f);
        filenameBox->setLineColor(nvgRGBA(255, 255, 255, 50));
        topRow->addView(filenameBox);
        }

        {

        auto* filenameBox = new brls::Box(brls::Axis::COLUMN);
        filenameBox->setHeightPercentage(100.f);
        filenameBox->setWidth(120.f);
        filenameBox->setFocusable(false);
        // filenameBox->setMarginRight(10.f);
        filenameBox->setMarginLeft(10.f);
        filenameBox->setAlignItems(brls::AlignItems::CENTER);

        auto *titleLabel = new brls::Label();
        titleLabel->setText("已启用金手指");
        titleLabel->setFontSize(13.f);
        titleLabel->setWidth(110.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        titleLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        titleLabel->setMarginRight(10.f);
        titleLabel->setMarginBottom(10.f);
        titleLabel->setMarginTop(10.f);
        titleLabel->setFocusable(false);

        filenameBox->addView(titleLabel);

        m_cheatCountLabel = new brls::Label();
        m_cheatCountLabel->setText("0 | 0 项");
        m_cheatCountLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        m_cheatCountLabel->setWidth(110.f);
        m_cheatCountLabel->setHeight(20.f);
        m_cheatCountLabel->setFontSize(18.f);
        m_cheatCountLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_cheatCountLabel->setMarginRight(10.f);
        m_cheatCountLabel->setFocusable(false);
        m_cheatCountLabel->setSingleLine(true);
        m_cheatCountLabel->setAnimated(true);
        m_cheatCountLabel->setAutoAnimate(true);
        filenameBox->addView(m_cheatCountLabel);

        topRow->addView(filenameBox);
        }
//




        selectChtBtn = new beiklive::ButtonBox();

        selectChtBtn->setBorderThickness(1.f);
        selectChtBtn->setBorderColor(nvgRGBA(255, 255, 255, 100));
        selectChtBtn->setCornerRadius(5.f);
        selectChtBtn->setWidth(170.f);
        selectChtBtn->setHeight(50.f);
        selectChtBtn->setMarginLeft(5.f);
        selectChtBtn->setMarginTop(10.f);
        selectChtBtn->setText("切换金手指");
        selectChtBtn->setIcon(BK_RES("img/ui/light/wenjian.png"));
        selectChtBtn->setMarginRight(4.f);
        selectChtBtn->registerClickAction([this](brls::View *) -> bool
                                          {
            std::vector<std::string> extensions = {"cht"};
            std::string currentCheatDir;
            std::string currentCheatFile;
            if (!m_gameEntry.cheatPath.empty()) {
                std::filesystem::path currentCheatPath(m_gameEntry.cheatPath);
                currentCheatDir = currentCheatPath.parent_path().string();
                currentCheatFile = currentCheatPath.filename().string();
            }
            beiklive::openFilePicker(extensions,
                [this](const std::string& path) {
                    _loadCheatsFromPath(path);
                    if (m_cheatPathCallback) m_cheatPathCallback(path);
                }, currentCheatDir, currentCheatFile);
            return true; });

        selectChtBtn->setCustomNavigationRoute(brls::FocusDirection::UP, selectChtBtn);

        topRow->addView(selectChtBtn);

        // 新增金手指按钮
        auto* addCheatBtn = new beiklive::ButtonBox();
        addCheatBtn->setBorderThickness(1.f);
        addCheatBtn->setBorderColor(nvgRGBA(255, 255, 255, 100));
        addCheatBtn->setCornerRadius(5.f);
        addCheatBtn->setWidth(170.f);
        addCheatBtn->setHeight(50.f);
        addCheatBtn->setMarginLeft(10.f);
        addCheatBtn->setMarginTop(10.f);
        addCheatBtn->setText("新增金手指");
        addCheatBtn->setIcon(BK_RES("img/ui/menu/cheat.png"));
        addCheatBtn->setMarginRight(4.f);
        addCheatBtn->registerClickAction([this](brls::View *) -> bool {
            if (m_cheatFileReadOnly)
            {
                brls::Application::notify("usrcheat.dat 为只读数据库，请切换到 .cht 后编辑");
                return true;
            }
            auto* ime = brls::Application::getImeManager();
            if (!ime) return true;

            ime->openForText(
                [this](std::string name) {
                    if (name.empty()) return;

                    std::function<void()> promptCode;
                    promptCode = [this, name, &promptCode]() {
                        auto* ime2 = brls::Application::getImeManager();
                        if (!ime2) return;
                        ime2->openForText(
                            [this, name, &promptCode](std::string code) {
                                if (code.empty()) return;
                                // BKTODO beiklive 格式校验还有问题，暂时不校验
                                // if (!isValidCheatCode(code))
                                // {
                                //     brls::Application::notify("金手指代码格式不正确，请重新输入");
                                //     promptCode();
                                //     return;
                                // }
                                code = convertCheatCode(code);
                                CheatEntry entry;
                                entry.desc = name;
                                entry.code = code;
                                entry.enabled = !isMgbaNativePlatform(m_gameEntry.platform);
                                if (isMgbaNativePlatform(m_gameEntry.platform))
                                    entry.codeType = beiklive::mgba_native::cheats::DetectCodeType(entry.code);
                                m_cheats.push_back(entry);
                                _saveEditableCheats();
                                _notifyCheatsChanged();
                                _rebuildCheatItems();
                            },
                            "金手指代码",
                            "",
                            256,
                            "",
                            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                    };
                    promptCode();
                },
                "金手指名称",
                "",
                128,
                "",
                brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            return true;
        });
        topRow->addView(addCheatBtn);

        wrapper->addView(topRow);



        // ====================================
        // 金手指列表
        // ====================================

        auto *itemContainer = new brls::ScrollingFrame();
        itemContainer->setGrow(1.f);
        itemContainer->setCornerRadius(10.f);
        itemContainer->setBorderThickness(1.f);
        itemContainer->setBorderColor(nvgRGBA(255, 255, 255, 50));
        // 金手指网格列表
        m_cheatItemBox = new brls::Box(brls::Axis::COLUMN);
        m_cheatItemBox->setGrow(1.f);
        m_cheatItemBox->setPadding(10.f, 20.f, 10.f, 20.f);

        itemContainer->addView(m_cheatItemBox);
        wrapper->addView(itemContainer);
        // 读取金手指文件
        if (!m_gameEntry.cheatPath.empty())
            _loadCheatsFromPath(m_gameEntry.cheatPath);

        return wrapper;
    }

        void GameMenuView::_loadCheatsFromPath(const std::string &path)
    {
        if (isMgbaNativePlatform(m_gameEntry.platform))
        {
            auto loaded = beiklive::mgba_native::cheats::LoadCheats(path);
            m_cheatFileReadOnly = !loaded.editable;
            m_cheats = std::move(loaded.entries);
        }
        else
        {
            auto loaded = beiklive::cheat::loadCheats({path, m_gameEntry.path, m_gameEntry.platform});
            m_cheatFileReadOnly = !loaded.editable;
            m_cheats = std::move(loaded.entries);
        }
        m_gameEntry.cheatPath = path;
        if (cheatPathLabel)
            cheatPathLabel->setText(beiklive::tools::getFileName(m_gameEntry.cheatPath));
        brls::Logger::info("Loaded {} cheats from {}", m_cheats.size(), path);
        _rebuildCheatItems();
    }

    void GameMenuView::_saveEditableCheats()
    {
        if (!m_cheatFileReadOnly && !m_gameEntry.cheatPath.empty())
        {
            if (isMgbaNativePlatform(m_gameEntry.platform))
            {
                beiklive::mgba_native::cheats::SaveChtFile(m_gameEntry.cheatPath, m_cheats);
            }
            else
            {
                beiklive::saveChtFile(m_gameEntry.cheatPath, m_cheats);
            }
        }
    }

    void GameMenuView::_notifyCheatsChanged()
    {
        if (m_cheatsChangedCallback)
            m_cheatsChangedCallback(m_cheats);
    }

    void GameMenuView::_rebuildCheatItems()
    {
        if (!m_cheatItemBox)
            return;
        m_cheatItemBox->clearViews(true);
        m_cheatSwitches.clear();
        m_cheatSwitches.resize(m_cheats.size(), nullptr);

        if (m_cheats.empty())
        {
            auto *label = new brls::Label();
            label->setText("该金手指文件无有效条目");
            label->setFontSize(14.f);
            label->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
            label->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            label->setFocusable(false);
            m_cheatItemBox->addView(label);
        }
        else
        {
            const bool showMgbaCodeType = isMgbaGbaPlatform(m_gameEntry.platform);
            for (int i = 0; i < (int)m_cheats.size(); ++i)
            {
                if (m_cheats[i].code.empty())
                {
                    auto* catButton = new beiklive::ButtonBox();
                    catButton->setText(m_cheats[i].desc);
                    catButton->setIcon(BK_RES("img/ui/menu/cheat.png"));
                    catButton->setFocusable(false);
                    catButton->setAlpha(0.45f);
                    catButton->setHeight(42.f);
                    catButton->setMarginTop(4.f);
                    catButton->setMarginBottom(4.f);
                    m_cheatItemBox->addView(catButton);
                    continue;
                }

                auto *sw = new SwitchButton();

                DISABLE_LR_NAVIGATION(sw);
                
                sw->registerAction("返回", brls::BUTTON_B, [this](brls::View *) -> bool{
                    brls::Application::giveFocus(selectChtBtn);
                    return true;
                });

                sw->setText(cheatRowText(m_cheats[i], showMgbaCodeType));
                sw->setState(m_cheats[i].enabled);
                int idx = i;
                sw->setOnToggle([this, idx](bool on)
                                {
                    if (idx < (int)m_cheats.size()) {
                        m_cheats[idx].enabled = on;
                        if (on && m_cheats[idx].exclusiveGroup >= 0) {
                            const int group = m_cheats[idx].exclusiveGroup;
                            for (int other = 0; other < (int)m_cheats.size(); ++other) {
                                if (other == idx || m_cheats[other].exclusiveGroup != group)
                                    continue;
                                m_cheats[other].enabled = false;
                                if (other < (int)m_cheatSwitches.size() && m_cheatSwitches[other])
                                    m_cheatSwitches[other]->setState(false);
                            }
                        }
                        brls::Logger::info("GameMenuView: cheat toggle idx={} enabled={} entries={} callback={}",
                                           idx, on, m_cheats.size(), static_cast<bool>(m_cheatToggleCallback));
                        if (m_cheatToggleCallback) m_cheatToggleCallback(idx, on);
                        _updateCheatCount();
                        _saveEditableCheats();
                    } });

                if (showMgbaCodeType)
                {
                    sw->registerAction("码型", brls::BUTTON_LB, [this, idx, sw](brls::View *) -> bool {
                        if (idx >= (int)m_cheats.size())
                            return true;
                        std::string current = beiklive::mgba_native::cheats::NormalizeCodeType(m_cheats[idx].codeType);
                        if (current.empty())
                            current = beiklive::mgba_native::cheats::DetectCodeType(m_cheats[idx].code);
                        m_cheats[idx].codeType = current == "RAW" ? "GS/CB" : "RAW";
                        sw->setText(cheatRowText(m_cheats[idx], true));
                        brls::Logger::info("GameMenuView: mGBA cheat type idx={} type={} code={}",
                                           idx, m_cheats[idx].codeType, m_cheats[idx].code);
                        _saveEditableCheats();
                        _notifyCheatsChanged();
                        brls::Application::notify("码型: " + m_cheats[idx].codeType);
                        return true;
                    });
                }

                // BUTTON_X: 修改金手指代码
                sw->registerAction("修改代码", brls::BUTTON_X, [this, idx](brls::View *) -> bool {
                    if (m_cheatFileReadOnly)
                    {
                        brls::Application::notify("usrcheat.dat 为只读数据库");
                        return true;
                    }
                    if (idx >= (int)m_cheats.size()) return true;
                    std::function<void()> promptCode;
                    promptCode = [this, idx, &promptCode]() {
                        auto* ime = brls::Application::getImeManager();
                        if (!ime) return;
                        if (idx >= (int)m_cheats.size()) return;
                        ime->openForText(
                            [this, idx, &promptCode](std::string code) {
                                if (code.empty()) return;
                                code = convertCheatCode(code);
                                // if (!isValidCheatCode(code))
                                // {
                                //     brls::Application::notify("金手指代码格式不正确，请重新输入");
                                //     promptCode();
                                //     return;
                                // }
                                if (idx >= (int)m_cheats.size()) return;
                                m_cheats[idx].code = code;
                                if (isMgbaNativePlatform(m_gameEntry.platform))
                                    m_cheats[idx].codeType = beiklive::mgba_native::cheats::DetectCodeType(m_cheats[idx].code);
                                _saveEditableCheats();
                                _notifyCheatsChanged();
                                _rebuildCheatItems();
                            },
                            "修改金手指代码",
                            "",
                            256,
                            m_cheats[idx].code,
                            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                    };
                    promptCode();
                    return true;
                });

                // BUTTON_Y: 修改金手指名称
                sw->registerAction("修改名称", brls::BUTTON_Y, [this, idx](brls::View *) -> bool {
                    if (m_cheatFileReadOnly)
                    {
                        brls::Application::notify("usrcheat.dat 为只读数据库");
                        return true;
                    }
                    if (idx >= (int)m_cheats.size()) return true;
                    auto* ime = brls::Application::getImeManager();
                    if (!ime) return true;
                    ime->openForText(
                        [this, idx](std::string name) {
                            if (name.empty()) return;
                            if (idx >= (int)m_cheats.size()) return;
                            m_cheats[idx].desc = name;
                            _saveEditableCheats();
                            _notifyCheatsChanged();
                            _rebuildCheatItems();
                        },
                        "修改金手指名称",
                        "",
                        128,
                        m_cheats[idx].desc,
                        brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                    return true;
                });

                // BUTTON_RT: 删除金手指
                sw->registerAction("删除", brls::BUTTON_RT, [this, idx](brls::View *) -> bool {
                    if (m_cheatFileReadOnly)
                    {
                        brls::Application::notify("usrcheat.dat 为只读数据库");
                        return true;
                    }
                    if (idx >= (int)m_cheats.size()) return true;
                    auto* dlg = new brls::Dialog("是否删除 \"" + m_cheats[idx].desc + "\" ?");
                    dlg->addButton("确认删除", [this, idx]() {
                        if (idx >= (int)m_cheats.size()) return;
                        if (m_cheatToggleCallback)
                            m_cheatToggleCallback(idx, false);
                        m_cheats.erase(m_cheats.begin() + idx);
                        _saveEditableCheats();
                        _notifyCheatsChanged();
                        _rebuildCheatItems();
                    });
                    dlg->addButton("取消", [](){});
                    dlg->open();
                    return true;
                });

                m_cheatSwitches[static_cast<size_t>(i)] = sw;
                m_cheatItemBox->addView(sw);
            }
        }
        _updateCheatCount();
    }

    void GameMenuView::_updateCheatCount()
    {
        if (!m_cheatCountLabel)
            return;
        int total = 0;
        int enabled = 0;
        for (auto &c : m_cheats)
        {
            if (c.code.empty())
                continue;
            ++total;
            if (c.enabled)
                ++enabled;
        }
        m_cheatCountLabel->setText(std::to_string(enabled) + " | " + std::to_string(total) + " 项");
    }

    // ============================================================
    // _createDisplayPanel
    // ============================================================
    brls::View *GameMenuView::_createDisplayPanel()
    {
        auto *wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setWidthPercentage(100.f);
        wrapper->setFocusable(false);

        auto *scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.f);
        scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
        scroll->setFocusable(false);

        auto *box = new brls::Box(brls::Axis::COLUMN);
        box->setPadding(10.f, 20.f, 20.f, 20.f);

        {
            // ── 快进速度快速调整 ──
            auto *ffHdr = new brls::Header();
            ffHdr->setTitle("快进速度");
            box->addView(ffHdr);

            std::vector<std::string> ffLabels = {"0.1倍", "0.5倍", "1倍", "1.25倍", "1.5倍", "1.75倍", "2倍", "3倍", "4倍", "5倍", "6倍", "7倍", "8倍", "9倍", "10倍"};
            static const float ffVals[] = {0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
            float curFF = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
            int ffIdx = 8;
            for (int i = 0; i < 15; ++i)
                if (ffVals[i] == curFF) { ffIdx = i; break; }
            auto *ffCell = new beiklive::SelectorButton();
            ffCell->setText("快进倍率");
            ffCell->setOptions(ffLabels, ffIdx);
            ffCell->setOnSelect(
                [](int i) {
                    static const float vals[] = {0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f, 9.0f, 10.0f};
                    if (i >= 0 && i < 15) SET_SETTING_KEY_FLOAT("fastforward.multiplier", vals[i]);
                });
            box->addView(ffCell);
            box->addView(makeHint("小于1倍时可在快进触发时实现慢动作效果"));

            if (m_gameEntry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGBA)) {
                auto *solarHdr = new brls::Header();
                solarHdr->setTitle("阳光强度");
                box->addView(solarHdr);

                std::vector<std::string> solarLabels = {
                    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10"};
                int curSolar = 5;
                try {
                    curSolar = std::stoi(GET_SETTING_KEY_STR("core.mgba_solar_sensor_level", "5"));
                } catch (...) {
                    curSolar = 5;
                }
                curSolar = std::clamp(curSolar, 0, 10);
                auto *solarCell = new beiklive::SelectorButton();
                solarCell->setText("太阳传感器等级");
                solarCell->setOptions(solarLabels, curSolar);
                solarCell->setOnSelect(
                    [](int idx) {
                        if (idx >= 0 && idx <= 10) {
                            SET_SETTING_KEY_STR("core.mgba_solar_sensor_level", std::to_string(idx));
                            GameSignal::instance().requestConfigUpdate();
                        }
                    });
                box->addView(solarCell);
                box->addView(makeHint("太阳传感器等级，默认设置为5"));
            }

            m_coreDisplaySettingsBox = new brls::Box(brls::Axis::COLUMN);
            box->addView(m_coreDisplaySettingsBox);

            beiklive::SelectorButton *IntegerCell = nullptr;
            brls::DetailCell *customCell = nullptr;

            {
                auto *hdr1 = new brls::Header();
                hdr1->setTitle("画面设置");
                box->addView(hdr1);

                // ScreenMode 枚举值到 UI 索引映射: 0(Fit)→0, 1(Fill)→1, 2(IntegerScale)→4, 3(FreeScale)→5, 4(4:3)→3
                static const int kScreenModeToUi[] = {0, 1, 4, 5, 3};
                int idx = (m_gameEntry.displayMode >= 0 && m_gameEntry.displayMode < 5)
                              ? kScreenModeToUi[m_gameEntry.displayMode] : 2;

                auto *modeCell = new beiklive::SelectorButton();
                IntegerCell = new beiklive::SelectorButton();
                customCell = new brls::DetailCell();
                std::vector<std::string> modes = {"(保持比例)Fit", "(填充)Fill", "(原始)Original", "4:3", "(整数倍)Integer", "(自定义)Custom"};
                std::vector<std::string> modeIds = {"fit", "fill", "original", "four_three", "integer", "custom"};

                IntegerCell->setFocusable(idx == 4);
                IntegerCell->setAlpha(idx == 4 ? 1.0f : 0.3f);
                customCell->setFocusable(idx == 5);
                customCell->setAlpha(idx == 5 ? 1.0f : 0.3f);

                modeCell->setText("画面模式");
                modeCell->setOptions(modes, idx);
                modeCell->setOnSelect(
                    [this, modeIds, IntegerCell, customCell](int selected)
                    {
                        if (selected >= 0 && selected < static_cast<int>(modeIds.size()))
                        {
                            IntegerCell->setFocusable(selected == 4);
                            IntegerCell->setAlpha(selected == 4 ? 1.0f : 0.3f);
                            customCell->setFocusable(selected == 5);
                            customCell->setAlpha(selected == 5 ? 1.0f : 0.3f);
                            static const int kUiToScreenMode[] = {0, 1, 0, 4, 2, 3};
                            m_gameEntry.displayMode = kUiToScreenMode[selected];
                            if (m_displayModeCallback)
                                m_displayModeCallback(modeIds[selected]);
                        }
                    });
                box->addView(modeCell);
            }

            {
                // ── 整数倍缩放 ──
                std::vector<std::string> intScaleLabels = {"自动(auto)", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8"};
                static const int intScaleVals[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
                int curIntScale = static_cast<int>(m_gameEntry.integerAspectRatio);
                int intScaleIdx = 0;
                for (int i = 0; i < 9; ++i)
                    if (intScaleVals[i] == curIntScale)
                    {
                        intScaleIdx = i;
                        break;
                    }
                IntegerCell->setText("整数倍缩放倍率");
                IntegerCell->setOptions(intScaleLabels, intScaleIdx);
                IntegerCell->setOnSelect(
                    [this](int idx)
                    {
                        if (idx >= 0 && idx < 9)
                        {
                            static const int vals[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
                            m_gameEntry.integerAspectRatio = static_cast<float>(vals[idx]);
                            if (m_integerScaleCallback)
                                m_integerScaleCallback(m_gameEntry.integerAspectRatio);
                        }
                    });
                box->addView(IntegerCell);
                box->addView(makeHint("仅在画面模式为整数倍时可用，选择auto则自动匹配最大整数倍"));

                // ── 自定义设置入口 ──
                customCell->setText("自定义设置");
                customCell->setDetailText("\uE14A");
                customCell->registerClickAction([this](brls::View *) -> bool
                                                {
                    _openCustomScaleSettings();
                    return true; });
                box->addView(customCell);
                box->addView(makeHint("仅在画面模式为自定义时可用，调整位置偏移和缩放比例"));
            }
        }

        {
            auto *hdr1 = new brls::Header();
            hdr1->setTitle("个性化设置");
            box->addView(hdr1);
        }

        auto *overlayCell = new brls::DetailCell();
        overlayCell->setText("遮罩设置");
        overlayCell->setDetailText("\uE14A");
        overlayCell->registerClickAction([this](brls::View *) -> bool
                                         {
            _openOverlaySettings();
            return true; });
        box->addView(overlayCell);

        auto *shaderCell = new brls::DetailCell();
        shaderCell->setText("着色器设置");
        shaderCell->setDetailText("\uE14A");
        shaderCell->registerClickAction([this](brls::View *) -> bool
                                        {
            _openShaderSettings();
            return true; });
        box->addView(shaderCell);

        scroll->setContentView(box);
        wrapper->addView(scroll);

        {
            m_ShaderSidePanel = new brls::Box(brls::Axis::COLUMN);
            m_ShaderSidePanel->setHideHighlight(true);
            m_ShaderSidePanel->setPositionType(brls::PositionType::ABSOLUTE);
            m_ShaderSidePanel->setPositionTop(0);
            m_ShaderSidePanel->setPositionLeft(0);
            m_ShaderSidePanel->setWidthPercentage(100.f);
            m_ShaderSidePanel->setHeightPercentage(100.f);
            m_ShaderSidePanel->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
            m_ShaderSidePanel->setFocusable(false);
            m_ShaderSidePanel->setVisibility(brls::Visibility::GONE);

            auto *row = new brls::Box(brls::Axis::ROW);
            row->setGrow(1.f);
            row->setJustifyContent(brls::JustifyContent::FLEX_END);
            row->setFocusable(false);

            auto *panel = new brls::Box(brls::Axis::COLUMN);
            panel->setWidth(420.f);
            panel->setHeightPercentage(100.f);
            panel->setBackgroundColor(nvgRGBA(30, 30, 35, 50));
            panel->setCornerRadius(12.f);
            panel->setPadding(20.f);
            panel->setAlignItems(brls::AlignItems::STRETCH);

            auto closeAct = [this](brls::View *)
            {
                beiklive::GameDB->set(m_gameEntry.path, "shaderEnabled", nlohmann::json(m_gameEntry.shaderEnabled));
                beiklive::GameDB->set(m_gameEntry.path, "shaderPath", nlohmann::json(m_gameEntry.shaderPath));
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaPath", nlohmann::json(m_gameEntry.shaderParaPath));
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaNames", nlohmann::json(m_gameEntry.shaderParaNames));
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaValues", nlohmann::json(m_gameEntry.shaderParaValues));
                beiklive::GameDB->flush();

                {
                    bool enable = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_SHADER, 1) != 0;
                    if(enable){
                        this->showShader(true);
                    }
                }

                _dismissSidePanel(3); return true; };

            auto *hdr = new brls::Header();
            hdr->setTitle("着色器设置");
            panel->addView(hdr);

            bool shaderOn = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, 0) && m_gameEntry.shaderEnabled;

            auto *toggleCell = new brls::BooleanCell();
            DISABLE_LR_NAVIGATION(toggleCell);
            (toggleCell)->setCustomNavigationRoute(brls::FocusDirection::UP, toggleCell);

            toggleCell->init("启用着色器", shaderOn,
                             [this](bool v)
                             {
                                 m_gameEntry.shaderEnabled = v;
                                 if (m_shaderToggleCallback)
                                     m_shaderToggleCallback(v);
                                 _rebuildShaderParamUI();
                             });
            toggleCell->registerAction("关闭", brls::BUTTON_B, closeAct);
            panel->addView(toggleCell);

            auto *hdr2 = new brls::Header();
            hdr2->setTitle("选择着色器文件");
            panel->addView(hdr2);

            shaderPathcell = new brls::DetailCell();
            DISABLE_LR_NAVIGATION(shaderPathcell);

            shaderPathcell->setText("");
            std::string curShader = m_gameEntry.shaderPath;
            shaderPathcell->setDetailText(curShader.empty() ? "未设置" : beiklive::tools::getFileName(curShader));
            shaderPathcell->registerAction("选择", brls::BUTTON_A,
                                     [this](brls::View *) -> bool
                                     {
                                         std::filesystem::path curShaderPath(m_gameEntry.shaderPath);
                                         std::string dir = curShaderPath.parent_path().string();
                                         std::string filename = curShaderPath.filename().string();
                                         beiklive::openFilePicker({"glslp", "glsl"}, [this](const std::string &path)
                                                                  {
                        if (m_gameEntry.shaderPath != path)
                        {
                            m_gameEntry.shaderParaPath.clear();
                            m_gameEntry.shaderParaNames.clear();
                            m_gameEntry.shaderParaValues.clear();
                        }
                        m_gameEntry.shaderPath = path;
                        shaderPathcell->setDetailText(beiklive::tools::getFileName(path));
                        if (m_shaderPathCallback) m_shaderPathCallback(path);
                        _rebuildShaderParamUI(); }, dir, filename);
                                         return true;
                                     });
            shaderPathcell->registerAction("关闭", brls::BUTTON_B, closeAct);
            panel->addView(shaderPathcell);

            auto *div = new brls::Rectangle(nvgRGBA(255, 255, 255, 40));
            div->setWidthPercentage(100.f);
            div->setHeight(1.f);
            div->setMarginTop(12.f);
            div->setMarginBottom(12.f);
            panel->addView(div);

            auto *paramHdr = new brls::Header();
            paramHdr->setTitle("着色器参数");
            panel->addView(paramHdr);
            paramHdr->setMarginBottom(12.f);

            auto *srcollbox = new brls::ScrollingFrame();
            srcollbox->setGrow(1.f);
            srcollbox->setScrollingIndicatorVisible(false);
            m_ShaderParamBox = new brls::Box(brls::Axis::COLUMN);
            m_ShaderParamBox->setPadding(10.f, 10.f, 10.f, 10.f);
            srcollbox->setCornerRadius(10.f);
            srcollbox->setBorderThickness(1.f);
            srcollbox->setBorderColor(nvgRGBA(255, 255, 255, 50));
            srcollbox->addView(m_ShaderParamBox);
            panel->addView(srcollbox);

            _rebuildShaderParamUI();
            // HintsBar 按钮提示栏
            auto *hintsBar = new beiklive::HintsBar();
            panel->addView(hintsBar);

            m_ShaderSidePanel->registerAction("关闭", brls::BUTTON_B, closeAct);
            row->addView(panel);
            m_ShaderSidePanel->addView(row);
            this->addView(m_ShaderSidePanel);
        }

        {
            m_OverlaySidePanel = new brls::Box(brls::Axis::COLUMN);
            m_OverlaySidePanel->setHideHighlight(true);
            m_OverlaySidePanel->setPositionType(brls::PositionType::ABSOLUTE);
            m_OverlaySidePanel->setPositionTop(0);
            m_OverlaySidePanel->setPositionLeft(0);
            m_OverlaySidePanel->setWidthPercentage(100.f);
            m_OverlaySidePanel->setHeightPercentage(100.f);
            m_OverlaySidePanel->setBackgroundColor(nvgRGBA(0, 0, 0, 60));
            m_OverlaySidePanel->setFocusable(false);
            m_OverlaySidePanel->setVisibility(brls::Visibility::GONE);

            auto *row = new brls::Box(brls::Axis::ROW);
            row->setGrow(1.f);
            row->setJustifyContent(brls::JustifyContent::FLEX_END);
            row->setFocusable(false);

            auto *panel = new brls::Box(brls::Axis::COLUMN);
            panel->setWidth(380.f);
            panel->setHeightPercentage(100.f);
            panel->setBackgroundColor(nvgRGBA(30, 30, 35, 50));
            panel->setCornerRadius(12.f);
            panel->setPadding(20.f);
            panel->setAlignItems(brls::AlignItems::STRETCH);

            auto closeAct = [this](brls::View *)
            { _dismissSidePanel(2); return true; };

            auto *hdr = new brls::Header();
            hdr->setTitle("遮罩设置");
            panel->addView(hdr);

            auto *toggleCell = new brls::BooleanCell();
            toggleCell->init("启用遮罩", m_gameEntry.overlayEnabled,
                             [this](bool v)
                             {
                                 m_gameEntry.overlayEnabled = v;
                                 if (m_overlayToggleCallback)
                                     m_overlayToggleCallback(v);
                             });
            DISABLE_LR_NAVIGATION(toggleCell);
            (toggleCell)->setCustomNavigationRoute(brls::FocusDirection::UP, toggleCell);

            toggleCell->registerAction("关闭", brls::BUTTON_B, closeAct);
            panel->addView(toggleCell);

            auto *hdr2 = new brls::Header();
            hdr2->setTitle("选择遮罩图片");
            panel->addView(hdr2);

            auto *pathCell = new brls::DetailCell();
            DISABLE_LR_NAVIGATION(pathCell);
            (pathCell)->setCustomNavigationRoute(brls::FocusDirection::DOWN, pathCell);

            pathCell->setText("");
            pathCell->setDetailText(m_gameEntry.overlayPath.empty() ? "未设置"
                                                                    : beiklive::tools::getFileName(m_gameEntry.overlayPath));
            pathCell->registerAction("选择", brls::BUTTON_A,
                                     [pathCell, this](brls::View *) -> bool
                                     {
                                         std::filesystem::path curOverlayPath(m_gameEntry.overlayPath);
                                         std::string dir = curOverlayPath.parent_path().string();
                                         std::string filename = curOverlayPath.filename().string();
                                         beiklive::openFilePicker({"png"}, [pathCell, this](const std::string &path)
                                                                  {
                        m_gameEntry.overlayPath = path;
                        pathCell->setDetailText(beiklive::tools::getFileName(path));
                        if (m_overlayPathCallback) m_overlayPathCallback(path); }, dir, filename);
                                         return true;
                                     });
            pathCell->registerAction("关闭", brls::BUTTON_B, closeAct);
            panel->addView(pathCell);

            panel->addView(new brls::Padding());

            // HintsBar 按钮提示栏
            auto *hintsBar = new beiklive::HintsBar();
            panel->addView(hintsBar);

            m_OverlaySidePanel->registerAction("关闭", brls::BUTTON_B, closeAct);
            row->addView(panel);
            m_OverlaySidePanel->addView(row);
            this->addView(m_OverlaySidePanel);
        }

        {

            m_CustomSidePanel = new brls::Box(brls::Axis::COLUMN);
            m_CustomSidePanel->setHideHighlight(true);
            m_CustomSidePanel->setPositionType(brls::PositionType::ABSOLUTE);
            m_CustomSidePanel->setPositionTop(0);
            m_CustomSidePanel->setPositionLeft(0);
            m_CustomSidePanel->setWidthPercentage(100.f);
            m_CustomSidePanel->setHeightPercentage(100.f);
            m_CustomSidePanel->setBackgroundColor(nvgRGBA(0, 0, 0, 60));
            m_CustomSidePanel->setFocusable(false);
            m_CustomSidePanel->setVisibility(brls::Visibility::GONE);

            auto *row = new brls::Box(brls::Axis::ROW);
            row->setGrow(1.f);
            row->setJustifyContent(brls::JustifyContent::FLEX_END);
            row->setFocusable(false);

            auto *panel = new brls::Box(brls::Axis::COLUMN);
            panel->setWidth(380.f);
            panel->setHeightPercentage(100.f);
            panel->setBackgroundColor(nvgRGBA(30, 30, 35, 50));
            panel->setCornerRadius(12.f);
            panel->setPadding(20.f);
            panel->setAlignItems(brls::AlignItems::STRETCH);

            auto closeAct = [this](brls::View *)
            { _dismissSidePanel(1); return true; };

            auto *hdr = new brls::Header();
            hdr->setTitle("自定义画面设置");
            panel->addView(hdr);

            // 从 entry 读取当前值
            float initX = m_gameEntry.customOffsetX;
            float initY = m_gameEntry.customOffsetY;
            float initScale = m_gameEntry.customScale > 0.f ? m_gameEntry.customScale : 1.f;

            auto *hdrX = new brls::Header();
            hdrX->setTitle("X轴偏移");
            panel->addView(hdrX);
            auto *xBtn = new beiklive::NumberButton();
            DISABLE_LR_NAVIGATION(xBtn);
            (xBtn)->setCustomNavigationRoute(brls::FocusDirection::UP, xBtn);

            xBtn->setText("");
            xBtn->setValue(initX);
            xBtn->setStep(1.f);
            xBtn->setDecimal(-1);
            xBtn->setOnChange([this](double v)
                              {
            m_gameEntry.customOffsetX = (float)v;
            if (m_customScaleCallback) m_customScaleCallback(m_gameEntry.customOffsetX, m_gameEntry.customOffsetY, m_gameEntry.customScale);
            if (m_displayModeCallback) m_displayModeCallback("custom"); });
            xBtn->registerAction("关闭", brls::BUTTON_B, closeAct);
            panel->addView(xBtn);

            auto *hdrY = new brls::Header();
            hdrY->setTitle("Y轴偏移");
            panel->addView(hdrY);
            auto *yBtn = new beiklive::NumberButton();
            DISABLE_LR_NAVIGATION(yBtn);
            yBtn->setText("");
            yBtn->setValue(initY);
            yBtn->setStep(1.f);
            yBtn->setDecimal(-1);
            yBtn->setOnChange([this](double v)
                              {
            m_gameEntry.customOffsetY = (float)v;
            if (m_customScaleCallback) m_customScaleCallback(m_gameEntry.customOffsetX, m_gameEntry.customOffsetY, m_gameEntry.customScale);
            if (m_displayModeCallback) m_displayModeCallback("custom"); });
            yBtn->registerAction("关闭", brls::BUTTON_B, closeAct);
            panel->addView(yBtn);

            auto *hdrS = new brls::Header();
            hdrS->setTitle("缩放比例");
            panel->addView(hdrS);
            auto *sBtn = new beiklive::NumberButton();
            DISABLE_LR_NAVIGATION(sBtn);
            sBtn->setText("");
            sBtn->setValue(initScale);
            sBtn->setStep(0.1f);
            sBtn->setDecimal(1);
            sBtn->setOnChange([this](double v)
                              {
            m_gameEntry.customScale = (float)v;
            if (m_customScaleCallback) m_customScaleCallback(m_gameEntry.customOffsetX, m_gameEntry.customOffsetY, m_gameEntry.customScale);
            if (m_displayModeCallback) m_displayModeCallback("custom"); });
            sBtn->registerAction("关闭", brls::BUTTON_B, closeAct);
            panel->addView(sBtn);

            // 重置按钮
            auto *resetBtn = new beiklive::ButtonBox();
            DISABLE_LR_NAVIGATION(resetBtn);
            resetBtn->setText("复原");
            resetBtn->setIcon(BK_RES("img/ui/menu/reset.png"));
            resetBtn->registerClickAction([xBtn, yBtn, sBtn, initX, initY, initScale, this](brls::View *) -> bool
                                          {
            xBtn->setValue(initX);
            yBtn->setValue(initY);
            sBtn->setValue(initScale);
            m_gameEntry.customOffsetX = initX;
            m_gameEntry.customOffsetY = initY;
            m_gameEntry.customScale = initScale;
            if (m_customScaleCallback) m_customScaleCallback(initX, initY, initScale);
            if (m_displayModeCallback) m_displayModeCallback("custom");
            return true; });
            panel->addView(resetBtn);

            auto *saveBtn = new beiklive::ButtonBox();
            resetBtn->setIcon(BK_RES("img/ui/menu/save.png"));
            DISABLE_LR_NAVIGATION(saveBtn);
            (saveBtn)->setCustomNavigationRoute(brls::FocusDirection::DOWN, saveBtn);
            saveBtn->setText("保存");
            saveBtn->registerClickAction([closeAct](brls::View *) -> bool
                                         {
            closeAct(nullptr);
            return true; });
            panel->addView(saveBtn);

            m_CustomSidePanel->registerAction("关闭", brls::BUTTON_B, closeAct);
            row->addView(panel);
            m_CustomSidePanel->addView(row);
            this->addView(m_CustomSidePanel);
        }

        // ── 同步设置到其他游戏 ──
        {
            auto *syncHdr = new brls::Header();
            syncHdr->setTitle("同步设置到其他游戏");
            box->addView(syncHdr);

            auto makeSyncBtn = [&](const std::string& text, std::function<void()> action) {
                auto *btn = new brls::DetailCell();
                btn->setText(text);
                btn->registerClickAction([this, action](brls::View*) -> bool {
                    action();
                    return true;
                });
                box->addView(btn);
            };

            makeSyncBtn("同步画面设置", [this]() {
                auto *dlg = new brls::Dialog("同步画面设置\n\n将当前游戏的画面模式、整数倍缩放、自定义偏移和缩放值同步到同平台所有游戏，确认继续？");
                dlg->addButton("取消", []() {});
                dlg->addButton("确认", [this]() { _syncDisplaySettings(); });
                dlg->open();
            });

            makeSyncBtn("同步遮罩路径", [this]() {
                auto *dlg = new brls::Dialog("同步遮罩开关、路径\n\n将当前游戏的遮罩路径同步到同平台所有游戏，同时更新全局默认遮罩路径，确认继续？");
                dlg->addButton("取消", []() {});
                dlg->addButton("确认", [this]() { _syncOverlayPath(); });
                dlg->open();
            });

            makeSyncBtn("同步着色器路径和参数", [this]() {
                auto *dlg = new brls::Dialog("同步着色器开关、路径和参数\n\n将当前游戏的着色器路径和参数同步到同平台所有游戏，同时更新全局默认着色器路径，确认继续？");
                dlg->addButton("取消", []() {});
                dlg->addButton("确认", [this]() { _syncShaderPath(); });
                dlg->open();
            });

            auto *hint = new brls::Label();
            hint->setText("将当前游戏的面板设置应用到同平台所有游戏，同步后自动保存并刷新全局默认值。画面模式=模式+整数倍+自定义偏移/缩放；遮罩=遮罩路径；着色器=GLSLP路径+参数");
            hint->setFontSize(14.f);
            hint->setTextColor(nvgRGB(154, 154, 154));
            hint->setMarginTop(10.f);
            hint->setMarginLeft(20.f);
            hint->setFocusable(false);
            box->addView(hint);
        }

        beiklive::GameDB->upsertByPath(m_gameEntry);
        beiklive::GameDB->flush();
        return wrapper;
    }

    // ============================================================
    // _openCustomScaleSettings - 自定义缩放子界面
    // ============================================================
    void GameMenuView::_openCustomScaleSettings()
    {
        m_CustomSidePanel->setVisibility(brls::Visibility::VISIBLE);
        m_panel->setVisibility(brls::Visibility::GONE);
        this->showHeader(false);
        this->showFooter(false);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 10));

        brls::Application::giveFocus(m_CustomSidePanel);
    }

    // ============================================================
    // _openShaderSettings – 着色器设置侧边栏
    // ============================================================
    void GameMenuView::_openShaderSettings()
    {
        _rebuildShaderParamUI();
        m_ShaderSidePanel->setVisibility(brls::Visibility::VISIBLE);
        m_panel->setVisibility(brls::Visibility::GONE);
        this->showHeader(false);
        this->showFooter(false);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        {
            bool enable = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_SHADER, 1) != 0;
            if(enable){
                this->showShader(false);
            }
            enable = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_SHOW_BG_IMAGE, 1) != 0;
            if(enable){
                this->showBackground(false);
            }
        }

        brls::Application::giveFocus(m_ShaderSidePanel);
    }

    // ============================================================
    // _openOverlaySettings – 遮罩设置侧边栏
    // ============================================================
    void GameMenuView::_openOverlaySettings()
    {
        m_OverlaySidePanel->setVisibility(brls::Visibility::VISIBLE);
        m_panel->setVisibility(brls::Visibility::GONE);
        this->showHeader(false);
        this->showFooter(false);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 10));

        brls::Application::giveFocus(m_OverlaySidePanel);
    }

    void GameMenuView::_dismissSidePanel(int idx)
    {

        switch (idx)
        {
        case 1:
            m_CustomSidePanel->setVisibility(brls::Visibility::GONE);
            break;
        case 2:
            m_OverlaySidePanel->setVisibility(brls::Visibility::GONE);
            break;
        case 3:
            m_ShaderSidePanel->setVisibility(brls::Visibility::GONE);
            break;
        default:
            // 处理所有侧边栏
            if (m_CustomSidePanel)
                m_CustomSidePanel->setVisibility(brls::Visibility::GONE);
            if (m_OverlaySidePanel)
                m_OverlaySidePanel->setVisibility(brls::Visibility::GONE);
            if (m_ShaderSidePanel)
                m_ShaderSidePanel->setVisibility(brls::Visibility::GONE);
            break;
        }

        m_panel->setVisibility(brls::Visibility::VISIBLE);
        this->showHeader(true);
        this->showFooter(true);
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 240));
        brls::Application::giveFocus(m_panel->getDefaultFocus());
    }

    void GameMenuView::_rebuildShaderParamUI()
    {
        if (!m_ShaderParamBox || !m_shaderParamsCallback)
            return;

        m_ShaderParamBox->clearViews(true);

        auto params = m_shaderParamsCallback();

        if (params.empty())
            return;

        bool isNewParams = false;
        if (params.size() != m_gameEntry.shaderParaNames.size() ||
            params.size() != m_gameEntry.shaderParaValues.size())
        {
            m_gameEntry.shaderParaNames.clear();
            m_gameEntry.shaderParaValues.clear();
            m_gameEntry.shaderParaPath = m_gameEntry.shaderPath;
            isNewParams = true;
        }
        else
        {
            for (size_t i = 0; i < params.size(); ++i)
            {
                if (params[i].name != m_gameEntry.shaderParaNames[i])
                {
                    m_gameEntry.shaderParaNames.clear();
                    m_gameEntry.shaderParaValues.clear();
                    m_gameEntry.shaderParaPath = m_gameEntry.shaderPath;
                    isNewParams = true;
                    break;
                }
            }
        }

        if (!isNewParams && m_gameEntry.shaderParaPath != m_gameEntry.shaderPath)
        {
            m_gameEntry.shaderParaNames.clear();
            m_gameEntry.shaderParaValues.clear();
            m_gameEntry.shaderParaPath = m_gameEntry.shaderPath;
            isNewParams = true;
        }

        int idx = 0;
        for (const auto &p : params)
        {
            auto *btn = new beiklive::NumberButton();
            DISABLE_LR_NAVIGATION(btn);

            btn->registerAction("返回", brls::BUTTON_B, [this](brls::View *) {
                brls::Application::giveFocus(shaderPathcell);
                return true;
            });

            btn->setText(p.desc);

            if (isNewParams)
            {
                m_gameEntry.shaderParaNames.push_back(p.name);
                m_gameEntry.shaderParaValues.push_back(p.value);
            }

            btn->setValue(static_cast<double>(m_gameEntry.shaderParaValues[idx]));
            btn->setStep(static_cast<double>(p.step));
            btn->setDecimal(2);

            std::string pname = m_gameEntry.shaderParaNames[idx];
            btn->setOnChange([this, pname, idx](double v) {
                m_gameEntry.shaderParaValues[idx] = static_cast<float>(v);
                if (m_shaderParamCallback) m_shaderParamCallback(pname, static_cast<float>(v));
            });

            m_ShaderParamBox->addView(btn);
            ++idx;
        }
    }

    // ============================================================
    // 同步设置到同平台其他游戏
    // ============================================================

    std::string GameMenuView::_getPlatformOverlayKey() const {
        return beiklive::tools::platformOverlayKey(m_gameEntry.platform);
    }

    std::string GameMenuView::_getPlatformShaderKey() const {
        return beiklive::tools::platformShaderKey(m_gameEntry.platform);
    }

    void GameMenuView::_syncDisplaySettings() {
        int platform = m_gameEntry.platform;
        auto games = beiklive::GameDB->getAll();
        int count = 0;
        for (auto& game : games) {
            if (game.platform != platform) continue;
            if (game.path == m_gameEntry.path) continue;
            game.displayMode      = m_gameEntry.displayMode;
            game.integerAspectRatio = m_gameEntry.integerAspectRatio;
            game.customScale      = m_gameEntry.customScale;
            game.customOffsetX    = m_gameEntry.customOffsetX;
            game.customOffsetY    = m_gameEntry.customOffsetY;
            game.ndsBottomOpacity = m_gameEntry.ndsBottomOpacity;
            beiklive::GameDB->upsertByPath(game);
            ++count;
        }
        beiklive::GameDB->flush();

        auto *dlg = new brls::Dialog("同步完成\n\n已同步画面设置到 " + std::to_string(count) + " 个游戏");
        dlg->addButton("确定", []() {});
        dlg->open();
    }

    void GameMenuView::_syncOverlayPath() {
        int platform = m_gameEntry.platform;
        auto games = beiklive::GameDB->getAll();
        int count = 0;
        for (auto& game : games) {
            if (game.platform != platform) continue;
            if (game.path == m_gameEntry.path) continue;
            game.overlayPath    = m_gameEntry.overlayPath;
            game.overlayEnabled = m_gameEntry.overlayEnabled;
            beiklive::GameDB->upsertByPath(game);
            ++count;
        }
        // 更新全局默认遮罩路径
        std::string key = _getPlatformOverlayKey();
        if (!key.empty())
            SET_SETTING_KEY_STR(key.c_str(), m_gameEntry.overlayPath);

        beiklive::GameDB->flush();

        auto *dlg = new brls::Dialog("同步完成\n\n已同步遮罩路径到 " + std::to_string(count) + " 个游戏");
        dlg->addButton("确定", []() {});
        dlg->open();
    }

    void GameMenuView::_syncShaderPath() {
        int platform = m_gameEntry.platform;
        auto games = beiklive::GameDB->getAll();
        int count = 0;
        for (auto& game : games) {
            if (game.platform != platform) continue;
            if (game.path == m_gameEntry.path) continue;
            game.shaderEnabled   = m_gameEntry.shaderEnabled;
            game.shaderPath      = m_gameEntry.shaderPath;
            game.shaderParaPath   = m_gameEntry.shaderParaPath;
            game.shaderParaNames  = m_gameEntry.shaderParaNames;
            game.shaderParaValues = m_gameEntry.shaderParaValues;
            beiklive::GameDB->upsertByPath(game);
            ++count;
        }
        // 更新全局默认着色器路径
        std::string key = _getPlatformShaderKey();
        if (!key.empty())
            SET_SETTING_KEY_STR(key.c_str(), m_gameEntry.shaderPath);

        beiklive::GameDB->flush();

        auto *dlg = new brls::Dialog("同步完成\n\n已同步着色器路径和参数到 " + std::to_string(count) + " 个游戏");
        dlg->addButton("确定", []() {});
        dlg->open();
    }

} // namespace beiklive
