#include "GameLibraryPage.hpp"
#include "core/forwarder/ForwarderInstaller.hpp"
#include "ui/widget/ButtonBox.hpp"
#include "ui/widget/GridBox.hpp"
#include "ui/widget/GridItem.hpp"
#include "ui/widget/TabFrame.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "ui/view/ImageView.hpp"
#include "core/ThreadPool.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include <borealis/views/applet_frame.hpp>
#include <borealis/views/dropdown.hpp>
#include <borealis/views/image.hpp>
#include <borealis/views/scrolling_frame.hpp>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace
{

namespace fs = std::filesystem;

bool deleteGameFileIfExists(const std::string& path)
{
    if (path.empty())
        return true;

    std::error_code ec;
    if (!std::filesystem::exists(path, ec))
        return true;

    ec.clear();
    return std::filesystem::remove(path, ec) && !ec;
}

bool copyBinaryFile(const fs::path& src, const fs::path& dst, std::string* error = nullptr)
{
    std::error_code ec;
    const fs::path parent = dst.parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent, ec);
        if (ec) {
            if (error) *error = ec.message();
            return false;
        }
    }

    std::ifstream in(src.string(), std::ios::binary);
    if (!in) {
        if (error) *error = "open source failed";
        return false;
    }

    std::ofstream out(dst.string(), std::ios::binary | std::ios::trunc);
    if (!out) {
        if (error) *error = "open target failed";
        return false;
    }

    out << in.rdbuf();
    out.flush();
    if (!out || in.bad()) {
        if (error) *error = "copy stream failed";
        return false;
    }

    return true;
}

std::string gameSaveDir(const beiklive::GameEntry& entry)
{
    std::string dir = entry.savePath.empty()
        ? beiklive::tools::defaultGameSavePath(entry.platform, entry.path)
        : entry.savePath;
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

std::string gameStem(const beiklive::GameEntry& entry)
{
    std::string stem = fs::path(entry.path).stem().string();
    return stem.empty() ? "game" : stem;
}

std::string gameSavPath(const beiklive::GameEntry& entry)
{
    return (fs::path(gameSaveDir(entry)) / (gameStem(entry) + ".sav")).string();
}

std::string timestampForFile()
{
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    localtime_r(&tt, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

std::vector<fs::path> listFilesByExtensions(const std::string& dir,
                                            const std::vector<std::string>& extensions)
{
    std::vector<fs::path> files;
    std::error_code ec;
    if (!fs::exists(dir, ec))
        return files;

    for (const auto& it : fs::directory_iterator(dir, ec)) {
        if (ec || !it.is_regular_file())
            continue;
        std::string ext = it.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end())
            files.push_back(it.path());
    }
    std::sort(files.begin(), files.end());
    return files;
}

} // namespace

namespace beiklive
{
    namespace
    {
        class ScreenshotGridItem : public brls::Box
        {
        public:
            ScreenshotGridItem(std::string imagePath, std::string title, std::string subText)
            {
                setAxis(brls::Axis::COLUMN);
                setFocusable(false);
                setHeight(230.f);
                setPadding(8.f);
                setBackgroundColor(nvgRGBA(255, 255, 255, 12));
                setBorderColor(nvgRGBA(255, 255, 255, 35));
                setBorderThickness(1.f);
                setCornerRadius(4.f);

                auto* image = new brls::Image();
                image->setImageFromFile(imagePath);
                image->setScalingType(brls::ImageScalingType::FIT);
                image->setHeight(168.f);
                image->setWidth(168.f * image->getOriginalImageWidth()/image->getOriginalImageHeight());
                setWidth(180.f * image->getOriginalImageWidth()/image->getOriginalImageHeight());
                image->setFocusable(false);
                addView(image);

                auto* titleLabel = new brls::Label();
                titleLabel->setText(std::move(title));
                titleLabel->setFontSize(15.f);
                titleLabel->setHeight(24.f);
                titleLabel->setSingleLine(true);
                titleLabel->setAnimated(true);
                titleLabel->setAutoAnimate(true);
                titleLabel->setFocusable(false);
                addView(titleLabel);

                auto* subLabel = new brls::Label();
                subLabel->setText(std::move(subText));
                subLabel->setFontSize(13.f);
                subLabel->setHeight(20.f);
                subLabel->setTextColor(nvgRGBA(180, 180, 180, 255));
                subLabel->setSingleLine(true);
                subLabel->setFocusable(false);
                addView(subLabel);
            }
        };

        class GameDataPage : public beiklive::Box
        {
        public:
            explicit GameDataPage(beiklive::GameEntry entry)
                : m_entry(std::move(entry))
            {
                showHeader(true);
                showFooter(true);
                getHeader()->setTitle(m_entry.title.empty() ? gameStem(m_entry) : m_entry.title);
                getHeader()->setPath(m_entry.path);
                setFocusable(false);
                _initLayout();
            }

        private:
            beiklive::GameEntry m_entry;
            beiklive::TabFrame* m_tabs = nullptr;
            beiklive::GridBox* m_stateGrid = nullptr;
            beiklive::GridBox* m_screenshotGrid = nullptr;
            brls::Box* m_batteryBox = nullptr;
            brls::Box* m_batteryActionsBox = nullptr;
            brls::Box* m_backupContainer = nullptr;
            beiklive::ButtonBox* m_exportSavBtn = nullptr;
            std::vector<beiklive::GridItem*> m_stateItems;
            std::vector<brls::View*> m_screenshotItems;
            std::vector<fs::path> m_screenshotPaths;
            std::vector<fs::path> m_backupPaths;

            std::string _saveDir() const { return gameSaveDir(m_entry); }
            std::string _statePath(int slot) const
            {
                return beiklive::tools::getStatePath(_saveDir(), m_entry.path, slot);
            }
            std::string _stateThumbPath(int slot) const
            {
                return beiklive::tools::getStateThumbPath(_saveDir(), m_entry.path, slot);
            }
            std::string _savPath() const { return gameSavPath(m_entry); }

            static beiklive::ButtonBox* _makeButton(const std::string& text,
                                                    const std::string& icon)
            {
                auto* btn = new beiklive::ButtonBox();
                btn->setText(text);
                btn->setIcon(icon);
                btn->setWidthPercentage(100.f);
                btn->setHeight(54.f);
                btn->setMarginBottom(8.f);
                return btn;
            }

            static brls::Label* _makeEmptyLabel(const std::string& text)
            {
                auto* label = new brls::Label();
                label->setText(text);
                label->setFontSize(18.f);
                label->setTextColor(nvgRGBA(180, 180, 180, 255));
                label->setMarginTop(24.f);
                label->setFocusable(false);
                return label;
            }

            static void _giveFocusSoon(brls::View* view)
            {
                if (!view)
                    return;
                brls::sync([view]() {
                    brls::Application::giveFocus(view);
                });
            }

            void _initLayout()
            {
                registerAction("返回", brls::BUTTON_B, [this](brls::View*) -> bool {
                    beiklive::popActivity(this);
                    return true;
                });

                m_tabs = new beiklive::TabFrame();
                m_tabs->setAnimationEnabled(false);

                auto* states = _createStatePanel();
                auto* shots = _createScreenshotPanel();
                auto* battery = _createBatteryPanel();

                m_tabs->addTab("即时存档管理", BK_RES("img/ui/menu/save.png"), nullptr,
                    [this]() {
                        _refreshStateList();
                    }, nullptr, states,
                    m_stateGrid ? m_stateGrid->getItemView(0) : states);
                m_tabs->addTab("游戏图片管理", BK_RES("img/ui/setting/display.png"), nullptr,
                    [this]() {
                        _refreshScreenshotList();
                    }, nullptr, shots,
                    m_screenshotGrid ? m_screenshotGrid->getItemView(0) : shots);
                m_tabs->addTab("电池存档管理", BK_RES("img/ui/menu/save.png"), nullptr,
                    [this]() { _refreshBackupList(); }, nullptr, battery, battery);
                m_tabs->addFinish();

                getContentBox()->addView(m_tabs);
                brls::sync([this]() {
                    if (m_tabs) m_tabs->onShow();
                });
            }

            brls::View* _createStatePanel()
            {
                auto* wrapper = new brls::Box(brls::Axis::COLUMN);
                wrapper->setGrow(1.f);
                wrapper->setFocusable(false);

                m_stateGrid = new beiklive::GridBox(2);
                m_stateGrid->setGrow(1.f);
                m_stateGrid->onItemClicked = [this](int slot) {
                    _confirmDeleteState(slot);
                };
                m_stateGrid->onItemX = [this](int slot) {
                    _confirmDeleteState(slot);
                };
                m_stateItems.clear();
                for (int slot = 0; slot < 10; ++slot) {
                    auto* item = new beiklive::GridItem(GridItemMode::SAVE_STATE, slot);
                    item->setEmpty(beiklive::tools::slotName(slot));
                    m_stateItems.push_back(item);
                    m_stateGrid->addItem([item]() -> brls::View* { return item; });
                }
                m_stateGrid->commit();
                wrapper->addView(m_stateGrid);
                return wrapper;
            }

            brls::View* _createScreenshotPanel()
            {
                auto* wrapper = new brls::Box(brls::Axis::COLUMN);
                wrapper->setGrow(1.f);
                wrapper->setFocusable(false);

                m_screenshotGrid = new beiklive::GridBox(2);
                m_screenshotGrid->setColumns(2);
                m_screenshotGrid->setGrow(1.f);
                m_screenshotGrid->onItemClicked = [this](int index) {
                    _openImagePreview(index);
                };
                m_screenshotGrid->onItemX = [this](int index) {
                    _confirmDeleteScreenshot(index);
                };
                m_screenshotGrid->onItemY = [this](int index) {
                    _confirmSetScreenshotAsCover(index);
                };
                wrapper->addView(m_screenshotGrid);
                return wrapper;
            }

            brls::View* _createBatteryPanel()
            {
                m_batteryBox = new brls::Box(brls::Axis::ROW);
                m_batteryBox->setGrow(1.f);
                m_batteryBox->setFocusable(false);
                m_batteryBox->setWidthPercentage(100.f);
                m_batteryBox->setAlignItems(brls::AlignItems::FLEX_START);

                m_batteryActionsBox = new brls::Box(brls::Axis::COLUMN);
                m_batteryActionsBox->setFocusable(false);
                m_batteryActionsBox->setWidth(320.f);
                m_batteryActionsBox->setMarginRight(18.f);

                auto* listBox = new brls::Box(brls::Axis::COLUMN);
                listBox->setGrow(1.f);
                listBox->setFocusable(false);

                m_exportSavBtn = _makeButton("导出存档", BK_RES("img/ui/menu/save.png"));
                m_exportSavBtn->registerClickAction([this](brls::View*) -> bool {
                    _exportSav();
                    return true;
                });
                m_batteryActionsBox->addView(m_exportSavBtn);

                auto* importBtn = _makeButton("导入存档", BK_RES("img/ui/light/wenjian.png"));
                importBtn->registerClickAction([this](brls::View*) -> bool {
                    _importSav();
                    return true;
                });
                m_batteryActionsBox->addView(importBtn);

                auto* backupBtn = _makeButton("存档备份", BK_RES("img/ui/menu/save.png"));
                backupBtn->registerClickAction([this](brls::View*) -> bool {
                    _backupSav();
                    return true;
                });
                m_batteryActionsBox->addView(backupBtn);

                auto* header = new brls::Header();
                header->setTitle("备份列表");
                listBox->addView(header);

                m_backupContainer = new brls::Box(brls::Axis::COLUMN);
                m_backupContainer->setGrow(1.f);
                m_backupContainer->setHeightPercentage(100.f);
                m_backupContainer->setFocusable(false);
                listBox->addView(m_backupContainer);

                m_batteryBox->addView(m_batteryActionsBox);
                m_batteryBox->addView(listBox);

                return m_batteryBox;
            }

            void _refreshStateList()
            {
                for (int slot = 0; slot < 10 && slot < static_cast<int>(m_stateItems.size()); ++slot) {
                    auto* item = m_stateItems[slot];
                    std::error_code ec;
                    const std::string state = _statePath(slot);
                    const std::string thumb = _stateThumbPath(slot);
                    if (fs::exists(state, ec)) {
                        item->setDataLoaded();
                        item->setTitle(beiklive::tools::slotName(slot));
                        item->setSubText(beiklive::tools::getFileModTimeStr(state));
                        if (fs::exists(thumb, ec))
                            item->setImagePath(thumb);
                    } else {
                        item->setEmpty(beiklive::tools::slotName(slot));
                    }
                }
            }

            void _confirmDeleteState(int slot)
            {
                std::error_code ec;
                if (!fs::exists(_statePath(slot), ec))
                    return;
                auto* dlg = new brls::Dialog("确认删除" + beiklive::tools::slotName(slot) + "？");
                dlg->addButton("取消", []() {});
                dlg->addButton("删除", [this, slot]() {
                    std::error_code removeEc;
                    fs::remove(_statePath(slot), removeEc);
                    fs::remove(_stateThumbPath(slot), removeEc);
                    brls::Application::notify(removeEc ? "删除失败" : "已删除存档");
                    _refreshStateList();
                });
                dlg->open();
            }

            void _refreshScreenshotList(bool refocus = false)
            {
                if (!m_screenshotGrid)
                    return;
                m_screenshotPaths = listFilesByExtensions(_saveDir(), {".png", ".jpg", ".jpeg"});
                m_screenshotItems.clear();
                m_screenshotGrid->clearItems();

                if (m_screenshotPaths.empty()) {
                    auto* empty = _makeEmptyLabel("暂无游戏截图");
                    m_screenshotGrid->addItem([empty]() -> brls::View* { return empty; });
                    m_screenshotGrid->commit();
                    if (refocus)
                        _giveFocusSoon(m_screenshotGrid->getItemView(0));
                    return;
                }

                for (size_t i = 0; i < m_screenshotPaths.size(); ++i) {
                    const std::string path = m_screenshotPaths[i].string();
                    auto* item = new ScreenshotGridItem(
                        path,
                        m_screenshotPaths[i].filename().string(),
                        beiklive::tools::getFileModTimeStr(path));
                    m_screenshotItems.push_back(item);
                    m_screenshotGrid->addItem([item]() -> brls::View* { return item; });
                }
                m_screenshotGrid->commit();
                if (refocus)
                    _giveFocusSoon(m_screenshotGrid->getItemView(0));
            }

            void _openImagePreview(int index)
            {
                if (index < 0 || index >= static_cast<int>(m_screenshotPaths.size()))
                    return;
                auto* page = new beiklive::Box(brls::Axis::COLUMN);
                page->showHeader(false);
                page->showFooter(true);
                page->setGrow(1.f);
                page->setFocusable(false);
                auto* imageView = new beiklive::ImageView(m_screenshotPaths[index].string());
                page->getContentBox()->addView(imageView);
                page->registerAction("关闭", brls::BUTTON_B, [page](brls::View*) -> bool {
                    beiklive::popActivity(page);
                    return true;
                });
                page->registerAction("关闭", brls::BUTTON_A, [page](brls::View*) -> bool {
                    beiklive::popActivity(page);
                    return true;
                });

                auto* frame = new brls::AppletFrame(page);
                HIDE_BRLS_BAR(frame);
                beiklive::pushActivity(frame, this, page);
            }

            void _confirmDeleteScreenshot(int index)
            {
                if (index < 0 || index >= static_cast<int>(m_screenshotPaths.size()))
                    return;
                const fs::path path = m_screenshotPaths[index];
                auto* dlg = new brls::Dialog("确认删除截图\n" + path.filename().string() + "？");
                dlg->addButton("取消", []() {});
                dlg->addButton("删除", [this, path]() {
                    std::error_code ec;
                    fs::remove(path, ec);
                    brls::Application::notify(ec ? "删除失败" : "已删除截图");
                    _refreshScreenshotList(true);
                });
                dlg->open();
            }

            void _confirmSetScreenshotAsCover(int index)
            {
                if (index < 0 || index >= static_cast<int>(m_screenshotPaths.size()))
                    return;
                const std::string cover = m_screenshotPaths[index].string();
                auto* dlg = new brls::Dialog("确认将该截图设置为封面？\n" +
                                             m_screenshotPaths[index].filename().string());
                dlg->addButton("取消", []() {});
                dlg->addButton("确认", [this, cover]() {
                    if (!beiklive::GameDB)
                        return;
                    beiklive::GameDB->set(m_entry.path, "logoPath", nlohmann::json(cover));
                    beiklive::GameDB->flush();
                    m_entry.logoPath = cover;
                    brls::Application::notify("已设置为封面图片");
                });
                dlg->open();
            }

            void _exportSav()
            {
                const std::string src = _savPath();
                std::error_code ec;
                if (!fs::exists(src, ec)) {
                    brls::Application::notify("未找到电池存档");
                    return;
                }
                auto* dlg = new brls::Dialog("确认导出当前电池存档？");
                dlg->addButton("取消", []() {});
                dlg->addButton("导出", [src]() {
                    fs::path exportDir("sdmc:/GBAStation/export");
                    std::string error;
                    if (!copyBinaryFile(src, exportDir / fs::path(src).filename(), &error)) {
                        brls::Logger::warning("导出电池存档失败: {} -> {}, error={}",
                            src, (exportDir / fs::path(src).filename()).string(), error);
                        brls::Application::notify("导出失败");
                        return;
                    }
                    brls::Application::notify("已导出存档");
                });
                dlg->open();
            }

            void _importSav()
            {
                auto* dlg = new brls::Dialog("确认导入外部 .sav 并覆盖当前电池存档？");
                dlg->addButton("取消", []() {});
                dlg->addButton("选择文件", [this]() {
                    beiklive::openFilePicker({"sav"}, [this](const std::string& selected) {
                        std::string error;
                        if (!copyBinaryFile(selected, _savPath(), &error)) {
                            brls::Logger::warning("导入电池存档失败: {} -> {}, error={}",
                                selected, _savPath(), error);
                            brls::Application::notify("导入失败");
                            return;
                        }
                        brls::Application::notify("已导入存档");
                        _refreshBackupList();
                    }, beiklive::path::GetRootPath());
                });
                dlg->open();
            }

            void _backupSav()
            {
                const std::string src = _savPath();
                std::error_code ec;
                if (!fs::exists(src, ec)) {
                    brls::Application::notify("未找到电池存档");
                    return;
                }
                auto* dlg = new brls::Dialog("确认为当前电池存档创建备份？");
                dlg->addButton("取消", []() {});
                dlg->addButton("备份", [this, src]() {
                    fs::path backup = fs::path(src).string() + ".bak_" + timestampForFile();
                    std::string error;
                    if (!copyBinaryFile(src, backup, &error)) {
                        std::error_code removeEc;
                        fs::remove(backup, removeEc);
                        brls::Logger::warning("备份电池存档失败: {} -> {}, error={}",
                            src, backup.string(), error);
                        brls::Application::notify("备份失败");
                        return;
                    }
                    brls::Application::notify("已创建备份");
                    _refreshBackupList();
                });
                dlg->open();
            }

            void _refreshBackupList(bool refocus = false)
            {
                if (!m_backupContainer)
                    return;
                m_backupContainer->clearViews(true);
                m_backupPaths.clear();

                const fs::path sav = _savPath();
                const std::string prefix = sav.filename().string() + ".bak_";
                std::error_code ec;
                for (const auto& it : fs::directory_iterator(_saveDir(), ec)) {
                    if (ec || !it.is_regular_file())
                        continue;
                    const std::string name = it.path().filename().string();
                    if (name.empty() || name.rfind(prefix, 0) != 0)
                        continue;
                    std::error_code sizeEc;
                    if (fs::file_size(it.path(), sizeEc) == 0 || sizeEc) {
                        continue;
                    }
                    m_backupPaths.push_back(it.path());
                }
                std::sort(m_backupPaths.begin(), m_backupPaths.end());

                if (m_backupPaths.empty())
                {
                    m_backupContainer->addView(_makeEmptyLabel("暂无电池存档备份"));
                    if (refocus)
                        _giveFocusSoon(m_exportSavBtn);
                    return;
                }

                auto* scroll = new brls::ScrollingFrame();
                scroll->setMargins(5.f, 5.f, 5.f, 5.f);
                scroll->setGrow(1.f);
                scroll->setFocusable(false);

                auto* list = new brls::Box(brls::Axis::COLUMN);
                list->setGrow(1.f);
                list->setFocusable(false);

                for (size_t i = 0; i < m_backupPaths.size(); ++i) {
                    auto* label = new brls::Label();
                    label->setText(m_backupPaths[i].filename().string());
                    label->setFontSize(17.f);
                    label->setHeight(42.f);
                    label->setWidthPercentage(100.f);
                    label->setFocusable(true);
                    label->setSingleLine(true);
                    label->setAnimated(true);
                    label->setAutoAnimate(true);
                    label->registerAction("还原", brls::BUTTON_A, [this, index = static_cast<int>(i)](brls::View*) -> bool {
                        _confirmRestoreBackup(index);
                        return true;
                    });
                    label->registerAction("删除", brls::BUTTON_X, [this, index = static_cast<int>(i)](brls::View*) -> bool {
                        _confirmDeleteBackup(index);
                        return true;
                    });
                    list->addView(label);
                }

                scroll->setContentView(list);
                m_backupContainer->addView(scroll);
                if (refocus)
                    _giveFocusSoon(list->getDefaultFocus() ? list : m_exportSavBtn);
            }

            void _confirmRestoreBackup(int index)
            {
                if (index < 0 || index >= static_cast<int>(m_backupPaths.size()))
                    return;
                const fs::path backup = m_backupPaths[index];
                auto* dlg = new brls::Dialog("确认还原备份\n" + backup.filename().string() + "？");
                dlg->addButton("取消", []() {});
                dlg->addButton("还原", [this, backup]() {
                    std::string error;
                    if (!copyBinaryFile(backup, _savPath(), &error)) {
                        brls::Logger::warning("还原电池存档失败: {} -> {}, error={}",
                            backup.string(), _savPath(), error);
                        brls::Application::notify("还原失败");
                        return;
                    }
                    brls::Application::notify("已还原存档");
                });
                dlg->open();
            }

            void _confirmDeleteBackup(int index)
            {
                if (index < 0 || index >= static_cast<int>(m_backupPaths.size()))
                    return;
                const fs::path backup = m_backupPaths[index];
                auto* dlg = new brls::Dialog("确认删除备份\n" + backup.filename().string() + "？");
                dlg->addButton("取消", []() {});
                dlg->addButton("删除", [this, backup]() {
                    std::error_code ec;
                    fs::remove(backup, ec);
                    brls::Application::notify(ec ? "删除失败" : "已删除备份");
                    _refreshBackupList(true);
                });
                dlg->open();
            }
        };
    }

    GameLibraryPage::GameLibraryPage()
    {
        this->showHeader(true);
        this->showFooter(true);
        this->getHeader()->setTitle("游戏库");
        this->setFocusable(false);

        m_grid = new GameGridView();
        m_grid->spanCount = 3;
        m_grid->estimatedRowHeight = 120;
        m_grid->estimatedRowSpace = 8;
        m_grid->setTitleFontSize(GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_LIBRARY_TITLE_SIZE, 0));
        m_grid->setMarginLeft(15.0f);
        m_grid->setMarginTop(0.0f);
        m_grid->setMarginBottom(10.0f);
        m_grid->setWidthPercentage(100.f);
        m_grid->setHeightPercentage(100.f);
        m_grid->setGrow(1.f);

        this->getContentBox()->addView(m_grid);

        m_grid->registerAction("退出游戏库", brls::BUTTON_B, [this](brls::View*) -> bool {
            beiklive::popActivity(this);
            return true;
        });



        m_grid->registerAction("分类", brls::BUTTON_Y, [this](brls::View*) -> bool {
            _showFilterDropdown();
            return true;
        });

        m_grid->registerAction("设置", brls::BUTTON_X, [this](brls::View*) -> bool {
            if (m_grid->isMultiSelectMode()) {
                _showMultiSelectSidebar();
                return true;
            }
            int idx = m_grid->getSelectedIndex();
            if (idx < 0 || static_cast<size_t>(idx) >= m_entries.size())
                return true;
            m_grid->setInteractionDisabled(true);
            _showGameOptionsPanel(m_entries[idx]);
            return true;
        });

        m_grid->registerAction("搜索", brls::BUTTON_RT, [this](brls::View*) -> bool {
            auto* ime = brls::Application::getPlatform()->getImeManager();
            if (!ime) return true;
            ime->openForText(
                [this](std::string text) {
                    m_isSearching = !text.empty();
                    m_searchTerm = text;
                    auto* alive = &m_alive;
                    ThreadPool::instance().enqueue([this, alive]() {
                        if (!alive->load()) return;
                        m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                        _filterEntries();
                        brls::sync([this, alive]() {
                            if (!alive->load()) return;
                            if (m_isSearching && m_entries.empty()) {
                                auto* dialog = new brls::Dialog("当前分类下无 \"" + m_searchTerm + "\"");
                                dialog->addButton("确认", []() {});
                                dialog->open();
                                return;
                            }
                            m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                            m_grid->setDefaultCellFocus(0);
                            m_dataSource = new GameLibraryDS(this);
                            m_grid->setDataSource(m_dataSource);
                            m_grid->reloadData();
                            _updateHeader();
                            brls::Application::giveFocus(m_grid);
                        });
                    });
                },
                "搜索游戏", "", 128, m_searchTerm,
                brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            return true;
        });

        m_grid->registerAction("排序", brls::BUTTON_LT, [this](brls::View*) -> bool {
            _showSortSelector();
            return true;
        });

        m_grid->onNextPage([this]() {
            if (!m_loadingMore) _loadNextPage();
        });

        m_grid->setFocusChangeCallback([this](int idx) {
            _currentFocusedIndex = idx;
        });

        _loadAndShowEntries();
    }

    GameLibraryPage::~GameLibraryPage()
    {
        m_alive.store(false);
    }

    void GameLibraryPage::willAppear(bool resetState)
    {
        brls::Box::willAppear(resetState);
        if (m_firstAppear) {
            m_firstAppear = false;
            return;
        }
        _reloadEntries();
    }

    size_t GameLibraryPage::GameLibraryDS::getItemCount()
    {
        return m_page ? static_cast<size_t>(m_page->m_visibleCount) : 0;
    }

    void GameLibraryPage::GameLibraryDS::populateItem(GridDrawItem& item, size_t index)
    {
        if (!m_page || index >= m_page->m_entries.size()) {
            beiklive::GridItem::populateEmpty(item, "空");
            return;
        }
        const auto& entry = m_page->m_entries[index];
        beiklive::GridItem::populateFromGameEntry(item, entry, GridItemMode::GAME_LIBRARY);
    }

    void GameLibraryPage::GameLibraryDS::onItemSelected(size_t index)
    {
        if (!m_page || index >= m_page->m_entries.size()) return;
        if (m_page->onGameSelected) {
            auto& cached = m_page->m_entries[index];
            auto fresh = beiklive::GameDB
                ? beiklive::GameDB->findByPath(cached.path)
                : std::optional<beiklive::GameEntry>{};
            m_page->onGameSelected(fresh.has_value() ? *fresh : cached);
        }
    }

    void GameLibraryPage::GameLibraryDS::clearData()
    {
    }

    void GameLibraryPage::_loadAndShowEntries()
    {
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _filterEntries();
            brls::sync([this, alive]() {
                if (!alive->load()) return;
                m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                m_grid->setDefaultCellFocus(0);
                m_dataSource = new GameLibraryDS(this);
                m_grid->setDataSource(m_dataSource);
                m_grid->reloadData();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
            });
        });
    }

    void GameLibraryPage::_filterEntries()
    {
        if (m_platformFilter == PlatformFilter::FAVORITE)
        {
            m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                [](const GameEntry& e) { return !e.favourite; }), m_entries.end());
        }
        else if (m_platformFilter != PlatformFilter::ALL)
        {
            int target = static_cast<int>(m_platformFilter);
            m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                [target](const GameEntry& e) { return e.platform != target; }), m_entries.end());
        }
        if (m_isSearching && !m_searchTerm.empty())
        {
            std::string lt = m_searchTerm;
            std::transform(lt.begin(), lt.end(), lt.begin(), [](unsigned char c) { return std::tolower(c); });
            m_entries.erase(std::remove_if(m_entries.begin(), m_entries.end(),
                [&lt](const GameEntry& e) {
                    std::string t = e.title;
                    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::tolower(c); });
                    return t.find(lt) == std::string::npos;
                }), m_entries.end());
        }

        switch (m_sortMode) {
        case SortMode::PLAY_TIME:
            std::sort(m_entries.begin(), m_entries.end(),
                [](const GameEntry& a, const GameEntry& b) { return a.playTime > b.playTime; });
            break;
        case SortMode::FIRST_LETTER:
            std::sort(m_entries.begin(), m_entries.end(),
                [](const GameEntry& a, const GameEntry& b) {
                    return _titleToSortKey(a.title) < _titleToSortKey(b.title);
                });
            break;
        default:
            std::sort(m_entries.begin(), m_entries.end(),
                [](const GameEntry& a, const GameEntry& b) { return a.lastPlayed > b.lastPlayed; });
            break;
        }
    }

    void GameLibraryPage::_loadNextPage()
    {
        brls::Logger::info("_loadNextPage: alive={} loading={} visible={} total={}",
            m_alive.load(), m_loadingMore, m_visibleCount, m_entries.size());
        if (!m_alive.load() || m_loadingMore) return;
        if (m_visibleCount >= static_cast<int>(m_entries.size())) return;

        m_loadingMore = true;
        m_visibleCount = std::min(m_visibleCount + PAGE_SIZE, static_cast<int>(m_entries.size()));
        m_grid->notifyDataChanged();
        m_loadingMore = false;
    }

    void GameLibraryPage::_showFilterDropdown()
    {
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            auto ae = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            bool hG = false, hC = false, hB = false, hN = false, hS = false, hD = false;
            int favCount = 0;
            for (auto& e : ae) {
                if (e.favourite) favCount++;
                switch (static_cast<beiklive::enums::EmuPlatform>(e.platform)) {
                    case beiklive::enums::EmuPlatform::EmuGBA: hG = true; break;
                    case beiklive::enums::EmuPlatform::EmuGBC: hC = true; break;
                    case beiklive::enums::EmuPlatform::EmuGB:  hB = true; break;
                    case beiklive::enums::EmuPlatform::EmuNES: hN = true; break;
                    case beiklive::enums::EmuPlatform::EmuSNES: hS = true; break;
                    case beiklive::enums::EmuPlatform::EmuNDS: hD = true; break;
                    default: break;
                }
            }
            brls::sync([this, alive, hG, hC, hB, hN, hS, hD, favCount]() {
                if (!alive->load()) return;
                std::vector<std::string> opts;
                std::vector<PlatformFilter> map;
                opts.push_back("所有"); map.push_back(PlatformFilter::ALL);
                if (favCount > 0) { opts.push_back("收藏 (" + std::to_string(favCount) + ")"); map.push_back(PlatformFilter::FAVORITE); }
                if (hG) { opts.push_back("GBA"); map.push_back(PlatformFilter::GBA); }
                if (hC) { opts.push_back("GBC"); map.push_back(PlatformFilter::GBC); }
                if (hB) { opts.push_back("GB");  map.push_back(PlatformFilter::GB);  }
                if (hN) { opts.push_back("FC"); map.push_back(PlatformFilter::NES); }
                if (hS) { opts.push_back("SFC"); map.push_back(PlatformFilter::SNES); }
                if (hD) { opts.push_back("NDS"); map.push_back(PlatformFilter::NDS); }
                int cur = 0;
                for (size_t i = 0; i < map.size(); i++)
                    if (map[i] == m_platformFilter) { cur = (int)i; break; }
                auto* dd = new brls::Dropdown("游戏分类", opts,
                    [this, map, alive](int sel) {
                        if (sel < 0 || sel >= (int)map.size()) return;
                        if (map[sel] == m_platformFilter) return;
                        m_platformFilter = map[sel];
                        ThreadPool::instance().enqueue([this, alive]() {
                            if (!alive->load()) return;
                            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                            _filterEntries();
                            if (m_platformFilter == PlatformFilter::FAVORITE && m_entries.empty()) {
                                m_platformFilter = PlatformFilter::ALL;
                                m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                                _filterEntries();
                                brls::Application::notify("收藏列表为空，已切换至所有游戏");
                            }
                            brls::sync([this, alive]() {
                                if (!alive->load()) return;
                                m_visibleCount = std::min(PAGE_SIZE, (int)m_entries.size());
                                m_grid->setDefaultCellFocus(0);
                                m_dataSource = new GameLibraryDS(this);
                                m_grid->setDataSource(m_dataSource);
                                m_grid->reloadData();
                                _updateHeader();
                                brls::Application::giveFocus(m_grid);
                            });
                        });
                    });
                brls::Application::pushActivity(new brls::Activity(dd));
            });
        });
    }

    void GameLibraryPage::_updateHeader()
    {
        if (m_isSearching)
            this->getHeader()->setInfo("搜索 \"" + m_searchTerm + "\" — " + std::to_string(m_entries.size()) + " 款");
        else
            this->getHeader()->setInfo("共 " + std::to_string(m_entries.size()) + " 款游戏");
        std::string fs;
        switch (m_platformFilter) {
            case PlatformFilter::ALL:      fs = "所有"; break;
            case PlatformFilter::GBA:      fs = "GBA";  break;
            case PlatformFilter::GBC:      fs = "GBC";  break;
            case PlatformFilter::GB:       fs = "GB";   break;
            case PlatformFilter::NES:      fs = "FC";  break;
            case PlatformFilter::SNES:     fs = "SFC"; break;
            case PlatformFilter::NDS:      fs = "NDS"; break;
            case PlatformFilter::FAVORITE: fs = "收藏"; break;
        }
        this->getHeader()->setPath((m_isSearching ? "搜索" : "分类") + (": " + fs));
    }

    std::string GameLibraryPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "";
        return beiklive::tools::formatPlayTime(seconds);
    }

    void GameLibraryPage::_showSortSelector()
    {
        std::vector<std::string> opts = {"最近游玩", "游玩时长", "首字母"};
        int cur = static_cast<int>(m_sortMode);
        auto* dd = new brls::Dropdown("排序方式", opts,
            [this](int sel) {
                if (sel < 0 || sel >= 3) return;
                auto newMode = static_cast<SortMode>(sel);
                if (newMode == m_sortMode) return;
                m_sortMode = newMode;
                _reloadEntries();
            },
            cur);
        brls::Application::pushActivity(new brls::Activity(dd));
    }

    std::string GameLibraryPage::_titleToSortKey(const std::string& title)
    {
        static nlohmann::json pinyinMap;
        static bool loaded = false;
        if (!loaded) {
            std::ifstream f(BK_RES("pinyin/pingyin.json"));
            if (f.is_open()) {
                f >> pinyinMap;
                loaded = true;
            }
        }

        std::string key;
        for (size_t i = 0; i < title.size(); i++) {
            std::string ch(1, title[i]);
            unsigned char c = static_cast<unsigned char>(title[i]);
            if (c >= 0x80 && i + 2 < title.size()) {
                ch = title.substr(i, 3);
                i += 2;
            }
            if (pinyinMap.contains(ch)) {
                key += pinyinMap[ch].get<std::string>();
            } else if (c >= 0x80) {
                key += "\xFF"; // unknown CJK, sort after everything
            } else if (std::isdigit(c)) {
                key += std::string(1, '\x00') + ch; // digits first
            } else if (std::isalpha(c)) {
                key += std::string(1, '\x01') + std::string(1, static_cast<char>(std::tolower(c)));
            } else {
                key += std::string(1, '\x02') + ch;
            }
        }
        return key;
    }

    void GameLibraryPage::_reloadEntries()
    {
        auto* alive = &m_alive;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            _filterEntries();
            if (m_platformFilter == PlatformFilter::FAVORITE && m_entries.empty()) {
                m_platformFilter = PlatformFilter::ALL;
                m_entries = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
                _filterEntries();
                brls::Application::notify("收藏列表为空，已切换至所有游戏");
            }
            brls::sync([this, alive]() {
                if (!alive->load()) return;
                m_visibleCount = std::min(PAGE_SIZE, static_cast<int>(m_entries.size()));
                m_grid->setDefaultCellFocus(0);
                m_dataSource = new GameLibraryDS(this);
                m_grid->setDataSource(m_dataSource);
                m_grid->reloadData();
                _updateHeader();
                brls::Application::giveFocus(m_grid);
            });
        });
    }

    void GameLibraryPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        std::string path = entry.path;
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);
        std::string fn = beiklive::tools::getFileNameWithoutExtension(entry.path);
        auto enterMultiSelect = [this](bool selectAll) {
            _hideGameOptionsPanel();
            m_grid->setMultiSelectMode(true);
            if (selectAll)
            {
                m_grid->selectAllForDelete(m_entries.size());
                brls::Application::notify("已全选当前列表中的全部游戏");
            }
            m_grid->setInteractionDisabled(false);
            brls::Application::giveFocus(m_grid);
        };

        m_gameOptionsSidebar->addButton("修改映射名称", BK_RES("img/ui/setting/emu.png"),
            [this, path, title = entry.title, fn, idx = m_grid->getSelectedIndex()](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                auto* ime = brls::Application::getPlatform()->getImeManager();
                if (!ime) { m_grid->setInteractionDisabled(false); return; }
                ime->openForText(
                    [this, path, fn, idx](std::string text) {
                        if (!text.empty() && beiklive::GameDB) {
                            beiklive::GameDB->set(path, "title", nlohmann::json(text));
                            beiklive::GameDB->flush();
                            beiklive::NameMappingManager->Set(fn, text, true);
                            beiklive::NameMappingManager->Save();
                            m_grid->setItemTitle(idx, text);
                        }
                        m_grid->setInteractionDisabled(false);
                    },
                    "编辑游戏名称", "", 128, title,
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            });

        m_gameOptionsSidebar->addButton("设置封面图", BK_RES("img/ui/setting/display.png"),
            [this, path, idx = m_grid->getSelectedIndex()](const beiklive::GameEntry& entry) {
                _hideGameOptionsPanel();
                fs::path currentLogo(entry.logoPath);
                beiklive::openFilePicker({"png", "jpg"},
                    [this, path, idx](const std::string& selectedPath) {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(path, "logoPath", nlohmann::json(selectedPath));
                            beiklive::GameDB->flush();
                            m_grid->setItemImagePath(idx, selectedPath);
                        }
                        m_grid->setInteractionDisabled(false);
                    },
                    currentLogo.parent_path().empty() ? beiklive::path::GetRootPath() : currentLogo.parent_path().string(),
                    currentLogo.filename().string());
            });

        m_gameOptionsSidebar->addButton("安装游戏前端", BK_RES("img/ui/setting/emu.png"),
            [this](const beiklive::GameEntry& game) {
                _hideGameOptionsPanel();
                m_grid->setInteractionDisabled(false);
                beiklive::forwarder::showInstallDialog(game);
            });

        if (beiklive::GetCoreOptions(entry.platform).size() > 1)
        {
            m_gameOptionsSidebar->addButton("核心选择", BK_RES("img/ui/setting/emu.png"),
                [this, path, platform = entry.platform, core = entry.core,
                 idx = m_grid->getSelectedIndex()](const beiklive::GameEntry&) {
                    _hideGameOptionsPanel();

                    const auto options = beiklive::GetCoreOptions(platform);
                    std::vector<std::string> names;
                    names.reserve(options.size());
                    for (const auto& option : options)
                        names.push_back(option.name);

                    auto* dropdown = new brls::Dropdown(
                        "核心选择",
                        names,
                        [this, path, idx, options](int selected) {
                            if (selected < 0 || selected >= static_cast<int>(options.size()))
                                return;
                            if (beiklive::GameDB) {
                                beiklive::GameDB->set(path, "core", nlohmann::json(options[selected].id));
                                beiklive::GameDB->flush();
                                if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size())
                                    m_entries[idx].core = options[selected].id;
                                brls::Application::notify("已切换核心：" + options[selected].name);
                            }
                            m_grid->setInteractionDisabled(false);
                        },
                        beiklive::GetCoreSelectionIndex(platform, core),
                        [this](int) {
                            m_grid->setInteractionDisabled(false);
                        });
                    brls::Application::pushActivity(new brls::Activity(dropdown));
                });
        }

        m_gameOptionsSidebar->addButton("游戏数据浏览", BK_RES("img/ui/menu/save.png"),
            [this, entry](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                _openGameDataPage(entry);
                m_grid->setInteractionDisabled(false);
            });

        m_gameOptionsSidebar->addButton("删除游戏", BK_RES("img/ui/menu/exit.png"),
            [this, path](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                auto* removeDlg = new brls::Dialog("是否从游戏库移除该游戏？");
                removeDlg->addButton("是", [this, path]() {
                    auto* romDlg = new brls::Dialog("是否删除 ROM 文件？");
                    auto deleteGame = [this, path](bool deleteRomFile) {
                        if (beiklive::GameDB) {
                            bool removedRecord = false;
                            if ((int)beiklive::GameDB->getAll().size() <= 1)
                            {
                                beiklive::GameDB->clearAll();
                                removedRecord = true;
                            }
                            else {
                                removedRecord = beiklive::GameDB->removeByPath(path);
                                if (removedRecord)
                                    beiklive::GameDB->flush();
                            }

                            bool removedFile = true;
                            if (removedRecord && deleteRomFile)
                                removedFile = deleteGameFileIfExists(path);

                            if (!removedRecord)
                                brls::Application::notify("删除失败");
                            else if (deleteRomFile)
                                brls::Application::notify(removedFile ? "已删除游戏" : "已移除记录，ROM 文件删除失败");
                            else
                                brls::Application::notify("已从游戏库移除该游戏");
                            _reloadEntries();
                        } else {
                            brls::Application::notify("删除失败");
                        }
                        m_grid->setInteractionDisabled(false);
                    };
                    romDlg->addButton("是", [deleteGame]() { deleteGame(true); });
                    romDlg->addButton("否", [deleteGame]() { deleteGame(false); });
                    romDlg->addButton("不删了", [this]() { m_grid->setInteractionDisabled(false); });
                    romDlg->open();
                });
                removeDlg->addButton("否", [this]() { m_grid->setInteractionDisabled(false); });
                removeDlg->addButton("不删了", [this]() { m_grid->setInteractionDisabled(false); });
                removeDlg->open();
            });

        m_gameOptionsSidebar->addButton(
            entry.favourite ? "取消收藏" : "加入收藏",
            BK_RES("img/ui/setting/emu.png"),
            [this, path, fav = entry.favourite, idx = m_grid->getSelectedIndex()](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                std::string msg = fav ? "确定要取消收藏吗？" : "确定要加入收藏吗？";
                auto* dlg = new brls::Dialog(msg);
                dlg->addButton("确认", [this, path, fav, idx]() {
                    if (beiklive::GameDB) {
                        beiklive::GameDB->set(path, "favourite", nlohmann::json(!fav));
                        beiklive::GameDB->flush();
                        m_grid->setItemFavourite(idx, !fav);
                    }
                    m_grid->setInteractionDisabled(false);
                });
                dlg->addButton("取消", [this]() { m_grid->setInteractionDisabled(false); });
                dlg->open();
            });

        m_gameOptionsSidebar->addButton(
            "多选",
            BK_RES("img/ui/setting/emu.png"),
            [enterMultiSelect](const beiklive::GameEntry&) {
                enterMultiSelect(false);
            });

        m_gameOptionsSidebar->addButton(
            "全选",
            BK_RES("img/ui/setting/emu.png"),
            [enterMultiSelect](const beiklive::GameEntry&) {
                enterMultiSelect(true);
            });

        m_gameOptionsSidebar->onClosed = [this]() {
            m_grid->setInteractionDisabled(false);
            brls::Application::giveFocus(m_grid);
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        };
        this->addView(m_gameOptionsSidebar);
        m_grid->setInteractionDisabled(true);
        m_gameOptionsSidebar->open(entry);
    }

    void GameLibraryPage::_hideGameOptionsPanel()
    {
        if (m_gameOptionsSidebar) {
            m_gameOptionsSidebar->close();
            m_gameOptionsSidebar->removeFromSuperView(true);
            m_gameOptionsSidebar = nullptr;
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        }
    }

    void GameLibraryPage::_showMultiSelectSidebar()
    {
        m_grid->setInteractionDisabled(true);
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        this->getBottomBar()->setVisibility(brls::Visibility::INVISIBLE);

        size_t count = m_grid->getDeleteSelection().size();

        m_gameOptionsSidebar->addButton(
            "取消多选",
            BK_RES("img/ui/setting/emu.png"),
            [this](const beiklive::GameEntry&) {
                _hideGameOptionsPanel();
                m_grid->clearDeleteSelection();
                m_grid->setInteractionDisabled(false);
                brls::Application::notify("已取消多选模式");
            });

        if (count > 0) {
            m_gameOptionsSidebar->addButton(
                "添加到收藏 (" + std::to_string(count) + ")",
                BK_RES("img/ui/setting/emu.png"),
                [this](const beiklive::GameEntry&) {
                    _hideGameOptionsPanel();
                    std::vector<int> sel(m_grid->getDeleteSelection().begin(),
                                         m_grid->getDeleteSelection().end());
                    auto* dlg = new brls::Dialog("确认将选中的 " +
                        std::to_string(sel.size()) + " 款游戏添加到收藏？");
                    dlg->addButton("取消", [this]() {
                        m_grid->setInteractionDisabled(false);
                    });
                    dlg->addButton("确认", [this, sel]() {
                        size_t updated = 0;
                        if (beiklive::GameDB) {
                            for (int idx : sel) {
                                if (idx < 0 || static_cast<size_t>(idx) >= m_entries.size())
                                    continue;
                                beiklive::GameDB->set(m_entries[idx].path, "favourite", nlohmann::json(true));
                                m_entries[idx].favourite = true;
                                m_grid->setItemFavourite(idx, true);
                                ++updated;
                            }
                            beiklive::GameDB->flush();
                        }
                        m_grid->clearDeleteSelection();
                        m_grid->setInteractionDisabled(false);
                        brls::Application::notify(updated > 0
                            ? "已添加到收藏：" + std::to_string(updated) + " 款"
                            : "未选择可收藏的游戏");
                        if (m_platformFilter == PlatformFilter::FAVORITE)
                            _reloadEntries();
                    });
                    dlg->open();
                });

            m_gameOptionsSidebar->addButton(
                "删除已选游戏 (" + std::to_string(count) + ")",
                BK_RES("img/ui/menu/exit.png"),
                [this](const beiklive::GameEntry&) {
                    _hideGameOptionsPanel();
                    std::vector<int> sel(m_grid->getDeleteSelection().begin(),
                                         m_grid->getDeleteSelection().end());
                    size_t n = sel.size();
                    std::string msg = "是否从游戏库移除这 " + std::to_string(n) + " 款游戏？";
                    auto* removeDlg = new brls::Dialog(msg);
                    removeDlg->addButton("是", [this, sel]() {
                        auto* romDlg = new brls::Dialog("是否删除 ROM 文件？");
                        auto deleteSelected = [this, sel](bool deleteRomFiles) {
                            bool allFilesRemoved = true;
                            bool removedAnyRecord = false;
                            if (beiklive::GameDB) {
                                if ((int)beiklive::GameDB->getAll().size() <= (int)sel.size())
                                {
                                    if (deleteRomFiles) {
                                        for (int idx : sel) {
                                            if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size())
                                                allFilesRemoved = deleteGameFileIfExists(m_entries[idx].path) && allFilesRemoved;
                                        }
                                    }
                                    beiklive::GameDB->clearAll();
                                    removedAnyRecord = true;
                                }
                                else {
                                    for (int idx : sel) {
                                        if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size()) {
                                            const auto& path = m_entries[idx].path;
                                            if (beiklive::GameDB->removeByPath(path)) {
                                                removedAnyRecord = true;
                                                if (deleteRomFiles)
                                                    allFilesRemoved = deleteGameFileIfExists(path) && allFilesRemoved;
                                            }
                                        }
                                    }
                                    beiklive::GameDB->flush();
                                }
                            }
                            if (!removedAnyRecord)
                                brls::Application::notify("删除失败");
                            else if (deleteRomFiles && !allFilesRemoved)
                                brls::Application::notify("已移除记录，部分 ROM 文件删除失败");
                            else
                                brls::Application::notify(deleteRomFiles ? "已删除所选游戏" : "已从游戏库移除所选游戏");
                            m_grid->clearDeleteSelection();
                            _reloadEntries();
                            m_grid->setInteractionDisabled(false);
                        };
                        romDlg->addButton("是", [deleteSelected]() { deleteSelected(true); });
                        romDlg->addButton("否", [deleteSelected]() { deleteSelected(false); });
                        romDlg->addButton("不删了", [this]() { m_grid->setInteractionDisabled(false); });
                        romDlg->open();
                    });
                    removeDlg->addButton("否", [this]() {
                        m_grid->setInteractionDisabled(false);
                    });
                    removeDlg->addButton("不删了", [this]() {
                        m_grid->setInteractionDisabled(false);
                    });
                    removeDlg->open();
                });
        }

        m_gameOptionsSidebar->onClosed = [this]() {
            m_grid->setInteractionDisabled(false);
            brls::Application::giveFocus(m_grid);
            this->getBottomBar()->setVisibility(brls::Visibility::VISIBLE);
        };
        this->addView(m_gameOptionsSidebar);
        m_gameOptionsSidebar->open(beiklive::GameEntry{});
    }

    void GameLibraryPage::_openGameDataPage(const beiklive::GameEntry& entry)
    {
        auto* page = new GameDataPage(entry);
        auto* frame = new brls::AppletFrame(page);
        HIDE_BRLS_BAR(frame);
        beiklive::pushActivity(frame, this, page);
    }

} // namespace beiklive
