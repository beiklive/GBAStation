# DraStic shader 目录分析与移植记录

来源目录：`build_switch/shaders`

## 目录概况

- `.dfx`：129 个。DraStic 的滤镜预设文件，定义 pass 链、输入纹理、输出纹理和 sampler。
- `.dsd`：108 个。单个 pass 的 vertex/fragment GLSL 源码。
- `.raw`：8 个。SMAA 等滤镜使用的查找表纹理。
- `.h/.hlsl`：4 个。FXAA/SMAA 相关 include。
- `_shader_format_.txt`：DraStic shader 格式说明。

当前 NDS stub 的滤镜链路是：DS framebuffer 纹理 -> 一个 Deko fragment shader -> 屏幕矩形。也就是说，当前系统天然支持“单 pass、只采样 framebuffer、不依赖外部 raw 纹理”的滤镜；多 pass/FBO/额外查找表纹理需要扩展渲染管线。

## 已移植到当前系统

这些滤镜已加入 `resources/config/nds_shaders.json`，菜单中可选择，并由 `NdsDrasticSimple_fsh.glsl` 统一实现：

- `drastic-linear`
- `drastic-grayscale`
- `drastic-nds-color`
- `drastic-natural-vision`
- `drastic-nds-color-natural-vision`
- `drastic-lcd1x`
- `drastic-lcd1x-nds-color`
- `drastic-lcd1x-natural-vision`
- `drastic-lcd1x-nds-color-natural-vision`
- `drastic-zfast`
- `drastic-zfast-lcd`
- `drastic-zfast-lcd-brightness`
- `drastic-zfast-lcd-nds-color`
- `drastic-zfast-lcd-natural-vision`
- `drastic-zfast-lcd-nds-color-natural-vision`
- `drastic-quilez`
- `drastic-scanlinesd`
- `drastic-scanlinesd-color`
- `drastic-scanlinesd-x`
- `drastic-scanlinesd-color-x`
- `drastic-dot-d4`
- `drastic-dot-hv4`

接入方式：

- `NdsShaderCatalog` 统一维护 `drastic-*` 名称到 shader code 的映射。
- `NdsGameLayer` 将这些名称映射到同一个 `Gfx::shaderMode_NdsDrasticSimple`。
- `NdsDekoRuntime` 将 shader code 写入 `param1.w`，fragment shader 根据 code 执行对应分支。
- UI 标签函数为这些长名称提供较短显示名。

这样不会为每个 DraStic 滤镜新增一个 Deko shader mode，也不会显著增加启动加载成本。

## 暂未放入可选列表的文件

以下类别已经分析，但没有作为“可选滤镜”加入列表，原因是当前渲染链路无法完整表达或源文件异常。

### 多 pass/FBO 链路

这类 `.dfx` 有 2 个或更多 `<pass>`，需要中间 render target。当前系统没有 NDS 滤镜专用 FBO pass 链。

典型文件：

- `2XBR_v3.7C_1x.dfx`
- `2XBR_v3.7C_2x.dfx`
- `5XBR_v3.7A_1x.dfx`
- `5XBR_v3.7A_2x.dfx`
- `5XBR_v3.7C_1x.dfx`
- `5XBR_v3.7C_2x.dfx`
- `HQ2X.dfx`
- `Linear2X.dfx`
- `Scale2X.dfx`
- `SABR_v3.0_1x.dfx`
- `SABR_v3.0_Optimized_1x.dfx`
- `SABR_v3.0_Optimized_2x.dfx`
- `FXAA HQ.dfx`

### 外部 raw 纹理/include 链路

- `SMAA.dfx` 使用 4 pass，并依赖 `AreaTex*.raw`、`SearchTex*.raw`、`SMAA.hlsl`。
- 当前 `Gfx::DrawRectangle` 只绑定 framebuffer 一张纹理，不能给滤镜 pass 绑定额外查找表。

### 重型 XBR/SABR/CRT 单 pass

部分 XBR/SABR/CRT 预设虽然是单 pass，但源码依赖大量 vertex 侧预计算 varying，直接照搬会要求生成对应 wrapper，且 shader 体积和寄存器压力较高。之前已有 `xbrz-freescale` 作为当前系统内的轻量实现，因此这批没有批量放入列表。

### 源文件异常

- `sharp_bilinear.dsd` 缺失标准 `<vertex>/<fragment>` 标签，并且源码里的 `*`、`/` 等运算符出现丢失，不能直接编译。
- `sharp_bilinear+nds_color.dsd`、`sharp_bilinear+natural_vision.dsd`、`sharp_bilinear+nds_color+natural_vision.dsd` 格式正常，可后续手工移植为单 pass。

## 后续如需完整移植

需要先扩展 NDS 滤镜渲染链：

1. 增加滤镜 pass 描述结构，解析或手写 `.dfx` pass 链。
2. 在 `Gfx` 中支持中间 FBO 纹理创建、复用与回收。
3. 支持一个 pass 绑定多个 sampler，包括 framebuffer、上一 pass 输出、raw 查找表。
4. 支持 per-pass shader、sampler filter、output scale。
5. 再把 XBR/SABR/SMAA/FXAA HQ 这类滤镜逐个加入列表。

在这些能力完成前，不建议把未实现的 `.dfx` 名称放入菜单，否则用户会以为可用，实际可能 fallback、编译失败或启动崩溃。
