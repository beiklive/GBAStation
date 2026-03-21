#include "UI/Utils/GameMenu.hpp"
#include "UI/Pages/FileListPage.hpp"
#include "Video/DisplayConfig.hpp"
#include <cmath>
#include <cstdio>

using beiklive::cfgGetBool;
using beiklive::cfgSetBool;
using beiklive::cfgGetStr;
using beiklive::cfgSetStr;
using beiklive::cfgGetFloat;
using beiklive::cfgGetInt;
using namespace brls::literals; // for _i18n
/// 状态槽位行高（像素）。
static constexpr float STATE_ROW_HEIGHT   = 80.0f;
/// 状态面板右侧预览图区域宽度百分比（相对于面板宽度）。
static constexpr float STATE_PREVIEW_WIDTH_PCT = 50.0f;

// ============================================================
// HideableSliderCell – 修复 SliderCell::getDefaultFocus() 不检查可见性的问题
//
// 问题根因：
//   brls::SliderCell::getDefaultFocus() 直接调用
//   slider->getDefaultFocus()，后者返回内部 pointer Rectangle 指针本身，
//   而不调用 pointer->getDefaultFocus()（不检查 isFocusable()）。
//   因此即使 SliderCell 被设置为 GONE，getDefaultFocus() 仍会返回非 nullptr，
//   导致 Box::getNextFocus() 遍历到隐藏的 SliderCell 就停止，
//   无法继续向上找到 m_filterCell（纹理过滤选项）。
//
// 修复方案：
//   子类化 SliderCell，在 getDefaultFocus() 中先检查自身可见性，
//   若已隐藏（GONE/INVISIBLE）则返回 nullptr，
//   使 Box::getNextFocus() 能跳过隐藏的滑条并正确找到下一个可聚焦视图。
// ============================================================
namespace {
/// 可见性感知滑条单元格：当自身不可见时返回 nullptr，
/// 确保焦点遍历能正确跳过隐藏的滑条。
class HideableSliderCell : public brls::SliderCell
{
public:
    brls::View* getDefaultFocus() override
    {
        if (getVisibility() != brls::Visibility::VISIBLE)
            return nullptr;
        return brls::SliderCell::getDefaultFocus();
    }
};
} // namespace



GameMenu::GameMenu()
{
    bklog::debug("GameMenu constructor");
    #undef ABSOLUTE
    setAxis(brls::Axis::COLUMN);
    setAlignItems(brls::AlignItems::CENTER);
    setJustifyContent(brls::JustifyContent::CENTER);
    setPositionType(brls::PositionType::ABSOLUTE);
    setPositionTop(0);
    setPositionLeft(0);
    setWidthPercentage(100);
    setHeightPercentage(100);
    setBackgroundColor(nvgRGBA(0, 0, 0, 200));

    setHideHighlight(true);
    setHideClickAnimation(true);
    setHideHighlightBorder(true);
    setHideHighlightBackground(true);

    // Button_B：隐藏菜单，返回游戏
    registerAction("beiklive/gamemenu/btn_return_game"_i18n, brls::BUTTON_B, [this](brls::View* v) {
        setVisibility(brls::Visibility::GONE);
        if (m_closeCallback) m_closeCallback();
        return true;
    });

    auto* mainbox = new brls::Box(brls::Axis::ROW);
    mainbox->setWidthPercentage(100.0f);
    mainbox->setGrow(1.0f);
    auto* leftBox = new brls::Box(brls::Axis::COLUMN);
    leftBox->setWidthPercentage(30.0f);
    leftBox->setAlignItems(brls::AlignItems::CENTER);
    auto* rightBox = new brls::Box(brls::Axis::COLUMN);
    rightBox->setWidthPercentage(70.0f);
    rightBox->setPadding(20.0f);

    mainbox->addView(leftBox);
    mainbox->addView(rightBox);
    // 左边栏
    {
        leftBox->addView(new brls::Padding());

        auto* label = new brls::Label();
        label->setText("beiklive/gamemenu/title"_i18n);
        label->setFontSize(40);
        label->setMarginBottom(20.f);

        leftBox->addView(label);

        // 隐藏所有右侧面板的辅助函数
        auto hideAllPanels = [this]() {
            if (m_cheatScrollFrame)
                m_cheatScrollFrame->setVisibility(brls::Visibility::GONE);
            if (m_displayScrollFrame)
                m_displayScrollFrame->setVisibility(brls::Visibility::GONE);
            if (m_saveStatePanel)
                m_saveStatePanel->setVisibility(brls::Visibility::GONE);
            if (m_loadStatePanel)
                m_loadStatePanel->setVisibility(brls::Visibility::GONE);
        };

        // ---- 返回游戏按钮 ----
        auto* btn = new brls::Button();
        btn->setShadowVisibility(false);
        
        btn->setText("beiklive/gamemenu/btn_return_game"_i18n);
        // 返回游戏按钮得到焦点时，隐藏所有面板
        btn->getFocusEvent()->subscribe([hideAllPanels](brls::View*) {
            hideAllPanels();
        });
        btn->registerAction("", brls::BUTTON_A, [this](brls::View* v) {
            // 隐藏整个菜单（this 为 GameMenu，v 为按钮本身）
            setVisibility(brls::Visibility::GONE);
            if (m_closeCallback) m_closeCallback();
            return true;
        });
        // 无子面板，阻止右键导航
        btn->registerAction("", brls::BUTTON_NAV_RIGHT, [](brls::View*) { return true; }, true);
        btn->setWidthPercentage(80.0f);
        leftBox->addView(btn);

        // ---- 保存状态按钮 ----
        auto* btnSaveState = new brls::Button();
        btnSaveState->setText("beiklive/gamemenu/btn_save_state"_i18n);
        btnSaveState->setWidthPercentage(80.0f);
        btnSaveState->setShadowVisibility(false);
        // 保存状态面板：横向容器，左侧条目列表 + 右侧预览图
        m_saveStatePanel = new brls::Box(brls::Axis::ROW);
        m_saveStatePanel->setVisibility(brls::Visibility::GONE);
        m_saveStatePanel->setGrow(1.0f);

        // 左侧：槽位列表（普通 Box）
        m_saveStateItemBox = new brls::Box(brls::Axis::COLUMN);
        // m_saveStateItemBox->setGrow(1.0f);
        // 右侧：预览图区域
        auto* savePreviewBox = new brls::Box(brls::Axis::COLUMN);
        savePreviewBox->setGrow(1.0f);
        savePreviewBox->setAlignItems(brls::AlignItems::CENTER);
        savePreviewBox->setJustifyContent(brls::JustifyContent::CENTER);
        m_savePreviewImage = new brls::Image();
        m_savePreviewImage->setScalingType(brls::ImageScalingType::FIT);
        m_savePreviewImage->setVisibility(brls::Visibility::GONE);
        m_saveNoDataLabel = new brls::Label();
        m_saveNoDataLabel->setText("beiklive/gamemenu/no_state_data"_i18n);
        m_saveNoDataLabel->setVisibility(brls::Visibility::GONE);
        savePreviewBox->addView(m_savePreviewImage);
        savePreviewBox->addView(m_saveNoDataLabel);

        // 构建槽位按钮并绑定焦点事件
        buildStatePanel(true, m_saveStateItemBox);

        m_saveStatePanel->addView(m_saveStateItemBox);
        m_saveStatePanel->addView(savePreviewBox);

        // 保存状态按钮得到焦点时，显示保存状态面板并重置预览图
        btnSaveState->getFocusEvent()->subscribe([this, hideAllPanels](brls::View*) {
            hideAllPanels();
            m_saveStatePanel->setVisibility(brls::Visibility::VISIBLE);
            refreshStatePanels();
        });
        // A 键将焦点转入保存状态槽位列表
        btnSaveState->registerAction("", brls::BUTTON_A, [this](brls::View*) {
            if (m_saveStateItemBox &&
                m_saveStatePanel->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_saveStateItemBox);
            return true;
        });
        btnSaveState->registerAction("", brls::BUTTON_NAV_RIGHT, [this](brls::View*) {
            if (m_saveStateItemBox &&
                m_saveStatePanel->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_saveStateItemBox);
            return true;
        });
        leftBox->addView(btnSaveState);
        rightBox->addView(m_saveStatePanel);

        // ---- 读取状态按钮 ----
        auto* btnLoadState = new brls::Button();
        btnLoadState->setText("beiklive/gamemenu/btn_load_state"_i18n);
        btnLoadState->setWidthPercentage(80.0f);
        btnLoadState->setShadowVisibility(false);

        // 读取状态面板：横向容器，左侧条目列表 + 右侧预览图
        m_loadStatePanel = new brls::Box(brls::Axis::ROW);
        m_loadStatePanel->setVisibility(brls::Visibility::GONE);
        m_loadStatePanel->setGrow(1.0f);

        // 左侧：槽位列表（普通 Box）
        m_loadStateItemBox = new brls::Box(brls::Axis::COLUMN);
        // m_loadStateItemBox->setGrow(1.0f);

        // 右侧：预览图区域
        auto* loadPreviewBox = new brls::Box(brls::Axis::COLUMN);
        loadPreviewBox->setGrow(1.0f);
        loadPreviewBox->setAlignItems(brls::AlignItems::CENTER);
        loadPreviewBox->setJustifyContent(brls::JustifyContent::CENTER);
        m_loadPreviewImage = new brls::Image();
        m_loadPreviewImage->setScalingType(brls::ImageScalingType::FIT);
        m_loadPreviewImage->setVisibility(brls::Visibility::GONE);
        m_loadNoDataLabel = new brls::Label();
        m_loadNoDataLabel->setText("beiklive/gamemenu/no_state_data"_i18n);
        m_loadNoDataLabel->setVisibility(brls::Visibility::GONE);
        loadPreviewBox->addView(m_loadPreviewImage);
        loadPreviewBox->addView(m_loadNoDataLabel);

        // 构建槽位按钮并绑定焦点事件
        buildStatePanel(false, m_loadStateItemBox);

        m_loadStatePanel->addView(m_loadStateItemBox);
        m_loadStatePanel->addView(loadPreviewBox);

        // 读取状态按钮得到焦点时，显示读取状态面板并重置预览图
        btnLoadState->getFocusEvent()->subscribe([this, hideAllPanels](brls::View*) {
            hideAllPanels();
            m_loadStatePanel->setVisibility(brls::Visibility::VISIBLE);
            refreshStatePanels();
        });
        // A 键将焦点转入读取状态槽位列表
        btnLoadState->registerAction("", brls::BUTTON_A, [this](brls::View*) {
            if (m_loadStateItemBox &&
                m_loadStatePanel->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_loadStateItemBox);
            return true;
        });
        btnLoadState->registerAction("", brls::BUTTON_NAV_RIGHT, [this](brls::View*) {
            if (m_loadStateItemBox &&
                m_loadStatePanel->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_loadStateItemBox);
            return true;
        });
        leftBox->addView(btnLoadState);
        rightBox->addView(m_loadStatePanel);

        // ---- 金手指按钮 ----
        auto* btn2 = new brls::Button();
        btn2->setText("beiklive/gamemenu/btn_cheats"_i18n);
        btn2->setWidthPercentage(80.0f);
        btn2->setShadowVisibility(false);
        // 金手指面板：ScrollingFrame 限高，避免内容溢出后焦点丢失
        m_cheatScrollFrame = new brls::ScrollingFrame();
        m_cheatScrollFrame->setVisibility(brls::Visibility::GONE);
        m_cheatScrollFrame->setGrow(1.0f);
        m_cheatScrollFrame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
        m_cheatItemBox = new brls::Box(brls::Axis::COLUMN);
        m_cheatScrollFrame->setContentView(m_cheatItemBox);
        // 金手指按钮得到焦点时，显示金手指面板并隐藏其他面板
        btn2->getFocusEvent()->subscribe([this, hideAllPanels](brls::View*) {
            hideAllPanels();
            m_cheatScrollFrame->setVisibility(brls::Visibility::VISIBLE);
        });
        // A 键将焦点转入金手指面板，使用户可以操作面板内容
        btn2->registerAction("", brls::BUTTON_A, [this](brls::View*) {
            if (m_cheatScrollFrame &&
                m_cheatScrollFrame->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_cheatScrollFrame);
            return true;
        });
        btn2->registerAction("", brls::BUTTON_NAV_RIGHT, [this](brls::View*) {
            if (m_cheatScrollFrame &&
                m_cheatScrollFrame->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_cheatScrollFrame);
            return true;
        });
        leftBox->addView(btn2);
        rightBox->addView(m_cheatScrollFrame);

        // ---- 画面设置按钮 ----
        auto* btnDisplay = new brls::Button();
        btnDisplay->setWidthPercentage(80.0f);
        btnDisplay->setText("beiklive/gamemenu/btn_display"_i18n);
        btnDisplay->setShadowVisibility(false);

        // ---- 构建画面设置面板 ----
        m_displayScrollFrame = new brls::ScrollingFrame();
        m_displayScrollFrame->setVisibility(brls::Visibility::GONE);
        m_displayScrollFrame->setGrow(1.0f);
        m_displayScrollFrame->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

        auto* displayBox = new brls::Box(brls::Axis::COLUMN);
        displayBox->setPadding(10.f, 20.f, 10.f, 20.f);

        // --- 画面设置 header ---
        auto* displayHeader = new brls::Header();
        displayHeader->setTitle("beiklive/gamemenu/header_display"_i18n);
        displayBox->addView(displayHeader);

        // 记录初始显示模式索引，供构造末尾统一更新可见性使用
        // 默认值 2 对应 "original"（索引: 0=fit, 1=fill, 2=original, 3=integer, 4=custom）
        int initDispModeIdx = 2;

        // --- 显示模式 ---
        {
            std::vector<std::string> dispModes = {
                "适应 (Fit)","填充 (Fill)","原始 (Original)","整数倍 (Integer)","自定义 (Custom)"
            };
            static const char* dispModeIds[] = { "fit","fill","original","integer","custom" };
            std::string curMode = cfgGetStr("display.mode","original");
            int dispModeIdx = 2;
            for (int i = 0; i < 5; ++i)
                if (curMode == dispModeIds[i]) { dispModeIdx = i; break; }
            initDispModeIdx = dispModeIdx;
            m_dispModeCell = new brls::SelectorCell();
            m_dispModeCell->init("beiklive/settings/display/mode"_i18n, dispModes, dispModeIdx,
                [this](int idx) {
                    static const char* ids[] = { "fit","fill","original","integer","custom" };
                    if (idx >= 0 && idx < 5) {
                        cfgSetStr("display.mode", ids[idx]);
                        if (!m_romFileName.empty())
                            setGameDataStr(m_romFileName, GAMEDATA_FIELD_DISPLAY_MODE, ids[idx]);
                        if (m_displayModeChangedCallback)
                            m_displayModeChangedCallback(beiklive::DisplayConfig::stringToMode(ids[idx]));
                        // 根据新模式更新位置/缩放区域和整数倍率选项的可见性
                        updateDisplayModeVisibility(idx);
                    }
                });
            displayBox->addView(m_dispModeCell);
        }

        // --- 整数倍缩放倍率 ---
        {
            static const int k_intScaleVals[] = { 0, 1, 2, 3, 4, 5, 6 };
            static const char* k_intScaleLabels[] = {
                "自动 (Auto)", "1x", "2x", "3x", "4x", "5x", "6x"
            };
            static constexpr int k_intScaleCount = 7;
            int curMult = cfgGetInt("display.integer_scale_mult", 0);
            int multIdx = 0;
            for (int i = 0; i < k_intScaleCount; ++i)
                if (curMult == k_intScaleVals[i]) { multIdx = i; break; }
            std::vector<std::string> intScaleLabels(k_intScaleLabels, k_intScaleLabels + k_intScaleCount);
            m_intScaleCell = new brls::SelectorCell();
            m_intScaleCell->init("beiklive/settings/display/int_scale"_i18n, intScaleLabels, multIdx,
                [this](int idx) {
                    static const int vals[] = { 0, 1, 2, 3, 4, 5, 6 };
                    static constexpr int cnt = 7;
                    if (idx >= 0 && idx < cnt && SettingManager) {
                        SettingManager->Set("display.integer_scale_mult",
                                            beiklive::ConfigValue(vals[idx]));
                        SettingManager->Save();
                        if (!m_romFileName.empty())
                            setGameDataInt(m_romFileName, GAMEDATA_FIELD_DISPLAY_INT_SCALE, vals[idx]);
                        if (m_displayIntScaleChangedCallback)
                            m_displayIntScaleChangedCallback(vals[idx]);
                    }
                });
            displayBox->addView(m_intScaleCell);
        }

        // --- 纹理过滤 ---
        {
            std::vector<std::string> filters = { "最近邻 (Nearest)","双线性 (Linear)" };
            std::string curFilter = cfgGetStr("display.filter","nearest");
            m_filterCell = new brls::SelectorCell();
            m_filterCell->init("beiklive/settings/display/filter"_i18n, filters,
                (curFilter == "linear") ? 1 : 0,
                [this](int idx) {
                    const char* fStr = (idx == 1) ? "linear" : "nearest";
                    cfgSetStr("display.filter", fStr);
                    if (!m_romFileName.empty())
                        setGameDataStr(m_romFileName, GAMEDATA_FIELD_DISPLAY_FILTER, fStr);
                    if (m_displayFilterChangedCallback)
                        m_displayFilterChangedCallback(beiklive::DisplayConfig::stringToFilterMode(fStr));
                });
            displayBox->addView(m_filterCell);
        }

        // --- 位置与缩放 header ---
        m_posScaleHeader = new brls::Header();
        m_posScaleHeader->setTitle("beiklive/gamemenu/header_pos_scale"_i18n);
        displayBox->addView(m_posScaleHeader);

        // --- X 坐标偏移滑条 ---
        m_xOffsetSlider = new HideableSliderCell();
        {
            float initX = cfgGetFloat("display.x_offset", 0.f);
            m_xOffsetSlider->init("beiklive/gamemenu/x_offset"_i18n,
                offsetToProgress(initX),
                [this](float p) {
                    float v = progressToOffset(p);
                    if (m_displayXOffsetChangedCallback)
                        m_displayXOffsetChangedCallback(v);
                });
            // 失去焦点时保存到 gamedata 和 SettingManager
            m_xOffsetSlider->slider->getFocusLostEvent()->subscribe([this](brls::View*) {
                float v = progressToOffset(m_xOffsetSlider->slider->getProgress());
                if (SettingManager) {
                    SettingManager->Set("display.x_offset", beiklive::ConfigValue(v));
                    SettingManager->Save();
                }
                if (!m_romFileName.empty())
                    setGameDataFloat(m_romFileName, GAMEDATA_FIELD_X_OFFSET, v);
            });
        }
        displayBox->addView(m_xOffsetSlider);

        // --- Y 坐标偏移滑条 ---
        m_yOffsetSlider = new HideableSliderCell();
        {
            float initY = cfgGetFloat("display.y_offset", 0.f);
            m_yOffsetSlider->init("beiklive/gamemenu/y_offset"_i18n,
                offsetToProgress(initY),
                [this](float p) {
                    float v = progressToOffset(p);
                    if (m_displayYOffsetChangedCallback)
                        m_displayYOffsetChangedCallback(v);
                });
            m_yOffsetSlider->slider->getFocusLostEvent()->subscribe([this](brls::View*) {
                float v = progressToOffset(m_yOffsetSlider->slider->getProgress());
                if (SettingManager) {
                    SettingManager->Set("display.y_offset", beiklive::ConfigValue(v));
                    SettingManager->Save();
                }
                if (!m_romFileName.empty())
                    setGameDataFloat(m_romFileName, GAMEDATA_FIELD_Y_OFFSET, v);
            });
        }
        displayBox->addView(m_yOffsetSlider);

        // --- 自定义缩放滑条 ---
        m_customScaleSlider = new HideableSliderCell();
        {
            float initScale = cfgGetFloat("display.custom_scale", 1.f);
            m_customScaleSlider->init("beiklive/gamemenu/custom_scale"_i18n,
                scaleToProgress(initScale),
                [this](float p) {
                    float v = progressToScale(p);
                    if (m_displayCustomScaleChangedCallback)
                        m_displayCustomScaleChangedCallback(v);
                });
            m_customScaleSlider->slider->getFocusLostEvent()->subscribe([this](brls::View*) {
                float v = progressToScale(m_customScaleSlider->slider->getProgress());
                if (SettingManager) {
                    SettingManager->Set("display.custom_scale", beiklive::ConfigValue(v));
                    SettingManager->Save();
                }
                if (!m_romFileName.empty())
                    setGameDataFloat(m_romFileName, GAMEDATA_FIELD_CUSTOM_SCALE, v);
            });
        }
        displayBox->addView(m_customScaleSlider);

        // --- 遮罩设置 header ---
        auto* overlayHeader = new brls::Header();
        overlayHeader->setTitle("beiklive/settings/display/header_overlay"_i18n);
        displayBox->addView(overlayHeader);

        // --- 遮罩开关 ---
        auto* overlayEnCell = new brls::BooleanCell();
        overlayEnCell->init("beiklive/settings/display/overlay_enable"_i18n,
                            cfgGetBool(KEY_DISPLAY_OVERLAY_ENABLED, false),
                            [this](bool v) {
                                cfgSetBool(KEY_DISPLAY_OVERLAY_ENABLED, v);
                                // 立即通知 GameView 遮罩启用状态已变更
                                if (m_overlayEnabledChangedCallback)
                                    m_overlayEnabledChangedCallback(v);
                            });
        displayBox->addView(overlayEnCell);

        // --- 遮罩路径选择（读/写 gamedataManager 的 overlay 字段）---
        m_overlayPathCell = new brls::DetailCell();
        m_overlayPathCell->setText("beiklive/settings/display/overlay_path"_i18n);
        m_overlayPathCell->setDetailText("beiklive/settings/display/overlay_not_set"_i18n);
        m_overlayPathCell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [this](brls::View*) {
                auto* flPage = new FileListPage();
                flPage->setFilter({"png"}, FileListPage::FilterMode::Whitelist);
                flPage->setDefaultFileCallback([this](const FileListItem& item) {
                    if (!m_romFileName.empty()) {
                        setGameDataStr(m_romFileName, GAMEDATA_FIELD_OVERLAY, item.fullPath);
                    } else {
                        bklog::warning("GameMenu: 游戏文件名未设置，无法保存遮罩路径");
                    }
                    m_overlayPathCell->setDetailText(
                        beiklive::string::extractFileName(item.fullPath));
                    // 通知 GameView 遮罩路径已变更，立即预加载新纹理
                    if (m_overlayChangedCallback)
                        m_overlayChangedCallback(item.fullPath);
                    brls::Application::popActivity();
                    // 显式恢复焦点到遮罩路径单元格，防止焦点残留在文件列表
                    brls::Application::giveFocus(m_overlayPathCell);
                });
                std::string overlayForDir = m_romFileName.empty() ? "" :
                    getGameDataStr(m_romFileName, GAMEDATA_FIELD_OVERLAY, "");
                std::string startPath = beiklive::string::extractDirPath(overlayForDir);
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
        displayBox->addView(m_overlayPathCell);

        // --- 着色器设置 header ---
        auto* shaderHeader = new brls::Header();
        shaderHeader->setTitle("beiklive/settings/display/header_shader"_i18n);
        displayBox->addView(shaderHeader);

        // --- 着色器开关（读取配置，即时回调 GameView）---
        m_shaderEnCell = new brls::BooleanCell();
        m_shaderEnCell->init("beiklive/settings/display/shader_enable"_i18n,
                           cfgGetBool(KEY_DISPLAY_SHADER_ENABLED, false),
                           [this](bool v) {
                               cfgSetBool(KEY_DISPLAY_SHADER_ENABLED, v);
                               if (!m_romFileName.empty())
                                   setGameDataStr(m_romFileName, GAMEDATA_FIELD_SHADER_ENABLED,
                                                  v ? "true" : "false");
                               if (m_shaderEnabledChangedCallback)
                                   m_shaderEnabledChangedCallback(v);
                           });
        displayBox->addView(m_shaderEnCell);

        // --- 着色器路径选择（读取 display.shader 配置，即时回调 GameView）---
        m_shaderPathCell = new brls::DetailCell();
        m_shaderPathCell->setText("beiklive/settings/display/shader_path"_i18n);
        {
            std::string cur = cfgGetStr(KEY_DISPLAY_SHADER_PATH, "");
            m_shaderPathCell->setDetailText(cur.empty()
                ? "beiklive/settings/display/overlay_not_set"_i18n
                : beiklive::string::extractFileName(cur));
        }
        m_shaderPathCell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [this](brls::View*) {
                auto* flPage = new FileListPage();
                flPage->setFilter({"glslp", "glsl"}, FileListPage::FilterMode::Whitelist);
                flPage->setDefaultFileCallback([this](const FileListItem& item) {
                    if (!m_romFileName.empty()) {
                        // 游戏运行中：仅更新游戏专属数据，不污染全局默认路径
                        setGameDataStr(m_romFileName, GAMEDATA_FIELD_SHADER_PATH, item.fullPath);
                    } else {
                        // 无游戏时：更新全局默认着色器路径
                        cfgSetStr(KEY_DISPLAY_SHADER_PATH, item.fullPath);
                    }
                    m_shaderPathCell->setDetailText(
                        beiklive::string::extractFileName(item.fullPath));
                    if (m_shaderPathChangedCallback)
                        m_shaderPathChangedCallback(item.fullPath);
                    brls::Application::popActivity();
                    // 显式恢复焦点到着色器路径单元格，防止焦点残留在文件列表
                    brls::Application::giveFocus(m_shaderPathCell);
                });
                // 使用游戏专属路径确定文件浏览器起始目录，避免跨游戏路径污染
                std::string curShaderPath = getGameDataStr(m_romFileName, GAMEDATA_FIELD_SHADER_PATH, "");
                // 无游戏专属路径时：无游戏运行则回退全局配置，有游戏则从根目录开始
                if (curShaderPath.empty() && m_romFileName.empty())
                    curShaderPath = cfgGetStr(KEY_DISPLAY_SHADER_PATH, "");
                std::string startPath = beiklive::string::extractDirPath(curShaderPath);
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
        displayBox->addView(m_shaderPathCell);

        // --- 着色器参数 box（在着色器路径单元格后，动态填充参数滑条）---
        m_shaderParamBox = new brls::Box(brls::Axis::COLUMN);
        // 默认显示"无可调整参数"提示标签
        {
            auto* noParamLabel = new brls::Label();
            noParamLabel->setText("beiklive/gamemenu/shader_no_params"_i18n);
            noParamLabel->setFontSize(16.f);
            noParamLabel->setMarginTop(4.f);
            noParamLabel->setMarginBottom(4.f);
            m_shaderParamBox->addView(noParamLabel);
        }
        displayBox->addView(m_shaderParamBox);

        // 根据初始显示模式设置可见性（位置/缩放区域和整数倍率选项）
        updateDisplayModeVisibility(initDispModeIdx);

        m_displayScrollFrame->setContentView(displayBox);
        rightBox->addView(m_displayScrollFrame);

        // 画面设置按钮得到焦点时，显示画面设置面板并隐藏其他面板
        btnDisplay->getFocusEvent()->subscribe([this, hideAllPanels](brls::View*) {
            hideAllPanels();
            m_displayScrollFrame->setVisibility(brls::Visibility::VISIBLE);
        });
        // A 键将焦点转入画面设置面板，使用户可以操作面板内容
        btnDisplay->registerAction("", brls::BUTTON_A, [this](brls::View*) {
            if (m_displayScrollFrame &&
                m_displayScrollFrame->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_displayScrollFrame);
            return true;
        });
        btnDisplay->registerAction("", brls::BUTTON_NAV_RIGHT, [this](brls::View*) {
            if (m_displayScrollFrame &&
                m_displayScrollFrame->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_displayScrollFrame);
            return true;
        });
        leftBox->addView(btnDisplay);

        auto* btn3 = new brls::Button();
        btn3->setWidthPercentage(80.0f);
        btn3->setShadowVisibility(false);
        btn3->setText("beiklive/gamemenu/btn_exit_game"_i18n);
        // 退出游戏按钮得到焦点时，隐藏所有面板
        btn3->getFocusEvent()->subscribe([hideAllPanels](brls::View*) {
            hideAllPanels();
        });
        btn3->registerAction("", brls::BUTTON_A, [this](brls::View* v) {
            // 若已设置退出回调（如 GameView 中的自动保存逻辑），则调用回调；
            // 否则直接退出整个 Activity，返回游戏列表
            if (m_exitGameCallback)
                m_exitGameCallback();
            else
                brls::Application::popActivity();
            return true;
        });
        // 无子面板，阻止右键导航
        btn3->registerAction("", brls::BUTTON_NAV_RIGHT, [](brls::View*) { return true; }, true);
        leftBox->addView(btn3);
        leftBox->addView(new brls::Padding());

        // ---- 修复焦点越界问题：最顶部按钮按上键循环到最底部按钮，反之亦然 ----
        btn->setCustomNavigationRoute(brls::FocusDirection::UP, btn3);
        btn3->setCustomNavigationRoute(brls::FocusDirection::DOWN, btn);
    }

    addView(mainbox);

    auto* bottomBar = new brls::BottomBar();
    bottomBar->setWidthPercentage(100.0f);
    addView(bottomBar);

    // 初始化 cheatItemBox：默认提示无金手指
    auto* noCheatLabel = new brls::Label();
    noCheatLabel->setText("beiklive/gamemenu/no_cheats"_i18n);
    m_cheatItemBox->addView(noCheatLabel);
}

GameMenu::~GameMenu()
{
    bklog::debug("GameMenu destructor");
}

// ============================================================
// updateDisplayModeVisibility – 根据显示模式索引更新相关设置项可见性
//
// modeIdx: 0=fit, 1=fill, 2=original, 3=integer, 4=custom
// - custom  模式：显示位置与缩放区域（header + 三个滑条）
// - integer 模式：显示整数倍率选项
// - 其他模式：隐藏上述选项
// ============================================================

void GameMenu::updateDisplayModeVisibility(int modeIdx)
{
    bool isCustom  = (modeIdx == 4); // "custom"
    bool isInteger = (modeIdx == 3); // "integer"

    auto customVis  = isCustom  ? brls::Visibility::VISIBLE : brls::Visibility::GONE;
    auto intVis     = isInteger ? brls::Visibility::VISIBLE : brls::Visibility::GONE;

    if (m_posScaleHeader)    m_posScaleHeader->setVisibility(customVis);
    // HideableSliderCell 已在 getDefaultFocus() 中检查自身可见性（GONE 时返回 nullptr），
    // 确保 Box::getNextFocus() 遍历时能跳过隐藏的滑条，正确找到相邻的可聚焦视图。
    // 额外保留 slider->setFocusable() 调用作为安全措施，
    // 防止外部通过 giveFocus(slider) 直接将焦点交给隐藏的滑条内部视图。
    if (m_xOffsetSlider) {
        m_xOffsetSlider->setVisibility(customVis);
        m_xOffsetSlider->slider->setFocusable(isCustom);
    }
    if (m_yOffsetSlider) {
        m_yOffsetSlider->setVisibility(customVis);
        m_yOffsetSlider->slider->setFocusable(isCustom);
    }
    if (m_customScaleSlider) {
        m_customScaleSlider->setVisibility(customVis);
        m_customScaleSlider->slider->setFocusable(isCustom);
    }
    if (m_intScaleCell)      m_intScaleCell->setVisibility(intVis);
}

// ============================================================
// buildStatePanel – 构建保存/读取状态槽位面板
//
// 为 container 填充 10 个槽位按钮。
// isSave 为 true 时构建保存面板，false 时构建读取面板。
// 每个按钮获得焦点时，通过 m_stateInfoCallback 查询槽位信息
// 并更新右侧预览图（m_save/loadPreviewImage）：
//   - 存档存在且有缩略图 → 显示缩略图
//   - 存档不存在或无缩略图 → 显示 NoData 标签
// ============================================================

void GameMenu::buildStatePanel(bool isSave, brls::Box* container)
{
    container->setWidth(180.f);
    container->setPadding(5.f, 10.f, 5.f, 10.f);
    container->setAlignItems(brls::AlignItems::CENTER);
    container->setJustifyContent(brls::JustifyContent::CENTER);

    brls::Button* firstBtn = nullptr;
    brls::Button* lastBtn  = nullptr;

    for (int slot = 0; slot < 10; ++slot) {
        // 槽位按钮
        auto* btn = new brls::Button();
        btn->setShadowVisibility(false);
        // btn->setHeight(STATE_ROW_HEIGHT);
        // 槽0 为自动档位，槽1-9 显示序号
        std::string slotLabel;
        if (slot == 0) {
            slotLabel = isSave ? "beiklive/gamemenu/save_slot_auto"_i18n
                               : "beiklive/gamemenu/load_slot_auto"_i18n;
        } else {
            const std::string prefix = isSave ? "beiklive/gamemenu/save_slot_prefix"_i18n
                                              : "beiklive/gamemenu/load_slot_prefix"_i18n;
            slotLabel = prefix + std::to_string(slot);
        }
        btn->setText(slotLabel);
        // btn->setGrow(1.f);

        // 获得焦点时更新右侧预览图
        int captSlot = slot;
        btn->getFocusEvent()->subscribe([this, captSlot, isSave](brls::View*) {
            brls::Image* previewImg  = isSave ? m_savePreviewImage  : m_loadPreviewImage;
            brls::Label* noDataLabel = isSave ? m_saveNoDataLabel   : m_loadNoDataLabel;
            if (!previewImg || !noDataLabel) return;

            if (m_stateInfoCallback) {
                auto info = m_stateInfoCallback(captSlot);
                if (info.exists && !info.thumbPath.empty()) {
                    previewImg->setImageFromFile(info.thumbPath);
                    previewImg->setVisibility(brls::Visibility::VISIBLE);
                    noDataLabel->setVisibility(brls::Visibility::GONE);
                    return;
                }
            }
            // 无存档或无缩略图：显示 NoData 标签
            previewImg->setVisibility(brls::Visibility::GONE);
            noDataLabel->setVisibility(brls::Visibility::VISIBLE);
        });

        // A 键：弹出确认对话框，确认后执行存/读档并关闭菜单
        btn->registerAction("", brls::BUTTON_A, [this, captSlot, isSave](brls::View*) {
            std::string confirmMsg = isSave
                ? "beiklive/gamemenu/save_confirm"_i18n
                : "beiklive/gamemenu/load_confirm"_i18n;
            auto* dialog = new brls::Dialog(confirmMsg);
            dialog->addButton("hints/cancel"_i18n, []() {});
            dialog->addButton("beiklive/hints/confirm"_i18n, [this, captSlot, isSave]() {
                // 先触发存/读档回调（GameView 将设置待处理的槽号）
                if (isSave) {
                    if (m_saveStateCallback) m_saveStateCallback(captSlot);
                } else {
                    if (m_loadStateCallback) m_loadStateCallback(captSlot);
                }
                // 关闭菜单并返回游戏
                setVisibility(brls::Visibility::GONE);
                if (m_closeCallback) m_closeCallback();
            });
            dialog->open();
            return true;
        });

        container->addView(btn);

        if (slot == 0) firstBtn = btn;
        lastBtn = btn;
    }

    // 循环导航：首条按上键到末条，末条按下键到首条
    if (firstBtn && lastBtn && firstBtn != lastBtn) {
        firstBtn->setCustomNavigationRoute(brls::FocusDirection::UP, lastBtn);
        lastBtn->setCustomNavigationRoute(brls::FocusDirection::DOWN, firstBtn);
    }
}

// ============================================================
// refreshStatePanels – 重置保存/读取状态面板预览图
//
// 将两个面板的预览图重置为 NoData 状态（隐藏图片，显示提示标签）。
// 当面板重新打开时调用，确保不残留上次的预览内容。
// 须在 UI 线程调用。
// ============================================================

void GameMenu::refreshStatePanels()
{
    // 重置保存状态预览：焦点未进入列表时不显示任何内容
    if (m_savePreviewImage)
        m_savePreviewImage->setVisibility(brls::Visibility::GONE);
    if (m_saveNoDataLabel)
        m_saveNoDataLabel->setVisibility(brls::Visibility::GONE);

    // 重置读取状态预览：焦点未进入列表时不显示任何内容
    if (m_loadPreviewImage)
        m_loadPreviewImage->setVisibility(brls::Visibility::GONE);
    if (m_loadNoDataLabel)
        m_loadNoDataLabel->setVisibility(brls::Visibility::GONE);
}

void GameMenu::setGameFileName(const std::string& fileName)
{
    m_romFileName = fileName;
    if (!m_overlayPathCell) return;

    if (fileName.empty()) {
        // 文件名为空时重置显示，避免显示旧游戏的遮罩信息
        m_overlayPathCell->setDetailText(
            "beiklive/settings/display/overlay_not_set"_i18n);
        return;
    }

    // 从 gamedataManager 读取游戏专属遮罩路径并刷新显示
    std::string overlayPath = getGameDataStr(fileName, GAMEDATA_FIELD_OVERLAY, "");
    if (!overlayPath.empty())
        m_overlayPathCell->setDetailText(
            beiklive::string::extractFileName(overlayPath));
    else
        m_overlayPathCell->setDetailText(
            "beiklive/settings/display/overlay_not_set"_i18n);

    // ── 从 gamedataManager 刷新显示设置（优先 gamedata，回退全局 setting）──
    static const char* dispModeIds[] = { "fit","fill","original","integer","custom" };
    if (m_dispModeCell) {
        std::string mode = getGamedataOrSettingStr(fileName,
            GAMEDATA_FIELD_DISPLAY_MODE, "display.mode", "original");
        int idx = 2;
        for (int i = 0; i < 5; ++i)
            if (mode == dispModeIds[i]) { idx = i; break; }
        m_dispModeCell->setSelection(idx, false);
        // 根据加载到的显示模式更新位置/缩放区域和整数倍率选项的可见性
        updateDisplayModeVisibility(idx);
    }

    if (m_filterCell) {
        std::string filt = getGamedataOrSettingStr(fileName,
            GAMEDATA_FIELD_DISPLAY_FILTER, "display.filter", "nearest");
        m_filterCell->setSelection((filt == "linear") ? 1 : 0, true);
    }

    if (m_intScaleCell) {
        static const int k_vals[] = { 0, 1, 2, 3, 4, 5, 6 };
        int mult = getGamedataOrSettingInt(fileName,
            GAMEDATA_FIELD_DISPLAY_INT_SCALE, "display.integer_scale_mult", 0);
        int idx = 0;
        for (int i = 0; i < 7; ++i)
            if (mult == k_vals[i]) { idx = i; break; }
        m_intScaleCell->setSelection(idx, true);
    }

    // ── 从 gamedataManager 刷新着色器设置（优先 gamedata，回退全局 setting）──
    if (m_shaderEnCell) {
        std::string en = getGamedataOrSettingStr(fileName,
            GAMEDATA_FIELD_SHADER_ENABLED, KEY_DISPLAY_SHADER_ENABLED, "false");
        m_shaderEnCell->setOn(en == "true" || en == "1", false);
    }
    if (m_shaderPathCell) {
        std::string path = getGamedataOrSettingStr(fileName,
            GAMEDATA_FIELD_SHADER_PATH, KEY_DISPLAY_SHADER_PATH, "");
        m_shaderPathCell->setDetailText(path.empty()
            ? "beiklive/settings/display/overlay_not_set"_i18n
            : beiklive::string::extractFileName(path));
    }
}

void GameMenu::setCheats(const std::vector<CheatEntry>& cheats)
{
    m_cheats = cheats;

    if (!m_cheatItemBox) return;

    // 清空旧内容
    m_cheatItemBox->clearViews(true);

    if (m_cheats.empty()) {
        // 无金手指：显示提示标签
        auto* label = new brls::Label();
        label->setText("beiklive/gamemenu/no_cheats"_i18n);
        m_cheatItemBox->addView(label);
        return;
    }

    // 逐条添加金手指 BooleanCell
    brls::BooleanCell* firstCell = nullptr;
    brls::BooleanCell* lastCell  = nullptr;

    for (int i = 0; i < static_cast<int>(m_cheats.size()); ++i) {
        auto* cell = new brls::BooleanCell();
        cell->init(m_cheats[i].desc, m_cheats[i].enabled,
            [this, i](bool v) {
                m_cheats[i].enabled = v;
                if (m_cheatToggleCallback)
                    m_cheatToggleCallback(i, v);
            });
        m_cheatItemBox->addView(cell);
        if (i == 0) firstCell = cell;
        lastCell = cell;
    }

    // 循环导航：首条按上键到末条，末条按下键到首条
    if (firstCell && lastCell && firstCell != lastCell) {
        firstCell->setCustomNavigationRoute(brls::FocusDirection::UP, lastCell);
        lastCell->setCustomNavigationRoute(brls::FocusDirection::DOWN, firstCell);
    }
}


// ============================================================
// updateShaderParams – 更新着色器参数区域（DetailCell + Dropdown）
//
// 清空 m_shaderParamBox 中的旧内容，然后根据 params 列表
// 重新填充 DetailCell。每个 DetailCell 点击后弹出 Dropdown，
// 选项由参数的 min/max/step 计算生成；选择后立即保存并生效。
// 须在 UI（主）线程调用。
// ============================================================

void GameMenu::updateShaderParams(const std::vector<beiklive::ShaderParamInfo>& params)
{
    if (!m_shaderParamBox) return;

    // 保存参数元数据，同时从 gamedataManager 恢复上次保存的值
    m_shaderParams = params;
    if (!m_romFileName.empty()) {
        std::vector<std::string> savedNames;
        std::vector<float>       savedVals;
        if (loadShaderParams(m_romFileName, savedNames, savedVals)) {
            for (size_t si = 0; si < savedNames.size() && si < savedVals.size(); ++si) {
                for (auto& sp : m_shaderParams) {
                    if (sp.name == savedNames[si]) {
                        sp.value = savedVals[si];
                        break;
                    }
                }
            }
        }
    }

    // 清空旧内容
    m_shaderParamBox->clearViews(true);

    if (m_shaderParams.empty()) {
        auto* label = new brls::Label();
        label->setText("beiklive/gamemenu/shader_no_params"_i18n);
        label->setFontSize(16.f);
        label->setMarginTop(4.f);
        label->setMarginBottom(4.f);
        m_shaderParamBox->addView(label);
        return;
    }

    for (size_t pi = 0; pi < m_shaderParams.size(); ++pi) {
        const auto& p = m_shaderParams[pi];
        // 范围检查：若 min >= max 则无意义，跳过
        if (p.maxValue <= p.minValue) continue;

        // 计算步长：若 step <= 0 则默认拆分为 20 步
        float step = (p.step > 0.f) ? p.step : (p.maxValue - p.minValue) / 20.f;

        // 生成选项列表（使用整数计数器避免浮点累积误差，格式化保留4位小数并去除末尾零）
        std::vector<std::string> opts;
        std::vector<float>       optVals;
        {
            int nSteps = static_cast<int>(std::round((p.maxValue - p.minValue) / step)) + 1;
            if (nSteps < 2) nSteps = 2;
            for (int si = 0; si < nSteps; ++si) {
                float v = p.minValue + si * step;
                if (v > p.maxValue + step * 0.001f) break;
                float clamped = std::min(v, p.maxValue);
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.4f", clamped);
                std::string s(buf);
                auto dot = s.find('.');
                if (dot != std::string::npos) {
                    auto last = s.find_last_not_of('0');
                    if (last != std::string::npos && last > dot)
                        s = s.substr(0, last + 1);
                    else if (last == dot)
                        s = s.substr(0, dot);
                }
                opts.push_back(s);
                optVals.push_back(clamped);
            }
        }
        if (opts.empty()) continue;

        // 找到当前值对应的选项索引（最近邻匹配）
        int curIdx = 0;
        float minDiff = std::fabs(optVals[0] - p.value);
        for (int oi = 1; oi < static_cast<int>(optVals.size()); ++oi) {
            float d = std::fabs(optVals[oi] - p.value);
            if (d < minDiff) { minDiff = d; curIdx = oi; }
        }

        auto* cell = new brls::DetailCell();
        std::string labelStr = p.desc.empty() ? p.name : p.desc;
        cell->setText(labelStr);
        cell->setDetailText(opts[static_cast<size_t>(curIdx)]);

        std::string paramName = p.name;
        size_t      paramIdx  = pi;

        cell->registerAction("beiklive/hints/confirm"_i18n, brls::BUTTON_A,
            [this, cell, paramName, paramIdx, opts, optVals, curIdx](brls::View*) mutable {
                auto* dropdown = new brls::Dropdown(
                    paramName,
                    opts,
                    [](int) {}, // 即时选择回调（暂不用）
                    curIdx,
                    [this, cell, paramName, paramIdx, opts, optVals](int sel) {
                        if (sel < 0 || sel >= static_cast<int>(optVals.size())) return;
                        float newVal = optVals[static_cast<size_t>(sel)];
                        cell->setDetailText(opts[static_cast<size_t>(sel)]);
                        // 更新本地参数缓存
                        if (paramIdx < m_shaderParams.size())
                            m_shaderParams[paramIdx].value = newVal;
                        // 立即通知 GameView 应用参数变更
                        if (m_shaderParamChangedCallback)
                            m_shaderParamChangedCallback(paramName, newVal);
                        // 将全部参数保存到 gamedataManager
                        if (!m_romFileName.empty()) {
                            std::vector<std::string> names;
                            std::vector<float>       vals;
                            for (const auto& sp : m_shaderParams) {
                                names.push_back(sp.name);
                                vals.push_back(sp.value);
                            }
                            saveShaderParams(m_romFileName, names, vals);
                        }
                    });
                brls::Application::pushActivity(new brls::Activity(dropdown));
                return true;
            }, false, false, brls::SOUND_CLICK);

        m_shaderParamBox->addView(cell);
    }
}

// ============================================================
// updateDisplayOffsetSliders – 更新 X/Y 偏移和自定义缩放滑条的显示值
// ============================================================

void GameMenu::updateDisplayOffsetSliders(float xOffset, float yOffset, float customScale)
{
    if (m_xOffsetSlider)
        m_xOffsetSlider->slider->setProgress(offsetToProgress(xOffset));
    if (m_yOffsetSlider)
        m_yOffsetSlider->slider->setProgress(offsetToProgress(yOffset));
    if (m_customScaleSlider)
        m_customScaleSlider->slider->setProgress(scaleToProgress(customScale));
}
