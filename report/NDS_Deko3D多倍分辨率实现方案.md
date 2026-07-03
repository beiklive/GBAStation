# NDS Deko3D 多倍分辨率实现方案

日期：2026-07-03

## 结论

当前仓库没有可直接移植的 Deko3D 多倍分辨率实现。

- `third_party/melonDS` 和 `third_party/melonDS-switch` 的多倍分辨率实现集中在 OpenGL / OpenGL Compute renderer。
- `third_party/ArcDelta_melonDS` 的 Switch Deko renderer 是独立实现，`GPU3D::DekoRenderer::SetRenderSettings()` 目前为空。
- ArcDelta Deko 的 3D compute shader、3D tile buffer、3D final buffer、2D compositor 和最终双屏 framebuffer 都大量固定为 `256x192`。

因此不能只改 Stub 菜单或只设置 `GL_ScaleFactor`。真正可见的多倍分辨率必须重构 Deko 3D 和 Deko 2D 合成链。

## 参考实现

### melonDS OpenGL / Compute

参考文件：

- `third_party/melonDS/src/GPU3D_Compute.cpp`
- `third_party/melonDS/src/GPU3D_Compute.h`
- `third_party/melonDS/src/GPU_OpenGL.cpp`
- `third_party/melonDS-switch/src/GPU3D_OpenGL.cpp`
- `third_party/melonDS-switch/src/GPU_OpenGL.cpp`

关键做法：

- `SetRenderSettings(scale, highResolutionCoordinates)` 里动态设置：

```text
ScreenWidth  = 256 * ScaleFactor
ScreenHeight = 192 * ScaleFactor
TilesPerLine = ScreenWidth / TileSize
TileLines    = ScreenHeight / TileSize
MaxWorkTiles = TilesPerLine * TileLines * 16
MaxYSpanSetups = BaseMaxYSpanSetups * ScaleFactor
```

- 顶点位置按 scale 放大：

```cpp
scaledX = HiresPositionX * ScaleFactor
scaledY = HiresPositionY * ScaleFactor
```

- Final 3D framebuffer 按 `ScreenWidth x ScreenHeight` 分配。
- OpenGL compositor 输出也按 scale 扩大，2D 像素按 scale 复制，3D 层使用高分辨率结果参与合成。

### ArcDelta Deko 当前状态

参考文件：

- `third_party/ArcDelta_melonDS/src/GPU3D_Deko.cpp`
- `third_party/ArcDelta_melonDS/src/GPU3D_Deko.h`
- `third_party/ArcDelta_melonDS/src/GPU2D_Deko.cpp`
- `third_party/ArcDelta_melonDS/src/GPU2D_Deko.h`
- `third_party/ArcDelta_melonDS/src/shaders/GPU3D_Comp.glsl`
- `third_party/ArcDelta_melonDS/src/shaders/ComposeBGOBJ_fsh.glsl`
- `nds_stub/CMakeLists.txt`

当前限制：

```cpp
void DekoRenderer::SetRenderSettings(GPU::RenderSettings& settings)
{

}
```

固定尺寸示例：

```text
GPU3D_Deko.h:
TilesPerLine = 256 / TileSize
TileLines    = 192 / TileSize
FinalTiles::ColorResult[256 * 192 * 2]

GPU3D_Deko.cpp:
dispatchCompute(256 / CoarseTileW, 192 / CoarseTileH)
dispatchCompute(256 / 8, 192 / 8, 1)
dispatchCompute(256 / 32, 192, 1)

GPU3D_Comp.glsl:
FramebufferStride = 256 * 192
TilesPerLine = 256 / TileSize
TileLines = 192 / TileSize
ColorResult[256 * 192 * 2]
imageStore(FinalFB, ivec2(gl_GlobalInvocationID.xy), ...)

GPU2D_Deko.cpp:
FinalFramebuffers: 256 x 192
IntermedFramebuffers: 256 x 192
_3DFramebuffer: 256 x 192
DisplayCaptureMemory: 256 * 192 * 4
DirectBitmapTexture: 256 x 192
```

## 为什么不能只放大 3D framebuffer

NDS Stub 当前显示链：

```text
GPU3D_Deko
    -> GPU2D_Deko::_3DFramebuffer
    -> GPU2D_Deko 合成 BG/OBJ/3D
    -> GPU2D_Deko::FinalFramebuffers[front][screen]
    -> NdsGameLayer drawScreens()
    -> Switch 屏幕
```

如果只把 `_3DFramebuffer` 改成 `512x384`，但 `FinalFramebuffers` 仍是 `256x192`，最终画面仍会被压回 256x192，再由 `NdsGameLayer` 放大显示，肉眼不会得到真正 x2 清晰度。

正确做法是：

```text
3D 内部渲染到 256*scale x 192*scale
2D compositor 输出到 256*scale x 192*scale
2D BG/OBJ 使用 nearest 复制到 scale 像素网格
3D BG0 使用高分辨率 3D framebuffer
NdsGameLayer 使用高分辨率 FinalFramebuffers 绘制到屏幕
```

## 目标行为

菜单档位：

```text
x1 = 256 x 192 per screen
x2 = 512 x 384 per screen
x3 = 768 x 576 per screen
x4 = 1024 x 768 per screen
```

显示策略：

- 2D 图层保持像素复制，不做模糊缩放。
- 3D 图层使用 Deko compute 以高分辨率 rasterize。
- 最终屏幕显示仍由 Stub 的 `NdsGameLayer` 控制布局。
- x2/x3/x4 是性能优先的可选项，x1 仍是稳定默认值。

## 分阶段实施方案

### 阶段 1：抽象 Deko 分辨率状态

目标：

- 不改变行为，只把固定尺寸集中成可配置状态。

实施点：

1. 在 `GPU3D_Deko` 增加成员：

```cpp
int ScaleFactor = 1;
int ScreenWidth = 256;
int ScreenHeight = 192;
int TileSize = 8;
int TilesPerLine = 32;
int TileLines = 24;
int MaxWorkTiles = 32 * 24 * 48;
int MaxYSpanSetupsRuntime = 6144 * 2;
int MaxYSpanIndicesRuntime = 64 * 2048;
```

2. 将 `GPU3D_Deko.h` 中依赖 `256/192` 的静态数组改成动态 allocation：

```cpp
std::vector<SpanSetupY> YSpanSetups;
std::vector<SetupIndices> YSpanIndices;
GpuMemHeap::Allocation TileMemory;
GpuMemHeap::Allocation FinalTileMemory;
GpuMemHeap::Allocation BinResultMemory;
```

3. x1 下所有尺寸保持原值，先构建验证无行为变化。

验收：

- x1 游戏启动、显示、音频、菜单、重置正常。
- 不引入 x2 功能，仅完成尺寸抽象。

风险：

- `sizeof(Tiles)` / `sizeof(FinalTiles)` 原本是编译期结构体大小，改成运行期 size 后需要谨慎对齐。

### 阶段 2：为 Deko compute shader 生成 scale 变体

目标：

- 保持 Deko shader 的 `local_size_x/y` 编译期常量，同时支持 x1/x2/x3/x4。

实施点：

1. 复制或参数化 `GPU3D_Comp.glsl`，增加 CMake configure 变量：

```glsl
#define ScreenWidth @NDS_DEKO_SCREEN_WIDTH@
#define ScreenHeight @NDS_DEKO_SCREEN_HEIGHT@
#define TileSize @NDS_DEKO_TILE_SIZE@
#define TilesPerLine @NDS_DEKO_TILES_PER_LINE@
#define TileLines @NDS_DEKO_TILE_LINES@
#define FramebufferStride (ScreenWidth * ScreenHeight)
```

2. `nds_stub/CMakeLists.txt` 为每个 scale 生成 shader：

```text
Rasterise..._x1.dksh
Rasterise..._x2.dksh
Rasterise..._x3.dksh
Rasterise..._x4.dksh
DepthBlend..._x1.dksh
DepthBlend..._x2.dksh
DepthBlend..._x3.dksh
DepthBlend..._x4.dksh
FinalPass..._x1.dksh
FinalPass..._x2.dksh
FinalPass..._x3.dksh
FinalPass..._x4.dksh
```

3. `GPU3D_Deko::SetRenderSettings()` 根据 scale 绑定对应 shader set。

验收：

- x1 使用 x1 shader，行为不变。
- x2/x3/x4 能加载 shader，不开始启用菜单。

风险：

- shader 数量会显著增加，RomFS 体积会上涨。
- `uam` 不支持某些宏展开时，需要改为生成临时 `.glsl`。

### 阶段 3：实现 GPU3D_Deko 高分辨率 rasterize

目标：

- 3D renderer 真正输出 `ScreenWidth x ScreenHeight` 的 `_3DFramebuffer`。

实施点：

1. 参考 `melonDS/src/GPU3D_Compute.cpp` 的 `SetRenderSettings()`：

```cpp
ScaleFactor = settings.GL_ScaleFactor;
ScreenWidth = 256 * ScaleFactor;
ScreenHeight = 192 * ScaleFactor;
TilesPerLine = ScreenWidth / TileSize;
TileLines = ScreenHeight / TileSize;
MaxWorkTiles = TilesPerLine * TileLines * 48;
MaxYSpanSetupsRuntime = BaseMaxYSpanSetups * ScaleFactor;
MaxYSpanIndicesRuntime = 64 * 2048 * ScaleFactor;
```

2. `RenderFrame()` 中 polygon positions 改为 scale 后坐标：

```cpp
if (settings.GL_BetterPolygons)
    x = (HiresPositionX * ScaleFactor) >> 4;
else
    x = FinalPositionX * ScaleFactor;
```

3. dispatch 改为 runtime 尺寸：

```cpp
dispatchCompute(TilesPerLine * TileLines / clearGroupSize, 1, 1);
dispatchCompute((RenderNumPolygons + 31) / 32, ScreenWidth / CoarseTileW, ScreenHeight / CoarseTileH);
dispatchCompute(ScreenWidth / TileSize, ScreenHeight / TileSize, 1);
dispatchCompute(ScreenWidth / 32, ScreenHeight, 1);
```

4. Final pass 写入 high-res 3D image。

验收：

- x2 下纯 3D 场景能看到高分辨率 3D 输出。
- x1 性能和画面不退化。

风险：

- `MaxWorkTiles * TileSize * TileSize` 在 x4 下显存/内存占用明显上涨。
- Switch 上 x4 可能不可用或性能很差，应允许自动降级。

### 阶段 4：改造 GPU2D_Deko compositor 支持高分辨率 final framebuffer

目标：

- 最终每屏输出为 `256*scale x 192*scale`，让高分辨率 3D 能真正显示出来。

实施点：

1. `GPU2D_Deko` 增加：

```cpp
int ScaleFactor = 1;
int OutputWidth = 256;
int OutputHeight = 192;
```

2. `FinalFramebuffers` 改为 `OutputWidth x OutputHeight`。
3. `_3DFramebuffer` 改为 `OutputWidth x OutputHeight` 或单独维护 high-res 3D image。
4. `ComposeBGOBJ_fsh.glsl` 增加 scale-aware 坐标：

```glsl
ivec2 outPos = ivec2(gl_FragCoord.xy);
ivec2 nativePos = outPos / ScaleFactor;
ivec2 high3DPos = outPos;
```

5. 2D BG/OBJ、window、palette、mosaic 仍按 native 256x192 取样，输出像素按 scale 复制。
6. 3D BG0 从 high-res 3D framebuffer 按 `high3DPos` 取样。

验收：

- x2 下 2D UI/文字仍像素锐利，不被线性模糊。
- x2 下 3D 模型边缘比 x1 更细。
- 游戏中 2D/3D 混合场景不出现错位。

风险：

- NDS capture/display FIFO 仍是原生 256x192，capture 场景可能需要专门降采样或按原生路径处理。
- window/mosaic/effects 需要逐项验证。

### 阶段 5：Stub 显示层接收高分辨率 framebuffer

目标：

- `NdsGameLayer` 使用当前 scale 对应的 texture 尺寸和 UV。

实施点：

1. `GPU2D::DekoRenderer` 暴露：

```cpp
int GetOutputWidth() const;
int GetOutputHeight() const;
int GetScaleFactor() const;
```

2. `NdsGameLayer` 在分辨率切换后重建 external textures：

```text
删除旧 framebuffer texture
用 OutputWidth/OutputHeight 创建新 external texture
drawScreens() UV 改为 OutputWidth/OutputHeight
```

3. 菜单切换分辨率流程：

```text
PresentQueue.waitIdle()
EmuQueue.waitIdle()
GPU::SetRenderSettings()
gameLayer.recreateFramebufferTextures()
```

验收：

- 菜单 x1/x2/x3/x4 切换不崩溃。
- 回到游戏后画面尺寸正确、上下屏正常。

风险：

- 切换时 GPU 队列和 external texture 生命周期必须严格同步，否则容易出现旧纹理句柄访问。

### 阶段 6：性能保护和降级策略

目标：

- 防止 x3/x4 在复杂游戏中拖垮帧率或显存。

实施点：

1. 先只开放 x1/x2 实测。
2. x3/x4 菜单保留但标记实验，必要时运行时拒绝：

```text
if allocation fails -> fallback x1
if frame time > threshold for N frames -> suggest fallback
```

3. 添加日志：

```text
Deko hires request scale=x2
Deko hires alloc 3D bytes=...
Deko hires alloc 2D bytes=...
Deko hires applied scale=x2
Deko hires fallback scale=x1 reason=...
```

验收：

- 宝可梦黑2 x1 仍稳定。
- x2 在小场景可运行，不崩溃。
- x3/x4 即使性能差，也不能导致崩溃。

## 推荐执行顺序

优先顺序：

```text
1. 完成阶段 1：尺寸抽象，保持 x1 不变
2. 完成阶段 2：生成 scale shader 变体
3. 完成阶段 3：只让 3D high-res 跑起来，临时 debug 输出
4. 完成阶段 4：2D compositor high-res final output
5. 完成阶段 5：Stub texture 重建和菜单切换
6. 完成阶段 6：性能保护
```

不建议直接一步到位，因为当前 Deko 路径同时涉及：

- GPU3D compute shader
- GPU3D GPU buffer layout
- GPU2D compositor shader
- GPU2D final framebuffer
- Stub external texture 生命周期
- NDS display capture

任何一个环节仍固定 256x192，最终都会表现为画面不变、错位、黑屏或崩溃。

## 第一阶段实施清单

下一步可以先开始阶段 1，目标是“可回滚、x1 无行为变化”：

1. 在 `GPU3D_Deko.h` 引入 runtime 尺寸字段。
2. 将 `MaxWorkTiles`、`MaxYSpanIndices`、`MaxYSpanSetups` 的静态依赖拆出来。
3. 将 `YSpanIndices/YSpanSetups/RenderPolygons` 中可变尺寸部分改为 `std::vector`。
4. 新增 `configureScale(int scale)`，但暂时只允许 scale=1。
5. 构建并实机验证 x1 完全不变。

完成第一阶段后，再进入 shader scale variants。这样风险最低，也最方便定位回归。
