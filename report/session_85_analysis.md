# Session 85 任务分析

## 任务目标

在 GameMenu 中添加"画面设置"选项，并添加对应的面板窗口，包含遮罩设置和着色器设置两个模块。

## 任务详情

### 遮罩设置模块（Header: 遮罩设置）
- 按钮1：遮罩开关（BooleanCell），直接复用 `KEY_DISPLAY_OVERLAY_ENABLED` 配置项
- 按钮2：遮罩路径选择（DetailCell），根据当前游戏平台显示对应路径选择器：
  - GBA 游戏 → `KEY_DISPLAY_OVERLAY_GBA_PATH`
  - GBC/GB 游戏 → `KEY_DISPLAY_OVERLAY_GBC_PATH`

### 着色器设置模块（Header: 着色器设置）
- 按钮1：着色器开关（BooleanCell）占位，功能不实现
- 按钮2：着色器选择（DetailCell）占位，功能不实现
- 按钮3：着色器参数设置（Button）占位，功能不实现

## 输入/输出分析

**输入：**
- 当前游戏平台（EmuPlatform：GBA 或 GB）
- 全局配置（SettingManager）中的遮罩设置

**输出：**
- GameMenu 中新增"画面设置"按钮
- 点击后在右侧面板显示画面设置内容

## 影响的文件

1. `resources/i18n/zh-Hans/beiklive.json` — 添加着色器相关 i18n 字符串
2. `resources/i18n/en-US/beiklive.json` — 添加着色器相关英文 i18n 字符串
3. `include/UI/Utils/GameMenu.hpp` — 添加 `setPlatform()` 方法和相关成员变量
4. `src/UI/Utils/GameMenu.cpp` — 实现"画面设置"面板
5. `src/UI/StartPageView.cpp` — 在 `launchGameActivity` 中传入游戏平台信息

## 技术挑战

1. **平台传递**：GameMenu 创建时可能不知道平台信息，需通过 `setPlatform()` 方法延迟设置
2. **互斥面板**：金手指面板和画面设置面板应互斥显示（只能显示一个）
3. **遮罩路径选择**：复用 SettingPage 中的文件选择逻辑，需要在 GameMenu.cpp 中包含 FileListPage.hpp
4. **循环导航**：左栏按钮增加后需要更新循环导航路由

## 解决方案

1. 添加 `setPlatform(EmuPlatform)` 方法，在 `launchGameActivity` 中从 romPath 扩展名检测平台后调用
2. 在按钮切换时将另一个面板隐藏（互斥）
3. 直接在 GameMenu.cpp 中复用 FileListPage 文件选择逻辑
4. 更新首尾按钮的自定义导航路由
