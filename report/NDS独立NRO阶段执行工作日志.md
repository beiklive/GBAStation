# NDS独立NRO阶段执行工作日志

## 2026-07-03 阶段A：Stub NRO链式启动闭环

### 目标

- 验证主程序能通过 libnx `envSetNextLoad()` 拉起独立 NDS NRO。
- 验证主程序能把 NDS ROM 路径作为 argv 传给独立 NRO。
- 验证独立 NRO 能通过 `envSetNextLoad()` 返回主程序。
- 不启动真实 melonDS，不接入 ArcDelta，不触碰 libretro/mGBA/现有 GameView 渲染音频路径。

### 已实施

- 新增 Switch-only NRO 启动封装：
  - `src/platform/switch/NroLauncher.hpp`
  - `src/platform/switch/NroLauncher.cpp`
- 新增最小 Stub NRO：
  - `src/platform/switch/nds_stub/NdsStubMain.cpp`
- Stub 行为：
  - 记录 `argc/argv`。
  - 识别 `argv[1]` 为 ROM 路径。
  - 识别 `--return <nro>` 为返回主程序路径。
  - 退出前尝试 `envSetNextLoad(returnNro, ...)` 返回主程序。
  - 写日志到：

```text
sdmc:/GBAStation/log/GBAStationNDSStub.log
/GBAStation/log/GBAStationNDSStub.log
sdmc:/GBAStationNDSStub.log
/GBAStationNDSStub.log
```

- `StartPage` 新增 NDS 外部 NRO 分流：
  - `nds.externalNro.enabled = 1` 时，NDS 游戏启动走独立 NRO。
  - 启动失败时回退原有路径。
  - 默认关闭，不影响当前 NDS/OpenGL 路径和 NDS Deko 实验页。
- 新增默认配置：

```text
nds.externalNro.enabled = 0
nds.externalNro.path = sdmc:/switch/GBAStationNDSStub.nro
nds.externalNro.returnPath = sdmc:/switch/GBAStation.nro
```

- CMake 新增独立产物：

```text
build_switch/GBAStationNDSStub.nro
```

- `switchbuild.sh` 新增 Stub NRO 打包和大小输出。

### 待验证

- Switch 编译通过。
- 将两个 NRO 放到 SD 卡：

```text
sdmc:/switch/GBAStation.nro
sdmc:/switch/GBAStationNDSStub.nro
```

- 配置：

```text
nds.externalNro.enabled=i|1
nds.externalNro.path=s|sdmc:/switch/GBAStationNDSStub.nro
nds.externalNro.returnPath=s|sdmc:/switch/GBAStation.nro
```

- 启动 NDS 游戏后应进入 Stub，再自动返回主程序。
- 查看 `GBAStationNDSStub.log` 中是否包含 ROM 路径。

### 构建记录

- 第一次 Switch 构建失败：
  - 原因：Stub target 命名为 `GBAStationNDSStub.elf`，CMake 输出实际变成 `GBAStationNDSStub.elf.elf`。
  - `elf2nro` 使用固定输入名 `GBAStationNDSStub.elf`，因此报错 `Failed to open input!`。
- 已修复：
  - Stub target 改名为 `GBAStationNDSStub`。
  - `elf2nro` 输入改为 `$<TARGET_FILE:GBAStationNDSStub>`。
- 第二次 Switch 构建通过：
  - 主程序：`build_switch/GBAStation.nro`
  - 大小：`26.71 MB`
  - Stub：`build_switch/GBAStationNDSStub.nro`
  - 大小：`0.29 MB`
  - 结论：阶段A最小 Stub NRO 链式启动闭环代码已通过编译和打包验证。
- 第三次 Switch 构建通过：
  - 新增启动前文件存在性检查：
    - NDS NRO 不存在时不退出主程序。
    - ROM 不存在时不退出主程序。
  - 产物大小保持：
    - `GBAStation.nro`：`26.71 MB`
    - `GBAStationNDSStub.nro`：`0.29 MB`

## 2026-07-03 阶段B：主程序移除 NDS 运行时，Stub 接管 NDS 入口数据

### 目标

- `GBAStation.nro` 不再链接 melonDS，不再进入旧 NDS `GamePage/GameView` 或 Deko probe 路径。
- Switch 主程序打开 NDS 游戏时只负责把 ROM path 传给独立 NRO。
- 独立 NRO 路径固定为 `/GBAStation/core/GBAStationNDSStub.nro`。
- `GBAStationNDSStub.nro` 根据传入 ROM path 读取 `/GBAStation/data/GameData_NDS.json` 中的 GameDB 数据。

### 已实施

- 主程序 CMake：
  - 排除 `src/emulator/melonds/**`。
  - 排除旧 Deko 占位页 `NdsDekoGamePage/View/MenuView`。
  - 移除主程序对 `melonds_core` 的链接。
  - 移除 Switch 主程序额外 `deko3d` 链接。
- `third_party/CMakeLists.txt`：
  - 新增 `BUILD_MELONDS_CORE` 开关，默认 `OFF`。
  - melonDS 静态库仅在显式开启时构建。
- `switchbuild.sh`：
  - 打包后额外复制 Stub 到 `build_switch/GBAStation/core/GBAStationNDSStub.nro`，与运行时路径 `/GBAStation/core/GBAStationNDSStub.nro` 对齐。
- `StartPage`：
  - Switch 上 NDS 游戏固定走外部 NRO。
  - 启动失败时只提示错误，不回退到旧 melonDS/GameView。
  - 默认 NDS Stub 路径改为 `/GBAStation/core/GBAStationNDSStub.nro`。
- 配置迁移：
  - 如果现有配置仍是旧路径 `sdmc:/switch/GBAStationNDSStub.nro`，启动时自动迁移为 `/GBAStation/core/GBAStationNDSStub.nro`。
- `EmulatorCoreFactory`：
  - 移除 `MelonDSCore` include。
  - `EmuNDS` 返回 `nullptr`，防止旧路径重新拉入 melonDS。
- `GBAStationNDSStub.nro`：
  - 读取 `/GBAStation/data/GameData_NDS.json` 或 `sdmc:/GBAStation/data/GameData_NDS.json`。
  - 按原始 path 和去掉 `sdmc:` 后的规范化 path 匹配记录。
  - 参数解析兼容 `argv[1]=rom` 和 `argv[1]=nro, argv[2]=rom` 两种形式。
  - 将匹配到的标题、存档路径、金手指路径、截图路径、NDS 布局和内部分辨率写入 `GBAStationNDSStub.log`。
- 主程序公共符号：
  - 新增 `src/emulator/IEmulatorVideoTexture.cpp`，把 `EmulatorGLMutex()` 从 melonDS 侧移到主程序公共实现。
- NDS 金手指解析：
  - `CheatSystem.cpp` 中的 melonDS `usrcheat.dat` 解析依赖改为 `GBASTATION_ENABLE_MELONDS_CHEAT_DAT` 条件编译。
  - 主 NRO 默认不再 include `ARDatabaseDAT.h / CRC32.h / NDS_Header.h`。

### 待验证

- Switch 构建通过。
- 主程序 NRO 体积应下降，且不应出现 `Linking melonDS (NDS) native core statically`。
- SD 卡放置路径：

```text
/GBAStation/core/GBAStationNDSStub.nro
/GBAStation/data/GameData_NDS.json
```

- 启动任意 NDS 游戏后，Stub 日志应包含：

```text
GBAStationNDSStub: romPath=...
GBAStationNDSStub: gameDb.found=1
GBAStationNDSStub: gameDb.title=...
```

### 构建记录

- 第一次阶段B构建失败：
  - 原因：`CheatSystem.cpp` 直接 include melonDS 的 `ARDatabaseDAT.h`。
  - 处理：将 melonDS `usrcheat.dat` 解析放入 `GBASTATION_ENABLE_MELONDS_CHEAT_DAT` 条件编译，主程序默认不编译。
- 第二次阶段B构建失败：
  - 原因：`GameView.cpp` 引用的 `EmulatorGLMutex()` 过去由 `MelonDSCore.cpp` 提供。
  - 处理：新增公共实现 `src/emulator/IEmulatorVideoTexture.cpp`。
- 第三次阶段B构建通过：
  - CMake 输出只包含：
    - `Linking FCEUmm (NES) core statically`
    - `Linking Snes9x (SNES) core statically`
    - `Linking Nestopia (NES) core statically`
    - `Linking Snes9x 2005 (SNES) core statically`
    - `Linking mGBA native core`
  - 未出现 `Linking melonDS (NDS) native core statically`。
  - 产物：

```text
build_switch/GBAStation.nro                         24.91 MB
build_switch/GBAStationNDSStub.nro                  0.82 MB
build_switch/GBAStation/core/GBAStationNDSStub.nro
```

  - 对比阶段A，主程序从 `26.71 MB` 降至 `24.91 MB`，说明 melonDS 静态链接已从主 NRO 移除。
- 第四次阶段B构建通过：
  - 补充旧配置路径迁移后重新验证。
  - 产物大小保持：
    - `GBAStation.nro`：`24.91 MB`
    - `GBAStationNDSStub.nro`：`0.82 MB`
