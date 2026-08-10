#include "FocusManager.hpp"

namespace beiklive
{
    void FocusManager::setCell(int x, int y, int columns, int rows)
    {
        m_cellX = x < 0 ? 0 : x;
        m_cellY = y < 0 ? 0 : y;
        if (columns > 0 && m_cellX >= columns)
            m_cellX = columns - 1;
        if (rows > 0 && m_cellY >= rows)
            m_cellY = rows - 1;
    }

    void FocusManager::moveLeft(int columns, int rows)
    {
        if (columns <= 0)
            return;
        m_cellX = (m_cellX + columns - 1) % columns;
    }

    void FocusManager::moveRight(int columns, int rows)
    {
        if (columns <= 0)
            return;
        m_cellX = (m_cellX + 1) % columns;
    }

    void FocusManager::moveUp(int rows)
    {
        if (m_cellY > 0)
            --m_cellY;
    }

    void FocusManager::moveDown(int rows)
    {
        if (m_cellY < rows - 1)
            ++m_cellY;
    }
} // namespace beiklive
