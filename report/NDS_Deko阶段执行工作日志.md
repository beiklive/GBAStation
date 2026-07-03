# NDS Deko阶段执行工作日志

## 2026-07-03 阶段0：准备和隔离

### 目标

- 为 NDS Deko 全屏模式建立独立页面骨架，不复用现有 `GamePage` / `GameView` / `GameMenuView`。
- 保持现有 libretro、mGBA、普通 melonDS OpenGL 路径默认不受影响。
- 只在 Switch 平台且显式开启 `nds.dekoMode.enabled = 1` 时进入实验路径。

### 已实施

- 新增 NDS 专属页面骨架：
  - `src/ui/page/NdsDekoGamePage.hpp`
  - `src/ui/page/NdsDekoGamePage.cpp`
  - `src/ui/view/NdsDekoGameView.hpp`
  - `src/ui/view/NdsDekoGameView.cpp`
  - `src/ui/view/NdsDekoGameMenuView.hpp`
  - `src/ui/view/NdsDekoGameMenuView.cpp`
- `NdsDekoGamePage` 负责：
  - 创建 NDS 专属 `GameView` 和 `GameMenuView` 占位页面。
  - 初始化 NDS 游戏条目路径、默认核心、存档目录和分辨率倍率。
  - 维护播放次数和最近游玩时间。
  - 处理菜单打开、关闭、退出到上一页。
- `NdsDekoGameView` 负责：
  - 注册 `BUTTON_START` 菜单触发。
  - 提供 Phase 0/Phase 1 状态占位显示。
- `NdsDekoGameMenuView` 负责：
  - 注册 `BUTTON_A` / `BUTTON_B` 继续。
  - 注册 `BUTTON_X` 退出。
- `StartPage` 已加入统一游戏启动 helper：
  - 普通路径继续创建 `GamePage`。
  - Switch + NDS + `nds.dekoMode.enabled != 0` 时创建 `NdsDekoGamePage`。
  - 最近游戏、游戏库、文件浏览器三个入口都走同一套分流逻辑。
- 修复 `NdsDekoGamePage::_closeMenu()` 中 `GameDB` 可能为空时直接 `flush()` 的风险。

### 隔离状态

- 默认配置下 `nds.dekoMode.enabled` 为 `0`，不会进入 NDS Deko 实验页面。
- 非 Switch 平台不会进入 NDS Deko 实验页面。
- 当前阶段没有接管 OpenGL / Deko3D 上下文，没有修改 libretro 或 mGBA 渲染音频逻辑。

### 待验证

- 使用 `switchbuild.sh` 编译确认新增页面和分流入口能通过 Switch 构建。
- 在 Switch 上手动设置 `nds.dekoMode.enabled = 1` 后确认：
  - NDS 游戏能进入专属占位页面。
  - `+` 能打开专属菜单。
  - `A/B` 能关闭菜单。
  - `X` 能退出回上一页。

### 下一阶段入口

- 阶段1开始实现最小 Deko3D 切换 probe：
  - 只验证 Deko 初始化、清屏、present、退出释放。
  - 暂不启动 melonDS 模拟线程。
  - 暂不绘制真实 NDS 画面。

### 构建记录

- 第一次 Switch 构建失败：
  - `NdsDekoGameView.cpp` 和 `NdsDekoGameMenuView.cpp` 使用了 `NVG_ALIGN_CENTER`。
  - 当前 borealis `Label::setHorizontalAlign()` 需要 `brls::HorizontalAlign`。
- 已修复：
  - 改为 `brls::HorizontalAlign::CENTER`。
- 第二次 Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.65 MB`
  - 结论：阶段0新增页面、菜单、`StartPage` 分流入口已通过编译和打包验证。

## 2026-07-03 阶段1：Deko3D切换 probe

### 目标

- 在不启动 melonDS、不接真实 NDS 画面的前提下，验证当前 OpenGL/Borealis 程序里是否能临时创建 Deko3D device/queue/swapchain，并全屏 present 后释放。
- 继续保持默认关闭，只有进入 `NdsDekoGamePage` 后才运行 probe。

### 已实施

- 新增最小 probe 模块：
  - `src/emulator/melonds/deko/NdsDekoProbe.hpp`
  - `src/emulator/melonds/deko/NdsDekoProbe.cpp`
- `RunNdsDekoProbe()` 在 Switch 平台执行：
  - 获取默认显示分辨率，失败时按掌机/底座模式回退到 `1280x720` 或 `1920x1080`。
  - 创建独立 `dk::Device`、`dk::Queue`、`dk::CmdBuf`。
  - 创建 2 个 RGBA8 present framebuffer。
  - 基于 `nwindowGetDefault()` 创建 Deko swapchain。
  - 记录两个清屏 command list。
  - acquire/clear/present 约 60 帧。
  - `waitIdle()` 后销毁 swapchain、cmd buffer、mem block、queue、device。
- `NdsDekoGameView::startProbe()` 已接入 probe：
  - 运行中显示阶段1状态。
  - 完成后显示成功/失败/不支持状态。
  - 输出日志字段：`supported`、`success`、`presentedFrames`、`message`。
- `CMakeLists.txt` 在 Switch 平台额外链接 `deko3d`。

### 隔离状态

- 仍未复用或修改现有 `GameView` / `GameMenuView`。
- 仍未修改 libretro 和 mGBA 渲染、音频逻辑。
- 仍未启动 melonDS 运行线程。
- 当前 probe 只验证图形上下文切换可行性。

### 风险记录

- 当前 Borealis Switch 构建使用 OpenGL/GLFW/EGL 路径；probe 会在同一进程中创建 Deko3D swapchain。
- 这一步正是需要验证的风险点：如果 Switch 实机上出现黑屏、UI 无法恢复或崩溃，应停止软切换路线，转入阶段1B或改评估独立 NRO / 全 Deko UI。

### 待验证

- 编译确认 `deko3d` 链接和新增 probe 通过。
- 实机开启 `nds.dekoMode.enabled = 1` 后测试：
  - 进入 NDS 游戏时能看到约 1 秒 Deko 清屏。
  - 返回 NDS 专属占位 UI 后字体和输入正常。
  - `+` 菜单、`A/B` 关闭、`X` 退出正常。
  - 连续进入/退出至少 10 次不崩。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：阶段1新增 Deko3D probe 已通过编译和链接验证。
- 尚未完成：
  - 实机运行验证。该阶段的真正结论必须以 Switch 上是否能稳定返回 Borealis UI 为准。

## 2026-07-03 阶段1实机崩溃处理

### 用户反馈

- 设置 `nds.dekoMode.enabled=i|1` 后，一打开 NDS 游戏程序即崩溃。
- `E:\GBAStation.log` 中最后记录停在：
  - `Pushing NdsDekoGamePage activity ...`
  - 之后没有出现 `NdsDekoGameView: stage1 probe start`
  - 也没有出现 `NdsDekoProbe: probe begin`

### 判断

- 崩溃发生在 NDS 专属页面 push / show 生命周期附近，或日志尚未 flush 前即进入 probe 崩溃。
- 当前阶段不能继续默认自动运行 Deko probe。
- `nds.dekoMode.enabled` 应先只用于验证 NDS 专属页面分流是否稳定。

### 已实施修复

- 将 Deko probe 拆为独立二级开关：
  - `nds.dekoMode.enabled=i|1`：只进入 NDS 专属页面。
  - `nds.dekoMode.probe.enabled=i|1`：进入专属页面后才运行 Deko3D 清屏 probe。
- 默认配置新增：
  - `nds.dekoMode.enabled = 0`
  - `nds.dekoMode.probe.enabled = 0`
- `NdsDekoGameView::startProbe()` 在 probe 未开启时只显示占位状态，不创建 Deko device/swapchain。
- 修复 `NdsDekoGamePage::_closeMenu()` 中 `GameDB` 为空时直接 `flush()` 的风险。

### 下一步实机验证顺序

1. 只保留 `nds.dekoMode.enabled=i|1`，不要设置 `nds.dekoMode.probe.enabled`。
2. 启动 NDS 游戏，确认能进入 NDS 专属占位页面。
3. 确认 `+` 打开菜单、`A/B` 返回、`X` 退出。
4. 若专属页面稳定，再设置 `nds.dekoMode.probe.enabled=i|1` 单独验证 Deko probe。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：崩溃保护修复已通过编译和打包验证。

## 2026-07-03 阶段1继续：Deko probe分级验证

### 用户反馈

- 只设置 `nds.dekoMode.enabled=i|1` 后，NDS 专属占位页可以正常显示。

### 判断

- `NdsDekoGamePage` / `NdsDekoGameView` / `NdsDekoGameMenuView` 的页面分流路径基本可用。
- 前一次崩溃高度集中在 Deko probe 或 OpenGL/Borealis 与 Deko3D 同进程资源切换。

### 已实施

- `RunNdsDekoProbe()` 改为分级 probe：
  - level 1：只创建并销毁 `dk::Device`。
  - level 2：创建并销毁 `dk::Device` + `dk::Queue`。
  - level 3：创建 Deko 离屏 framebuffer memory/images，不触碰 `nwindowGetDefault()` 和 swapchain。
  - level 4：基于 `nwindowGetDefault()` 创建 `dk::Swapchain`，但不 acquire/present。
  - level 5：创建 command list，执行 acquire/clear/present。
- 新增配置：
  - `nds.dekoMode.probe.level=i|1`
- UI 日志和占位页会显示：
  - 请求级别 `requestedLevel`
  - 实际通过级别 `reachedLevel`
  - present 帧数 `presentedFrames`

### 下一步实机验证顺序

保持：

```text
nds.dekoMode.enabled=i|1
nds.dekoMode.probe.enabled=i|1
```

然后逐次修改：

```text
nds.dekoMode.probe.level=i|1
nds.dekoMode.probe.level=i|2
nds.dekoMode.probe.level=i|3
nds.dekoMode.probe.level=i|4
nds.dekoMode.probe.level=i|5
```

每次只提高一级。若某一级崩溃，下一轮读取 `GBAStation.log`，用最后一条 `NdsDekoProbe:` 日志定位崩溃点。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：Deko probe 分级验证版本已通过编译和打包验证。

## 2026-07-03 阶段1继续：level 3崩溃后的再拆分

### 用户反馈

- 设置旧版 `nds.dekoMode.probe.level=i|3` 后崩溃。
- `E:\GBAStation.log` 最后可靠日志为：
  - `NdsDekoProbe: level 1 create device`
  - `NdsDekoProbe: level 2 create queue`
  - 之后未看到完成 level 3 的日志。

### 判断

- 旧版 level 3 同时包含 framebuffer memory/images 与 `dk::SwapchainMaker{device, nwindowGetDefault(), ...}`。
- 需要拆出“离屏 framebuffer”和“绑定默认窗口创建 swapchain”，才能判断是否为 OpenGL/GLFW 已占用 `nwindow` 导致。

### 已实施

- 新 level 3：只创建 Deko framebuffer memory/images，不创建 swapchain。
- 新 level 4：创建 swapchain，但不 present。
- 新 level 5：执行 acquire/clear/present。
- 增加更细的 `NdsDekoProbe:` 日志：
  - device created
  - queue created
  - framebuffer layout initialized
  - image memory created
  - framebuffer images initialized
  - swapchain created

### 下一步实机验证

- 重新测试：
  - `nds.dekoMode.probe.level=i|3`
  - 如果通过，再测试 `i|4`
- 若新 level 3 通过而 level 4 崩溃，可基本确认软切换路线卡在同一 `nwindow` 创建第二套 Deko swapchain。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：level 3/4/5 再拆分版本已通过编译和打包验证。

## 2026-07-03 阶段1继续：按键触发probe

### 用户反馈

- 反复修改配置文件测试 probe level 太麻烦，希望直接通过打印日志定位。

### 已实施

- NDS Deko 占位页新增 `Y` 键触发分级 probe：
  - 第一次按 `Y`：运行 level 1。
  - 若成功，下一次自动运行 level 2。
  - 依次推进到 level 5。
- `+` 仍用于打开 NDS 专属菜单。
- 旧配置开关仍保留：
  - `nds.dekoMode.probe.enabled` 和 `nds.dekoMode.probe.level` 不再自动触发 probe。
  - 进入页面后只通过 `Y` 手动逐级运行。

### 推荐测试方式

只保留：

```text
nds.dekoMode.enabled=i|1
```

进入 NDS 占位页后按 `Y`，每按一次看日志和页面提示。如果某次按下后崩溃，读取 `GBAStation.log` 最后一条 `NdsDekoProbe:` 即可定位。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：按 `Y` 逐级触发 Deko probe 的版本已通过编译和打包验证。

## 2026-07-03 阶段1继续：level 4前再拆分

### 用户日志结论

- 新 level 3 已完整通过：
  - `framebuffer layout initialized`
  - `image memory created`
  - `framebuffer images initialized`
  - `finish level=3 success=true`
- 按 `Y` 继续到旧 level 4 后，最后日志停在：
  - `create image memory, stride=3932160, total=7864320`
  - 没有出现 `image memory created`

### 判断

- 崩溃还没有到 `SwapchainMaker`。
- 与新 level 3 相比，旧 level 4 的差异主要是 framebuffer layout 带 `DkImageFlags_UsagePresent`。
- 需要继续区分“带 present flag 的 layout”与“基于该 layout 分配/初始化 image memory”。

### 已实施

- probe 改为 7 级：
  - level 1：`dk::Device`
  - level 2：`dk::Queue`
  - level 3：不带 `UsagePresent` 的离屏 framebuffer memory/images
  - level 4：只创建带 `UsagePresent` 的 framebuffer layout，不分配 image memory
  - level 5：带 `UsagePresent` 的 framebuffer memory/images
  - level 6：`dk::SwapchainMaker{device, nwindowGetDefault(), ...}`
  - level 7：acquire/clear/present
- 进入 NDS 占位页后不再读取旧 probe 配置自动执行。
- 只通过 `Y` 手动逐级运行，避免旧配置残留导致一进页面就崩。

### 下一步实机验证

- 只保留 `nds.dekoMode.enabled=i|1`。
- 进入 NDS 占位页后连续按 `Y`。
- 如果 level 4 通过、level 5 崩，说明带 `UsagePresent` 的 framebuffer memory/images 不能在当前 OpenGL/GLFW 环境下安全创建。
- 如果 level 5 通过、level 6 崩，才说明问题集中在同一 `nwindow` 上创建 Deko swapchain。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：手动触发 + 7 级细分 probe 已通过编译和打包验证。

## 2026-07-03 阶段1继续：level 6崩溃后的窗口/swapchain再拆分

### 用户日志结论

- 只设置 `nds.dekoMode.enabled=i|1` 后，NDS 专属占位页可以进入。
- 连续按 `Y` 后，level 1 到 level 5 均完整通过。
- level 6 崩溃前最后可靠日志停在：
  - `NdsDekoProbe: framebuffer layout initialized, presentFlag=yes, size=3932160, alig...`
- 已通过的信息：
  - `dk::Device` 可创建/销毁。
  - `dk::Queue` 可创建/销毁。
  - 不带 `UsagePresent` 的 framebuffer memory/images 可创建/销毁。
  - 带 `UsagePresent` 的 framebuffer layout 可创建。
  - 带 `UsagePresent` 的 framebuffer memory/images 至少在 level 5 单独测试中可创建/销毁。

### 判断

- 当前崩溃仍集中在 Deko3D 接近窗口/交换链接管的路径，但旧 level 6 覆盖范围仍偏大。
- 需要把 `nwindowGetDefault()`、`dk::SwapchainMaker` 构造、`create()`、`setSwapInterval()`、命令列表、`acquire/present` 逐步拆开。
- 如果最终确认崩在 `SwapchainMaker.create()`，基本可以判定：在当前 Borealis/GLFW/OpenGL 已经拥有默认窗口显示链路时，软创建第二套 Deko swapchain 不可靠。

### 已实施

- probe 从 7 级扩展到 10 级：
  - level 1：`dk::Device`
  - level 2：`dk::Queue`
  - level 3：不带 `UsagePresent` 的离屏 framebuffer memory/images
  - level 4：只创建带 `UsagePresent` 的 framebuffer layout
  - level 5：带 `UsagePresent` 的 framebuffer memory/images
  - level 6：只调用并记录 `nwindowGetDefault()`，不创建 framebuffer 和 swapchain
  - level 7：重新创建 present framebuffer，然后细分记录 `nwindowGetDefault()`、`SwapchainMaker` 构造、`swapchain.create()`
  - level 8：`swapchain.setSwapInterval(1)`
  - level 9：创建 command buffer、command memory、clear command lists
  - level 10：逐帧记录 `acquireImage`、`submitCommands`、`presentImage`
- `NdsDekoGameView` 的 `Y` 键自动推进上限同步改为 level 10。

### 下一步实机验证

- 保持只设置：

```text
nds.dekoMode.enabled=i|1
```

- 进入 NDS 专属占位页后继续按 `Y`。
- 如果 level 6 通过而 level 7 崩溃，请重点查看最后一条日志是：
  - `level 7 get default nwindow before swapchain ...`
  - `level 7 construct swapchain maker ...`
  - `level 7 swapchain create begin`
  - `level 7 swapchain create end`
- 其中若停在 `swapchain create begin` 后，说明崩溃基本锁定在同一默认窗口上的 Deko swapchain 创建。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.71 MB`
  - 结论：level 6 到 level 10 再拆分版本已通过编译和打包验证。

## 2026-07-03 阶段1继续：level 7崩溃日志缓冲处理

### 用户日志结论

- 最新 `E:\GBAStation.log` 显示：
  - level 1 到 level 6 全部通过。
  - level 6 中 `nwindowGetDefault()` 返回非空指针：`0xf1b5d1400`。
  - level 7 开始后，最后一行停在半截日志：
    - `NdsDekoProbe: framebuffer layout ...`
- 由于 level 4 和 level 5 已经单独证明 present-capable layout 与 framebuffer memory/images 可以创建，level 7 最后一行半截不能直接说明崩在 layout。
- 更可能的情况是 Borealis Logger 的文件输出在崩溃时没有完整 flush，真实崩溃点可能已经到了更靠后的 image 初始化、`nwindowGetDefault()`、`SwapchainMaker` 或 `swapchain.create()`。

### 已实施

- 在 `NdsDekoProbe.cpp` 中新增同步 checkpoint 日志：
  - 保留原有 `GBAStation.log` 的 `NdsDekoProbe:` 输出。
  - 同时追加写入 `NdsDekoProbe.checkpoint.log`。
  - 每条 checkpoint 都 `fflush()` 并立即 `fclose()`，减少崩溃时丢最后几行的概率。
- checkpoint 文件尝试路径：
  - `/GBAStation/log/NdsDekoProbe.checkpoint.log`
  - `sdmc:/GBAStation/log/NdsDekoProbe.checkpoint.log`
- 对所有危险调用补充 begin/end checkpoint：
  - device / queue
  - framebuffer layout
  - image memory
  - framebuffer image 初始化
  - `nwindowGetDefault()`
  - `SwapchainMaker` 构造
  - `swapchain.create()`
  - `setSwapInterval()`
  - command list
  - acquire / submit / present

### 下一步实机验证

- 继续只设置：

```text
nds.dekoMode.enabled=i|1
```

- 进入 NDS 专属占位页后按 `Y` 到崩溃。
- 崩溃后除了 `GBAStation.log`，请优先查看/提供：

```text
GBAStation/log/NdsDekoProbe.checkpoint.log
```

- 这个文件的最后一条完整日志将作为下一步路线判断依据。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：同步 checkpoint 日志版本已通过编译和打包验证。

## 2026-07-03 阶段1结论：软创建第二套Deko swapchain不可行

### 用户 checkpoint 结论

- `E:\NdsDekoProbe.checkpoint.log` 完整记录了 level 1 到 level 7。
- level 1 到 level 6 全部通过：
  - `dk::Device` 可创建。
  - `dk::Queue` 可创建。
  - offscreen framebuffer memory/images 可创建。
  - present-capable framebuffer layout 可创建。
  - present-capable framebuffer memory/images 可创建。
  - `nwindowGetDefault()` 可调用，并返回非空指针。
- level 7 崩溃前最后完整 checkpoint 为：

```text
level 7 swapchain create begin
```

- 没有出现：

```text
level 7 swapchain create end
```

### 判断

- 崩溃点已经锁定在：

```cpp
swapchain = swapchainMaker.create();
```

- 这证明当前 Borealis/OpenGL/GLFW 已经持有默认 `nwindow` 显示链路时，在同一个默认窗口上软创建第二套 Deko swapchain 会崩溃。
- 因此“进入 NDS 页面后直接额外创建 Deko swapchain 接管画面，退出再回 Borealis/OpenGL”这条软切换路线不可行。
- 这也解释了为什么 ArcDelta_melonDS 可行：它从程序启动阶段就由 Deko3D 独占初始化窗口、device、queue、swapchain；它不是在一个已经运行的 OpenGL/Borealis swapchain 旁边再插入第二套 Deko swapchain。

### 已实施保护

- `NdsDekoGameView` 的 `Y` 键自动推进上限从 level 10 收口到 level 6。
- 用户继续按 `Y` 不会再触发已知会崩溃的 level 7。
- level 7 到 level 10 探针代码暂时保留，用作后续硬切换/独立进程方案验证依据，但不会由普通 UI 自动触发。

### 后续路线

- 阶段1软切换结论：阻塞。
- 下一阶段只剩三条可行方向：
  - 硬切换：进入 NDS 前销毁或释放 Borealis/GLFW/OpenGL 的窗口显示链路，再初始化 Deko；退出时重建 Borealis/OpenGL。风险高，改动范围大。
  - 独立 NRO：NDS Deko 模式做成单独程序，由当前前端传参启动，ArcDelta_melonDS 更接近这个模型。隔离最好，但需要解决启动参数、返回前端、存档/配置共享。
  - 放弃 Deko 接管：回到当前 OpenGL/melonDS 路径，只优化 x1 下 CPU/GPU/内存和同步策略。风险最低，但性能上限低于原生 Deko 独占架构。

### 构建记录

- Switch 构建通过：
  - 产物：`build_switch/GBAStation.nro`
  - 大小：`26.70 MB`
  - 结论：level 7 崩溃结论记录与 UI 防误触保护已通过编译和打包验证。
