#include "UI/List_view.hpp"
#include "UI/game_view.hpp"

#include <vector>

#include "Utils/fileUtils.hpp"
#include "Utils/strUtils.hpp"

using namespace brls::literals; // for _i18n

// FileListView fileListViewA;
// FileListView fileListViewB;

std::vector<FileListView*> fileViewStack; // 用于保存 FileListView 的栈

#if defined(SWITCH)
#define ROOT_PATH "/"
#elif defined(WIN32)
#define ROOT_PATH "F:/games/GBA"
#else
#define ROOT_PATH "/Users/beiklive"
#endif

std::string G_CurrentDir = ROOT_PATH;

RecyclerCell::RecyclerCell()
{
    this->inflateFromXMLRes("xml/mgba_xml/cell/Img_text_cell.xml");
    // this->getView("img_text_cell_root")->setHighlightAlphaTransparent(true, 0.0f); // 取消选中高亮背景
}

RecyclerCell* RecyclerCell::create()
{
    return new RecyclerCell();
}

void RecyclerCell::onFocusGained()
{
    brls::RecyclerCell::onFocusGained();

    this->accent->setVisibility(brls::Visibility::VISIBLE);
}

void RecyclerCell::onFocusLost()
{
    brls::RecyclerCell::onFocusLost();

    this->accent->setVisibility(brls::Visibility::INVISIBLE);
}

// DATA SOURCE

int DataSource::numberOfSections(brls::RecyclerFrame* recycler)
{
    return 1;
}

int DataSource::numberOfRows(brls::RecyclerFrame* recycler, int section)
{
    return listItems.size();
}

std::string DataSource::titleForHeader(brls::RecyclerFrame* recycler, int section)
{
    return "";
}

RecyclerCell* DataSource::cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    RecyclerCell* item = (RecyclerCell*)recycler->dequeueReusableCell(cell_name);
    item->label->setText(listItems[indexPath.row].text);

    auto theme = brls::Application::getPlatform()->getThemeVariant();
    switch (theme)
    {
        case brls::ThemeVariant::LIGHT:
            item->image->setImageFromRes(listItems[indexPath.row].imageRes + "_light.png");
            break;
        case brls::ThemeVariant::DARK:
            item->image->setImageFromRes(listItems[indexPath.row].imageRes + "_dark.png");
            break;
    }
    return item;
}

// 处理点击事件
void DataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath)
{
    FileListView* fileListView = nullptr;
    if (cell_name == CELL_NAME_HOME)
    {
        if (indexPath.row == 0)
        {
            fileViewStack.clear(); // 清空栈
            G_CurrentDir = ROOT_PATH;
            fileListView = new FileListView();

            // 尝试从配置中获取上次的目录，如果存在且有效，则设置为当前目录
            if(auto lastPath = gameRunner->settingConfig->Get("lastDirectory"); lastPath && lastPath->AsString())
            {
                std::string lastPathStr = *lastPath->AsString();
                if (!lastPathStr.empty())
                {
                    // struct VDir* dir = VDirOpen(lastPathStr.c_str());
                    // if (dir) {
                    //     dir->close(dir);
                    //     G_CurrentDir = lastPathStr;
                    //     brls::Logger::info("Set current path to lastDirectory: {}", lastPathStr);
                    // }
                }
            }


            fileListView->ListCurrentDir(G_CurrentDir);
            fileListView->applyItems();

        }
    }
    else if (cell_name == CELL_NAME_FILE)
    {
        brls::Logger::info("File selected: " + listItems[indexPath.row].text + " at index " + std::to_string(indexPath.row) + " at section " + std::to_string(indexPath.section));
        brls::Logger::info("G_CurrentDir: " + G_CurrentDir);

        if (indexPath.row == 0 && listItems[indexPath.row].text == "..")
        {
            // 返回上一级目录
            std::string selectedPath = beiklive::file::getParentPath(G_CurrentDir);
            if (selectedPath.empty())
            {
                selectedPath = "/";
            }
            G_CurrentDir = selectedPath;
            fileListView = new FileListView();
            fileListView->ListCurrentDir(G_CurrentDir);
            fileViewStack[fileViewStack.size() - 1]->recycler->dismiss(
                []()
                {
                    brls::Logger::debug("pre FileListView dismissed.");
                });
            fileListView->applyItems();
        }
        else
        {
            if (G_CurrentDir == "/")
            {
                G_CurrentDir = "";
            }
            std::string selectedPath = G_CurrentDir + "/" + listItems[indexPath.row].text;
            auto pathType            = beiklive::file::getPathType(selectedPath);
            if (pathType == beiklive::file::PathType::Directory)
            {
                G_CurrentDir = selectedPath;
                fileListView = new FileListView();
                fileListView->ListCurrentDir(G_CurrentDir);
                fileViewStack[fileViewStack.size() - 1]->recycler->dismiss(
                    []()
                    {
                        brls::Logger::debug("pre FileListView dismissed.");
                    });
                fileListView->applyItems();
            }else if(pathType == beiklive::file::PathType::File){
                brls::Logger::debug("Item is file, creating GameView for: " + selectedPath);
                // 创建 gameview并显示


                // BKTODO: 这里名称暂时使用显示名称，后续修改列表数据结构支持实际文件名
                gameRunner->settingConfig->Set("lastDirectory", G_CurrentDir); // 保存当前目录到配置中
                gameRunner->settingConfig->Save(); // 立即保存配置
                gameRunner->gameFile.path = G_CurrentDir;
                gameRunner->gameFile.name = listItems[indexPath.row].text;
                gameRunner->gameFile.displayName = listItems[indexPath.row].text;
                
                #ifdef __SWITCH__
                    auto* frame = new brls::AppletFrame(new GameView(selectedPath));
                    frame->setBackground(brls::ViewBackground::NONE);
                    frame->setHeaderVisibility(brls::Visibility::GONE);
                    frame->setFooterVisibility(brls::Visibility::GONE);
                    brls::Application::pushActivity(new brls::Activity(frame));
                #endif
                brls::Logger::debug("GameView presented");
            }
        }
    }
    else if (cell_name == CELL_NAME_SETTINGS)
    {
        brls::Logger::info("Item selected: " + listItems[indexPath.row].text);
    }

    if (fileListView != nullptr)
    {
        fileListView->getAppletFrameItem()->title = G_CurrentDir;
        fileViewStack.push_back(fileListView); // 将新的 FileListView 添加到栈顶
        recycler->present(fileListView);
    }
    // 更新全局当前路径
    gameRunner->currentPath = G_CurrentDir;

    return;
}

void DataSource::setCellName(std::string name)
{
    this->cell_name = name;
}

ListView::ListView()
{
    // Inflate the tab from the XML file
    this->inflateFromXMLRes("xml/mgba_xml/view/list_view.xml");

    dataSource = new DataSource();
    brls::Logger::debug("ListView created.");
}

ListView::~ListView()
{
    brls::Logger::debug("ListView destroyed.");
}

void ListView::setCellName(std::string name)
{
    cell_name = name;
}
std::vector<ImgTextCell> ListView::getItems()
{
    return dataSource->listItems;
}

void ListView::applyItems()
{
    recycler->estimatedRowHeight = 70;
    recycler->registerCell(cell_name, []()
        { return RecyclerCell::create(); });
    dataSource->setCellName(cell_name);
    recycler->setDataSource(dataSource);
}
void ListView::clearItems()
{
    if (dataSource->listItems.empty())
        return;

    dataSource->listItems.clear();
}
void ListView::addItem(std::string title, std::string imageRes)
{
    brls::Logger::debug("Adding item: " + title + ", " + imageRes);
    dataSource->listItems.push_back(ImgTextCell(title, imageRes));
}

HomeMenuListView::HomeMenuListView()
{
    this->setCellName(CELL_NAME_HOME);
    this->clearItems();

    this->addItem("beiklive/select/file"_i18n, "img/ui/folder");
    this->addItem("beiklive/select/recent"_i18n, "img/ui/history");
    this->addItem("beiklive/select/favorites"_i18n, "img/ui/bookmark");
    this->applyItems();
    brls::Logger::debug("HomeMenuListView created.");
}

HomeMenuListView::~HomeMenuListView()
{
    brls::Logger::debug("HomeMenuListView destroyed.");
}

brls::View* HomeMenuListView::create()
{
    return new HomeMenuListView();
}

FileListView::FileListView()
{
    this->setCellName(CELL_NAME_FILE);
    brls::Logger::debug("FileCell G_CurrentDir: " + G_CurrentDir);
    getAppletFrameItem()->title = G_CurrentDir;

    brls::Logger::debug("FileListView created.");
}

FileListView::~FileListView()
{
    brls::Logger::debug("FileListView destroyed.");
}

brls::View* FileListView::create()
{
    return new FileListView();
}

bool FileListView::FileFliter(std::string fileSuffix)
{
    if (isFilterEnabled)
    {
        for (const std::string& suffix : filterList)
        {
            if (beiklive::string::iequals(suffix, fileSuffix))
            {
                return true;
            }
        }
        return false;
    }
    return true;
}

void FileListView::ListCurrentDir(std::string path)
{

    G_CurrentDir = path;
    brls::Logger::debug("target directory: " + G_CurrentDir);

    std::vector<std::string> files = beiklive::file::listDir(G_CurrentDir);

    this->clearItems();
    if (G_CurrentDir != "/")
    {
        this->addItem("..", "img/file/up");
    }
    for (const std::string& file : files)
    {
        std::string fileName = beiklive::string::extractFileName(file);
        std::string iconPath;
        bool shouldAdd = true; // 默认添加

        auto pathType = beiklive::file::getPathType(file);
        if (pathType == beiklive::file::PathType::Directory)
        {
            iconPath = "img/ui/folder";
        }
        else if (pathType == beiklive::file::PathType::File)
        {
            std::string suffix = beiklive::string::getFileSuffix(fileName);
            if (!this->FileFliter(suffix))
            {
                shouldAdd = false; // 过滤器拒绝，不添加
            }
            else
            {
                // 根据后缀选择对应图标
                if (beiklive::string::iequals(suffix, "gba"))
                    iconPath = "img/file/gba";
                else if (beiklive::string::iequals(suffix, "gb")
                    || beiklive::string::iequals(suffix, "gbc"))
                    iconPath = "img/file/gb";
                else if (beiklive::string::iequals(suffix, "zip"))
                    iconPath = "img/file/zip";
                else if (beiklive::string::iequals(suffix, "png") || beiklive::string::iequals(suffix, "jpg") || beiklive::string::iequals(suffix, "jpeg") || beiklive::string::iequals(suffix, "gif"))
                    iconPath = "img/file/image";
                else
                    iconPath = "img/file/file";
            }
        }
        else
        { // 其他路径类型（如未知）
            iconPath = "img/file/file";
        }

        if (shouldAdd)
        {
            this->addItem(fileName, iconPath);
        }
    }
}