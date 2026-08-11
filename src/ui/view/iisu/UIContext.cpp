#include "UIContext.hpp"

#include <cmath>
#include <fstream>

#include <nlohmann/json.hpp>

#include "ImageWidget.hpp"
#include "WidgetFactory.hpp"

namespace beiklive
{
    namespace
    {
        constexpr const char* LAYOUT_DIR = "GBAStation/theme/iisu";
    } // namespace

    UIContext::UIContext()
    {
        // 注入动画管理器
        m_layout.setAnimationManager(&m_animations);
        m_panelLayout.setAnimationManager(&m_animations);

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
            case WidgetType::Image: {
                item.widget = WidgetFactory::createImage(desc.path);
                if (auto* image =
                        dynamic_cast<ImageWidget*>(item.widget.get()))
                    image->setSpeed(desc.speedMul);
                break;
            }
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

    bool UIContext::loadMainPageFromFile(const std::string& fileName)
    {
        const std::string path =
            std::string(LAYOUT_DIR) + "/" + fileName;

        std::ifstream in(path);
        if (!in)
            return false;

        nlohmann::json root;
        try {
            in >> root;
        } catch (...) {
            return false;
        }
        if (!root.contains("items") || !root["items"].is_array())
            return false;

        // 网格参数（可选）
        if (root.contains("grid") && root["grid"].is_object()) {
            const auto& grid = root["grid"];
            auto& cfg = m_layout.grid().config();
            if (grid.contains("columns"))
                cfg.columns = grid.value("columns", cfg.columns);
            if (grid.contains("rows"))
                cfg.rows = grid.value("rows", cfg.rows);
            if (grid.contains("cell_width"))
                cfg.cellWidth = grid.value("cell_width", cfg.cellWidth);
            if (grid.contains("cell_height"))
                cfg.cellHeight = grid.value("cell_height", cfg.cellHeight);
            if (grid.contains("gap"))
                cfg.gap = grid.value("gap", cfg.gap);
        }

        std::vector<FolderItemDescriptor> items;
        for (const auto& j : root["items"]) {
            if (!j.is_object())
                continue;

            FolderItemDescriptor desc;
            const std::string type = j.value("type", "empty");
            if (type == "game_cover")
                desc.type = WidgetType::GameCover;
            else if (type == "folder")
                desc.type = WidgetType::Folder;
            else if (type == "image")
                desc.type = WidgetType::Image;
            else if (type == "live")
                desc.type = WidgetType::Live;
            else
                desc.type = WidgetType::Empty;

            // 保存时 data_id 对 image 记录的是图片路径
            const std::string dataId = j.value("data_id", "");
            desc.id = dataId;
            desc.path = dataId;

            desc.x = j.value("x", 0);
            desc.y = j.value("y", 0);
            desc.w = j.value("w", 1);
            desc.h = j.value("h", 1);
            desc.focusable = j.value("focusable", true);
            desc.speedMul = j.value("speed", 1.f);
            items.push_back(std::move(desc));
        }
        if (items.empty())
            return false;

        setMainPage(items);
        return true;
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
