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

    m_contentBox = new brls::Box(brls::Axis::COLUMN);
    m_contentBox->setFocusable(false);
    m_contentBox->setWidth(View::AUTO);
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

    for (auto* item : m_attachedItems)
    {
        item->removeFromSuperView(false);
        std::string id = "default";
        auto it = m_queueMap.begin();
        if (it != m_queueMap.end()) id = it->first;
        for (auto& pair : m_queueMap)
        {
            if (pair.second) continue;
            id = pair.first;
            break;
        }
        auto qit = m_queueMap.find(id);
        if (qit != m_queueMap.end())
            qit->second->push_back(item);
        else
            delete item;
    }
    m_attachedItems.clear();

    m_itemCount = m_dataSource->getItemCount();
    int rows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    float totalH = rows * m_estimatedRowHeight + 10.0f;
    m_contentBox->setHeight(totalH);

    m_visibleMin = 0;
    m_visibleMax = 0;

    if (m_itemCount > 0)
        addCellAt(0, true);

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
    for (auto* item : m_attachedItems)
    {
        if (static_cast<size_t>(item->getGridIndex()) == index)
            return item;
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

float RecyclingGrid::getYForItem(size_t index) const
{
    return getRowIndex(index) * m_estimatedRowHeight;
}

size_t RecyclingGrid::getCellStartIndex() const
{
    brls::Rect visibleFrame = this->getVisibleFrame();
    float top = visibleFrame.getMinY();
    if (top <= 0) return 0;

    int startRow = static_cast<int>(top / m_estimatedRowHeight);
    if (startRow < 0) startRow = 0;
    size_t startIdx = static_cast<size_t>(startRow) * m_spanCount;

    int prefetchRows = m_preFetchLine;
    int prefetchStart = startRow - prefetchRows;
    if (prefetchStart < 0) prefetchStart = 0;
    startIdx = static_cast<size_t>(prefetchStart) * m_spanCount;

    return startIdx;
}

size_t RecyclingGrid::getCellEndIndex() const
{
    brls::Rect visibleFrame = this->getVisibleFrame();
    float bottom = visibleFrame.getMaxY();

    int endRow = static_cast<int>(bottom / m_estimatedRowHeight) + 1;
    int rows = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;
    if (endRow > rows) endRow = rows;

    int prefetchRows = m_preFetchLine;
    int prefetchEnd = endRow + prefetchRows;
    if (prefetchEnd > rows) prefetchEnd = rows;

    size_t endIdx = static_cast<size_t>(prefetchEnd) * m_spanCount;
    if (endIdx > m_itemCount) endIdx = m_itemCount;

    return endIdx;
}

void RecyclingGrid::draw(NVGcontext* vg, float x, float y, float w, float h,
                          brls::Style style, brls::FrameContext* ctx)
{
    if (m_focusOnReload && m_attachedItems.size() > 0)
    {
        m_focusOnReload = false;
        brls::Application::giveFocus(m_attachedItems[0]);
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

    size_t newMin = getCellStartIndex();
    size_t newMax = getCellEndIndex();

    if (newMin == m_visibleMin && newMax == m_visibleMax)
    {
        if (m_attachedItems.empty() && m_itemCount > 0)
            addCellAt(0, true);
        return;
    }

    recycleOutOfRangeCells();

    if (m_attachedItems.empty() && m_itemCount > 0)
    {
        addCellAt(newMin, true);
    }
    else
    {
        if (m_attachedItems.size() > 0)
        {
            RecyclingGridItem* firstItem = m_attachedItems.front();
            size_t firstIdx = static_cast<size_t>(firstItem->getGridIndex());

            if (firstIdx > newMin)
                addCellAt(newMin, false);

            RecyclingGridItem* lastItem = m_attachedItems.back();
            size_t lastIdx = static_cast<size_t>(lastItem->getGridIndex());

            if (lastIdx < newMax - 1)
            {
                size_t startAdd = lastIdx + 1;
                if (startAdd < newMin) startAdd = newMin;
                addCellAt(startAdd, true);
            }
        }
        else
        {
            addCellAt(newMin, true);
        }
    }

    m_visibleMin = newMin;
    m_visibleMax = newMax;

    if (!m_requestNextPage && m_attachedItems.size() > 0)
    {
        RecyclingGridItem* lastItem = m_attachedItems.back();
        size_t lastIdx = static_cast<size_t>(lastItem->getGridIndex());

        if (lastIdx + 1 >= m_itemCount && m_itemCount > 0)
        {
            m_requestNextPage = true;
            if (onNextPage)
                onNextPage();
        }
    }
}

void RecyclingGrid::recycleOutOfRangeCells()
{
    size_t newMin = getCellStartIndex();
    size_t newMax = getCellEndIndex();

    auto it = m_attachedItems.begin();
    while (it != m_attachedItems.end())
    {
        RecyclingGridItem* item = *it;
        size_t idx = static_cast<size_t>(item->getGridIndex());

        if (idx < newMin || idx >= newMax)
        {
            item->removeFromSuperView(false);
            it = m_attachedItems.erase(it);

            std::string id = "default";
            if (!m_queueMap.empty())
                id = m_queueMap.begin()->first;
            auto qit = m_queueMap.find(id);
            if (qit != m_queueMap.end())
                qit->second->push_back(item);
            else
                delete item;
        }
        else
        {
            ++it;
        }
    }
}

void RecyclingGrid::addCellAt(size_t startIndex, bool below)
{
    if (!m_dataSource) return;

    size_t endIndex = getCellEndIndex();
    if (startIndex >= endIndex) return;

    float contentWidth = this->getWidth();
    float padding = 10.0f;
    float cellWidth = (contentWidth - padding * 2) / m_spanCount;

    if (below)
    {
        for (size_t i = startIndex; i < endIndex && i < m_itemCount; ++i)
        {
            if (getGridItemByIndex(i)) continue;

            RecyclingGridItem* cell = m_dataSource->cellForRow(this, i);
            if (!cell) continue;

            cell->setGridIndex(static_cast<int>(i));
            cell->setWidth(cellWidth);
            cell->setHeight(m_itemHeight);
            cell->setMarginRight(5.0f);
            cell->setMarginLeft(5.0f);

            m_attachedItems.push_back(cell);
            m_contentBox->addView(cell);
        }
    }
    else
    {
        std::vector<RecyclingGridItem*> newItems;
        for (size_t i = startIndex; i < endIndex && i < m_itemCount; ++i)
        {
            if (getGridItemByIndex(i)) continue;

            RecyclingGridItem* cell = m_dataSource->cellForRow(this, i);
            if (!cell) continue;

            cell->setGridIndex(static_cast<int>(i));
            cell->setWidth(cellWidth);
            cell->setHeight(m_itemHeight);
            cell->setMarginRight(5.0f);
            cell->setMarginLeft(5.0f);

            newItems.push_back(cell);
        }

        m_attachedItems.insert(m_attachedItems.begin(), newItems.begin(), newItems.end());
        for (auto* cell : newItems)
            m_contentBox->addView(cell);
    }

    setupNavigation();
}

void RecyclingGrid::setupNavigation()
{
    if (m_attachedItems.empty()) return;

    auto& items = m_attachedItems;
    size_t n = items.size();

    for (size_t i = 0; i < n; ++i)
    {
        RecyclingGridItem* cell = items[i];
        int idx = cell->getGridIndex();
        int row = idx / m_spanCount;
        int col = idx % m_spanCount;

        int rowCount = (static_cast<int>(m_itemCount) + m_spanCount - 1) / m_spanCount;

        if (row > 0)
        {
            int upIdx = idx - m_spanCount;
            RecyclingGridItem* upCell = getGridItemByIndex(upIdx);
            if (upCell)
                cell->setCustomNavigationRoute(brls::FocusDirection::UP, upCell);
        }

        if (row < rowCount - 1)
        {
            int downIdx = idx + m_spanCount;
            if (downIdx < getItemCount())
            {
                RecyclingGridItem* downCell = getGridItemByIndex(downIdx);
                if (downCell)
                    cell->setCustomNavigationRoute(brls::FocusDirection::DOWN, downCell);
            }
        }

        if (col > 0)
        {
            int leftIdx = idx - 1;
            RecyclingGridItem* leftCell = getGridItemByIndex(leftIdx);
            if (leftCell)
                cell->setCustomNavigationRoute(brls::FocusDirection::LEFT, leftCell);
        }

        if (col < m_spanCount - 1)
        {
            int rightIdx = idx + 1;
            if (rightIdx < getItemCount())
            {
                RecyclingGridItem* rightCell = getGridItemByIndex(rightIdx);
                if (rightCell)
                    cell->setCustomNavigationRoute(brls::FocusDirection::RIGHT, rightCell);
            }
        }
    }
}

} // namespace beiklive
