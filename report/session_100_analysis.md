# Session 100 分析文档

## 任务目标

根据 Issue 要求：
1. 在 SettingPage 的画面设置中添加着色器开关
2. 在 GameMenu 中添加着色器开关（即时生效）和着色器路径选择（即时生效）
3. 给 #101 中新增的渲染管线代码添加 log，目前使用着色器渲染游戏画面是黑的

## 代码结构分析

### 相关文件

| 文件 | 用途 |
|------|------|
| `include/Video/RenderChain.hpp` | 渲染链接口，封装 RetroShaderPipeline |
| `src/Video/RenderChain.cpp` | 渲染链实现 |
| `include/Video/renderer/RetroShaderPipeline.hpp` | 多通道着色器管线接口 |
| `src/Video/renderer/RetroShaderPipeline.cpp` | 着色器管线实现 |
| `src/UI/Pages/SettingPage.cpp` | 设置页面，包含 `buildDisplayTab()` |
| `include/UI/Utils/GameMenu.hpp` | 游戏内菜单接口 |
| `src/UI/Utils/GameMenu.cpp` | 游戏内菜单实现 |
| `src/Game/game_view.cpp` | 游戏视图，包含着色器初始化和渲染循环 |
| `include/common.hpp` | 全局配置键常量 |

### 配置设计

原有设计：`display.shader` 存储着色器路径（空=直通）

新增设计：
- `display.shaderEnabled` (bool)：着色器开关
- `display.shader`（字符串，`KEY_DISPLAY_SHADER_PATH`）：着色器 .glslp 预设路径

## 可能的挑战与解决方案

### 黑屏问题分析

着色器渲染黑屏的潜在原因：

1. **GL 状态未正确保存/恢复**
   - 原代码只保存了 FBO、视口、程序，未保存 VAO 和纹理绑定
   - 解决：在 `process()` 中新增 `GL_VERTEX_ARRAY_BINDING` 和 `GL_TEXTURE_BINDING_2D` 的保存/恢复

2. **着色器编译或 FBO 问题**
   - 缺乏足够日志难以排查
   - 解决：添加每通道详细日志和 GL 错误检查

3. **NVG 图像句柄问题**
   - 着色器输出为 FBO 附件纹理，NVG 包装后可能有问题
   - 解决：通过设置 `m_nvgImageSrc = 0` 强制 draw() 重建图像句柄

### 即时生效设计

GameMenu 回调 → GameView.setGameMenu() → `m_renderChain.setShader()` → 立即重建管线

## 实现方案

### 1. SettingPage 添加着色器控件

在 `buildDisplayTab()` 末尾添加：
- `BooleanCell`（`KEY_DISPLAY_SHADER_ENABLED`）
- `DetailCell`（`.glslp` 路径选择，`KEY_DISPLAY_SHADER_PATH`）

### 2. GameMenu 实现着色器控件

替换原有占位符代码：
- `BooleanCell`：从配置读取初始值，触发 `m_shaderEnabledChangedCallback`
- `DetailCell`（m_shaderPathCell）：FileListPage 过滤 `.glslp`，触发 `m_shaderPathChangedCallback`

### 3. GameView 注册回调

```cpp
m_gameMenu->setShaderEnabledChangedCallback([this](bool enabled) {
    std::string path = cfgGetStr(KEY_DISPLAY_SHADER_PATH, "");
    m_renderChain.setShader((enabled && !path.empty()) ? path : "");
    m_nvgImageSrc = 0; // 强制重建 NVG 图像
});
m_gameMenu->setShaderPathChangedCallback([this](const std::string& newPath) {
    bool enabled = cfgGetBool(KEY_DISPLAY_SHADER_ENABLED, false);
    m_renderChain.setShader((enabled && !newPath.empty()) ? newPath : "");
    m_nvgImageSrc = 0;
});
```

### 4. 渲染管线日志

- 添加通道级 debug 日志（输入/输出纹理 ID、尺寸）
- 添加 `#ifndef NDEBUG` 保护的 `glGetError()` 检查
- 保存/恢复更完整的 GL 状态（VAO、纹理绑定）

### 5. 工具函数提取

新增 `beiklive::string::extractDirPath()`，替换各处重复的路径目录提取逻辑。
