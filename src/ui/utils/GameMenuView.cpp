#include "GameMenuView.hpp"
#include "core/Tools.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include <filesystem>
#include "borealis/core/cache_helper.hpp"

namespace beiklive
{
    // ============================================================
    // 辅助函数：获取文件最后修改时间的字符串（委托给 Tools 公共函数）
    // ============================================================
    static std::string getFileModTimeStr(const std::string& path)
    {
        return beiklive::tools::getFileModTimeStr(path);
    }

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
            [this]() {
                if (m_onResume) m_onResume();
            },
            [this](){_refreshStatePanel(true);_refreshStatePanel(false);}

        );

        m_panel->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            brls::sync([this]() {
                _clearGridItemsFocus();
                if (m_onResume) m_onResume();
            });
            return true;
        });
        // 2. 保存状态（绑定保存状态面板）
        m_savePanel = _createSaveStatePanel();
        m_panel->addTab(
            "保存状态", 
            BK_RES("img/ui/menu/save.png"),
            nullptr, 
            [this](){_refreshStatePanel(true);}, 
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
            [this](){_refreshStatePanel(false);},  
            nullptr, 
            m_loadPanel,
            m_loadGrid->getItemView(0) // 默认聚焦第一个槽位
        );

        // 4. 金手指设置
        auto* cheatPanel = _createCheatPanel();
        m_panel->addTab(
            "金手指设置",
            BK_RES("img/ui/menu/cheat.png"),
            nullptr, nullptr, nullptr,
            cheatPanel
        );

        // 5. 画面设置
        auto* displayPanel = _createDisplayPanel();
        m_panel->addTab(
            "画面设置",
            BK_RES("img/ui/menu/display.png"),
            nullptr, nullptr, nullptr,
            displayPanel
        );


        //插入分割线
        m_panel->addDivider();


        // TODO 添加重置游戏（重启游戏）功能
        m_panel->addTab(
            "重置游戏", 
            BK_RES("img/ui/menu/reset.png"), 
            [this]() {
            if (m_onReset) m_onReset();
            }
        );



        // 6. 退出游戏（无面板）
        m_panel->addTab(
            "退出游戏", 
            BK_RES("img/ui/menu/exit.png"), 
            [this]() {
            if (m_onExit) m_onExit();
            }
        );


        m_panel->addFinish();
        this->getContentBox()->addView(m_panel);

    }


    // ============================================================
    // _slotName
    // ============================================================

    std::string GameMenuView::_slotName(int slot)
    {
        return (slot == 0) ? "自动存档" : "槽位 " + std::to_string(slot);
    }

    // ============================================================
    // _createSaveStatePanel
    // ============================================================

    brls::View* GameMenuView::_createSaveStatePanel()
    {
        auto* wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(wrapper);

        auto* titleLabel = new brls::Label();
        titleLabel->setText("保存状态");
        titleLabel->setFontSize(18.f);
        titleLabel->setMarginBottom(8.f);
        titleLabel->setMarginTop(8.f);
        titleLabel->setMarginLeft(18.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);
        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        auto* grid = new beiklive::GridBox(2);
        grid->setGrow(1.f);
        m_saveGrid = grid;

        m_saveItems.clear();
        for (int slot = 0; slot < 10; ++slot)
        {
            auto* item = new beiklive::GridItem(GridItemMode::SAVE_STATE, slot);
            item->setEmpty(_slotName(slot));
            m_saveItems.push_back(item);

            beiklive::GridItem* captItem = item;
            grid->addItem([captItem]() -> brls::View* { return captItem; });
        }
        // GridBox 的 onItemClicked 触发确认对话框
        grid->onItemClicked = [this](int slot) {
            // auto* dialog = new brls::Dialog("确认保存到" + _slotName(slot) + "？");
            // dialog->addButton("取消", []() {});
            // dialog->addButton("确认", [this, slot]() {
                if (m_saveStateCallback) m_saveStateCallback(slot);

                brls::sync([this]() {
                    if (m_onResume) m_onResume();
                });

            // dialog->open();
        };

        wrapper->addView(grid);
        return wrapper;
    }

    // ============================================================
    // _createLoadStatePanel
    // ============================================================

    brls::View* GameMenuView::_createLoadStatePanel()
    {
        auto* wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(wrapper);

        auto* titleLabel = new brls::Label();
        titleLabel->setText("读取状态");
        titleLabel->setFontSize(18.f);
        titleLabel->setMarginBottom(8.f);
        titleLabel->setMarginTop(8.f);
        titleLabel->setMarginLeft(18.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::LEFT);

        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        auto* grid = new beiklive::GridBox(2);
        grid->setGrow(1.f);
        m_loadGrid = grid;

        m_loadItems.clear();
        for (int slot = 0; slot < 10; ++slot)
        {
            auto* item = new beiklive::GridItem(GridItemMode::SAVE_STATE, slot);
            item->setEmpty(_slotName(slot));
            m_loadItems.push_back(item);

            beiklive::GridItem* captItem = item;
            grid->addItem([captItem]() -> brls::View* { return captItem; });
        }

        grid->onItemClicked = [this](int slot) {
            // auto* dialog = new brls::Dialog("确认从" + _slotName(slot) + "读取？");
            // dialog->addButton("取消", []() {});
            // dialog->addButton("确认", [this, slot]() {
                if (m_loadStateCallback) m_loadStateCallback(slot);
                brls::sync([this]() {
                    if (m_onResume) m_onResume();
                });
            // });
            // dialog->open();
        };

        wrapper->addView(grid);
        return wrapper;
    }

    void GameMenuView::_clearGridItemsFocus()
    {
        for (auto* item : m_saveItems)
        {
            if (item) item->setFocusable(false);
        }
        for (auto* item : m_loadItems)
        {
            if (item) item->setFocusable(false);
        }
    }



    // ============================================================
    // _refreshStatePanel – 异步扫描存档并更新 GridItem 显示
    // ============================================================

    void GameMenuView::_refreshStatePanel(bool isSave)
    {
        if (!m_stateInfoCallback) return;

        auto infoCallback = m_stateInfoCallback;

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, infoCallback, isSave]() {
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
                            // 这里需要先清除旧缓存（如果有的话），否则同一路径的缩略图更新后可能无法刷新显示
                            int oldTex = brls::TextureCache::instance().getCache(info.thumbPath);
                            if (oldTex > 0) {
                                brls::TextureCache::instance().removeCache(static_cast<size_t>(oldTex));
                                brls::TextureCache::instance().markDirty(static_cast<size_t>(oldTex));
                            }
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
            });
        });
    }

    void GameMenuView::draw(NVGcontext* vg, float x, float y, float w, float h,
                            brls::Style style, brls::FrameContext* ctx)
    {
        Box::draw(vg, x, y, w, h, style, ctx);
    }

    void GameMenuView::onShow()
    {
        m_panel->onShow();
    }

    // ============================================================
    // _createCheatPanel
    // ============================================================
    brls::View* GameMenuView::_createCheatPanel()
    {
        // 子界面容器
        auto* wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        wrapper->setWidthPercentage(100.f);
        HIDE_BRLS_HIGHLIGHT(wrapper);
        // wrapper->setWireframeEnabled(true);

        // 顶栏
        auto* topRow = new brls::Box(brls::Axis::ROW);
        topRow->setFocusable(false);
        topRow->setAlignItems(brls::AlignItems::CENTER);
        // topRow->setPadding(12.f, 16.f, 8.f, 16.f);
        topRow->setHeight(30.f);
        topRow->setWidthPercentage(100.f);
        topRow->setWireframeEnabled(true);

        auto* titleLabel = new brls::Label();
        titleLabel->setText("当前金手指文件：");
        titleLabel->setFontSize(20.f);
        titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        titleLabel->setMarginRight(10.f);
        titleLabel->setFocusable(false);
        topRow->addView(titleLabel);

        m_cheatCountLabel = new brls::Label();
        m_cheatCountLabel->setText("共 0 项 | 已启用 0 项");
        m_cheatCountLabel->setFontSize(14.f);
        m_cheatCountLabel->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        m_cheatCountLabel->setGrow(1.f);
        m_cheatCountLabel->setFocusable(false);
        topRow->addView(m_cheatCountLabel);

        topRow->addView(new brls::Padding());

        auto* selectChtBtn = new brls::Button();
        selectChtBtn->setText("选择金手指文件(cht)");
        // selectChtBtn->setIcon(BK_RES("img/ui/menu/cheat.png"));
        selectChtBtn->setMarginRight(4.f);
        // selectChtBtn->setWidth(80.f);
        selectChtBtn->registerClickAction([this](brls::View*) -> bool {
            beiklive::openFilePicker({"cht"},
                [this](const std::string& path) {
                    _loadCheatsFromPath(path);
                    if (m_cheatPathCallback) m_cheatPathCallback(path);
                });
            return true;
        });
        topRow->addView(selectChtBtn);

        auto* enableAllBtn = new brls::Button();
        enableAllBtn->setText("全部开启");
        // enableAllBtn->setIcon(BK_RES("img/ui/setting/display.png"));
        enableAllBtn->setMarginRight(4.f);
        enableAllBtn->registerClickAction([this](brls::View*) -> bool {
            _setAllCheatsEnabled(true);
            return true;
        });
        topRow->addView(enableAllBtn);

        auto* disableAllBtn = new brls::Button();
        disableAllBtn->setText("全部关闭");
        // disableAllBtn->setIcon(BK_RES("img/ui/setting/display.png"));
        disableAllBtn->registerClickAction([this](brls::View*) -> bool {
            _setAllCheatsEnabled(false);
            return true;
        });
        topRow->addView(disableAllBtn);

        wrapper->addView(topRow);

        // 金手指网格列表
        m_cheatItemBox = new beiklive::GridBox(2);
        m_cheatItemBox->setWireframeEnabled(true);
        wrapper->addView(m_cheatItemBox);
        // 读取金手指文件
        if (!m_gameEntry.cheatPath.empty())
            _loadCheatsFromPath(m_gameEntry.cheatPath);

        return wrapper;
    }

    void GameMenuView::_loadCheatsFromPath(const std::string& path)
    {
        m_cheats = beiklive::parseChtFile(path);
        m_gameEntry.cheatPath = path;
        // _rebuildCheatItems();
    }

    void GameMenuView::_rebuildCheatItems()
    {
        if (!m_cheatItemBox) return;
        m_cheatItemBox->clearViews(true);
        m_cheatSwitches.clear();

        if (m_cheats.empty()) {
            auto* label = new brls::Label();
            label->setText("该金手指文件无有效条目");
            label->setFontSize(14.f);
            label->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
            label->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            label->setFocusable(false);
            m_cheatItemBox->addView(label);
        } else {
            for (int i = 0; i < (int)m_cheats.size(); ++i) {
                auto* sw = new SwitchButton();
                sw->setText(m_cheats[i].desc);
                sw->setState(m_cheats[i].enabled);
                int idx = i;
                sw->setOnToggle([this, idx](bool on) {
                    if (idx < (int)m_cheats.size()) {
                        m_cheats[idx].enabled = on;
                        if (m_cheatToggleCallback) m_cheatToggleCallback(idx, on);
                        _updateCheatCount();
                    }
                });
                m_cheatSwitches.push_back(sw);
                m_cheatItemBox->addView(sw);
            }
        }
        _updateCheatCount();
    }

    void GameMenuView::_updateCheatCount()
    {
        if (!m_cheatCountLabel) return;
        int total = (int)m_cheats.size();
        int enabled = 0;
        for (auto& c : m_cheats) if (c.enabled) ++enabled;
        m_cheatCountLabel->setText("共 " + std::to_string(total) + " 项 | 已启用 " + std::to_string(enabled) + " 项");
    }

    void GameMenuView::_setAllCheatsEnabled(bool enabled)
    {
        for (int i = 0; i < (int)m_cheats.size(); ++i) {
            m_cheats[i].enabled = enabled;
            if (i < (int)m_cheatSwitches.size()) {
                m_cheatSwitches[i]->setState(enabled);
                if (m_cheatToggleCallback) m_cheatToggleCallback(i, enabled);
            }
        }
        _updateCheatCount();
    }

    // ============================================================
    // _createDisplayPanel
    // ============================================================
    brls::View* GameMenuView::_createDisplayPanel()
    {
        auto* wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);

        auto* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.f);
        scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
        scroll->setFocusable(false);

        auto* box = new brls::Box(brls::Axis::COLUMN);
        box->setPadding(10.f, 20.f, 20.f, 20.f);

        auto* hdr1 = new brls::Header();
        hdr1->setTitle("画面设置");
        box->addView(hdr1);

        auto* modeCell = new brls::SelectorCell();
        std::vector<std::string> modes = {"Fit", "Fill", "Original", "Integer", "Custom"};
        std::string curMode = GET_SETTING_KEY_STR("display.mode", "original");
        int idx = 2;
        std::vector<std::string> ids = {"fit", "fill", "original", "integer", "custom"};
        for (int i = 0; i < 5; ++i) if (ids[i] == curMode) { idx = i; break; }
        modeCell->init("画面模式", modes, idx,
            [ids](int i) { if (i >= 0 && i < 5) SET_SETTING_KEY_STR("display.mode", ids[i]); });
        box->addView(modeCell);

        std::vector<std::string> filters = {"Nearest", "Linear"};
        std::string curFilter = GET_SETTING_KEY_STR("display.filter", "nearest");
        int fi = (curFilter == "linear") ? 1 : 0;
        auto* filterCell = new brls::SelectorCell();
        filterCell->init("纹理过滤", filters, fi,
            [](int i) { SET_SETTING_KEY_STR("display.filter", i == 1 ? "linear" : "nearest"); });
        box->addView(filterCell);

        auto* overlayCell = new brls::BooleanCell();
        overlayCell->init("启用遮罩",
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_ENABLED, 0) != 0,
            [](bool v) { SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_ENABLED, v ? 1 : 0); });
        box->addView(overlayCell);

        auto* shaderCell = new brls::BooleanCell();
        shaderCell->init("启用着色器",
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, 0) != 0,
            [](bool v) { SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, v ? 1 : 0); });
        box->addView(shaderCell);

        scroll->setContentView(box);
        wrapper->addView(scroll);
        return wrapper;
    }

} // namespace beiklive
