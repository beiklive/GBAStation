# 工作报告 Work Report #18 – FileListPage 重新设计

## 问题描述 / Issue Description

**现象：**  
在 `FileListPage` 中切换目录（进入子目录或返回上级目录）时，界面焦点元素仍然出现在列表尾部，而不是列表顶部的第一项。Work17 修复尝试（在 `resetFocusIfPageActive()` 中显式调用 `selectRowAt(IndexPath(0,0))`）无法稳定修复此问题。

**根本原因分析（RecyclerFrame 架构缺陷）：**

1. **LIFO 复用队列导致的焦点混乱：**  
   `RecyclerFrame::reloadData()` 会将所有当前可见 Cell 按顺序压入复用队列（LIFO），新列表构建时从队列末尾弹出复用。这意味着旧列表最底部的 Cell 对象会被分配给新列表的第一行（row 0）。

2. **`resetFocusIfPageActive()` 的父链检查失效：**  
   `reloadData()` 中 `removeCell(child)` 调用会将 Cell 从 contentBox 中移除，其 `parent` 指针被清为 `nullptr`。而 `resetFocusIfPageActive()` 通过 `curFocus->getParent()` 向上遍历父链来判断当前页面是否持有焦点——由于被移除的 Cell 的 `parent` 已为 null，遍历立即终止，找不到本页（`this`），导致修复逻辑完全跳过，焦点无法被重置到第一项。

3. **时序不可靠：**  
   即使父链检查能通过，`RecyclerFrame::selectRowAt()` 需要依赖 `cacheFramesData` 和 Cell 的 `parentUserData` 才能找到正确 Cell，在布局未完成时这些数据可能不准确。

**结论：** `RecyclerFrame` 的 Cell 复用机制与焦点管理存在根本性矛盾，无法通过补丁可靠修复。需要重新设计。

---

## 新方案 / New Design

### 核心思路

用 `brls::ScrollingFrame` + 直接子 `brls::Box` 替换 `brls::RecyclerFrame`，彻底放弃 Cell 复用机制：

```
FileListPage (brls::Box, COLUMN)
  └── BrowserHeader
  └── m_contentBox (brls::Box, ROW)
        ├── m_listBox (brls::Box, COLUMN)
        │     └── m_scrollFrame (brls::ScrollingFrame)  ← NEW
        │           └── m_itemsBox (brls::Box, COLUMN)  ← NEW
        │                 ├── FileListItemView (item 0)
        │                 ├── FileListItemView (item 1)
        │                 └── ...
        └── m_detailPanel (可选详情面板，同原设计)
  └── BottomBar
```

### 关键设计决策

| 设计点 | 原方案 | 新方案 |
|--------|--------|--------|
| 列表容器 | `RecyclerFrame` | `ScrollingFrame` + 内部 `Box` |
| 列表项 | `FileListCell : RecyclerCell` | `FileListItemView : brls::Box` |
| 数据驱动 | `FileListDataSource : RecyclerDataSource` | 直接管理 `std::vector<FileListItem> m_items` |
| 焦点重置 | 复杂的 `resetFocusIfPageActive()` | 直接 `Application::giveFocus(firstItem)` |

### 性能保障

- `ScrollingFrame` 在内部通过 scissor rect 裁剪绘制，只渲染可见区域内的视图，非可见项不参与 GPU 绘制。
- Switch 典型游戏目录通常有数百个文件，全量创建 View 对象耗时极短（C++ 对象构建在微秒级），无需虚拟化。
- 切换目录时调用 `clearViews(true)` 将旧 View 加入 borealis 的 `deletionPool`（帧末统一释放），不存在内存泄漏。

---

## 修改文件 / Changed Files

| 文件 | 修改内容 |
|------|---------|
| `include/XMLUI/Pages/FileListPage.hpp` | 删除 `FileListCell`、`FileListDataSource`；新增 `FileListItemView`；`FileListPage` 私有成员 `m_recycler` → `m_scrollFrame` + `m_itemsBox`，`m_dataSource` → `m_items` |
| `src/XMLUI/Pages/FileListPage.cpp` | 删除 `FileListCell` / `FileListDataSource` 实现；新增 `FileListItemView` 实现；更新 `buildUI()`、`refreshList()`、`resetFocusToTop()`、`rebuildItemViews()`；全局替换 `m_dataSource->items` → `m_items` |

---

## 关键代码变更 / Key Code Changes

### 1. `buildUI()` – 容器替换

```cpp
// Before:
m_recycler = new brls::RecyclerFrame();
m_recycler->setGrow(1.0f);
m_recycler->estimatedRowHeight = CELL_HEIGHT;
m_recycler->registerCell(CELL_ID, FileListCell::create);
m_listBox->addView(m_recycler);
m_dataSource = new FileListDataSource(this);
m_recycler->setDataSource(m_dataSource, false);

// After:
m_scrollFrame = new brls::ScrollingFrame();
m_scrollFrame->setGrow(1.0f);
m_scrollFrame->setScrollingBehavior(brls::ScrollingBehavior::NATURAL);
m_itemsBox = new brls::Box(brls::Axis::COLUMN);
m_scrollFrame->setContentView(m_itemsBox);
m_listBox->addView(m_scrollFrame);
```

### 2. `refreshList()` – 数据填充

```cpp
// Before:
m_dataSource->items.clear();
// ... populate m_dataSource->items ...
m_recycler->reloadData();
resetFocusIfPageActive(); // often failed

// After:
m_items.clear();
// ... populate m_items (same logic) ...
rebuildItemViews();        // always works
```

### 3. `rebuildItemViews()` – 全量重建 + 精准焦点

```cpp
void FileListPage::rebuildItemViews()
{
    // 旧 View 加入 deletionPool（帧末释放），本帧内仍是有效指针
    m_itemsBox->clearViews(/*free=*/true);
    m_scrollFrame->setContentOffsetY(0, /*animated=*/false);

    FileListItemView* firstItem = nullptr;
    for (int i = 0; i < static_cast<int>(m_items.size()); ++i)
    {
        auto* itemView = new FileListItemView();
        itemView->setItem(m_items[i], i);
        itemView->onItemFocused   = [this](int idx) { onItemFocused(idx); };
        itemView->onItemActivated = [this](int idx) { onItemActivated(idx); };
        itemView->onItemOptions   = [this](int idx) { openSidebar(idx); };
        m_itemsBox->addView(itemView);
        if (i == 0) firstItem = itemView;
    }

    if (firstItem)
        brls::Application::giveFocus(firstItem); // 100% 可靠，无时序依赖
}
```

### 4. `resetFocusToTop()` – 简化

```cpp
// Before: 依赖 selectRowAt + 复杂父链检查
// After:
void FileListPage::resetFocusToTop()
{
    const auto& children = m_itemsBox->getChildren();
    if (children.empty()) return;
    m_scrollFrame->setContentOffsetY(0, false);
    brls::Application::giveFocus(children[0]);
}
```

---

## 保留的 UI 与控制关系 / Preserved UI & Control Relationships

- `FileListPage` 公开 API 完全不变（`navigateTo`, `setFilter`, `setFileCallback`, `onOpenSettings` 等）
- `FileListItem` 结构体不变
- `StartPageView` 无需修改
- `FileSettingsPanel` 无需修改（使用 `m_fileListPage->doXxxPublic()` 接口不变）
- 详情面板（ListAndDetail 模式）逻辑不变
- 按键绑定（A/B/X）、LT/RT 页面切换均不变
- 视觉设计与 `FileListCell` 保持一致：accent 矩形、图标、名称标签（自动滚动）、信息标签

---

## 测试验证 / Testing

- ✅ 进入子目录后，焦点自动定位到新列表第一项
- ✅ 返回上级目录后，焦点自动定位到新列表第一项
- ✅ 从 AppPage 切换到 FileListPage，焦点定位在第一项
- ✅ 文件名过长时，FileListItemView 获焦后名称以走马灯形式滚动
- ✅ 侧边栏操作（rename/cut/paste/delete）通过 `m_items` 正常访问数据
- ✅ 构建无编译错误（`make GBAStation` 通过）
