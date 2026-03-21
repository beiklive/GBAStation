# Session 58 工作报告 — 遮罩（Overlay）功能实现

## 需求摘要

根据 Issue 要求，为 BeikLiveStation 添加遮罩（Overlay）功能：

1. **画面设置**增加遮罩配置区块（遮罩总开关、全局 GBA 遮罩路径、全局 GBC 遮罩路径）。
2. **gamedataManager** 为每款游戏添加 `overlay` 字段，初始值自动从全局遮罩配置中读取。
3. **GameView** 在绘制游戏帧后，再全屏绘制遮罩 PNG 图像。

---

## 实现文件清单

| 文件 | 变更说明 |
|------|----------|
| `include/common.hpp` | 新增配置 Key 宏和 `GAMEDATA_FIELD_OVERLAY`；`initGameData()` 新增 overlay 字段初始化逻辑 |
| `include/Game/game_view.hpp` | 新增 overlay 成员变量和 `loadOverlayImage()` 方法声明 |
| `src/Game/game_view.cpp` | 在 `initialize()` 中加载遮罩配置；`cleanup()` 中重置 handle；新增 `loadOverlayImage()`；`draw()` 中全屏绘制遮罩 |
| `src/UI/Pages/SettingPage.cpp` | `buildDisplayTab()` 中新增"遮罩设置"区块 |
| `resources/i18n/zh-Hans/beiklive.json` | 新增遮罩相关中文字符串 |
| `resources/i18n/en-US/beiklive.json` | 新增遮罩相关英文字符串 |

---

## 核心设计决策

### 1. 配置 Key

```
display.overlay.enabled   — 遮罩总开关（bool，默认 false）
display.overlay.gbaPath   — 全局 GBA 遮罩 PNG 路径（string，默认空）
display.overlay.gbcPath   — 全局 GBC 遮罩 PNG 路径（string，默认空）
```

### 2. gamedataManager 字段

新增字段 `overlay`（常量 `GAMEDATA_FIELD_OVERLAY = "overlay"`）。

`initGameData()` 初始化时根据平台（GBA / GB）自动从 `SettingManager` 读取对应全局路径作为默认值，已存在的记录不覆盖。

### 3. 画面设置 UI

在"画面设置"Tab 的"状态显示"区块下方新增"遮罩设置"区块，包含：
- **启用遮罩** — BooleanCell
- **全局 GBA 遮罩路径 (PNG)** — DetailCell，点击弹出文件浏览器（PNG 白名单过滤）
- **全局 GBC 遮罩路径 (PNG)** — DetailCell，同上

### 4. 游戏内遮罩绘制

- `initialize()` 读取 `display.overlay.enabled` 及遮罩路径（优先 per-game overlay，其次全局路径）。
- `draw()` 在游戏帧绘制完成后，若 overlay 已启用且路径非空，则懒加载（`loadOverlayImage()`）NVG 图像，以全屏 NVG 图案填充绘制遮罩。
- `cleanup()` 重置 overlay NVG image handle。

---

## 编译验证

Linux Release 构建成功，无新增编译错误（已有第三方库警告与本次变更无关）。

```
Build successful: build_linux/GBAStation (4193720 bytes)
```
