#include "UI/Utils/GameMenu.hpp"
#include "UI/Pages/FileListPage.hpp"

using beiklive::cfgGetBool;
using beiklive::cfgSetBool;
using namespace brls::literals; // for _i18n
/// 状态槽位行高（像素）。
static constexpr float STATE_ROW_HEIGHT   = 80.0f;
/// 状态面板右侧预览图区域宽度百分比（相对于面板宽度）。
static constexpr float STATE_PREVIEW_WIDTH_PCT = 50.0f;



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


        // ---- 构建画面设置面板 ----
        m_displayScrollFrame = new brls::ScrollingFrame();
        m_displayScrollFrame->setVisibility(brls::Visibility::GONE);
        m_displayScrollFrame->setGrow(1.0f);
        m_displayScrollFrame->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

        auto* displayBox = new brls::Box(brls::Axis::COLUMN);
        displayBox->setPadding(10.f, 20.f, 10.f, 20.f);

        // --- 遮罩设置 header ---
        auto* overlayHeader = new brls::Header();
        overlayHeader->setTitle("beiklive/settings/display/header_overlay"_i18n);
        displayBox->addView(overlayHeader);

        // --- 遮罩开关 ---
        auto* overlayEnCell = new brls::BooleanCell();
        overlayEnCell->init("beiklive/settings/display/overlay_enable"_i18n,
                            cfgGetBool(KEY_DISPLAY_OVERLAY_ENABLED, false),
                            [](bool v) { cfgSetBool(KEY_DISPLAY_OVERLAY_ENABLED, v); });
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
                });
                std::string startPath = m_romFileName.empty() ? "" :
                    getGameDataStr(m_romFileName, GAMEDATA_FIELD_OVERLAY, "");
                if (!startPath.empty()) {
                    auto pos = startPath.rfind('/');
#ifdef _WIN32
                    auto posW = startPath.rfind('\\');
                    if (posW != std::string::npos &&
                        (pos == std::string::npos || posW > pos))
                        pos = posW;
#endif
                    if (pos != std::string::npos)
                        startPath = startPath.substr(0, pos);
                }
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

        // --- 着色器开关（占位） ---
        auto* shaderEnCell = new brls::BooleanCell();
        shaderEnCell->init("beiklive/settings/display/shader_enable"_i18n, false,
                           [](bool) { /* 功能待实现 */ });
        displayBox->addView(shaderEnCell);

        // --- 着色器选择（占位） ---
        auto* shaderSelectCell = new brls::DetailCell();
        shaderSelectCell->setText("beiklive/settings/display/shader_select"_i18n);
        shaderSelectCell->setDetailText("beiklive/settings/display/not_implemented"_i18n);
        displayBox->addView(shaderSelectCell);

        // --- 着色器参数设置（占位） ---
        auto* shaderParamsBtn = new brls::Button();
        shaderParamsBtn->setText("beiklive/settings/display/shader_params"_i18n);
        displayBox->addView(shaderParamsBtn);

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
        
        btn3->setText("beiklive/gamemenu/btn_exit_game"_i18n);
        // 退出游戏按钮得到焦点时，隐藏所有面板
        btn3->getFocusEvent()->subscribe([hideAllPanels](brls::View*) {
            hideAllPanels();
        });
        btn3->registerAction("", brls::BUTTON_A, [](brls::View* v) {
            // 直接退出整个 Activity，返回游戏列表
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

