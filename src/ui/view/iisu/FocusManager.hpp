#pragma once

namespace beiklive
{
    /// 焦点管理器：按网格单元格导航（空白格也可聚焦；Item 覆盖焦点格时成为当前项）
    class FocusManager
    {
    public:
        int cellX() const { return m_cellX; }
        int cellY() const { return m_cellY; }

        void setCell(int x, int y, int columns, int rows);
        void moveLeft(int columns, int rows);
        void moveRight(int columns, int rows);
        void moveUp(int rows);
        void moveDown(int rows);
        void resetToFirst() { m_cellX = 0; m_cellY = 0; }

    private:
        int m_cellX = 0;
        int m_cellY = 0;
    };
} // namespace beiklive
