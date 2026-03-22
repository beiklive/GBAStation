# 工作汇报：修复金手指文件选择崩溃与移除无效操作

## 任务目标

1. 修复 #138 的金手指功能在选择金手指文件后和不选择直接关闭选择界面时程序会崩溃的问题
2. 移除 X 菜单中在 Switch 平台无效的删除/剪切/粘贴功能

---

## 任务分析

### 问题一：金手指文件选择崩溃

**根因分析：**

`FileListPage.cpp`、`GameLibraryPage.cpp`、`StartPageView.cpp` 三处金手指文件选择回调使用了
`brls::sync([]() { brls::Application::popActivity(); })` 来延迟关闭文件选择界面。

虽然注释中说明这是为了"避免在 handleAction 回调链内部直接调用 popActivity 导致焦点恢复时崩溃"，
但实际测试发现该延迟方案仍然导致崩溃。

对比同类已工作的代码（GameMenu.cpp 中的遮罩路径/着色器路径选择），这些回调均直接调用
`brls::Application::popActivity()` 而非使用 `brls::sync`，且正常工作。

**修复方案：**

将三处金手指文件选择回调中的 `brls::sync(popActivity)` 替换为直接调用
`brls::Application::popActivity()`，与遮罩/着色器路径选择的成熟模式保持一致。

### 问题二：X 菜单删除/剪切/粘贴在 Switch 无效

**根因分析：**

`FileListPage::openSidebar()` 中注册了剪切（cut）、粘贴（paste）、删除（delete）三个操作，
底层使用 `std::filesystem::rename` 和 `fs::remove_all`，这些 API 在 Switch 平台
（NX 文件系统）可能无法正常工作。

**修复方案（按问题陈述建议）：**

直接移除以下三项功能：
- 剪切（doCut）
- 粘贴（doPaste）
- 删除（doDelete）

同时清理相关的头文件声明和私有成员变量。

---

## 修改内容

### 修改文件列表

| 文件 | 修改内容 |
|------|---------|
| `src/UI/Pages/FileListPage.cpp` | 1. 金手指回调改为直接 `popActivity()`；2. 移除 openSidebar() 中的 cut/paste/delete 选项；3. 删除 doCut/doPaste/doDelete 实现 |
| `src/UI/Pages/GameLibraryPage.cpp` | 金手指回调改为直接 `popActivity()` |
| `src/UI/StartPageView.cpp` | 金手指回调改为直接 `popActivity()` |
| `include/UI/Pages/FileListPage.hpp` | 移除剪贴板相关成员变量、公有/私有方法声明 |

### 关键变更

**修复崩溃（三处均相同）：**
```cpp
// 修改前
setGameDataStr(captureFileName, GAMEDATA_FIELD_CHEATPATH, chtItem.fullPath);
brls::sync([]() { brls::Application::popActivity(); });  // 仍会崩溃

// 修改后
setGameDataStr(captureFileName, GAMEDATA_FIELD_CHEATPATH, chtItem.fullPath);
brls::Application::popActivity();  // 与遮罩/着色器选择保持一致
```

**移除 X 菜单无效操作：**
- 从 `openSidebar()` 删除 cut/paste/delete 的 opts.push_back 调用
- 从 `FileListPage.cpp` 删除 `doCut`、`doPaste`、`doDelete` 函数实现
- 从 `FileListPage.hpp` 删除 `m_clipboardItem`、`m_hasClipboard` 成员变量
- 从 `FileListPage.hpp` 删除 `hasClipboardItem()`、`getClipboardItem()`、
  `doCutPublic()`、`doPastePublic()`、`doDeletePublic()`、
  `doCut()`、`doPaste()`、`doDelete()` 方法声明

---

## 验证

- Linux 平台编译成功（无编译错误，仅有两处预存警告：
  - `main.cpp:14`：`'sceLibcHeapSize' initialized and declared 'extern'`（PS4 平台遗留）
  - `src/common.cpp:55`：`'k_xmbColorNames' defined but not used`（未使用变量，与本次修改无关））
- 代码结构与同类已工作的功能（遮罩/着色器路径选择）保持一致
