#include "GameDataPage.hpp"

#include "core/ThreeDsTitlePaths.hpp"
#include "core/Tools.hpp"
#include "core/common.h"
#include "ui/utils/FilePickerHelper.hpp"

#include <miniz.h>

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <sstream>

#ifdef __SWITCH__
#include <sys/stat.h>
#endif

namespace fs = std::filesystem;

namespace
{
    bool ensureDirectory(const fs::path& directory, std::string* error = nullptr)
    {
        if (directory.empty())
            return true;

#ifdef __SWITCH__
        const std::string path = directory.lexically_normal().generic_string();
        size_t componentStart = 0;
        const size_t deviceSeparator = path.find(":/");
        if (deviceSeparator != std::string::npos)
            componentStart = deviceSeparator + 2;
        else if (!path.empty() && path.front() == '/')
            componentStart = 1;

        size_t separator = componentStart;
        while (separator <= path.size()) {
            separator = path.find('/', separator);
            const std::string current = separator == std::string::npos
                ? path : path.substr(0, separator);
            if (current.size() > componentStart &&
                mkdir(current.c_str(), 0777) != 0 && errno != EEXIST) {
                if (error) {
                    *error = "创建目录失败: " + current + " (" +
                             std::strerror(errno) + ")";
                }
                return false;
            }
            if (separator == std::string::npos)
                break;
            ++separator;
        }
        return true;
#else
        std::error_code ec;
        fs::create_directories(directory, ec);
        if (ec) {
            if (error) {
                *error = "创建目录失败: " + directory.string() +
                         " (" + ec.message() + ")";
            }
            return false;
        }
        return true;
#endif
    }

    bool copyBinaryFile(const fs::path& source, const fs::path& target,
                        std::string* error = nullptr)
    {
        const fs::path parent = target.parent_path();
        if (!ensureDirectory(parent, error))
            return false;
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

    bool directoryContainsFiles(const fs::path& directory)
    {
        if (directory.empty())
            return false;
        std::error_code ec;
        for (fs::recursive_directory_iterator it(directory, ec), end;
             !ec && it != end; it.increment(ec)) {
            if (it->is_regular_file(ec) && !ec)
                return true;
            ec.clear();
        }
        return false;
    }

    std::size_t countRegularFiles(const fs::path& directory)
    {
        if (directory.empty())
            return 0;
        std::size_t count = 0;
        std::error_code ec;
        fs::recursive_directory_iterator iterator(
            directory, fs::directory_options::skip_permission_denied, ec);
        const fs::recursive_directory_iterator end;
        while (!ec && iterator != end) {
            if (iterator->is_regular_file(ec) && !ec)
                ++count;
            ec.clear();
            iterator.increment(ec);
        }
        return ec ? 0 : count;
    }

    bool pathExists(const fs::path& path)
    {
        if (path.empty())
            return false;
        std::error_code ec;
        return fs::exists(path, ec) && !ec;
    }

    bool clearDirectoryContents(const fs::path& directory, std::string* error = nullptr)
    {
        if (error) error->clear();
        if (directory.empty()) {
            if (error) *error = "save directory is empty";
            return false;
        }

        std::error_code ec;
        const bool exists = fs::exists(directory, ec);
        if (ec) {
            if (error) *error = ec.message();
            return false;
        }
        if (!exists)
            return true;
        if (!fs::is_directory(directory, ec) || ec) {
            if (error) *error = ec ? ec.message() : "save path is not a directory";
            return false;
        }

        std::vector<fs::path> children;
        fs::directory_iterator iterator(directory, ec);
        const fs::directory_iterator end;
        while (!ec && iterator != end) {
            children.push_back(iterator->path());
            iterator.increment(ec);
        }
        if (ec) {
            if (error) *error = ec.message();
            return false;
        }

        bool success = true;
        for (const auto& child : children) {
            std::error_code removeError;
            fs::remove_all(child, removeError);
            if (removeError) {
                success = false;
                if (error && error->empty())
                    *error = removeError.message();
            }
        }
        return success;
    }

    std::string formatByteSize(std::uint64_t bytes)
    {
        static constexpr const char* units[] = {"B", "KiB", "MiB", "GiB"};
        double value = static_cast<double>(bytes);
        int unit = 0;
        while (value >= 1024.0 && unit < 3) {
            value /= 1024.0;
            ++unit;
        }
        std::ostringstream stream;
        stream << std::fixed << std::setprecision(unit == 0 ? 0 : 2)
               << value << ' ' << units[unit];
        return stream.str();
    }

    std::string zipError(const mz_zip_archive& archive)
    {
        const char* message = mz_zip_get_error_string(mz_zip_get_last_error(
            const_cast<mz_zip_archive*>(&archive)));
        return message ? message : "zip operation failed";
    }

    struct ArchiveInputStream
    {
        std::ifstream stream;
        mz_uint64 offset{};
        bool failed{};
    };

    size_t readArchiveInput(void* opaque, mz_uint64 offset, void* buffer, size_t size)
    {
        auto* input = static_cast<ArchiveInputStream*>(opaque);
        if (!input || !buffer || input->failed)
            return 0;

        if (input->offset != offset) {
            input->stream.clear();
            input->stream.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
            if (!input->stream) {
                input->failed = true;
                return 0;
            }
        }

        input->stream.read(static_cast<char*>(buffer), static_cast<std::streamsize>(size));
        const size_t bytesRead = static_cast<size_t>(input->stream.gcount());
        input->offset = offset + bytesRead;
        if (input->stream.bad())
            input->failed = true;
        return bytesRead;
    }

    bool createDirectoryArchive(const fs::path& source, const fs::path& target,
                                std::string* error)
    {
        if (error) error->clear();
        if (source.empty() || target.empty()) {
            if (error) *error = "save or archive path is empty";
            return false;
        }
        if (!directoryContainsFiles(source)) {
            if (error) *error = "save directory is empty";
            return false;
        }

        if (!ensureDirectory(target.parent_path(), error))
            return false;

        std::error_code ec;

        mz_zip_archive archive{};
        if (!mz_zip_writer_init_file_v2(
                &archive, target.string().c_str(), 0, MZ_ZIP_FLAG_WRITE_ZIP64)) {
            if (error) *error = zipError(archive);
            return false;
        }

        bool success = true;
        for (fs::recursive_directory_iterator it(source, ec), end;
             success && !ec && it != end; it.increment(ec)) {
            if (!it->is_regular_file(ec)) {
                ec.clear();
                continue;
            }
            const fs::path relative = it->path().lexically_relative(source);
            bool validRelative = !relative.empty() && relative != "." &&
                                 !relative.is_absolute();
            for (const auto& component : relative) {
                if (component == "..") {
                    validRelative = false;
                    break;
                }
            }
            if (!validRelative) {
                if (error) {
                    *error = "无法计算存档相对路径: " + it->path().generic_string();
                }
                success = false;
                break;
            }
            const std::string archiveName = (fs::path("data") / relative).generic_string();
            std::error_code sizeError;
            const auto fileSize = fs::file_size(it->path(), sizeError);
            if (sizeError) {
                if (error) {
                    *error = "读取文件大小失败: " + relative.generic_string() +
                             " (" + sizeError.message() + ")";
                }
                success = false;
                break;
            }

            ArchiveInputStream input;
            input.stream.open(it->path(), std::ios::binary);
            if (!input.stream) {
                if (error)
                    *error = "打开存档文件失败: " + relative.generic_string();
                success = false;
                break;
            }

            success = mz_zip_writer_add_read_buf_callback(
                &archive, archiveName.c_str(), readArchiveInput, &input,
                static_cast<mz_uint64>(fileSize), nullptr, nullptr, 0,
                MZ_BEST_SPEED, nullptr, 0, nullptr, 0) != 0;
            if (!success && error) {
                *error = std::string(input.failed ? "读取存档文件失败: " : "压缩存档文件失败: ") +
                         relative.generic_string() + " (" + zipError(archive) + ")";
            }
        }
        if (ec)
            success = false;
        if (success)
            success = mz_zip_writer_finalize_archive(&archive) != 0;
        if (!success && error && error->empty())
            *error = ec ? ec.message() : zipError(archive);
        const bool ended = mz_zip_writer_end(&archive) != 0;
        if (success && !ended && error)
            *error = zipError(archive);
        success = success && ended;
        if (!success) {
            std::error_code removeError;
            fs::remove(target, removeError);
        }
        return success;
    }

    bool safeArchiveRelativePath(std::string name, fs::path& output)
    {
        std::replace(name.begin(), name.end(), '\\', '/');
        while (name.rfind("data/", 0) == 0)
            name.erase(0, 5);
        if (name.empty() || name.front() == '/' || name.find(':') != std::string::npos)
            return false;

        const fs::path relative(name);
        if (relative.is_absolute())
            return false;
        for (const auto& component : relative) {
            if (component == "..")
                return false;
        }
        output = relative.lexically_normal();
        return !output.empty() && output != ".";
    }

    bool replaceDirectoryFromArchive(const fs::path& archivePath, const fs::path& target,
                                     std::string* error)
    {
        if (error) error->clear();
        if (archivePath.empty() || target.empty()) {
            if (error) *error = "save or archive path is empty";
            return false;
        }
        mz_zip_archive archive{};
        if (!mz_zip_reader_init_file(&archive, archivePath.string().c_str(), 0)) {
            if (error) *error = zipError(archive);
            return false;
        }

        struct ArchiveFile {
            mz_uint index{};
            fs::path relative;
        };
        std::vector<ArchiveFile> files;
        constexpr mz_uint64 MaxArchiveOutputSize = 1024ULL * 1024ULL * 1024ULL;
        constexpr mz_uint MaxArchiveFileCount = 100000;
        mz_uint64 outputSize = 0;
        bool success = true;
        const mz_uint count = mz_zip_reader_get_num_files(&archive);
        if (count > MaxArchiveFileCount) {
            success = false;
            if (error) *error = "archive contains too many files";
        }
        for (mz_uint index = 0; success && index < count; ++index) {
            mz_zip_archive_file_stat stat{};
            if (!mz_zip_reader_file_stat(&archive, index, &stat)) {
                success = false;
                break;
            }
            if (mz_zip_reader_is_file_a_directory(&archive, index))
                continue;
            if (stat.m_uncomp_size > MaxArchiveOutputSize - outputSize) {
                success = false;
                if (error) *error = "archive is too large";
                break;
            }
            outputSize += stat.m_uncomp_size;
            fs::path relative;
            if (!safeArchiveRelativePath(stat.m_filename, relative)) {
                success = false;
                if (error) *error = "archive contains an unsafe path";
                break;
            }
            files.push_back({index, std::move(relative)});
        }
        if (files.empty()) {
            success = false;
            if (error && error->empty()) *error = "archive contains no save files";
        }

        const fs::path temporary = target.string() + ".import_tmp";
        const fs::path previous = target.string() + ".import_previous_" + timestampForFile();
        std::error_code ec;
        fs::remove_all(temporary, ec);
        ec.clear();
        if (fs::exists(previous, ec) || ec) {
            success = false;
            if (error && error->empty()) *error = ec ? ec.message() : "recovery directory exists";
        }
        ec.clear();
        if (success) {
            success = ensureDirectory(temporary, error);
        }
        for (const auto& file : files) {
            if (!success)
                break;
            const fs::path destination = temporary / file.relative;
            if (!ensureDirectory(destination.parent_path(), error) ||
                !mz_zip_reader_extract_to_file(
                           &archive, file.index, destination.string().c_str(), 0)) {
                success = false;
                break;
            }
        }
        if (!success && error && error->empty())
            *error = ec ? ec.message() : zipError(archive);
        mz_zip_reader_end(&archive);

        if (success) {
            ec.clear();
            if (fs::exists(target, ec) && !ec)
                fs::rename(target, previous, ec);
            if (!ec)
                fs::rename(temporary, target, ec);
            if (ec) {
                std::error_code rollbackError;
                if (!fs::exists(target, rollbackError) && fs::exists(previous, rollbackError))
                    fs::rename(previous, target, rollbackError);
                success = false;
                if (error) *error = ec.message();
            }
        }

        ec.clear();
        fs::remove_all(temporary, ec);
        if (success) {
            ec.clear();
            fs::remove_all(previous, ec);
        }
        return success;
    }

    std::string trimText(std::string value)
    {
        const auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
        value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                                                [&](unsigned char c) { return !isSpace(c); }));
        value.erase(std::find_if(value.rbegin(), value.rend(),
                                 [&](unsigned char c) { return !isSpace(c); }).base(),
                    value.end());
        return value;
    }

    std::string sanitizeCheatName(std::string value)
    {
        value = trimText(std::move(value));
        value.erase(std::remove_if(value.begin(), value.end(), [](char c) {
            return c == '[' || c == ']' || c == '\r' || c == '\n' || c == '\0';
        }), value.end());
        return trimText(std::move(value));
    }

    std::string normalizeGatewayCode(const std::string& input)
    {
        std::vector<std::string> words;
        std::string word;
        const auto flushWord = [&]() {
            if (word.empty())
                return true;
            if (word.size() % 8 != 0)
                return false;
            for (size_t offset = 0; offset < word.size(); offset += 8) {
                std::string value = word.substr(offset, 8);
                std::transform(value.begin(), value.end(), value.begin(),
                    [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
                words.push_back(std::move(value));
            }
            word.clear();
            return true;
        };

        for (const unsigned char c : input) {
            if (std::isxdigit(c)) {
                word.push_back(static_cast<char>(c));
            } else if (!flushWord()) {
                return {};
            }
        }
        if (!flushWord() || words.empty() || words.size() % 2 != 0)
            return {};

        std::string output;
        for (size_t index = 0; index < words.size(); index += 2) {
            if (!output.empty())
                output.push_back('\n');
            output += words[index] + " " + words[index + 1];
        }
        return output;
    }

    std::vector<beiklive::GameDataView::CheatItem> loadGatewayCheats(const fs::path& path)
    {
        std::vector<beiklive::GameDataView::CheatItem> cheats;
        std::ifstream input(path);
        if (!input)
            return cheats;

        beiklive::GameDataView::CheatItem current;
        const auto finishCurrent = [&]() {
            current.name = sanitizeCheatName(std::move(current.name));
            current.code = trimText(std::move(current.code));
            current.comments = trimText(std::move(current.comments));
            if (!current.code.empty()) {
                if (current.name.empty())
                    current.name = "未命名金手指";
                cheats.push_back(std::move(current));
            }
            current = {};
        };

        std::string line;
        while (std::getline(input, line)) {
            line.erase(std::remove(line.begin(), line.end(), '\0'), line.end());
            line = trimText(std::move(line));
            if (line.size() >= 2 && line.front() == '[' && line.back() == ']') {
                finishCurrent();
                current.name = line.substr(1, line.size() - 2);
            } else if (line == "*citra_enabled") {
                current.enabled = true;
            } else if (!line.empty() && line.front() == '*') {
                if (!current.comments.empty())
                    current.comments.push_back('\n');
                current.comments += line.substr(1);
            } else if (!line.empty()) {
                if (!current.code.empty())
                    current.code.push_back('\n');
                current.code += line;
            }
        }
        finishCurrent();
        return cheats;
    }

    bool saveGatewayCheats(const fs::path& path,
                           const std::vector<beiklive::GameDataView::CheatItem>& cheats)
    {
        if (!ensureDirectory(path.parent_path()))
            return false;

        std::error_code ec;

        const fs::path temporary = path.string() + ".tmp";
        const fs::path previous = path.string() + ".previous";
        std::ofstream output(temporary, std::ios::trunc);
        if (!output)
            return false;
        for (const auto& cheat : cheats) {
            output << '[' << sanitizeCheatName(cheat.name) << "]\n";
            if (cheat.enabled)
                output << "*citra_enabled\n";
            std::istringstream comments(cheat.comments);
            std::string comment;
            while (std::getline(comments, comment))
                output << '*' << comment << '\n';
            output << trimText(cheat.code) << "\n\n";
        }
        output.flush();
        if (!output)
            return false;
        output.close();

        fs::remove(previous, ec);
        ec.clear();
        const bool hadPrevious = fs::exists(path, ec);
        if (ec)
            return false;
        if (hadPrevious)
            fs::rename(path, previous, ec);
        if (ec)
            return false;
        fs::rename(temporary, path, ec);
        if (ec) {
            std::error_code rollback;
            if (hadPrevious)
                fs::rename(previous, path, rollback);
            return false;
        }
        fs::remove(previous, ec);
        return true;
    }
}

namespace beiklive
{
    GameDataPage::GameDataPage(beiklive::GameEntry entry)
        : m_entry(std::move(entry))
    {
        if (_isThreeDs()) {
            const std::string titleId = _threeDsTitleId();
            if (!titleId.empty() && titleId != m_entry.threeDsTitleId) {
                m_entry.threeDsTitleId = titleId;
                if (beiklive::GameDB) {
                    beiklive::GameDB->set(m_entry.path, "3ds_titleid", nlohmann::json(titleId));
                    beiklive::GameDB->flush();
                }
            }
        }
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
        ensureDirectory(directory);
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
        const bool isGenesis =
            m_entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGenesis);
        return (fs::path(_saveDir()) / (gameStem(m_entry) + (isGenesis ? ".srm" : ".sav"))).string();
    }

    bool GameDataPage::_isThreeDs() const
    {
        return m_entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::Emu3DS);
    }

    std::string GameDataPage::_threeDsTitleId() const
    {
        return beiklive::three_ds::resolveTitleId(m_entry.threeDsTitleId, m_entry.path);
    }

    std::string GameDataPage::_batterySaveDir() const
    {
        if (!_isThreeDs())
            return _saveDir();
        return beiklive::three_ds::saveDataPath(_threeDsTitleId());
    }

    void GameDataPage::_initView()
    {
        m_view = new beiklive::GameDataView(m_entry);
        m_view->setGrow(1.f);
        m_view->onBack = [this]() { _closeAnimated(); };
        m_view->onSectionChanged = [this](GameDataView::Section section) {
            if (section == GameDataView::Section::STATES) _refreshStateList();
            else if (section == GameDataView::Section::SCREENSHOTS) _refreshScreenshotList();
            else if (section == GameDataView::Section::BATTERY) _refreshBackupList();
            else if (section == GameDataView::Section::CHEATS) _refreshCheats();
            else _refreshManagedContent();
        };
        if (!_isThreeDs()) {
            m_view->onDeleteState = [this](int slot) { _confirmDeleteState(slot); };
            m_view->onDeleteScreenshot = [this](int index) { _confirmDeleteScreenshot(index); };
            m_view->onSetScreenshotCover =
                [this](int index) { _confirmSetScreenshotAsCover(index); };
        }
        m_view->onExportSave = [this]() { _exportSav(); };
        m_view->onImportSave = [this]() { _importSav(); };
        m_view->onDeleteSave = [this]() { _confirmDeleteSav(); };
        m_view->onBackupSave = [this]() { _backupSav(); };
        m_view->onClearShaderCache = [this]() { _confirmClearShaderCache(); };
        m_view->onRestoreBackup = [this](int index) { _confirmRestoreBackup(index); };
        m_view->onDeleteBackup = [this](int index) { _confirmDeleteBackup(index); };
        m_view->onAddCheat = [this]() { _addCheat(); };
        m_view->onCheatOptions = [this](int index) { _showCheatOptions(index); };
        m_view->onToggleManagedContent = [this](GameDataView::Section section, int index) {
            _confirmToggleManagedContent(section, index);
        };
        m_view->onDeleteManagedContent = [this](GameDataView::Section section, int index) {
            _confirmDeleteManagedContent(section, index);
        };
        getContentBox()->addView(m_view);

        if (!_isThreeDs()) {
            _refreshStateList();
            _refreshScreenshotList();
        }
        _refreshBackupList();
        if (_isThreeDs()) {
            _refreshCheats();
            _refreshManagedContent();
        }
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
        std::error_code ec;
        bool saveExists = false;
        if (_isThreeDs()) {
            const std::string titleId = _threeDsTitleId();
            if (titleId.empty()) {
                m_view->setBackups({}, false);
                return;
            }
            const std::string backupDir = beiklive::three_ds::backupDirectory(titleId);
            for (const auto& item : fs::directory_iterator(backupDir, ec)) {
                if (ec || !item.is_regular_file()) continue;
                std::string extension = item.path().extension().string();
                std::transform(extension.begin(), extension.end(), extension.begin(),
                    [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
                if (extension != ".zip") continue;
                std::error_code sizeError;
                if (fs::file_size(item.path(), sizeError) == 0 || sizeError) continue;
                m_backupPaths.push_back(item.path());
            }
            saveExists = directoryContainsFiles(_batterySaveDir());
        } else {
            const fs::path save = _savPath();
            const std::string prefix = save.filename().string() + ".bak_";
            for (const auto& item : fs::directory_iterator(_saveDir(), ec)) {
                if (ec || !item.is_regular_file()) continue;
                const std::string name = item.path().filename().string();
                if (name.rfind(prefix, 0) != 0) continue;
                std::error_code sizeError;
                if (fs::file_size(item.path(), sizeError) == 0 || sizeError) continue;
                m_backupPaths.push_back(item.path());
            }
            ec.clear();
            saveExists = fs::exists(save, ec) && !ec;
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
        m_view->setBackups(std::move(items), saveExists);
    }

    void GameDataPage::_refreshCheats()
    {
        if (!m_view || !_isThreeDs())
            return;
        const std::string path = beiklive::three_ds::cheatFilePath(_threeDsTitleId());
        m_cheats = path.empty() ? std::vector<GameDataView::CheatItem>{}
                                : loadGatewayCheats(path);
        m_view->setCheats(m_cheats);
    }

    void GameDataPage::_refreshManagedContent()
    {
        if (!m_view || !_isThreeDs())
            return;
        const std::string titleId = _threeDsTitleId();
        const auto makeItem = [](std::string label, std::string emptyText,
                                 std::string enabledPath, std::string disabledPath) {
            GameDataView::ManagedContentItem item;
            item.label = std::move(label);
            item.emptyText = std::move(emptyText);
            item.enabledPath = std::move(enabledPath);
            item.disabledPath = std::move(disabledPath);
            item.enabledFileCount = countRegularFiles(item.enabledPath);
            item.disabledFileCount = countRegularFiles(item.disabledPath);
            item.enabledExists = pathExists(item.enabledPath);
            item.disabledExists = pathExists(item.disabledPath);
            return item;
        };

        m_view->setLoadContent(
            makeItem("纹理", "未导入",
                     beiklive::three_ds::texturePath(titleId),
                     beiklive::three_ds::disabledTexturePath(titleId)),
            makeItem("MOD", "未导入",
                     beiklive::three_ds::modPath(titleId),
                     beiklive::three_ds::disabledModPath(titleId)));
        m_view->setAddons(
            makeItem("更新", "未安装",
                     beiklive::three_ds::updateTitlePath(titleId),
                     beiklive::three_ds::disabledUpdateTitlePath(titleId)),
            makeItem("DLC", "未安装",
                     beiklive::three_ds::dlcTitlePath(titleId),
                     beiklive::three_ds::disabledDlcTitlePath(titleId)));
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
        if (_isThreeDs()) {
            const std::string titleId = _threeDsTitleId();
            const fs::path source = _batterySaveDir();
            if (titleId.empty()) {
                brls::Application::notify("缺少3DS Title ID，无法定位存档");
                return;
            }
            if (!directoryContainsFiles(source)) {
                brls::Application::notify("未找到3DS游戏存档");
                return;
            }
            auto* dialog = new brls::Dialog("确认导出当前3DS游戏存档为压缩包？");
            dialog->addButton("取消", []() {});
            dialog->addButton("导出", [source, titleId]() {
                const fs::path target = fs::path(beiklive::three_ds::exportDirectory()) /
                    (titleId + "_" + timestampForFile() + ".zip");
                std::string error;
                if (!createDirectoryArchive(source, target, &error)) {
                    brls::Logger::warning("导出3DS存档失败: {} -> {}, error={}",
                        source.string(), target.string(), error);
                    brls::Application::notify("导出失败：" + error);
                    return;
                }
                brls::Application::notify("已导出到 GBAStation/export/3DS");
            });
            dialog->open();
            return;
        }

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
        if (_isThreeDs()) {
            if (_threeDsTitleId().empty()) {
                brls::Application::notify("缺少3DS Title ID，无法定位存档");
                return;
            }
            auto* dialog = new brls::Dialog("确认导入3DS存档压缩包并覆盖当前存档？");
            dialog->addButton("取消", []() {});
            const auto alive = m_alive;
            dialog->addButton("选择文件", [this, alive]() {
                beiklive::openFilePicker({"zip"}, [this, alive](const std::string& selected) {
                    if (!alive->load()) return;
                    std::string error;
                    if (!replaceDirectoryFromArchive(selected, _batterySaveDir(), &error)) {
                        brls::Logger::warning("导入3DS存档失败: {}", error);
                        brls::Application::notify("导入失败");
                        return;
                    }
                    brls::Application::notify("已导入3DS存档");
                    _refreshBackupList();
                    m_view->restoreFocus();
                }, beiklive::path::GetRootPath());
            });
            dialog->open();
            return;
        }

        const bool isGenesis =
            m_entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGenesis);
        const std::string extension = isGenesis ? "srm" : "sav";
        auto* dialog = new brls::Dialog(
            "确认导入外部 ." + extension + " 并覆盖当前电池存档？");
        dialog->addButton("取消", []() {});
        const auto alive = m_alive;
        dialog->addButton("选择文件", [this, alive, extension]() {
            beiklive::openFilePicker({extension}, [this, alive](const std::string& selected) {
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
        if (_isThreeDs()) {
            const std::string titleId = _threeDsTitleId();
            const fs::path source = _batterySaveDir();
            if (titleId.empty()) {
                brls::Application::notify("缺少3DS Title ID，无法定位存档");
                return;
            }
            if (!directoryContainsFiles(source)) {
                brls::Application::notify("未找到3DS游戏存档");
                return;
            }
            auto* dialog = new brls::Dialog("确认为当前3DS游戏存档创建备份？");
            dialog->addButton("取消", []() {});
            dialog->addButton("备份", [this, source, titleId]() {
                const fs::path backup = fs::path(beiklive::three_ds::backupDirectory(titleId)) /
                    (titleId + "_" + timestampForFile() + ".zip");
                std::string error;
                if (!createDirectoryArchive(source, backup, &error)) {
                    brls::Logger::warning("备份3DS存档失败: {} -> {}, error={}",
                        source.string(), backup.string(), error);
                    brls::Application::notify("备份失败：" + error);
                    return;
                }
                brls::Application::notify("已创建3DS存档备份");
                _refreshBackupList();
                m_view->restoreFocus();
            });
            dialog->open();
            return;
        }

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
            const bool restored = _isThreeDs()
                ? replaceDirectoryFromArchive(backup, _batterySaveDir(), &error)
                : copyBinaryFile(backup, _savPath(), &error);
            if (!restored) {
                brls::Logger::warning("还原存档失败: {}", error);
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

    void GameDataPage::_confirmDeleteSav()
    {
        const bool isThreeDs = _isThreeDs();
        const fs::path savePath = isThreeDs ? fs::path(_batterySaveDir()) : fs::path(_savPath());
        if (savePath.empty()) {
            brls::Application::notify("无法定位游戏存档");
            return;
        }

        const bool saveExists = isThreeDs ? directoryContainsFiles(savePath) : [&]() {
            std::error_code ec;
            return fs::is_regular_file(savePath, ec) && !ec;
        }();
        if (!saveExists) {
            brls::Application::notify(isThreeDs ? "未找到3DS游戏存档" : "未找到电池存档");
            return;
        }

        auto* dialog = new brls::Dialog(
            isThreeDs
                ? "确认删除该游戏存档目录中的所有文件？\n此操作不可撤销，备份文件不会被删除。"
                : (m_entry.platform == static_cast<int>(beiklive::enums::EmuPlatform::EmuGenesis)
                       ? "确认删除该游戏的 .srm 存档？\n此操作不可撤销，备份文件不会被删除。"
                       : "确认删除该游戏的 .sav 存档？\n此操作不可撤销，备份文件不会被删除。"));
        dialog->addButton("取消", [this]() { m_view->restoreFocus(); });
        dialog->addButton("删除", [this, isThreeDs, savePath]() {
            std::string error;
            bool success = false;
            if (isThreeDs) {
                success = clearDirectoryContents(savePath, &error);
            } else {
                std::error_code ec;
                success = fs::remove(savePath, ec);
                if (ec) error = ec.message();
            }
            if (!success && !error.empty())
                brls::Logger::warning("删除游戏存档失败: path={} error={}",
                                      savePath.string(), error);
            brls::Application::notify(success ? "已删除游戏存档" : "删除游戏存档失败");
            _refreshBackupList();
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_confirmClearShaderCache()
    {
        const std::string titleId = _threeDsTitleId();
        if (titleId.empty()) {
            brls::Application::notify("缺少3DS Title ID，无法清理缓存");
            return;
        }
        const auto stats = beiklive::three_ds::shaderCacheStats(titleId);
        if (!stats.valid) {
            brls::Application::notify("读取着色器缓存信息失败");
            return;
        }
        auto* dialog = new brls::Dialog(
            "确认清除该游戏的着色器缓存？\nTitle ID: " + titleId +
            "\n缓存文件：" + std::to_string(stats.fileCount) +
            " 个\n占用空间：" + formatByteSize(stats.totalBytes) +
            "\n下次启动游戏时将重新编译着色器。");
        dialog->addButton("取消", []() {});
        dialog->addButton("清除", [this, titleId]() {
            const bool success = beiklive::three_ds::clearShaderCache(titleId);
            brls::Application::notify(success ? "已清除着色器缓存" : "清除着色器缓存失败");
            m_view->restoreFocus();
        });
        dialog->open();
    }

    bool GameDataPage::_saveCheats()
    {
        const std::string path = beiklive::three_ds::cheatFilePath(_threeDsTitleId());
        if (path.empty() || !saveGatewayCheats(path, m_cheats)) {
            brls::Application::notify("保存金手指失败");
            return false;
        }
        return true;
    }

    void GameDataPage::_addCheat()
    {
        const fs::path cheatPath =
            beiklive::three_ds::cheatFilePath(_threeDsTitleId());
        std::string directoryError;
        if (cheatPath.empty() ||
            !ensureDirectory(cheatPath.parent_path(), &directoryError)) {
            brls::Logger::warning("创建金手指目录失败: path={} error={}",
                                  cheatPath.string(), directoryError);
            brls::Application::notify("创建金手指文件失败");
            return;
        }
        std::error_code existsError;
        const bool cheatFileExists = fs::exists(cheatPath, existsError);
        if (existsError) {
            brls::Logger::warning("检查金手指文件失败: path={} error={}",
                                  cheatPath.string(), existsError.message());
            brls::Application::notify("创建金手指文件失败");
            return;
        }
        if (!cheatFileExists) {
            std::ofstream file(cheatPath, std::ios::app);
            if (!file) {
                brls::Logger::warning("创建金手指文件失败: path={}",
                                      cheatPath.string());
                brls::Application::notify("创建金手指文件失败");
                return;
            }
        }
        auto* ime = brls::Application::getPlatform()->getImeManager();
        if (!ime) {
            brls::Application::notify("输入法不可用");
            return;
        }
        const auto alive = m_alive;
        ime->openForText(
            [this, alive](std::string text) {
                if (!alive->load()) return;
                const std::string name = sanitizeCheatName(std::move(text));
                if (name.empty()) {
                    m_view->restoreFocus();
                    return;
                }
                auto* codeIme = brls::Application::getPlatform()->getImeManager();
                if (!codeIme) {
                    brls::Application::notify("输入法不可用");
                    m_view->restoreFocus();
                    return;
                }
                codeIme->openForText(
                    [this, alive, name](std::string codeText) {
                        if (!alive->load()) return;
                        const std::string code = normalizeGatewayCode(codeText);
                        if (code.empty()) {
                            if (!codeText.empty())
                                brls::Application::notify(
                                    "金手指代码无效，请使用XXXXXXXX XXXXXXXX格式");
                            m_view->restoreFocus();
                            return;
                        }
                        m_cheats.push_back({name, code, {}, false});
                        if (_saveCheats()) {
                            brls::Application::notify("已新增金手指");
                            _refreshCheats();
                        } else {
                            m_cheats.pop_back();
                        }
                        m_view->restoreFocus();
                    },
                    "输入金手指内容", "每行格式：XXXXXXXX XXXXXXXX", 4096, "",
                    brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
            },
            "输入金手指名称", "", 128, "",
            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
    }

    void GameDataPage::_showCheatOptions(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_cheats.size()))
            return;
        auto* dialog = new brls::Dialog("管理金手指\n" + m_cheats[static_cast<size_t>(index)].name);
        dialog->addButton("修改金手指名称", [this, index]() { _editCheatName(index); });
        dialog->addButton("修改金手指代码", [this, index]() { _editCheatCode(index); });
        dialog->addButton("删除金手指", [this, index]() { _confirmDeleteCheat(index); });
        dialog->addButton("取消", [this]() { m_view->restoreFocus(); });
        dialog->open();
    }

    void GameDataPage::_editCheatName(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_cheats.size()))
            return;
        auto* ime = brls::Application::getPlatform()->getImeManager();
        if (!ime) return;
        const auto alive = m_alive;
        const std::string current = m_cheats[static_cast<size_t>(index)].name;
        ime->openForText(
            [this, alive, index](std::string text) {
                if (!alive->load() || index < 0 || index >= static_cast<int>(m_cheats.size()))
                    return;
                const std::string name = sanitizeCheatName(std::move(text));
                if (!name.empty()) {
                    m_cheats[static_cast<size_t>(index)].name = name;
                    if (_saveCheats()) {
                        brls::Application::notify("已修改金手指名称");
                        _refreshCheats();
                    }
                }
                m_view->restoreFocus();
            },
            "修改金手指名称", "", 128, current,
            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
    }

    void GameDataPage::_editCheatCode(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_cheats.size()))
            return;
        auto* ime = brls::Application::getPlatform()->getImeManager();
        if (!ime) return;
        const auto alive = m_alive;
        const std::string current = m_cheats[static_cast<size_t>(index)].code;
        ime->openForText(
            [this, alive, index](std::string text) {
                if (!alive->load() || index < 0 || index >= static_cast<int>(m_cheats.size()))
                    return;
                const std::string code = normalizeGatewayCode(text);
                if (code.empty()) {
                    if (!text.empty())
                        brls::Application::notify(
                            "金手指代码无效，请使用XXXXXXXX XXXXXXXX格式");
                    m_view->restoreFocus();
                    return;
                }
                m_cheats[static_cast<size_t>(index)].code = code;
                if (_saveCheats()) {
                    brls::Application::notify("已修改金手指代码");
                    _refreshCheats();
                }
                m_view->restoreFocus();
            },
            "修改金手指代码", "每行格式：XXXXXXXX XXXXXXXX", 4096, current,
            brls::KeyboardKeyDisableBitmask::KEYBOARD_DISABLE_NONE);
    }

    void GameDataPage::_confirmDeleteCheat(int index)
    {
        if (index < 0 || index >= static_cast<int>(m_cheats.size()))
            return;
        const std::string name = m_cheats[static_cast<size_t>(index)].name;
        auto* dialog = new brls::Dialog("确认删除金手指\n" + name + "？");
        dialog->addButton("取消", [this]() { m_view->restoreFocus(); });
        dialog->addButton("删除", [this, index]() {
            if (index < 0 || index >= static_cast<int>(m_cheats.size())) return;
            m_cheats.erase(m_cheats.begin() + index);
            if (_saveCheats()) {
                brls::Application::notify("已删除金手指");
                _refreshCheats();
            }
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_confirmToggleManagedContent(GameDataView::Section section, int index)
    {
        if (index < 0 || index > 1)
            return;
        const std::string titleId = _threeDsTitleId();
        std::string label;
        std::string enabledPath;
        std::string disabledPath;
        if (section == GameDataView::Section::LOAD_CONTENT) {
            label = index == 0 ? "纹理" : "MOD";
            enabledPath = index == 0 ? beiklive::three_ds::texturePath(titleId)
                                     : beiklive::three_ds::modPath(titleId);
            disabledPath = index == 0 ? beiklive::three_ds::disabledTexturePath(titleId)
                                      : beiklive::three_ds::disabledModPath(titleId);
        } else if (section == GameDataView::Section::ADDONS) {
            label = index == 0 ? "更新" : "DLC";
            enabledPath = index == 0 ? beiklive::three_ds::updateTitlePath(titleId)
                                     : beiklive::three_ds::dlcTitlePath(titleId);
            disabledPath = index == 0 ? beiklive::three_ds::disabledUpdateTitlePath(titleId)
                                      : beiklive::three_ds::disabledDlcTitlePath(titleId);
        } else {
            return;
        }

        const bool enabledExists = pathExists(enabledPath);
        const bool disabledExists = pathExists(disabledPath);
        if (enabledExists && disabledExists) {
            brls::Application::notify(label + "同时存在启用和停用目录，请先手动处理冲突");
            return;
        }
        if (!enabledExists && !disabledExists) {
            brls::Application::notify(label + (section == GameDataView::Section::ADDONS
                ? "未安装" : "未导入"));
            return;
        }

        const bool enable = disabledExists;
        const std::string action = enable ? "启用" : "停用";
        const std::string source = enable ? disabledPath : enabledPath;
        auto* dialog = new brls::Dialog(
            "确认" + action + label + "？\n当前路径：\n" + source);
        dialog->addButton("取消", [this]() { m_view->restoreFocus(); });
        dialog->addButton(action, [this, enabledPath, disabledPath, enable, action, label]() {
            const bool success = beiklive::three_ds::setManagedContentEnabled(
                enabledPath, disabledPath, enable);
            brls::Application::notify(success ? "已" + action + label : action + label + "失败");
            _refreshManagedContent();
            m_view->restoreFocus();
        });
        dialog->open();
    }

    void GameDataPage::_confirmDeleteManagedContent(GameDataView::Section section, int index)
    {
        if (index < 0 || index > 1)
            return;
        const std::string titleId = _threeDsTitleId();
        std::string label;
        std::string enabledPath;
        std::string disabledPath;
        if (section == GameDataView::Section::LOAD_CONTENT) {
            label = index == 0 ? "纹理" : "MOD";
            enabledPath = index == 0 ? beiklive::three_ds::texturePath(titleId)
                                     : beiklive::three_ds::modPath(titleId);
            disabledPath = index == 0 ? beiklive::three_ds::disabledTexturePath(titleId)
                                      : beiklive::three_ds::disabledModPath(titleId);
        } else if (section == GameDataView::Section::ADDONS) {
            label = index == 0 ? "更新" : "DLC";
            enabledPath = index == 0 ? beiklive::three_ds::updateTitlePath(titleId)
                                     : beiklive::three_ds::dlcTitlePath(titleId);
            disabledPath = index == 0 ? beiklive::three_ds::disabledUpdateTitlePath(titleId)
                                      : beiklive::three_ds::disabledDlcTitlePath(titleId);
        } else {
            return;
        }

        const bool present = pathExists(enabledPath) || pathExists(disabledPath);
        if (!present) {
            brls::Application::notify(label + (section == GameDataView::Section::ADDONS
                ? "未安装" : "未导入"));
            return;
        }
        auto* dialog = new brls::Dialog(
            "确认永久删除" + label + "？\n此操作不可撤销。\n" + enabledPath);
        dialog->addButton("取消", [this]() { m_view->restoreFocus(); });
        dialog->addButton("删除", [this, enabledPath, disabledPath, label]() {
            const bool success = beiklive::three_ds::deleteManagedContent(
                enabledPath, disabledPath);
            brls::Application::notify(success ? "已删除" + label : "删除" + label + "失败");
            _refreshManagedContent();
            m_view->restoreFocus();
        });
        dialog->open();
    }
}
