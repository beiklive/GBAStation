#include "GameMenuView.hpp"
#include "core/Tools.hpp"
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

        m_panel->addTab(
            "金手指设置", 
            BK_RES("img/ui/menu/cheat.png"), 
            nullptr, 
            nullptr, 
            nullptr, 
            cheatPanel
        );

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

        m_panel->addTab(
            "画面设置", 
            BK_RES("img/ui/menu/display.png"), 
            nullptr, 
            nullptr, 
            nullptr, 
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

} // namespace beiklive
