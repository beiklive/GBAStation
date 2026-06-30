#include "GameDetailPage.hpp"
#include "core/Tools.hpp"
#include "core/cheat/CheatSystem.hpp"

#include <filesystem>
#include <fstream>

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
    // ============================================================

    std::string GameDetailPage::_getStatePath(int slot) const
    {
        std::string dir = m_entry.savePath.empty()
                          ? beiklive::path::savePath()
                          : m_entry.savePath;
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        return beiklive::tools::getStatePath(dir, m_entry.path, slot);
    }

    std::string GameDetailPage::_getStateThumbPath(int slot) const
    {
        return beiklive::tools::getStateThumbPath(
            m_entry.savePath.empty() ? beiklive::path::savePath() : m_entry.savePath,
            m_entry.path, slot);
    }

    std::string GameDetailPage::_slotName(int slot)
    {
        return beiklive::tools::slotName(slot);
    }

    // ============================================================
    // _initLayout
    // ============================================================

    void GameDetailPage::_initLayout()
    {
        auto* mainRow = new brls::Box(brls::Axis::ROW);
        mainRow->setGrow(1.f);
        mainRow->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(mainRow);

        // ── 左侧按钮栏 ──
        m_leftPanel = new brls::Box(brls::Axis::COLUMN);
        m_leftPanel->setWidthPercentage(25.f);
        m_leftPanel->setHeightPercentage(100.f);
        m_leftPanel->setAlignItems(brls::AlignItems::FLEX_END);
        m_leftPanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_leftPanel->setPadding(24.f);
        m_leftPanel->setBackgroundColor(nvgRGBA(0, 0, 0, 20));
        m_leftPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_leftPanel);

        // ── 右侧内容区 ──
        m_rightPanel = new brls::Box(brls::Axis::COLUMN);
        m_rightPanel->setGrow(1.f);
        m_rightPanel->setHeightPercentage(100.f);
        m_rightPanel->setAlignItems(brls::AlignItems::FLEX_START);
        m_rightPanel->setJustifyContent(brls::JustifyContent::FLEX_START);
        m_rightPanel->setPadding(16.f);
        m_rightPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_rightPanel);

        m_rightPanel->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
            brls::sync([this]() {
                brls::Application::giveFocus(m_leftPanel->getDefaultFocus());
            });
            return true;
        });

        // ── 存档面板 ──
        m_savePanel = _createSavePanel();
        m_rightPanel->addView(m_savePanel);
        m_allPanels.push_back(m_savePanel);

        // ── 金手指面板 ──
        auto* cheatWrapper = _createCheatPanel();
        m_rightPanel->addView(cheatWrapper);
        m_allPanels.push_back(cheatWrapper);

        // ── 成就面板（占位）──
        auto* achieveWrapper = _createAchievePanel();
        m_rightPanel->addView(achieveWrapper);
        m_allPanels.push_back(achieveWrapper);

        // ── 左侧功能按钮 ──
        auto* btnSave    = _createMenuButton("存档",   m_savePanel);
        auto* btnCheat   = _createMenuButton("金手指", cheatWrapper);
        auto* btnAchieve = _createMenuButton("成就",   achieveWrapper);

        m_leftPanel->addView(btnSave);
        m_leftPanel->addView(btnCheat);
        m_leftPanel->addView(btnAchieve);

        mainRow->addView(m_leftPanel);
        mainRow->addView(m_rightPanel);

        this->getContentBox()->addView(mainRow);

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

        btn->onFocusGainedCallback = [this, btn, panel]() {
            btn->setBackgroundColor(nvgRGBA(255, 255, 255, 30));
            _hideAllPanels();
            panel->setVisibility(brls::Visibility::VISIBLE);
            if (panel == m_savePanel)
                _refreshSaveList();
            else if (panel == m_cheatPanel || panel == m_cheatPanel->getParent())
                _refreshCheatList();
        };
        btn->onFocusLostCallback = [btn]() {
            btn->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
        };
        btn->registerClickAction([panel](brls::View*) -> bool {
            brls::Application::giveFocus(panel);
            return true;
        });

        return btn;
    }

    // ============================================================
    // 存档面板
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

            int captSlot = slot;
            item->registerAction("删除", brls::BUTTON_X, [this, captSlot](brls::View*) -> bool {
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
        m_saveGrid->commit();

        wrapper->addView(m_saveGrid);
        return wrapper;
    }

    void GameDetailPage::_refreshSaveList()
    {
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
                _refreshSaveList();
            });
        });
    }

    // ============================================================
    // 金手指面板
    // ============================================================

    std::string GameDetailPage::_getCheatPath() const
    {
        if (!m_entry.cheatPath.empty())
            return m_entry.cheatPath;

        std::string stem;
        if (!m_entry.path.empty())
            stem = fs::path(m_entry.path).stem().string();
        else
            stem = "game";

        return beiklive::path::cheatPath() + "/" + stem + ".cht";
    }

    brls::View* GameDetailPage::_createCheatPanel()
    {
        auto* wrapper = new brls::Box(brls::Axis::COLUMN);
        wrapper->setGrow(1.f);
        wrapper->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(wrapper);

        auto* titleLabel = new brls::Label();
        titleLabel->setText("金手指");
        titleLabel->setFontSize(24.f);
        titleLabel->setMarginBottom(8.f);
        titleLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        titleLabel->setFocusable(false);
        wrapper->addView(titleLabel);

        m_cheatScroll = new brls::ScrollingFrame();
        m_cheatScroll->setGrow(1.f);
        m_cheatScroll->setFocusable(false);

        m_cheatListBox = new brls::Box(brls::Axis::COLUMN);
        m_cheatListBox->setGrow(1.f);
        m_cheatListBox->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_cheatListBox);

        m_cheatScroll->setContentView(m_cheatListBox);
        wrapper->addView(m_cheatScroll);

        m_cheatPanel = wrapper;
        return wrapper;
    }

    void GameDetailPage::_refreshCheatList()
    {
        if (!m_cheatListBox) return;
        m_cheatListBox->clearViews(true);

        std::string path = _getCheatPath();
        m_cheatEntries = beiklive::cheat::loadCheats({path, m_entry.path, m_entry.platform}).entries;

        if (m_cheatEntries.empty())
        {
            auto* emptyLbl = new brls::Label();
            emptyLbl->setText("无金手指条目");
            emptyLbl->setFontSize(16.f);
            emptyLbl->setTextColor(nvgRGBA(150, 150, 150, 255));
            emptyLbl->setHorizontalAlign(brls::HorizontalAlign::CENTER);
            emptyLbl->setFocusable(false);
            emptyLbl->setMarginTop(20.f);
            m_cheatListBox->addView(emptyLbl);
            return;
        }

        for (size_t i = 0; i < m_cheatEntries.size(); ++i)
        {
            const auto& cheat = m_cheatEntries[i];
            int idx = static_cast<int>(i);

            auto* btn = new beiklive::ButtonBox();
            btn->setAxis(brls::Axis::ROW);
            btn->setJustifyContent(brls::JustifyContent::CENTER);
            btn->setAlignItems(brls::AlignItems::CENTER);
            btn->setWidthPercentage(95.f);
            btn->setHeight(46.f);
            btn->setMarginBottom(4.f);
            btn->setFocusable(true);
            btn->setHideHighlightBackground(true);
            btn->setHideHighlightBorder(true);
            btn->setHideClickAnimation(false);

            std::string displayText = cheat.desc;
            if (!cheat.enabled)
                displayText = "[禁用] " + displayText;

            auto* lbl = new brls::Label();
            lbl->setText(displayText);
            lbl->setFontSize(16.f);
            lbl->setHorizontalAlign(brls::HorizontalAlign::LEFT);
            lbl->setFocusable(false);
            lbl->setSingleLine(true);
            lbl->setAnimated(true);
            lbl->setAutoAnimate(true);
            lbl->setGrow(1.f);
            btn->addView(lbl);

            // X 键：修改金手指代码
            btn->registerAction("修改代码", brls::BUTTON_X, [this, idx](brls::View*) -> bool {
                if (idx < 0 || idx >= static_cast<int>(m_cheatEntries.size())) return true;
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) return true;
                ime->openForText(
                    [this, idx](std::string text) {
                        if (text.empty() || idx >= static_cast<int>(m_cheatEntries.size())) return;
                        m_cheatEntries[idx].code = text;
                        _saveCheats();
                        _refreshCheatList();
                    },
                    "修改金手指代码",
                    "",
                    256,
                    m_cheatEntries[idx].code,
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                return true;
            });

            // Y 键：删除金手指条目
            btn->registerAction("删除条目", brls::BUTTON_Y, [this, idx](brls::View*) -> bool {
                if (idx < 0 || idx >= static_cast<int>(m_cheatEntries.size())) return true;
                auto* dialog = new brls::Dialog("确认删除金手指 \"" + m_cheatEntries[idx].desc + "\"？");
                dialog->addButton("取消", []() {});
                dialog->addButton("删除", [this, idx]() {
                    _deleteCheat(idx);
                });
                dialog->open();
                return true;
            });

            // ZR 键：修改金手指名称
            btn->registerAction("修改名称", brls::BUTTON_RT, [this, idx](brls::View*) -> bool {
                if (idx < 0 || idx >= static_cast<int>(m_cheatEntries.size())) return true;
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) return true;
                ime->openForText(
                    [this, idx](std::string text) {
                        if (text.empty() || idx >= static_cast<int>(m_cheatEntries.size())) return;
                        m_cheatEntries[idx].desc = text;
                        _saveCheats();
                        _refreshCheatList();
                    },
                    "修改金手指名称",
                    "",
                    128,
                    m_cheatEntries[idx].desc,
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                return true;
            });

            btn->onFocusGainedCallback = [btn]() {
                btn->setBackgroundColor(nvgRGBA(255, 255, 255, 20));
            };
            btn->onFocusLostCallback = [btn]() {
                btn->setBackgroundColor(nvgRGBA(0, 0, 0, 0));
            };

            m_cheatListBox->addView(btn);
        }
    }

    void GameDetailPage::_saveCheats()
    {
        std::string path = _getCheatPath();
        std::string dir = fs::path(path).parent_path().string();
        if (!dir.empty())
        {
            std::error_code ec;
            fs::create_directories(dir, ec);
        }
        beiklive::saveChtFile(path, m_cheatEntries);
    }

    void GameDetailPage::_deleteCheat(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_cheatEntries.size())) return;
        m_cheatEntries.erase(m_cheatEntries.begin() + index);
        _saveCheats();
        _refreshCheatList();
    }

    // ============================================================
    // 成就面板（占位）
    // ============================================================

    brls::View* GameDetailPage::_createAchievePanel()
    {
        m_achievePanel = new brls::Box(brls::Axis::COLUMN);
        m_achievePanel->setGrow(1.f);
        m_achievePanel->setAlignItems(brls::AlignItems::CENTER);
        m_achievePanel->setJustifyContent(brls::JustifyContent::CENTER);
        m_achievePanel->setVisibility(brls::Visibility::GONE);
        m_achievePanel->setFocusable(false);
        auto* label = new brls::Label();
        label->setText("成就（待实现）");
        label->setFontSize(18.f);
        label->setFocusable(false);
        m_achievePanel->addView(label);
        return m_achievePanel;
    }

} // namespace beiklive
