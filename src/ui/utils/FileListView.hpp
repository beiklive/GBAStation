#pragma once
#include <borealis.hpp>
#include <vector>
#include <string>
#include <functional>

#include "ListItem.hpp"
#include "core/common.h"

/* Usage example */
/*  
    auto fileListView = new beiklive::FileListView();
    getContentBox()->addView(fileListView);
    // 绑定手柄按键事件
    fileListView->onItemActionBind = [this](beiklive::ListItemCell& cell)
    {
        // 为每个列表项绑定一个设置按键（例如 X 键， 不要给A键设置， A键使用setItemClickListener）
        cell.registerAction(
            "设置",
            brls::BUTTON_X,
            [&cell](brls::View*) {
                // 此处设置按键功能
                return true;
            });
    };


    fileListView->setItemClickListener([fileListView](const beiklive::ListItem& item){
        beiklive::ListItemList *items = new beiklive::ListItemList();
        for (int i = 1; i <= 20; i++)
        {
            items->push_back({"SubGame " + std::to_string(i), "SubTitle " + std::to_string(i), "resources\\img\\ui\\light\\bangzhu_64.png", "data" + std::to_string(i)});
        }
        fileListView->setListItems(items);
    });
    beiklive::ListItemList *items = new beiklive::ListItemList();
    for (int i = 1; i <= 20; i++)
    {
        items->push_back({"Game " + std::to_string(i), "SubTitle " + std::to_string(i), "resources\\img\\ui\\light\\bangzhu_64.png", "data" + std::to_string(i)});
    }
    fileListView->Init();
    fileListView->setListItems(items);
*/

namespace beiklive
{
    /**
     * @brief RecyclerDataSource 是一个抽象基类，用于为 RecyclerFrame 提供数据和行为。
     *
     * 该类定义了一组虚函数，子类需要实现这些函数以自定义 RecyclerFrame 的内容、布局和交互行为。
     *
     * 方法说明：
     * - numberOfSections: 返回 RecyclerFrame 中的分区数量，默认实现为 1。
     * - numberOfRows: 返回指定分区中的行数，默认实现为 0。
     * - cellForRow: 返回指定位置的单元格对象，默认实现为 nullptr。
     * - cellForHeader: 返回指定分区的表头单元格对象，需子类实现。
     * - heightForRow: 返回指定位置行的高度，返回 -1 表示自动缩放，默认实现为 -1。
     * - heightForHeader: 返回指定分区表头的高度，返回 -1 表示自动缩放，需子类实现。
     * - titleForHeader: 返回指定分区表头的标题，默认实现为空字符串。
     * - didSelectRowAt: 当某一行被选中时调用，默认实现为空。

     */
    class FileListDataSource
        : public brls::RecyclerDataSource
    {
    public:
        int numberOfSections(brls::RecyclerFrame* recycler) override;
        int numberOfRows(brls::RecyclerFrame* recycler, int section) override;
        beiklive::ListItemCell* cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) override;
        void didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath) override;
        std::string titleForHeader(brls::RecyclerFrame* recycler, int section) override;

        std::function<void(const beiklive::ListItem&)> onItemClicked; // 点击列表项时的回调函数，参数为被点击的游戏条目
        void setListItems(ListItemList* items);
    private:
        ListItemList* m_items = nullptr; // 存储文件路径列表
    };






    class FileListView : public brls::Box
    {
    public:
        FileListView();

        // 设置列表项点击事件的回调函数
        void setItemClickListener(std::function<void(const beiklive::ListItem&)> listener);
        // 更新列表数据并刷新 UI
        void setListItems(ListItemList* items);
        // 设置item的按键绑定
        std::function<void(beiklive::ListItemCell&)> onItemActionBind;

        void Init();

    private:
        brls::RecyclerFrame*  m_recycler = nullptr;
        FileListDataSource* m_dataSource = nullptr;

    };
}
