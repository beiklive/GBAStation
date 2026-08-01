//
// Created by cpasjuste on 30/01/18.
//

#include "pemu.h"
#include "ui_menu.h"

#include <algorithm>

using namespace c2d::config;

namespace {
constexpr const char *MENU_RESUME = "继续游戏";
constexpr const char *MENU_RESTART = "重启游戏";
constexpr const char *MENU_SAVE_STATE = "保存状态";
constexpr const char *MENU_LOAD_STATE = "读取状态";
constexpr const char *MENU_EXIT = "退出游戏";
constexpr const char *MENU_TAB_PREFIX = "TAB:";
constexpr const char *MENU_EMPTY = "此页暂无设置";

bool isFrontendMenuAction(const std::string &name) {
    return name == MENU_RESUME || name == MENU_RESTART ||
           name == MENU_SAVE_STATE || name == MENU_LOAD_STATE ||
           name == MENU_EXIT;
}

bool isTabHeader(const std::string &name) {
    return name.compare(0, 4, MENU_TAB_PREFIX) == 0;
}

bool isSelectableCustomAction(const std::string &name) {
    return isFrontendMenuAction(name) || name == "STATES" || name == "QUIT";
}

std::string translateGroupName(const std::string &name) {
    if (name == "UI_FILTERING") return "过滤";
    if (name == "UI_OPTIONS") return "界面";
    if (name == "EMULATION") return "模拟";
    if (name == "GAMEPAD") return "按键";
    if (name == "KEYBOARD") return "键盘";
    if (name == "ROMS") return "游戏目录";
    if (name == "OTHER") return "其他";
    return name;
}

std::string translateOptionName(const std::string &name) {
    if (name == "FILTER_FAVORITES") return "只显示收藏";
    if (name == "FILTER_MISSING") return "隐藏缺失游戏";
    if (name == "FILTER_CLONES") return "隐藏克隆版本";
    if (name == "FILTER_SYSTEM") return "系统";
    if (name == "FILTER_GENRE") return "类型";
    if (name == "FILTER_DATE") return "年份";
    if (name == "FILTER_EDITOR") return "发行商";
    if (name == "FILTER_DEVELOPER") return "开发商";
    if (name == "FILTER_PLAYERS") return "玩家数";
    if (name == "FILTER_RATING") return "评分";
    if (name == "SHOW_ZIP_NAMES") return "显示压缩包名称";
    if (name == "FULLSCREEN") return "全屏";
    if (name == "SKIN_ASPECT") return "菜单比例";
    if (name == "FONT_SCALING") return "字体缩放";
    if (name == "VIDEO_SNAP_DELAY") return "视频预览延迟";
    if (name == "SKIN") return "菜单皮肤";
    if (name == "SCALING") return "画面缩放";
    if (name == "SCALING_MODE") return "缩放模式";
    if (name == "FILTER") return "纹理过滤";
    if (name == "EFFECT") return "着色器";
    if (name == "WAIT_RENDERING") return "等待渲染";
    if (name == "SHOW_FPS") return "显示帧率";
    if (name == "FORCE_60HZ") return "强制 60Hz";
    if (name == "AUDIO_FREQUENCY") return "音频频率";
    if (name == "AUDIO_INTERPOLATION") return "音频插值";
    if (name == "AUDIO_FM_INTERPOLATION") return "FM 音频插值";
    if (name == "ROTATION") return "竖屏旋转";
    if (name == "NEOBIOS") return "NeoGeo BIOS";
    if (name == "FRAMESKIP") return "跳帧";
    if (name == "JOY_UP") return "上";
    if (name == "JOY_DOWN") return "下";
    if (name == "JOY_LEFT") return "左";
    if (name == "JOY_RIGHT") return "右";
    if (name == "JOY_A") return "按钮 A";
    if (name == "JOY_B") return "按钮 B";
    if (name == "JOY_X") return "按钮 X";
    if (name == "JOY_Y") return "按钮 Y";
    if (name == "JOY_LT") return "L";
    if (name == "JOY_RT") return "R";
    if (name == "JOY_LB") return "ZL";
    if (name == "JOY_RB") return "ZR";
    if (name == "JOY_SELECT") return "投币 / Select";
    if (name == "JOY_START") return "开始";
    if (name == "JOY_MENU1") return "菜单键 1";
    if (name == "JOY_MENU2") return "菜单键 2";
    if (name == "JOY_DEADZONE") return "摇杆死区";
    if (name == "KEY_UP") return "键盘 上";
    if (name == "KEY_DOWN") return "键盘 下";
    if (name == "KEY_LEFT") return "键盘 左";
    if (name == "KEY_RIGHT") return "键盘 右";
    if (name == "KEY_A") return "键盘 A";
    if (name == "KEY_B") return "键盘 B";
    if (name == "KEY_X") return "键盘 X";
    if (name == "KEY_Y") return "键盘 Y";
    if (name == "KEY_LT") return "键盘 L";
    if (name == "KEY_RT") return "键盘 R";
    if (name == "KEY_LB") return "键盘 ZL";
    if (name == "KEY_RB") return "键盘 ZR";
    if (name == "KEY_SELECT") return "键盘 投币";
    if (name == "KEY_START") return "键盘 开始";
    if (name == "KEY_MENU1") return "键盘 菜单1";
    if (name == "KEY_MENU2") return "键盘 菜单2";
    return name;
}

bool groupUsesGameConfig(const std::string &group) {
    return group == "EMULATION" || group == "GAMEPAD" || group == "KEYBOARD";
}

bool isInputConfigGroup(const std::string &group) {
    return group == "GAMEPAD" || group == "KEYBOARD";
}
}

class MenuLine : public c2d::RectangleShape {

public:
    MenuLine(UiMain *u, FloatRect &rect, Skin::TextGroup &tg) : RectangleShape(rect) {
        pMain = u;
        m_textGroup = tg;
        Font *font = pMain->getSkin()->getFont();

        p_name = new Text("OPTION NAME", m_textGroup.size, font);
        p_name->setFillColor(m_textGroup.color);
        p_name->setOutlineThickness(m_textGroup.outlineSize);
        p_name->setOutlineColor(m_textGroup.outlineColor);
        p_name->setOrigin(Origin::Left);
        p_name->setPosition(2 * pMain->getScaling().x, MenuLine::getSize().y / 2);
        p_name->setSizeMax((MenuLine::getSize().x * 0.55f), 0);
        MenuLine::add(p_name);

        p_value = new Text("OPTION VALUE", m_textGroup.size, font);
        p_value->setFillColor(m_textGroup.color);
        p_value->setOutlineThickness(m_textGroup.outlineSize);
        p_value->setOutlineColor(m_textGroup.outlineColor);
        p_value->setOrigin(Origin::Left);
        p_value->setPosition((MenuLine::getSize().x * 0.6f), MenuLine::getSize().y / 2);
        p_value->setSizeMax(MenuLine::getSize().x * 0.38f, 0);
        MenuLine::add(p_value);

        p_sprite = new Sprite();
        MenuLine::add(p_sprite);
    }

    void refresh() {
        set(p_name->getString(), p_option);
    }

    void set(const std::string &name, Option *option) {
        p_option = option;

        // reset
        setVisibility(Visibility::Visible);
        p_sprite->setVisibility(Visibility::Hidden);
        p_name->setString(name);
        p_name->setSizeMax((MenuLine::getSize().x * 0.55f), 0);
        p_value->setVisibility(Visibility::Visible);
        setFillColor(Color::Transparent);

        // this is a menu title (or custom option)
        if (!option) {
            // custom options
            if (isTabHeader(name)) {
                p_name->setString(name.substr(4));
                p_name->setSizeMax((MenuLine::getSize().x * 0.96f), 0);
                p_value->setVisibility(Visibility::Hidden);
                setFillColor(pMain->getUiMenu()->getOutlineColor());
                return;
            }
            if (name == "STATES" || name == "QUIT" || isFrontendMenuAction(name)) {
                p_value->setVisibility(Visibility::Visible);
                p_value->setString(isFrontendMenuAction(name) ? ">" : "GO");
                return;
            }
            if (name == MENU_EMPTY) {
                p_value->setVisibility(Visibility::Hidden);
                return;
            }
            p_value->setVisibility(Visibility::Hidden);
            setFillColor(pMain->getUiMenu()->getOutlineColor());
            return;
        }

        // this is an option
        if (option->getFlags() & PEMUConfig::Flags::INPUT) {
            Skin::Button *button = pMain->getSkin()->getButton(option->getInteger());
            if (button && option->getId() < PEMUConfig::OptId::JOY_DEADZONE) {
                if (button->texture) {
                    p_sprite->setTexture(button->texture, true);
                    p_sprite->setVisibility(Visibility::Visible);
                    p_value->setVisibility(Visibility::Hidden);
                    float scaling = std::min(
                            getSize().x / (float) p_sprite->getSize().x,
                            getSize().y / (float) p_sprite->getSize().y);
                    p_sprite->setScale(scaling, scaling);
                    p_sprite->setPosition((MenuLine::getSize().x * 0.6f), MenuLine::getSize().y / 2);
                    p_sprite->setOrigin(Origin::Left);
                } else {
                    p_sprite->setVisibility(Visibility::Hidden);
                    p_value->setVisibility(Visibility::Visible);
                    p_value->setString(button->name);
                }
            } else {
                char btn[16];
                snprintf(btn, 16, "%i", option->getInteger());
                p_value->setVisibility(Visibility::Visible);
                p_value->setString(btn);
            }
        } else {
            p_value->setVisibility(Visibility::Visible);
            p_value->setString(option->getString());
        }
    }

    UiMain *pMain = nullptr;
    Text *p_name = nullptr;
    Text *p_value = nullptr;
    Sprite *p_sprite = nullptr;
    Option *p_option = nullptr;
    Skin::TextGroup m_textGroup;
};

UiMenu::UiMenu(UiMain *uiMain) : SkinnedRectangle(uiMain, {"OPTIONS_MENU"}) {
    ui = uiMain;
    alpha = UiMenu::getAlpha();

    // menu title
    title = new SkinnedText(uiMain, {"OPTIONS_MENU", "TITLE_TEXT"});
    UiMenu::add(title);

    // retrieve skin config for options items
    textGroup = ui->getSkin()->getText({"OPTIONS_MENU", "ITEMS_TEXT"});

    // calculate number of items shown
    lineHeight = (float) textGroup.size + (2 * ui->getScaling().y);
    maxLines = (int) (UiMenu::getSize().y / lineHeight);
    if ((float) maxLines * lineHeight < UiMenu::getSize().y) {
        lineHeight = UiMenu::getSize().y / (float) maxLines;
    }

    // add selection rectangle (highlight)
    highlight = new RectangleShape({16, 16});
    ui->getSkin()->loadRectangleShape(highlight, {"SKIN_CONFIG", "HIGHLIGHT"});
    highlight->setSize(UiMenu::getSize().x - 2, lineHeight - (highlight->getOutlineThickness() * 2));
    highlight->move(1, 0);
    UiMenu::add(highlight);

    // add options items
    for (unsigned int i = 0; i < (unsigned int) maxLines; i++) {
        FloatRect r = {0, lineHeight * (float) i, UiMenu::getSize().x, lineHeight};
        auto line = new MenuLine(ui, r, textGroup);
        lines.push_back(line);
        UiMenu::add(line);
    }

    // tween
    Vector2f targetPos = {UiMenu::getPosition().x - UiMenu::getSize().x,
                          UiMenu::getPosition().y};
    tweenPosition = new TweenPosition({UiMenu::getPosition()}, targetPos, 0.2f);
    tweenPosition->setState(TweenState::Stopped);
    UiMenu::add(tweenPosition);

    // hide by default
    UiMenu::setVisibility(Visibility::Hidden);
}

void UiMenu::load(bool isGame) {
    isRomMenu = isGame;
    isEmuRunning = ui->getUiEmu()->isVisible();
    Game game = ui->getUiRomList()->getSelection();

    if (isRomMenu) {
        ui->getConfig()->loadGame(game);
        bool useZipName = ui->getConfig()->getOption(PEMUConfig::OptId::UI_SHOW_ZIP_NAMES)->getInteger();
        title->setString(useZipName ? Utility::removeExt(game.path) : game.name);
    } else {
        title->setString("MAIN OPTIONS");
    }

    // set options items
    optionIndex = highlightIndex = 0;
    menu_options.clear();
    frontendGameMenu = isRomMenu && isEmuRunning;
    buildTabs();
    rebuildCurrentTab();

    setAlpha(isEmuRunning ? (uint8_t) (alpha - 50) : (uint8_t) alpha);

    // update options lines/items
    setSelectionAbsolute(0);
    if (hasSelectableOptions() && !isSelectable(menu_options.at(optionIndex + highlightIndex))) {
        onKeyDown();
    } else {
        updateLines();
    }

    // finally, show me
    if (!isVisible()) {
        setLayer(1);
        setVisibility(Visibility::Visible, true);
    }
}

void UiMenu::buildTabs() {
    menu_tabs.clear();
    tabIndex = 0;

    if (frontendGameMenu) {
        menu_tabs.push_back({"游戏", "", true, false});
    }

    auto groups = ui->getConfig()->getGroups();
    for (auto &group: *groups) {
        const std::string groupName = group.getName();
        if (groupName == "ROMS") continue;
        if (isInputConfigGroup(groupName)) continue;

        bool hasVisibleOption = false;
        auto options = group.getOptions();
        for (auto &option: *options) {
            if (option.getFlags() & PEMUConfig::Flags::HIDDEN) continue;
            auto opt = ui->getConfig()->get(option.getId(), isRomMenu && groupUsesGameConfig(groupName));
            if (!opt || isOptionHidden(opt)) continue;
            hasVisibleOption = true;
            break;
        }

        if (hasVisibleOption) {
            menu_tabs.push_back({
                translateGroupName(groupName),
                groupName,
                false,
                isRomMenu && groupUsesGameConfig(groupName)
            });
        }
    }

    if (!frontendGameMenu) {
        menu_tabs.push_back({"其他", "", true, false});
    }
}

void UiMenu::refreshTitle() {
    if (menu_tabs.empty()) {
        title->setString(frontendGameMenu ? "FBNeo 菜单" : "设置");
        return;
    }

    std::string tabs;
    for (size_t i = 0; i < menu_tabs.size(); ++i) {
        if (!tabs.empty()) tabs += "  ";
        tabs += (i == (size_t) tabIndex ? "[" + menu_tabs[i].name + "]" : menu_tabs[i].name);
    }
    title->setString(frontendGameMenu ? "FBNeo 菜单  " + tabs : "设置  " + tabs);
}

void UiMenu::rebuildCurrentTab() {
    menu_options.clear();

    if (menu_tabs.empty()) {
        menu_options.push_back({MENU_EMPTY, nullptr});
        return;
    }

    const auto &tab = menu_tabs.at(tabIndex);
    menu_options.push_back({std::string(MENU_TAB_PREFIX) + "ZL/ZR 切换：" + tab.name, nullptr});

    if (tab.actionPage) {
        if (frontendGameMenu) {
            menu_options.push_back({MENU_RESUME, nullptr});
            menu_options.push_back({MENU_RESTART, nullptr});
            menu_options.push_back({MENU_SAVE_STATE, nullptr});
            menu_options.push_back({MENU_LOAD_STATE, nullptr});
            menu_options.push_back({MENU_EXIT, nullptr});
        } else {
            if (isRomMenu) menu_options.push_back({"STATES", nullptr});
            menu_options.push_back({"QUIT", nullptr});
        }
    } else {
        auto group = ui->getConfig()->getGroup(tab.group);
        if (group) {
            auto options = group->getOptions();
            for (auto &option: *options) {
                if (option.getFlags() & PEMUConfig::Flags::HIDDEN) continue;
                auto opt = ui->getConfig()->get(option.getId(), tab.gameConfig);
                if (!opt || isOptionHidden(opt)) continue;
                menu_options.push_back({translateOptionName(option.getName()), opt});
            }
        }
    }

    if (menu_options.size() == 1) {
        menu_options.push_back({MENU_EMPTY, nullptr});
    }

    optionIndex = highlightIndex = 0;
    refreshTitle();
}

bool UiMenu::isSelectable(const MenuOption &menuOption) const {
    if (menuOption.option) return true;
    return isSelectableCustomAction(menuOption.name);
}

bool UiMenu::hasSelectableOptions() const {
    for (const auto &menuOption: menu_options) {
        if (isSelectable(menuOption)) return true;
    }
    return false;
}

void UiMenu::setSelectionAbsolute(int index) {
    if (menu_options.empty()) {
        optionIndex = highlightIndex = 0;
        return;
    }

    index = std::max(0, std::min(index, (int) menu_options.size() - 1));
    if (index < maxLines) {
        optionIndex = 0;
        highlightIndex = index;
    } else {
        optionIndex = index - maxLines + 1;
        highlightIndex = maxLines - 1;
    }
}

void UiMenu::switchTab(int direction) {
    if (menu_tabs.size() <= 1) return;

    tabIndex += direction;
    if (tabIndex < 0) {
        tabIndex = (int) menu_tabs.size() - 1;
    } else if (tabIndex >= (int) menu_tabs.size()) {
        tabIndex = 0;
    }

    rebuildCurrentTab();
    setSelectionAbsolute(0);
    if (hasSelectableOptions() && !isSelectable(menu_options.at(optionIndex + highlightIndex))) {
        onKeyDown();
    } else {
        updateLines();
    }
}

void UiMenu::updateLines() {
    for (unsigned int i = 0; i < (unsigned int) maxLines; i++) {
        if (optionIndex + i >= menu_options.size()) {
            lines[i]->setVisibility(Visibility::Hidden);
            continue;
        }
        // set line data
        auto menuOption = menu_options.at(optionIndex + i);
        lines[i]->set(menuOption.name, menuOption.option);
        // set highlight position and color
        if ((int) i == highlightIndex) {
            highlight->setPosition({highlight->getPosition().x,
                                    lines[i]->getPosition().y + highlight->getOutlineThickness()});
            lines[i]->p_value->setOutlineColor(getOutlineColor());
        } else {
            lines[i]->p_value->setOutlineColor(textGroup.outlineColor);
        }
    }
}

void UiMenu::onKeyUp() {
    if (menu_options.empty() || !hasSelectableOptions()) return;

    int index = optionIndex + highlightIndex;
    for (int step = 0; step < (int) menu_options.size(); ++step) {
        index--;
        if (index < 0) {
            index = (int) menu_options.size() - 1;
        }
        if (isSelectable(menu_options.at(index))) {
            setSelectionAbsolute(index);
            break;
        }
    }
    updateLines();
}

void UiMenu::onKeyDown() {
    if (menu_options.empty() || !hasSelectableOptions()) return;

    int index = optionIndex + highlightIndex;
    for (int step = 0; step < (int) menu_options.size(); ++step) {
        index++;
        if (index >= (int) menu_options.size()) {
            index = 0;
        }
        if (isSelectable(menu_options.at(index))) {
            setSelectionAbsolute(index);
            break;
        }
    }
    updateLines();
}

bool UiMenu::onInput(c2d::Input::Player *players) {
    unsigned int buttons = players[0].buttons;

    if (ui->getUiStateMenu()->isVisible()) {
        return C2DObject::onInput(players);
    }

    // TAB LEFT / RIGHT
    if (buttons & Input::Button::LB) {
        switchTab(-1);
        return true;
    }
    if (buttons & Input::Button::RB) {
        switchTab(1);
        return true;
    }

    // UP
    if (buttons & Input::Button::Up) {
        onKeyUp();
    }

    // DOWN
    if (buttons & Input::Button::Down) {
        onKeyDown();
    }

    // LEFT /RIGHT
    if (buttons & Input::Button::Left || buttons & Input::Button::Right) {
        if (menu_options.empty() || highlightIndex < 0 || highlightIndex >= (int) lines.size()) {
            return true;
        }
        auto option = lines.at(highlightIndex)->p_option;
        if (!option || option->getArray().size() <= 1) return true;
        needSave = true;
        if (buttons & Input::Button::Left) {
            option->setArrayMovePrev();
        } else {
            option->setArrayMoveNext();
        }
        if (!option->getComment().empty()) {
            ui->getUiStatusBox()->show(option->getComment());
        }

        switch (option->getId()) {
            case PEMUConfig::OptId::UI_FILTER_FAVORITES:
            case PEMUConfig::OptId::UI_FILTER_AVAILABLE:
            case PEMUConfig::OptId::UI_SHOW_ZIP_NAMES:
            case PEMUConfig::OptId::UI_FILTER_CLONES:
            case PEMUConfig::OptId::UI_FILTER_SYSTEM:
            case PEMUConfig::OptId::UI_FILTER_EDITOR:
            case PEMUConfig::OptId::UI_FILTER_DEVELOPER:
            case PEMUConfig::OptId::UI_FILTER_PLAYERS:
            case PEMUConfig::OptId::UI_FILTER_RATING:
            case PEMUConfig::OptId::UI_FILTER_DATE:
            case PEMUConfig::OptId::UI_FILTER_GENRE: {
                std::string name = Utility::toUpper(option->getName());
                std::string value = Utility::toUpper(option->getString());
                if (option->getComment().empty()) {
                    ui->getUiStatusBox()->show("%s: %s", name.c_str(), value.c_str());
                }
                ui->getUiRomList()->updateRomList();
                break;
            }

            case PEMUConfig::OptId::EMU_ROTATION:
            case PEMUConfig::OptId::EMU_SCALING:
                if (isEmuRunning) {
                    ui->getUiEmu()->getVideo()->updateScaling();
                    auto gw = (float) ui->getUiEmu()->getVideo()->getTextureRect().width;
                    auto gh = (float) ui->getUiEmu()->getVideo()->getTextureRect().height;
                    float gr = std::max(
                            (float) ui->getUiEmu()->getVideo()->aspect.x /
                            (float) ui->getUiEmu()->getVideo()->aspect.y,
                            (float) ui->getUiEmu()->getVideo()->aspect.y /
                            (float) ui->getUiEmu()->getVideo()->aspect.x);
                    float ow = gw * ui->getUiEmu()->getVideo()->getScale().x;
                    float oh = gh * ui->getUiEmu()->getVideo()->getScale().y;
                    float ratio = std::max(ow / oh, oh / ow);
                    ui->getUiStatusBox()->show(
                            "GAME: %ix%i - RATIO: %.2f | OUTPUT: %ix%i - RATIO: %.2f - SCALING: %.2fx%.2f",
                            (int) gw, (int) gh, gr, (int) ow, (int) oh, ratio,
                            ui->getUiEmu()->getVideo()->getScale().x, ui->getUiEmu()->getVideo()->getScale().y);
                }
                break;
            case PEMUConfig::OptId::EMU_SCALING_MODE:
                if (option->getString() == "AUTO") {
                    ui->getUiStatusBox()->show("TRY TO KEEP INTEGER SCALING IF ASPECT RATIO IS NOT TOO DIVERGENT");
                } else if (option->getString() == "ASPECT") {
                    ui->getUiStatusBox()->show("KEEP GAME ASPECT RATIO - SOME SHADERS MAY NOT RENDER CORRECTLY");
                } else {
                    ui->getUiStatusBox()->show(
                            "FORCE INTEGER SCALING - ASPECT RATIO MAY BE WRONG BUT SHADERS WILL RENDER CORRECTLY");
                }
                if (isEmuRunning) {
                    ui->getUiEmu()->getVideo()->updateScaling();
                }
                break;
            case PEMUConfig::OptId::EMU_FILTER:
                if (isEmuRunning) {
                    ui->getUiEmu()->getVideo()->setFilter((Texture::Filter) option->getArrayIndex());
                }
                break;
            case PEMUConfig::OptId::EMU_SHADER:
                if (isEmuRunning) {
                    ui->getUiEmu()->getVideo()->setShader(option->getArrayIndex());
                    ui->getUiStatusBox()->show(option->getString());
                }
                break;
#ifdef __VITA__
                case PEMUConfig::OptId::EMU_WAIT_RENDERING:
                        if (isEmuRunning) {
                            ((PSP2Renderer *) ui)->setWaitRendering(option->getInteger());
                        }
                        break;
#endif
            case PEMUConfig::OptId::UI_VIDEO_SNAP_DELAY:
                ui->getUiRomList()->setVideoSnapDelay(option->getInteger());
                break;

            default:
                break;
        }

        // update option line
        lines.at(highlightIndex)->refresh();
    }

    // FIRE1 (ENTER)
    if (buttons & Input::Button::A) {
        if (menu_options.empty() || highlightIndex < 0 || highlightIndex >= (int) lines.size()) {
            return true;
        }
        auto option = lines.at(highlightIndex)->p_option;
        const std::string actionName = lines.at(highlightIndex)->p_name->getString();
        if (frontendGameMenu && isFrontendMenuAction(actionName)) {
            if (actionName == MENU_RESUME) {
                setVisibility(Visibility::Hidden, true);
                ui->getUiEmu()->resume();
            } else if (actionName == MENU_RESTART) {
                ss_api::Game game = ui->getUiEmu()->getCurrentGame();
                const bool exitOnStop = ui->getUiEmu()->getExitOnStop();
                setVisibility(Visibility::Hidden, true);
                ui->getConfig()->loadGame(game);
                ui->getUiEmu()->setExitOnStop(false);
                ui->getUiEmu()->stop();
                ui->getUiEmu()->setExitOnStop(exitOnStop);
                ui->getUiEmu()->load(game);
                ui->getInput()->clear();
            } else if (actionName == MENU_SAVE_STATE) {
                ui->getUiStateMenu()->quickSaveSlot(0);
                setVisibility(Visibility::Hidden, true);
                ui->getUiEmu()->resume();
            } else if (actionName == MENU_LOAD_STATE) {
                ui->getUiStateMenu()->quickLoadSlot(0);
                setVisibility(Visibility::Hidden, true);
                ui->getUiEmu()->resume();
            } else if (actionName == MENU_EXIT) {
                setVisibility(Visibility::Hidden, true);
                ui->getUiEmu()->stop();
                ui->getInput()->clear();
            }
        } else if (option && (option->getFlags() & PEMUConfig::Flags::INPUT)) {
            int new_key = 0;
            int res = ui->getUiMessageBox()->show("重新绑定", "请按下一个按键", "", "", &new_key, 9);
            if (res != MessageBox::TIMEOUT) {
                needSave = true;
                option->setInteger(new_key);
                lines.at(highlightIndex)->refresh();
            }
        } else if (lines.at(highlightIndex)->p_name->getString() == "STATES") {
            setVisibility(Visibility::Hidden, true);
            ui->getUiStateMenu()->setVisibility(Visibility::Visible, true);
        } else if (lines.at(highlightIndex)->p_name->getString() == "QUIT") {
            if (isEmuRunning) {
                setVisibility(Visibility::Hidden, true);
                ui->getUiEmu()->stop();
                ui->getUiRomList()->setVisibility(Visibility::Visible);
                ui->getInput()->clear();
            } else {
                // be sure options are saved before exiting
                if (isRomMenu) ui->getConfig()->saveGame();
                else ui->getConfig()->save();
                needSave = false;
                ui->done = true;
            }
        }
    }

    // FIRE2 (BACK)
    if (buttons & Input::Button::Menu1 || buttons & Input::Button::Menu2 || buttons & Input::Button::B) {
        setVisibility(Visibility::Hidden, true);
        if (isEmuRunning) {
            ui->getUiEmu()->resume();
        }
    }

    return true;
}

void UiMenu::setVisibility(Visibility visibility, bool tweenPlay) {
    if (ui->getUiRomList() && ui->getUiRomList()->isVisible()) {
        ui->getUiRomList()->getBlur()->setVisibility(visibility, true);
    }

    if (visibility == Visibility::Hidden && needSave) {
        if (isRomMenu) ui->getConfig()->saveGame();
        else ui->getConfig()->save();
        needSave = false;
    }

    RectangleShape::setVisibility(visibility, tweenPlay);
}

UiMenu::~UiMenu() {
    printf("~UIMenuNew\n");
}
