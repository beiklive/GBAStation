# FileListPage 功能实现报告

## 工作概述

本次工作基于需求，在 `FileListPage` 中使用纯代码（无 XML）实现了完整的文件列表界面，并更新了 `StartPageView` 以支持根据 `start_page_index` 切换 APP 界面和文件列表界面。

---

## 一、新增 / 修改的文件

| 文件 | 说明 |
|------|------|
| `include/XMLUI/Pages/FileListPage.hpp` | 全新实现：FileListItem 结构体、FileListCell、FileListDataSource、FileListPage 类定义 |
| `src/XMLUI/Pages/FileListPage.cpp` | FileListPage 完整实现 |
| `include/XMLUI/StartPageView.hpp` | 添加 AppPage/FileListPage 成员及切换相关方法声明 |
| `src/XMLUI/StartPageView.cpp` | 根据 start_page_index 初始化界面，实现 RT/LT 切换逻辑 |
| `resources/i18n/zh-Hans/beiklive.json` | 新增 hints/confirm、hints/operate、file/file_select、sidebar/* 等 i18n 键 |
| `resources/i18n/en-US/beiklive.json` | 同上（英文版） |

---

## 二、功能实现详情

### 1. FileListItem 结构体

```cpp
struct FileListItem {
    std::string    fileName;    // 实际文件/目录名（不含路径）
    std::string    mappedName;  // 来自 NameMappingManager 的映射名（空=未映射）
    bool           isDir;       // 是否为目录
    std::string    fullPath;    // 绝对路径
    std::uintmax_t fileSize;    // 文件大小（字节，仅文件）
    int            childCount;  // 子项数量（仅目录）

    const std::string& displayName() const; // mappedName 优先，否则 fileName
};
```

### 2. 文件过滤器（FilterMode）

- 支持 **Whitelist（白名单）** 和 **Blacklist（黑名单）** 两种模式
- 后缀比较忽略大小写（使用 `beiklive::string::iequals`）
- 通过 `setFilter(suffixes, mode)` 和 `setFilterEnabled(bool)` 控制
- StartPageView 默认设置白名单：`gba, gb, gbc, zip`

### 3. 名称映射（NameMappingManager）

- 文件：使用「文件名（不含后缀）」作为 key 查询 `NameMappingManager`
- 目录：使用「目录名」作为 key 查询
- 映射名保存到 `FileListItem::mappedName`；无映射则保留原名

### 4. 列表元素设计（FileListCell）

```
[accent rect] [name label (grow=1)] [info label (gray, right)]
```

- 左侧：显示名称（`displayName()`，含映射）
- 右侧灰色：
  - 目录 → `N items`（直接子项数量）
  - 文件 → 文件大小（B / KB / MB / GB）
- 焦点时左侧 accent 矩形可见，突出当前选中行

### 5. 按键绑定

| 按键 | 操作 |
|------|------|
| **A** | 打开目录 / 调用文件回调 |
| **B** | 返回上级目录 |
| **X** | 打开当前聚焦元素的侧边栏 |
| **+（START）** | 由外部（非 StartPageView 场景）绑定关闭 |
| **RT** | StartPageView 内：FileListPage → AppPage |
| **LT** | StartPageView 内：AppPage → FileListPage |

### 6. 侧边栏（Dropdown）

使用 `brls::Dropdown` 实现操作菜单，包含：

- **重命名**：调用 `ImeManager::openForText`，完成后执行 `fs::rename`
- **设置映射名称**：调用 `ImeManager::openForText`，保存到 `NameMappingManager`
- **剪切**：将当前项保存到 `FileListPage` 内部剪切板
- **粘贴**：仅在剪切板有数据时显示；执行 `fs::rename` 到当前目录
- **删除**：二次确认 Dropdown，确认后执行 `fs::remove_all`

### 7. Header / BottomBar

- `brls::Header`：title = `"文件选择"`（_i18n），subtitle = 当前路径（路径变化时更新）
- `brls::BottomBar`：显示平台标准提示信息

### 8. 两种布局模式

| 模式 | 说明 |
|------|------|
| `LayoutMode::ListOnly` | 列表占全宽，详情面板隐藏（默认） |
| `LayoutMode::ListAndDetail` | 左 2/3 列表 + 右 1/3 详情面板（缩略图、名称、映射名、大小/子项数） |

通过 `setLayoutMode(mode)` 或 `SettingManager` 控制。

### 9. StartPageView 切换逻辑

```
start_page_index == 0  →  初始显示 AppPage   (LT 切换到 FileListPage)
start_page_index == 1  →  初始显示 FileListPage (RT 切换到 AppPage)
```

- 两个页面均为延迟创建（第一次需要时才 `new`）
- 切换时通过 `setVisibility(GONE/VISIBLE)` 隐藏/显示，避免重复创建
- 切换后重新注册对应按键绑定

---

## 三、内存安全措施

- `FileListDataSource` 通过 `setDataSource(ds, /*deleteDataSource=*/false)` 注册，
  由 `FileListPage` 析构时手动 `delete`，避免 RecyclerFrame 已析构后再被删除
- 所有 View 子对象通过 `addView()` 托管给 borealis，框架负责生命周期
- `m_appPage` / `m_fileListPage` 在 StartPageView 中通过 `addView()` 托管，无需手动 `delete`
- Lambda 捕获避免悬空引用（使用 `[this]` 或值捕获）

---

## 四、已知局限与后续优化建议

1. **缩略图**：当前只查找与文件同名的 `.png` 文件。可扩展为查找专用 thumb 目录。
2. **列表宽度自定义**：`setLayoutMode` 暂使用固定百分比（66%/33%），后续可通过参数控制。
3. **重命名/删除反馈**：操作失败时仅打印 log，可添加 Toast/Dialog 提示用户。
4. **剪切板跨目录支持**：目前使用 `fs::rename`，跨分区时会失败；可改为先 copy 再 delete。
5. **分页/大目录**：目录子项过多时 RecyclerFrame 会处理虚拟滚动，无需额外优化。
