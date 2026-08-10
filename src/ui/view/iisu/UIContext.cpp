#include "UIContext.hpp"

#include <cmath>

#include "WidgetFactory.hpp"

namespace beiklive
{
    UIContext::UIContext()
    {
        // 浮层面板网格：3 行 N 列，向右延伸，支持横向滚动
        auto& cfg = m_panelLayout.grid().config();
        cfg.rows = 3;
        cfg.cellWidth = 160.f;
        cfg.cellHeight = 160.f;
        cfg.gap = 14.f;
        m_panelLayout.grid().setScrollable(true);
    }

    void UIContext::injectServices(const LayoutItem& item)
    {
        if (!item.widget)
            return;
        item.widget->setTextureManager(&m_textures);
        // 内部元素圆角比格子圆角小 6px
        item.widget->setCornerRadius(m_layout.grid().config().radius - 6.f);
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
            case WidgetType::Live:
                item.widget = WidgetFactory::createLive(desc.id);
                break;
            default:
                break;
        }
        return item;
    }

    void UIContext::setMainPage(const std::vector<FolderItemDescriptor>& items)
    {
        closeFolder();
        m_layout.clear();
        for (const auto& desc : items)
            addItem(descriptorToItem(desc));
        m_layout.resetFocusToFirst();
    }

    void UIContext::openFolder(const std::string& id)
    {
        if (!m_folderProvider.getFolder(id))
            return;
        m_folderId = id;
        m_folderOpen = true;
        m_panelLayout.clear();
        const auto items = m_folderProvider.getFolderItems(id);
        for (const auto& desc : items) {
            m_panelLayout.addItem(descriptorToItem(desc));
            injectServices(m_panelLayout.items().back());
        }
        // 列数按条目数计算（3 行，向右延伸）
        auto& cfg = m_panelLayout.grid().config();
        cfg.columns = std::max(1, static_cast<int>(
            std::ceil(static_cast<double>(items.size()) / 3.0)));
        m_panelLayout.grid().setScrollX(0.f);
        m_panelLayout.resetFocusToFirst();
    }

    void UIContext::closeFolder()
    {
        if (!m_folderOpen)
            return;
        m_folderOpen = false;
        m_folderId.clear();
        m_panelLayout.clear();
    }
} // namespace beiklive
