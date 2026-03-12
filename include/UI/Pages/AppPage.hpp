#pragma once

#include <borealis.hpp>
#include <cmath>
#include <cctype>
#include <functional>
#include <string>
#include <vector>

#include "common.hpp"

using namespace beiklive;


class GameCard : public brls::Box
{
public:
    explicit GameCard(const GameEntry& entry);

    /// 卡片获得焦点时触发（可用于更新底部信息区）
    std::function<void(const GameEntry&)> onFocused;

    /// 按下 A / 触屏点击时触发（用于启动游戏）
    std::function<void(const GameEntry&)> onActivated;

    const GameEntry& getEntry() const { return m_entry; }

    // ── Borealis 虚函数 ──

    void onChildFocusGained(brls::View* directChild, brls::View* focusedView) override;
    void onChildFocusLost(brls::View* directChild, brls::View* focusedView) override;
    void draw(NVGcontext* vg,
              float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;

private:
    GameEntry  m_entry;
    bool       m_focused = false;
    float      m_scale   = 1.0f;   ///< 当前渲染缩放比，由 draw() 平滑插值
    std::string m_title;
    brls::Label*           m_titleLabel  = nullptr;
    brls::Image*           m_coverImage  = nullptr;

    bool  m_clickAnimating = false;
    float m_clickT         = 0.0f;
    float m_clickScale     = 1.0f;

    void triggerClickBounce();

};


class AppPage : public brls::Box {
public:
    AppPage();

    /// 批量设置游戏列表（清除旧卡片）
    void setGames(const std::vector<GameEntry>& games);

    /// 追加单个游戏卡片
    void addGame(const GameEntry& entry);

    /// 游戏被激活时调用（启动游戏的回调）
    std::function<void(const GameEntry&)> onGameSelected;


private:
    brls::HScrollingFrame* m_scroll      = nullptr;
    brls::Box*             m_cardRow     = nullptr;

    void onCardFocused(const GameEntry& entry);
    void onCardActivated(const GameEntry& entry);

    brls::Box*             m_titleRow     = nullptr;
    beiklive::UI::ButtonBar*             m_ButtonRow     = nullptr;

    brls::BottomBar*             m_bottomRow     = nullptr;

    // brls::Label*           m_titleLabel  = nullptr;

};