#include "RecyclingGrid.hpp"

#include <algorithm>
#include <cmath>

namespace beiklive {

RecyclingGrid::RecyclingGrid(int spanCount, float itemHeight, float itemSpace)
    : m_spanCount(spanCount)
    , m_itemHeight(itemHeight)
    , m_itemSpace(itemSpace)
    , m_estimatedRowHeight(itemHeight + itemSpace)
{
    this->setFocusable(false);
    this->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
    this->setScrollingIndicatorVisible(false);

    this->setWidth(View::AUTO);
    this->setHeight(View::AUTO);

    m_contentBox = new brls::Box(brls::Axis::COLUMN);
    m_contentBox->setFocusable(false);
    m_contentBox->setWidthPercentage(100);
    this->setContentView(m_contentBox);
}

void RecyclingGrid::registerCell(const std::string& identifier, std::function<RecyclingGridItem*()> factory)
{
    m_allocationMap[identifier] = std::move(factory);
    m_queueMap[identifier] = new std::vector<RecyclingGridItem*>();
}

RecyclingGridItem* RecyclingGrid::dequeueReusableCell(const std::string& identifier)
{
    auto it = m_queueMap.find(identifier);
    if (it != m_queueMap.end() && !it->second->empty())
    {
        RecyclingGridItem* cell = it->second->back();
        it->second->pop_back();
        cell->prepareForReuse();
        return cell;
    }

    auto allocIt = m_allocationMap.find(identifier);
    if (allocIt != m_allocationMap.end())
        return allocIt->second();

    return nullptr;
}

void RecyclingGrid::setDataSource(RecyclingGridDataSource* source)
{
    m_dataSource = source;
}

void RecyclingGrid::reloadData()
{
    if (!m_dataSource) return;

    for (auto& [row, cells] : m_attachedRows)
    {
        for (auto& ci : cells)
        {
            ci.cell->removeFromSuperView(false);
            std::string id = m_queueMap.begin()->first;
            auto qit = m_queueMap.find(id);
            if (qit != m_queueMap.end())
                qit->second->push_back(ci.cell);
            else
                delete ci.cell;
        }
    }
    m_attachedRows.clear();
    m_contentBox->clearViews(true);

    m_itemCount = m_dataSource->getItemCount();
    int rows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    float totalH = rows * m_estimatedRowHeight + 10.0f;
    m_contentBox->setHeight(totalH);

    m_visibleMinRow = 0;
    m_visibleMaxRow = 0;

    brls::Rect visibleFrame = this->getVisibleFrame();
    int startRow = static_cast<int>(visibleFrame.getMinY() / m_estimatedRowHeight);
    if (startRow < 0) startRow = 0;
    startRow = std::max(0, startRow - m_preFetchLine);

    int totalRows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    int endRow = std::min(totalRows, startRow + 4);

    for (int r = startRow; r < endRow; ++r)
        addCellsForRow(r);

    m_visibleMinRow = startRow;
    m_visibleMaxRow = endRow;

    m_focusOnReload = true;
    invalidate();
}

void RecyclingGrid::notifyDataChanged()
{
    if (!m_dataSource) return;
    m_itemCount = m_dataSource->getItemCount();
    int rows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    float totalH = rows * m_estimatedRowHeight + 10.0f;
    m_contentBox->setHeight(totalH);
    m_requestNextPage = false;
    invalidate();
}

RecyclingGridItem* RecyclingGrid::getGridItemByIndex(size_t index)
{
    int row = static_cast<int>(index) / m_spanCount;
    auto it = m_attachedRows.find(row);
    if (it == m_attachedRows.end()) return nullptr;
    int col = static_cast<int>(index) % m_spanCount;
    for (auto& ci : it->second)
    {
        if (ci.col == col) return ci.cell;
    }
    return nullptr;
}

void RecyclingGrid::setSpanCount(int count)
{
    if (m_spanCount == count) return;
    m_spanCount = count;
    reloadData();
}

void RecyclingGrid::setItemHeight(float height)
{
    m_itemHeight = height;
    m_estimatedRowHeight = height + m_itemSpace;
}

void RecyclingGrid::setItemSpace(float space)
{
    m_itemSpace = space;
    m_estimatedRowHeight = m_itemHeight + space;
}

int RecyclingGrid::getRowIndex(size_t itemIndex) const
{
    return static_cast<int>(itemIndex) / m_spanCount;
}

int RecyclingGrid::getItemCount() const
{
    return static_cast<int>(m_itemCount);
}

float RecyclingGrid::getContentHeightForRows(int rows) const
{
    return rows * m_estimatedRowHeight;
}

size_t RecyclingGrid::getCellStartIndex()
{
    brls::Rect visibleFrame = this->getVisibleFrame();
    float top = visibleFrame.getMinY();
    if (top <= 0) return 0;

    int startRow = static_cast<int>(top / m_estimatedRowHeight);
    if (startRow < 0) startRow = 0;
    startRow = std::max(0, startRow - m_preFetchLine);
    return static_cast<size_t>(startRow) * m_spanCount;
}

size_t RecyclingGrid::getCellEndIndex()
{
    brls::Rect visibleFrame = this->getVisibleFrame();
    float bottom = visibleFrame.getMaxY();

    int endRow = static_cast<int>(bottom / m_estimatedRowHeight) + 1;
    int rows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    if (endRow > rows) endRow = rows;
    endRow = std::min(rows, endRow + m_preFetchLine);

    size_t endIdx = static_cast<size_t>(endRow) * m_spanCount;
    if (endIdx > m_itemCount) endIdx = m_itemCount;
    return endIdx;
}

void RecyclingGrid::draw(NVGcontext* vg, float x, float y, float w, float h,
                          brls::Style style, brls::FrameContext* ctx)
{
    if (m_focusOnReload && !m_attachedRows.empty())
    {
        m_focusOnReload = false;
        auto& firstRow = m_attachedRows.begin()->second;
        if (!firstRow.empty() && firstRow[0].cell)
            brls::Application::giveFocus(firstRow[0].cell);
    }

    itemsRecyclingLoop();

    brls::ScrollingFrame::draw(vg, x, y, w, h, style, ctx);
}

void RecyclingGrid::onFocusGained()
{
    brls::ScrollingFrame::onFocusGained();
}

void RecyclingGrid::onFocusLost()
{
    brls::ScrollingFrame::onFocusLost();
}

void RecyclingGrid::itemsRecyclingLoop()
{
    if (!m_dataSource || m_itemCount == 0) return;

    brls::Rect visibleFrame = this->getVisibleFrame();

    int totalRows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    int newMinRow = std::max(0, static_cast<int>(visibleFrame.getMinY() / m_estimatedRowHeight) - m_preFetchLine);
    int newMaxRow = std::min(totalRows, static_cast<int>(visibleFrame.getMaxY() / m_estimatedRowHeight) + 1 + m_preFetchLine);
    if (newMaxRow < newMinRow + 1) newMaxRow = newMinRow + 1;

    if (newMinRow == m_visibleMinRow && newMaxRow == m_visibleMaxRow)
    {
        if (m_attachedRows.empty() && totalRows > 0)
            addCellsForRow(0);
        return;
    }

    recycleOutOfRangeRows();

    for (int r = newMinRow; r < newMaxRow; ++r)
    {
        if (m_attachedRows.find(r) == m_attachedRows.end())
            addCellsForRow(r);
    }

    m_visibleMinRow = newMinRow;
    m_visibleMaxRow = newMaxRow;

    if (!m_requestNextPage && m_visibleMaxRow >= totalRows)
    {
        m_requestNextPage = true;
        if (onNextPage)
            onNextPage();
    }
}

void RecyclingGrid::recycleOutOfRangeRows()
{
    brls::Rect visibleFrame = this->getVisibleFrame();
    int totalRows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    int newMinRow = std::max(0, static_cast<int>(visibleFrame.getMinY() / m_estimatedRowHeight) - m_preFetchLine);
    int newMaxRow = std::min(totalRows, static_cast<int>(visibleFrame.getMaxY() / m_estimatedRowHeight) + 1 + m_preFetchLine);
    if (newMaxRow < newMinRow + 1) newMaxRow = newMinRow + 1;

    std::string id = m_queueMap.begin()->first;

    auto it = m_attachedRows.begin();
    while (it != m_attachedRows.end())
    {
        int row = it->first;
        if (row < newMinRow || row >= newMaxRow)
        {
            for (auto& ci : it->second)
            {
                ci.cell->removeFromSuperView(false);
                auto qit = m_queueMap.find(id);
                if (qit != m_queueMap.end())
                    qit->second->push_back(ci.cell);
                else
                    delete ci.cell;
            }
            auto* rowBox = dynamic_cast<brls::Box*>(it->second[0].cell->getParent());
            if (rowBox)
            {
                rowBox->clearViews(true);
                rowBox->removeFromSuperView(true);
            }
            it = m_attachedRows.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void RecyclingGrid::addCellsForRow(int row)
{
    if (!m_dataSource) return;

    int totalRows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    if (row < 0 || row >= totalRows) return;

    float cellWidthPct = 100.0f / m_spanCount;

    auto* rowBox = new brls::Box(brls::Axis::ROW);
    rowBox->setFocusable(false);
    rowBox->setWidthPercentage(100);
    rowBox->setHeight(m_estimatedRowHeight);
    rowBox->setAlignItems(brls::AlignItems::CENTER);

    std::vector<CellItem> cellsInRow;
    int startIdx = row * m_spanCount;
    int endIdx = std::min(startIdx + m_spanCount, static_cast<int>(m_itemCount));

    for (int i = startIdx; i < endIdx; ++i)
    {
        RecyclingGridItem* cell = m_dataSource->cellForRow(this, i);
        if (!cell) continue;

        cell->setGridIndex(i);
        cell->setWidthPercentage(cellWidthPct);
        cell->setHeight(m_itemHeight);
        cell->setMargins(5.0f, 5.0f, 5.0f, 5.0f);

        cellsInRow.push_back({cell, row, i - startIdx});
        rowBox->addView(cell);
    }

    m_contentBox->addView(rowBox);
    m_attachedRows[row] = std::move(cellsInRow);
    setupNavigation();
}

void RecyclingGrid::setupNavigation()
{
    for (auto& [row, cells] : m_attachedRows)
    {
        for (auto& ci : cells)
        {
            RecyclingGridItem* cell = ci.cell;
            int idx = cell->getGridIndex();

            int totalRows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;

            if (row > 0)
            {
                auto prevIt = m_attachedRows.find(row - 1);
                if (prevIt != m_attachedRows.end() && ci.col < static_cast<int>(prevIt->second.size()))
                {
                    cell->setCustomNavigationRoute(brls::FocusDirection::UP, prevIt->second[ci.col].cell);
                }
            }

            if (row < totalRows - 1)
            {
                auto nextIt = m_attachedRows.find(row + 1);
                if (nextIt != m_attachedRows.end() && ci.col < static_cast<int>(nextIt->second.size()))
                {
                    cell->setCustomNavigationRoute(brls::FocusDirection::DOWN, nextIt->second[ci.col].cell);
                }
            }

            if (ci.col > 0)
            {
                cell->setCustomNavigationRoute(brls::FocusDirection::LEFT, cells[ci.col - 1].cell);
            }

            if (ci.col < m_spanCount - 1 && ci.col + 1 < static_cast<int>(cells.size()))
            {
                cell->setCustomNavigationRoute(brls::FocusDirection::RIGHT, cells[ci.col + 1].cell);
            }
        }
    }
}

} // namespace beiklive
