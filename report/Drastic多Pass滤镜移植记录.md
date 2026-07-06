# DraStic 多 pass 滤镜移植记录

## 本次实现

- 在 `Gfx` draw-call 队列中新增 `NdsMultiPass` 绘制类型。
- 新增 `TextureCreateRenderTarget()`，专门创建可作为中间 render target 的 RGBA8 纹理。
- NDS 层维护两张中间纹理，尺寸变化时重建，稳定时复用。
- 多 pass 执行流程：
  1. 原 DS framebuffer 作为输入。
  2. 第一个 pass 输出到临时纹理 A。
  3. 后续中间 pass 在临时纹理 A/B 之间 ping-pong。
  4. 最终 pass 切回主 framebuffer，绘制到当前 NDS 屏幕矩形。
- 每个 pass 支持独立 shader、独立 sampler、独立 DraStic simple code。

## 本次重新接入的滤镜列表

`resources/config/nds_shaders.json` 已加入 `build_switch/shaders` 下 129 个 `.dfx` 文件对应的规范化 id。

规范化规则：

- 小写。
- 非字母数字转换为 `-`。
- 前缀统一为 `drastic-`。

例如：

- `FXAA HQ.dfx` -> `drastic-fxaa-hq`
- `5XBR_v3.7A + LCD3x_2x.dfx` -> `drastic-5xbr-v3-7a-lcd3x-2x`
- `CRT-Geom-no-Curvature.dfx` -> `drastic-crt-geom-no-curvature`

## 当前映射策略

因为当前还没有完整解析 DraStic `.dfx/.dsd` 的运行时系统，129 个 id 不是逐行编译原 DraStic GLSL，而是映射到当前 Switch 可运行 shader 组合：

- `dot/dot-clear/xbrz-freescale/lcd-grid-v2-nds-color`：保留已有原生移植。
- `linear/grayscale/nds-color/natural/lcd/zfast/quilez/scanline/dot-d4/dot-hv4`：走 `NdsDrasticSimple_fsh.glsl`。
- `xbr/sabr/hq/scale2x`：走当前 `NdsXbrzFreescale_fsh.glsl` 作为锐化/边缘平滑近似。
- 带 `lcd3x` 的 XBR/SABR 预设：`xbrz -> lcd` 两 pass。
- 带 `crt/scanline` 的 XBR/SABR 预设：`xbrz -> scanline` 两 pass。
- `1x/2x/linear2x/fxaa-hq/smaa` 这类链式预设：使用两 pass 链，最终 pass 做线性/平滑输出。

## 仍未做的等价项

以下能力还没有实现，所以部分滤镜是“可选且可运行的近似效果”，不是 DraStic 原始效果逐行等价：

- 运行时解析 `.dfx` pass 描述。
- 编译并加载每个 `.dsd` 作为独立 Deko shader。
- 外部 raw 查找表纹理绑定，例如 SMAA 的 `AreaTex`/`SearchTex`。
- 多 sampler pass。
- 每个原始 preset 的宏参数和 output scale 完整还原。

后续如果要做到完全等价，下一步应该是增加一个 build-time 转换器：读取 `.dfx`，转换 `.dsd` 到 Deko GLSL，生成 pass manifest，再由 NDS stub 读取 manifest 执行。
