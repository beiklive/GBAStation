#pragma once

#include "common.hpp"
#include <borealis.hpp>
#include <functional>



class GameMenu: public brls::Box
{
public:
    GameMenu();
    ~GameMenu();

    /// 设置"返回游戏"回调。菜单关闭时在 UI 线程调用。
    void setCloseCallback(std::function<void()> cb) { m_closeCallback = std::move(cb); }

private:
    std::function<void()> m_closeCallback;
};
