#include "FilePickerHelper.hpp"
#include "core/common.h"
#include "ui/page/FileListPage.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace beiklive
{

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
        flPage->onFileSelected = [onSelected](beiklive::DirListData item)
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
                brls::Application::popActivity();
            }
        };
        if (hasStartDir)
            flPage->setPath(startPath);

        auto* container = new brls::Box(brls::Axis::COLUMN);
        container->setGrow(1.0f);
        container->addView(flPage);
        container->registerAction("关闭"_i18n, brls::BUTTON_START,
                                  [](brls::View*) { brls::Application::popActivity(); return true; });

        auto* frame = new brls::AppletFrame(container);
        frame->setHeaderVisibility(brls::Visibility::GONE);
        frame->setFooterVisibility(brls::Visibility::GONE);
        frame->setBackground(brls::ViewBackground::NONE);
        brls::Application::pushActivity(new brls::Activity(frame));

        if (!hasStartDir)
            flPage->showDriveList();
    }

} // namespace beiklive
