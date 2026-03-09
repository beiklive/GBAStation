#pragma once

#include <borealis.hpp>
#include <cmath>
#include <cctype>
#include <functional>
#include <string>
#include <vector>

#include "common.hpp"

using namespace beiklive;

class BackGroundPage : public brls::Box {
public:
    BackGroundPage();

    /// 批量设置游戏列表（清除旧卡片）
    void setImagePath(const std::string& path);

private:
    brls::Image* m_bgImage = nullptr;
    // brls::Label*           m_titleLabel  = nullptr;

};