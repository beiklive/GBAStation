# RetroArch 渲染链重构设计报告

## 1. 目标

本次工作没有推翻项目里现有的 `ShaderChain` / `GlslpLoader` 架构，而是在已有 RetroArch
GLSL 兼容实现基础上，补齐当前渲染链里最关键的缺口，使其更接近文档目标中的最小可运行
 RetroArch shader runtime：

- 继续支持 `.glslp` / `.glsl`
- 继续支持 `#pragma parameter`
- 继续支持 multi-pass / LUT / per-pass scale
- 新增对 `PassPrevNTexture` 的真实绑定
- 新增对 `PrevTexture` / `Prev1..Prev6Texture` 的真实帧历史缓存
- 为这些新增纹理补齐 `TextureSize` / `InputSize` 运行时 uniform

相关代码位置：

- `include/Game/ShaderChain.hpp`
- `src/Game/ShaderChain.cpp`
- `include/Game/GlslpLoader.hpp`
- `src/Game/GlslpLoader.cpp`
- `src/UI/game_view.cpp`

---

## 2. 当前项目中的渲染链结构

`GameView` 在初始化时总会先创建一条基础 shader chain：

1. 创建核心视频纹理 `m_texture`
2. 调用 `ShaderChain::initBuiltin()`
3. 如果配置了 `.glsl` / `.glslp`，再由 `GlslpLoader` 把用户 pass 追加到 chain 中
4. 每帧在 `GameView::draw()` 中调用 `ShaderChain::run()`
5. 将最终纹理包装成 NanoVG image 再显示到界面

运行时链路如下：

```text
libretro frame -> m_texture
               -> ShaderChain pass0 (builtin)
               -> ShaderChain pass1..N
               -> final output texture
               -> NanoVG image
               -> screen
```

其中：

- `pass0` 是内置 pass，用于把源纹理规范化输出到 FBO
- `pass1..N` 是从 `.glsl` / `.glslp` 动态加载的 RetroArch pass
- 每个 pass 都有独立 `program / fbo / outputTex`

---

## 3. 原有实现已经具备的能力

在本次修改前，仓库已经完成了大量 RetroArch GLSL 兼容工作：

- `GlslpLoader::parseGlslFile()` 已支持：
  - RetroArch 标准 `#if defined(VERTEX)` / `#elif defined(FRAGMENT)`
  - 自定义 `#pragma stage vertex/fragment`
  - `#pragma parameter` 剥离
- `GlslpLoader::loadGlslpIntoChain()` 已支持：
  - `shaders = N`
  - `shader0..N`
  - `filter_linear`
  - `wrap_mode`
  - `scale_type / scale_x / scale_y`
  - `frame_count_mod`
  - `alias`
  - `textures = ...` LUT
  - `#reference`
- `ShaderChain::run()` 已支持：
  - `Texture/Source`
  - `InputSize`
  - `TextureSize/SourceSize`
  - `OutputSize`
  - `FinalViewportSize`
  - `FrameCount`
  - `FrameDirection`
  - `OrigTexture`
  - `OrigInputSize`
  - `PassNTexture`
  - `{alias}Texture`

也就是说，本次并不是从零实现，而是补足“剩余最影响兼容性”的纹理引用能力。

---

## 4. 原有渲染链的缺口

虽然基础功能已经齐全，但仍有两个明显缺口会影响 RetroArch shader 兼容性：

### 4.1 `PassPrevNTexture` 未实现

部分 shader 会使用相对当前 pass 的“更早 pass 输出”，例如：

```glsl
uniform vec2 PassPrev3TextureSize;
uniform vec2 PassPrev3InputSize;
uniform sampler2D PassPrev3Texture;
```

这类模式在 `resources/shaders/shaders_glsl/phosphor-line v2.0/` 中大量存在。

原实现只支持：

- `PassNTexture`
- `{alias}Texture`

但没有支持 `PassPrevNTexture` / `PassPrevNInputSize` / `PassPrevNTextureSize`。

### 4.2 `PrevTexture` 仍然只是回退到当前源纹理

原实现虽然会查找这些 uniform：

- `PrevTexture`
- `Prev1Texture`
- `Prev2Texture`
- ...
- `Prev6Texture`

但运行时并没有帧历史缓存，所有这些 uniform 最终都退化回当前源纹理，这和 RetroArch
实际语义不一致，也让依赖历史帧的 shader 无法正常工作。

---

## 5. 本次代码改动

## 5.1 `ShaderPass` 增加输入尺寸记录

文件：`include/Game/ShaderChain.hpp`

在 `ShaderPass` 中新增：

- `inputW`
- `inputH`

用途：记录“该 pass 最近一次渲染时接收到的输入尺寸”。

这样在后续 pass 引用某个旧 pass 纹理时，可以同时正确提供：

- `xxxTextureSize` -> 被引用 pass 的输出纹理尺寸
- `xxxInputSize` -> 被引用 pass 当时的输入尺寸

这与 RetroArch 多通道 shader 的尺寸语义更一致。

---

## 5.2 扩展 `PassTexBinding`

文件：`include/Game/ShaderChain.hpp`

原来 `PassTexBinding` 只记录：

- 源 pass 索引
- sampler uniform 位置
- 纹理单元偏移

现在扩展为：

- `kind`
  - `PassOutput`
  - `FrameHistory`
- `srcPassIdx`
- `historyIndex`
- `loc`
- `textureSizeLoc`
- `inputSizeLoc`
- `textureSizeIsVec2`
- `inputSizeIsVec2`
- `texUnitOffset`

这让同一套绑定系统可以同时处理：

1. 其他 pass 的输出纹理
2. 历史帧纹理
3. 它们对应的尺寸 uniform

---

## 5.3 新增帧历史纹理池

文件：

- `include/Game/ShaderChain.hpp`
- `src/Game/ShaderChain.cpp`

新增私有结构：

```cpp
struct FrameHistorySlot {
    GLuint tex = 0;
    int    w   = 0;
    int    h   = 0;
};
```

并新增私有成员：

- `std::vector<FrameHistorySlot> m_frameHistory`

以及三个辅助方法：

- `_clearFrameHistory()`
- `_ensureHistoryTexture(...)`
- `_captureFrameHistory(...)`

### 工作方式

每帧所有 pass 完成之后：

1. 取内置 `pass0` 的 FBO 输出
2. 复制到一个历史纹理槽位
3. 把该槽位插入 `m_frameHistory[0]`
4. 旧历史依次后移
5. 最多保留 6 帧

于是：

- `PrevTexture` / `Prev1Texture` -> `m_frameHistory[0]`
- `Prev2Texture` -> `m_frameHistory[1]`
- ...
- `Prev6Texture` -> `m_frameHistory[5]`

如果历史帧尚不存在，则仍安全回退到当前源纹理，避免首帧出现空绑定。

---

## 5.4 `_lookupPassTextures()` 新增三类解析

文件：`src/Game/ShaderChain.cpp`

现在该函数除了已有的：

- `PassNTexture`
- `{alias}Texture`

还会继续查找：

### A. `PassPrevNTexture`

语义：相对于当前 pass，向前回溯第 N 个 pass 的输出。

例如当前 pass 是链中的第 5 个 pass：

- `PassPrev1Texture` -> 前一个 pass
- `PassPrev2Texture` -> 前两个 pass
- `PassPrev3Texture` -> 前三个 pass

并同步查找：

- `PassPrevNTextureSize`
- `PassPrevNInputSize`

### B. `PrevTexture`

绑定为上一帧历史纹理。

### C. `Prev1Texture` .. `Prev6Texture`

绑定为更早的历史帧纹理，并同步查找：

- `PrevNTextureSize`
- `PrevNInputSize`

---

## 5.5 `run()` 中补齐真实运行时绑定

文件：`src/Game/ShaderChain.cpp`

每次 pass 渲染时，除了已有的 `Texture / InputSize / OutputSize / LUT` 绑定外，
现在还会为每个额外纹理引用自动设置：

- sampler 纹理单元
- `xxxTextureSize`
- `xxxInputSize`

其中：

### 引用 pass 输出时

- 纹理 = 被引用 pass 的 `outputTex`
- `TextureSize` = 被引用 pass 的 `outW/outH`
- `InputSize` = 被引用 pass 的 `inputW/inputH`

### 引用历史帧时

- 纹理 = 历史帧缓存纹理
- `TextureSize` = 历史纹理尺寸
- `InputSize` = 历史纹理尺寸

---

## 6. 现在的完整渲染链

修改后，一帧执行流程可以表示为：

```text
当前 libretro 帧 -> srcTex
                 -> pass0 builtin FBO
                 -> pass1
                 -> pass2
                 -> ...
                 -> passN
                 -> finalTex

同时：
pass0.outputTex --copy--> frame history ring
```

而任意一个用户 pass 现在都可以拿到这些输入：

- 当前输入：`Texture`
- 原始输入：`OrigTexture`
- 绝对 pass 引用：`PassNTexture`
- 相对 pass 引用：`PassPrevNTexture`
- 别名引用：`AliasTexture`
- 历史帧引用：`PrevTexture / Prev1..Prev6Texture`
- LUT：`textures = ...`

---

## 7. 与文档目标的对应关系

| 文档目标 | 当前状态 |
|---|---|
| 解析 `.glslp` preset | 已完成 |
| 解析 `.glsl` shader | 已完成 |
| 支持 `#pragma parameter` | 已完成 |
| 支持 multi-pass shader | 已完成 |
| 生成 vertex / fragment shader | 已完成 |
| 构建 shader pipeline | 已完成 |
| FBO pipeline | 已完成 |
| LUT | 已完成 |
| PassTexture | 已完成 |
| feedback / relative pass reference | 本次补齐 `PassPrevNTexture` |
| history textures | 本次补齐 `PrevTexture/Prev1..Prev6Texture` |
| runtime uniforms | 已补齐到新增纹理引用的 `InputSize/TextureSize` |

---

## 8. 验证方式

本仓库没有现成的单元测试基础设施，因此本次主要采用：

1. 代码路径审查
2. 资源 shader 反向核对
3. 桌面 CMake 构建尝试

### 构建现状

当前环境下，桌面 CMake 配置会被系统缺失依赖阻断：

- 默认脚本缺少 `wayland-scanner`
- 关闭 Wayland 后又缺少 X11 头文件/库

因此本次无法在当前沙箱里完成完整桌面链接验证；这属于环境依赖问题，不是本次改动引入的
代码错误。

---

## 9. 后续仍可继续扩展的方向

当前渲染链已经更接近 RetroArch 最小运行时，但若继续追求更高兼容率，建议后续再扩展：

1. `PrevTextureSize` / `PrevInputSize` 之外的更多历史 uniform 别名兼容
2. feedback pass 的更完整 preset 语义（若后续发现具体 preset 仍依赖额外字段）
3. 对更多 RetroArch shader 集进行批量 smoke test
4. 在仓库中补一个轻量的 parser / shader-chain 测试目标，降低后续回归风险

---

## 10. 结论

本次修改保持了项目现有代码结构，仅对 `ShaderChain` 做了局部增强，但实质上把渲染链从
“已经能跑大多数基础 RetroArch shader”推进到了“能够正确处理历史帧与相对 pass 引用”
这一更完整的兼容层级。

对用户最直接的收益是：

- 依赖 `PassPrevNTexture` 的多 pass shader 不再缺输入
- `PrevTexture/PrevNTexture` 不再全部退化成当前帧
- 这些新增纹理引用的尺寸 uniform 也能同步正确工作

这使项目更接近文档中提出的 RetroArch GLSL shader runtime 目标。
