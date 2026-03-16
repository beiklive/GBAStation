# Session 87 任务分析

## 任务目标

修改 `GameMenu` 中 leftbox 里的按钮行为：
- 除"返回游戏"和"退出游戏"外，其他按钮得到焦点时自动显示对应面板，隐藏其他面板
- "返回游戏"和"退出游戏"按钮得到焦点时，隐藏所有面板
- 按 A 键时，将焦点转入对应面板，使用户可以操作面板内容

## 输入分析

### 当前实现（改动前）

- `GameMenu` 的 leftbox 有四个按钮："返回游戏"、"金手指"、"画面设置"、"退出游戏"
- "金手指"和"画面设置"按钮通过 `BUTTON_A` 的回调切换对应面板的可见性（toggle 逻辑）
- 面板仅在按 A 键时才显示/隐藏，悬停/聚焦不触发任何面板变化

### 问题

- 当前行为需要用户主动按 A 才能看到面板，不符合"焦点触发显示"的交互预期
- 参考 TabFrame：侧边栏按钮聚焦时自动切换右侧内容区，更符合直觉

## 解决方案

### 方案选择

使用 `brls::View::getFocusEvent()->subscribe()` 监听焦点获取事件（borealis 提供的 `GenericEvent`），
在按钮聚焦时触发面板显示切换，代替原有的 BUTTON_A toggle 逻辑。

这与 TabFrame / Sidebar 的实现思路相同（SidebarItem 在 `onFocusGained()` 中触发 `activeEvent`），
但无需新增组件，直接在 GameMenu 内通过事件订阅实现。

### 具体修改

1. 在构造函数内定义 `hideAllPanels` 辅助 lambda（捕获 `this`，隐藏所有面板）
2. **"返回游戏"按钮**：订阅 focusEvent → 隐藏所有面板；BUTTON_A → 关闭菜单（不变）
3. **"金手指"按钮**：订阅 focusEvent → 显示金手指面板；BUTTON_A → `giveFocus` 到面板
4. **"画面设置"按钮**：订阅 focusEvent → 显示画面设置面板；BUTTON_A → `giveFocus` 到面板
5. **"退出游戏"按钮**：订阅 focusEvent → 隐藏所有面板；BUTTON_A → 退出 Activity（不变）

### 导航流程

```
[用户悬停金手指按钮] → focusEvent → 金手指面板出现
[用户按 A 键]       → BUTTON_A  → 焦点进入金手指面板
[用户在面板内操作]                  → 正常操作 BooleanCell 等
[用户按 B 键/左键]  → 自然导航  → 焦点返回侧边栏按钮
[焦点回到金手指按钮] → focusEvent → 金手指面板继续显示
[用户移动到画面设置] → focusEvent → 画面设置面板出现，金手指面板隐藏
```

## 可能的挑战

- `hideAllPanels` lambda 在被其他 lambda 捕获时，`m_cheatScrollFrame`/`m_displayScrollFrame`
  还未初始化（为 nullptr）。已通过 null 检查（`if (m_cheatScrollFrame)`）处理。
- `brls::Application::giveFocus(scrollFrame)` 需要 ScrollingFrame 及其内容可见且可聚焦，
  已添加可见性判断（`getVisibility() == VISIBLE`）作为保护。
