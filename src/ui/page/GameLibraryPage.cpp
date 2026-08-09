#include "GameLibraryPage.hpp"
#include "CoverEditorPage.hpp"
#include "core/Translation.hpp"
#include "GameDataPage.hpp"
#include "SteamGridDbPage.hpp"
#include "core/SteamGridDb.hpp"
#include "core/ThreeDsTitlePaths.hpp"
#include "core/forwarder/ForwarderInstaller.hpp"
#include "ui/utils/MaterialIcons.hpp"
#include "ui/utils/NdsEnvironment.hpp"
#include "ui/widget/ButtonBox.hpp"
#include "ui/widget/GridBox.hpp"
#include "ui/widget/GridItem.hpp"
#include "ui/widget/TabFrame.hpp"
#include "ui/utils/FilePickerHelper.hpp"
#include "ui/view/ImageView.hpp"
#include "core/ThreadPool.hpp"
#include "ui/utils/CheatMatcher.hpp"
#include <borealis/core/logger.hpp>
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
    if (path.empty()) {
        brls::Logger::info("[Game Delete] entry path empty, nothing to remove");
        return true;
    }

    std::error_code ec;
    const bool exists = std::filesystem::exists(path, ec);
    if (ec) {
        brls::Logger::warning(
            "[Game Delete] entry exists failed: path={} code={} error={}",
            path, ec.value(), ec.message());
        return false;
    }
    if (!exists) {
        brls::Logger::info("[Game Delete] entry already missing: {}", path);
        return true;
    }

    ec.clear();
    const bool removed = std::filesystem::remove(path, ec);
    if (ec || !removed) {
        brls::Logger::warning(
            "[Game Delete] entry remove failed: path={} removed={} code={} error={}",
            path, removed, ec.value(), ec ? ec.message() : "remove returned false");
        return false;
    }
    brls::Logger::info("[Game Delete] entry removed: {}", path);
    return true;
}

bool deleteGameFilesForEntry(const beiklive::GameEntry& entry)
{
    brls::Logger::info(
        "[Game Delete] file removal begin: platform={} path={} stored_title_id={}",
        entry.platform, entry.path, entry.threeDsTitleId);
    if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS))
    {
        const std::string titleId = beiklive::three_ds::resolveTitleId(
            entry.threeDsTitleId, entry.path);
        if (titleId.empty())
            brls::Logger::warning(
                "[Game Delete] unable to resolve 3DS title id: path={}", entry.path);
        const bool titleFilesRemoved = titleId.empty() ||
            beiklive::three_ds::deleteInstalledContentAndShaderCache(titleId);
        const bool entryFileRemoved = deleteGameFileIfExists(entry.path);
        brls::Logger::info(
            "[Game Delete] 3DS file removal result: path={} title_id={} "
            "title_files_removed={} entry_file_removed={} success={}",
            entry.path, titleId, titleFilesRemoved, entryFileRemoved,
            titleFilesRemoved && entryFileRemoved);
        return entryFileRemoved && titleFilesRemoved;
    }
    const bool removed = deleteGameFileIfExists(entry.path);
    brls::Logger::info(
        "[Game Delete] file removal result: path={} success={}", entry.path, removed);
    return removed;
}

std::vector<beiklive::GameEntry> loadLibraryEntries()
{
    auto entries = beiklive::GameDB
        ? beiklive::GameDB->getAll()
        : std::vector<beiklive::GameEntry>{};

    auto mostRecentlyPlayed = std::max_element(
        entries.begin(), entries.end(),
        [](const beiklive::GameEntry& lhs, const beiklive::GameEntry& rhs) {
            return lhs.lastPlayed < rhs.lastPlayed;
        });
    if (mostRecentlyPlayed != entries.end() && !mostRecentlyPlayed->lastPlayed.empty() &&
        beiklive::tools::tryUseSavestateThumbnailCover(*mostRecentlyPlayed) &&
        beiklive::GameDB) {
        beiklive::GameDB->set(mostRecentlyPlayed->path, "logoPath",
                              nlohmann::json(mostRecentlyPlayed->logoPath));
    }
    return entries;
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

        class LegacyGameDataPage : public beiklive::Box
        {
        public:
            explicit LegacyGameDataPage(beiklive::GameEntry entry)
                : m_entry(std::move(entry))
            {
                showHeader(false);
                showFooter(false);
                // getHeader()->setTitle(m_entry.title.empty() ? gameStem(m_entry) : m_entry.title);
                // getHeader()->setPath(m_entry.path);
                setFocusable(false);
                getContentBox()->setMargins(0.f, 0.f, 0.f, 0.f);
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
                registerAction(L("返回"), brls::BUTTON_B, [this](brls::View*) -> bool {
                    beiklive::popActivity(this);
                    return true;
                });

                m_tabs = new beiklive::TabFrame();
                m_tabs->setAnimationEnabled(false);

                auto* states = _createStatePanel();
                auto* shots = _createScreenshotPanel();
                auto* battery = _createBatteryPanel();

                m_tabs->addTab(L("即时存档管理"), BK_RES("img/ui/menu/save.png"), nullptr,
                    [this]() {
                        _refreshStateList();
                    }, nullptr, states,
                    m_stateGrid ? m_stateGrid->getItemView(0) : states);
                m_tabs->addTab(L("游戏图片管理"), BK_RES("img/ui/setting/display.png"), nullptr,
                    [this]() {
                        _refreshScreenshotList();
                    }, nullptr, shots,
                    m_screenshotGrid ? m_screenshotGrid->getItemView(0) : shots);
                m_tabs->addTab(L("电池存档管理"), BK_RES("img/ui/menu/save.png"), nullptr,
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

                m_exportSavBtn = _makeButton(L("导出存档"), BK_RES("img/ui/menu/save.png"));
                m_exportSavBtn->registerClickAction([this](brls::View*) -> bool {
                    _exportSav();
                    return true;
                });
                m_batteryActionsBox->addView(m_exportSavBtn);

                auto* importBtn = _makeButton(L("导入存档"), BK_RES("img/ui/light/wenjian.png"));
                importBtn->registerClickAction([this](brls::View*) -> bool {
                    _importSav();
                    return true;
                });
                m_batteryActionsBox->addView(importBtn);

                auto* backupBtn = _makeButton(L("存档备份"), BK_RES("img/ui/menu/save.png"));
                backupBtn->registerClickAction([this](brls::View*) -> bool {
                    _backupSav();
                    return true;
                });
                m_batteryActionsBox->addView(backupBtn);

                auto* header = new brls::Header();
                header->setTitle(L("备份列表"));
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
                auto* dlg = new brls::Dialog(L("确认删除") + beiklive::tools::slotName(slot) + "？");
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("删除"), [this, slot]() {
                    std::error_code removeEc;
                    fs::remove(_statePath(slot), removeEc);
                    fs::remove(_stateThumbPath(slot), removeEc);
                    brls::Application::notify(removeEc ? L("删除失败") : L("已删除存档"));
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
                page->registerAction(L("关闭"), brls::BUTTON_B, [page](brls::View*) -> bool {
                    beiklive::popActivity(page);
                    return true;
                });
                page->registerAction(L("关闭"), brls::BUTTON_A, [page](brls::View*) -> bool {
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
                auto* dlg = new brls::Dialog(L("确认删除截图\n") + path.filename().string() + "？");
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("删除"), [this, path]() {
                    std::error_code ec;
                    fs::remove(path, ec);
                    brls::Application::notify(ec ? L("删除失败") : L("已删除截图"));
                    _refreshScreenshotList(true);
                });
                dlg->open();
            }

            void _confirmSetScreenshotAsCover(int index)
            {
                if (index < 0 || index >= static_cast<int>(m_screenshotPaths.size()))
                    return;
                const std::string cover = m_screenshotPaths[index].string();
                auto* dlg = new brls::Dialog(L("确认将该截图设置为封面？\n") +
                                             m_screenshotPaths[index].filename().string());
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("确认"), [this, cover]() {
                    if (!beiklive::GameDB)
                        return;
                    beiklive::GameDB->set(m_entry.path, "logoPath", nlohmann::json(cover));
                    beiklive::GameDB->flush();
                    m_entry.logoPath = cover;
                    brls::Application::notify(L("已设置为封面图片"));
                });
                dlg->open();
            }

            void _exportSav()
            {
                const std::string src = _savPath();
                std::error_code ec;
                if (!fs::exists(src, ec)) {
                    brls::Application::notify(L("未找到电池存档"));
                    return;
                }
                auto* dlg = new brls::Dialog(L("确认导出当前电池存档？"));
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("导出"), [src]() {
                    fs::path exportDir("sdmc:/GBAStation/export");
                    std::string error;
                    if (!copyBinaryFile(src, exportDir / fs::path(src).filename(), &error)) {
                        brls::Logger::warning("导出电池存档失败: {} -> {}, error={}",
                            src, (exportDir / fs::path(src).filename()).string(), error);
                        brls::Application::notify(L("导出失败"));
                        return;
                    }
                    brls::Application::notify(L("已导出存档"));
                });
                dlg->open();
            }

            void _importSav()
            {
                auto* dlg = new brls::Dialog(L("确认导入外部 .sav 并覆盖当前电池存档？"));
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("选择文件"), [this]() {
                    beiklive::openFilePicker({"sav"}, [this](const std::string& selected) {
                        std::string error;
                        if (!copyBinaryFile(selected, _savPath(), &error)) {
                            brls::Logger::warning("导入电池存档失败: {} -> {}, error={}",
                                selected, _savPath(), error);
                            brls::Application::notify(L("导入失败"));
                            return;
                        }
                        brls::Application::notify(L("已导入存档"));
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
                    brls::Application::notify(L("未找到电池存档"));
                    return;
                }
                auto* dlg = new brls::Dialog(L("确认为当前电池存档创建备份？"));
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("备份"), [this, src]() {
                    fs::path backup = fs::path(src).string() + ".bak_" + timestampForFile();
                    std::string error;
                    if (!copyBinaryFile(src, backup, &error)) {
                        std::error_code removeEc;
                        fs::remove(backup, removeEc);
                        brls::Logger::warning("备份电池存档失败: {} -> {}, error={}",
                            src, backup.string(), error);
                        brls::Application::notify(L("备份失败"));
                        return;
                    }
                    brls::Application::notify(L("已创建备份"));
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
                    label->registerAction(L("还原"), brls::BUTTON_A, [this, index = static_cast<int>(i)](brls::View*) -> bool {
                        _confirmRestoreBackup(index);
                        return true;
                    });
                    label->registerAction(L("删除"), brls::BUTTON_X, [this, index = static_cast<int>(i)](brls::View*) -> bool {
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
                auto* dlg = new brls::Dialog(L("确认还原备份\n") + backup.filename().string() + "？");
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("还原"), [this, backup]() {
                    std::string error;
                    if (!copyBinaryFile(backup, _savPath(), &error)) {
                        brls::Logger::warning("还原电池存档失败: {} -> {}, error={}",
                            backup.string(), _savPath(), error);
                        brls::Application::notify(L("还原失败"));
                        return;
                    }
                    brls::Application::notify(L("已还原存档"));
                });
                dlg->open();
            }

            void _confirmDeleteBackup(int index)
            {
                if (index < 0 || index >= static_cast<int>(m_backupPaths.size()))
                    return;
                const fs::path backup = m_backupPaths[index];
                auto* dlg = new brls::Dialog(L("确认删除备份\n") + backup.filename().string() + "？");
                dlg->addButton(L("取消"), []() {});
                dlg->addButton(L("删除"), [this, backup]() {
                    std::error_code ec;
                    fs::remove(backup, ec);
                    brls::Application::notify(ec ? L("删除失败") : L("已删除备份"));
                    _refreshBackupList(true);
                });
                dlg->open();
            }
        };
    }

    GameLibraryPage::GameLibraryPage()
        : GameLibraryPage(PreparedData{})
    {
    }

    GameLibraryPage::GameLibraryPage(PreparedData preparedData)
    {
        this->showHeader(false);
        this->showFooter(false);
        this->getHeader()->setTitle(L("游戏库"));
        this->setFocusable(false);
        this->getContentBox()->setMargins(0.f, 0.f, 0.f, 0.f);

        m_libraryView = new GameGridView();
        m_libraryView->setTitleFontSize(GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_LIBRARY_TITLE_SIZE, 0));
        m_libraryView->setViewMode(
            GET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_LIBRARY_VIEW_MODE, 0) == 1
                ? GameGridView::ViewMode::LIST
                : GameGridView::ViewMode::GRID);
        m_libraryView->setMarginLeft(0.0f);
        m_libraryView->setMarginTop(0.0f);
        m_libraryView->setMarginBottom(10.0f);
        m_libraryView->setWidthPercentage(100.f);
        m_libraryView->setHeightPercentage(100.f);
        m_libraryView->setGrow(1.f);

        this->getContentBox()->addView(m_libraryView);
        m_searchOverlay = new beiklive::NanoSearchOverlay();
        m_searchOverlay->onClosed = [this]() {
            if (!m_isClosing && m_libraryView) {
                if (!m_searchOverlaySubmitted)
                    m_libraryView->setInteractionDisabled(false);
                m_searchOverlaySubmitted = false;
                brls::Application::giveFocus(m_libraryView);
            }
        };
        this->getContentBox()->addView(m_searchOverlay);

        m_libraryView->registerAction(L("退出游戏库"), brls::BUTTON_B, [this](brls::View*) -> bool {
            if (m_libraryView->isDeleteAnimationRunning())
                return true;
            if (m_libraryView->isMultiSelectMode()) {
                m_libraryView->clearDeleteSelection();
                brls::Application::notify(L("已退出多选模式"));
                return true;
            }
            if (m_isClosing)
                return true;
            m_isClosing = true;
            ++m_reloadGeneration;
            if (m_hasPlatformReloadDelay) {
                brls::cancelDelay(m_platformReloadDelayId);
                m_hasPlatformReloadDelay = false;
            }
            auto alive = m_aliveToken;
            m_libraryView->playExitAnimation([this, alive]() {
                if (alive->load())
                    beiklive::popActivity(this, false);
            });
            return true;
        });



        m_libraryView->registerAction(L("切换视图"), brls::BUTTON_Y, [this](brls::View*) -> bool {
            if (m_isClosing || m_libraryView->isDeleteAnimationRunning()) return true;
            m_libraryView->toggleViewMode();
            SET_SETTING_KEY_INT(beiklive::SettingKey::KEY_UI_LIBRARY_VIEW_MODE,
                m_libraryView->getViewMode() == GameGridView::ViewMode::LIST ? 1 : 0);
            _updateHeader();
            return true;
        });

        m_libraryView->registerAction(L("上一平台"), brls::BUTTON_LB, [this](brls::View*) -> bool {
            if (m_libraryView->isDeleteAnimationRunning()) return true;
            _cyclePlatformFilter(-1);
            return true;
        });

        m_libraryView->registerAction(L("下一平台"), brls::BUTTON_RB, [this](brls::View*) -> bool {
            if (m_libraryView->isDeleteAnimationRunning()) return true;
            _cyclePlatformFilter(1);
            return true;
        });

        m_libraryView->registerAction(L("分类"), brls::BUTTON_LSB, [this](brls::View*) -> bool {
            if (m_isClosing || m_libraryView->isDeleteAnimationRunning()) return true;
            _showFilterDropdown();
            return true;
        });

        m_libraryView->registerAction(L("多选"), brls::BUTTON_X, [this](brls::View*) -> bool {
            if (m_isClosing || m_libraryView->isDeleteAnimationRunning()) return true;
            if (m_libraryView->isMultiSelectMode()) {
                if (m_libraryView->getDeleteSelection().empty())
                    brls::Application::notify(L("请先勾选至少一款游戏"));
                else
                    _showMultiSelectSidebar();
                return true;
            }
            m_libraryView->setMultiSelectMode(true);
            brls::Application::notify(L("已进入多选模式"));
            return true;
        });

        m_libraryView->registerAction(L("全选"), brls::BUTTON_START, [this](brls::View*) -> bool {
            if (m_isClosing || m_libraryView->isDeleteAnimationRunning()) return true;
            if (!m_libraryView->isMultiSelectMode())
                return false;
            if (m_libraryView->isAllSelectedForDelete(m_entries.size()))
                m_libraryView->deselectAllForDelete();
            else
                m_libraryView->selectAllForDelete(m_entries.size());
            return true;
        });

        m_libraryView->registerAction(L("搜索"), brls::BUTTON_RT, [this](brls::View*) -> bool {
            if (m_isClosing || m_libraryView->isDeleteAnimationRunning()) return true;
            if (!m_searchOverlay || m_searchOverlay->isOpen()) return true;
            m_libraryView->setInteractionDisabled(true);
            m_searchOverlaySubmitted = false;
            m_searchOverlay->open(m_searchTerm, [this](const std::string& text) {
                m_searchOverlaySubmitted = true;
                m_isSearching = !text.empty();
                m_searchTerm = text;
                _reloadEntries();
            });
            return true;
        });

        m_libraryView->registerAction(L("排序"), brls::BUTTON_LT, [this](brls::View*) -> bool {
            if (m_isClosing || m_libraryView->isDeleteAnimationRunning()) return true;
            _showSortSelector();
            return true;
        });

        m_libraryView->setFocusChangeCallback([this](int idx) {
            _currentFocusedIndex = idx;
            if (idx >= 0)
                m_filterFocusIndices[static_cast<int>(m_platformFilter)] = idx;
        });

        if (preparedData.ready) {
            m_deferredPreparedData = std::move(preparedData);
            m_libraryView->setInteractionDisabled(true);
            m_libraryView->showLoadingSkeleton();
            auto alive = m_aliveToken;
            brls::delay(16, [this, alive]() {
                if (!alive->load() || !m_deferredPreparedData.ready)
                    return;
                auto prepared = std::move(m_deferredPreparedData);
                m_deferredPreparedData = {};
                const uint64_t generation = ++m_reloadGeneration;
                _presentReloadedEntries(
                    generation, std::move(prepared.entries),
                    std::move(prepared.filters), PlatformFilter::ALL,
                    false, false, "");
            });
        } else {
            _loadAndShowEntries();
        }
    }

    GameLibraryPage::PreparedData GameLibraryPage::prepareInitialData()
    {
        PreparedData prepared;
        prepared.entries = loadLibraryEntries();
        prepared.filters = _buildAvailableFilters(prepared.entries);
        _filterAndSortEntries(
            prepared.entries, PlatformFilter::ALL, SortMode::LAST_PLAYED,
            false, "");
        prepared.ready = true;
        return prepared;
    }

    GameLibraryPage::~GameLibraryPage()
    {
        m_aliveToken->store(false);
        ++m_reloadGeneration;
        if (m_hasPlatformReloadDelay)
            brls::cancelDelay(m_platformReloadDelayId);
    }

    void GameLibraryPage::willAppear(bool resetState)
    {
        brls::Box::willAppear(resetState);
        if (m_firstAppear) {
            m_firstAppear = false;
            return;
        }
        if (m_libraryView)
            m_libraryView->resetLaunchAnimation();
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
        auto& cached = m_page->m_entries[index];
        auto fresh = beiklive::GameDB
            ? beiklive::GameDB->findByPath(cached.path)
            : std::optional<beiklive::GameEntry>{};
        const auto& entry = fresh.has_value() ? *fresh : cached;

        if (m_page->m_libraryView) {
            m_page->m_libraryView->setInteractionDisabled(true);
            m_page->_showGameOptionsPanel(entry);
        }
    }

    void GameLibraryPage::GameLibraryDS::clearData()
    {
    }

    void GameLibraryPage::_loadAndShowEntries()
    {
        if (m_libraryView) {
            m_libraryView->setInteractionDisabled(true);
            m_libraryView->showLoadingSkeleton();
        }
        _reloadEntries();
    }

    void GameLibraryPage::resetLaunchOverlay()
    {
        if (m_libraryView)
            m_libraryView->resetLaunchAnimation();
    }

    std::vector<GameLibraryPage::PlatformFilter> GameLibraryPage::_buildAvailableFilters(
        const std::vector<beiklive::GameEntry>& entries)
    {
        bool hasFavourite = false;
        bool hasGba = false;
        bool hasGbc = false;
        bool hasGb = false;
        bool hasNes = false;
        bool hasSnes = false;
        bool hasNds = false;
        bool hasThreeDs = false;
        bool hasGenesis = false;
        bool hasArcade = false;
        bool hasDreamcast = false;
        bool hasPsp = false;
        for (const auto& entry : entries) {
            hasFavourite = hasFavourite || entry.favourite;
            switch (static_cast<beiklive::enums::EmuPlatform>(entry.platform)) {
                case beiklive::enums::EmuPlatform::EmuGBA:  hasGba = true; break;
                case beiklive::enums::EmuPlatform::EmuGBC:  hasGbc = true; break;
                case beiklive::enums::EmuPlatform::EmuGB:   hasGb = true; break;
                case beiklive::enums::EmuPlatform::EmuNES:  hasNes = true; break;
                case beiklive::enums::EmuPlatform::EmuSNES: hasSnes = true; break;
                case beiklive::enums::EmuPlatform::EmuNDS:  hasNds = true; break;
                case beiklive::enums::EmuPlatform::Emu3DS:  hasThreeDs = true; break;
                case beiklive::enums::EmuPlatform::EmuGenesis: hasGenesis = true; break;
                case beiklive::enums::EmuPlatform::EmuArcade: hasArcade = true; break;
                case beiklive::enums::EmuPlatform::EmuDreamcast: hasDreamcast = true; break;
                case beiklive::enums::EmuPlatform::EmuPSP: hasPsp = true; break;
                default: break;
            }
        }

        std::vector<PlatformFilter> filters{PlatformFilter::ALL};
        if (hasFavourite) filters.push_back(PlatformFilter::FAVORITE);
        if (hasGba) filters.push_back(PlatformFilter::GBA);
        if (hasGbc) filters.push_back(PlatformFilter::GBC);
        if (hasGb) filters.push_back(PlatformFilter::GB);
        if (hasNes) filters.push_back(PlatformFilter::NES);
        if (hasSnes) filters.push_back(PlatformFilter::SNES);
        if (hasNds) filters.push_back(PlatformFilter::NDS);
        if (hasThreeDs) filters.push_back(PlatformFilter::THREEDS);
        if (hasGenesis) filters.push_back(PlatformFilter::GENESIS);
        if (hasArcade) filters.push_back(PlatformFilter::ARCADE);
        if (hasDreamcast) filters.push_back(PlatformFilter::DREAMCAST);
        if (hasPsp) filters.push_back(PlatformFilter::PSP);
        return filters;
    }

    void GameLibraryPage::_rebuildAvailableFilters(
        const std::vector<beiklive::GameEntry>& entries)
    {
        m_availableFilters = _buildAvailableFilters(entries);
    }

    int GameLibraryPage::_savedFocusIndex() const
    {
        auto it = m_filterFocusIndices.find(static_cast<int>(m_platformFilter));
        if (it == m_filterFocusIndices.end() || it->second < 0)
            return 0;
        if (m_entries.empty())
            return 0;
        return std::min(it->second, static_cast<int>(m_entries.size()) - 1);
    }

    void GameLibraryPage::_cyclePlatformFilter(int direction)
    {
        if (m_isClosing || m_libraryView->isMultiSelectMode() || m_availableFilters.size() <= 1)
            return;

        auto it = std::find(m_availableFilters.begin(), m_availableFilters.end(),
                            m_platformFilter);
        int index = it == m_availableFilters.end()
            ? 0
            : static_cast<int>(std::distance(m_availableFilters.begin(), it));
        const int count = static_cast<int>(m_availableFilters.size());
        index = (index + (direction < 0 ? -1 : 1) + count) % count;
        const auto next = m_availableFilters[static_cast<size_t>(index)];
        if (next == m_platformFilter)
            return;

        m_platformFilter = next;
        m_platformAnimationDirection = direction < 0 ? -1 : 1;
        _updateHeader();
        _schedulePlatformReload();
    }

    void GameLibraryPage::_schedulePlatformReload()
    {
        const uint64_t generation = ++m_reloadGeneration;
        if (m_libraryView)
            m_libraryView->setInteractionDisabled(true);
        if (m_hasPlatformReloadDelay)
            brls::cancelDelay(m_platformReloadDelayId);

        auto alive = m_aliveToken;
        m_hasPlatformReloadDelay = true;
        m_platformReloadDelayId = brls::delay(60, [this, alive, generation]() {
            if (!alive->load() || m_isClosing || generation != m_reloadGeneration.load())
                return;
            m_hasPlatformReloadDelay = false;
            _reloadEntries(generation, true);
        });
    }

    void GameLibraryPage::_filterEntries()
    {
        _filterAndSortEntries(m_entries, m_platformFilter, m_sortMode,
                              m_isSearching, m_searchTerm);
    }

    void GameLibraryPage::_filterAndSortEntries(
        std::vector<beiklive::GameEntry>& entries,
        PlatformFilter platformFilter,
        SortMode sortMode,
        bool isSearching,
        const std::string& searchTerm)
    {
        if (platformFilter == PlatformFilter::FAVORITE)
        {
            entries.erase(std::remove_if(entries.begin(), entries.end(),
                [](const GameEntry& e) { return !e.favourite; }), entries.end());
        }
        else if (platformFilter != PlatformFilter::ALL)
        {
            int target = static_cast<int>(platformFilter);
            entries.erase(std::remove_if(entries.begin(), entries.end(),
                [target](const GameEntry& e) { return e.platform != target; }), entries.end());
        }
        if (isSearching && !searchTerm.empty())
        {
            std::string lt = searchTerm;
            std::transform(lt.begin(), lt.end(), lt.begin(), [](unsigned char c) { return std::tolower(c); });
            entries.erase(std::remove_if(entries.begin(), entries.end(),
                [&lt](const GameEntry& e) {
                    std::string t = e.title;
                    std::transform(t.begin(), t.end(), t.begin(), [](unsigned char c) { return std::tolower(c); });
                    return t.find(lt) == std::string::npos;
                }), entries.end());
        }

        switch (sortMode) {
        case SortMode::PLAY_TIME:
            std::stable_sort(entries.begin(), entries.end(),
                [](const GameEntry& a, const GameEntry& b) { return a.playTime > b.playTime; });
            break;
        case SortMode::FIRST_LETTER:
            std::stable_sort(entries.begin(), entries.end(),
                [](const GameEntry& a, const GameEntry& b) {
                    return _titleToSortKey(a.title) < _titleToSortKey(b.title);
                });
            break;
        default:
            std::stable_sort(entries.begin(), entries.end(),
                [](const GameEntry& a, const GameEntry& b) { return a.lastPlayed > b.lastPlayed; });
            break;
        }
    }

    void GameLibraryPage::_showFilterDropdown()
    {
        auto alive = m_aliveToken;
        ThreadPool::instance().enqueue([this, alive]() {
            if (!alive->load()) return;
            auto ae = beiklive::GameDB ? beiklive::GameDB->getAll() : std::vector<beiklive::GameEntry>{};
            bool hG = false, hC = false, hB = false, hN = false, hS = false, hD = false, h3 = false;
            bool hMD = false, hArcade = false, hDc = false, hPsp = false;
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
                    case beiklive::enums::EmuPlatform::Emu3DS: h3 = true; break;
                    case beiklive::enums::EmuPlatform::EmuGenesis: hMD = true; break;
                    case beiklive::enums::EmuPlatform::EmuArcade: hArcade = true; break;
                    case beiklive::enums::EmuPlatform::EmuDreamcast: hDc = true; break;
                    case beiklive::enums::EmuPlatform::EmuPSP: hPsp = true; break;
                    default: break;
                }
            }
            brls::sync([this, alive, hG, hC, hB, hN, hS, hD, h3, hMD, hArcade, hDc, hPsp, favCount]() {
                if (!alive->load()) return;
                std::vector<std::string> opts;
                std::vector<PlatformFilter> map;
                opts.push_back(L("所有")); map.push_back(PlatformFilter::ALL);
                if (favCount > 0) { opts.push_back(L("收藏 (") + std::to_string(favCount) + ")"); map.push_back(PlatformFilter::FAVORITE); }
                if (hG) { opts.push_back("GBA"); map.push_back(PlatformFilter::GBA); }
                if (hC) { opts.push_back("GBC"); map.push_back(PlatformFilter::GBC); }
                if (hB) { opts.push_back("GB");  map.push_back(PlatformFilter::GB);  }
                if (hN) { opts.push_back("FC"); map.push_back(PlatformFilter::NES); }
                if (hS) { opts.push_back("SFC"); map.push_back(PlatformFilter::SNES); }
                if (hD) { opts.push_back("NDS"); map.push_back(PlatformFilter::NDS); }
                if (h3) { opts.push_back("3DS"); map.push_back(PlatformFilter::THREEDS); }
                if (hMD) { opts.push_back("MD"); map.push_back(PlatformFilter::GENESIS); }
                if (hArcade) { opts.push_back("Arcade"); map.push_back(PlatformFilter::ARCADE); }
                if (hDc) { opts.push_back("DC"); map.push_back(PlatformFilter::DREAMCAST); }
                if (hPsp) { opts.push_back("PSP"); map.push_back(PlatformFilter::PSP); }
                int cur = 0;
                for (size_t i = 0; i < map.size(); i++)
                    if (map[i] == m_platformFilter) { cur = (int)i; break; }
                auto* dd = new brls::Dropdown(L("游戏分类"), opts,
                    [this, map, alive, cur](int sel) {
                        if (sel < 0 || sel >= (int)map.size()) return;
                        if (map[sel] == m_platformFilter) return;
                        m_platformFilter = map[sel];
                        m_platformAnimationDirection = sel < cur ? -1 : 1;
                        _updateHeader();
                        _reloadEntries();
                    }, cur);
                brls::Application::pushActivity(new brls::Activity(dd));
            });
        });
    }

    void GameLibraryPage::_updateHeader()
    {
        std::string detail;
        if (m_isSearching)
            detail = L("搜索 \"") + m_searchTerm + L("\" · ") + std::to_string(m_entries.size()) + L(" 款");
        else
            detail = "共 " + std::to_string(m_entries.size()) + L(" 款游戏");
        this->getHeader()->setInfo(detail);
        std::string fs;
        switch (m_platformFilter) {
            case PlatformFilter::ALL:      fs = L("所有"); break;
            case PlatformFilter::GBA:      fs = "GBA";  break;
            case PlatformFilter::GBC:      fs = "GBC";  break;
            case PlatformFilter::GB:       fs = "GB";   break;
            case PlatformFilter::NES:      fs = "FC";  break;
            case PlatformFilter::SNES:     fs = "SFC"; break;
            case PlatformFilter::NDS:      fs = "NDS"; break;
            case PlatformFilter::THREEDS:  fs = "3DS"; break;
            case PlatformFilter::GENESIS:  fs = "MD"; break;
            case PlatformFilter::ARCADE:   fs = "Arcade"; break;
            case PlatformFilter::DREAMCAST: fs = "DC"; break;
            case PlatformFilter::PSP:      fs = "PSP"; break;
            case PlatformFilter::FAVORITE: fs = L("收藏"); break;
        }
        this->getHeader()->setPath((m_isSearching ? L("搜索") : L("分类")) + (": " + fs));
        if (m_libraryView) {
            m_libraryView->setLibraryContext(fs, detail);
            auto filterName = [](PlatformFilter filter) -> std::string {
                switch (filter) {
                    case PlatformFilter::ALL: return L("所有");
                    case PlatformFilter::FAVORITE: return L("收藏");
                    case PlatformFilter::GBA: return "GBA";
                    case PlatformFilter::GBC: return "GBC";
                    case PlatformFilter::GB: return "GB";
                    case PlatformFilter::NES: return "FC";
                    case PlatformFilter::SNES: return "SFC";
                    case PlatformFilter::NDS: return "NDS";
                    case PlatformFilter::THREEDS: return "3DS";
                    case PlatformFilter::GENESIS: return "MD";
                    case PlatformFilter::ARCADE: return "Arcade";
                    case PlatformFilter::DREAMCAST: return "DC";
                    case PlatformFilter::PSP: return "PSP";
                }
                return L("所有");
            };
            std::vector<std::string> labels;
            labels.reserve(m_availableFilters.size());
            int selected = 0;
            for (size_t i = 0; i < m_availableFilters.size(); ++i) {
                labels.push_back(filterName(m_availableFilters[i]));
                if (m_availableFilters[i] == m_platformFilter)
                    selected = static_cast<int>(i);
            }
            m_libraryView->setPlatformCarousel(std::move(labels), selected,
                                        m_platformAnimationDirection);
            m_platformAnimationDirection = 0;
        }
    }

    std::string GameLibraryPage::_formatPlayTime(int seconds)
    {
        if (seconds <= 0) return "";
        return beiklive::tools::formatPlayTime(seconds);
    }

    void GameLibraryPage::_showSortSelector()
    {
        std::vector<std::string> opts = {L("最近游玩"), L("游玩时长"), L("首字母")};
        int cur = static_cast<int>(m_sortMode);
        auto* dd = new brls::Dropdown(L("排序方式"), opts,
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

    void GameLibraryPage::_presentReloadedEntries(
        uint64_t requestGeneration,
        std::vector<beiklive::GameEntry> entries,
        std::vector<PlatformFilter> filters,
        PlatformFilter resolvedFilter,
        bool favoriteFallback,
        bool isSearching,
        const std::string& searchTerm)
    {
        if (!m_aliveToken->load() || requestGeneration != m_reloadGeneration.load() ||
            m_isClosing)
            return;

        const bool initialPresentation = !m_hasPresentedInitialData;
        m_entries = std::move(entries);
        m_availableFilters = std::move(filters);
        m_platformFilter = resolvedFilter;
        if (favoriteFallback)
            brls::Application::notify(L("收藏列表为空，已切换至所有游戏"));
        m_visibleCount = static_cast<int>(m_entries.size());
        m_libraryView->setDefaultCellFocus(static_cast<size_t>(_savedFocusIndex()));
        m_dataSource = new GameLibraryDS(this);
        m_libraryView->setDataSource(m_dataSource);
        _updateHeader();
        m_libraryView->reloadData();
        m_libraryView->setInteractionDisabled(false);
        brls::Application::giveFocus(m_libraryView);
        if (initialPresentation) {
            m_hasPresentedInitialData = true;
            m_libraryView->restartEntranceAnimation();
        }
        if (isSearching && m_entries.empty()) {
            auto* dialog = new brls::Dialog(
                L("当前分类下无 \"") + searchTerm + L("\""));
            dialog->addButton(L("确认"), []() {});
            dialog->open();
        }
    }

    void GameLibraryPage::_reloadEntries(uint64_t requestGeneration,
                                         bool useFastSnapshot)
    {
        if (m_isClosing)
            return;
        if (requestGeneration == 0) {
            if (m_hasPlatformReloadDelay) {
                brls::cancelDelay(m_platformReloadDelayId);
                m_hasPlatformReloadDelay = false;
            }
            requestGeneration = ++m_reloadGeneration;
        }

        if (m_libraryView)
            m_libraryView->setInteractionDisabled(true);
        auto alive = m_aliveToken;
        const PlatformFilter platformFilter = m_platformFilter;
        const SortMode sortMode = m_sortMode;
        const bool isSearching = m_isSearching;
        const std::string searchTerm = m_searchTerm;

        if (useFastSnapshot) {
            auto allEntries = beiklive::GameDB
                ? beiklive::GameDB->getAll()
                : std::vector<beiklive::GameEntry>{};
            auto availableFilters = _buildAvailableFilters(allEntries);
            auto filteredEntries = allEntries;
            PlatformFilter resolvedFilter = platformFilter;
            _filterAndSortEntries(filteredEntries, resolvedFilter, sortMode,
                                  isSearching, searchTerm);
            bool favoriteFallback = false;
            if (resolvedFilter == PlatformFilter::FAVORITE && filteredEntries.empty()) {
                resolvedFilter = PlatformFilter::ALL;
                filteredEntries = std::move(allEntries);
                _filterAndSortEntries(filteredEntries, resolvedFilter, sortMode,
                                      isSearching, searchTerm);
                favoriteFallback = true;
            }
            _presentReloadedEntries(
                requestGeneration, std::move(filteredEntries),
                std::move(availableFilters), resolvedFilter, favoriteFallback,
                isSearching, searchTerm);
            return;
        }

        ThreadPool::instance().enqueue([
            this, alive, requestGeneration, platformFilter,
            sortMode, isSearching, searchTerm]() {
            if (!alive->load()) return;
            auto allEntries = loadLibraryEntries();
            auto availableFilters = _buildAvailableFilters(allEntries);
            auto filteredEntries = allEntries;
            PlatformFilter resolvedFilter = platformFilter;
            _filterAndSortEntries(filteredEntries, resolvedFilter, sortMode,
                                  isSearching, searchTerm);
            bool favoriteFallback = false;
            if (resolvedFilter == PlatformFilter::FAVORITE && filteredEntries.empty()) {
                resolvedFilter = PlatformFilter::ALL;
                filteredEntries = std::move(allEntries);
                _filterAndSortEntries(filteredEntries, resolvedFilter, sortMode,
                                      isSearching, searchTerm);
                favoriteFallback = true;
            }
            if (!alive->load())
                return;
            brls::sync([
                this, alive, requestGeneration,
                entries = std::move(filteredEntries),
                filters = std::move(availableFilters),
                resolvedFilter, favoriteFallback,
                isSearching, searchTerm]() mutable {
                if (!alive->load()) return;
                _presentReloadedEntries(
                    requestGeneration, std::move(entries), std::move(filters),
                    resolvedFilter, favoriteFallback, isSearching, searchTerm);
            });
        });
    }

    void GameLibraryPage::_showGameOptionsPanel(const beiklive::GameEntry& entry)
    {
        std::string path = entry.path;
        const int selectedIndex = m_libraryView->getSelectedIndex();
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        m_gameOptionsSidebar->setNanoVgMenu(true);
        this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        std::string fn = beiklive::tools::getFileNameWithoutExtension(entry.path);

        m_gameOptionsSidebar->addButton(L("启动游戏"), beiklive::material::PLAY_ARROW,
            [this, entry, selectedIndex](const beiklive::GameEntry&) {
                if (entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuNDS) &&
                    !beiklive::ensureNdsEnvironmentReady()) {
                    m_libraryView->setInteractionDisabled(false);
                    return;
                }
                _closeGameOptionsPanelAnimated([this, entry, selectedIndex]() {
                    auto alive = m_aliveToken;
                    m_libraryView->playLaunchAnimation(
                        static_cast<size_t>(std::max(0, selectedIndex)),
                        [this, alive, entry]() {
                            if (alive->load() && onGameSelected)
                                onGameSelected(entry);
                        }, true);
                }, true);
            });

        m_gameOptionsSidebar->addButton(
            entry.favourite ? L("取消收藏") : L("加入收藏"),
            entry.favourite ? beiklive::material::FAVORITE : beiklive::material::FAVORITE_BORDER,
            [this, path, favourite = entry.favourite, selectedIndex](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated(
                    [this, path, favourite, selectedIndex]() {
                        if (beiklive::GameDB) {
                            beiklive::GameDB->set(path, "favourite", nlohmann::json(!favourite));
                            beiklive::GameDB->flush();
                            if (selectedIndex >= 0 &&
                                static_cast<size_t>(selectedIndex) < m_entries.size())
                                m_entries[static_cast<size_t>(selectedIndex)].favourite = !favourite;
                            m_libraryView->setItemFavourite(
                                static_cast<size_t>(std::max(0, selectedIndex)), !favourite);
                        }
                        m_libraryView->setInteractionDisabled(false);
                        brls::Application::giveFocus(m_libraryView);
                        if (m_platformFilter == PlatformFilter::FAVORITE && favourite)
                            _reloadEntries();
                    });
            });

        const int operationsMenu = m_gameOptionsSidebar->addSubmenu(
            L("游戏操作"), beiklive::material::SETTINGS);

        m_gameOptionsSidebar->addSubmenuButton(operationsMenu, L("修改映射名称"), beiklive::material::EDIT,
            [this, path, title = entry.title, fn, idx = m_libraryView->getSelectedIndex()](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, path, title, fn, idx]() {
                    auto* ime = brls::Application::getPlatform()->getImeManager();
                    if (!ime) { m_libraryView->setInteractionDisabled(false); return; }
                    ime->openForText(
                        [this, path, fn, idx](std::string text) {
                            if (!text.empty() && beiklive::GameDB) {
                                beiklive::GameDB->set(path, "title", nlohmann::json(text));
                                beiklive::GameDB->flush();
                                beiklive::NameMappingManager->Set(fn, text, true);
                                beiklive::NameMappingManager->Save();
                                m_libraryView->setItemTitle(idx, text);
                            }
                            m_libraryView->setInteractionDisabled(false);
                        },
                        L("编辑游戏名称"), "", 128, title,
                        brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
                });
            });

        const int coverMenu = m_gameOptionsSidebar->addNestedSubmenu(
            operationsMenu, L("修改封面"), beiklive::material::IMAGE);

        m_gameOptionsSidebar->addNestedSubmenuButton(
            operationsMenu, coverMenu, L("从 SteamGridDB 获取"),
            beiklive::material::CLOUD_DOWNLOAD,
            [this, entry, idx = m_libraryView->getSelectedIndex()](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, entry, idx]() {
                    if (!beiklive::steamgriddb::hasApiKey()) {
                        m_libraryView->setInteractionDisabled(false);
                        brls::Application::giveFocus(m_libraryView);
                        brls::Application::notify(
                            L("请去设置-模拟器页面输入 SteamGridDB Api Key"));
                        return;
                    }
                    beiklive::openSteamGridDbPage(entry,
                        [this, idx](const std::string& coverPath) {
                            if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size())
                                m_entries[static_cast<size_t>(idx)].logoPath = coverPath;
                            if (idx >= 0)
                                m_libraryView->setItemImagePath(idx, coverPath);
                        });
                    m_libraryView->setInteractionDisabled(false);
                });
            });

        m_gameOptionsSidebar->addNestedSubmenuButton(
            operationsMenu, coverMenu, L("从本地选择"), 0xE2C8,
            [this, idx = m_libraryView->getSelectedIndex()](const beiklive::GameEntry& entry) {
                const auto pickerLocation = beiklive::getGameCoverPickerLocation(entry);
                _closeGameOptionsPanelAnimated([this, entry, idx, pickerLocation]() {
                    beiklive::openFilePicker({"png", "jpg"},
                        [this, entry, idx](const std::string& selectedPath) {
                            if (selectedPath.empty()) {
                                m_libraryView->setInteractionDisabled(false);
                                return;
                            }
                            beiklive::openCoverEditorPage(entry, selectedPath,
                                [this, idx](const std::string& coverPath) {
                                    if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size())
                                        m_entries[static_cast<size_t>(idx)].logoPath = coverPath;
                                    if (idx >= 0)
                                        m_libraryView->setItemImagePath(idx, coverPath);
                                });
                            m_libraryView->setInteractionDisabled(false);
                        },
                        pickerLocation.startPath,
                        pickerLocation.filename);
                });
            });

        m_gameOptionsSidebar->addSubmenuButton(operationsMenu, L("安装到 Switch 桌面"), beiklive::material::INSTALL_APP,
            [this](const beiklive::GameEntry& game) {
                _closeGameOptionsPanelAnimated([this, game]() {
                    m_libraryView->setInteractionDisabled(false);
                    beiklive::forwarder::showInstallDialog(game);
                });
            });

        m_gameOptionsSidebar->addButton(L("数据管理"), beiklive::material::STORAGE,
            [this, entry](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, entry]() {
                    _openGameDataPage(entry);
                    m_libraryView->setInteractionDisabled(false);
                });
            });

        if (beiklive::GetCoreOptions(entry.platform).size() > 1)
        {
            m_gameOptionsSidebar->addSubmenuButton(operationsMenu, L("核心切换"), beiklive::material::MEMORY,
                [this, path, platform = entry.platform, core = entry.core,
                 idx = m_libraryView->getSelectedIndex()](const beiklive::GameEntry&) {
                    _closeGameOptionsPanelAnimated(
                        [this, path, platform, core, idx]() {
                            const auto options = beiklive::GetCoreOptions(platform);
                            std::vector<std::string> names;
                            names.reserve(options.size());
                            for (const auto& option : options)
                                names.push_back(option.name);

                            auto* dropdown = new brls::Dropdown(
                                L("核心切换"),
                                names,
                                [this, path, idx, options](int selected) {
                                    if (selected < 0 || selected >= static_cast<int>(options.size()))
                                        return;
                                    if (beiklive::GameDB) {
                                        beiklive::GameDB->set(path, "core", nlohmann::json(options[selected].id));
                                        beiklive::GameDB->flush();
                                        if (idx >= 0 && static_cast<size_t>(idx) < m_entries.size())
                                            m_entries[idx].core = options[selected].id;
                                        brls::Application::notify(L("已切换核心：") + options[selected].name);
                                    }
                                    m_libraryView->setInteractionDisabled(false);
                                },
                                beiklive::GetCoreSelectionIndex(platform, core),
                                [this](int) {
                                    m_libraryView->setInteractionDisabled(false);
                                });
                            brls::Application::pushActivity(new brls::Activity(dropdown));
                        });
                });
        }

        m_gameOptionsSidebar->addSubmenuButton(operationsMenu, L("删除游戏"), beiklive::material::DELETE_ICON,
            [this, selectedIndex](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this, selectedIndex]() {
                    auto deleteGame = [this, selectedIndex](bool deleteRomFile) {
                        _deleteEntriesAsync({selectedIndex}, deleteRomFile);
                    };
                    auto* dialog = new brls::Dialog(
                        L("请选择游戏的删除方式"));
                    dialog->addButton(L("仅从库中移除"),
                        [deleteGame]() { deleteGame(false); });
                    dialog->addButton(L("移除并删除文件"),
                        [deleteGame]() { deleteGame(true); });
                    dialog->addButton(L("取消"), [this]() {
                        m_libraryView->setInteractionDisabled(false);
                        brls::Application::giveFocus(m_libraryView);
                    });
                    dialog->setCancelable(false);
                    dialog->open();
                });
            });

        m_gameOptionsSidebar->onClosed = [this]() {
            m_libraryView->setInteractionDisabled(false);
            brls::Application::giveFocus(m_libraryView);
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        };
        m_gameOptionsSidebar->onCloseRequested = [this]() {
            _closeGameOptionsPanelAnimated({});
        };
        this->addView(m_gameOptionsSidebar);
        m_libraryView->setInteractionDisabled(true);
        m_gameOptionsSidebar->open(entry);
    }

    void GameLibraryPage::_hideGameOptionsPanel()
    {
        if (m_gameOptionsSidebar) {
            m_gameOptionsSidebar->close();
            m_gameOptionsSidebar->removeFromSuperView(true);
            m_gameOptionsSidebar = nullptr;
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        }
    }

    void GameLibraryPage::_closeGameOptionsPanelAnimated(
        std::function<void()> completion, bool launchTransition)
    {
        if (!m_gameOptionsSidebar) {
            if (completion) completion();
            return;
        }
        auto* sidebar = m_gameOptionsSidebar;
        auto alive = m_aliveToken;
        auto finishClose = [this, alive, sidebar,
                        completion = std::move(completion)]() mutable {
            brls::sync([this, alive, sidebar,
                        completion = std::move(completion)]() mutable {
                if (!alive->load()) return;
                if (m_gameOptionsSidebar == sidebar)
                    m_gameOptionsSidebar = nullptr;
                sidebar->removeFromSuperView(true);
                this->getBottomBar()->setVisibility(brls::Visibility::GONE);
                if (completion) completion();
            });
        };
        if (launchTransition)
            sidebar->closeForLaunch(std::move(finishClose));
        else
            sidebar->close(std::move(finishClose));
    }

    void GameLibraryPage::_deleteEntriesAsync(std::vector<int> indices,
                                              bool deleteRomFiles)
    {
        std::sort(indices.begin(), indices.end());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());
        indices.erase(std::remove_if(indices.begin(), indices.end(),
            [this](int index) {
                return index < 0 || static_cast<size_t>(index) >= m_entries.size();
            }), indices.end());
        if (indices.empty()) {
            m_libraryView->setInteractionDisabled(false);
            return;
        }

        struct DeleteTarget {
            int index;
            beiklive::GameEntry entry;
        };
        std::vector<DeleteTarget> targets;
        targets.reserve(indices.size());
        for (int index : indices)
            targets.push_back({index, m_entries[static_cast<size_t>(index)]});

        m_libraryView->beginDeleteAnimation(indices);
        auto alive = m_aliveToken;
        ThreadPool::instance().enqueue([
            this, alive, targets = std::move(targets),
            deleteRomFiles]() mutable {
            std::vector<int> removedIndices;
            bool allFilesRemoved = true;
            if (alive->load() && beiklive::GameDB) {
                std::vector<DeleteTarget> databaseTargets;
                databaseTargets.reserve(targets.size());
                for (const auto& target : targets) {
                    if (!alive->load()) return;
                    const bool filesRemoved = !deleteRomFiles ||
                        deleteGameFilesForEntry(target.entry);
                    if (!filesRemoved) {
                        allFilesRemoved = false;
                        brls::Logger::warning(
                            "[Game Delete] file removal failed, database record retained: path={}",
                            target.entry.path);
                        continue;
                    }
                    databaseTargets.push_back(target);
                }

                const size_t databaseCount = beiklive::GameDB->getAll().size();
                if (databaseCount > 0 && databaseTargets.size() >= databaseCount) {
                    brls::Logger::info(
                        "[Game Delete] clearing entire database: targets={} database_count={} "
                        "delete_files={}",
                        databaseTargets.size(), databaseCount, deleteRomFiles);
                    beiklive::GameDB->clearAll();
                    for (const auto& target : databaseTargets)
                        removedIndices.push_back(target.index);
                } else {
                    for (const auto& target : databaseTargets) {
                        if (!alive->load()) return;
                        const bool removedRecord =
                            beiklive::GameDB->removeByPath(target.entry.path);
                        brls::Logger::info(
                            "[Game Delete] database remove result: path={} removed={}",
                            target.entry.path, removedRecord);
                        if (!removedRecord)
                            continue;
                        removedIndices.push_back(target.index);
                    }
                    if (!removedIndices.empty())
                        beiklive::GameDB->flush();
                }
            }

            if (!alive->load()) return;
            brls::sync([this, alive, requested = targets.size(),
                        removedIndices = std::move(removedIndices),
                        deleteRomFiles, allFilesRemoved]() mutable {
                if (!alive->load()) return;
                if (removedIndices.empty()) {
                    m_libraryView->cancelDeleteAnimation();
                    brls::Application::notify(
                        deleteRomFiles && !allFilesRemoved
                            ? L("游戏文件删除失败，记录已保留")
                            : L("删除失败"));
                    return;
                }
                if (removedIndices.size() != requested) {
                    m_libraryView->cancelDeleteAnimation();
                    m_libraryView->beginDeleteAnimation(removedIndices);
                }
                m_libraryView->completeDeleteAnimation([
                    this, alive, removedCount = removedIndices.size(),
                    deleteRomFiles, allFilesRemoved]() {
                    if (!alive->load()) return;
                    m_libraryView->clearDeleteSelection();
                    if (deleteRomFiles && !allFilesRemoved)
                        brls::Application::notify(L("部分游戏删除失败，失败记录已保留"));
                    else
                        brls::Application::notify(deleteRomFiles
                            ? L("已删除 ") + std::to_string(removedCount) + L(" 款游戏")
                            : L("已从游戏库移除 ") + std::to_string(removedCount) + L(" 款游戏"));
                    _reloadEntries(0, true);
                });
            });
        });
    }

    void GameLibraryPage::_showMultiSelectSidebar()
    {
        m_libraryView->setInteractionDisabled(true);
        _hideGameOptionsPanel();
        m_gameOptionsSidebar = new beiklive::GameOptionsSidebar();
        m_gameOptionsSidebar->setNanoVgMenu(true);
        this->getBottomBar()->setVisibility(brls::Visibility::GONE);

        size_t count = m_libraryView->getDeleteSelection().size();
        m_gameOptionsSidebar->setNanoVgPreviewIcon(
            beiklive::material::SELECT_ALL,
            L("已选择 ") + std::to_string(count) + L(" 款游戏"));

        m_gameOptionsSidebar->addButton(
            L("退出多选"),
            beiklive::material::CLOSE,
            [this](const beiklive::GameEntry&) {
                _closeGameOptionsPanelAnimated([this]() {
                    m_libraryView->clearDeleteSelection();
                    m_libraryView->setInteractionDisabled(false);
                    brls::Application::giveFocus(m_libraryView);
                    brls::Application::notify(L("已退出多选模式"));
                });
            });

        if (count > 0) {
            m_gameOptionsSidebar->addButton(
                L("删除已选游戏 (") + std::to_string(count) + ")",
                beiklive::material::DELETE_SWEEP_ICON,
                [this](const beiklive::GameEntry&) {
                    std::vector<int> sel(m_libraryView->getDeleteSelection().begin(),
                                         m_libraryView->getDeleteSelection().end());
                    _closeGameOptionsPanelAnimated([this, sel]() {
                        size_t n = sel.size();
                        auto deleteSelected = [this, sel](bool deleteRomFiles) {
                            _deleteEntriesAsync(sel, deleteRomFiles);
                        };
                        auto* dialog = new brls::Dialog(
                            L("请选择这 ") + std::to_string(n) +
                            L(" 款游戏的删除方式"));
                        dialog->addButton(L("仅从库中移除"),
                            [deleteSelected]() { deleteSelected(false); });
                        dialog->addButton(L("移除并删除文件"),
                            [deleteSelected]() { deleteSelected(true); });
                        dialog->addButton(L("取消"), [this]() {
                            m_libraryView->setInteractionDisabled(false);
                            brls::Application::giveFocus(m_libraryView);
                        });
                        dialog->setCancelable(false);
                        dialog->open();
                    });
                });

            m_gameOptionsSidebar->addButton(
                L("收藏选中游戏 (") + std::to_string(count) + ")",
                beiklive::material::FAVORITE,
                [this](const beiklive::GameEntry&) {
                    std::vector<int> sel(m_libraryView->getDeleteSelection().begin(),
                                         m_libraryView->getDeleteSelection().end());
                    _closeGameOptionsPanelAnimated([this, sel]() {
                        auto* dlg = new brls::Dialog(L("确认将选中的 ") +
                            std::to_string(sel.size()) + L(" 款游戏添加到收藏？"));
                        dlg->addButton(L("取消"), [this]() {
                            m_libraryView->setInteractionDisabled(false);
                        });
                        dlg->addButton(L("确认"), [this, sel]() {
                            size_t updated = 0;
                            if (beiklive::GameDB) {
                                for (int idx : sel) {
                                    if (idx < 0 || static_cast<size_t>(idx) >= m_entries.size())
                                        continue;
                                    beiklive::GameDB->set(m_entries[idx].path, "favourite", nlohmann::json(true));
                                    m_entries[idx].favourite = true;
                                    m_libraryView->setItemFavourite(idx, true);
                                    ++updated;
                                }
                                beiklive::GameDB->flush();
                            }
                            m_libraryView->clearDeleteSelection();
                            m_libraryView->setInteractionDisabled(false);
                            brls::Application::notify(updated > 0
                                ? L("已添加到收藏：") + std::to_string(updated) + L(" 款")
                                : L("未选择可收藏的游戏"));
                            if (m_platformFilter == PlatformFilter::FAVORITE)
                                _reloadEntries();
                        });
                        dlg->open();
                    });
                });

            m_gameOptionsSidebar->addButton(
                L("取消收藏选中游戏 (") + std::to_string(count) + ")",
                beiklive::material::FAVORITE_BORDER,
                [this](const beiklive::GameEntry&) {
                    std::vector<int> sel(m_libraryView->getDeleteSelection().begin(),
                                         m_libraryView->getDeleteSelection().end());
                    _closeGameOptionsPanelAnimated([this, sel]() {
                        auto* dlg = new brls::Dialog(L("确认取消收藏选中的 ") +
                            std::to_string(sel.size()) + L(" 款游戏？"));
                        dlg->addButton(L("取消"), [this]() {
                            m_libraryView->setInteractionDisabled(false);
                        });
                        dlg->addButton(L("确认"), [this, sel]() {
                            size_t updated = 0;
                            if (beiklive::GameDB) {
                                for (int idx : sel) {
                                    if (idx < 0 || static_cast<size_t>(idx) >= m_entries.size())
                                        continue;
                                    beiklive::GameDB->set(m_entries[idx].path, "favourite", nlohmann::json(false));
                                    m_entries[idx].favourite = false;
                                    m_libraryView->setItemFavourite(idx, false);
                                    ++updated;
                                }
                                beiklive::GameDB->flush();
                            }
                            m_libraryView->clearDeleteSelection();
                            m_libraryView->setInteractionDisabled(false);
                            brls::Application::notify(updated > 0
                                ? L("已取消收藏：") + std::to_string(updated) + L(" 款")
                                : L("未选择可取消收藏的游戏"));
                            if (m_platformFilter == PlatformFilter::FAVORITE)
                                _reloadEntries();
                        });
                        dlg->open();
                    });
                });
        }

        m_gameOptionsSidebar->onClosed = [this]() {
            m_libraryView->setInteractionDisabled(false);
            brls::Application::giveFocus(m_libraryView);
            this->getBottomBar()->setVisibility(brls::Visibility::GONE);
        };
        m_gameOptionsSidebar->onCloseRequested = [this]() {
            _closeGameOptionsPanelAnimated({});
        };
        this->addView(m_gameOptionsSidebar);
        m_gameOptionsSidebar->open(beiklive::GameEntry{});
    }

    void GameLibraryPage::_openGameDataPage(const beiklive::GameEntry& entry)
    {
        auto* page = new beiklive::GameDataPage(entry);
        auto* frame = new brls::AppletFrame(page);
        HIDE_BRLS_BAR(frame);
        beiklive::pushActivity(frame, this, page);
    }

} // namespace beiklive
