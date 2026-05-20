#include "FileListView.hpp"

namespace beiklive {

// ===== FileListDataSource =====

int FileListDataSource::numberOfRows(brls::RecyclerFrame* recycler, int section) {
    return count();
}

brls::RecyclerCell* FileListDataSource::cellForRow(brls::RecyclerFrame* recycler, brls::IndexPath index) {
    auto* cell = dynamic_cast<beiklive::ListItemCell*>(recycler->dequeueReusableCell("Cell"));
    if (!cell) return nullptr;

    const auto& item = m_items[index.row];
    cell->setTitle(item.text);
    cell->setSubTitle(item.subText);
    cell->setIcon(item.iconPath);
    cell->setFullData(item.data);

    if (onBindCell)
        onBindCell(*cell);

    return cell;
}

void FileListDataSource::didSelectRowAt(brls::RecyclerFrame* recycler, brls::IndexPath indexPath) {
    if (onItemClicked && indexPath.row < (int)m_items.size())
        onItemClicked(m_items[indexPath.row]);
}

void FileListDataSource::setItems(const std::vector<beiklive::ListItem>& items) {
    m_items = items;
}

void FileListDataSource::appendItems(const std::vector<beiklive::ListItem>& items) {
    m_items.insert(m_items.end(), items.begin(), items.end());
}

void FileListDataSource::clear() {
    m_items.clear();
}

const beiklive::ListItem& FileListDataSource::getItem(int index) const {
    return m_items[index];
}

// ===== FileListView =====

FileListView::FileListView() {
    this->setAxis(brls::Axis::COLUMN);
    this->setWidthPercentage(100);
    this->setGrow(1.f);
    this->setFocusable(false);
    m_dataSource = new FileListDataSource();

    m_recycler = new brls::RecyclerFrame();
    m_recycler->estimatedRowHeight = 66.f;
    m_recycler->setPadding(20.f);
    m_recycler->setScrollingIndicatorVisible(false);
    m_recycler->setGrow(1.f);
    m_recycler->setScrollingBehavior(brls::ScrollingBehavior::CENTERED);

    m_recycler->registerCell("Cell", []() {
        return new beiklive::ListItemCell();
    });
    m_recycler->setDataSource(m_dataSource, false);

    this->addView(m_recycler);

    // // Lock focus inside the list
    // this->setCustomNavigationRoute(brls::FocusDirection::UP, this);
    // this->setCustomNavigationRoute(brls::FocusDirection::DOWN, this);
    // this->setCustomNavigationRoute(brls::FocusDirection::LEFT, this);
    // this->setCustomNavigationRoute(brls::FocusDirection::RIGHT, this);

    // UP/DOWN: single-step (fired once), long-press handled in frame()
    this->registerAction("上", brls::BUTTON_UP, [this](brls::View*) {
        moveUp();
        return true;
    }, false, false, brls::SOUND_FOCUS_CHANGE);

    this->registerAction("下", brls::BUTTON_DOWN, [this](brls::View*) {
        moveDown();
        return true;
    }, false, false, brls::SOUND_FOCUS_CHANGE);

    // LEFT/RIGHT: page up/down (Switch/VitaShell style)
    this->registerAction("上页", brls::BUTTON_LEFT, [this](brls::View*) {
        movePageUp();
        return true;
    }, false, false, brls::SOUND_FOCUS_CHANGE);

    this->registerAction("下页", brls::BUTTON_RIGHT, [this](brls::View*) {
        movePageDown();
        return true;
    }, false, false, brls::SOUND_FOCUS_CHANGE);

    m_lastFrameTime = std::chrono::steady_clock::now();
}

void FileListView::frame(brls::FrameContext* ctx) {
    brls::Box::frame(ctx);

    // Only handle long press when this list has focus
    if (!this->isFocused() && !this->isChildFocused())
        return;

    auto now = std::chrono::steady_clock::now();
    float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
    m_lastFrameTime = now;
    if (dt <= 0.f || dt > 0.5f) dt = 0.016f;

    auto& state = brls::Application::getControllerState();

    // ── Long press UP ──
    bool upNow = state.buttons[brls::BUTTON_UP];
    if (upNow && !m_prevUp) {
        m_holdUpTime = 0.f;
        m_holdUpRepeat = 0.f;
    }
    if (upNow) {
        m_holdUpTime += dt;
        if (m_holdUpTime > HOLD_INITIAL_DELAY) {
            m_holdUpRepeat += dt;
            float interval = m_holdUpTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdUpRepeat >= interval) {
                m_holdUpRepeat -= interval;
                moveUp();
            }
        }
    }
    m_prevUp = upNow;

    // ── Long press DOWN ──
    bool downNow = state.buttons[brls::BUTTON_DOWN];
    if (downNow && !m_prevDown) {
        m_holdDownTime = 0.f;
        m_holdDownRepeat = 0.f;
    }
    if (downNow) {
        m_holdDownTime += dt;
        if (m_holdDownTime > HOLD_INITIAL_DELAY) {
            m_holdDownRepeat += dt;
            float interval = m_holdDownTime > HOLD_ACCEL_TIME ? HOLD_REPEAT_FAST : HOLD_REPEAT;
            while (m_holdDownRepeat >= interval) {
                m_holdDownRepeat -= interval;
                moveDown();
            }
        }
    }
    m_prevDown = downNow;
}

brls::View* FileListView::getDefaultFocus() {
    if (m_totalItemCount > 0)
        m_recycler->setDefaultCellFocus(brls::IndexPath(0, m_focusedIndex));
    return m_recycler;
}

// ── Data ──

void FileListView::appendItems(const std::vector<beiklive::ListItem>& items) {
    if (items.empty()) return;
    bool firstBatch = (m_totalItemCount == 0);
    m_dataSource->appendItems(items);
    m_totalItemCount = m_dataSource->count();
    m_recycler->reloadData();
    if (firstBatch && m_totalItemCount > 0)
        setFocusedIndex(0);
}

void FileListView::clearItems() {
    m_dataSource->clear();
    m_recycler->reloadData();
    m_focusedIndex = 0;
    m_totalItemCount = 0;
}

void FileListView::finishLoading() {
    m_totalItemCount = m_dataSource->count();
    m_recycler->reloadData();
    brls::sync([this]() {
        if (m_totalItemCount > 0)
            setFocusedIndex(0);
    });
}

int FileListView::itemCount() const {
    return m_totalItemCount;
}

void FileListView::setOnItemClicked(std::function<void(const beiklive::ListItem&)> cb) {
    m_dataSource->onItemClicked = std::move(cb);
}

void FileListView::setOnBindCell(std::function<void(beiklive::ListItemCell&)> cb) {
    m_dataSource->onBindCell = std::move(cb);
}

// ── Focus movement ──

void FileListView::setFocusedIndex(int newIndex) {
    if (newIndex < 0 || m_totalItemCount == 0) return;
    if (newIndex >= m_totalItemCount)
        newIndex = m_totalItemCount - 1;
    if (newIndex == m_focusedIndex) return;

    int oldIndex = m_focusedIndex;
    m_focusedIndex = newIndex;

    if (onItemFocusLost && oldIndex < m_dataSource->count())
        onItemFocusLost(m_dataSource->getItem(oldIndex));

    m_recycler->setDefaultCellFocus(brls::IndexPath(0, m_focusedIndex));

    if (!m_recycler->isFocused() && !m_recycler->isChildFocused())
        brls::Application::giveFocus(m_recycler);

    m_recycler->selectRowAt(brls::IndexPath(0, m_focusedIndex), false);

    if (onItemFocused && m_focusedIndex < m_dataSource->count())
        onItemFocused(m_dataSource->getItem(m_focusedIndex));
}

void FileListView::moveUp() {
    if (m_focusedIndex > 0)
        setFocusedIndex(m_focusedIndex - 1);
}

void FileListView::moveDown() {
    if (m_focusedIndex < m_totalItemCount - 1)
        setFocusedIndex(m_focusedIndex + 1);
}

void FileListView::movePageUp() {
    setFocusedIndex(m_focusedIndex - PAGE_STEP);
}

void FileListView::movePageDown() {
    setFocusedIndex(m_focusedIndex + PAGE_STEP);
}

void FileListView::moveTo(int index) {
    setFocusedIndex(index);
}

} // namespace beiklive
