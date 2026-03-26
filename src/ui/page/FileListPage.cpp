#include "FileListPage.hpp"

namespace beiklive
{

    FileListPage::FileListPage()
    {
        brls::Logger::debug("FileListPage initialized");
        fileListView = new beiklive::FileListView();
        fileListView->setItemClickListener([this](const beiklive::ListItem& item){
            for(const auto& dirItem : m_dirItems)
            {
                if(dirItem.fullPath == item.data)
                {
                    if(dirItem.itemType == beiklive::enums::FileType::DIRECTORY)
                    {
                        // 目录项，进入目录
                        setPath(item.data);
                        brls::Logger::info("Directory selected: " + item.data);
                    }
                    else
                    {
                        // 文件项，执行操作（如启动游戏）
                        brls::Logger::info("File selected: " + item.data);
                    }
                }

            }
        });
        fileListView->onItemActionBind = [this](beiklive::ListItemCell& cell)
        {
            // 为每个列表项绑定一个设置按键（例如 X 键， 不要给A键设置， A键使用setItemClickListener）
            cell.registerAction(
                "返回上一级",
                brls::BUTTON_X,
                [this](brls::View*) {
                    // 此处设置按键功能
                    navigateUp();
                    return true;
                });
        };



        fileListView->Init();
        this->getContentBox()->addView(fileListView);
        this->showHeader(true);
        this->showFooter(true);
        // this->showBackground(false);
        // this->showShader(false);
    }

    void FileListPage::setFliter(beiklive::enums::FilterMode mode, std::vector<std::string> extensions)
    {
        // 设置过滤模式和扩展名
        m_filterMode = mode;
        m_filterExtensions = extensions;
    }

    FileListPage::~FileListPage()
    {
        brls::Logger::debug("FileListPage destroyed.");
    }

    void FileListPage::setPath(const std::string path)
    {
        m_currentPath = path;
        m_previousPath = m_currentPath;
        brls::Logger::info("Setting path: " + path);
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, path]()
        {
            refreshDirList(path);
            beiklive::ListItemList *items = new beiklive::ListItemList();
            for (const auto &dirItem : m_dirItems)
            {
                items->push_back({dirItem.fileName, dirItem.fileSize, dirItem.iconPath, dirItem.fullPath});
            }

            ASYNC_RELEASE
            brls::sync([this, items]()
            {
                // 构建新的列表数据
                fileListView->setListItems(items);
            });
        });

    }
    bool FileListPage::passesFilter(const std::string suffix)
    {
        if (m_filterMode == beiklive::enums::FilterMode::None)
            return true;
        if (m_filterMode == beiklive::enums::FilterMode::Whitelist)
        {
            for (const auto &ext : m_filterExtensions)
            {
                if (suffix == ext)
                    return true;
            }
            return false; // 白名单模式，未找到匹配项，不通过
        }
        else if (m_filterMode == beiklive::enums::FilterMode::Blacklist)
        {
            for (const auto &ext : m_filterExtensions)
            {
                if (suffix == ext)
                    return false; // 黑名单模式，找到匹配项，不通过
            }
            return true; // 黑名单模式，未找到匹配项，通过
        }
        return true; // 默认通过
    }

    void FileListPage::navigateUp()
    {
        // 实现向上导航的逻辑
        if (m_currentPath.empty())
            return; // 当前路径为空，无需导航
        std::string parentPath = fs::path(m_currentPath).parent_path().string();
        if (parentPath == m_currentPath)
            return; // 已经到达根目录，无需导航
        setPath(parentPath);
        
    }

    void FileListPage::refreshDirList(const std::string dirPath)
    {
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
            return; // 路径无效，返回空

        // 更新当前路径和上一个路径
        m_previousPath = m_currentPath;
        m_currentPath = dirPath;
        // 刷新目录列表
        m_dirItems.clear();
        for (const auto &entry : fs::directory_iterator(dirPath))
        {
            std::string fullPath = entry.path().string();
            std::string name = entry.path().filename().string();
            bool isDir = entry.is_directory();
            // 过滤文件
            if (!isDir)
            {
                std::string suffix = beiklive::tools::getFileExtension(entry.path());
                if (!passesFilter(suffix))
                    continue; // 不通过过滤，跳过此项
            }
            // 补充数据
            m_dirItems.push_back(
                beiklive::DirListData{
                    name,
                    fullPath,
                    beiklive::tools::getIconPath(fullPath),
                    beiklive::tools::getFileType(fullPath),
                    beiklive::tools::getFileSizeString(fullPath),
                    beiklive::tools::countEntries(fullPath),
                });
        }
    }

} // namespace beiklive
