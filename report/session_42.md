# Session 42 – 移除 StartPageView 切换功能 & 新增 ProImage 类

## 任务概述

### 任务 1
移除 `StartPageView` 中 App 界面与文件列表切换功能，启动页面只显示 App 界面，文件列表通过 AppPage 的 `m_ButtonRow` 中的按钮打开，并移除切换按钮绑定。

### 任务 2
在 `UI/Utils` 中添加 `ProImage` 类，继承自 `brls::Image`，支持：
- Kawase Blur 模糊处理（可开关）
- 播放 GIF 动画
- 播放着色器动画（PSP 线条动画）

---

## 修改文件

### 1. `include/UI/StartPageView.hpp`

**变更：**
- 移除 `m_activeIndex` 成员变量（0/1 标记当前显示页面）
- 将 `showFileListPage()` 改名为 `openFileListPage()`，语义更清晰

```diff
-    int  m_activeIndex = 0; ///< 0 = AppPage, 1 = FileListPage
-    void showFileListPage();
+    void openFileListPage();
```

---

### 2. `src/UI/StartPageView.cpp`

**变更：**

#### `Init()` 函数
- 移除基于 `KEY_UI_START_PAGE` 配置的 startPageIndex 分支逻辑
- 启动页面始终创建并显示 `AppPage`

```diff
- int startPageIndex = 0;
- if (SettingManager && SettingManager->Contains(KEY_UI_START_PAGE))
-     startPageIndex = *gameRunner->settingConfig->Get(KEY_UI_START_PAGE)->AsInt();
- if (startPageIndex == 0) { createAppPage(); showAppPage(); }
- else { createFileListPage(); showFileListPage(); }
+ createAppPage();
+ showAppPage();
```

#### `createAppPage()` 函数
- 新增 `m_appPage->onOpenFileList` 回调绑定，点击文件列表按钮时调用 `openFileListPage()`

```cpp
m_appPage->onOpenFileList = [this]() {
    openFileListPage();
};
```

#### `showAppPage()` 函数
- 移除 LT 按钮绑定（原来 LT → 切换到 FileListPage）
- 移除 `m_activeIndex` 赋值
- 移除 `beiklive::swallow(this, brls::BUTTON_RT)`

#### `openFileListPage()` 函数（原 `showFileListPage()`）
- 移除 RT 按钮绑定（原来 RT → 切换回 AppPage）
- 改用 **B 键** 返回 AppPage（更符合 brls 导航惯例）
- 从视图树中移除 AppPage（detach，不销毁），再添加 FileListPage

```cpp
// Bind B → return to AppPage
registerAction("beiklive/hints/APP"_i18n,
               brls::ControllerButton::BUTTON_B,
               [this](brls::View*) {
                   removeView(m_fileListPage, false);
                   showAppPage();
                   return true;
               });
```

---

### 3. `include/UI/Pages/AppPage.hpp`

**变更：**
- 新增 `onOpenFileList` 回调成员

```cpp
/// 用户点击"文件列表"按钮时调用
std::function<void()> onOpenFileList;
```

---

### 4. `src/UI/Pages/AppPage.cpp`

**变更：**
- 将 `m_ButtonRow` 中各按钮标签统一为正确的中文名称（之前全部显示为"文件列表"）
- 将"文件列表"按钮的点击回调改为调用 `onOpenFileList`（若已绑定则触发）

```diff
- m_ButtonRow->addButton(..., "文件列表", []() { brls::Logger::debug("文件列表"); });
- m_ButtonRow->addButton(..., "文件列表", []() { brls::Logger::debug("数据管理"); });
- m_ButtonRow->addButton(..., "文件列表", []() { brls::Logger::debug("设置"); });
- m_ButtonRow->addButton(..., "文件列表", []() { brls::Logger::debug("关于"); });
- m_ButtonRow->addButton(..., "文件列表", []() { brls::Logger::debug("退出程序"); });
+ m_ButtonRow->addButton(..., "文件列表", [this]() { if (onOpenFileList) onOpenFileList(); });
+ m_ButtonRow->addButton(..., "数据管理", []() { brls::Logger::debug("数据管理"); });
+ m_ButtonRow->addButton(..., "设置",     []() { brls::Logger::debug("设置"); });
+ m_ButtonRow->addButton(..., "关于",     []() { brls::Logger::debug("关于"); });
+ m_ButtonRow->addButton(..., "退出程序", []() { brls::Logger::debug("退出程序"); });
```

---

### 5. `include/UI/Utils/ProImage.hpp` （新增文件）

新增 `ProImage` 类，继承自 `brls::Image`。

```cpp
namespace beiklive::UI {

enum class ShaderAnimationType { NONE, PSP_LINES };

class ProImage : public brls::Image {
public:
    // Kawase Blur
    void setBlurEnabled(bool enabled);
    bool isBlurEnabled() const;
    void setBlurRadius(float radius);
    float getBlurRadius() const;

    // Animated GIF
    void setImageFromGif(const std::string& path);

    // Shader Animation
    void setShaderAnimation(ShaderAnimationType type);
    ShaderAnimationType getShaderAnimation() const;

    // Override
    void draw(NVGcontext* vg, float x, float y, float w, float h,
              brls::Style style, brls::FrameContext* ctx) override;
    ...
};
```

---

### 6. `src/UI/Utils/ProImage.cpp` （新增文件）

`ProImage` 的完整实现，包含：

#### Kawase Blur（`drawBlur()`）
- 在当前图片绘制完成后，叠加多个偏移副本（共 3 轮 × 8 方向 = 24 次），每次副本透明度为 `BASE_ALPHA = 0.12`
- 偏移量随每轮递增（`r * pass / BLUR_PASSES`）
- 使用 `nvgScissor` 确保不超出控件边界

#### GIF 动画（`setImageFromGif()`）
- 使用 stb_image 内置的 `stbi_load_gif_from_memory()` 解码所有帧
- 每帧创建 NanoVG 纹理（`nvgCreateImageRGBA`）存入 `m_gifFrames`
- 在 `draw()` 中按帧延迟时间（`delay_ms`）逐帧切换 `texture`，调用 `invalidate()` 持续驱动重绘

#### PSP 线条动画（`drawPspLines()`）
- 绘制若干条半透明斜线（角度约 20°），在纵向方向随时间匀速流动
- 用 `std::fmod` 实现无缝循环
- 透明度随位置正弦变化，产生渐入渐出效果

---

## 测试验证

项目在 Linux 桌面平台（`-DPLATFORM_DESKTOP=ON`）下成功完整编译，无新增错误（仅存在一处已有的 `nvgRGBA(256)` 溢出警告，与本次改动无关）。

---

## 用法示例

```cpp
// 使用 ProImage 替换 brls::Image
auto* img = new beiklive::UI::ProImage();
img->setImageFromFile("path/to/image.png");

// 开启模糊，半径 12px
img->setBlurEnabled(true);
img->setBlurRadius(12.0f);

// 播放 GIF
img->setImageFromGif("path/to/animation.gif");

// 叠加 PSP 风格线条动画
img->setShaderAnimation(beiklive::UI::ShaderAnimationType::PSP_LINES);
```
