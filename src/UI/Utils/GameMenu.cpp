#include "UI/Utils/GameMenu.hpp"
#include "UI/Pages/FileListPage.hpp"

using beiklive::cfgGetBool;
using beiklive::cfgSetBool;
using namespace brls::literals; // for _i18n
/// 金手指滚动面板最大高度（像素）。
static constexpr float CHEAT_SCROLL_HEIGHT   = 400.0f;
/// 画面设置面板最大高度（像素）。
static constexpr float DISPLAY_SCROLL_HEIGHT = 400.0f;
/// 状态槽位缩略图宽度（像素）。
static constexpr float STATE_THUMB_WIDTH  = 120.0f;
/// 状态槽位缩略图高度（像素）。
static constexpr float STATE_THUMB_HEIGHT = 75.0f;
/// 状态槽位行高（像素）。
static constexpr float STATE_ROW_HEIGHT   = 80.0f;



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
    registerAction("返回游戏", brls::BUTTON_B, [this](brls::View* v) {
        setVisibility(brls::Visibility::GONE);
        if (m_closeCallback) m_closeCallback();
        return true;
    });

    auto* mainbox = new brls::Box(brls::Axis::ROW);
    mainbox->setWidthPercentage(100.0f);
    mainbox->setGrow(1.0f);
    auto* leftBox = new brls::Box(brls::Axis::COLUMN);
    leftBox->setWidthPercentage(30.0f);
    auto* rightBox = new brls::Box(brls::Axis::COLUMN);
    rightBox->setWidthPercentage(70.0f);
    mainbox->addView(leftBox);
    mainbox->addView(rightBox);
    // 左边栏
    {
        auto* label = new brls::Label();
        label->setText("Game Menu");
        leftBox->addView(label);
        leftBox->addView(new brls::Padding());

        // 隐藏所有右侧面板的辅助函数
        auto hideAllPanels = [this]() {
            if (m_cheatScrollFrame)
                m_cheatScrollFrame->setVisibility(brls::Visibility::GONE);
            if (m_displayScrollFrame)
                m_displayScrollFrame->setVisibility(brls::Visibility::GONE);
            if (m_saveStateScrollFrame)
                m_saveStateScrollFrame->setVisibility(brls::Visibility::GONE);
            if (m_loadStateScrollFrame)
                m_loadStateScrollFrame->setVisibility(brls::Visibility::GONE);
        };

        // ---- 返回游戏按钮 ----
        auto* btn = new brls::Button();
        btn->setText("返回游戏");
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
        leftBox->addView(btn);

        // ---- 保存状态按钮 ----
        auto* btnSaveState = new brls::Button();
        btnSaveState->setText("beiklive/gamemenu/btn_save_state"_i18n);

        // 保存状态面板：ScrollingFrame 包含 10 个槽位行
        m_saveStateScrollFrame = new brls::ScrollingFrame();
        m_saveStateScrollFrame->setVisibility(brls::Visibility::GONE);
        m_saveStateScrollFrame->setGrow(1.0f);
        m_saveStateScrollFrame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
        m_saveStateItemBox = new brls::Box(brls::Axis::COLUMN);
        buildStatePanel(true, m_saveStateItemBox, m_saveThumbImages);
        m_saveStateScrollFrame->setContentView(m_saveStateItemBox);

        // 保存状态按钮得到焦点时，显示保存状态面板并刷新缩略图
        btnSaveState->getFocusEvent()->subscribe([this, hideAllPanels](brls::View*) {
            hideAllPanels();
            m_saveStateScrollFrame->setVisibility(brls::Visibility::VISIBLE);
            refreshStatePanels();
        });
        // A 键将焦点转入保存状态面板
        btnSaveState->registerAction("", brls::BUTTON_A, [this](brls::View*) {
            if (m_saveStateScrollFrame &&
                m_saveStateScrollFrame->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_saveStateScrollFrame);
            return true;
        });
        leftBox->addView(btnSaveState);
        rightBox->addView(m_saveStateScrollFrame);

        // ---- 读取状态按钮 ----
        auto* btnLoadState = new brls::Button();
        btnLoadState->setText("beiklive/gamemenu/btn_load_state"_i18n);

        // 读取状态面板：ScrollingFrame 包含 10 个槽位行
        m_loadStateScrollFrame = new brls::ScrollingFrame();
        m_loadStateScrollFrame->setVisibility(brls::Visibility::GONE);
        m_loadStateScrollFrame->setGrow(1.0f);
        m_loadStateScrollFrame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
        m_loadStateItemBox = new brls::Box(brls::Axis::COLUMN);
        buildStatePanel(false, m_loadStateItemBox, m_loadThumbImages);
        m_loadStateScrollFrame->setContentView(m_loadStateItemBox);

        // 读取状态按钮得到焦点时，显示读取状态面板并刷新缩略图
        btnLoadState->getFocusEvent()->subscribe([this, hideAllPanels](brls::View*) {
            hideAllPanels();
            m_loadStateScrollFrame->setVisibility(brls::Visibility::VISIBLE);
            refreshStatePanels();
        });
        // A 键将焦点转入读取状态面板
        btnLoadState->registerAction("", brls::BUTTON_A, [this](brls::View*) {
            if (m_loadStateScrollFrame &&
                m_loadStateScrollFrame->getVisibility() == brls::Visibility::VISIBLE)
                brls::Application::giveFocus(m_loadStateScrollFrame);
            return true;
        });
        leftBox->addView(btnLoadState);
        rightBox->addView(m_loadStateScrollFrame);

        // ---- 金手指按钮 ----
        auto* btn2 = new brls::Button();
        btn2->setText("金手指");
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

        leftBox->addView(btn2);
        rightBox->addView(m_cheatScrollFrame);

        // ---- 画面设置按钮 ----
        auto* btnDisplay = new brls::Button();
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
        leftBox->addView(btnDisplay);

        auto* btn3 = new brls::Button();
        btn3->setText("退出游戏");
        // 退出游戏按钮得到焦点时，隐藏所有面板
        btn3->getFocusEvent()->subscribe([hideAllPanels](brls::View*) {
            hideAllPanels();
        });
        btn3->registerAction("", brls::BUTTON_A, [](brls::View* v) {
            // 直接退出整个 Activity，返回游戏列表
            brls::Application::popActivity();
            return true;
        });
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
    noCheatLabel->setText("无金手指");
    m_cheatItemBox->addView(noCheatLabel);
}

GameMenu::~GameMenu()
{
    bklog::debug("GameMenu destructor");
}

// ============================================================
// buildStatePanel – 构建保存/读取状态槽位面板
//
// 为 container 填充 10 个槽位行，每行包含：
//   - 缩略图（初始隐藏，refreshStatePanels() 时更新）
//   - 槽位按钮（槽0: 自动档位，槽1-9: 对应序号）
// isSave 为 true 时构建保存面板，false 时构建读取面板。
// thumbImages[10] 存储各槽位缩略图指针供后续刷新使用。
// ============================================================

void GameMenu::buildStatePanel(bool isSave, brls::Box* container, brls::Image* thumbImages[10])
{
    container->setPadding(5.f, 10.f, 5.f, 10.f);

    brls::Button* firstBtn = nullptr;
    brls::Button* lastBtn  = nullptr;

    for (int slot = 0; slot < 10; ++slot) {
        // 每个槽位一行：缩略图（左）+ 按钮（右）
        auto* row = new brls::Box(brls::Axis::ROW);
        row->setHeight(STATE_ROW_HEIGHT);
        row->setAlignItems(brls::AlignItems::CENTER);

        // 缩略图（初始隐藏，有状态文件时才显示）
        auto* thumb = new brls::Image();
        thumb->setWidth(STATE_THUMB_WIDTH);
        thumb->setMinWidth(STATE_THUMB_WIDTH);
        thumb->setHeight(STATE_THUMB_HEIGHT);
        thumb->setScalingType(brls::ImageScalingType::FIT);
        thumb->setVisibility(brls::Visibility::GONE);
        thumbImages[slot] = thumb;
        row->addView(thumb);

        // 槽位按钮
        auto* btn = new brls::Button();
        // 槽0 为自动档位，槽1-9 显示序号
        std::string slotLabel;
        if (slot == 0) {
            slotLabel = isSave
                ? "beiklive/gamemenu/save_slot_auto"_i18n
                : "beiklive/gamemenu/load_slot_auto"_i18n;
        } else {
            slotLabel = isSave
                ? ("beiklive/gamemenu/save_slot_prefix"_i18n + std::to_string(slot))
                : ("beiklive/gamemenu/load_slot_prefix"_i18n + std::to_string(slot));
        }
        btn->setText(slotLabel);
        btn->setGrow(1.f);

        // A 键：弹出确认对话框，确认后执行存/读档并关闭菜单
        int  captSlot   = slot;
        bool captIsSave = isSave;
        btn->registerAction("", brls::BUTTON_A, [this, captSlot, captIsSave](brls::View*) {
            std::string confirmMsg = captIsSave
                ? "beiklive/gamemenu/save_confirm"_i18n
                : "beiklive/gamemenu/load_confirm"_i18n;
            auto* dialog = new brls::Dialog(confirmMsg);
            dialog->addButton("beiklive/hints/cancel"_i18n, []() {});
            dialog->addButton("beiklive/hints/confirm"_i18n, [this, captSlot, captIsSave]() {
                // 先触发存/读档回调（GameView 将设置待处理的槽号）
                if (captIsSave) {
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

        row->addView(btn);
        container->addView(row);

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
// refreshStatePanels – 刷新保存/读取状态面板的缩略图显示
//
// 调用 m_stateInfoCallback 查询各槽位信息，
// 更新缩略图的显示/隐藏状态和图片内容。
// 须在 UI 线程调用。
// ============================================================

void GameMenu::refreshStatePanels()
{
    if (!m_stateInfoCallback) return;

    for (int slot = 0; slot < 10; ++slot) {
        auto info = m_stateInfoCallback(slot);

        // 刷新保存状态面板缩略图
        if (m_saveThumbImages[slot]) {
            if (!info.thumbPath.empty()) {
                m_saveThumbImages[slot]->setImageFromFile(info.thumbPath);
                m_saveThumbImages[slot]->setVisibility(brls::Visibility::VISIBLE);
            } else {
                m_saveThumbImages[slot]->setVisibility(brls::Visibility::GONE);
            }
        }

        // 刷新读取状态面板缩略图
        if (m_loadThumbImages[slot]) {
            if (!info.thumbPath.empty()) {
                m_loadThumbImages[slot]->setImageFromFile(info.thumbPath);
                m_loadThumbImages[slot]->setVisibility(brls::Visibility::VISIBLE);
            } else {
                m_loadThumbImages[slot]->setVisibility(brls::Visibility::GONE);
            }
        }
    }
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
        label->setText("无金手指");
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

