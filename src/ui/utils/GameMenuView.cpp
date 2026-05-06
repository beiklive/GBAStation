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
    static brls::Label *makeHint(const std::string &text)
    {
        auto *lbl = new brls::Label();
        lbl->setText(text);
        lbl->setFontSize(16.f);
        lbl->setTextColor(nvgRGB(154, 154, 154));
        lbl->setMarginBottom(10.f);
        lbl->setMarginTop(10.f);
        lbl->setMarginLeft(20.f);
        lbl->setFocusable(false);
        return lbl;
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
            }
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
        _refreshStatePanel(true);
        _refreshStatePanel(false);
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
        // topRow->setWireframeEnabled(true);

        auto* titleLabel = new brls::Label();
        titleLabel->setText("当前金手指文件：");
        titleLabel->setFontSize(20.f);
        titleLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        titleLabel->setMarginRight(10.f);
        titleLabel->setFocusable(false);
        topRow->addView(titleLabel);

        cheatPathLabel = new brls::Label();
        cheatPathLabel->setText(beiklive::tools::getFileName(m_gameEntry.cheatPath));
        cheatPathLabel->setFontSize(14.f);
        cheatPathLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        cheatPathLabel->setMarginRight(10.f);
        cheatPathLabel->setFocusable(false);
        topRow->addView(cheatPathLabel);

        topRow->addView(new brls::Padding());
        m_cheatCountLabel = new brls::Label();
        m_cheatCountLabel->setText("共 0 项 | 已启用 0 项");
        m_cheatCountLabel->setFontSize(14.f);
        m_cheatCountLabel->setTextColor(GET_THEME_COLOR("brls/text"));
        m_cheatCountLabel->setGrow(1.f);
        m_cheatCountLabel->setFocusable(false);
        topRow->addView(m_cheatCountLabel);


        auto* selectChtBtn = new brls::Button();
        selectChtBtn->setText("选择金手指文件(cht)");
        selectChtBtn->setMarginRight(4.f);
        selectChtBtn->registerClickAction([this](brls::View*) -> bool {
            beiklive::openFilePicker({"cht"},
                [this](const std::string& path) {
                    _loadCheatsFromPath(path);
                    if (m_cheatPathCallback) m_cheatPathCallback(path);
                });
            return true;
        });
        topRow->addView(selectChtBtn);



        wrapper->addView(topRow);
        auto* itemContainer = new brls::ScrollingFrame();
        itemContainer->setGrow(1.f);
        // 金手指网格列表
        m_cheatItemBox = new brls::Box(brls::Axis::COLUMN);
        m_cheatItemBox->setGrow(1.f);
        m_cheatItemBox->setPadding(0.f, 20.f, 0.f, 20.f);

        itemContainer->addView(m_cheatItemBox);
        wrapper->addView(itemContainer);
        // 读取金手指文件
        if (!m_gameEntry.cheatPath.empty())
            _loadCheatsFromPath(m_gameEntry.cheatPath);

        return wrapper;
    }

    void GameMenuView::_loadCheatsFromPath(const std::string& path)
    {
        m_cheats = beiklive::parseChtFile(path);
        m_gameEntry.cheatPath = path;
        brls::Logger::info("Loaded {} cheats from {}", m_cheats.size(), path);
        _rebuildCheatItems();
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


    // ============================================================
    // _createDisplayPanel
    // ============================================================
    brls::View* GameMenuView::_createDisplayPanel()
    {
        auto* wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setVisibility(brls::Visibility::GONE);
        wrapper->setGrow(1.f);
        wrapper->setWidthPercentage(100.f);
        wrapper->setFocusable(false);

        auto* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.f);
        scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
        scroll->setFocusable(false);

        auto* box = new brls::Box(brls::Axis::COLUMN);
        box->setPadding(10.f, 20.f, 20.f, 20.f);

        {
            auto* hdr1 = new brls::Header();
            hdr1->setTitle("画面设置");
            box->addView(hdr1);

            // ── 画面模式 ──
            auto* modeCell = new beiklive::SelectorButton();
            auto* IntegerCell = new beiklive::SelectorButton();
            auto* customCell = new brls::DetailCell();
            std::vector<std::string> modes = {"(保持比例)Fit", "(填充)Fill", "(原始)Original", "(整数倍)Integer", "(自定义)Custom"};
            std::vector<std::string> modeIds = {"fit", "fill", "original", "integer", "custom"};
            std::string curMode = GET_SETTING_KEY_STR("display.mode", "original");
            int idx = 2;
            for (int i = 0; i < 5; ++i) if (modeIds[i] == curMode) { idx = i; break; }
            IntegerCell->setFocusable(idx == 3);
            customCell->setFocusable(idx == 4);

            modeCell->setText("画面模式");
            modeCell->setOptions(modes, idx);
            modeCell->setOnSelect(
                [this, modeIds, IntegerCell, customCell](int idx) {
                    if (idx >= 0 && idx < (int)modeIds.size()) {
                        SET_SETTING_KEY_STR("display.mode", modeIds[idx]);
                        IntegerCell->setFocusable(idx == 3);
                        customCell->setFocusable(idx == 4);
                        if (m_displayModeCallback) m_displayModeCallback(modeIds[idx]);
                    }
                }
            );
            box->addView(modeCell);

            // ── 整数倍缩放 ──
            std::vector<std::string> intScaleLabels = {"自动(auto)", "x1", "x2", "x3", "x4", "x5"};
            static const int intScaleVals[] = {0, 1, 2, 3, 4, 5};
            int curIntScale = GET_SETTING_KEY_INT("display.integer_scale_mult", 0);
            int intScaleIdx = 0;
            for (int i = 0; i < 6; ++i) if (intScaleVals[i] == curIntScale) { intScaleIdx = i; break; }
            IntegerCell->setText("整数倍缩放倍率");
            IntegerCell->setOptions(intScaleLabels, intScaleIdx);
            IntegerCell->setOnSelect(
                [this](int idx) {
                    if (idx >= 0 && idx < 6) {
                        static const int vals[] = {0, 1, 2, 3, 4, 5};
                        SET_SETTING_KEY_INT("display.integer_scale_mult", vals[idx]);
                        if (m_displayModeCallback) {
                            std::string cur = GET_SETTING_KEY_STR("display.mode", "original");
                            m_displayModeCallback(cur);
                        }
                    }
                }
            );
            box->addView(IntegerCell);
            box->addView(makeHint("仅在画面模式为整数倍时可用，选择auto则自动匹配最大整数倍"));

            // ── 自定义设置入口 ──
            customCell->setText("自定义设置");
            customCell->setDetailText(">>");
            customCell->registerClickAction([this](brls::View*) -> bool {
                _openCustomScaleSettings();
                return true;
            });
            box->addView(customCell);
            box->addView(makeHint("仅在画面模式为自定义时可用，调整位置偏移和缩放比例"));
        }

        // ── 纹理过滤 ──
        std::vector<std::string> filters = {"(像素)Nearest", "(平滑)Linear"};
        std::string curFilter = GET_SETTING_KEY_STR("display.filter", "nearest");
        int fi = (curFilter == "linear") ? 1 : 0;
        auto* filterCell = new beiklive::SelectorButton();
        filterCell->setText("纹理过滤");
        filterCell->setOptions(filters, fi);
        filterCell->setOnSelect(
            [this](int idx) {
                std::string val = (idx == 1) ? "linear" : "nearest";
                SET_SETTING_KEY_STR("display.filter", val);
                if (m_filterCallback) m_filterCallback(val);
            }
        );
        box->addView(filterCell);
        box->addView(makeHint("不影响着色器渲染效果"));

        {
            auto* hdr1 = new brls::Header();
            hdr1->setTitle("个性化设置");
            box->addView(hdr1);
        }

        auto* overlayCell = new brls::DetailCell();
        overlayCell->setText("遮罩设置");
        overlayCell->setDetailText(">>");
        overlayCell->registerClickAction([this](brls::View*) -> bool {
            _openOverlaySettings();
            return true;
        });
        box->addView(overlayCell);

        auto* shaderCell = new brls::DetailCell();
        shaderCell->setText("着色器设置");
        shaderCell->setDetailText(">>");
        shaderCell->registerClickAction([this](brls::View*) -> bool {
            _openShaderSettings();
            return true;
        });
        box->addView(shaderCell);

        scroll->setContentView(box);
        wrapper->addView(scroll);
        return wrapper;
    }

    // ============================================================
    // _openCustomScaleSettings - 自定义缩放子界面
    // ============================================================
    void GameMenuView::_openCustomScaleSettings()
    {
        auto* content = new beiklive::Box();
        content->setGrow(1.f);
        auto* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.f);
        auto* box = new brls::Box(brls::Axis::COLUMN);
        box->setPadding(10.f, 20.f, 20.f, 20.f);

        auto* hdr = new brls::Header();
        hdr->setTitle("自定义画面设置");
        box->addView(hdr);

        // X 偏移
        auto* xCell = new brls::SliderCell();
        xCell->init("X轴偏移", GET_SETTING_KEY_FLOAT("display.x_offset", 0.f),
            [](float v) { SET_SETTING_KEY_FLOAT("display.x_offset", v); });
        box->addView(xCell);

        // Y 偏移
        auto* yCell = new brls::SliderCell();
        yCell->init("Y轴偏移", GET_SETTING_KEY_FLOAT("display.y_offset", 0.f),
            [](float v) { SET_SETTING_KEY_FLOAT("display.y_offset", v); });
        box->addView(yCell);

        // 自定义缩放
        auto* sCell = new brls::SliderCell();
        sCell->init("缩放比例", GET_SETTING_KEY_FLOAT("display.custom_scale", 1.f),
            [](float v) { SET_SETTING_KEY_FLOAT("display.custom_scale", v); });
        box->addView(sCell);

        auto* hint = new brls::Label();
        hint->setText("偏移范围: -200 ~ 200 px | 缩放: 0.5 ~ 3.0 x");
        hint->setFontSize(13.f);
        hint->setTextColor(GET_THEME_COLOR("brls/text_disabled"));
        hint->setFocusable(false);
        hint->setMarginTop(10.f);
        box->addView(hint);

        scroll->setContentView(box);
        content->getContentBox()->addView(scroll);

        auto* frame = new brls::AppletFrame(content);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
    }

    // ============================================================
    // _openShaderSettings – 着色器设置子界面
    // ============================================================
    void GameMenuView::_openShaderSettings()
    {
        auto* content = new beiklive::Box();
        content->setGrow(1.f);
        content->setFocusable(false);

        auto* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.f);
        scroll->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);

        auto* box = new brls::Box(brls::Axis::COLUMN);
        box->setPadding(10.f, 20.f, 20.f, 20.f);

        auto* hdr = new brls::Header();
        hdr->setTitle("着色器设置");
        box->addView(hdr);

        // ── 启停着色器 ──
        bool shaderOn = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, 0) != 0;
        auto* toggleCell = new brls::BooleanCell();
        toggleCell->init("启用着色器", shaderOn,
            [this](bool v) {
                SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_SHADER_ENABLED, v ? 1 : 0);
                if (m_shaderToggleCallback) m_shaderToggleCallback(v);
            });
        box->addView(toggleCell);

        // ── 选择着色器文件 ──
        auto* pathCell = new brls::DetailCell();
        pathCell->setText("着色器文件");
        std::string curShader = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_DISPLAY_SHADER_PATH, "");
        pathCell->setDetailText(curShader.empty() ? "未设置" : beiklive::tools::getFileName(curShader));
        pathCell->registerAction("选择", brls::BUTTON_A,
            [this, pathCell](brls::View*) -> bool {
                std::string dir = GET_SETTING_KEY_STR(beiklive::SettingKey::KEY_DISPLAY_SHADER_PATH, "");
                auto pos = dir.rfind('/');
#ifdef _WIN32
                auto posW = dir.rfind('\\');
                if (posW != std::string::npos && (pos == std::string::npos || posW > pos)) pos = posW;
#endif
                if (pos != std::string::npos) dir = dir.substr(0, pos); else dir = "";
                beiklive::openFilePicker({"glslp", "glsl"},
                    [this, pathCell](const std::string& path) {
                        SET_SETTING_KEY_STR(beiklive::SettingKey::KEY_DISPLAY_SHADER_PATH, path);
                        pathCell->setDetailText(beiklive::tools::getFileName(path));
                        if (m_shaderPathCallback) m_shaderPathCallback(path);
                    }, dir);
                return true;
            });
        box->addView(pathCell);

        // ── 着色器参数（外部注入时填充）──
        // TODO: 若 GameView 有活跃着色器参数，通过回调获取并动态创建 SliderCell

        scroll->setContentView(box);
        content->getContentBox()->addView(scroll);

        auto* frame = new brls::AppletFrame(content);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
    }

    // ============================================================
    // _openOverlaySettings – 遮罩设置子界面
    // ============================================================
    void GameMenuView::_openOverlaySettings()
    {
        auto* content = new beiklive::Box();
        content->setGrow(1.f);
        content->setFocusable(false);

        auto* scroll = new brls::ScrollingFrame();
        scroll->setGrow(1.f);

        auto* box = new brls::Box(brls::Axis::COLUMN);
        box->setPadding(10.f, 20.f, 20.f, 20.f);

        auto* hdr = new brls::Header();
        hdr->setTitle("遮罩设置");
        box->addView(hdr);

        bool overlayOn = GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_ENABLED, 0) != 0;
        auto* toggleCell = new brls::BooleanCell();
        toggleCell->init("启用遮罩", overlayOn,
            [](bool v) {
                SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_ENABLED, v ? 1 : 0);
            });
        box->addView(toggleCell);

        auto makeOverlayCell = [&](const std::string& cfgKey, const std::string& label) {
            auto* cell = new brls::DetailCell();
            cell->setText(label);
            std::string cur = GET_SETTING_KEY_STR(cfgKey, "");
            cell->setDetailText(cur.empty() ? "未设置" : beiklive::tools::getFileName(cur));
            cell->registerAction("选择", brls::BUTTON_A,
                [cell, cfgKey](brls::View*) -> bool {
                    std::string dir = GET_SETTING_KEY_STR(cfgKey, "");
                    auto pos = dir.rfind('/');
#ifdef _WIN32
                    auto posW = dir.rfind('\\');
                    if (posW != std::string::npos && (pos == std::string::npos || posW > pos)) pos = posW;
#endif
                    if (pos != std::string::npos) dir = dir.substr(0, pos); else dir = "";
                    beiklive::openFilePicker({"png"},
                        [cell, cfgKey](const std::string& path) {
                            SET_SETTING_KEY_STR(cfgKey, path);
                            cell->setDetailText(beiklive::tools::getFileName(path));
                        }, dir);
                    return true;
                });
            return cell;
        };

        box->addView(makeOverlayCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBA_PATH, "GBA 遮罩"));
        box->addView(makeOverlayCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GBC_PATH, "GBC 遮罩"));
        box->addView(makeOverlayCell(beiklive::SettingKey::KEY_DISPLAY_OVERLAY_GB_PATH,  "GB 遮罩"));

        scroll->setContentView(box);
        content->getContentBox()->addView(scroll);

        auto* frame = new brls::AppletFrame(content);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));
    }
    }
