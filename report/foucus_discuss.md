# 焦点防错方案总结

## 概述

本文档总结在 borealis UI 框架下，切换页面和 View 时常见的焦点错误及其对应防错方案。

---

## borealis 焦点系统核心机制

### isFocusable() 规则
```cpp
bool View::isFocusable() {
    return this->focusable && this->visibility == Visibility::VISIBLE;
}
```
- 只有 **自身** 的 `focusable == true` **且** `visibility == VISIBLE` 时，该 view 才被视为可聚焦
- 父级 GONE 并 **不会** 使子级 `isFocusable()` 返回 false，需手动处理

### Box::getDefaultFocus() 遍历顺序
1. 若自身 `isFocusable()` → 返回自身
2. 若 `lastFocusedView` 存在且其子树有可聚焦项 → 返回该子树结果
3. 按 `defaultFocusedIndex` 查找
4. 遍历所有 `children` 找第一个可聚焦后代

### navigate() 中的可见性校验
```cpp
if (nextFocus->getDefaultFocus()
    && currentFocus != nextFocus->getDefaultFocus()
    && nextFocus->getVisibility() == Visibility::VISIBLE)
    Application::giveFocus(nextFocus);
else
    currentFocus->shakeHighlight(direction);  // 无效导航 → 仅抖动
```
- `setCustomNavigationRoute` 设置的 **目标 View** 必须 VISIBLE 才能跳转成功
- `giveFocus(target)` 内部再次调用 `target->getDefaultFocus()`，因此可以安全地传入容器 Box

---

## 常见焦点错误及防错方案

### 问题一：焦点停在容器 Box 而非内部 item

**现象**：按 RIGHT 后焦点落在子面板的 wrapper Box，需再操作一次才到具体控件。

**根因**：对 wrapper Box 调用了 `setFocusable(true)`，导致 `getDefaultFocus()` 在第 1 步就返回自身，不再向下遍历。

**方案**：
- wrapper Box **始终保持** `setFocusable(false)`
- 通过 `setCustomNavigationRoute(direction, wrapperBox)` 导航到容器时，borealis 会自动调用 `wrapperBox->getDefaultFocus()` 递归找到第一个可聚焦叶子节点（例如 LazyCell / GridItem）

```cpp
// ✅ 正确：wrapper 不可聚焦，导航自动递归到子节点
btn->setCustomNavigationRoute(brls::FocusDirection::RIGHT, sonPanel);
// sonPanel->setFocusable(false) （保持默认值即可）

// ❌ 错误：wrapper 设为可聚焦，焦点停在 wrapper 本身
sonPanel->setFocusable(true);
btn->setCustomNavigationRoute(brls::FocusDirection::RIGHT, sonPanel);
```

---

### 问题二：焦点进入 GONE 面板

**现象**：切换到读取状态 Button 并按 RIGHT，焦点却进入了已设为 GONE 的保存状态面板。

**根因**：
1. 保存面板中的 `LazyCell` 子项自身 `focusable=true` 且 `visibility=VISIBLE`（从未被显式修改），因此即使父级 wrapper GONE，`savePanel->getDefaultFocus()` 仍可遍历到这些 LazyCell
2. `Box::lastFocusedView` 记忆了上次聚焦在 savePanel，后续调用 `m_viewPanel->getDefaultFocus()` 时优先尝试 savePanel 子树，从而触发上述遍历

**方案**：
- **根本修复**：wrapper Box **不设** `setFocusable(true)`，使 `savePanel->getDefaultFocus()` 在 wrapper GONE 时无法通过 `nextFocus->getVisibility() == VISIBLE` 的检查，导航被安全拒绝（仅抖动）
- **辅助手段**：隐藏面板时同步将其内部可聚焦子项设为 `setFocusable(false)`，或将整个内容区设为 GONE，彻底断开焦点遍历路径

```cpp
// ✅ 方案：隐藏时同步清理子项可聚焦状态
void _hideAllPanels() {
    for (auto* panel : m_allPanels) {
        panel->setVisibility(brls::Visibility::GONE);
        panel->setFocusable(false);
        // 若内部有 LazyCell/GridItem 需要也设为 false：
        // for (auto* cell : lazyCells) cell->setFocusable(false);
    }
}
```

---

### 问题三：registerClickAction 缺少返回值导致崩溃

**现象**：点击 Button 时程序崩溃。

**根因**：`registerClickAction` 的 lambda 声明为 `-> bool` 但末尾无 `return` 语句，属于 C++ 未定义行为（UB），在 Release 模式下极易崩溃。

**方案**：所有声明了返回值的 lambda **必须**有明确的 `return` 语句。

```cpp
// ✅ 正确
btn->registerClickAction([this, sonPanel](brls::View*) -> bool {
    brls::Application::giveFocus(sonPanel);
    return true;
});

// ❌ 错误：UB，可能崩溃
btn->registerClickAction([this, sonPanel](brls::View*) -> bool {
    brls::Application::giveFocus(sonPanel);
    // 缺少 return true;
});
```

---

### 问题四：切换 View 时焦点溢出到外层容器

**现象**：在横向滚动卡片列表中按 UP/DOWN，焦点跳出了倒带界面，落到其他 UI 区域。

**根因**：borealis 的方向导航在当前视图树无法找到目标时会逐级向上传递，最终可能跳到父级或兄弟视图。

**方案**：对希望"锁定"在某个区域内的 item，通过 `setCustomNavigationRoute` 将不需要的方向指向自身，形成"焦点循环阻断"。

```cpp
// ✅ 禁止 UP/DOWN 导航离开当前 item 区域
for (auto* item : m_items) {
    item->setCustomNavigationRoute(brls::FocusDirection::UP,   item);
    item->setCustomNavigationRoute(brls::FocusDirection::DOWN, item);
}
```

---

### 问题五：面板切换后焦点未正确转移

**现象**：切换面板后焦点仍在旧面板，或新面板无法接收焦点。

**方案**：
1. 切换前先调用 `_hideAllPanels()` 隐藏所有面板
2. 显示目标面板后，通过 `brls::Application::giveFocus(targetPanel)` 或 `giveFocus(firstItem)` 明确转移焦点
3. 在 `brls::sync([]{...})` 内执行所有 UI 操作，避免在 UI 线程外修改焦点状态

```cpp
// ✅ 正确的面板切换 + 焦点转移模式
brls::sync([this, targetPanel]() {
    _hideAllPanels();
    targetPanel->setVisibility(brls::Visibility::VISIBLE);
    brls::Application::giveFocus(targetPanel); // getDefaultFocus() 自动找到第一个 item
});
```

---

### 问题六：异步刷新后焦点无效

**现象**：面板内容通过 `brls::async` 异步刷新后，item 尚未 focusable，导致导航时找不到焦点目标。

**方案**：
- 在 `brls::sync` 回调内（即异步完成后）才调用 `setFocusable(true)` 和 `giveFocus`
- 若 item 默认就是 focusable（如 LazyCell），无需手动设置；只需保证父级 wrapper 可见

```cpp
brls::async([this]() {
    // 耗时操作（扫描文件等）
    auto infos = scanSlots();
    brls::sync([this, infos]() {
        for (int i = 0; i < items.size(); ++i) {
            items[i]->setFocusable(true);  // 完成后才设为可聚焦
            items[i]->updateDisplay(infos[i]);
        }
    });
});
```

---

## 总结：切换页面/View 的焦点防错清单

| 场景 | 防错要点 |
|------|----------|
| 显示子面板 | wrapper Box 保持 `setFocusable(false)`，由 `getDefaultFocus()` 自动向下递归 |
| 隐藏子面板 | `setVisibility(GONE)` + `setFocusable(false)` 同时处理 wrapper 和关键子项 |
| 自定义导航路由 | 目标 View 必须 VISIBLE 且 `getDefaultFocus()` 返回非空，否则只会抖动 |
| 锁定焦点区域 | 对边界 item 的不需要方向设置 `setCustomNavigationRoute(dir, self)` |
| 点击/action 回调 | `-> bool` lambda **必须** 以 `return true/false;` 结尾，避免 UB |
| 所有 UI 操作 | 在 `brls::sync([]{...})` 内执行，确保在 UI 线程安全运行 |
| 耗时操作 | 用 `brls::async` 执行，结果通过 `brls::sync` 回传到 UI 线程再更新 |
