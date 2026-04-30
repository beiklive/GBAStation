#include "FilePickerHelper.hpp"
#include "core/common.h"
#include "ui/page/FileListPage.hpp"

namespace beiklive
{

    void openFilePicker(
        const std::vector<std::string>& extensions,
        std::function<void(const std::string&)> onSelected,
        const std::string& startPath)
    {
        auto* flPage = new beiklive::FileListPage();
        flPage->setFliter(beiklive::enums::FilterMode::Whitelist, extensions);
        flPage->onFileSelected = [onSelected](beiklive::DirListData item)
        {
            if (item.itemType != beiklive::enums::FileType::DRIVE &&
                item.itemType != beiklive::enums::FileType::DIRECTORY)
            {
                onSelected(item.fullPath);
                brls::Application::popActivity();
            }
        };
        if (!startPath.empty())
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

        if (startPath.empty())
            flPage->showDriveList();
    }

} // namespace beiklive
