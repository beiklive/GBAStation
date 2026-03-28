#pragma once
#include "core/common.h"
#include "ui/utils/Box.hpp"

#include "ui/utils/FileListView.hpp"
#include "core/Tools.hpp"
#include <functional>
namespace beiklive
{

    class FileListPage : public beiklive::Box
    {
    private:

        beiklive::enums::FilterMode m_filterMode = beiklive::enums::FilterMode::None;
        std::vector<std::string> m_filterExtensions;

        std::string m_currentPath;
        std::string m_previousPath;
        bool m_isAtDriveList = false; // 当前是否处于磁盘列表视图

        std::vector<beiklive::DirListData> m_dirItems;

        beiklive::FileListView* fileListView;

        void refreshDirList(const std::string dirPath, beiklive::ListItemList* items);
        bool passesFilter(const std::string suffix);
        void navigateUp();


    public:
        FileListPage();
        ~FileListPage();

        void updatePath();
        void updateIndex(std::string fileName);
        void setFliter(beiklive::enums::FilterMode mode, std::vector<std::string> extensions);
        void setPath(const std::string path);
        void showDriveList(); // 显示磁盘/驱动器列表

        //打开文件时的回调
        std::function<void(beiklive::DirListData)> onFileSelected;



    };

} // namespace beiklive
