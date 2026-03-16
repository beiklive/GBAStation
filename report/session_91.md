# Session 91 工作汇报

## 任务完成情况

**任务**：修改 `m_loadStateScrollFrame` 和 `m_saveStateScrollFrame`，改成左边ScrollFrame，右边box内嵌Image，ScrollFrame子元素获得焦点后更新Image，如果状态不存在就显示NoData

**状态**：✅ 已完成

## 变更文件

| 文件 | 变更说明 |
|------|----------|
| `include/UI/Utils/GameMenu.hpp` | 重构成员变量：新增面板容器、预览图、NoData标签；移除每行缩略图数组；更新方法签名 |
| `src/UI/Utils/GameMenu.cpp` | 重构状态面板布局（左ScrollFrame+右Image Box）；更新 buildStatePanel 和 refreshStatePanels |
| `resources/i18n/zh-Hans/beiklive.json` | 添加 `gamemenu.no_state_data` 键值 |
| `resources/i18n/en-US/beiklive.json` | 添加 `gamemenu.no_state_data` 键值 |

## 新布局结构

```
rightBox (70% 宽)
├── m_saveStatePanel (ROW, GONE/VISIBLE)
│   ├── m_saveStateScrollFrame (ScrollFrame, grow=1)  ← 左侧槽位按钮列表
│   │   └── m_saveStateItemBox
│   │       ├── slot0 按钮 → 焦点时更新 m_savePreviewImage
│   │       └── ...
│   └── savePreviewBox (Box, 50% 宽)                  ← 右侧预览区
│       ├── m_savePreviewImage (Image, 存档存在时显示)
│       └── m_saveNoDataLabel (Label, 无存档时显示)
└── m_loadStatePanel (同上结构)
```

## 核心行为

1. 用户导航至"保存状态"/"读取状态"按钮 → 显示对应面板并重置预览为 NoData
2. 用户按 A 键 → 焦点进入左侧 ScrollFrame
3. 焦点移到某槽位按钮 → 调用 `m_stateInfoCallback(slot)`：
   - 存档存在且有缩略图 → 右侧显示预览图
   - 否则 → 右侧显示"无存档数据"标签
