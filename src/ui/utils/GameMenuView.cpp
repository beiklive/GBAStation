#include "GameMenuView.hpp"
#include <filesystem>
#include <chrono>
#include <ctime>

namespace beiklive
{
    // ============================================================
    // 辅助函数：获取文件最后修改时间的字符串
    // ============================================================
    static std::string getFileModTimeStr(const std::string& path)
    {
        std::error_code ec;
        auto ftime = std::filesystem::last_write_time(path, ec);
        if (ec) return "";
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() +
            std::chrono::system_clock::now());
        std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
        char buf[64];
        std::tm* tm = std::localtime(&tt);
        if (!tm) return "";
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm);
        return std::string(buf);
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
        this->setBackgroundColor(nvgRGBA(0, 0, 0, 160));
        this->setWidthPercentage(100.f);
        this->setHeightPercentage(100.f);
        this->setJustifyContent(brls::JustifyContent::CENTER);
        this->setAlignItems(brls::AlignItems::CENTER);

        this->registerAction("返回游戏", brls::BUTTON_B, [this](brls::View*) -> bool {
            if (m_onResume) m_onResume();
            return true;
        });

        // 居中面板
        m_panel = new brls::Box();
        m_panel->setAxis(brls::Axis::ROW);
        m_panel->setJustifyContent(brls::JustifyContent::CENTER);
        m_panel->setAlignItems(brls::AlignItems::CENTER);
        m_panel->setWidthPercentage(100.f);
        m_panel->setGrow(1.f);
        m_panel->setBackground(brls::ViewBackground::NONE);
        m_panel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_panel);

        // 左侧按钮栏
        m_contrlPanel = new brls::Box();
        m_contrlPanel->setAxis(brls::Axis::COLUMN);
        m_contrlPanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_contrlPanel->setAlignItems(brls::AlignItems::FLEX_END);
        m_contrlPanel->setWidthPercentage(30.f);
        m_contrlPanel->setHeightPercentage(100.f);
        m_contrlPanel->setBackgroundColor(nvgRGBA(0, 0, 0, 10));
        m_contrlPanel->setPadding(24.f);
        m_panel->addView(m_contrlPanel);

        // 右侧视图栏
        m_viewPanel = new brls::Box();
        m_viewPanel->setAxis(brls::Axis::COLUMN);
        m_viewPanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_viewPanel->setAlignItems(brls::AlignItems::FLEX_START);
        m_viewPanel->setHeightPercentage(100.f);
        m_viewPanel->setGrow(1.f);
        m_viewPanel->setBackground(brls::ViewBackground::NONE);
        m_viewPanel->setPadding(16.f);
        m_panel->addView(m_viewPanel);

        // 标题
        m_title = new brls::Label();
        m_title->setText("游戏菜单");
        m_title->setFontSize(24.f);
        m_title->setMarginBottom(24.f);
        m_title->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        m_contrlPanel->addView(m_title);

        // ── 创建 6 个菜单按钮 ──────────────────────────────────────────────

        // 1. 返回游戏（无面板）
        m_btnResume = _createMenuButton("▶  返回游戏", [this]() {
            if (m_onResume) m_onResume();
        });
        m_contrlPanel->addView(m_btnResume);

        // 2. 保存状态（绑定保存状态面板）
        brls::View* savePanel = _createSaveStatePanel();
        m_viewPanel->addView(savePanel);
        m_allPanels.push_back(savePanel);
        m_savePanel = savePanel;
        auto* btnSave = _createMenuButton("💾  保存状态", [](){}, savePanel);
        m_contrlPanel->addView(btnSave);

        // 3. 读取状态（绑定读取状态面板）
        brls::View* loadPanel = _createLoadStatePanel();
        m_viewPanel->addView(loadPanel);
        m_allPanels.push_back(loadPanel);
        m_loadPanel = loadPanel;
        auto* btnLoad = _createMenuButton("📂  读取状态", [](){}, loadPanel);
        m_contrlPanel->addView(btnLoad);

        // 4. 金手指设置（简单占位面板）
        auto* cheatPanel = new brls::Box(brls::Axis::COLUMN);
        cheatPanel->setVisibility(brls::Visibility::GONE);
        cheatPanel->setGrow(1.f);
        cheatPanel->setAlignItems(brls::AlignItems::CENTER);
        cheatPanel->setJustifyContent(brls::JustifyContent::CENTER);
        cheatPanel->setFocusable(false);
        auto* cheatPlaceholder = new brls::Label();
        cheatPlaceholder->setText("金手指设置（待实现）");
        cheatPlaceholder->setFontSize(16.f);
        cheatPlaceholder->setFocusable(false);
        cheatPanel->addView(cheatPlaceholder);
        m_viewPanel->addView(cheatPanel);
        m_allPanels.push_back(cheatPanel);
        auto* btnCheat = _createMenuButton("🎮  金手指设置", [](){}, cheatPanel);
        m_contrlPanel->addView(btnCheat);

        // 5. 画面设置（简单占位面板）
        auto* displayPanel = new brls::Box(brls::Axis::COLUMN);
        displayPanel->setVisibility(brls::Visibility::GONE);
        displayPanel->setGrow(1.f);
        displayPanel->setAlignItems(brls::AlignItems::CENTER);
        displayPanel->setJustifyContent(brls::JustifyContent::CENTER);
        displayPanel->setFocusable(false);
        auto* displayPlaceholder = new brls::Label();
        displayPlaceholder->setText("画面设置（待实现）");
        displayPlaceholder->setFontSize(16.f);
        displayPlaceholder->setFocusable(false);
        displayPanel->addView(displayPlaceholder);
        m_viewPanel->addView(displayPanel);
        m_allPanels.push_back(displayPanel);
        auto* btnDisplay = _createMenuButton("🖥  画面设置", [](){}, displayPanel);
        m_contrlPanel->addView(btnDisplay);

        // 6. 退出游戏（无面板）
        m_btnExit = _createMenuButton("✕  退出游戏", [this]() {
            if (m_onExit) m_onExit();
        });
        m_contrlPanel->addView(m_btnExit);

        this->addView(m_panel);

        auto *mBottonBar = new brls::BottomBar();
        mBottonBar->setWidthPercentage(100.f);
        this->addView(mBottonBar);
    }

    // ============================================================
    // _hideAllPanels
    // ============================================================

    void GameMenuView::_hideAllPanels()
    {
        for (auto* panel : m_allPanels)
        {
            if (panel) {
                panel->setVisibility(brls::Visibility::GONE);
                panel->setFocusable(false);
            }
        }
    }

    // ============================================================
    // _createMenuButton
    // ============================================================

    beiklive::ButtonBox* GameMenuView::_createMenuButton(const std::string& text,
                                                          std::function<void()> onClick,
                                                          brls::View* sonPanel)
    {
        beiklive::ButtonBox* btn = new beiklive::ButtonBox();
        btn->setAxis(brls::Axis::ROW);
        btn->setJustifyContent(brls::JustifyContent::CENTER);
        btn->setAlignItems(brls::AlignItems::CENTER);
        btn->setWidthPercentage(100.f);
        btn->setHeight(48.f);
        btn->setMarginBottom(16.f);
        btn->setFocusable(true);
        btn->setHideHighlightBackground(true);
        btn->setHideHighlightBorder(true);
        btn->setHideClickAnimation(false);

        brls::Label* lbl = new brls::Label();
        lbl->setText(text);
        lbl->setFontSize(18.f);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        btn->addView(lbl);

        if (sonPanel)
        {
            // 绑定面板：focus 时显示面板并设为可 focus，失焦时由其他按钮 focus 触发隐藏
            btn->onFocusGainedCallback = [this, btn, sonPanel]() {
                btn->setBackgroundColor(nvgRGBA(255, 255, 255, 20));
                _hideAllPanels();
                sonPanel->setVisibility(brls::Visibility::VISIBLE);
                sonPanel->setFocusable(true);
                // 保存/读取状态面板：异步刷新槽位信息
                if (m_savePanel && sonPanel == m_savePanel)
                    _refreshStatePanel(true);
                else if (m_loadPanel && sonPanel == m_loadPanel)
                    _refreshStatePanel(false);
            };
            btn->onFocusLostCallback = [btn]() {
                btn->setBackgroundColor(nvgRGBA(0, 0, 0, 10));
            };
            // 点击时将焦点转入面板
            btn->registerClickAction([sonPanel](brls::View*) -> bool {
                brls::Application::giveFocus(sonPanel);
                return true;
            });
        }
        else
        {
            // 无面板按钮：focus 时隐藏所有面板，click 时执行回调
            btn->onFocusGainedCallback = [this, btn]() {
                btn->setBackgroundColor(nvgRGBA(255, 255, 255, 20));
                _hideAllPanels();
            };
            btn->onFocusLostCallback = [btn]() {
                btn->setBackgroundColor(nvgRGBA(0, 0, 0, 10));
            };
            btn->registerClickAction([onClick](brls::View*) -> bool {
                onClick();
                return true;
            });
        }

        return btn;
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
        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        auto* grid = new beiklive::GridBox(2);
        grid->setGrow(1.f);

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
            auto* dialog = new brls::Dialog("确认保存到" + _slotName(slot) + "？");
            dialog->addButton("取消", []() {});
            dialog->addButton("确认", [this, slot]() {
                if (m_saveStateCallback) m_saveStateCallback(slot);
                brls::sync([this]() {
                    if (m_onResume) m_onResume();
                });
            });
            dialog->open();
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
        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        auto* grid = new beiklive::GridBox(2);
        grid->setGrow(1.f);

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
            auto* dialog = new brls::Dialog("确认从" + _slotName(slot) + "读取？");
            dialog->addButton("取消", []() {});
            dialog->addButton("确认", [this, slot]() {
                if (m_loadStateCallback) m_loadStateCallback(slot);
                brls::sync([this]() {
                    if (m_onResume) m_onResume();
                });
            });
            dialog->open();
        };

        wrapper->addView(grid);
        return wrapper;
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

            ASYNC_RELEASE
            brls::sync([this, infos = std::move(infos), isSave]() {
                auto& items = isSave ? m_saveItems : m_loadItems;
                for (int slot = 0; slot < 10 && slot < static_cast<int>(items.size()); ++slot)
                {
                    auto* item = items[slot];
                    if (!item) continue;
                    const auto& info = infos[slot];
                    if (info.exists)
                    {
                        item->setDataLoaded();
                        item->setTitle(_slotName(slot));
                        item->setSubText(info.timeStr.empty() ? "时间未知" : info.timeStr);
                        if (!info.thumbPath.empty())
                            item->setImagePath(info.thumbPath);
                    }
                    else
                    {
                        item->setEmpty(_slotName(slot));
                    }
                }
            });
        });
    }

    void GameMenuView::draw(NVGcontext* vg, float x, float y, float w, float h,
                            brls::Style style, brls::FrameContext* ctx)
    {
        Box::draw(vg, x, y, w, h, style, ctx);
    }

} // namespace beiklive
