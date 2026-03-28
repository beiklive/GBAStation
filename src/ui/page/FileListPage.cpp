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
                        brls::Logger::debug("Entering: " + item.data);
                    }else if(dirItem.itemType == beiklive::enums::FileType::NONE)
                    {
                        // 这是返回上一级的特殊项
                        navigateUp();
                        brls::Logger::debug("Navigating up from: " + item.data);
                    }
                    else
                    {
                        // 文件项，执行操作（如启动游戏、查看图片）
                        brls::Logger::debug("File selected: " + item.data);
                        if(onFileSelected)
                            onFileSelected(dirItem);
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
        m_previousPath = m_currentPath;
        m_currentPath = path;
        m_isAtDriveList = false;
        brls::Logger::debug("Setting path: " + m_currentPath);
        brls::Logger::debug("Previous path: " + m_previousPath);
        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN, path]()
        {
            try {
                beiklive::ListItemList *items = new beiklive::ListItemList();
                refreshDirList(path, items);
				brls::Logger::debug("Directory items loaded: " + std::to_string(items->size()));
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
        m_isAtDriveList = true;
        m_currentPath = "";

        ASYNC_RETAIN
        brls::async([ASYNC_TOKEN]()
        {
            std::vector<std::string> drives = beiklive::tools::getLogicalDrives();
            const std::string driveIcon = beiklive::tools::getIconPath(beiklive::enums::FileType::DRIVE);
            m_dirItems.clear();
            beiklive::ListItemList* items = new beiklive::ListItemList();
            for (const auto& drive : drives)
            {
                // 填充 m_dirItems
                m_dirItems.push_back(beiklive::DirListData{
                    drive,
                    drive,
                    driveIcon,
                    beiklive::enums::FileType::DRIVE,
                    "",
                    0,
                    });

                // 直接添加到 items，避免二次遍历
                items->push_back({ drive, "本地磁盘", driveIcon, drive });
            }
            ASYNC_RELEASE
            brls::sync([this, items]()
            {
                fileListView->setListItems(items);
            });
        });
    }

    void FileListPage::refreshDirList(const std::string dirPath, beiklive::ListItemList* items)
    {
        std::error_code ec;
        if (!fs::exists(dirPath, ec) || !fs::is_directory(dirPath, ec))
            return;

        m_previousPath = m_currentPath;
        m_currentPath = dirPath;
        m_dirItems.clear();

        // 添加“返回上一级”条目（此部分无重复调用，无需优化）
        std::string parentPath = fs::path(m_currentPath).parent_path().string();
        if (parentPath != m_currentPath)
        {
            std::string upIcon = beiklive::tools::getIconPath(beiklive::enums::FileType::NONE);
            m_dirItems.push_back({
                "..", m_currentPath, upIcon, beiklive::enums::FileType::NONE,
                "返回上一级", 0
                });
            items->push_back({ "..", "返回上一级", upIcon, m_currentPath });
        }

        // 遍历目录
        for (const auto& entry : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec) { ec.clear(); continue; }

            std::error_code entryEc;
            const auto& path = entry.path();               // 保存 path 对象，避免重复构造
            std::string fullPath = path.string();
            std::string name = path.filename().string();
            bool isDir = entry.is_directory(entryEc);
            if (entryEc) continue;                         // 无法访问的条目跳过

            // 文件过滤（仅对非目录生效）
            if (!isDir)
            {
                std::string suffix = beiklive::tools::getFileExtension(path);
                if (!passesFilter(suffix))
                    continue;
            }

            // --- 核心优化：只调用一次 getFileType，并复用结果 ---
            beiklive::enums::FileType fileType = beiklive::tools::getFileType(fullPath);
            std::string iconPath = beiklive::tools::getIconPath(fileType);

            // 根据类型分别获取大小字符串和条目数（避免不必要的调用）
            std::string sizeStr;
            int entryCount = 0;
            if (isDir)
            {
                entryCount = beiklive::tools::countEntries(fullPath);
                sizeStr = "";          // 目录通常不显示大小，可根据实际需求调整
            }
            else
            {
                sizeStr = beiklive::tools::getFileSizeString(fullPath);
                // entryCount 保持 0（文件无子条目）
            }

            // 添加到 m_dirItems
            m_dirItems.push_back({
                name, fullPath, iconPath, fileType,
                sizeStr, entryCount
                });

            // 添加到 items（显示文本：目录显示条目数，文件显示大小）
            std::string displayText = isDir ? (std::to_string(entryCount) + " items") : sizeStr;
            items->push_back({ name, displayText, iconPath, fullPath });
        }
    }

} // namespace beiklive
