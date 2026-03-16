# Session 92 任务分析文档

## 任务目标

修改 GameMenu 相关代码，解决以下5个需求：

1. 保存和读取状态的槽位列表不再使用 ScrollingFrame，改为普通 Box
2. "返回游戏"和"退出游戏"按钮无子面板，但按右键时焦点仍会移动到右侧，需阻止该行为
3. 移除 GameMenu leftbox 里面按钮的边框（高亮边框）
4. 完善 GameMenu.cpp 的国际化显示（替换硬编码中文字符串）
5. 当焦点未移动到保存/读取状态子面板时，状态预览图和"无数据"标签均不显示；只有槽位按钮获得焦点时才显示

## 涉及文件

- `src/UI/Utils/GameMenu.cpp` - 主要修改文件
- `include/UI/Utils/GameMenu.hpp` - 成员变量调整
- `resources/i18n/zh-Hans/beiklive.json` - 新增国际化 key
- `resources/i18n/en-US/beiklive.json` - 新增国际化 key（英文）

## 需求分析

### 需求1：ScrollingFrame → Box
**当前结构：**
- `m_saveStatePanel` (Row Box)
  - `m_saveStateScrollFrame` (ScrollingFrame)
    - `m_saveStateItemBox` (Column Box，含10个槽位按钮)
  - `savePreviewBox` (Column Box，含预览图)

**目标结构：**
- `m_saveStatePanel` (Row Box)
  - `m_saveStateItemBox` (Column Box，含10个槽位按钮，直接添加)
  - `savePreviewBox` (Column Box，含预览图)

**修改点：**
- hpp：删除 `m_saveStateScrollFrame` 和 `m_loadStateScrollFrame` 成员
- cpp：删除 ScrollingFrame 的创建和配置代码，将 ItemBox 直接加入 StatePanel
- A键回调中把 `giveFocus(scrollFrame)` 改为 `giveFocus(itemBox)`

### 需求2：阻止右键导航
**方案：** 为"返回游戏"和"退出游戏"按钮注册 `BUTTON_NAV_RIGHT` action，消费该事件，阻止 borealis 的导航逻辑。
```cpp
btn->registerAction("", brls::BUTTON_NAV_RIGHT, [](brls::View*) { return true; }, true);
```

### 需求3：移除按钮高亮边框
**方案：** 调用 `setHideHighlightBorder(true)` 或使用 `BUTTONSTYLE_BORDERLESS`。
分析：
- `BUTTONSTYLE_DEFAULT` 本身没有 `borderThickness`，但仍有 highlight border（焦点时的矩形高亮框）
- `setHideHighlightBorder(true)` 可隐藏该高亮框
- 对 leftBox 中所有按钮调用此方法

### 需求4：国际化
**需新增的 i18n key（gamemenu 节点下）：**
- `title`: "Game Menu" / "Game Menu"
- `btn_return_game`: "返回游戏" / "Return to Game"
- `btn_cheats`: "金手指" / "Cheats"
- `btn_exit_game`: "退出游戏" / "Exit Game"
- `no_cheats`: "无金手指" / "No Cheats"

### 需求5：焦点未在子面板时隐藏预览区
**当前行为：** `refreshStatePanels()` 被调用时，`m_saveNoDataLabel` 显示，`m_savePreviewImage` 隐藏。
**目标行为：** 面板打开时（`refreshStatePanels()` 调用时），两者都隐藏；只有槽位按钮获得焦点时，才根据存档情况显示图片或"无数据"标签。

**修改点：**
- `refreshStatePanels()` 中将 `noDataLabel->setVisibility(GONE)` 替换原来的 `VISIBLE`

## 潜在挑战

- ScrollingFrame 和 Box 的行为差异：Box 不会自动滚动，需确保10个槽位按钮在面板可见区域内
- 阻止右键导航时需选用正确的按钮常量（`BUTTON_NAV_RIGHT` 而非 `BUTTON_RIGHT`）
