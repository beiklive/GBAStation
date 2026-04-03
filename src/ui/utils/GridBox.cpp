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
        // LazyCell 自身作为可聚焦元素，管理焦点路由
        this->setFocusable(true);
        this->setAxis(brls::Axis::ROW);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setJustifyContent(brls::JustifyContent::CENTER);

        // 注册 A 键点击动作
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
        // 首次绘制时执行延迟加载
        if (!m_loaded && m_factory)
        {
            m_loaded = true;
            brls::View* content = m_factory();
            if (content)
            {
                // 内容视图不参与焦点管理，由 LazyCell 统一持有焦点
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
        this->setHeightPercentage(100.f);
        this->setGrow(1.0f);

        // 垂直滚动容器，焦点移动时自动滚动保持聚焦单元格可见
        m_scrollFrame = new brls::ScrollingFrame();
        m_scrollFrame->setGrow(1.0f);
        m_scrollFrame->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);

        // 网格内容：垂直堆叠的行盒子
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
        rebuild();
    }

    void GridBox::addItem(std::function<brls::View*()> factory)
    {
        m_factories.push_back(std::move(factory));
        rebuild();
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
        m_gridContent->clearViews(true);
    }

    void GridBox::rebuild()
    {
        // 清除旧布局
        m_cells.clear();
        m_gridContent->clearViews(true);

        if (m_factories.empty() || m_columns <= 0)
            return;

        const int total   = static_cast<int>(m_factories.size());
        const int rowCount = (total + m_columns - 1) / m_columns;

        for (int row = 0; row < rowCount; ++row)
        {
            // 每行使用 ROW 方向的盒子
            auto* rowBox = new brls::Box(brls::Axis::ROW);
            rowBox->setFocusable(false);
            rowBox->setAlignItems(brls::AlignItems::CENTER);
            rowBox->setPaddingTop(5.0f);
            rowBox->setPaddingBottom(5.0f);

            m_cells.emplace_back(); // 新增一行

            for (int col = 0; col < m_columns; ++col)
            {
                int idx = row * m_columns + col;
                if (idx >= total)
                    break;

                auto* cell = new LazyCell(m_factories[idx], idx);
                cell->setMargins(5.0f, 5.0f, 5.0f, 5.0f);

                // 注入 GridBox 级别的回调
                cell->onClicked = [this](int index)
                {
                    if (onItemClicked)
                        onItemClicked(index);
                };
                cell->onFocused = [this](int index)
                {
                    if (onItemFocused)
                        onItemFocused(index);
                };

                rowBox->addView(cell);
                m_cells[row].push_back(cell);
            }

            m_gridContent->addView(rowBox);
        }

        // 完成布局后配置所有单元格的焦点路由
        setupNavigation();
    }

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

                // ── 向上 ──────────────────────────────────────
                {
                    // 目标行：上一行（到顶则循环到末行）
                    int targetRow = (row > 0) ? (row - 1) : (rowCount - 1);
                    // 目标列：同列，若目标行列数不足则取末列（防止空行越界）
                    int tColCount = static_cast<int>(m_cells[targetRow].size());
                    if (tColCount == 0)
                        continue;
                    int targetCol = std::min(col, tColCount - 1);
                    cell->setCustomNavigationRoute(brls::FocusDirection::UP,
                                                   m_cells[targetRow][targetCol]);
                }

                // ── 向下 ──────────────────────────────────────
                {
                    // 目标行：下一行（到底则循环到首行）
                    int targetRow = (row < rowCount - 1) ? (row + 1) : 0;
                    int tColCount = static_cast<int>(m_cells[targetRow].size());
                    if (tColCount == 0)
                        continue;
                    int targetCol = std::min(col, tColCount - 1);
                    cell->setCustomNavigationRoute(brls::FocusDirection::DOWN,
                                                   m_cells[targetRow][targetCol]);
                }

                // ── 向左 ──────────────────────────────────────
                {
                    // 目标列：左边一列（到头则循环到当前行末列）
                    int targetCol = (col > 0) ? (col - 1) : (colCount - 1);
                    cell->setCustomNavigationRoute(brls::FocusDirection::LEFT,
                                                   m_cells[row][targetCol]);
                }

                // ── 向右 ─────────────────────────────────────
                {
                    // 目标列：右边一列（到尾则循环到当前行首列）
                    int targetCol = (col < colCount - 1) ? (col + 1) : 0;
                    cell->setCustomNavigationRoute(brls::FocusDirection::RIGHT,
                                                   m_cells[row][targetCol]);
                }
            }
        }
    }

} // namespace beiklive
