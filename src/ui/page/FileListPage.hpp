#pragma once
#include "core/common.h"
#include "UI/utils/FileListView.hpp"
#include "Utils/Tools.hpp"
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

        void refreshDirList(const std::string dirPath);
        bool passesFilter(const std::string suffix);
        void navigateUp();


    public:
        FileListPage();
        ~FileListPage();

        void setFliter(beiklive::enums::FilterMode mode, std::vector<std::string> extensions);
        void setPath(const std::string path);
        void showDriveList(); // 显示磁盘/驱动器列表





    };

} // namespace beiklive
