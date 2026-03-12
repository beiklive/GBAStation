# Session 43 – 文件列表 pushActivity / GIF 时钟控速 / PSP XMB 波纹着色器

## 任务概述

### 任务 1：文件列表改用 pushActivity
将 `openFileListPage()` 从 `addView` 模式改为 `pushActivity` 模式，并绑定 `+`（BUTTON_START）关闭页面（`popActivity`）。

### 任务 2：ProImage GIF 播放速率改用时钟控制
用 `std::chrono::steady_clock` 替换固定帧间距假设（`ASSUMED_FRAME_MS`），以真实的壁钟时间驱动 GIF 帧切换。

### 任务 3：ProImage PSP 效果改为 XMB 波纹着色器
删除 `drawPspLines`（NanoVG 斜线动画），改用 OpenGL GLSL 着色器绘制经典 PSP XMB 波纹丝带背景，并在 `StartPageView` 背景上启用。

---

## 修改文件

### 1. `include/UI/StartPageView.hpp`

**变更：**
- 移除 `m_fileListPage` 成员（文件列表页面现在由 Activity 自行管理生命周期）
- 移除 `m_settingsPanel` 成员（设置面板随每个 Activity 新建）
- 移除 `createFileListPage()` 声明
- 移除 `onFileSettingsRequested()` 声明

```diff
-    FileListPage*      m_fileListPage  = nullptr;
-    FileSettingsPanel* m_settingsPanel = nullptr;
-    void createFileListPage();
-    void onFileSettingsRequested(const FileListItem& item, int itemIndex);
```

---

### 2. `src/UI/StartPageView.cpp`

**构造函数：**
- 移除 `m_settingsPanel` 的构造与配置代码
- 新增 `m_bgImage->setShaderAnimation(PSP_XMB_RIPPLE)` 以启用 PSP XMB 波纹背景

**删除方法：**
- `createFileListPage()` – 逻辑内联到 `openFileListPage()`
- `onFileSettingsRequested()` – 由 lambda 替代

**`openFileListPage()` 重写（核心改动）：**
```cpp
void StartPageView::openFileListPage()
{
    // 1. 新建 FileListPage（每次 push 时全新创建）
    auto* fileListPage = new FileListPage();

    // 2. 新建 Box 容器（Activity 的内容视图）
    auto* container = new brls::Box(brls::Axis::COLUMN);

    // 3. 新建 FileSettingsPanel，以 GONE 可见性加入容器
    //    （始终归容器所有，避免内存泄漏）
    auto* settingsPanel = new FileSettingsPanel();
    settingsPanel->setVisibility(brls::Visibility::GONE);
    container->addView(settingsPanel);

    // 4. 绑定设置面板回调
    fileListPage->onOpenSettings = [container, settingsPanel, fileListPage]
        (const FileListItem& item, int idx)
    {
        container->removeView(settingsPanel, false); // 提升至顶层
        container->addView(settingsPanel);
        settingsPanel->showForItem(item, idx, fileListPage);
    };

    // 5. 注册各扩展名回调（图片 / ROM）

    // 6. 导航到上次路径或默认路径
    container->addView(fileListPage);

    // 7. 绑定 + 键关闭（BUTTON_START = Switch + 键）
    container->registerAction("close", brls::BUTTON_START,
        [](brls::View*) { brls::Application::popActivity(); return true; });

    // 8. 推送新 Activity
    auto* frame = new brls::AppletFrame(container);
    frame->setHeaderVisibility(brls::Visibility::GONE);
    frame->setFooterVisibility(brls::Visibility::GONE);
    brls::Application::pushActivity(new brls::Activity(frame));
}
```

---

### 3. `include/UI/Utils/ProImage.hpp`

**变更：**
- 将 `PSP_LINES` 枚举值改名为 `PSP_XMB_RIPPLE`，语义更准确
- 新增 `#include <chrono>` 及 `#include <glad/glad.h>`（在 `BOREALIS_USE_OPENGL` 守卫内）
- 新增 GIF 计时成员：`m_gifLastTime`, `m_gifTimerStarted`
- 新增着色器计时成员：`m_shaderLastTime`, `m_shaderTimerStarted`
- 新增 PSP XMB OpenGL 资源成员（仅在 `BOREALIS_USE_OPENGL` 下编译）：
  - `m_xmbProgram`, `m_xmbVAO`, `m_xmbVBO` – 着色器程序与几何数据
  - `m_xmbFbo`, `m_xmbFboTex`, `m_xmbFboW`, `m_xmbFboH` – 离屏帧缓冲
  - `m_xmbNvgImage` – NanoVG 图像句柄
- 新增私有方法：`initXmbShader()`, `resizeXmbFbo()`, `drawPspXmbShader()`, `freeXmbResources()`
- 移除 `drawPspLines()` 声明

---

### 4. `src/UI/Utils/ProImage.cpp`

#### GIF 时钟控速（任务 2）

```diff
- m_gifElapsedMs += ASSUMED_FRAME_MS;
- if (m_gifElapsedMs >= threshold) { ... advance one frame ... }
+ // 用真实壁钟时间推进
+ float deltaMs = duration<float, milli>(now - m_gifLastTime).count();
+ m_gifLastTime  = now;
+ m_gifElapsedMs += deltaMs;
+
+ float threshold = gifFrames[currentFrame].delay_ms;
+ while (m_gifElapsedMs >= threshold) {   // 支持大 delta 跨多帧
+     m_gifElapsedMs -= threshold;
+     m_gifCurrentFrame = (m_gifCurrentFrame + 1) % frames.size();
+     threshold = gifFrames[currentFrame].delay_ms;  // 更新为新帧延迟
+ }
```

着色器动画时间也改为 `chrono::steady_clock` delta 累加，替代固定的 `ASSUMED_FRAME_SEC`。

#### PSP XMB 波纹着色器（任务 3）

新增 GLSL 着色器源码：

**顶点着色器**（`k_xmbVertSrc`）：
- 渲染覆盖 Widget 区域的全屏四边形（NDC 坐标）
- 输出 UV 坐标 `vUV` 到片段着色器

**片段着色器**（`k_xmbFragSrc`）：
- 深蓝渐变背景
- 24 条丝带：基于 XMB 公式（来自 RetroArch）`cos(z*4) * cos(z + time/10 + x)` 计算波动位移
- 叠加 IQ 噪声使波纹更有机感
- 丝带颜色：蓝色到紫色渐变，底部加亮
- 竖向渐晕（柔和边缘）

**`initXmbShader()`**：编译并链接 GLSL 程序，创建 VAO/VBO。

**`resizeXmbFbo(w, h)`**：按 Widget 尺寸创建/更新离屏 FBO 与纹理。
- GLES2 兼容：内部格式使用 `GL_RGBA`（非 `GL_RGBA8`）

**`drawPspXmbShader(vg, x, y, w, h)`**：
1. 保存 GL 状态（FBO、viewport、program、VAO、VBO）
2. 绑定 FBO，设置 viewport = widget 尺寸，清空颜色
3. 绑定着色器，写入 uTime / uResolution 均匀量
4. 上传并绘制全屏四边形
5. 恢复所有已保存的 GL 状态
6. 将 FBO 纹理导入 NanoVG（`nvgImageFromRawTexture` 封装，避免宏重复）
7. 通过 NanoVG `nvgImagePattern` 绘制到屏幕

新增 `nvgImageFromRawTexture()` 辅助函数，封装后端分支选择逻辑（避免重复 `#ifdef`）。

---

## 技术说明

### FBO 与 NanoVG 共存

NanoVG 在 `nvgEndFrame()` 时批量刷新 GL 指令，`draw()` 调用期间不修改 GL 状态。因此，可在 `draw()` 内安全进行 GL 着色器渲染，只需：
- 保存/恢复 FBO 绑定、viewport、shader program、VAO、VBO
- NanoVG 在 `renderFlush` 时自行设置 blend、stencil 等状态，无需特殊处理

### 内存安全

- `settingsPanel` 以 `GONE` 可见性加入 `container`，始终归 container 所有；Activity 弹出时随容器一同销毁，不存在泄漏。
- lambda 捕获的三个指针（`container`、`settingsPanel`、`fileListPage`）生命周期完全一致，不存在悬空指针风险。

---

## 编译验证

Linux 桌面平台（`-DPLATFORM_DESKTOP=ON`）完整编译通过，仅存在已有的第三方库警告与 `nvgRGBA(256)` 溢出警告（与本次改动无关）。
