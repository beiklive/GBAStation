#include "GameMenuView.hpp"
#include "core/Tools.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "ui/utils/UiHelper.hpp"
#include <filesystem>
#include "borealis/core/cache_helper.hpp"
#include <borealis/views/dialog.hpp>

namespace beiklive
{
    // ============================================================
    // 辅助函数：获取文件最后修改时间的字符串（委托给 Tools 公共函数）
    // ============================================================
    static std::string getFileModTimeStr(const std::string &path)
    {
        return beiklive::tools::getFileModTimeStr(path);
    }
    static bool isValidCheatCode(const std::string &code)
    {
        if (code.empty())
            return false;

        size_t b = code.find_first_not_of(" \t");
        if (b == std::string::npos)
            return false;
        size_t e = code.find_last_not_of(" \t");
        std::string line = code.substr(b, e - b + 1);
        if (line.empty())
            return false;

        if (line.find(':') != std::string::npos ||
            line.find('+') != std::string::npos)
        {
            return true;
        }

        auto sp = line.find(' ');
        if (sp != std::string::npos)
        {
            std::string addr = line.substr(0, sp);
            std::string val = line.substr(sp + 1);
            size_t vb = val.find_first_not_of(" \t");
            if (vb != std::string::npos)
                val = val.substr(vb);
            if (addr.size() == 8 &&
                std::all_of(addr.begin(), addr.end(),
                            [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); }) &&
                (val.size() == 4 || val.size() == 8) &&
                std::all_of(val.begin(), val.end(),
                            [](char c) { return std::isxdigit(static_cast<unsigned char>(c)); }))
            {
                return true;
            }
        }

        return false;
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

            // 将 ASYNC_RELEASE 移入 brls::sync 回调内部，确保在 UI 线程执行时
            // 检查视图是否已销毁，避免 View 析构与 brls::sync 投递之间的竞态条件。
            brls::sync([ASYNC_TOKEN, infos = std::move(infos), isSave]() {
                ASYNC_RELEASE
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
            beiklive::openFilePicker({"cht"},
                [this](const std::string& path) {
                    _loadCheatsFromPath(path);
                    if (m_cheatPathCallback) m_cheatPathCallback(path);
                });
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
                                if (!isValidCheatCode(code))
                                {
                                    brls::Application::notify("金手指代码格式不正确，请重新输入");
                                    promptCode();
                                    return;
                                }
                                CheatEntry entry;
                                entry.desc = name;
                                entry.code = code;
                                entry.enabled = true;
                                m_cheats.push_back(entry);
                                if (!m_gameEntry.cheatPath.empty())
                                    beiklive::saveChtFile(m_gameEntry.cheatPath, m_cheats);
                                if (m_cheatToggleCallback)
                                {
                                    int newIdx = static_cast<int>(m_cheats.size()) - 1;
                                    m_cheatToggleCallback(newIdx, true);
                                }
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
        m_cheats = beiklive::parseChtFile(path);
        m_gameEntry.cheatPath = path;
        brls::Logger::info("Loaded {} cheats from {}", m_cheats.size(), path);
        _rebuildCheatItems();
    }

    void GameMenuView::_rebuildCheatItems()
    {
        if (!m_cheatItemBox)
            return;
        m_cheatItemBox->clearViews(true);
        m_cheatSwitches.clear();

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
            for (int i = 0; i < (int)m_cheats.size(); ++i)
            {
                auto *sw = new SwitchButton();

                DISABLE_LR_NAVIGATION(sw);
                
                sw->registerAction("返回", brls::BUTTON_B, [this](brls::View *) -> bool{
                    brls::Application::giveFocus(selectChtBtn);
                    return true;
                });

                sw->setText(m_cheats[i].desc);
                sw->setState(m_cheats[i].enabled);
                int idx = i;
                sw->setOnToggle([this, idx](bool on)
                                {
                    if (idx < (int)m_cheats.size()) {
                        m_cheats[idx].enabled = on;
                        if (m_cheatToggleCallback) m_cheatToggleCallback(idx, on);
                        _updateCheatCount();
                        if (!m_gameEntry.cheatPath.empty())
                            beiklive::saveChtFile(m_gameEntry.cheatPath, m_cheats);
                    } });

                // BUTTON_X: 修改金手指代码
                sw->registerAction("修改代码", brls::BUTTON_X, [this, idx](brls::View *) -> bool {
                    if (idx >= (int)m_cheats.size()) return true;
                    std::function<void()> promptCode;
                    promptCode = [this, idx, &promptCode]() {
                        auto* ime = brls::Application::getImeManager();
                        if (!ime) return;
                        if (idx >= (int)m_cheats.size()) return;
                        ime->openForText(
                            [this, idx, &promptCode](std::string code) {
                                if (code.empty()) return;
                                if (!isValidCheatCode(code))
                                {
                                    brls::Application::notify("金手指代码格式不正确，请重新输入");
                                    promptCode();
                                    return;
                                }
                                if (idx >= (int)m_cheats.size()) return;
                                m_cheats[idx].code = code;
                                if (!m_gameEntry.cheatPath.empty())
                                    beiklive::saveChtFile(m_gameEntry.cheatPath, m_cheats);
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
                    if (idx >= (int)m_cheats.size()) return true;
                    auto* ime = brls::Application::getImeManager();
                    if (!ime) return true;
                    ime->openForText(
                        [this, idx](std::string name) {
                            if (name.empty()) return;
                            if (idx >= (int)m_cheats.size()) return;
                            m_cheats[idx].desc = name;
                            if (!m_gameEntry.cheatPath.empty())
                                beiklive::saveChtFile(m_gameEntry.cheatPath, m_cheats);
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
                    if (idx >= (int)m_cheats.size()) return true;
                    auto* dlg = new brls::Dialog("是否删除 \"" + m_cheats[idx].desc + "\" ?");
                    dlg->addButton("确认删除", [this, idx]() {
                        if (idx >= (int)m_cheats.size()) return;
                        if (m_cheatToggleCallback)
                            m_cheatToggleCallback(idx, false);
                        m_cheats.erase(m_cheats.begin() + idx);
                        if (!m_gameEntry.cheatPath.empty())
                            beiklive::saveChtFile(m_gameEntry.cheatPath, m_cheats);
                        _rebuildCheatItems();
                    });
                    dlg->addButton("取消", [](){});
                    dlg->open();
                    return true;
                });

                m_cheatSwitches.push_back(sw);
                m_cheatItemBox->addView(sw);
            }
        }
        _updateCheatCount();
    }

    void GameMenuView::_updateCheatCount()
    {
        if (!m_cheatCountLabel)
            return;
        int total = (int)m_cheats.size();
        int enabled = 0;
        for (auto &c : m_cheats)
            if (c.enabled)
                ++enabled;
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

            std::vector<std::string> ffLabels = {"0.1倍", "0.5倍", "1倍", "1.25倍", "1.5倍", "1.75倍", "2倍", "3倍", "4倍", "5倍", "6倍", "7倍", "8倍"};
            static const float ffVals[] = {0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
            float curFF = GET_SETTING_KEY_FLOAT("fastforward.multiplier", 4.0f);
            int ffIdx = 8;
            for (int i = 0; i < 13; ++i)
                if (ffVals[i] == curFF) { ffIdx = i; break; }
            auto *ffCell = new beiklive::SelectorButton();
            ffCell->setText("快进倍率");
            ffCell->setOptions(ffLabels, ffIdx);
            ffCell->setOnSelect(
                [](int i) {
                    static const float vals[] = {0.1f, 0.5f, 1.0f, 1.25f, 1.5f, 1.75f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f};
                    if (i >= 0 && i < 13) SET_SETTING_KEY_FLOAT("fastforward.multiplier", vals[i]);
                });
            box->addView(ffCell);
            box->addView(makeHint("小于1倍时可在快进触发时实现慢动作效果"));

            auto *hdr1 = new brls::Header();
            hdr1->setTitle("画面设置");
            box->addView(hdr1);

            // ── 画面模式 ──
            // ScreenMode 枚举值到 UI 索引映射: 0(Fit)→0, 1(Fill)→1, 2(IntegerScale)→3, 3(FreeScale)→4
            static const int kScreenModeToUi[] = {0, 1, 3, 4};
            int idx = (m_gameEntry.displayMode >= 0 && m_gameEntry.displayMode < 4)
                          ? kScreenModeToUi[m_gameEntry.displayMode] : 2;

            auto *modeCell = new beiklive::SelectorButton();
            auto *IntegerCell = new beiklive::SelectorButton();
            auto *customCell = new brls::DetailCell();
            std::vector<std::string> modes = {"(保持比例)Fit", "(填充)Fill", "(原始)Original", "(整数倍)Integer", "(自定义)Custom"};
            std::vector<std::string> modeIds = {"fit", "fill", "original", "integer", "custom"};

            IntegerCell->setFocusable(idx == 3);
            IntegerCell->setAlpha(idx == 3? 1.0f: 0.3f);
            customCell->setFocusable(idx == 4);
            customCell->setAlpha(idx == 4? 1.0f: 0.3f);
            
            modeCell->setText("画面模式");
            modeCell->setOptions(modes, idx);
            modeCell->setOnSelect(
                [this, modeIds, IntegerCell, customCell](int idx)
                {
                    if (idx >= 0 && idx < (int)modeIds.size())
                    {
                        IntegerCell->setFocusable(idx == 3);
                        IntegerCell->setAlpha(idx == 3? 1.0f: 0.3f);
                        customCell->setFocusable(idx == 4);
                        customCell->setAlpha(idx == 4? 1.0f: 0.3f);
                        static const int kUiToScreenMode[] = {0, 1, 0, 2, 3};
                        m_gameEntry.displayMode = kUiToScreenMode[idx];
                        if (m_displayModeCallback)
                            m_displayModeCallback(modeIds[idx]);
                    }
                });
            box->addView(modeCell);

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

        // ── 纹理过滤 ──
        std::vector<std::string> filters = {"(像素)Nearest", "(平滑)Linear"};
        std::string curFilter = GET_SETTING_KEY_STR("display.filter", "nearest");
        int fi = (curFilter == "linear") ? 1 : 0;
        auto *filterCell = new beiklive::SelectorButton();
        filterCell->setText("纹理过滤");
        filterCell->setOptions(filters, fi);
        filterCell->setOnSelect(
            [this](int idx)
            {
                std::string val = (idx == 1) ? "linear" : "nearest";
                SET_SETTING_KEY_STR("display.filter", val);
                if (m_filterCallback)
                    m_filterCallback(val);
            });
        box->addView(filterCell);
        box->addView(makeHint("不影响着色器渲染效果"));

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
            m_ShaderSidePanel->setBackgroundColor(nvgRGBA(0, 0, 0, 60));
            m_ShaderSidePanel->setFocusable(false);
            m_ShaderSidePanel->setVisibility(brls::Visibility::GONE);

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
            {
                beiklive::GameDB->set(m_gameEntry.path, "shaderEnabled", nlohmann::json(m_gameEntry.shaderEnabled));
                beiklive::GameDB->set(m_gameEntry.path, "shaderPath", nlohmann::json(m_gameEntry.shaderPath));
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaNames", nlohmann::json(m_gameEntry.shaderParaNames));
                beiklive::GameDB->set(m_gameEntry.path, "shaderParaValues", nlohmann::json(m_gameEntry.shaderParaValues));
                beiklive::GameDB->flush();
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
                                         std::string dir = m_gameEntry.shaderPath;
                                         auto pos = dir.rfind(beiklive::path::SPLIT_CHAR);
                                         if (pos != std::string::npos)
                                             dir = dir.substr(0, pos);
                                         else
                                             dir = "";
                                         beiklive::openFilePicker({"glslp", "glsl"}, [this](const std::string &path)
                                                                  {
                        m_gameEntry.shaderPath = path;
                        shaderPathcell->setDetailText(beiklive::tools::getFileName(path));
                        if (m_shaderPathCallback) m_shaderPathCallback(path);
                        _rebuildShaderParamUI(); }, dir);
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
                                         std::string dir = m_gameEntry.overlayPath;
                                         auto pos = dir.rfind(beiklive::path::SPLIT_CHAR);
                                         if (pos != std::string::npos)
                                             dir = dir.substr(0, pos);
                                         else
                                             dir = "";
                                         beiklive::openFilePicker({"png"}, [pathCell, this](const std::string &path)
                                                                  {
                        m_gameEntry.overlayPath = path;
                        pathCell->setDetailText(beiklive::tools::getFileName(path));
                        if (m_overlayPathCallback) m_overlayPathCallback(path); }, dir);
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
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 10));

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
        if (params.size() != m_gameEntry.shaderParaNames.size())
        {
            m_gameEntry.shaderParaNames.clear();
            m_gameEntry.shaderParaValues.clear();
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
                    isNewParams = true;
                    break;
                }
            }
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
