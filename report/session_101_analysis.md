# Session 101 分析报告：着色器选择与尺寸稳定性修复

## 任务目标

1. 着色器文件选择支持 `.glslp` 和 `.glsl` 两种格式
2. 分析并修复使用 `agb001.glslp` 时着色器管线输出尺寸不稳定的问题

## 问题分析

### 问题一：着色器文件选择仅支持 .glslp

**定位**：
- `src/UI/Utils/GameMenu.cpp` 第 338 行：`flPage->setFilter({"glslp"}, ...)`
- `src/UI/Pages/SettingPage.cpp` 第 999 行：`flPage->setFilter({"glslp"}, ...)`

**原因**：FileListPage 的过滤器只包含 `"glslp"`，不包含 `"glsl"`，导致用户无法选择单个 `.glsl` 着色器文件。

### 问题二：管线不支持 .glsl 单文件

**定位**：`src/Video/renderer/RetroShaderPipeline.cpp` 的 `init()` 方法

**原因**：`RetroShaderPipeline::init()` 直接调用 `GLSLPParser::parse()`，该函数期望文件格式为 `.glslp`（需包含 `shaders = N` 等配置键）。传入 `.glsl` 文件时解析失败，管线未加载。

### 问题三：尺寸一直在变（核心问题）

**定位**：`src/Game/game_view.cpp` 第 2176-2178 行

```cpp
GLuint chainOut = m_renderChain.run(m_texture, m_texWidth, m_texHeight,
                                    static_cast<unsigned>(width),   // ← 问题所在
                                    static_cast<unsigned>(height));  // ← 问题所在
```

**根本原因分析**：
1. `draw()` 函数的 `width` 和 `height` 参数是 `float` 类型（borealis 布局坐标系）
2. borealis 在每帧布局计算中，这些浮点值可能存在微小波动（如 1279.9998 和 1280.0001）
3. `static_cast<unsigned>` 执行**截断**而非四舍五入：
   - `1280.0001f` → 截断 → `1280`
   - `1279.9998f` → 截断 → `1279`
4. 对于 `agb001.glslp` 的 Pass 1（`scale_type = viewport`），输出尺寸计算为：
   ```
   outW = round(viewW × 1.0) = round(1279) = 1279  ← 某些帧
   outW = round(viewW × 1.0) = round(1280) = 1280  ← 其他帧
   ```
5. `allocateFBO()` 中的判断：
   ```cpp
   if (pass.fbo && pass.width == w && pass.height == h) return true;
   ```
   只要尺寸不一致，就会**重新分配 FBO**，生成新的 OpenGL 纹理 ID
6. 新的纹理 ID 导致 `m_nvgImageSrc != displayTex`，触发 NVG 图像句柄的**删除和重建**
7. 结果：每帧都在分配/释放 GPU 资源，造成性能下降和视觉不稳定

**修复方案**：将 `static_cast<unsigned>(width/height)` 改为 `static_cast<unsigned>(std::lround(width/height))`，使用四舍五入代替截断，确保浮点微小波动不影响整数尺寸。

## 修复方案

### 修复一：着色器文件过滤器添加 glsl 支持
- `GameMenu.cpp` 和 `SettingPage.cpp` 中的 `setFilter({"glslp"}, ...)` 改为 `setFilter({"glslp", "glsl"}, ...)`

### 修复二：RetroShaderPipeline 支持 .glsl 单文件
- 在 `RetroShaderPipeline::init()` 中检测文件扩展名
- 若为 `.glsl`：创建单通道描述符（`scale_type = viewport, scale = 1.0, filterLinear = true`）
- 若为 `.glslp`：走原有 GLSLPParser 解析路径

### 修复三：视口尺寸四舍五入
- `game_view.cpp` 中改用 `std::lround()` 转换浮点视口尺寸，确保结果稳定

## 影响范围

| 文件 | 修改类型 |
|------|---------|
| `src/Game/game_view.cpp` | 修复浮点截断问题 |
| `src/Video/renderer/RetroShaderPipeline.cpp` | 新增 .glsl 单文件支持 |
| `src/UI/Utils/GameMenu.cpp` | 着色器选择 UI 添加 glsl 过滤 |
| `src/UI/Pages/SettingPage.cpp` | 着色器选择 UI 添加 glsl 过滤 |
