#include "GameDetailPage.hpp"
#include "core/Tools.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace beiklive
{
    GameDetailPage::GameDetailPage(const beiklive::GameEntry& entry)
        : m_entry(entry)
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle(entry.title.empty() ? entry.path : entry.title);
        this->setFocusable(false);
        _initLayout();
    }

    // ============================================================
    // _getStatePath / _getStateThumbPath
    // 与 GameView 中相同的命名规则：{saveDir}/{stem}.ss{slot}
    // ============================================================

    std::string GameDetailPage::_getStatePath(int slot) const
    {
        std::string dir = m_entry.savePath.empty()
                          ? beiklive::path::savePath()
                          : m_entry.savePath;

        std::string stem;
        if (!m_entry.path.empty())
            stem = fs::path(m_entry.path).stem().string();
        else
            stem = "game";

        if (!dir.empty())
        {
            std::error_code ec;
            fs::create_directories(dir, ec);
        }

        std::string sep;
        if (!dir.empty() && dir.back() != '/' && dir.back() != '\\')
            sep = "/";

        return dir + sep + stem + ".ss" + std::to_string(slot);
    }

    std::string GameDetailPage::_getStateThumbPath(int slot) const
    {
        return _getStatePath(slot) + ".png";
    }

    std::string GameDetailPage::_slotName(int slot)
    {
        return (slot == 0) ? "自动存档" : "槽位 " + std::to_string(slot);
    }

    // ============================================================
    // _initLayout
    // ============================================================

    void GameDetailPage::_initLayout()
    {
        // 主横向容器
        auto* mainRow = new brls::Box(brls::Axis::ROW);
        mainRow->setGrow(1.f);
        mainRow->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(mainRow);

        // ── 左侧按钮栏 ───────────────────────────────────────────────────
        m_leftPanel = new brls::Box(brls::Axis::COLUMN);
        m_leftPanel->setWidthPercentage(25.f);
        m_leftPanel->setHeightPercentage(100.f);
        m_leftPanel->setAlignItems(brls::AlignItems::FLEX_END);
        m_leftPanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_leftPanel->setPadding(24.f);
        m_leftPanel->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
        m_leftPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_leftPanel);

        // ── 右侧内容区 ───────────────────────────────────────────────────
        m_rightPanel = new brls::Box(brls::Axis::COLUMN);
        m_rightPanel->setGrow(1.f);
        m_rightPanel->setHeightPercentage(100.f);
        m_rightPanel->setAlignItems(brls::AlignItems::FLEX_START);
        m_rightPanel->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_rightPanel->setPadding(16.f);
        m_rightPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_rightPanel);

        // 注册 B 键：从右侧面板退回到左侧按钮
        m_rightPanel->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            brls::sync([this]() {
                brls::Application::giveFocus(m_leftPanel->getDefaultFocus());
            });
            return true;
        });

        // ── 创建三个右侧内容面板 ─────────────────────────────────────────

        // 1. 存档面板
        m_savePanel = _createSavePanel();
        m_rightPanel->addView(m_savePanel);
        m_allPanels.push_back(m_savePanel);

        // 2. 金手指面板（占位）
        m_cheatPanel = new brls::Box(brls::Axis::COLUMN);
        m_cheatPanel->setGrow(1.f);
        m_cheatPanel->setAlignItems(brls::AlignItems::CENTER);
        m_cheatPanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_cheatPanel->setVisibility(brls::Visibility::GONE);
        m_cheatPanel->setFocusable(false);
        auto* cheatLabel = new brls::Label();
        cheatLabel->setText("金手指（待实现）");
        cheatLabel->setFontSize(18.f);
        cheatLabel->setFocusable(false);
        m_cheatPanel->addView(cheatLabel);
        m_rightPanel->addView(m_cheatPanel);
        m_allPanels.push_back(m_cheatPanel);

        // 3. 成就面板（占位）
        m_achievePanel = new brls::Box(brls::Axis::COLUMN);
        m_achievePanel->setGrow(1.f);
        m_achievePanel->setAlignItems(brls::AlignItems::CENTER);
        m_achievePanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_achievePanel->setVisibility(brls::Visibility::GONE);
        m_achievePanel->setFocusable(false);
        auto* achieveLabel = new brls::Label();
        achieveLabel->setText("成就（待实现）");
        achieveLabel->setFontSize(18.f);
        achieveLabel->setFocusable(false);
        m_achievePanel->addView(achieveLabel);
        m_rightPanel->addView(m_achievePanel);
        m_allPanels.push_back(m_achievePanel);

        // ── 创建左侧三个功能按钮 ─────────────────────────────────────────
        auto* btnSave    = _createMenuButton("存档",   m_savePanel);
        auto* btnCheat   = _createMenuButton("金手指", m_cheatPanel);
        auto* btnAchieve = _createMenuButton("成就",   m_achievePanel);

        m_leftPanel->addView(btnSave);
        m_leftPanel->addView(btnCheat);
        m_leftPanel->addView(btnAchieve);

        mainRow->addView(m_leftPanel);
        mainRow->addView(m_rightPanel);

        this->getContentBox()->addView(mainRow);

        // 默认显示存档面板并将焦点给存档按钮
        brls::sync([this, btnSave]() {
            brls::Application::giveFocus(btnSave);
        });
    }

    // ============================================================
    // _hideAllPanels
    // ============================================================

    void GameDetailPage::_hideAllPanels()
    {
        for (auto* panel : m_allPanels)
        {
            if (panel)
            {
                panel->setVisibility(brls::Visibility::GONE);
                panel->setFocusable(false);
            }
        }
    }

    // ============================================================
    // _createMenuButton
    // ============================================================

    beiklive::ButtonBox* GameDetailPage::_createMenuButton(const std::string& text,
                                                           brls::View* panel)
    {
        auto* btn = new beiklive::ButtonBox();
        btn->setAxis(brls::Axis::ROW);
        btn->setJustifyContent(brls::JustifyContent::CENTER);
        btn->setAlignItems(brls::AlignItems::CENTER);
        btn->setWidthPercentage(100.f);
        btn->setHeight(52.f);
        btn->setMarginBottom(12.f);
        btn->setFocusable(true);
        btn->setHideHighlightBackground(true);
        btn->setHideHighlightBorder(true);
        btn->setHideClickAnimation(false);

        auto* lbl = new brls::Label();
        lbl->setText(text);
        lbl->setFontSize(20.f);
        lbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        lbl->setFocusable(false);
        btn->addView(lbl);

        // 焦点获得：高亮并显示对应面板
        btn->onFocusGainedCallback = [this, btn, panel]() {
            btn->setBackgroundColor(nvgRGBA(255, 255, 255, 30));
            _hideAllPanels();
            panel->setVisibility(brls::Visibility::VISIBLE);
            // 存档面板：异步刷新存档列表
            if (panel == m_savePanel)
                _refreshSaveList();
        };
        btn->onFocusLostCallback = [btn]() {
            btn->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        };
        // 点击：将焦点转入右侧面板
        btn->registerClickAction([panel](brls::View*) -> bool {
            brls::Application::giveFocus(panel);
            return true;
        });

        return btn;
    }

    // ============================================================
    // _createSavePanel – 创建存档面板（10 个槽位）
    // ============================================================

    brls::View* GameDetailPage::_createSavePanel()
    {
        auto* wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(wrapper);

        auto* titleLabel = new brls::Label();
        titleLabel->setText("存档");
        titleLabel->setFontSize(24.f);
        titleLabel->setMarginBottom(8.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        m_saveGrid = new beiklive::GridBox(2);
        m_saveGrid->setGrow(1.f);

        m_saveItems.clear();
        for (int slot = 0; slot < 10; ++slot)
        {
            auto* item = new beiklive::GridItem(GridItemMode::SAVE_STATE, slot);
            item->setEmpty(_slotName(slot));
            m_saveItems.push_back(item);

            // X 键：删除存档
            int captSlot = slot;
            beiklive::GridItem* captItem = item;
            item->registerAction("删除", brls::BUTTON_X, [this, captSlot, captItem](brls::View*) -> bool {
                std::string statePath = _getStatePath(captSlot);
                std::error_code ec;
                if (!fs::exists(statePath, ec))
                    return true;

                auto* dialog = new brls::Dialog("确认删除" + _slotName(captSlot) + "？");
                dialog->addButton("取消", []() {});
                dialog->addButton("删除", [this, captSlot]() {
                    _deleteSaveFile(captSlot);
                });
                dialog->open();
                return true;
            });

            beiklive::GridItem* capturedItem = item;
            m_saveGrid->addItem([capturedItem]() -> brls::View* { return capturedItem; });
        }

        wrapper->addView(m_saveGrid);
        return wrapper;
    }

    // ============================================================
    // _refreshSaveList – 异步扫描存档目录并刷新显示
    // ============================================================

    void GameDetailPage::_refreshSaveList()
    {
        // 保存 lambda 捕获所需的路径获取函数
        auto getStatePath = [this](int s) { return _getStatePath(s); };
        auto getThumbPath = [this](int s) { return _getStateThumbPath(s); };

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, getStatePath, getThumbPath]() {
            std::vector<SaveFileInfo> infos;
            infos.reserve(10);
            for (int slot = 0; slot < 10; ++slot)
            {
                SaveFileInfo info;
                info.slot      = slot;
                info.statePath = getStatePath(slot);
                info.thumbPath = getThumbPath(slot);
                std::error_code ec;
                info.exists = fs::exists(info.statePath, ec);
                if (info.exists)
                    info.timeStr = beiklive::tools::getFileModTimeStr(info.statePath);
                infos.push_back(std::move(info));
            }

            ASYNC_RELEASE
            brls::sync([this, infos = std::move(infos)]() {
                for (int slot = 0; slot < 10 && slot < static_cast<int>(m_saveItems.size()); ++slot)
                {
                    auto* item = m_saveItems[slot];
                    if (!item) continue;
                    const auto& info = infos[slot];
                    if (info.exists)
                    {
                        item->setDataLoaded();
                        item->setTitle(_slotName(slot));
                        item->setSubText(info.timeStr.empty() ? "时间未知" : info.timeStr);
                        if (!info.thumbPath.empty())
                        {
                            std::error_code ec;
                            if (fs::exists(info.thumbPath, ec))
                                item->setImagePath(info.thumbPath);
                        }
                    }
                    else
                    {
                        item->setEmpty(_slotName(slot));
                    }
                }
            });
        });
    }

    // ============================================================
    // _deleteSaveFile – 删除指定槽位的存档文件和缩略图
    // ============================================================

    void GameDetailPage::_deleteSaveFile(int slot)
    {
        std::string statePath = _getStatePath(slot);
        std::string thumbPath = _getStateThumbPath(slot);

        brls::async([this, slot, statePath, thumbPath]() {
            std::error_code ec;
            bool ok = true;
            if (fs::exists(statePath, ec))
                ok = fs::remove(statePath, ec);
            if (fs::exists(thumbPath, ec))
                fs::remove(thumbPath, ec);

            brls::sync([this, slot, ok]() {
                if (ok)
                    brls::Application::notify("已删除" + _slotName(slot));
                else
                    brls::Application::notify("删除失败");
                // 刷新存档列表
                _refreshSaveList();
            });
        });
    }

} // namespace beiklive
