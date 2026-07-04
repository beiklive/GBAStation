#pragma once

#include <cstdio>

#include "nds_stub/NdsMenuLayer.hpp"
#include "nds_stub/ui/UiPrimitives.hpp"

namespace beiklive::nds_stub::ui {

using Gfx::Color;
using Gfx::Vector2f;

constexpr float kScreenW = 1280.0f;
constexpr float kScreenH = 720.0f;
constexpr float kLeftX = 56.0f;
constexpr float kLeftY = 120.0f;
constexpr float kMenuW = 280.0f;
constexpr float kItemH = 58.0f;
constexpr float kItemGap = 18.0f;
constexpr float kSeparatorX = 340.0f;
constexpr float kContentX = 380.0f;
constexpr float kContentY = 110.0f;
constexpr float kContentW = 840.0f;
constexpr float kContentH = 520.0f;

constexpr int itemIndex(NdsMenuLayer::Item item)
{
    return static_cast<int>(item);
}

const char* filterLabel(bool linear);
const char* itemLabel(NdsMenuLayer::Item item);
const char* itemIcon(NdsMenuLayer::Item item);
float menuItemY(int index);

// 绘制菜单背景半透明遮罩
// 从顶部到底部由亮变暗的8条渐变横条，产生深邃的背景层次感
void drawOverlay();
// 绘制顶部标题栏：显示"游戏菜单"文字 + 底部分割线
void drawHeader();
// 绘制左侧菜单中"重置游戏"项上方的分割线，将菜单分为两部分
void drawMenuSeparator();
// 绘制左侧菜单栏：渐变高亮选中项、图标+文字标签列表、分割线
void drawLeftMenu(int selected, int previousSelected, float selectionProgress);
// 绘制底部操作提示栏：半透明背景 + 分割线 + B按钮(返回)和A按钮(确定)图标及文字
void drawFooter();
// 绘制单个存档槽卡片
// 左侧为缩略图区域（已有存档显示"NDS"水印，空槽显示"+"号）
// 右侧为槽位编号和状态文本（"已有状态"或"空存档槽"）
// 选中时卡片放大并显示蓝色发光边框
void drawSaveSlotCard(int slot, Vector2f pos, bool focused, bool existing);
// 绘制6个存档槽的2x3网格布局
// 保存模式(loadMode=false)所有槽均可保存；读取模式(loadMode=true)仅前2个槽显示为"已有"
void drawSaveSlotGrid(bool loadMode);
// 绘制通用信息页面：标题 + 分割线 + 描述文本（用于金手指/重置/退出/返回等页）
void drawInfoPage(const char* title, const char* body, float offsetX, float opacity);
// 绘制画面设置页面
// 显示两行设置项：画面过滤（Linear/Nearest）和快进倍率（x1~x4）
// 底部提示"A 切换过滤，左右键调整快进倍率"
void drawDisplayPage(bool linearFiltering, int fastForwardMultiplier, float offsetX, float opacity);
// 绘制右侧内容区框架
// 半透明背景面板 + 当前页面内容 + 页面切换动画（旧页向左滑出，新页从右侧滑入）
void drawTabFrame(NdsMenuLayer::Item item,
                  NdsMenuLayer::Item previousItem,
                  float pageProgress,
                  bool linearFiltering,
                  int fastForwardMultiplier);

} // namespace beiklive::nds_stub::ui
