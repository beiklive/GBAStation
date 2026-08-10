#pragma once

#include <string>
#include <vector>

#include "AnimationManager.hpp"
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
        UIContext();

        TextureManager& textures() { return m_textures; }
        const TextureManager& textures() const { return m_textures; }

        LayoutManager& layout() { return m_layout; }
        const LayoutManager& layout() const { return m_layout; }

        /// 文件夹浮层子布局（悬浮在当前界面上，不覆盖整区）
        LayoutManager& panelLayout() { return m_panelLayout; }
        const LayoutManager& panelLayout() const { return m_panelLayout; }

        GameDbProvider& gameProvider() { return m_gameProvider; }
        const GameDbProvider& gameProvider() const { return m_gameProvider; }

        GameDbFolderProvider& folderProvider() { return m_folderProvider; }
        const GameDbFolderProvider& folderProvider() const { return m_folderProvider; }

        /// 动画系统
        AnimationManager& animations() { return m_animations; }
        const AnimationManager& animations() const { return m_animations; }

        void addItem(const LayoutItem& item);

        /// 主页面布局（根页面）
        void setMainPage(const std::vector<FolderItemDescriptor>& items);
        /// 从 GBAStation/theme/iisu/<fileName> 加载主页面布局，成功返回 true
        bool loadMainPageFromFile(const std::string& fileName);
        /// 打开文件夹浮层（悬浮显示子布局）
        void openFolder(const std::string& id);
        /// 关闭文件夹浮层
        void closeFolder();
        bool isFolderOpen() const { return m_folderOpen; }
        const std::string& currentFolderId() const { return m_folderId; }

    private:
        LayoutItem descriptorToItem(const FolderItemDescriptor& desc) const;
        void injectServices(const LayoutItem& item);

        TextureManager m_textures;
        GameDbProvider m_gameProvider;
        GameDbFolderProvider m_folderProvider;
        AnimationManager m_animations;
        LayoutManager m_layout;
        LayoutManager m_panelLayout;
        bool m_folderOpen = false;
        std::string m_folderId;
    };
} // namespace beiklive
