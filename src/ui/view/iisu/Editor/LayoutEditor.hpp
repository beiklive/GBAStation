#pragma once

#include <string>

#include "../GridSystem.hpp"
#include "../LayoutItem.hpp"

namespace beiklive
{
    class LayoutManager;

    /// 布局编辑器：编辑模式下移动/缩放 Tile，并保存布局 JSON
    class LayoutEditor
    {
    public:
        explicit LayoutEditor(LayoutManager* layout);

        void enter();
        void exit();
        bool isActive() const { return m_active; }

        /// 抬起/放下当前焦点 Widget（抬起时放大可移动，再按放下）
        bool toggleLift();
        bool isLifted() const { return m_lifted != nullptr; }
        LayoutItem* lifted() const { return m_lifted; }

        /// 移动当前选中的 Tile（碰撞 + 边界检测），成功返回 true
        bool moveItem(int dx, int dy);

        /// 保存布局 JSON 到 GBAStation/theme/iisu/<fileName>
        bool save(const std::string& fileName) const;

        /// 编辑覆盖层：网格辅助线 + 选中 Tile 强调边框与 [WxH] 角标
        void draw(NVGcontext* vg, int fontId);

    private:
        bool checkCollision(const LayoutItem& a, const LayoutItem& b) const;

        LayoutManager* m_layout = nullptr;
        bool m_active = false;
        LayoutItem* m_lifted = nullptr;
    };
} // namespace beiklive
