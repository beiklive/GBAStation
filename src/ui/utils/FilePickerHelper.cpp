#include "FilePickerHelper.hpp"
#include "core/Tools.hpp"
#include "core/common.h"
#include "ui/page/FileListPage.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace beiklive
{

    FilePickerLocation getGameCoverPickerLocation(const GameEntry& entry)
    {
        const auto platform = static_cast<beiklive::enums::EmuPlatform>(entry.platform);
        const std::string defaultLogo = beiklive::tools::getDefaultLogoPath(platform);
        const bool usesBuiltInLogo = entry.logoPath.empty() ||
            entry.logoPath == defaultLogo || entry.logoPath.rfind("romfs:/", 0) == 0;

        if (usesBuiltInLogo) {
            FilePickerLocation location;
            location.startPath = entry.savePath.empty()
                ? beiklive::tools::defaultGameSavePath(entry.platform, entry.path)
                : entry.savePath;

            std::error_code ec;
            fs::create_directories(location.startPath, ec);
            return location;
        }

        const fs::path currentLogo(entry.logoPath);
        return {currentLogo.parent_path().string(), currentLogo.filename().string()};
    }

    void openFilePicker(
        const std::vector<std::string>& extensions,
        std::function<void(const std::string&)> onSelected,
        const std::string& startPath,
        const std::string& filename)
    {
        auto* flPage = new beiklive::FileListPage();
        flPage->setFliter(beiklive::enums::FilterMode::Whitelist, extensions);
        bool hasStartDir = false;
        if (!startPath.empty()) {
            std::error_code ec;
            hasStartDir = fs::exists(startPath, ec) && fs::is_directory(startPath, ec);
        }
        if (hasStartDir && !filename.empty()) {
            std::error_code ec;
            if (fs::exists(fs::path(startPath) / filename, ec))
                flPage->setInitialFocusFilename(filename);
        }
        flPage->onFileSelected = [flPage, onSelected](beiklive::DirListData item)
        {
            if (item.itemType != beiklive::enums::FileType::DRIVE &&
                item.itemType != beiklive::enums::FileType::DIRECTORY)
            {
                std::error_code ec;
                if (!fs::exists(item.fullPath, ec) || !fs::is_regular_file(item.fullPath, ec)) {
                    brls::Application::notify("文件不存在");
                    return;
                }
                onSelected(item.fullPath);
                flPage->requestClose();
            }
        };
        if (hasStartDir)
            flPage->setPath(startPath);

        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setGrow(1.0f);
        container->addView(flPage);
        container->registerAction("关闭"_i18n, brls::BUTTON_START,
                                  [flPage](brls::View*) { flPage->requestClose(); return true; });

        auto* frame = new brls::AppletFrame(container);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame),
                                        brls::TransitionAnimation::FADE);

        if (!hasStartDir)
            flPage->showDriveList();
    }

    void openDirectoryPicker(std::function<void(const std::string&)> onSelected,
                             const std::string& startPath)
    {
        auto* flPage = new beiklive::FileListPage();
        flPage->setDirSelectionMode(true);
        flPage->onDirectorySelected = [onSelected = std::move(onSelected)](
                                          const std::string& path) {
            onSelected(path);
        };
        std::error_code ec;
        const bool hasStartDir = !startPath.empty() && fs::is_directory(startPath, ec) && !ec;
        if (hasStartDir)
            flPage->setPath(startPath);

        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setGrow(1.0f);
        container->addView(flPage);
        container->registerAction("关闭"_i18n, brls::BUTTON_START,
                                  [flPage](brls::View*) { flPage->requestClose(); return true; });
        auto* frame = new brls::AppletFrame(container);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame),
                                        brls::TransitionAnimation::FADE);
        if (!hasStartDir)
            flPage->showDriveList();
    }

} // namespace beiklive
