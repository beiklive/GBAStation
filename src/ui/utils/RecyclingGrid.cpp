#include "RecyclingGrid.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>

// ── RecyclingGridContentBox ────────────────────────────────────

RecyclingGridContentBox::RecyclingGridContentBox(RecyclingGrid* recycler)
    : Box(brls::Axis::ROW), m_recycler(recycler) {}

brls::View* RecyclingGridContentBox::getNextFocus(brls::FocusDirection direction, brls::View* currentView)
{
    auto* next = m_recycler->getNextCellFocus(direction, currentView);
    return next;
}

// ── RecyclingGrid ──────────────────────────────────────────────

RecyclingGrid::RecyclingGrid()
{
    setFocusable(false);
    setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
    setScrollingIndicatorVisible(false);

    m_contentBox = new RecyclingGridContentBox(this);
    setContentView(m_contentBox);
}

RecyclingGrid::~RecyclingGrid()
{
    delete m_dataSource;
    for (auto& it : m_queueMap)
    {
        for (auto* item : *it.second)
        {
            item->setParent(nullptr);
            delete item;
        }
        delete it.second;
    }
}

void RecyclingGrid::registerCell(const std::string& identifier,
                                  std::function<RecyclingGridItem*()> allocation)
{
    m_queueMap[identifier] = new std::vector<RecyclingGridItem*>();
    m_allocationMap[identifier] = std::move(allocation);
}

RecyclingGridItem* RecyclingGrid::dequeueReusableCell(const std::string& identifier)
{
    RecyclingGridItem* cell = nullptr;
    auto it = m_queueMap.find(identifier);

    if (it != m_queueMap.end())
    {
        auto* vec = it->second;
        if (!vec->empty())
        {
            auto* curFocus = brls::Application::getCurrentFocus();
            if (vec->back() == curFocus && vec->size() > 1)
            {
                cell = vec->front();
                vec->erase(vec->begin());
            }
            else
            {
                cell = vec->back();
                vec->pop_back();
            }
        }
    }

    if (!cell)
    {
        auto allocIt = m_allocationMap.find(identifier);
        if (allocIt != m_allocationMap.end())
        {
            cell = allocIt->second();
            cell->reuseIdentifier = identifier;
        }
    }

    if (cell)
    {
        cell->prepareForReuse();
        cell->setHeight(brls::View::AUTO);
    }

    return cell;
}

void RecyclingGrid::setDataSource(RecyclingGridDataSource* source)
{
    if (m_dataSource) delete m_dataSource;
    m_requestNextPage = false;
    m_dataSource = source;
    if (m_layouted) reloadData();
}

RecyclingGridDataSource* RecyclingGrid::getDataSource() const
{
    return m_dataSource;
}

// ── reloadData ─────────────────────────────────────────────────

void RecyclingGrid::reloadData()
{
    if (!m_layouted)
    {
        m_layouted = true;
        m_oldWidth = getWidth();
        if (m_oldWidth != m_oldWidth) m_oldWidth = 1200;
    }

    auto& children = m_contentBox->getChildren();
    for (auto* child : children)
    {
        if (auto* item = dynamic_cast<RecyclingGridItem*>(child))
        {
            queueReusableCell(item);
            child->willDisappear(true);
        }
    }
    children.clear();

    visibleMin = UINT32_MAX;
    visibleMax = 0;

    m_renderedFrame = brls::Rect();
    m_renderedFrame.size.width = getWidth();
    if (m_renderedFrame.size.width != m_renderedFrame.size.width)
        m_renderedFrame.size.width = m_oldWidth;

    setContentOffsetY(0, false);
    if (!m_dataSource || m_dataSource->getItemCount() <= 0)
    {
        m_contentBox->setHeight(0);
        return;
    }

    size_t cellFocusIndex = m_defaultCellFocus;
    if (cellFocusIndex >= m_dataSource->getItemCount())
        cellFocusIndex = m_dataSource->getItemCount() - 1;

    m_contentBox->setHeight(
        (estimatedRowHeight + estimatedRowSpace) * (float)getRowCount()
        - estimatedRowSpace + m_paddingTop + m_paddingBottom);

    size_t lineHeadIndex = cellFocusIndex / spanCount * spanCount;
    m_renderedFrame.origin.y = getHeightByCellIndex(lineHeadIndex);
    addCellAt(lineHeadIndex, true);

    itemsRecyclingLoop();

    auto* firstCell = getGridItemByIndex(0);
    if (firstCell)
        brls::Application::giveFocus(firstCell);
}

void RecyclingGrid::notifyDataChanged()
{
    if (!m_dataSource) return;
    m_contentBox->setHeight(
        (estimatedRowHeight + estimatedRowSpace) * getRowCount()
        - estimatedRowSpace + m_paddingTop + m_paddingBottom);
    m_requestNextPage = false;
    invalidate();
}

void RecyclingGrid::clearData()
{
    if (m_dataSource)
    {
        m_dataSource->clearData();
        reloadData();
    }
}

// ── 查询 ───────────────────────────────────────────────────────

RecyclingGridItem* RecyclingGrid::getGridItemByIndex(size_t index)
{
    for (auto* v : m_contentBox->getChildren())
    {
        auto* item = dynamic_cast<RecyclingGridItem*>(v);
        if (item && item->getIndex() == index)
            return item;
    }
    return nullptr;
}

size_t RecyclingGrid::getItemCount() const
{
    return m_dataSource ? m_dataSource->getItemCount() : 0;
}

size_t RecyclingGrid::getRowCount() const
{
    size_t count = m_dataSource ? m_dataSource->getItemCount() : 0;
    return count > 0 ? (count - 1) / spanCount + 1 : 0;
}

float RecyclingGrid::getHeightByCellIndex(size_t index, size_t start) const
{
    if (index <= start) return 0;
    return (estimatedRowHeight + estimatedRowSpace) * (size_t)((index - start) / spanCount);
}

// ── 焦点 ───────────────────────────────────────────────────────

void RecyclingGrid::setDefaultCellFocus(size_t index) { m_defaultCellFocus = index; }

size_t RecyclingGrid::getDefaultCellFocus() const { return m_defaultCellFocus; }

brls::View* RecyclingGrid::getDefaultFocus()
{
    if (m_dataSource && m_dataSource->getItemCount() > 0)
        return ScrollingFrame::getDefaultFocus();
    return nullptr;
}

void RecyclingGrid::setFocusChangeCallback(std::function<void(size_t)> callback)
{
    m_focusChangeCallback = std::move(callback);
}

void RecyclingGrid::onChildFocusGained(View* directChild, View* focusedView)
{
    ScrollingFrame::onChildFocusGained(directChild, focusedView);
    View* v = focusedView;
    while (v && !dynamic_cast<RecyclingGridItem*>(v))
        v = v->getParent();
    if (v)
    {
        size_t idx = static_cast<RecyclingGridItem*>(v)->getIndex();
        m_defaultCellFocus = idx;
        if (m_focusChangeCallback) m_focusChangeCallback(idx);
    }
}

// ── 导航 ───────────────────────────────────────────────────────

brls::View* RecyclingGrid::getNextCellFocus(brls::FocusDirection direction, brls::View* currentView)
{
    if (!m_dataSource || m_dataSource->getItemCount() == 0)
        return nullptr;

    void* parentUserData = currentView->getParentUserData();
    if (!parentUserData) return nullptr;

    size_t currentFocusIndex = *reinterpret_cast<size_t*>(parentUserData);
    int offset = 1;
    size_t dataCount = m_dataSource->getItemCount();

    if (direction == brls::FocusDirection::UP)
        offset = -spanCount;
    else if (direction == brls::FocusDirection::DOWN)
        offset = spanCount;
    else if (direction == brls::FocusDirection::LEFT)
        offset = -1;
    else if (direction == brls::FocusDirection::RIGHT)
        offset = 1;
    else
        return nullptr;

    int target = static_cast<int>(currentFocusIndex) + offset;
    if (target < 0 || static_cast<size_t>(target) >= dataCount)
    {
        View* next = getParentNavigationDecision(this, nullptr, direction);
        if (!next && hasParent()) next = getParent()->getNextFocus(direction, this);
        return next;
    }

    itemsRecyclingLoop();

    for (auto* v : m_contentBox->getChildren())
    {
        auto* item = dynamic_cast<RecyclingGridItem*>(v);
        if (item && item->getIndex() == static_cast<size_t>(target))
            return item->getDefaultFocus();
    }
    return nullptr;
}

// ── 分页 ───────────────────────────────────────────────────────

void RecyclingGrid::onNextPage(const std::function<void()>& callback)
{
    m_nextPageCallback = callback;
}

// ── 布局 ───────────────────────────────────────────────────────

void RecyclingGrid::onLayout()
{
    ScrollingFrame::onLayout();
    float width = getFrame().getWidth();
    if (width != width) return;
    if (!m_contentBox) return;
    m_contentBox->setWidth(width);
    if (checkWidth())
    {
        m_layouted = true;
        reloadData();
    }
}

bool RecyclingGrid::checkWidth()
{
    float width = getWidth();
    if (m_oldWidth == -1) m_oldWidth = width;
    if ((int)m_oldWidth != (int)width && width != 0)
    {
        m_oldWidth = width;
        return true;
    }
    m_oldWidth = width;
    return false;
}

// ── draw ───────────────────────────────────────────────────────

void RecyclingGrid::draw(NVGcontext* vg, float x, float y, float w, float h,
                          brls::Style style, brls::FrameContext* ctx)
{
    itemsRecyclingLoop();
    ScrollingFrame::draw(vg, x, y, w, h, style, ctx);
}

// ── Cell 回收循环 ──────────────────────────────────────────────

void RecyclingGrid::addCellAt(size_t index, bool downSide)
{
    if (!m_dataSource) return;

    for (auto* it : m_contentBox->getChildren())
    {
        auto* item = dynamic_cast<RecyclingGridItem*>(it);
        if (item && item->getIndex() == index) return;
    }

    RecyclingGridItem* cell = m_dataSource->cellForRow(this, index);
    if (!cell) return;

    float cellHeight = estimatedRowHeight;
    float cellWidth = (m_renderedFrame.getWidth() - m_paddingLeft - m_paddingRight) / spanCount
                      - cell->getMarginLeft() - cell->getMarginRight();
    float cellX = m_renderedFrame.getMinX() + m_paddingLeft;
    cellX += (m_renderedFrame.getWidth() - m_paddingLeft - m_paddingRight) / spanCount * (index % spanCount);

    cell->setWidth(cellWidth - estimatedRowSpace);
    cell->setHeight(cellHeight);
    cell->setDetachedPositionX(cellX);
    cell->setDetachedPositionY(getHeightByCellIndex(index) + m_paddingTop);
    cell->detach();
    cell->setIndex(index);

    m_contentBox->getChildren().insert(m_contentBox->getChildren().end(), cell);

    size_t* userdata = reinterpret_cast<size_t*>(malloc(sizeof(size_t)));
    *userdata = index;
    cell->setParent(m_contentBox, userdata);

    m_contentBox->invalidate();
    cell->View::willAppear();

    if (index < visibleMin) visibleMin = index;
    if (index > visibleMax) visibleMax = index;

    if (index % spanCount == 0)
    {
        if (!downSide)
            m_renderedFrame.origin.y -= cellHeight + estimatedRowSpace;
        m_renderedFrame.size.height += cellHeight + estimatedRowSpace;
    }
}

void RecyclingGrid::removeCell(brls::View* view)
{
    if (!view) return;

    auto& children = m_contentBox->getChildren();
    for (size_t i = 0; i < children.size(); i++)
    {
        if (children[i] == view)
        {
            children.erase(children.begin() + i);
            view->willDisappear(true);
            invalidate();
            return;
        }
    }
}

void RecyclingGrid::queueReusableCell(RecyclingGridItem* cell)
{
    m_queueMap.at(cell->reuseIdentifier)->push_back(cell);
    if (brls::Application::getCurrentFocus() != cell)
        cell->setParent(nullptr);
    cell->cacheForReuse();
}

void RecyclingGrid::itemsRecyclingLoop()
{
    if (!m_dataSource || m_dataSource->getItemCount() == 0) return;

    brls::Rect visibleFrame = getVisibleFrame();

    // 回收上方元素
    while (true)
    {
        RecyclingGridItem* minCell = nullptr;
        for (auto* it : m_contentBox->getChildren())
        {
            auto* item = dynamic_cast<RecyclingGridItem*>(it);
            if (item && item->getIndex() == visibleMin)
            {
                minCell = item;
                break;
            }
        }

        if (!minCell) break;
        if (minCell->getIndex() >= m_dataSource->getItemCount())
            break;

        float cellHeight = estimatedRowHeight;
        if (minCell->getDetachedPosition().y + cellHeight +
            getHeightByCellIndex(
                std::min(visibleMin + static_cast<size_t>(spanCount) * 3, m_dataSource->getItemCount()),
                visibleMin) >= visibleFrame.getMinY())
            break;

        m_renderedFrame.origin.y += minCell->getIndex() % spanCount == 0
            ? cellHeight + estimatedRowSpace : 0;
        m_renderedFrame.size.height -= minCell->getIndex() % spanCount == 0
            ? cellHeight + estimatedRowSpace : 0;

        queueReusableCell(minCell);
        removeCell(minCell);
        visibleMin++;
    }

    // 回收下方元素
    while (visibleMax > 0 && visibleMax < m_dataSource->getItemCount())
    {
        RecyclingGridItem* maxCell = nullptr;
        for (auto* it : m_contentBox->getChildren())
        {
            auto* item = dynamic_cast<RecyclingGridItem*>(it);
            if (item && item->getIndex() == visibleMax)
            {
                maxCell = item;
                break;
            }
        }

        if (!maxCell) break;
        if (visibleMax == 0) break;

        size_t compareIdx = visibleMax > static_cast<size_t>(spanCount) * 3
            ? visibleMax - static_cast<size_t>(spanCount) * 3 : 0;
        if (maxCell->getDetachedPosition().y -
            getHeightByCellIndex(visibleMax, compareIdx) <= visibleFrame.getMaxY())
            break;

        m_renderedFrame.size.height -= maxCell->getIndex() % spanCount == 0
            ? estimatedRowHeight + estimatedRowSpace : 0;

        queueReusableCell(maxCell);
        removeCell(maxCell);
        visibleMax--;
    }

    // 上方添加
    while (visibleMin > 0 && visibleMin - 1 < m_dataSource->getItemCount())
    {
        if ((visibleMin) % spanCount == 0)
        {
            if (m_renderedFrame.getMinY() +
                getHeightByCellIndex(
                    std::min(visibleMin + static_cast<size_t>(spanCount) * 3, m_dataSource->getItemCount()),
                    visibleMin) < visibleFrame.getMinY())
                break;
        }
        addCellAt(visibleMin - 1, false);
    }

    // 下方添加
    while (visibleMax + 1 < m_dataSource->getItemCount())
    {
        size_t nextIdx = visibleMax + 1;
        if ((nextIdx) % spanCount == 0)
        {
            if (m_renderedFrame.getMaxY() -
                getHeightByCellIndex(
                    nextIdx,
                    nextIdx > static_cast<size_t>(spanCount) * 3 ? nextIdx - static_cast<size_t>(spanCount) * 3 : 0) >
                visibleFrame.getMaxY())
            {
                m_requestNextPage = false;
                break;
            }
        }
        addCellAt(nextIdx, true);
    }

    // 触底触发 onNextPage
    if (visibleMax + 1 >= getItemCount())
    {
        if (!m_requestNextPage && m_nextPageCallback)
        {
            if (m_dataSource && m_dataSource->getItemCount() > 0)
            {
                m_requestNextPage = true;
                m_nextPageCallback();
            }
        }
    }
}

// ── 工厂 ───────────────────────────────────────────────────────

brls::View* RecyclingGrid::create() { return new RecyclingGrid(); }
