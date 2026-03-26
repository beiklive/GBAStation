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
                    if(dirItem.itemType == beiklive::enums::FileType::DIRECTORY ||
                       dirItem.itemType == beiklive::enums::FileType::DRIVE)
                    {
                        setPath(item.data);
                        brls::Logger::info("Entering: " + item.data);
                    }
                    else
                    {
                        // 文件项，执行操作（如启动游戏）
                        brls::Logger::info("File selected: " + item.data);
                    }
                    break;
                }

            }
        });
        fileListView->onItemActionBind = [this](beiklive::ListItemCell& cell)
        {
            // 为每个列表项绑定一个设置按键（例如 X 键， 不要给A键设置， A键使用setItemClickListener）
            cell.registerAction(
                "返回上一级",
                brls::BUTTON_B,
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
        m_isAtDriveList = false;
        brls::Logger::info("Setting path: " + path);
        brls::Application::notify("正在加载目录: " + path);
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, path]()
        {
            try {
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
            } catch (const std::exception& e) {
                brls::Logger::error("refreshDirList exception: " + std::string(e.what()));
                brls::Application::notify("refreshDirList exception: " + std::string(e.what()));
            }
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
        if (m_currentPath.empty() || m_isAtDriveList)
            return; // 当前已在磁盘列表，无需再向上
        std::string parentPath = fs::path(m_currentPath).parent_path().string();
        if (parentPath == m_currentPath)
        {
            // 已到达根目录，无法继续向上
            brls::Application::notify("已到达根目录");
#ifdef _WIN32
            showDriveList();
#endif
            return;
        }
        setPath(parentPath);
    }

    void FileListPage::showDriveList()
    {
#ifndef _WIN32
        setPath("/");
        return;
#endif
        brls::Logger::info("Showing drive list");
        m_isAtDriveList = true;
        m_currentPath = "";

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]()
        {
            std::vector<std::string> drives = beiklive::tools::getLogicalDrives();
            const std::string driveIcon = beiklive::tools::getIconPath(beiklive::enums::FileType::DRIVE);
            m_dirItems.clear();
            for (const auto& drive : drives)
            {
                m_dirItems.push_back(beiklive::DirListData{
                    drive,
                    drive,
                    driveIcon,
                    beiklive::enums::FileType::DRIVE,
                    "",
                    0,
                });
            }

            beiklive::ListItemList* items = new beiklive::ListItemList();
            for (const auto& driveItem : m_dirItems)
                items->push_back({driveItem.fileName, "本地磁盘", driveItem.iconPath, driveItem.fullPath});

            ASYNC_RELEASE
            brls::sync([this, items]()
            {
                fileListView->setListItems(items);
            });
        });
    }

    void FileListPage::refreshDirList(const std::string dirPath)
    {
        std::error_code ec;
        if (!fs::exists(dirPath, ec) || !fs::is_directory(dirPath, ec))
            return; // 路径无效，返回空

        // 更新当前路径和上一个路径
        m_previousPath = m_currentPath;
        m_currentPath = dirPath;
        // 刷新目录列表
        m_dirItems.clear();
        for (const auto &entry : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec) { ec.clear(); continue; }
            std::error_code entryEc;
            std::string fullPath = entry.path().string();
            std::string name = entry.path().filename().string();
            bool isDir = entry.is_directory(entryEc);
            if (entryEc) continue; // 无法访问的条目，跳过
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
