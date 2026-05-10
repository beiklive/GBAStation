#include "GridBox.hpp"

#include <algorithm>

namespace beiklive
{

    // ═══════════════════════════════════════════════════════════════════════
    // LazyCell
    // ═══════════════════════════════════════════════════════════════════════

    LazyCell::LazyCell(std::function<brls::View*()> factory, int index)
        : m_factory(std::move(factory))
        , m_index(index)
    {
        this->setFocusable(true);
        this->setAxis(brls::Axis::ROW);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);

        this->registerAction(
            "确认",
            brls::BUTTON_A,
            [this](brls::View*) -> bool
            {
                if (onClicked)
                    onClicked(m_index);
                return true;
            },
            false,
            false,
            brls::SOUND_CLICK);
    }

    void LazyCell::draw(NVGcontext* vg, float x, float y, float w, float h,
                        brls::Style style, brls::FrameContext* ctx)
    {
        if (!m_loaded && m_factory)
        {
            m_loaded = true;
            brls::View* content = m_factory();
            if (content)
            {
                content->setFocusable(false);
                this->addView(content);
                this->invalidate();
            }
        }

        brls::Box::draw(vg, x, y, w, h, style, ctx);
    }

    void LazyCell::onFocusGained()
    {
        brls::Box::onFocusGained();
        if (onFocused)
            onFocused(m_index);
    }

    void LazyCell::onFocusLost()
    {
        brls::Box::onFocusLost();
    }

    // ═══════════════════════════════════════════════════════════════════════
    // GridBox
    // ═══════════════════════════════════════════════════════════════════════

    GridBox::GridBox(int columns)
        : m_columns(columns)
    {
        this->setAxis(brls::Axis::COLUMN);
        this->setWidthPercentage(100.f);
        this->setGrow(1.0f);
        this->setFocusable(false);

        m_scrollFrame = new brls::ScrollingFrame();
        m_scrollFrame->setGrow(1.0f);
        m_scrollFrame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);
        m_scrollFrame->setScrollingIndicatorVisible(false);
        m_scrollFrame->setFocusable(false);

        m_gridContent = new brls::Box(brls::Axis::COLUMN);
        m_gridContent->setPadding(5.0f);

        m_scrollFrame->setContentView(m_gridContent);
        this->addView(m_scrollFrame);
    }

    void GridBox::setColumns(int columns)
    {
        if (m_columns == columns)
            return;
        m_columns = columns;
        m_renderedCount = 0;
        renderAll();
    }

    void GridBox::addItem(std::function<brls::View*()> factory)
    {
        m_factories.push_back(std::move(factory));
    }

    void GridBox::commit()
    {
        renderAll();
    }

    void GridBox::commitAppend()
    {
        renderMore(static_cast<int>(m_factories.size()));
    }

    brls::View *GridBox::getItemView(int index) const
    {
        if (index < 0 || index >= static_cast<int>(m_factories.size()))
            return nullptr;
        int row = index / m_columns;
        int col = index % m_columns;
        if (row >= static_cast<int>(m_cells.size()) || col >= static_cast<int>(m_cells[row].size()))
            return nullptr;
        return m_cells[row][col];
    }

    void GridBox::clearItems()
    {
        m_factories.clear();
        m_cells.clear();
        m_rowBoxes.clear();
        m_gridContent->clearViews(true);
        m_renderedCount = 0;
    }

    // ── 内部工厂函数 ───────────────────────────────────────────────────────

    LazyCell* GridBox::makeCell(int idx, float widthPercent)
    {
        auto* cell = new LazyCell(m_factories[idx], idx);
        if (m_hideHightlight)
            HIDE_BRLS_HIGHLIGHT(cell);
        cell->setMargins(5.0f, 5.0f, 5.0f, 5.0f);
        cell->setWidthPercentage(widthPercent);
        cell->onClicked = [this](int index) {
            if (onItemClicked) onItemClicked(index);
        };
        cell->onFocused = [this](int index) {
            if (onItemFocused) onItemFocused(index);
        };
        return cell;
    }

    brls::Box* GridBox::makeRowBox()
    {
        auto* rowBox = new brls::Box(brls::Axis::ROW);
        rowBox->setGrow(1.0f);
        rowBox->setFocusable(false);
        rowBox->setAlignItems(brls::AlignItems::CENTER);
        rowBox->setJustifyContent(brls::JustifyContent::CENTER);
        rowBox->setPaddingTop(5.0f);
        rowBox->setPaddingBottom(5.0f);
        return rowBox;
    }

    // ── 全量渲染 ───────────────────────────────────────────────────────────

    void GridBox::renderAll()
    {
        m_cells.clear();
        m_rowBoxes.clear();
        m_gridContent->clearViews(true);

        int total = static_cast<int>(m_factories.size());
        if (total <= 0 || m_columns <= 0)
        {
            m_renderedCount = 0;
            return;
        }

        int rowCount = (total + m_columns - 1) / m_columns;
        float widthPercent = 96.f / m_columns;

        for (int row = 0; row < rowCount; ++row)
        {
            brls::Box* rowBox = makeRowBox();

            m_cells.emplace_back();
            m_rowBoxes.push_back(rowBox);

            for (int col = 0; col < m_columns; ++col)
            {
                int idx = row * m_columns + col;
                if (idx >= total)
                    break;

                LazyCell* cell = makeCell(idx, widthPercent);
                rowBox->addView(cell);
                m_cells[row].push_back(cell);
            }

            m_gridContent->addView(rowBox);
        }

        m_renderedCount = total;
        setupNavigation();
    }

    // ── 增量渲染 ───────────────────────────────────────────────────────────

    void GridBox::renderMore(int newTotal)
    {
        int total = static_cast<int>(m_factories.size());
        newTotal = std::min(newTotal, total);
        if (newTotal <= m_renderedCount)
            return;

        float widthPercent = 96.f / m_columns;
        int idx = m_renderedCount;
        int remaining = newTotal - m_renderedCount;

        // 先填满末行（如果有空位）
        int lastRow = static_cast<int>(m_rowBoxes.size()) - 1;
        if (lastRow >= 0)
        {
            int lastRowCells = static_cast<int>(m_cells[lastRow].size());
            if (lastRowCells < m_columns)
            {
                brls::Box* rowBox = m_rowBoxes[lastRow];
                int space = m_columns - lastRowCells;
                int addToRow = std::min(space, remaining);

                for (int i = 0; i < addToRow; ++i)
                {
                    LazyCell* cell = makeCell(idx, widthPercent);
                    rowBox->addView(cell);
                    m_cells[lastRow].push_back(cell);
                    ++idx;
                    --remaining;
                }
            }
        }

        // 创建新行
        while (remaining > 0)
        {
            brls::Box* rowBox = makeRowBox();

            m_cells.emplace_back();
            m_rowBoxes.push_back(rowBox);

            int addToRow = std::min(m_columns, remaining);
            for (int col = 0; col < addToRow; ++col)
            {
                LazyCell* cell = makeCell(idx, widthPercent);
                rowBox->addView(cell);
                m_cells.back().push_back(cell);
                ++idx;
                --remaining;
            }

            m_gridContent->addView(rowBox);
        }

        m_renderedCount = newTotal;
        setupNavigation();
    }

    // ── 导航设置 ───────────────────────────────────────────────────────────

    void GridBox::setupNavigation()
    {
        const int rowCount = static_cast<int>(m_cells.size());
        if (rowCount == 0)
            return;

        for (int row = 0; row < rowCount; ++row)
        {
            const int colCount = static_cast<int>(m_cells[row].size());

            for (int col = 0; col < colCount; ++col)
            {
                LazyCell* cell = m_cells[row][col];

                // 向上
                {
                    int targetRow = (row > 0) ? (row - 1) : (rowCount - 1);
                    int tColCount = static_cast<int>(m_cells[targetRow].size());
                    if (tColCount == 0)
                        continue;
                    int targetCol = std::min(col, tColCount - 1);
                    cell->setCustomNavigationRoute(brls::FocusDirection::UP,
                                                   m_cells[targetRow][targetCol]);
                }

                // 向下
                {
                    int targetRow = (row < rowCount - 1) ? (row + 1) : 0;
                    int tColCount = static_cast<int>(m_cells[targetRow].size());
                    if (tColCount == 0)
                        continue;
                    int targetCol = std::min(col, tColCount - 1);
                    cell->setCustomNavigationRoute(brls::FocusDirection::DOWN,
                                                   m_cells[targetRow][targetCol]);
                }

                // 向左
                {
                    int targetCol = (col > 0) ? (col - 1) : (colCount - 1);
                    cell->setCustomNavigationRoute(brls::FocusDirection::LEFT,
                                                   m_cells[row][targetCol]);
                }

                // 向右
                {
                    int targetCol = (col < colCount - 1) ? (col + 1) : 0;
                    cell->setCustomNavigationRoute(brls::FocusDirection::RIGHT,
                                                   m_cells[row][targetCol]);
                }
            }
        }
    }

} // namespace beiklive
