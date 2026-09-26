# GBAStation UI 梳理：游戏库 / 设置 / 关于 / 数据管理 / SwitchLayout

> 口径：只读分析 5 个界面（全文阅读源码 + grep 交叉验证），所有结论带 `file:line`。
> 证据不足处标「未确认」，不做推测。行号以当前工作区源码为准。

---

## 0. 一句话结论

**这 5 个界面里，只有 1 个是普通 borealis 控件树，其余 4 个的"真实渲染路径"都是「单块全自绘 nanovg Canvas」**：

| 界面 | 实际生效的渲染方式 | 证据 |
|---|---|---|
| 游戏库 `GameLibraryPage` | `getContentBox()` 里只挂 2 个自绘 View：`GameGridView` + `NanoSearchOverlay` | GameLibraryPage.cpp:819/832、833/842 |
| 设置 `SettingPage` | `init()` 里只挂 1 个 `NanoSettingsCanvas`（纯自绘） | SettingPage.cpp:6374-6389（`new NanoSettingsCanvas` 6386） |
| 关于 `AboutPage` | 构造函数里只挂 1 个 `AboutMainCanvas`（纯自绘） | AboutPage.cpp:3424-3446（`new AboutMainCanvas` 3431） |
| 数据管理 `DataManagementPage` | `init()` 里只挂 1 个 `DataManagementCanvas`（纯自绘） | DataManagementPage.cpp:1854-1979（`new DataManagementCanvas` 1973） |
| SwitchLayout（主界面） | 纯自绘：整个视图只有 1 个 `brls::Box` 节点，卡片/按钮/文字全用 nanovg 画 | SwitchLayout.hpp:21、hpp:54-63、cpp:158-168、cpp:913-928 |

由此有两个必须分开看的"控件层"：

1. **外壳与弹窗层**：`beiklive::Box`（页面壳）、`brls::AppletFrame` / `brls::Activity`（子页面）、
   `brls::Dialog` / `brls::Dropdown`（确认框、下拉）、`brls::Label` / `brls::Rectangle`（进度弹窗内）——**这些是真的 borealis 控件**。
2. **内容层**：列表条目、开关、选择器、卡片格子、徽章、滚动条、提示条、进度条——**全是 nanovg 自绘，没有对应的控件类**。

**另一个关键发现：4 个页面里都留着大量「已无调用点」的 borealis 控件树代码**（旧版 TabFrame/List 实现）。
`brls::SelectorCell` / `brls::BooleanCell` / `brls::DetailCell` / `brls::Header` / `brls::ScrollingFrame` /
`beiklive::DetailCell` / `beiklive::GridBox` **几乎只活在这些死函数里**（详见 §4）。

---

## 1. 导航关系（谁调起谁）

主界面 `SwitchLayout` 的"功能按钮行"是唯一入口（6 个功能项，SwitchLayout.cpp:146-153），
点击后由 `StartPage` 接管跳转：

| 功能项（原文） | 回调 | 跳转目标 | file:line |
|---|---|---|---|
| `L("游戏库")` | `onGameLibraryOpened` | `beiklive::GameLibraryPage` | SwitchLayout.cpp:147/886 → StartPage.cpp:1818-1822/2041 |
| `L("文件列表")` | `onFileBrowserOpened` | `beiklive::FileListPage` | SwitchLayout.cpp:148/887 → StartPage.cpp:1824-1828/2063 |
| `L("数据管理")` | `onDataManagementOpened` | `beiklive::DataManagementPage` | SwitchLayout.cpp:149/888 → StartPage.cpp:1829-1833/2174 |
| `L("设置")` | `onSettingsOpened` | `beiklive::SettingPage` | SwitchLayout.cpp:150/889 → StartPage.cpp:1834-1838/2140 |
| `L("关于")` | `onAboutOpened` | `beiklive::AboutPage` | SwitchLayout.cpp:151/890 → StartPage.cpp:1839-1843/2157 |
| `L("退出")` | `onExitRequested` | `brls::Application::quit()` | SwitchLayout.cpp:152/891 → StartPage.cpp:1849-1861 |

四个页面统一用 `new beiklive::XxxPage()` → `new brls::AppletFrame(page)` → `HIDE_BRLS_BAR(frame)` →
`brls::Application::pushActivity(new brls::Activity(frame), TransitionAnimation::NONE)`
（StartPage.cpp:2138-2184）；进页前都会先播 `switchLayout->playExitAnimation(...)`。

游戏库内的二级入口：点卡片按 A → `onGameOptions` → `StartPage::_showGameOptionsPanel`
→ `beiklive::GameOptionsSidebar`（SwitchLayout.cpp:872-873、StartPage.cpp:2188-2199）。

---

## 2. 游戏库：GameLibraryPage

- 类/继承：`beiklive::GameLibraryPage : public beiklive::Box`（GameLibraryPage.hpp:16）
- 入口：`GameLibraryPage(PreparedData)` **GameLibraryPage.cpp:811-970**（唯一搭 UI 的地方）；构造 806-811；
  `willAppear()` 992-1002；`prepareInitialData()` 972-982；数据源 `GameLibraryDS` 1004-1046
- 无 `onLayout` / `frame` 覆写；页头页脚关闭后仍写值：`showHeader(false)` 813、`showFooter(false)` 814、
  `getHeader()->setTitle(L("游戏库"))` 815

### 2.1 面板 / 区块

| 面板 | 实现方式 | 关键内容 | file:line |
|---|---|---|---|
| 顶部工具栏 | **自绘** `GameGridView::_drawToolbar` | 左：`L("游戏库")`+分类详情；中：平台轮播（LB/RB）；右：Y 键图标+`L("切换为列表")`/`L("切换为网格")` | RecyclingGrid.cpp:2236-2346（2242/2248/2312-2324） |
| 游戏库主体 | **自绘** `GameGridView`（`brls::View` 子类，全局命名空间） | GRID 3 列 / LIST 左列表+右详情；空格文案 `"空"`；空态 `L("当前分类暂无游戏")` | RecyclingGrid.hpp:18/87、RecyclingGrid.cpp:1595-1732（空态 1630） |
| 列表模式右侧详情面板 | **自绘** `_drawDetailsPanel` | 标题、平台徽章、游玩时长、`L("封面预览")`+封面 | RecyclingGrid.cpp:2438-2512（`L("封面预览")` 2508） |
| 底部提示条 | **自绘** `_drawFooter`/`_drawHint` | 见 §2.3 | RecyclingGrid.cpp:2380-2436 |
| 搜索浮层 | `beiklive::NanoSearchOverlay`（整体自绘） | `L("搜索游戏库")`、`L("支持游戏标题、ROM 文件名与拼音模糊匹配")`、三行选项、底部 A/B 提示 | GameLibraryPage.cpp:833/842/925-937；NanoSearchOverlay.cpp:168-266 |
| 游戏操作侧边栏 | `beiklive::GameOptionsSidebar`（`setNanoVgMenu(true)` 走自绘菜单） | `L("启动游戏")`1541、`L("加入收藏")/L("取消收藏")`1560、`L("游戏操作")`1582、`L("修改映射名称")`1584、`L("修改封面")`1606、`L("从 SteamGridDB 获取")`1609、`L("从本地选择")`1632、`L("安装到 Switch 桌面")`1656、`L("数据管理")`1664、`L("核心切换")`1674、`L("删除游戏")`1709 | GameLibraryPage.cpp:1536-1740；GameOptionsSidebar.cpp:401-796 |
| 多选批量侧边栏 | 同上（复用） | `L("已选择 N 款游戏")`1898、`L("退出多选")`1901、`L("删除已选游戏")`1914、`L("收藏选中游戏")`1941、`L("取消收藏选中游戏")`1978 | GameLibraryPage.cpp:1891-2024 |
| 启动过场遮罩 | **自绘** `_drawLaunchOverlay` | 选中封面放大/淡出（无文字） | RecyclingGrid.cpp:2091-2221（调用 1727-1728） |
| 弹窗层 | `brls::Dialog` / `brls::Dropdown` + `brls::Activity` | 分类、排序、核心切换、各类确认框 | GameLibraryPage.cpp:1286/1375/1685（Dropdown）、13 处 Dialog（§2.2） |
| 页头（隐藏） | `beiklive::HeaderBar` | `L("游戏库")`、`L("共 N 款游戏")`、`L("分类: 平台")` | GameLibraryPage.cpp:815/1300-1327 |
| 页脚（隐藏） | `brls::BottomBar`（经 `beiklive::Box::getBottomBar()`） | 侧边栏显示时 `Visibility::GONE` | GameLibraryPage.cpp:1538/1733/1749/1770/1893/2018 |

### 2.2 控件清单

**borealis 原生（共 10 类）**

| 控件 | 用途/文字 | 位置（样例） |
|---|---|---|
| `brls::Dialog` | 13 处确认/选择框，如 `L("确认删除截图")`599、`L("确认将该截图设置为封面？")`615、`L("确认导出当前电池存档？")`637、`L("确认导入外部 .sav 并覆盖当前电池存档？")`655、`L("确认还原备份")`773、`L("请选择游戏的删除方式")`1716、`L("请选择这 N 款游戏的删除方式")`1924、`L("确认将选中的 N 款游戏添加到收藏？")`1947 | 526/599/615/637/655/681/773/793/1445/1715/1924/1947/1984 |
| `brls::Dropdown` | `L("游戏分类")`1286、`L("排序方式")`1375、`L("核心切换")`1685 | 同上 |
| `brls::Activity` / `brls::AppletFrame` | 承载 Dropdown / 图片预览页 / 数据管理页 | 1295/1384/1704、589/1030 |
| `brls::Box` / `brls::Label` / `brls::Image` | 旧数据页（死代码）里的截图卡片、备份列表 | 402/428/450/456/461/490/737、273/283/348/742、264-271 |
| `brls::Header` / `brls::ScrollingFrame` | 同上（死代码） | 486-488 / 732-735 |
| `brls::BottomBar` | 页脚（本页 GONE） | 1538 等 |

**本仓库自定义（共 11 类）**

| 控件 | 用途/文字 | 位置 |
|---|---|---|
| `beiklive::Box` | 页面基类 | GameLibraryPage.hpp:16 |
| `GameGridView`（全局命名空间） | 游戏库主体（自绘） | RecyclingGrid.hpp:18；实例 819/832 |
| `beiklive::HeaderBar` | 页头（已隐藏） | 815/1307/1327 |
| `beiklive::NanoSearchOverlay` | 搜索浮层 | 833/842 |
| `beiklive::GameOptionsSidebar` | 单卡侧边栏 + 多选侧边栏 | 1536/1891（挂载 1738/2023） |
| `beiklive::TabFrame` / `beiklive::GridBox` / `beiklive::GridItem` / `beiklive::ButtonBox` / `beiklive::ImageView` | **旧数据页死代码**：`L("即时存档管理")`380、`L("游戏图片管理")`385、`L("电池存档管理")`390、`L("导出存档")`465、`L("导入存档")`472、`L("存档备份")`479、`L("备份列表")`487 | 311/373、312/406/432、318/416、337-343、578 |
| `ScreenshotGridItem`（文件内匿名命名空间） | 截图卡片（`brls::Image`+2 个 Label） | 250-292（实例 557） |

**纯自绘（无控件类）**：卡面/封面/平台徽章/标题跑马灯/副文本/时长/空格/收藏图标/骨架屏/滚动条/工具栏/提示条/启动遮罩
（RecyclingGrid.cpp:1734-2221、2236-2531）。

### 2.3 交互与提示

按键注册全部挂在 `m_libraryView`（`GameGridView`）上：

| 键 | 提示原文 | file:line |
|---|---|---|
| B | `L("退出游戏库")` | 844-866 |
| Y | `L("切换视图")` | 870-880 |
| LB / RB | `L("上一平台")` / `L("下一平台")` | 882-886 / 888-892 |
| LSB | `L("分类")` | 894-898 |
| X | `L("多选")` | 900-912 |
| START | `L("全选")`（非多选态放行） | 914-923 |
| RT / LT | `L("搜索")` / `L("排序")` | 925-937 / 939-943 |

底部提示条（自绘，RecyclingGrid.cpp:2419-2434）：
常态 `L("选择")`A / `L("返回")`B / `L("多选")`X / `L("搜索")`RT / `L("排序")`LT；
多选态 `L("勾选/取消勾选")`A / `L("退出多选")`B / `L("批量操作")`X / `L("全选")`/`L("取消全选")`START；
LIST 模式追加 `L("上翻一页")`LEFT / `L("下翻一页")`RIGHT。
搜索浮层：`L("返回")`B / `L("选择")`A（NanoSearchOverlay.cpp:264-265），IME 标题 `L("搜索游戏")`、占位 `L("输入标题或文件名")`（159）。

Toast（`brls::Application::notify`）共 20+ 条，例如 `L("已删除存档")`532、`L("已设置为封面图片")`624、
`L("未找到电池存档")`634、`L("已导出存档")`648、`L("已创建备份")`694、`L("已还原存档")`783、
`L("已进入多选模式")`910、`L("请去设置-模拟器页面输入 SteamGridDB Api Key")`1616-1617、
`L("已切换核心：")`1696、`L("游戏文件删除失败，记录已保留")`1862、`L("已从游戏库移除 ")`1879-1880。

### 2.4 死代码

- `LegacyGameDataPage`（GameLibraryPage.cpp:294-803）：旧版数据管理页，含 `beiklive::TabFrame` + `GridBox` + `GridItem` + `ButtonBox` + `brls::ScrollingFrame` + `brls::Header`，全仓无实例化。
- `GameGridItem`（src/ui/widget/GameGridItem.hpp:9）：本页未使用。

---

## 3. 设置：SettingPage

- 类/继承：`class SettingPage : public beiklive::Box`（SettingPage.hpp:9）
- 入口：构造 6362-6368 → `init()` 6374-6389：`NanoSettingsHost` → `new NanoSettingsCanvas(host)` 6386 → `getContentBox()->addView(canvas)`
- 真实 frame/draw：`NanoSettingsCanvas::frame` 1497-1531、`draw` 1533-1557

### 3.1 外层结构（唯一生效路径）

```
SettingPage (beiklive::Box)                     6362
└─ getContentBox()
   └─ NanoSettingsCanvas (brls::View, 纯自绘)      6386 / 1489
      ├─ _drawHeader     6 个分类 Pill              4641-4698
      ├─ _drawContent    条目列表 + 自绘滚动条        4700-4852
      │   ├─ _drawSection / _drawItem              4921-4939 / 4941-5021
      │   └─ _drawToggle / _drawBinding            5023-5044 / 5078-5149
      ├─ _drawFooter     按键提示                   5151-5198
      ├─ _drawSelector   选择器覆盖层               5200-5265
      └─ _drawSteamDialog SteamGridDB 弹窗          5267-5399
```

4 个数据源模式（`_activeItems()` 4243-4250 决定）：分类 `m_categories[6]` / 核心浏览器 / 核心设置 / 按键映射。

### 3.2 面板 / 分组（6 个 Tab，SettingPage.cpp:1567-1570）

`模拟器` / `按键` / `游戏` / `显示` / `声音` / `调试`（带 Material 字形码位 0xE322/0xE30F/0xE338/0xE333/0xE050/0xE868）

| Tab | 分组（Section 原文） | 代表条目 |
|---|---|---|
| 模拟器 | `核心设置` / `存档与封面` / `界面` / `背景` / `界面`（第二次同名）/ `语言 / Language` / `SteamGridDB` | 核心管理；自动保存游戏状态、自动保存间隔、启动时自动加载、退出游戏时自动保存、使用存档截图作为封面；颜色主题、主页布局(重启后生效)、IISU 卡片行数；动态渐变背景、渐变主题、启用背景图片、背景图片路径、播放背景视频声音、背景视频音量、GIF播放速度；文件列表滚动动画、游戏库标题字号；语言 / Language；SteamGridDB API Key、清空 SteamGridDB 缓存（1777-1899） |
| 按键 | `游戏平台` | 13 个平台按键映射（GBA/GBC/GB/FC-NES/SFC/NDS/3DS/MD/Arcade/DC/PSP/Saturn/GC·Wii），未装外置核心会跳过（1904-1933） |
| 游戏 | `快进` / `倒带` | 启用快进、快进触发模式、快进倍率、快进时静音；启用倒带、倒带触发模式、倒带时静音、倒带步进、可视化倒带界面、状态保存间隔、最大倒带缓存、缩略图压缩策略（1935-1977） |
| 显示 | `画面` / `默认遮罩` / `默认着色器` | 画面模式、整数倍缩放、显示快进/倒带/静音/FPS 覆盖层；9 个平台遮罩、9 个平台着色器（2188-2239） |
| 声音 | `音频输出` | 主音量、按钮音效、按键音效音量、目标缓冲延迟、最大缓冲延迟、音画同步修正、切换淡入淡出（2242-2290） |
| 调试 | `日志` | 日志级别、输出日志到文件、调试信息覆盖层（2292-2327） |

核心设置页还按核心分了大量分组（mGBA / melonDS / DraStic / libretro 通用 / FBNeo / Flycast / PPSSPP /
DuckStation / YabaSanshiro / Dolphin / Genesis / Azahar 3DS），例如 `mGBA 系统·视频·音频·输入与性能` 2868-2913、
`Azahar 3DS 系统·图形与性能·视频流·纹理·音频与输入·调试` 3731-3975。
按键映射页分组：`游戏按键`/`功能热键`/`NDS:3DS 指针与屏幕`/`NDS 特殊功能`/`连发`（4108-4227），Arcade 另有独立分支 4112-4163。
核心浏览器过滤条：全部平台/任天堂系/世嘉系/索尼系/街机类/其他平台（自绘，4854-4881）。

### 3.3 控件与自绘元素

条目类型（全部自绘，`NanoSettingKind`）：

| 类型 | 数量（`grep -c "push_back(_xxx"`，排除定义行） |
|---|---|
| `Section` 分组标题 | 64 |
| `Toggle` 开关行 | 75 |
| `Selector` 选择器行（左/右箭头式） | 75 |
| `Action` 动作行 | 15（另有 `_filePickerItem` 5 + `_directoryItem` 1） |
| `TextValue` | 1（`_textValue` 2830；"用户名"走 IME 3770/3719） |
| `Platform` 平台入口 | 1 调用点（运行时 13） |
| `Binding` 按键绑定行 | 1 调用点（`_addBinding` 4233，运行时 5-40+） |

自绘元素：分类 Pill 栏、条目卡片（底/描边/渐变焦点框/图标/标题/副标题/值）、开关（圆角轨道+圆点）、
选择器覆盖层（居中面板 + 选项列表）、按键绑定捕获视图 `KeyCaptureView`（298-514，2 键组合、倒计时确认）、
SteamGridDB 弹窗（5267-5399）、底部提示条、自绘滚动条。

### 3.4 交互与提示

- `NanoSettingsCanvas::registerAction` ×15（1462-1484）：方向键 8 个（`_move(±2)` / `_adjust(±1)`）、
  LB `上一类`、RB `下一类`、A `选择`、X `清除`、BACK `恢复默认`、B `返回`；
  `setCustomNavigationRoute(四向→this)` 1425-1429；无 `getNextFocus` 覆写；滚动跟随 `_ensureFocusedVisible()` 4590-4613
- `KeyCaptureView` 注册 21 个手柄键 + 全部键盘 scancode（418-451）；文案 `L("按键捕获")`803、
  `L("当前绑定会在倒计时结束后确认")`806、`L("保持按键组合，松开后等待自动确认")`868、`L("松开所有已按下按键")`/`L("松开后开始捕获，最多可同时绑定 2 个按键")`469-470、`L("按下要绑定的按键")`/`L("支持手柄、摇杆方向、键盘和双键组合")`479-480、`L("已确认")`881
- 底部提示原文（5151-5175）：`返回`/`返回分类`/`返回平台`/`返回核心`/`返回核心管理`/`返回模拟器`/`取消`、`清除绑定`5160、`恢复默认`5163、`确认`/`选择`5164、`下一分类`/`上一分类`5167-5168、`下一类`/`上一类`5172-5173
- 面板页脚：`方向键选择 · A 确认 · B 返回`4762、`方向键选择 · A 进入平台 · B 返回模拟器`4809、`方向键选择 · A 查看设置 · B 返回核心管理`4810

### 3.5 死代码（重要）

`ModernSettingFrame` 1136-1277、`SettingsCategoryBar` 974-1134、`SettingsFooterBar` 915-972、
`SettingsSectionHeaderView` 105-142，以及 `buildUITab` / `buildGameTab` / `buildDisplayTab` / `buildAudioTab` /
`buildKeyBindTab` / `buildDebugTab`（5413/5652/5786/5918/6203/6251）**均无调用点**（只有声明 SettingPage.hpp:20-25 + 定义）。
它们内部承载了全部兼容层控件：`brls::SelectorCell` 29 个、`brls::BooleanCell` 21 个、`brls::DetailCell` 3 个、
`beiklive::DetailCell` 7 个。本文件 **`brls::Header` / `brls::ListItem` / `brls::InputListItem`(Slider) 均为 0 次**
（数值项一律用离散 SelectorCell/选择器列表，没有滑块）。

---

## 4. 关于：AboutPage

- 类/继承：`AboutPage : public beiklive::Box`（AboutPage.hpp:8）
- 入口：`AboutPage::AboutPage()` **AboutPage.cpp:3424-3446**（`brls::sync` 内 `showHeader(false)/showFooter(false)`
  → `new AboutMainCanvas(...)` 3431 → `contentBox->addView` 3443 → `giveFocus` 3444）
- **非** ScrollFrame、非三栏；是「顶栏标题 + 3 个 Tab 胶囊 + 单页内容区（左右双卡片）+ 底部按键提示」四段式

### 4.1 面板 / 区块

| 区块（原文） | 实现 | file:line |
|---|---|---|
| 顶栏：`GBAStation` + 版本号 | 自绘 `AboutMainCanvas::_draw*` | 3003-3063 |
| Tab 胶囊：`项目信息` / `更新与资源` / `支持作者` | 自绘（`_drawHeader` 类） | 3003-3063、内容分派 2816-2841 |
| `项目信息` 内容 | 自绘左右双卡片（左栏 345/390px） | `_drawInfo` 3136 |
| `更新与资源` 内容 | 自绘（含焦点 0..3 的条目） | `_drawUpdate` 3259 |
| `支持作者` 内容 | 自绘（头像 + 二维码/收款图 + 机种徽标） | `_drawSupport` 3347-3388 |
| 底部按键提示 | 自绘 | 3390-3421 |
| 卡片底板 | 自绘 `_drawPanel`（外阴影 `nvgBoxGradient` + 半透明填充 + 1.5px 描边） | 2962-3001 |
| 圆形头像 / 机种徽标 pill | 自绘 | 3090-3113 / 3115-3134 |

子页面（独立 Activity push，不嵌在本页）：

| 子页面 | 实现 | file:line |
|---|---|---|
| 更新日志 + FAQ（同一 Canvas，`faqMode` 开关） | `ChangelogCanvas`（自绘） | 890-1356 |
| 在线资源列表/下载 | `OnlineResourceCanvas`（自绘） | 1396-1997 |
| 下载进度弹窗 | `ResourceTransferDialog`（`brls::Dialog`）+ `ResourceProgressCanvas` | 2174-2212 / 1999-2172 |
| 更新检查 | `brls::Dialog` / `beiklive::UpdateDialog` / `beiklive::UpdatePage` | 3627 / 3633 / 3647 |

### 4.2 数据来源

| 显示内容 | 来源 | file:line |
|---|---|---|
| 版本号 | 编译宏 `APP_VERSION` | CMakeLists.txt:249；AboutPage.cpp:3428 |
| 更新源 `download.nswiki.cn` | **硬编码字面量** | AboutPage.cpp:3433 |
| 更新日志 | `readTextFile(BK_RES("changelog"))` | 3429-3430 |
| FAQ 全文 | cpp 内静态 `kFaqContent`（不读文件） | 770-867 |
| 在线资源清单 | HTTP GET `https://file.beiklive.top/file/GBAStation/res_version.json`，`parseResourceManifest` 解析 | 25-26 / 2249 / 176 |
| 是否"可更新/已安装" | 本地 `sdmc:/GBAStation/update/res_version.ini` 比对 | 103-126 / 224-229 |
| 图片 | `romfs:/img/beiklive.png`、`img/QQ.png`、`img/pay.png` | 2890-2894 |
| 机种徽标名称与颜色 | **硬编码数组** | 3192-3209 |
| `GitHub beiklive/GBAStation`、`BiliBili BEIKLIVE` | 纯绘制文字，**无点击跳转** | 3172-3179 |

**页面上并不存在的东西（已 grep 确认）**：无 `许可`/`License`、无 `编译时间`/`__DATE__`/`__TIME__`、无致谢名单区块；
版权相关内容只有自撰的 `免责声明` 与 `免费声明`（3253-3256）。

### 4.3 交互与提示

- 主画布 2702-2762：LEFT/NAV_LEFT/LB → 上一 Tab；RIGHT/NAV_RIGHT/RB → 下一 Tab（90ms 去抖 2897-2906）；
  UP/DOWN 仅在有焦点条目的 Tab 内移动；A `打开`（0.22s 点击动画 2798-2805）；B `返回` → `popActivity`
- 底部提示原文 3416-3420：`返回`(B) / `打开`(A) / `下一页`(RB) / `上一页`(LB)
- `ChangelogCanvas`：UP/DOWN 切版本、NAV_UP/DOWN + LB/RB 滚动 ±180px、B 返回；提示 `返回`/`下翻`/`上翻`（1311-1313）；FAQ chip 文字 `问`/`答`（1224）
- `OnlineResourceCanvas`：方向键+NAV 移动（80ms 去抖）、LB `上一类`/RB `下一类`、A `下载`、
  B 返回；提示 `返回`/`下载`/`下一类`/`上一类`（1985-1988）；状态徽章 `可更新`/`已安装`（1830）
- 下载进度弹窗 `setFocusable(false)` + `setCancelable(false)`（2005/2178）→ **下载中不可取消**

### 4.4 死代码

`_buildInfoTab` 3450 / `_buildUpdateTab` 3590 / `_buildSupportTab` 3980 / `_updateCheatDatabase` 3665 /
`_downloadNdsFirmware` 3807 / `_downloadNdsCheatDatabase` 3903 **全仓无调用点**；
`UpdateTabCanvas` 2402-2662 只在 `_buildUpdateTab` 里被 new；
`brls::ScrollingFrame/Label/Image/Header/Padding` 只出现在这些死函数中。

---

## 5. 数据管理：DataManagementPage

- 类/继承：`class DataManagementPage : public beiklive::Box`（DataManagementPage.hpp:21）
- 入口：构造 1682-1692（`showHeader(false)`/`showFooter(false)`/`setFocusable(false)`/`init()`）→
  `init()` 1854-1979 → `m_mainCanvas = new DataManagementCanvas(std::move(tabs), [this]{ popActivity(this); })` 1973
- 继承 `draw()` 覆写 1700-1705；`friend class ScanProgressDialogView`（hpp:30，进度弹窗直读私有原子量）

### 5.1 面板 / 区块

| 面板（原文） | 实现 | 内容/操作 | file:line |
|---|---|---|---|
| 页头 | 自绘 `_drawHeader` | `L("数据管理")` 1014 + 副标题 `L("导入、维护与远程管理游戏库")` 1017 + 居中 3D 标签选择器 1030-1072 + 分隔线 | 1006-1081 |
| Tab「整合包导入」概览（左栏） | 自绘 `_drawOverview` | 图标、标题、summary/detail、徽标 `"LPL"`/`L("先选平台")` | 1863-1882 / 1105-1162 |
| Tab「扫描导入」概览 | 自绘 | 徽标 `L("N 项已开启")`（统计开启项） | 1900-1926 |
| Tab「数据处理」概览 | 自绘 | 徽标 `L("谨慎操作")` | 1930-1971 |
| 条目列表面板（右栏） | 自绘 `_drawItems`/`_drawItem` | 行高按条目数 116/74/62 px；自绘滚动条；条目含开关、徽标、右箭头 | 1164-1208 / 1210-1298 |
| 底部提示条 | 自绘 `_drawFooter`/`_drawHint` | 按键图标 + 文字 | 1322-1335 / 1300-1320 |
| 弹窗「选择游戏平台」 | 画布内置模态 `OpenModal`/`_drawModal` | 3 列平台卡片（过滤未安装外置核心的平台），A 选中 | 2345-2371 / 1487-1586 |
| 弹窗「设置各平台游戏扫描目录」 | 画布内置模态 | 每平台卡片显示路径 / `L("未配置扫描目录")` / `L("已配置")` / `L("未配置")` | 2373-2406 |
| 进度弹窗 | `ScanProgressDialogView`（`brls::Box`：3×`brls::Label` + `brls::Box` 轨道 + `brls::Rectangle` 条）+ 外层 `brls::Dialog` | 标题/当前项/计数/进度条 | 1990-2156 / 2158-2165 |
| 弹窗「导入 RetroArch 游戏列表」 | `LplImportConfirmView`（自绘）+ `brls::Dialog` | 平台徽标、LPL 文件名卡片、说明；按钮 `取消`/`确定导入` | 197-314 / 2426-2437 |
| 弹窗「Web 管理服务已启动」 | `QRCodeView`（自绘二维码）+ `brls::Label` + `brls::Dialog` | 二维码、访问地址、保活提示、`关闭服务` | 3007-3068 |

条目徽标原文：`"LPL"`、`L("先选平台")`、`L(" 项已开启")`、`L("谨慎操作")`、`"开始"`、`"管理"`、`"局域网"`、
`"3DS"`、`L("不会删除 ROM")`、`L("危险操作")`、`".lpl"`（1144-1165、1873-1971）。

### 5.2 控件清单

| 类别 | 控件 | 用途 | 位置 |
|---|---|---|---|
| 原生 | `brls::Dialog` | 确认 / 信息 / 进度 / Web 服务弹窗 | 2161/2429/2473/2490/2499/2724/2894/2952/2955/3014/3061 |
| 原生 | `brls::Label` | 进度弹窗标题/当前项/计数、Web 弹窗文字 | 2001/2009/2018/3035/3046/3053 |
| 原生 | `brls::Rectangle` | 进度条填充（`nvgRGB(79,193,255)`，完成转 `nvgRGB(129,199,132)`） | 2033-2038 / 2095 |
| 原生 | `brls::Box` / `brls::AppletFrame` / `brls::Activity` | 弹窗内容容器、承载 `FileListPage` 的全屏壳 | 1994/2027/2443、2452-2456/2705-2709 |
| 原生 | `brls::ScrollingFrame` / `brls::Header` | **仅死代码**（旧版页） | 2214/2278 / 2216/2281 |
| 自定义 | `beiklive::Box` | 页面基类 | hpp:21 |
| 自定义 | `beiklive::FileListPage` | 选 lpl 文件 / 选扫描目录 | 2410-2458 / 2684-2711 |
| 自定义 | `beiklive::VideoBackgroundView` | 仅静态调用 `setSharedAudioSuspended(true)` | 3001 |
| 自定义 | `beiklive::DetailCell` / `beiklive::GridBox` / `beiklive::LazyCell` | **仅死代码**（旧版整合包页、2 列网格页） | 1638/2246/2283/2295/2306；1627-1654 |
| 自绘 | `DataManagementCanvas` | 页面主体（三标签、焦点、滚动、模态） | 643-1587（实例 1973） |
| 自绘 | `ScanProgressDialogView` / `LplImportConfirmView` / `QRCodeView` | 进度、LPL 确认、二维码 | 1990-2156 / 197-314 / 147-195 |

### 5.3 危险操作与确认流程

| 操作（原文） | 二次确认 | 回调 |
|---|---|---|
| `清空游戏库`（badge `危险操作`） | **两级** `brls::Dialog`：`L("确定要清空游戏库吗？")` → `L("真的要清空游戏库吗？\n数据都会丢失哦。")`，按钮 `取消`/`确定` | `clearGameLibrary()` 2948-2971（`GameDB->clearAll()` + 清空 database 目录 2960-2963） |
| `移除无效游戏记录`（badge `不会删除 ROM`） | 单级 `brls::Dialog`：`L("确定要从游戏库中移除无效游戏吗？…")`，按钮 `取消`/`确认移除` | `removeInvalidGames()` 2890-2946（后台线程 2903-2943） |
| `安装 CIA 文件` | 无确认；非 Switch 只 `notify(L("CIA安装器仅支持Switch"))` | `launchCiaInstaller()` 2973-3005 |
| `启动 Web 管理服务` | 无确认；失败弹 `L("Web 管理服务启动失败…")` | `startWebService()` 3007-3068 |
| `停止 Web 管理服务` | 无（弹窗 `setCancelable(false)`） | 3063-3066 |
| `导入 LPL 游戏列表` | 有：`LplImportConfirmView` + `取消`/`确定导入` | `startImport()` 2461-2603（确认 2426-2437） |
| `开始扫描` | 无；空配置时弹 `L("请先为至少一个机型选择扫描目录")` | `startScanAll()` 2714-2839（拦截 2722-2728） |
| 修改扫描目录 | 无，选完立即保存 `scan.path.*` | `pickScanDir()` 2682-2712 / `setScanPath()` 2627-2649 |

错误文案：`L("无法打开LPL文件")`2473、`L("LPL文件解析失败")`2490、`L("LPL文件无数据")`2499、
`L("游戏库数据已清空")`/`L("清空游戏库时发生错误")`2966、`L("CIA安装器启动失败：")`2996、
`L("正在启动CIA安装器...")`3002。

### 5.4 进度 / 异步

`ScanProgressDialogView::draw()` 轮询页面原子量：`m_bar->setWidth(480.f * frac)`、`m_countLabel->setText(cur + " / " + tot)`，
每帧 `invalidate()`（2047-2131）；错误态保留弹窗（2055-2071）；完成态进度条转绿 + 摘要
`L("共处理 N 个游戏，跳过 M 个已有游戏")` / `L("已移除 N 个无效游戏记录")`（2088-2114）；
`CloseAsync()` 2130/2137-2147。后台线程 `std::thread m_importThread`（hpp:63），
三个启动点：LPL 导入 2526、扫描导入 2746、清理无效记录 2903；状态原子量 hpp:64-74。

### 5.5 交互与提示

| 键 | 行为 | 提示原文 |
|---|---|---|
| LEFT/RIGHT/NAV | 切标签（模态内改为移动焦点） | 空 label |
| LB / RB | 上一标签 / 下一标签 | `L("上一页")` / `L("下一页")` 732-741 |
| UP/DOWN | 条目焦点移动 | 空 label 742-745 |
| A | 激活（条目有 `toggle` 时是切换开关） | `L("选择")` 746-749（footer 里会显示 `L("切换")`） |
| B | 关模态 / 请求关页 | `L("返回")` 750-753 |

页脚提示（1328-1334）：`L("返回")` / `L("切换")`或`L("选择")` / `L("下一页")` / `L("上一页")`；
模态右下角 `"方向键选择  ·  A 确认  ·  B 关闭"` 1583-1584。
去抖 `_acceptNavigation()` 85ms（940-950）；入场进度门槛 0.72/0.85（1337-1395）；点击动画 0.2s（835-844）。

### 5.6 死代码

`buildBundleImportTab()` 2207 / `buildDataProcessingTab()` 2271（旧版 ScrollingFrame + 14/3 个 `beiklive::DetailCell`）、
`DataManagementGridPage` 1589-1680（`beiklive::GridBox(2)` + `DetailCell`）——`init()` 中均未引用。

---

## 6. SwitchLayout（Switch 风格主界面）

- 类/继承：`class SwitchLayout : public beiklive::Layout`（SwitchLayout.hpp:21；`Layout : public brls::Box` Layout.hpp:10）
- 生命周期：构造 cpp:121、析构 172、`refreshGameList()` 191、`frame()` 508、`draw()` 913
- **纯自绘**证据：`getDefaultFocus()/getNextFocus()/getParentNavigationDecision()` 全部返回 this（hpp:54-63）；
  `setCustomNavigationRoute(四向→this)`（cpp:130-133）；用空 label 的 action 吞按键（cpp:157-168）；
  全文**无 `addView`**、**无 `brls::Label/Image/Button`**；`draw()` 只调 `_drawGames/_drawFunctions/_drawFooterHint/_drawPico8Shortcut`（cpp:921-928）

### 6.1 区域（按屏幕位置）

| 区域 | 绘制方法 | 内容 | file:line |
|---|---|---|---|
| 游戏卡片行（上半屏） | `_drawGames` | 10 个固定槽位（`HOME_CARD_SLOTS=10`），卡 220×350、间距 20、起点 x=30、y=115 | cpp:931/934、cpp:19-25 |
| 单张卡片 | `_drawGameCard` | 封面 + 阴影 + 标题（24px，溢出跑马灯）+ 平台徽章 + 游玩时长/上次游玩 | cpp:1082-1207 |
| 空卡片 | `_drawEmptyCard` | 虚线上边框占位块 + 可选渐变焦点框 | cpp:998-1044 |
| 删除动画卡片 | `_drawDeletingCard` | 抖动 + 红色描边 + 删除图标 + 收缩 | cpp:1046-1080 |
| 封面 | `_drawCover` | `nvgImagePattern` 圆角裁剪；无纹理时 shimmer 占位 | cpp:1209-1246 |
| 加载骨架 | `_drawGames` 内 `m_loading` 分支 | 10 个 shimmer 方块 | cpp:938-967 |
| 功能按钮行（下部胶囊条） | `_drawFunctions` | pitch 100、条高 95、`y + h - 90 - 95`；胶囊 + 图标 + 文字 | cpp:1248-1370 |
| 底部提示栏（右） | `_drawFooterHint` | A `L("选择")`/`L("打开")` + START `L("设置主页")` | cpp:1372-1403 |
| 底部状态栏（左） | `_drawFooterHint` | WiFi 字形 + 时钟（`%H:%M:%S`，每秒刷新） | cpp:1405-1426、cpp:730-745 |
| Pico8 快捷入口（左下） | `_drawPico8Shortcut` | LB 键胶囊 + PICO-8 logo（`img/pico8_logo_vector.png`） | cpp:1429-1550 |

### 6.2 视觉元素（自绘）

卡片阴影 `nvgBoxGradient`+`NVG_HOLE`（1106-1114）、卡片底/描边（1116-1122）、封面（1240-1245）、
占位 shimmer（1215-1224）、标题跑马灯（1130-1149）、平台徽章（1171-1183）、时长/上次游玩（1151-1198）、
渐变焦点边框 `drawGradientFocusBorder`（1200-1205/1037-1042）、渐变焦点圆 `drawGradientFocusCircle`（1354-1359）、
功能胶囊条（1268-1292）、功能图标（1329-1352）、点击缩放（1308-1327）、删除态（1068-1078）、
底部键位字形（`brls::Hint::getKeyIcon`，1388-1394）、WiFi/时钟（1405-1426）、Pico8 胶囊与 logo（1479-1544）。

### 6.3 交互模型

- 输入入口：`_captureInputState()` 643、`_updatePico8ShortcutInput()` 660、`_handleInput()` 760（frame 末尾 638 调用）
- 总闸门：未聚焦 / `isInputBlocks()` / 各种动画中 → 直接清空计时返回（775-788）
- 两行焦点 `enum FocusRow { GAMES, FUNCTIONS }`（hpp:66-70）：
  `_moveHorizontal` 837（按槽位数/功能数取模循环，首尾相接 `m_fastScroll`）、
  `_moveVertical` 853（只在两行间切换）、`_activateCurrent` 865、`_activateFunction` 883、`_updateTargetScroll` 896
- 按键：A 上升沿激活（798-800）；左右/上下 含摇杆阈值 ±0.5（769-772、790-797）；
  **长按连发仅左右**（`HOLD_DELAY=0.30s`、`HOLD_REPEAT=0.085s`，26-27、811-828）；上下无连发
- LB = Pico8 快捷入口：**不是长按触发，而是"按住缩放 + 松手触发"**——按下 `m_pico8HoldActive`（691-695，缩放目标 1.065），
  松手进入 `m_pico8ReleaseAnimating`（697-701），`m_pico8ReleaseTime >= 0.13f` 且回正后触发（708-721）
- START `L("设置主页")` 不在本视图注册，由 StartPage 注册（StartPage.cpp:1862-1867）
- **B / X / Y / L / R / ZL / ZR：未确认**（本文件 grep 无这些 BUTTON_* 引用；若存在处理在其他层）
- 提示条原文：`m_focusRow == GAMES ? L("选择") : L("打开")`（1377-1378）、`L("设置主页")`（1403）

### 6.4 数据与状态

- 列表：`refreshGameList(beiklive::GameList)` 191；`GameList = std::vector<GameEntry>`（enums.h:170）；
  用到的字段 `path/title/logoPath/platform/playTime/lastPlayed`；只取前 10 条（19、217-224）；
  按 path 恢复选中（211-214、250-258）；删除动画期间刷新请求缓存（`m_pendingGameList`，193-197/598-603）
- 纹理：工作线程 `stbi_load` 解码并缩放到最长边 384(Switch)/512(桌面) → 主线程每帧最多上传 2/4 张
  `nvgCreateImageRGBA`，失败记 `m_failedTextures`，dirty 时淘汰（hpp:95-104、1605-1775）
- 状态：时钟 `%H:%M:%S`（730-745）、网络 `getIpAddress()` + `hasWirelessConnection()||hasEthernetConnection()`（747-757）；
  **电量未确认**（本文件无 battery 相关代码）

### 6.5 与页面的关系

点卡片 → `onGameOptions` → `GameOptionsSidebar`（872-873）；点功能项 → §1 表格；
LB 松手 → `onPico8Opened` → `Pico8Page`（716-721、StartPage.cpp:2016-2018）；
反向注入 `beginPico8ReturnAnimation/setPico8ReturnProgress/finishPico8ReturnAnimation`（461/475/483）。
注意：`Layout::onGameActivated`（Layout.hpp:21）在本文件中**未被调用**。

---

## 7. 跨页面汇总

### 7.1 borealis 原生控件使用矩阵

| 控件 | 游戏库 | 设置 | 关于 | 数据管理 | SwitchLayout |
|---|---|---|---|---|---|
| `beiklive::Box`（页面壳，继承 brls::Box） | ✅ 基类 | ✅ 基类 | ✅ 基类 | ✅ 基类 | ✅ 基类（Layout→brls::Box） |
| `brls::AppletFrame` + `brls::Activity` | ✅ 2/3 | ✅（仅兼容层） | ✅（子页 push） | ✅ | — |
| `brls::Dialog` | ✅ 13 | ❌ 0 | ✅（更新/资源） | ✅ 11 | ❌ |
| `brls::Dropdown` | ✅ 3 | ❌ | ❌ | ❌ | ❌ |
| `brls::Label` | ✅（死代码） | ✅（进度/兼容层） | ✅（子页） | ✅（进度弹窗） | ❌ |
| `brls::Rectangle` | ❌ | ✅（兼容层） | ❌ | ✅（进度条） | ❌ |
| `brls::SelectorCell` / `BooleanCell` | ❌ | ✅ **仅死代码**（29/21） | ❌ | ❌ | ❌ |
| `brls::DetailCell` | ❌ | ✅ 仅死代码（3） | ❌ | ❌ | ❌ |
| `brls::Header` / `brls::ScrollingFrame` | ✅ 仅死代码 | 0 / 仅兼容层 | ✅ 仅死代码 | ✅ 仅死代码 | ❌ |
| `brls::BottomBar` | ✅（GONE） | — | — | — | ❌ |
| `brls::Hint::getKeyIcon` | ✅（自绘用） | ✅ | ✅ | ✅ | ✅ |

### 7.2 本仓库自定义控件 / 视图

| 类 | 定义 | 用途 | 使用页面 |
|---|---|---|---|
| `beiklive::Box` | widget/Box.hpp:15 | 页面壳（背景层/页头/页脚/内容层） | 4 个页面 + SwitchLayout 基类 |
| `beiklive::HeaderBar` | widget/Header.hpp:8 | 页头标题/副标题（本批页面基本隐藏） | 游戏库、设置（兼容层） |
| `beiklive::HintsBar` | widget/HintsBar.hpp:9 | 底部按键提示栏 | **本批 5 个界面均未使用** |
| `beiklive::DetailCell` | widget/DetailCell.hpp:11 | 左文+右值的单行卡片 | 仅死代码路径 |
| `beiklive::GridBox` / `LazyCell` / `GridItem` | widget/GridBox.hpp:45、18；GridItem.hpp:16 | 2 列网格 / 懒加载单元 / 格子内容 | 仅死代码路径 |
| `beiklive::TabFrame` | widget/TabFrame.hpp:9 | 分页容器 | 仅死代码路径 |
| `beiklive::ButtonBox` | widget/ButtonBox.hpp:10 | 带按钮的卡片 | 仅死代码路径 |
| `beiklive::RoundButton` / `SwitchButton` / `SelectorButton` / `NumberButton` | widget/RoundButton.hpp:9、FunctionButtons.hpp:12/37/63 | 圆按钮 / 开关 / 选择 / 数字按钮 | 本批界面**未直接使用**（AboutPage 用到 `SwitchButton` 之名） |
| `beiklive::GameCard` | widget/GameCard.hpp:10 | 游戏卡片（带焦点缩放/信息滑入） | 未在本次 5 界面（SwitchLayout 自绘卡片） |
| `beiklive::GameOptionsSidebar` | view/GameOptionsSidebar.hpp:22 | 右侧操作侧边栏（支持 nanoVG 自绘模式） | 游戏库 |
| `beiklive::NanoSearchOverlay` | view/NanoSearchOverlay.hpp:11 | 搜索浮层（自绘） | 游戏库 |
| `GameGridView`（全局命名空间） | view/RecyclingGrid.hpp:18 | 游戏库主体（网格/列表/详情/多选/删除动画） | 游戏库 |
| `beiklive::FileListPage` | page/FileListPage.hpp:13 | 文件/目录选择 | 数据管理 |
| `beiklive::ImageView` | view/ImageView.hpp:8 | 全屏图片预览 | 游戏库（死代码路径） |
| `beiklive::UpdateDialog` / `beiklive::UpdatePage` | widget/UpdateDialog.hpp:9、page/UpdatePage.hpp | 更新提示 / 更新页 | 关于 |
| `beiklive::VideoBackgroundView` / `GifBackgroundView` | widget/*.hpp | 共享背景（MP4/GIF） | 数据管理（静态调用）等 |

### 7.3 自绘元素矩阵（"同一类控件，各页各画一遍"）

| 自绘元素 | 游戏库 | 设置 | 关于 | 数据管理 | SwitchLayout |
|---|---|---|---|---|---|
| 页面标题/副标题栏 | ✅ | ✅ 6 Pill | ✅ 3 Pill | ✅ 3D 选择器 | — |
| 列表条目行（图标+标题+副标题+值/箭头） | ✅（格子） | ✅（`_drawItem`） | ✅（更新/资源项） | ✅（`_drawItem`） | — |
| 开关（Toggle） | — | ✅ | — | ✅ | — |
| 选择器（左/右选项） | ✅ 平台轮播 | ✅ + 覆盖层 | — | ✅ 标签选择器 | ✅ 功能行 |
| 卡片/面板底（阴影+描边+圆角） | ✅ | ✅ `_drawPanel` | ✅ `_drawPanel` | ✅ `_drawPanel` | ✅ |
| 渐变焦点框/流光 | ✅ | ✅ | ✅ | ✅ | ✅ |
| 自绘滚动条 | ✅ | ✅ | ✅ | ✅ | ✅（横向） |
| 底部按键提示条 | ✅ | ✅ | ✅ | ✅ | ✅ |
| 徽章 pill | ✅ | — | ✅ 机种 | ✅ 状态 | ✅ 平台 |
| 进度条 | — | — | ✅（资源下载） | ✅ | — |
| 模态/遮罩 | — | ✅ 选择器 | ✅ | ✅ 平台/目录 | — |
| 特殊：二维码 | — | — | ✅（收款） | ✅（Web 服务） | — |

---

## 8. 对"统一组件库"的映射建议（本节是建议，不是代码里已有的抽象）

如果把上表按"控件语义"收敛，这 5 个界面真正需要的组件只有十几类（其余都是布局与动画）：

| 自绘元素 | 建议抽象为 | 现有哪些实现可合并 |
|---|---|---|
| 列表条目（图标+标题+副标题+右值/开关/箭头） | `SettingItem`（type: Action/Toggle/Selector/TextValue/Binding） | SettingPage `_drawItem/_drawToggle/_drawBinding`、DataManagementPage `_drawItem` |
| 分页/分类选择器 | `TabStrip`（Pill 或 3D 轮播） | SettingPage `_drawHeader`、DataManagementPage `_drawHeader`、AboutPage Tab 胶囊、SwitchLayout 功能行 |
| 卡片网格（封面/网格项） | `CardGrid` | GameGridView、DataManagementPage `DataManagementGridPage`（死代码） |
| 覆盖层菜单 / 侧边栏 | `OverlayMenu`（遮罩 + 面板 + 按钮列表 + 模态焦点） | GameOptionsSidebar、NanoSearchOverlay、SettingPage `_drawSelector`/Steam 弹窗、DataManagementPage `_drawModal` |
| 确认/信息/进度弹窗 | `Dialog`（`brls::Dialog` + 进度内容视图） | ScanProgressDialogView、LplImportConfirmView、ResourceTransferDialog |
| 底部按键提示条 | `HintsBar`（已存在但本批 5 界面都没用，各自自绘） | 5 处各自 `_drawFooter`/`_drawHint` |
| 徽章 | `Badge` | 各页 `_drawBadge` |
| 自绘滚动条 | `ScrollBar` | 5 处重复实现 |
| 二维码 | `QRCodeView` | AboutPage / DataManagementPage 各一份 |

---

## 9. 证据、口径与未确认项

- **读法**：5 个文件全文阅读（GameLibraryPage.cpp 2035 行、SettingPage.cpp 6391 行、AboutPage.cpp 4043 行、
  DataManagementPage.cpp 3070 行、SwitchLayout.cpp 1776 行），并用 grep 交叉验证"死代码/调用点/控件出现次数"。
- **数量口径**：文中的"N 个控件类"按"代码里出现并实例化/作基类的 View 类型"计，含仅出现在死代码里的（已单独标注）；
  不计数据源类与页面类本身。不同统计起点会得到不同数字（例如设置页 20~23 个），已在各处写明口径。
- **未确认**：
  1. SwitchLayout 里 B / X / Y / L / R / ZL / ZR 的处理位置（本文件无引用，可能在 StartPage 层）；
  2. SwitchLayout 的电量指示（本文件无 battery 代码）；
  3. 各页"死代码"是否被其它构建配置/历史分支引用（本仓库 `src/` 内 grep 无调用点）；
  4. AboutPage 的在线资源下载是否有 URL 校验/回退（未逐行核对网络层实现）。
