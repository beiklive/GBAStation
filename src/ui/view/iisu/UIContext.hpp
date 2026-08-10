#pragma once

#include <string>
#include <vector>

#include "FolderDataProvider.hpp"
#include "FolderWidget.hpp"
#include "GameCoverWidget.hpp"
#include "GameDataProvider.hpp"
#include "LayoutManager.hpp"
#include "TextureManager.hpp"
#include "Widget.hpp"

namespace beiklive
{
    /// 统一上下文：所有 Widget 从 Context 获取服务（布局 / 资源 / 游戏数据 / 文件夹）
    class UIContext
    {
    public:
        TextureManager& textures() { return m_textures; }
        const TextureManager& textures() const { return m_textures; }

        LayoutManager& layout() { return m_layout; }
        const LayoutManager& layout() const { return m_layout; }

        GameDbProvider& gameProvider() { return m_gameProvider; }
        const GameDbProvider& gameProvider() const { return m_gameProvider; }

        GameDbFolderProvider& folderProvider() { return m_folderProvider; }
        const GameDbFolderProvider& folderProvider() const { return m_folderProvider; }

        void addItem(const LayoutItem& item);

        /// 主页面布局（根页面）
        void setMainPage(const std::vector<FolderItemDescriptor>& items);
        /// 展开文件夹子布局（支持嵌套，栈式记录）
        void openFolder(const std::string& id);
        /// 返回上一级布局
        void closeFolder();
        bool isFolderOpen() const { return !m_folderStack.empty(); }
        const std::string& currentFolderId() const;

    private:
        LayoutItem descriptorToItem(const FolderItemDescriptor& desc) const;
        void rebuildCurrentPage();
        void injectServices(const LayoutItem& item);

        TextureManager m_textures;
        GameDbProvider m_gameProvider;
        GameDbFolderProvider m_folderProvider;
        LayoutManager m_layout;
        std::vector<FolderItemDescriptor> m_mainItems;
        std::vector<std::string> m_folderStack;
    };
} // namespace beiklive
