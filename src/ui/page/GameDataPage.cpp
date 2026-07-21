#include "GameDataPage.hpp"

#include "core/Tools.hpp"
#include "core/common.h"
#include "ui/utils/FilePickerHelper.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

namespace
{
    bool copyBinaryFile(const fs::path& source, const fs::path& target,
                        std::string* error = nullptr)
    {
        std::error_code ec;
        const fs::path parent = target.parent_path();
        if (!parent.empty()) {
            fs::create_directories(parent, ec);
            if (ec) {
                if (error) *error = ec.message();
                return false;
            }
        }
        std::ifstream input(source.string(), std::ios::binary);
        if (!input) {
            if (error) *error = "open source failed";
            return false;
        }
        std::ofstream output(target.string(), std::ios::binary | std::ios::trunc);
        if (!output) {
            if (error) *error = "open target failed";
            return false;
        }
        output << input.rdbuf();
        output.flush();
        if (!output || input.bad()) {
            if (error) *error = "copy stream failed";
            return false;
        }
        return true;
    }

    std::string gameStem(const beiklive::GameEntry& entry)
    {
        const std::string stem = fs::path(entry.path).stem().string();
        return stem.empty() ? "game" : stem;
    }

    std::string timestampForFile()
    {
        const auto now = std::chrono::system_clock::now();
        const auto value = std::chrono::system_clock::to_time_t(now);
        std::tm time{};
#ifdef _WIN32
        localtime_s(&time, &value);
#else
        localtime_r(&value, &time);
#endif
        std::ostringstream stream;
        stream << std::put_time(&time, "%Y%m%d_%H%M%S");
        return stream.str();
    }

    std::vector<fs::path> listImages(const std::string& directory)
    {
        std::vector<fs::path> files;
        std::error_code ec;
        for (const auto& item : fs::directory_iterator(directory, ec)) {
            if (ec || !item.is_regular_file()) continue;
            std::string extension = item.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
            if (extension == ".png" || extension == ".jpg" || extension == ".jpeg")
                files.push_back(item.path());
        }
        std::sort(files.begin(), files.end(), std::greater<fs::path>());
        return files;
    }
}

namespace beiklive
{
    GameDataPage::GameDataPage(beiklive::GameEntry entry)
        : m_entry(std::move(entry))
    {
        showHeader(false);
        showFooter(false);
        setFocusable(false);
        getContentBox()->setMargins(0.f, 0.f, 0.f, 0.f);
        _initView();
    }

    GameDataPage::~GameDataPage()
    {
        m_alive->store(false);
    }

    std::string GameDataPage::_saveDir() const
    {
        std::string directory = m_entry.savePath.empty()
            ? beiklive::tools::defaultGameSavePath(m_entry.platform, m_entry.path)
            : m_entry.savePath;
        std::error_code ec;
        fs::create_directories(directory, ec);
        return directory;
    }

    std::string GameDataPage::_statePath(int slot) const
    {
        return beiklive::tools::getStatePath(_saveDir(), m_entry.path, slot);
    }

    std::string GameDataPage::_stateThumbPath(int slot) const
    {
        return beiklive::tools::getStateThumbPath(_saveDir(), m_entry.path, slot);
    }

    std::string GameDataPage::_savPath() const
    {
        return (fs::path(_saveDir()) / (gameStem(m_entry) + ".sav")).string();
    }

    void GameDataPage::_initView()
    {
        m_view = new beiklive::GameDataView(m_entry);
        m_view->setGrow(1.f);
        m_view->onBack = [this]() { _closeAnimated(); };
        m_view->onSectionChanged = [this](GameDataView::Section section) {
            if (section == GameDataView::Section::STATES) _refreshStateList();
            else if (section == GameDataView::Section::SCREENSHOTS) _refreshScreenshotList();
            else _refreshBackupList();
        };
        m_view->onDeleteState = [this](int slot) { _confirmDeleteState(slot); };
        m_view->onDeleteScreenshot = [this](int index) { _confirmDeleteScreenshot(index); };
        m_view->onSetScreenshotCover = [this](int index) { _confirmSetScreenshotAsCover(index); };
        m_view->onExportSave = [this]() { _exportSav(); };
        m_view->onImportSave = [this]() { _importSav(); };
        m_view->onBackupSave = [this]() { _backupSav(); };
        m_view->onRestoreBackup = [this](int index) { _confirmRestoreBackup(index); };
        m_view->onDeleteBackup = [this](int index) { _confirmDeleteBackup(index); };
        getContentBox()->addView(m_view);

        _refreshStateList();
        _refreshScreenshotList();
        _refreshBackupList();
        m_view->restoreFocus();
    }

    void GameDataPage::_closeAnimated()
    {
        if (m_closing || !m_view) return;
        m_closing = true;
        const auto alive = m_alive;
        m_view->playExitAnimation([this, alive]() {
            if (alive->load())
                beiklive::popActivity(this, false);
        });
    }

    void GameDataPage::_refreshStateList()
    {
        if (!m_view) return;
        std::vector<GameDataView::StateSlot> slots;
        slots.reserve(10);
        for (int slot = 0; slot < 10; ++slot) {
            GameDataView::StateSlot data;
            data.title = beiklive::tools::slotName(slot);
            const std::string state = _statePath(slot);
            const std::string thumbnail = _stateThumbPath(slot);
            std::error_code ec;
            data.exists = fs::exists(state, ec) && !ec;
            if (data.exists) {
                data.time = beiklive::tools::getFileModTimeStr(state);
                ec.clear();
                if (fs::exists(thumbnail, ec) && !ec)
                    data.thumbnail = thumbnail;
            }
            slots.push_back(std::move(data));
        }
        m_view->setStateSlots(std::move(slots));
    }

    void GameDataPage::_refreshScreenshotList()
    {
        if (!m_view) return;
        m_screenshotPaths = listImages(_saveDir());
        std::vector<GameDataView::MediaItem> items;
        items.reserve(m_screenshotPaths.size());
        for (const auto& path : m_screenshotPaths) {
            items.push_back({
                path.string(), path.filename().string(),
                beiklive::tools::getFileModTimeStr(path.string())
            });
        }
        m_view->setScreenshots(std::move(items));
    }

    void GameDataPage::_refreshBackupList()
    {
        if (!m_view) return;
        m_backupPaths.clear();
        const fs::path save = _savPath();
        const std::string prefix = save.filename().string() + ".bak_";
        std::error_code ec;
        for (const auto& item : fs::directory_iterator(_saveDir(), ec)) {
            if (ec || !item.is_regular_file()) continue;
            const std::string name = item.path().filename().string();
            if (name.rfind(prefix, 0) != 0) continue;
            std::error_code sizeError;
            if (fs::file_size(item.path(), sizeError) == 0 || sizeError) continue;
            m_backupPaths.push_back(item.path());
        }
        std::sort(m_backupPaths.begin(), m_backupPaths.end(), std::greater<fs::path>());
        std::vector<GameDataView::MediaItem> items;
        items.reserve(m_backupPaths.size());
        for (const auto& path : m_backupPaths) {
            items.push_back({
                path.string(), path.filename().string(),
                beiklive::tools::getFileModTimeStr(path.string())
            });
        }
        ec.clear();
        const bool saveExists = fs::exists(save, ec) && !ec;
        m_view->setBackups(std::move(items), saveExists);
    }

    void GameDataPage::_confirmDeleteState(int slot)
    {
        std::error_code ec;
        if (!fs::exists(_statePath(slot), ec)) return;
        auto* dialog = new brls::Dialog("确认删除" + beiklive::tools::slotName(slot) + "？");
        dialog->addButton("取消", []() {});
        dialog->addButton("删除", [this, slot]() {
            std::error_code stateError;
            fs::remove(_statePath(slot), stateError);
            std::error_code thumbError;
            fs::remove(_stateThumbPath(slot), thumbError);
            brls::Application::notify(stateError ? "删除失败" : "已删除存档");
            _refreshStateList();
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_confirmDeleteScreenshot(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_screenshotPaths.size())) return;
        const fs::path path = m_screenshotPaths[static_cast<size_t>(index)];
        auto* dialog = new brls::Dialog("确认删除截图\n" + path.filename().string() + "？");
        dialog->addButton("取消", []() {});
        dialog->addButton("删除", [this, path]() {
            std::error_code ec;
            fs::remove(path, ec);
            brls::Application::notify(ec ? "删除失败" : "已删除截图");
            _refreshScreenshotList();
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_confirmSetScreenshotAsCover(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_screenshotPaths.size())) return;
        const std::string cover = m_screenshotPaths[static_cast<size_t>(index)].string();
        auto* dialog = new brls::Dialog(
            "确认将该截图设置为封面？\n" +
            m_screenshotPaths[static_cast<size_t>(index)].filename().string());
        dialog->addButton("取消", []() {});
        dialog->addButton("确认", [this, cover]() {
            if (!beiklive::GameDB) return;
            beiklive::GameDB->set(m_entry.path, "logoPath", nlohmann::json(cover));
            beiklive::GameDB->flush();
            m_entry.logoPath = cover;
            m_view->setCoverPath(cover);
            brls::Application::notify("已设置为封面图片");
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_exportSav()
    {
        const std::string source = _savPath();
        std::error_code ec;
        if (!fs::exists(source, ec)) {
            brls::Application::notify("未找到电池存档");
            return;
        }
        auto* dialog = new brls::Dialog("确认导出当前电池存档？");
        dialog->addButton("取消", []() {});
        dialog->addButton("导出", [source]() {
            const fs::path directory("sdmc:/GBAStation/export");
            std::string error;
            if (!copyBinaryFile(source, directory / fs::path(source).filename(), &error)) {
                brls::Logger::warning("导出电池存档失败: {}", error);
                brls::Application::notify("导出失败");
                return;
            }
            brls::Application::notify("已导出存档");
        });
        dialog->open();
    }

    void GameDataPage::_importSav()
    {
        auto* dialog = new brls::Dialog("确认导入外部 .sav 并覆盖当前电池存档？");
        dialog->addButton("取消", []() {});
        const auto alive = m_alive;
        dialog->addButton("选择文件", [this, alive]() {
            beiklive::openFilePicker({"sav"}, [this, alive](const std::string& selected) {
                if (!alive->load()) return;
                std::string error;
                if (!copyBinaryFile(selected, _savPath(), &error)) {
                    brls::Logger::warning("导入电池存档失败: {}", error);
                    brls::Application::notify("导入失败");
                    return;
                }
                brls::Application::notify("已导入存档");
                _refreshBackupList();
                m_view->restoreFocus();
            }, beiklive::path::GetRootPath());
        });
        dialog->open();
    }

    void GameDataPage::_backupSav()
    {
        const std::string source = _savPath();
        std::error_code ec;
        if (!fs::exists(source, ec)) {
            brls::Application::notify("未找到电池存档");
            return;
        }
        auto* dialog = new brls::Dialog("确认为当前电池存档创建备份？");
        dialog->addButton("取消", []() {});
        dialog->addButton("备份", [this, source]() {
            const fs::path backup = source + ".bak_" + timestampForFile();
            std::string error;
            if (!copyBinaryFile(source, backup, &error)) {
                std::error_code removeError;
                fs::remove(backup, removeError);
                brls::Logger::warning("备份电池存档失败: {}", error);
                brls::Application::notify("备份失败");
                return;
            }
            brls::Application::notify("已创建备份");
            _refreshBackupList();
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_confirmRestoreBackup(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_backupPaths.size())) return;
        const fs::path backup = m_backupPaths[static_cast<size_t>(index)];
        auto* dialog = new brls::Dialog("确认还原备份\n" + backup.filename().string() + "？");
        dialog->addButton("取消", []() {});
        dialog->addButton("还原", [this, backup]() {
            std::string error;
            if (!copyBinaryFile(backup, _savPath(), &error)) {
                brls::Logger::warning("还原电池存档失败: {}", error);
                brls::Application::notify("还原失败");
                return;
            }
            brls::Application::notify("已还原存档");
            _refreshBackupList();
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_confirmDeleteBackup(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_backupPaths.size())) return;
        const fs::path backup = m_backupPaths[static_cast<size_t>(index)];
        auto* dialog = new brls::Dialog("确认删除备份\n" + backup.filename().string() + "？");
        dialog->addButton("取消", []() {});
        dialog->addButton("删除", [this, backup]() {
            std::error_code ec;
            fs::remove(backup, ec);
            brls::Application::notify(ec ? "删除失败" : "已删除备份");
            _refreshBackupList();
            m_view->restoreFocus();
        });
        dialog->open();
    }
}
