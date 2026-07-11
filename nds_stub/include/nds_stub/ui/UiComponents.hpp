#pragma once

#include <array>
#include <cstdio>
#include <string>
#include <vector>

#include "nds_stub/NdsMenuLayer.hpp"
#include "nds_stub/NdsShaderCatalog.hpp"
#include "nds_stub/ui/UiPrimitives.hpp"

namespace beiklive::nds_stub::ui {

using Gfx::Color;
using Gfx::Vector2f;

struct UiMetrics {
    float screenW;
    float screenH;
    float leftX;
    float leftY;
    float menuW;
    float itemH;
    float itemGap;
    float separatorX;
    float separatorY;
    float separatorH;
    float contentX;
    float contentY;
    float contentW;
    float contentH;
    float contentBodyTop;
    float contentBodyH;
    float saveCardW;
    float saveCardH;
    float saveCardGapX;
    float saveCardGapY;
    float settingStepY;
    float contentScissorPad;
    int saveColumns;
};

const UiMetrics& menuMetrics();
void setMenuMetricsOrientation(int orientation);
int saveSlotColumns();
float contentBodyHeight();
float saveCardHeight();
float saveCardGapY();
float settingStepY();

#define kScreenW (::beiklive::nds_stub::ui::menuMetrics().screenW)
#define kScreenH (::beiklive::nds_stub::ui::menuMetrics().screenH)
#define kLeftX (::beiklive::nds_stub::ui::menuMetrics().leftX)
#define kLeftY (::beiklive::nds_stub::ui::menuMetrics().leftY)
#define kMenuW (::beiklive::nds_stub::ui::menuMetrics().menuW)
#define kItemH (::beiklive::nds_stub::ui::menuMetrics().itemH)
#define kItemGap (::beiklive::nds_stub::ui::menuMetrics().itemGap)
#define kSeparatorX (::beiklive::nds_stub::ui::menuMetrics().separatorX)
#define kContentX (::beiklive::nds_stub::ui::menuMetrics().contentX)
#define kContentY (::beiklive::nds_stub::ui::menuMetrics().contentY)
#define kContentW (::beiklive::nds_stub::ui::menuMetrics().contentW)
#define kContentH (::beiklive::nds_stub::ui::menuMetrics().contentH)

constexpr int itemIndex(NdsMenuLayer::Item item)
{
    return static_cast<int>(item);
}

const char* filterLabel(bool linear);
const char* itemLabel(NdsMenuLayer::Item item);
float menuItemY(int index);

// 绘制菜单背景半透明遮罩
// 从顶部到底部由亮变暗的8条渐变横条，产生深邃的背景层次感
void drawOverlay(float alphaScale = 1.0f);
// 绘制顶部标题栏：显示"游戏菜单"文字 + 底部分割线
void drawHeader(float offsetY = 0.0f);
// 绘制游戏层状态徽标：FPS 左上、快进右上、暂停顶部居中。
void drawGameStatusBadges(double fps,
                          bool showFps,
                          bool fastForwardActive,
                          bool showFastForward,
                          bool paused);
// 绘制左侧菜单中"重置游戏"项上方的分割线，将菜单分为两部分
void drawMenuSeparator(float offsetY = 0.0f);
// 绘制左侧菜单栏：渐变高亮选中项、图标+文字标签列表、分割线
void drawLeftMenu(int selected,
                  int previousSelected,
                  float selectionProgress,
                  bool tabsFocused,
                  float offsetY = 0.0f);
// 绘制底部操作提示栏：半透明背景 + 分割线 + B按钮(返回)和A按钮(确定)图标及文字
void drawFooter(bool contentFocused, bool canDelete, float offsetY = 0.0f);
// 绘制单个存档槽卡片
// 左侧为缩略图区域（已有存档显示"NDS"水印，空槽显示"+"号）
// 右侧为槽位编号和状态文本（"已有状态"或"空存档槽"）
// 选中时卡片放大并显示蓝色发光边框
void drawSaveSlotCard(int slot, Vector2f pos, bool focused, const NdsStateSlotInfo& info, float offsetY = 0.0f);
// 绘制6个存档槽的2x3网格布局
// 保存模式(loadMode=false)所有槽均可保存；读取模式(loadMode=true)仅前2个槽显示为"已有"
void drawSaveSlotGrid(const std::array<NdsStateSlotInfo, 10>& slots,
                      int focusedSlot,
                      bool contentFocused,
                      float offsetX,
                      float scrollY,
                      float opacity,
                      float offsetY = 0.0f);
void drawStateSlotPage(const char* title,
                       const std::array<NdsStateSlotInfo, 10>& slots,
                       int focusedSlot,
                       bool contentFocused,
                       std::uint32_t previewTexture,
                       int previewWidth,
                       int previewHeight,
                       bool previewAttempted,
                       float offsetX,
                       float opacity,
                       float offsetY = 0.0f);
// 绘制通用信息页面：标题 + 分割线 + 描述文本（用于金手指/重置/退出/返回等页）
void drawInfoPage(const char* title, const char* body, float offsetX, float offsetY, float opacity);
// 绘制画面设置页面
// 显示两行设置项：画面过滤（Linear/Nearest）和快进倍率（x1~x4）
// 底部提示"A 切换过滤，左右键调整快进倍率"
void drawDisplayPage(bool linearFiltering,
                     float fastForwardMultiplier,
                     int renderScale,
                     bool integerScale,
                     int layout,
                     int orientation,
                     int screenGap,
                     int focusedRow,
                     bool contentFocused,
                     float offsetX,
                     float offsetY,
                     float opacity,
                     float scrollY = 0.0f);
void drawDeleteDialog(int slot, float opacity);
void drawSyncConfirmDialog(NdsMenuAction action, float opacity);
void drawSyncResultDialog(NdsMenuAction action, int count, float opacity);
void drawBusyDialog(const char* title, const char* body, float opacity);
void drawToast(const std::string& message, float progress, float opacity);
void drawCustomLayoutSidebar(const NdsCustomLayoutSettings& settings,
                             int focusedRow,
                             float progress,
                             float opacity = 1.0f);
void drawOverlaySidebar(const NdsDisplaySettings& display,
                        int focusedRow,
                        float progress,
                        float opacity = 1.0f);
void drawShaderSidebar(const NdsDisplaySettings& display,
                       int focusedRow,
                       float paramScrollY,
                       float progress,
                       float opacity = 1.0f);
void drawShaderListOverlay(const std::vector<NdsShaderListEntry>& entries,
                           const std::vector<std::string>& path,
                           const std::string& currentType,
                           int focusedRow,
                           float scrollY,
                           float opacity = 1.0f);
void drawFilePicker(const std::string& directory,
                    const std::vector<NdsFilePickerEntry>& entries,
                    int focusedRow,
                    float scrollY,
                    std::uint32_t previewTexture,
                    int previewWidth,
                    int previewHeight,
                    const std::string& previewPath,
                    bool previewVisible,
                    float progress,
                    float opacity = 1.0f);
// 绘制右侧内容区框架
// 半透明背景面板 + 当前页面内容 + 页面切换动画（旧页向左滑出，新页从右侧滑入）
void drawTabFrame(NdsMenuLayer::Item item,
                  NdsMenuLayer::Item previousItem,
                  float pageProgress,
                  const NdsDisplaySettings& display,
                  const std::array<NdsStateSlotInfo, 10>& slots,
                  const std::vector<NdsCheatItem>& cheats,
                  const std::vector<int>& visibleCheats,
                  int contentFocus,
                  bool contentFocused,
                  float contentScrollY,
                  std::uint32_t statePreviewTexture,
                  int statePreviewWidth,
                  int statePreviewHeight,
                  bool statePreviewAttempted,
                  float offsetY = 0.0f);

} // namespace beiklive::nds_stub::ui
