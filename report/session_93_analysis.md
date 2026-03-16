# Session 93 - DataPage功能完善分析

## 任务目标

完善 DataPage 功能，实现游戏数据管理页面，包含：
1. AppPage 数据管理按钮连接到 DataPage
2. DataPage 使用 BrowserHeader + TabFrame 结构
3. TabFrame 侧边栏展示所有已记录游戏
4. 每个游戏的子面板为 TopTabFrame（顶部选项卡）
5. 三个子面板：存档（ProImage 三列）、相册（ProImage 三列）、金手指（DetailCell 列表）

## 输入输出

**输入：**
- gamedataManager 中存储的游戏路径数据
- NameMappingManager 中的游戏显示名称映射
- 文件系统中的存档缩略图（saves/游戏名/）
- 文件系统中的相册截图（albums/游戏名/）
- 金手指文件（.cht 格式）

**输出：**
- 可视化的游戏数据管理界面
- 支持浏览、删除存档/相册图片
- 支持修改金手指代码

## 实现方案

### 1. ConfigManager 扩展
新增 `GetAllKeys()` 方法，允许遍历所有配置键，用于找出所有 `.gamepath` 条目。

### 2. TopTabFrame 组件
自定义顶部选项卡框架：
- 顶部水平按钮行（brls::Button）
- 内容区域（按选中的 tab 切换）
- `addTab(label, creator)` 接口与 TabFrame 保持一致

### 3. DataPage 结构
```
DataPage (BBox, COLUMN)
├── BrowserHeader（标题：游戏数据）
└── brls::TabFrame
    ├── 游戏1 → TopTabFrame
    │   ├── 存档 → ScrollingFrame + 三列 ProImage
    │   ├── 相册 → ScrollingFrame + 三列 ProImage
    │   └── 金手指 → ScrollingFrame + DetailCell 列表
    ├── 游戏2 → ...
    └── ...（无游戏时显示提示）
```

### 4. 存档/相册子面板
- 使用 `collectImages()` 收集指定目录下所有图片文件
- 三列布局：每 3 张图片一行
- 异步加载：`ProImage::setImageFromFileAsync()`
- A 键：打开 ImageView 全屏查看
- X 键：弹出确认对话框，删除后隐藏对应 UI 元素
- 存档删除时同时删除对应状态文件（去掉图片扩展名）

### 5. 金手指子面板
- 本地解析 `.cht` 文件（RetroArch 格式）
- 每条金手指显示为 DetailCell（名称 + 代码）
- A 键：调用系统 IME 输入新代码，保存并更新 UI

## 修改的文件

| 文件 | 修改内容 |
|------|----------|
| `include/Utils/ConfigManager.hpp` | 新增 `GetAllKeys()` 声明 |
| `src/Utils/ConfigManager.cpp` | 实现 `GetAllKeys()` |
| `include/UI/Pages/DataPage.hpp` | 新建，声明 DataPage、TopTabFrame |
| `src/UI/Pages/DataPage.cpp` | 新建，完整实现 |
| `include/UI/Pages/AppPage.hpp` | 新增 `onOpenDataPage` 回调 |
| `src/UI/Pages/AppPage.cpp` | 连接"数据管理"按钮 |
| `include/UI/StartPageView.hpp` | 新增 `openDataPage()` 声明 |
| `src/UI/StartPageView.cpp` | include DataPage.hpp，实现 `openDataPage()`，绑定回调 |
| `resources/i18n/zh-Hans/beiklive.json` | 新增 datapage 中文字符串 |
| `resources/i18n/en-US/beiklive.json` | 新增 datapage 英文字符串 |

## 潜在挑战与解决方案

1. **遍历 gamedataManager 键**：ConfigManager 不暴露迭代接口 → 新增 `GetAllKeys()`
2. **顶部选项卡**：borealis 原生 TabFrame 只支持左侧栏 → 自定义 TopTabFrame
3. **删除后 UI 更新**：需要捕获视图指针 → lambda 捕获 `img` 指针，删除后调用 `setVisibility(GONE)`
4. **存档文件关联**：存档缩略图为 `.ss0.png` 等 → 删除时去掉扩展名得到状态文件路径
5. **金手指文件不存在**：显示"无金手指"提示

## 安全性说明

无安全漏洞引入。文件删除操作均有用户确认对话框保护。
