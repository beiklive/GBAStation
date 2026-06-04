#include "FlashGameMenuView.hpp"
#include "ui/utils/AnimationHelper.hpp"
#include "ui/utils/DetailCell.hpp"
#include "game/flash/FlashKeymap.hpp"

#include <borealis/views/dropdown.hpp>
#include <borealis/views/dialog.hpp>

namespace beiklive::flash {

FlashGameMenuView::FlashGameMenuView(beiklive::GameEntry gameData)
    : m_gameEntry(std::move(gameData))
{
    _initLayout();
}

FlashGameMenuView::~FlashGameMenuView() {}

void FlashGameMenuView::_initLayout()
{
    this->setFocusable(false);
    this->setAxis(brls::Axis::COLUMN);
    HIDE_BRLS_HIGHLIGHT(this);
    this->setBackgroundColor(nvgRGBA(0, 0, 0, 240));
    this->setWidthPercentage(100.f);
    this->setHeightPercentage(100.f);
    this->setJustifyContent(brls::JustifyContent::CENTER);
    this->setAlignItems(brls::AlignItems::CENTER);

    m_panel = new beiklive::TabFrame();
    HIDE_BRLS_HIGHLIGHT(m_panel);

    m_panel->addTab(
        "返回游戏",
        BK_RES("img/ui/menu/back.png"),
        [this]() { if (m_onResume) m_onResume(); });

    m_panel->registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
        brls::sync([this]() { if (m_onResume) m_onResume(); });
        return true;
    });

    m_keymapPanel = dynamic_cast<brls::Box*>(_createKeymapPanel());
    m_panel->addTab(
        "按键映射",
        BK_RES("img/ui/menu/display.png"),
        nullptr, nullptr, nullptr,
        m_keymapPanel);

    m_panel->addTab(
        "重置游戏",
        BK_RES("img/ui/menu/reset.png"),
        [this]() { if (m_onReset) m_onReset(); });

    m_panel->addTab(
        "退出游戏",
        BK_RES("img/ui/menu/exit.png"),
        [this]() { if (m_onExit) m_onExit(); });

    m_panel->addFinish();
    this->getContentBox()->addView(m_panel);
}

brls::View* FlashGameMenuView::_createKeymapPanel()
{
    _rebuildKeymapPanel();
    return m_keymapPanel;
}

void FlashGameMenuView::_rebuildKeymapPanel()
{
    if (m_keymapPanel) {
        m_keymapPanel->clearViews();
    } else {
        m_keymapPanel = new brls::Box(brls::Axis::COLUMN);
        m_keymapPanel->setVisibility(brls::Visibility::GONE);
        m_keymapPanel->setGrow(1.f);
        m_keymapPanel->setFocusable(false);
        HIDE_BRLS_HIGHLIGHT(m_keymapPanel);
    }

    const auto& bindings = FlashKeymap::getActiveBindings();

    static const int keyCount = sizeof(beiklive::k_flashKeyNames) / sizeof(beiklive::k_flashKeyNames[0]);
    std::vector<std::string> keyOptions;
    for (int i = 0; i < keyCount; ++i)
        keyOptions.push_back(beiklive::k_flashKeyNames[i]);

    for (const auto& btnName : beiklive::k_editableFlashButtons) {
        std::string currentKey = "(none)";
        auto it = bindings.find(btnName);
        if (it != bindings.end())
            currentKey = it->second;

        int selIdx = 0;
        for (int i = 0; i < keyCount; ++i) {
            if (currentKey == beiklive::k_flashKeyNames[i]) {
                selIdx = i;
                break;
            }
        }

        auto* cell = new beiklive::DetailCell();
        cell->setLeftText(btnName);
        cell->setRightText(currentKey);
        cell->setFocusable(true);

        std::string btnNameCopy = btnName;
        cell->registerAction("选择", brls::BUTTON_A, [this, btnNameCopy, keyOptions, selIdx](brls::View*) -> bool {
            auto* dropdown = new brls::Dropdown(
                std::string("绑定 ") + btnNameCopy + " 按键",
                keyOptions,
                [this, btnNameCopy](int idx) {
                    if (m_keyBindingCallback) {
                        if (idx >= 0 && idx < keyCount) {
                            m_keyBindingCallback(btnNameCopy, beiklive::k_flashKeyNames[idx]);
                        } else {
                            m_keyBindingCallback(btnNameCopy, "(none)");
                        }
                    }
                    _rebuildKeymapPanel();
                },
                selIdx,
                [](int) {}
            );
            dropdown->setWidth(300.f);
            dropdown->setHeight(400.f);
            dropdown->show([]() {}, true, 150.f);
            return true;
        });

        m_keymapPanel->addView(cell);
    }
}

void FlashGameMenuView::onShow()
{
    _rebuildKeymapPanel();
}

void FlashGameMenuView::draw(NVGcontext* vg, float x, float y, float w, float h,
                             brls::Style style, brls::FrameContext* ctx)
{
    Box::draw(vg, x, y, w, h, style, ctx);
}

} // namespace beiklive::flash
