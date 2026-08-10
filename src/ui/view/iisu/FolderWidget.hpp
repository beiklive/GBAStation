#pragma once

#include <functional>
#include <optional>
#include <string>

#include "FolderDataProvider.hpp"
#include "Widget.hpp"

namespace beiklive
{
    /// 文件夹 Widget：标题 + 图标 + 子项数量；A 键激活展开子布局
    /// 不保存子元素，通过 FolderDataProvider 获取子布局
    class FolderWidget : public Widget
    {
    public:
        explicit FolderWidget(std::string folderId);

        void setFolderId(const std::string& id);
        const std::string& folderId() const { return m_folderId; }
        void setFolderDataProvider(FolderDataProvider* provider);

        void draw(NVGcontext* vg, const GridRect& rect) override;
        void onActivate() override;
        void onFocus() override;
        void onBlur() override;

        std::string typeName() const override { return "folder"; }
        std::string dataId() const override { return m_folderId; }

        /// 激活回调（由 UIContext 注入：展开文件夹子布局）
        std::function<void()> onActivated;

    private:
        std::string m_folderId;
        FolderDataProvider* m_provider = nullptr;

        std::optional<FolderInfo> m_info;
        bool m_infoResolved = false;
        bool m_focused = false;
    };
} // namespace beiklive
