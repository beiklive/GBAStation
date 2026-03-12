# Session 45 - SettingPage 标签页内容不显示问题分析与修复

## 问题描述

`SettingPage` 中存在侧边栏（Sidebar），对应的 `buildXxx()` 函数在选中标签时确实被调用，
但屏幕上不显示任何内容面板（只有侧边栏可见）。

涉及标签：
- 界面设置 → `buildUITab()`
- 游戏设置 → `buildGameTab()`
- 画面设置 → `buildDisplayTab()`
- 声音设置 → `buildAudioTab()`
- 按键预设 → `buildKeyBindTab()`
- 调试工具 → `buildDebugTab()`

---

## 根因分析

### 涉及文件

| 文件 | 说明 |
|------|------|
| `src/UI/Pages/SettingPage.cpp` | SettingPage 构造函数与标签构建函数 |
| `third_party/borealis/library/lib/views/tab_frame.cpp` | TabFrame 实现 |
| `third_party/borealis/library/lib/core/box.cpp` | Box / inflateFromXMLString 实现 |

### 布局层级

```
AppletFrame
  └── SettingPage (beiklive::UI::BBox → brls::Box, axis=ROW, grow=1.0 by AppletFrame)
        └── TabFrame (brls::Box, axis=ROW, width="auto", height="auto")
              ├── Sidebar  (fixed width = @style/brls/tab_frame/sidebar_width)
              └── [content view added here when tab selected, grow=1.0]
```

### 问题所在

`TabFrame` 通过 `inflateFromXMLString` 自身初始化时，XML 中设置了
`width="auto"` 和 `height="auto"`。在 Yoga flexbox 布局中，`auto` 表示
元素宽度由其子节点内容决定。

由于 `SettingPage::SettingPage()` 中**从未对 `m_tabframe` 调用
`setGrow(1.0f)`**，TabFrame 不会扩展填充父容器（SettingPage）的可用空间，
其宽度仅等于侧边栏的固定宽度。

当用户选择标签后，`TabFrame::addTab` 的回调将内容视图以
`newContent->setGrow(1.0f)` 加入 TabFrame，但 TabFrame 内可用宽度为 **0**
（全部空间已被侧边栏占满），内容视图虽被创建但无法渲染，画面上不可见。

这解释了现象：
- 侧边栏可见（它有固定宽度，不依赖 grow）
- `buildXxx()` 被调用（TabFrame 确实触发了回调，内容视图被创建）
- 内容区域不可见（TabFrame 没有足够空间容纳 grow=1.0 的内容视图）

---

## 修复方案

**文件：** `src/UI/Pages/SettingPage.cpp`

**修改前：**
```cpp
SettingPage::SettingPage()
{
    m_tabframe = new brls::TabFrame();
    Init();
    addView(m_tabframe);
}
```

**修改后：**
```cpp
SettingPage::SettingPage()
{
    m_tabframe = new brls::TabFrame();
    m_tabframe->setGrow(1.0f);   // ← 使 TabFrame 填满 SettingPage 的可用宽度
    Init();
    addView(m_tabframe);
}
```

`setGrow(1.0f)` 使 TabFrame 在其父容器（SettingPage，axis=ROW）中
扩展以占满全部可用宽度，TabFrame 内部的 Sidebar 仍占固定宽度，
右侧剩余空间由内容视图（grow=1.0）填充，标签页内容得以正确显示。

---

## 变更文件

- `src/UI/Pages/SettingPage.cpp`：在 `SettingPage::SettingPage()` 中
  增加 `m_tabframe->setGrow(1.0f);`（+1 行）

---

## 验证

修复后，`SettingPage` 各标签页内容区域应在切换标签时正常显示在侧边栏右侧，
与 `brls::TabFrame` 的设计意图一致。
