#include "UIContext.hpp"

#include "WidgetFactory.hpp"

namespace beiklive
{
    void UIContext::injectServices(const LayoutItem& item)
    {
        if (!item.widget)
            return;
        item.widget->setTextureManager(&m_textures);
        // 内部元素圆角比格子圆角小 4px
        item.widget->setCornerRadius(m_layout.grid().config().radius - 4.f);
        if (auto* cover = dynamic_cast<GameCoverWidget*>(item.widget.get()))
            cover->setGameDataProvider(&m_gameProvider);
        if (auto* folder = dynamic_cast<FolderWidget*>(item.widget.get())) {
            folder->setFolderDataProvider(&m_folderProvider);
            const std::string id = folder->folderId();
            folder->onActivated = [this, id]() { openFolder(id); };
        }
    }

    void UIContext::addItem(const LayoutItem& item)
    {
        m_layout.addItem(item);
        if (m_layout.items().empty())
            return;
        injectServices(m_layout.items().back());
    }

    LayoutItem UIContext::descriptorToItem(
        const FolderItemDescriptor& desc) const
    {
        LayoutItem item;
        item.x = desc.x;
        item.y = desc.y;
        item.w = desc.w;
        item.h = desc.h;
        item.focusable = desc.focusable;
        switch (desc.type) {
            case WidgetType::GameCover:
                item.widget = WidgetFactory::createGameCover(desc.id);
                break;
            case WidgetType::Folder:
                item.widget = WidgetFactory::createFolder(desc.id);
                break;
            case WidgetType::Image:
                item.widget = WidgetFactory::createImage(desc.path);
                break;
            default:
                break;
        }
        return item;
    }

    void UIContext::rebuildCurrentPage()
    {
        m_layout.clear();

        std::vector<FolderItemDescriptor> items;
        if (m_folderStack.empty())
            items = m_mainItems;
        else
            items = m_folderProvider.getFolderItems(m_folderStack.back());

        for (const auto& desc : items)
            addItem(descriptorToItem(desc));

        m_layout.resetFocusToFirst();
    }

    void UIContext::setMainPage(const std::vector<FolderItemDescriptor>& items)
    {
        m_mainItems = items;
        m_folderStack.clear();
        rebuildCurrentPage();
    }

    void UIContext::openFolder(const std::string& id)
    {
        if (!m_folderProvider.getFolder(id))
            return;
        m_folderStack.push_back(id);
        rebuildCurrentPage();
    }

    void UIContext::closeFolder()
    {
        if (m_folderStack.empty())
            return;
        m_folderStack.pop_back();
        rebuildCurrentPage();
    }

    const std::string& UIContext::currentFolderId() const
    {
        static const std::string kRoot;
        return m_folderStack.empty() ? kRoot : m_folderStack.back();
    }
} // namespace beiklive
