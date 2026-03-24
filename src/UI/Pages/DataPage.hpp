#pragma once

#include <borealis.hpp>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

#include "common.hpp"

/// 顶部选项卡框架：选项卡按钮在顶部，内容区域在下方。
/// 接口与 brls::TabFrame 类似，使用 addTab(label, creator) 添加选项卡。
class TopTabFrame : public brls::Box
{
  public:
    TopTabFrame();

    /// 添加一个选项卡
    /// @param label    选项卡按钮显示的文字
    /// @param creator  返回选项卡内容视图的工厂函数
    void addTab(const std::string& label, std::function<brls::View*()> creator);

    /// 切换到指定索引的选项卡（0-based）
    void focusTab(int index);

  private:
    brls::Box*  m_tabBar      = nullptr; ///< 顶部选项卡按钮行
    brls::Box*  m_contentArea = nullptr; ///< 内容区域容器

    struct TabInfo {
        std::string label;
        std::function<brls::View*()> creator;
        brls::Button* button = nullptr;
    };
    std::vector<TabInfo> m_tabs;
    int m_activeIndex = -1;

    void activateTab(int index);
};

/// 游戏数据管理页面。
/// - 标题栏：BrowserHeader 显示"游戏数据"
/// - 主体：TabFrame，侧边栏按 gamedataManager 中的游戏列表填充
/// - 每个游戏的子面板：TopTabFrame（存档、相册、金手指）
class DataPage : public beiklive::UI::BBox
{
  public:
    DataPage();

  private:
    brls::TabFrame* m_tabFrame = nullptr;

    /// 构建某款游戏的 TopTabFrame（包含存档/相册/金手指三个子面板）
    TopTabFrame* buildGamePanel(const std::string& fileName, const std::string& gamePath);

    /// 构建存档子面板：ScrollFrame + 三列 ProImage 缩略图
    brls::View* buildSavesPanel(const std::string& gameStem);

    /// 构建相册子面板：ScrollFrame + 三列 ProImage 图片
    brls::View* buildAlbumPanel(const std::string& gameStem);

    /// 构建金手指子面板：ScrollFrame + DetailCell 列表
    brls::View* buildCheatPanel(const std::string& fileName);
};
