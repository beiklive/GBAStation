#include "FileListPage.hpp"
#include <algorithm>

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
        // 给item绑定按键
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
        // 给item绑定额外焦点事件
        fileListView->onItemFocused = [this](std::string title)
        {
            brls::Logger::debug("Item focused: " + title);
            updateIndex(title);
            // TODO: 文件信息预览也在这里更新
        };
        fileListView->onItemFocusLost = [this](std::string title)
        {
            brls::Logger::debug("Item focus lost: " + title);
        };


        fileListView->Init();
        this->getContentBox()->addView(fileListView);
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("文件浏览");
        this->getHeader()->setInfo("123456");
        // this->showBackground(false);
        // this->showShader(false);
    }

void FileListPage::updatePath()
{
    this->getHeader()->setPath(m_currentPath);
}
void FileListPage::updateIndex(std::string fileName)
{
    // 更新索引逻辑
    int index = 0;
    for(auto& item : m_dirItems)
    {

        if(item.fileName == fileName)
        {
            this->getHeader()->setInfo(std::to_string(index + 1) + "/" + std::to_string(m_dirItems.size()));
            break;
        }
        index++;
    }
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
        brls::Application::blockInputs();

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
                    updatePath();
                    brls::Application::unblockInputs();
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
            return;

        m_previousPath = m_currentPath;
        m_currentPath = dirPath;
        m_dirItems.clear();

        // "返回上一级"条目
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

        // 使用临时结构体分别收集目录和文件，排序后再写入结果
        struct RawEntry {
            std::string name;
            std::string fullPath;
            bool        isDir;
        };

        std::vector<RawEntry> dirs;
        std::vector<RawEntry> files;

        for (const auto& entry : fs::directory_iterator(
                dirPath, fs::directory_options::skip_permission_denied, ec))
        {
            if (ec) { ec.clear(); continue; }

            std::error_code entryEc;
            bool isDir = entry.is_directory(entryEc);
            if (entryEc) continue;

            const auto& path = entry.path();
            std::string name     = path.filename().string();
            std::string fullPath = path.string();

            // 文件过滤（仅对非目录生效）
            {
                std::string suffix = beiklive::tools::getFileExtension(path);
                    continue;
            }

            if (isDir)
                dirs.push_back({ std::move(name), std::move(fullPath), true });
            else
                files.push_back({ std::move(name), std::move(fullPath), false });
        }

        // 按名称排序（不区分大小写）
        auto nameLess = [](const RawEntry& a, const RawEntry& b) {
            std::string la = a.name, lb = b.name;
            for (auto& c : la) c = static_cast<char>(std::tolower((unsigned char)c));
            for (auto& c : lb) c = static_cast<char>(std::tolower((unsigned char)c));
            return la < lb;
        };
        std::sort(dirs.begin(),  dirs.end(),  nameLess);
        std::sort(files.begin(), files.end(), nameLess);

        // 目录在前、文件在后写入结果
        auto appendEntry = [&](const RawEntry& raw) {
            beiklive::enums::FileType fileType = beiklive::tools::getFileType(raw.fullPath);
            std::string iconPath = beiklive::tools::getIconPath(fileType);

            std::string sizeStr;
            size_t entryCount = 0;
            if (raw.isDir) {
                entryCount = beiklive::tools::countEntries(raw.fullPath);
            } else {
                sizeStr = beiklive::tools::getFileSizeString(raw.fullPath);
            }

            m_dirItems.push_back({
                raw.name, raw.fullPath, iconPath, fileType,
                sizeStr, entryCount
            });

            std::string displayText = raw.isDir
                ? (std::to_string(entryCount) + " items")
                : sizeStr;
            items->push_back({ raw.name, displayText, iconPath, raw.fullPath });
        };

        for (const auto& d : dirs)  appendEntry(d);
        for (const auto& f : files) appendEntry(f);
    }

} // namespace beiklive
