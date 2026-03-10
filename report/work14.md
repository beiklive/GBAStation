# Work14 任务汇报

## 任务概述

本次工作（work14）按照需求完成了以下功能开发与问题修复。

---

## 已完成任务

### 1. 制作 ImageView（图片查看器）

**文件：**
- `include/XMLUI/Pages/ImageView.hpp`（新建）
- `src/XMLUI/Pages/ImageView.cpp`（新建）

**功能描述：**
- 通过构造函数传入图片文件路径打开并显示图片
- 背景全黑（NVG 绘制纯黑矩形）
- 支持缩放：`BUTTON_A` 放大，`BUTTON_Y` 缩小，`BUTTON_X` 重置视图
- 支持方向键平移图片（按比例偏移）
- `BUTTON_B` 关闭该 View（调用 `brls::Application::popActivity()`）
- 其他无关按键（LT/RT/LB/RB/START/BACK）均被 swallow 禁用
- 图片懒加载：首次 `draw()` 时通过 NVG 加载图片，失败时显示文件路径提示
- 在 `StartPageView` 中的图片文件回调已启用，点击图片文件将打开 `ImageView`

---

### 2. 新增 logoManager 配置管理器

**文件：**
- `include/common.hpp`（修改）
- `main.cpp`（修改）

**功能描述：**
- 新增全局变量 `extern beiklive::ConfigManager* logoManager;`
- 在 `ConfigManagerInit()` 中初始化，保存路径为 `BK_APP_CONFIG_DIR + "logo_mapping.cfg"`
- 保存规则：key = 文件名（不含后缀），value = logo 图片路径
- 启动时尝试加载，失败时记录警告日志

---

### 3. FileListItem 增加 logo 路径字段

**文件：**
- `include/XMLUI/Pages/FileListPage.hpp`（修改）
- `src/XMLUI/Pages/FileListPage.cpp`（修改）

**功能描述：**
- `FileListItem` 结构体新增 `std::string logoPath;` 字段
- 在 `refreshList()` 中通过 `lookupLogoPath()` 查询 `logoManager` 获取 logo 路径
- 新增 `SettingManager` 配置项 `UI.logoLoadMode`：
  - `0`：不显示 logo 图片
  - `1`（默认）：子元素聚焦后读取（on-focus）
  - `2`：提前预取（prefetch，refreshList 时一并查询）
- `updateDetailPanel()` 显示优先级：
  1. logoManager 中的 logo 路径（若存在且文件有效）
  2. 文件同名目录下的 `.png` 缩略图
  3. 默认应用图标（BK_APP_DEFAULT_LOGO）

---

### 4. 设置弹窗改为 StartPageView 中创建

**文件：**
- `include/XMLUI/StartPageView.hpp`（修改）
- `src/XMLUI/StartPageView.cpp`（修改）
- `include/XMLUI/Pages/FileListPage.hpp`（修改）
- `src/XMLUI/Pages/FileListPage.cpp`（修改）

**功能描述：**
- 新建 `FileSettingsPanel` 类，继承自 `brls::Box`
  - 使用 `setPositionType(brls::PositionType::ABSOLUTE)` 绝对定位
  - 在 `StartPageView::onLayout()` 中动态设置：宽高各为屏幕的 70%，居中显示
  - 初始 Visibility 为 GONE，需要时显示
- `FileListPage` 新增公开回调 `onOpenSettings`，按 `BUTTON_X` 时触发
- `StartPageView` 注册该回调，调用 `FileSettingsPanel::showForItem()`
- 弹窗功能键包括：
  - **重命名**（仅 Switch 平台）
  - **设置映射名称**
  - **剪切**
  - **粘贴**（有剪贴板内容时显示）
  - **删除**
  - **新建文件夹**（IME 输入文件夹名，调用 `fs::create_directory`）
  - 弹窗内 `BUTTON_B` 关闭面板
- `FileListPage` 新增公开方法 `doRenamePublic`、`doSetMappingPublic`、`doCutPublic`、`doPastePublic`、`doDeletePublic`、`doNewFolder()` 供面板调用

---

### 5. Windows 端隐藏重命名功能

**文件：**
- `src/XMLUI/Pages/FileListPage.cpp`（修改）
- `src/XMLUI/StartPageView.cpp`（修改）

**功能描述：**
- 在 `openSidebar()` 的 Fallback Dropdown 以及 `FileSettingsPanel::showForItem()` 中：
  - 重命名选项均被 `#ifdef __SWITCH__` / `#endif` 包裹
  - 在 Windows/Linux/macOS 端不显示重命名选项

---

### 6. Windows 端目录返回支持盘符列表

**文件：**
- `src/XMLUI/Pages/FileListPage.cpp`（修改）
- `include/XMLUI/Pages/FileListPage.hpp`（修改）

**功能描述：**
- 新增静态函数 `getWindowsDrives()`（仅 `_WIN32` 编译），通过 `GetLogicalDrives()` 获取所有可用盘符（如 `C:\`, `D:\`）
- `FileListPage` 新增 `bool m_inDriveListMode` 成员变量
- `navigateUp()` 逻辑（`_WIN32`）：
  - 当 `m_inDriveListMode == true` 时，直接 return，不再向上导航
  - 当位于磁盘根路径（如 `C:\`）时，调用 `showDriveList()` 切换到盘符列表
  - 其余情况正常向上导航
- `showDriveList()` 将 `m_inDriveListMode` 置为 true，将所有盘符作为 `FileListItem`（isDir=true）填入列表
- 进入盘符（点击 A）后调用 `refreshList(item.fullPath)` 正常导航，同时 `m_inDriveListMode` 被重置为 false
- 处于盘符列表时，`BUTTON_B` 虽仍可按下，但 `navigateUp()` 会立即返回（无异常）

---

### 7. 过滤后空目录显示 `..` 子元素

**文件：**
- `src/XMLUI/Pages/FileListPage.cpp`（修改）

**功能描述：**
- 在 `refreshList()` 过滤完成后，若列表为空且当前路径非根目录：
  - 自动插入一个 `fileName = ".."`, `fullPath = getParentPath(m_currentPath)`, `isDir = true` 的 `FileListItem`
  - 激活该条目将调用 `refreshList(parent_path)` 导航到上级目录，实现焦点获取和返回功能

---

### 8. I18n 国际化字符串更新

**文件：**
- `resources/i18n/en-US/beiklive.json`（修改）
- `resources/i18n/zh-Hans/beiklive.json`（修改）

**新增字符串：**
- `hints/close`：Close / 关闭
- `hints/zoom_in`：Zoom In / 放大
- `hints/zoom_out`：Zoom Out / 缩小
- `hints/reset`：Reset View / 重置视图
- `file/drives`：Drive List / 磁盘列表
- `sidebar/new_folder`：New Folder / 新建文件夹
- `sidebar/close_panel`：Close / 关闭

---

## 未完成任务

无。所有需求均已完成实现。

---

## 技术说明

### FileSettingsPanel 架构说明
面板使用绝对定位，在 `StartPageView::onLayout()` 中根据父容器尺寸动态设置为屏幕的 70%（宽高），并居中（左/上各偏移 15%）。面板位于 StartPageView 的视图树中，渲染层级高于子页面，实现视觉上的遮盖效果。

### Windows 盘符模式说明
`BUTTON_B` 在盘符模式下不会导致崩溃，因为 `navigateUp()` 会在 `m_inDriveListMode == true` 时立即返回。按键提示仍显示"上级目录"，但按下无效。如需完全隐藏该提示，可通过 `unregisterAction` + 重新注册实现（暂未实现，功能安全）。

### ImageView 图片加载
使用 NVG `nvgCreateImage()` 懒加载，首次 `draw()` 时加载。图片加载后通过 `nvgImagePattern()` + `nvgFillPaint()` 渲染，支持任意缩放和平移偏移。图片卸载由 NVGcontext 生命周期管理。
