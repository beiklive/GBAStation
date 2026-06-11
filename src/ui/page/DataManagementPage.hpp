#pragma once

#include "core/common.h"
#include "core/Tools.hpp"
#include "ui/widget/Box.hpp"
#include "ui/widget/GridBox.hpp"
#include "ui/widget/GameGridItem.hpp"

namespace beiklive
{
    /**
     * DataManagementPage – 数据管理页面
     *
     * 布局：单个 GridBox（4 列），每项为 GameGridItem（200x250）
     * 展示数据库中所有游戏条目，点击进入 GameDetailPage。
     */
    class DataManagementPage : public beiklive::Box
    {
    public:
        DataManagementPage();
        ~DataManagementPage() = default;

    private:
        beiklive::GridBox* m_grid = nullptr;
        std::vector<beiklive::GameEntry> m_entries;

        void _loadEntries();
        void _rebuildGrid();
    };

} // namespace beiklive
