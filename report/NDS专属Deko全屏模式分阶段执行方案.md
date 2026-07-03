# NDS专属Deko全屏模式分阶段执行方案

## 目标

为 Switch 平台的 NDS 游戏新增一条独立运行路径，不再复用现有 `GamePage`、`GameView`、`GameMenuView` 的 OpenGL 渲染链。新的 NDS 专属页面负责验证并逐步接入 Deko3D，使 NDS 游戏可以在进入游戏后由 Deko3D 接管全屏画面，退出或暂停后回到现有 Borealis/OpenGL UI。

核心目标按优先级排序：

1. 先验证 Deko3D 与现有 Borealis/OpenGL 能否可靠切换。
2. NDS 专属页面和菜单与现有普通游戏页面隔离，不破坏 libretro、mGBA、现有音频和菜单逻辑。
3. 保持现有菜单触发逻辑、按键逻辑、存档/读档/退出的业务语义。
4. 在切换可行后，再逐步移植 ArcDelta_melonDS 的 Deko GPU2D/GPU3D 后端。

## 当前架构约束

现有主程序在 `src/main.cpp` 中初始化 Borealis：

- `brls::Application::init()`
- `brls::Application::createWindow(...)`
- 主循环 `while (brls::Application::mainLoop())`

普通游戏页由 `GamePage` 创建：

- `GameView`：负责模拟线程、音频推送、OpenGL 纹理上传、游戏画面绘制。
- `GameMenuView`：负责菜单 UI、保存/读取/退出/画面设置等回调。
- `RewindSelectorView`：普通核心倒带 UI。

NDS 当前也被塞进同一套 OpenGL GameView 管线：

```text
melonDS RunFrame
  -> MelonDSVideo::Capture CPU framebuffer
  -> GameView pendingFrame
  -> GameRenderer::uploadFrame glTexSubImage2D
  -> OpenGL/NanoVG/Borealis 显示
```

ArcDelta_melonDS 的 Switch 路径不同：

```text
melonDS GPU2D_Deko / GPU3D_Deko
  -> Deko dk::Image framebuffer
  -> TextureCreateExternal
  -> Deko queue present
```

因此本方案不是给现有 `GameView` 增加一个 renderer，而是新增 NDS 专属运行时。

## 总体设计

新增一套 NDS 专属页面：

```text
NdsDekoGamePage
  ├─ NdsDekoGameView
  └─ NdsDekoGameMenuView
```

职责划分：

- `NdsDekoGamePage`
  - 作为 Borealis 页面壳，负责启动/停止 NDS Deko 会话。
  - 负责菜单和游戏视图之间的业务回调绑定。
  - 负责退出游戏时的自动存档、清理和返回上级页面。

- `NdsDekoGameView`
  - 不使用 `GameRenderer`、`RenderChain`、`GameTexture`。
  - 不参与普通 OpenGL 游戏画面上传。
  - 第一阶段只负责发起 Deko 切换验证。
  - 后续阶段负责启动 NDS Deko 主循环、输入轮询、音频推送、画面显示。

- `NdsDekoGameMenuView`
  - 保留普通 `GameMenuView` 的菜单触发语义和按键行为。
  - UI 可先复用 Borealis 控件，但不复用普通 GameView 的 OpenGL 画面路径。
  - 后续若全屏 Deko 运行时无法叠加 Borealis 菜单，则改为 Deko 原生菜单。

新增底层模块：

```text
src/emulator/melonds/deko/
  NdsDekoSession.hpp/.cpp
  NdsDekoRuntime.hpp/.cpp
  NdsDekoVideo.hpp/.cpp
  NdsDekoInput.hpp/.cpp
  NdsDekoAudio.hpp/.cpp
  NdsDekoSwitchGfx.hpp/.cpp
```

第一阶段只需要 `NdsDekoSession` 和最小 `NdsDekoSwitchGfx`。

## 页面路由策略

在创建游戏页时分流：

```text
if platform == NDS && __SWITCH__ && nds.dekoMode.enabled
    create NdsDekoGamePage
else
    create existing GamePage
```

建议新增配置项：

```ini
nds.dekoMode.enabled = false
nds.dekoMode.debugOverlay = true
nds.dekoMode.fallbackToOpenGL = true
```

默认先关闭，便于测试版手动开启。确认稳定后再默认开启。

## 阶段0：准备和隔离

目标：建立最小代码边界，不改变现有 GamePage/GameView/GameMenuView 行为。

任务：

1. 新增 NDS 专属页面类空壳：
   - `src/ui/page/NdsDekoGamePage.hpp/.cpp`
   - `src/ui/view/NdsDekoGameView.hpp/.cpp`
   - `src/ui/view/NdsDekoGameMenuView.hpp/.cpp`

2. 新增构建开关：
   - `BEIKLIVE_NDS_DEKO_EXPERIMENTAL`
   - 只在 `__SWITCH__` 下启用。
   - 非 Switch 编译时这些类退化为空实现或不参与构建。

3. 在游戏启动入口分流，但默认仍走旧路径：
   - `nds.dekoMode.enabled = false`
   - 保证普通 GB/GBA/NES/SFC/libretro/mGBA 不受影响。

验收标准：

- Switch 编译通过。
- 默认配置下所有游戏仍走现有页面。
- 开启配置后能进入新的 NDS 页面空壳，并能返回 UI。

风险：

- 页面分流位置选错可能影响普通游戏启动。

回退：

- 关闭 `nds.dekoMode.enabled` 即可回到旧路径。

## 阶段1：Deko3D切换可行性验证

目标：只验证 OpenGL/Borealis 与 Deko3D 的窗口接管是否可行，不接 melonDS。

这是整个方案的生死线。若此阶段不稳定，不应继续移植 GPU2D/GPU3D。

### 方案1A：软切换验证

流程：

```text
Borealis UI
  -> 进入 NdsDekoGamePage
  -> 暂停 Borealis 游戏视图交互
  -> NdsDekoSession::runProbe()
  -> 初始化 Deko device/queue/swapchain
  -> 全屏绘制纯色/测试图形 3 秒
  -> 销毁 Deko
  -> 返回 Borealis UI
```

任务：

1. 从 ArcDelta 抽取最小 Deko 初始化代码：
   - `nwindowGetDefault()`
   - `dk::Device`
   - `PresentQueue`
   - `Swapchain`
   - 一个 RGBA8 framebuffer
   - 清屏并 present

2. 不初始化 melonDS，不初始化 Deko GPU2D/GPU3D。

3. 加入日志：

```text
NdsDekoSession: probe begin
NdsDekoSession: deko init ok
NdsDekoSession: present frame N
NdsDekoSession: deko deinit ok
NdsDekoSession: returned to borealis
```

4. 测试返回 Borealis 后：
   - UI 不黑屏。
   - 输入可用。
   - 图片和字体仍显示。
   - 再次进入 Deko probe 不崩溃。

验收标准：

- 连续进入/退出 Deko probe 10 次不崩。
- Home 键挂起/恢复后不崩。
- 掌机/底座模式切换后不崩。
- 返回后普通 GBA/mGBA 游戏还能启动并正常渲染。

### 方案1B：硬切换验证

如果软切换失败，需要测试更彻底的生命周期：

```text
停止 Borealis mainLoop 中的游戏页面
  -> 主动释放或重建 Borealis OpenGL/EGL 资源
  -> Deko 接管 nwindow
  -> Deko 退出
  -> 重建 Borealis window/render context
```

风险很高，因为 Borealis/NanoVG 的字体、图片、纹理资源可能没有运行时重建接口。

验收标准：

- 能完整释放并重建 Borealis UI。
- 所有 UI 纹理资源恢复。
- 不引入全局状态污染。

判断：

- 若 1A 成功，优先走 1A。
- 若 1A 失败、1B 成本过高，则应停止 Deko 全屏方案，改评估独立 NRO 或完全迁移 Switch UI 到 Deko。

## 阶段2：NDS专属页面业务闭环

目标：在不接 Deko GPU 后端前，先把 NDS 专属页面的业务逻辑跑通。

任务：

1. `NdsDekoGamePage` 复制当前 `GamePage` 的业务骨架，但不复用 `GameView`。

2. 保持菜单触发语义：
   - 快捷键打开菜单。
   - 菜单打开时暂停游戏会话。
   - B 键返回游戏。
   - 退出菜单触发自动存档和清理。

3. 保持按键逻辑：
   - 继续使用 `GameInputManager` 的映射结果。
   - 或抽出 `GameInputRouter`，让普通 GameView 和 NDS Deko 共用按键解释层。

4. 先实现伪运行循环：
   - Deko probe 画面作为游戏画面。
   - 菜单可打开、关闭、退出。
   - 自动存档先只记录日志，不实际保存。

验收标准：

- NDS 专属页面可以进入、打开菜单、返回、退出。
- 菜单快捷键、确认、返回键逻辑与普通游戏一致。
- 不依赖普通 `GameView` 的成员函数。

需要避免：

- 不要在 `NdsDekoGameMenuView` 中直接调用 `GameView::_on...`。
- 菜单回调应改为 NDS 专属接口，如：

```cpp
class INdsDekoGameActions {
public:
    virtual void resume() = 0;
    virtual void reset() = 0;
    virtual void requestExit() = 0;
    virtual void saveState(int slot) = 0;
    virtual void loadState(int slot) = 0;
    virtual void applyCheats(const std::vector<CheatEntry>& cheats) = 0;
};
```

## 阶段3：最小NDS Deko运行时

目标：NDS Deko 页面能加载 ROM，跑 melonDS CPU/SPU，但画面仍可先用 CPU framebuffer 或调试色块。

任务：

1. 新增 `NdsDekoRuntime`：
   - 管理 melonDS NDS 实例。
   - 加载 BIOS/ROM/SRAM。
   - RunFrame。
   - SaveState/LoadState。
   - ApplyCheats。

2. 暂时不启用 Deko GPU2D/GPU3D。

3. 音频先接现有 `AudioManager`：
   - 避免直接复用 ArcDelta `audren`，因为本项目已有 `audout` 共享流和 `BKAudioPlayer`。
   - NDS Deko Runtime 只负责 `DrainAudio -> AudioManager::pushSamples`。

4. 输入接现有映射：
   - `NdsDekoInput` 将 `GameInputManager` 的状态转换为 melonDS 按键。
   - 触摸坐标独立适配 Deko 全屏布局。

验收标准：

- 能加载 NDS ROM。
- 能运行帧并输出音频。
- 暂停/恢复不破音。
- 退出时 SRAM 保存正常。
- 不显示真实游戏画面也可以，先以日志和音频验证运行时。

## 阶段4：移植ArcDelta Deko显示前端

目标：引入 Deko 的 framebuffer/纹理/present 机制，但仍不启用完整 GPU2D/GPU3D。

任务：

1. 抽取 ArcDelta 前端 Switch Gfx 的最小子集：
   - `Device`
   - `PresentQueue`
   - `EmuQueue`
   - `TextureHeap`
   - `DataHeap`
   - `TextureCreateExternal`
   - `DrawRectangle`
   - `StartFrame/EndFrame`
   - fence wait/signal

2. 改名并隔离命名空间：

```cpp
namespace beiklive::nds_deko::gfx
```

3. 避免全局符号冲突：
   - 不直接把 ArcDelta `Gfx` 原名编进主工程。
   - 不复用 ArcDelta frontend `main.cpp`。

4. 先显示一个 CPU 生成的 256x384 测试图或 melonDS CPU framebuffer。

验收标准：

- Deko 全屏能显示两块 NDS 屏幕。
- 能做 vertical/horizontal/top/bottom 基础布局。
- 能打开/关闭菜单后恢复画面。

## 阶段5：移植GPU2D_Deko

目标：把 NDS 2D 合成从 CPU framebuffer 转到 Deko。

任务：

1. 从 ArcDelta 移植：
   - `GPU2D_Deko.h/.cpp`
   - 依赖的 shader 资源。
   - 相关 `UploadBuffer`、heap、descriptor 管理。

2. 对齐当前 melonDS 版本 API：
   - 当前项目使用 `third_party/melonDS`，ArcDelta 是另一个 fork。
   - 需要逐个适配 `GPU2D::Renderer2D` 接口、`Framebuffer` 存储、dirty VRAM API。

3. 只启用 x1。

4. 保留 CPU renderer fallback：

```text
GPU2D_Deko init failed -> fallback to current software NDS path
```

验收标准：

- 2D 小游戏画面正确。
- Pokemon Black 2 菜单/2D 场景无明显颜色错误。
- 退出/重进不泄露 Deko resource。

## 阶段6：移植GPU3D_Deko

目标：让宝可梦黑2这类复杂3D游戏摆脱 software 3D 瓶颈。

任务：

1. 从 ArcDelta 移植：
   - `GPU3D_Deko.h/.cpp`
   - Deko compute shaders。
   - texture cache、polygon/binning/final pass 相关结构。

2. 与 GPU2D_Deko 共享最终 3D framebuffer。

3. 按 ArcDelta 模型使用：

```text
GPU3D_Deko compute final pass
  -> GPU2D_Deko ComposeBGOBJ
  -> FinalFramebuffers
  -> Deko present
```

4. 先禁用高倍分辨率。

验收标准：

- Pokemon Black 2 开场和复杂场景可进入。
- x1 颜色正确。
- 无首帧卡死。
- 无 UI 纹理污染，因为此路径不使用 Borealis/OpenGL 画游戏画面。
- 帧率相较 software path 明显提升。

## 阶段7：NDS专属菜单Deko化

目标：若 Borealis 菜单无法在 Deko 接管期间叠加，则实现 Deko 原生菜单。

两种模式：

### 模式A：暂停Deko，返回Borealis菜单

流程：

```text
按菜单键
  -> pause NDS runtime
  -> destroy/suspend Deko present
  -> show NdsDekoGameMenuView in Borealis
  -> resume
  -> re-enter Deko
```

优点：

- 可继续使用 Borealis 菜单控件。
- 复用当前菜单样式和大量逻辑。

缺点：

- 每次菜单要切换图形上下文。
- 切换延迟可能明显。

### 模式B：Deko原生菜单

流程：

```text
按菜单键
  -> NDS runtime pause
  -> Deko framebuffer 继续 present
  -> Deko 绘制菜单 overlay
```

优点：

- 不切回 OpenGL，稳定性更好。
- 体验接近 ArcDelta。

缺点：

- 需要重写菜单渲染和控件。
- 不能直接复用 Borealis 控件。

推荐：

- 第一版走模式A，因为目标是验证 Deko 可行性。
- 若模式A切换成本太高，再做模式B。

## 阶段8：功能补齐

目标：让 NDS Deko 模式达到可替换当前 NDS GameView 的水平。

任务：

1. 存档：
   - SRAM 自动保存。
   - save state/load state。
   - 缩略图生成。

2. 金手指：
   - 复用 `CheatEntry`。
   - 菜单维护显示状态。
   - runtime 负责应用到 melonDS。

3. 画面布局：
   - vertical。
   - horizontal。
   - top only。
   - bottom only。
   - priority top。
   - custom/hybrid 可后置。

4. 触摸：
   - 根据 Deko 屏幕布局反算 NDS 坐标。
   - 保留虚拟指针模式。

5. 快进：
   - 使用 ArcDelta 模型：

```text
while fastForward && totalFrameLength < frameBudget
    RunFrame
```

   - 音频策略独立设计，避免当前“声音高频/静音/跳帧”问题。

6. 性能统计：
   - frame time histogram。
   - RunFrame ms。
   - GPU queue wait。
   - audio buffer fill。

验收标准：

- NDS 常用菜单功能完整。
- 重启、读档、退出、自动存档稳定。
- 按键逻辑与普通 GameView 一致。

## 阶段9：性能与稳定性验证

测试矩阵：

1. 小型2D游戏：
   - x1 60 FPS。
   - 颜色正确。
   - 音频无爆音。

2. Pokemon Black 2：
   - 开场。
   - 复杂3D场景。
   - 城市/战斗/菜单切换。
   - 存档读档后不黑屏。

3. 生命周期：
   - 连续启动/退出 20 次。
   - 菜单打开/关闭 50 次。
   - Home 挂起/恢复。
   - 掌机/底座模式切换。

4. 回归：
   - mGBA 游戏启动、画面、音频。
   - libretro NES/SFC 游戏启动、画面、音频。
   - 普通 GamePage 菜单。

验收标准：

- NDS Deko 模式崩溃率为 0。
- 普通核心无回归。
- Pokemon Black 2 x1 接近或稳定 60 FPS。

## 主要风险

### 风险1：Borealis/OpenGL无法与Deko可靠切换

影响：

- Deko 全屏模式无法嵌入当前程序。

应对：

- 第一阶段专门验证。
- 若失败，考虑独立 NDS NRO 或整体 Switch UI Deko 化。

### 风险2：ArcDelta melonDS 与当前 melonDS API 差异较大

影响：

- GPU2D_Deko/GPU3D_Deko 移植成本高。

应对：

- 先只抽取前端 Deko Gfx。
- 再逐步对齐 GPU2D。
- 最后移植 GPU3D。

### 风险3：音频后端冲突

影响：

- ArcDelta 使用 audren，本项目使用 audout/BKAudioPlayer/AudioManager。

应对：

- 不移植 ArcDelta 音频后端。
- NDS Deko runtime 继续使用本项目 `AudioManager`。

### 风险4：菜单体验割裂

影响：

- Deko 全屏期间无法直接显示 Borealis 菜单。

应对：

- 初期采用暂停切回 Borealis 菜单。
- 稳定后做 Deko 原生菜单 overlay。

### 风险5：工程污染普通核心

影响：

- libretro/mGBA 被新图形后端影响。

应对：

- 所有 Deko 代码放在 `src/emulator/melonds/deko` 和 `src/ui/nds_deko`。
- CMake 使用独立开关。
- 普通核心不链接或不调用 NDS Deko runtime。

## 建议文件结构

```text
src/ui/page/NdsDekoGamePage.hpp
src/ui/page/NdsDekoGamePage.cpp
src/ui/view/NdsDekoGameView.hpp
src/ui/view/NdsDekoGameView.cpp
src/ui/view/NdsDekoGameMenuView.hpp
src/ui/view/NdsDekoGameMenuView.cpp

src/emulator/melonds/deko/NdsDekoSession.hpp
src/emulator/melonds/deko/NdsDekoSession.cpp
src/emulator/melonds/deko/NdsDekoRuntime.hpp
src/emulator/melonds/deko/NdsDekoRuntime.cpp
src/emulator/melonds/deko/NdsDekoSwitchGfx.hpp
src/emulator/melonds/deko/NdsDekoSwitchGfx.cpp
src/emulator/melonds/deko/NdsDekoInput.hpp
src/emulator/melonds/deko/NdsDekoInput.cpp
src/emulator/melonds/deko/NdsDekoAudio.hpp
src/emulator/melonds/deko/NdsDekoAudio.cpp
```

后续移植 ArcDelta 后端：

```text
src/emulator/melonds/deko/GPU2D_Deko_Adapter.cpp
src/emulator/melonds/deko/GPU3D_Deko_Adapter.cpp
resources/nds_deko/shaders/*.dksh
```

## 第一阶段最小实现清单

第一阶段只做以下内容：

1. 新增配置开关 `nds.dekoMode.enabled`。
2. NDS 游戏启动时可进入 `NdsDekoGamePage`。
3. `NdsDekoGamePage` 创建 `NdsDekoGameView` 和 `NdsDekoGameMenuView`。
4. `NdsDekoGameView` 不运行真实游戏，只调用 `NdsDekoSession::runProbe()`。
5. `runProbe()` 做 Deko 初始化、清屏、present、销毁。
6. 返回 Borealis UI。
7. 菜单键仍可打开 NDS 专属菜单。
8. 退出键返回游戏列表。

第一阶段不做：

- 不移植 GPU2D_Deko。
- 不移植 GPU3D_Deko。
- 不加载 NDS ROM。
- 不处理真实游戏音频。
- 不做高倍分辨率。

## 第一阶段验收日志

建议输出：

```text
NdsDekoGamePage: start
NdsDekoGameView: probe requested
NdsDekoSession: init begin
NdsDekoSession: nwindow acquired
NdsDekoSession: device created
NdsDekoSession: swapchain created
NdsDekoSession: present frame=1
NdsDekoSession: present frame=180
NdsDekoSession: deinit begin
NdsDekoSession: deinit end
NdsDekoGamePage: returned to Borealis
```

若失败，需要记录：

```text
NdsDekoSession: failed step=<step> result=<hex>
NdsDekoSession: fallback to existing GamePage
```

## 推荐执行顺序

1. 先完成阶段0和阶段1。
2. 真机确认 Deko probe 可连续进入退出。
3. 再开始阶段2的 NDS 专属页面业务闭环。
4. 再做阶段3的最小 NDS runtime。
5. 最后移植 ArcDelta GPU2D/GPU3D。

## 决策点

完成阶段1后做一次明确决策：

- 如果 Deko probe 稳定：继续阶段2。
- 如果 Deko probe 可显示但返回 UI 不稳定：尝试硬切换验证。
- 如果硬切换也不稳定：停止当前方案，改评估独立 NDS NRO。
- 如果 Deko 和 Borealis 切换稳定但菜单体验差：继续运行时移植，菜单后续 Deko 化。

## 结论

新增 NDS 专属 `NdsDekoGamePage/NdsDekoGameView/NdsDekoGameMenuView` 是合理方向。它能让 NDS 性能优化脱离现有 OpenGL GameView 的限制，也能保护 libretro 和 mGBA 的渲染音频逻辑。

但这个方案必须从 Deko3D 切换验证开始。只有确认 Deko 能接管默认窗口并可靠返回 Borealis/OpenGL 后，移植 ArcDelta 的 Deko GPU2D/GPU3D 才有工程意义。
