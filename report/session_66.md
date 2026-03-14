# Session 66 – 将自定义面板改为 SelectorCell（Dropdown）

## 任务目标

1. 把 `m_appSettingsPanel`（`AppGameSettingsPanel`）从自定义侧边栏面板改成使用 `brls::Dropdown` 来选择功能。
2. `FileListPage::openSidebar` 也改成使用 `brls::Dropdown` 来选择功能，移除旧的 `FileSettingsPanel` 自定义面板委托机制。

---

## 需求分析

### 背景

原来的实现中，有两个自定义的绝对定位覆盖面板（`AppGameSettingsPanel` 和 `FileSettingsPanel`）用于展示操作菜单。这两个面板都通过手动构建带按钮的 `Box` 实现，逻辑复杂、维护成本高。

`brls::Dropdown`（即 `SelectorCell` 内部所用的组件）可以直接呈现一组选项供用户选择，接口简洁，与 borealis UI 风格一致。

### 完整需求整理

1. **`m_appSettingsPanel` 替换**：
   - 删除 `AppGameSettingsPanel` 类（`.hpp` 和 `.cpp` 中的声明与实现）。
   - 删除 `StartPageView::m_appSettingsPanel` 成员变量。
   - 在 `m_appPage->onGameOptions` 回调中，直接使用 `brls::Dropdown` 弹出选项菜单，包含：设置映射名称、选择 Logo、从列表移除。

2. **`FileListPage::openSidebar` 替换**：
   - 删除 `FileSettingsPanel` 类（`.hpp` 和 `.cpp` 中的声明与实现）。
   - 删除 `FileListPage::onOpenSettings` 回调成员（不再需要外部委托）。
   - `openSidebar` 始终直接使用 `brls::Dropdown` 弹出选项菜单，包含：重命名（Switch 平台）、设置映射名称（Switch 平台）、选择 Logo（游戏文件）、剪切、粘贴（有剪贴板时）、删除、新建文件夹。
   - 补充了之前仅在 `FileSettingsPanel` 中存在而在 Dropdown fallback 中缺失的选项（`select_logo`、`new_folder`）。

3. **`StartPageView::openFileListPage` 简化**：
   - 移除 `FileSettingsPanel` 的创建和 `onOpenSettings` 的绑定代码。
   - 容器结构更简洁。

---

## 变更文件清单

| 文件 | 变更内容 |
|------|---------|
| `include/UI/StartPageView.hpp` | 删除 `FileSettingsPanel`、`AppGameSettingsPanel` 类声明；删除 `m_appSettingsPanel` 成员 |
| `src/UI/StartPageView.cpp` | 删除两个面板类的全部实现；`createAppPage()` 中用 `brls::Dropdown` 替换 `AppGameSettingsPanel`；`openFileListPage()` 中移除 `FileSettingsPanel` 相关代码 |
| `include/UI/Pages/FileListPage.hpp` | 删除 `onOpenSettings` 成员 |
| `src/UI/Pages/FileListPage.cpp` | 重写 `openSidebar`：移除 `onOpenSettings` 委托路径，始终使用 `brls::Dropdown`；补充 `select_logo`、`new_folder` 选项 |

---

## 功能对比

### AppGameSettingsPanel → Dropdown（游戏卡片 X 键）

| 选项 | 原面板 | 新 Dropdown |
|------|--------|------------|
| 设置映射名称 | ✅ | ✅ |
| 选择 Logo | ✅ | ✅ |
| 从列表移除 | ✅ | ✅ |

### FileSettingsPanel → openSidebar Dropdown（文件列表 X 键）

| 选项 | 原面板 | 新 Dropdown |
|------|--------|------------|
| 重命名（Switch） | ✅ | ✅ |
| 设置映射名称（Switch） | ✅ | ✅ |
| 选择 Logo（游戏文件） | ✅ | ✅（新增） |
| 剪切 | ✅ | ✅ |
| 粘贴 | ✅ | ✅ |
| 删除 | ✅ | ✅ |
| 新建文件夹 | ✅ | ✅（新增） |

---

## 编译验证

在 Linux 桌面平台编译通过（仅有已有 warning，无新增 error）。

```
[100%] Linking CXX executable BKStation
[100%] Built target BKStation
```

---

## 结论

- 用 `brls::Dropdown` 统一替换了两处自定义侧边栏面板，代码更简洁。
- 删除了约 350 行面板类代码，消除了绝对定位覆盖层的复杂性。
- 功能完整保留，并补充了之前缺失的 `select_logo` 和 `new_folder` 选项至文件列表操作 Dropdown。
