# BeikLiveStation

基于 [libretro](https://www.libretro.com/) 核心与 [borealis](https://github.com/xfangfang/borealis) UI 框架构建的跨平台模拟器前端，当前内置 mGBA（Game Boy Advance）libretro 核心，支持 Linux、macOS、Windows 及 Nintendo Switch 平台。

---

## 目录

- [功能概览](#功能概览)
- [环境依赖](#环境依赖)
- [构建说明](#构建说明)
- [运行方式](#运行方式)
- [配置参考](#配置参考)
- [项目结构](#项目结构)

---

## 功能概览

| 功能 | 说明 |
|------|------|
| **libretro 核心加载** | 运行时动态加载（Linux/macOS/Windows），Switch 平台静态链接 |
| **游戏画面渲染** | OpenGL 纹理上传 + NanoVG 绘制，支持 XRGB8888 格式 |
| **多种画面缩放模式** | Fit / Fill / Original / IntegerScale / Custom |
| **纹理过滤模式** | Nearest（像素风格）/ Linear（平滑插值） |
| **跨平台音频输出** | ALSA（Linux）/ WinMM（Windows）/ CoreAudio（macOS）/ audout（Switch） |
| **手柄输入** | 通过 borealis ControllerState 统一抽象，支持自定义按键映射 |
| **键盘输入** | 自动检测键盘模式，支持字符串名称或数值方式配置 |
| **快进（Fast-Forward）** | 可配置倍率、静音、按住或切换模式 |
| **倒带（Rewind）** | 可选功能，支持配置缓冲帧数、倒带步长及静音 |
| **FPS 显示** | 可在画面上叠加实时帧率 |
| **状态叠加层** | 快进和倒带时显示状态提示 |
| **存档序列化接口** | `LibretroLoader` 已暴露 `serialize` / `unserialize` 接口 |
| **后处理着色器链** | OpenGL 着色器通道（ShaderChain）框架，可串联多道后处理效果 |
| **配置文件管理** | 基于 ConfigManager，所有参数持久化存储，首次运行自动生成默认配置 |

---

## 环境依赖

### 通用依赖

| 依赖项 | 版本要求 | 说明 |
|--------|----------|------|
| CMake | ≥ 3.10 | 构建系统 |
| C++17 | — | 编译器需支持 C++17 标准 |
| OpenGL | — | 渲染后端 |

### Linux

```bash
sudo apt install \
    cmake build-essential \
    libgl1-mesa-dev libglu1-mesa-dev \
    libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libdbus-1-dev \
    libasound2-dev
```

### macOS

- Xcode Command Line Tools
- CMake（推荐通过 [Homebrew](https://brew.sh/) 安装）：`brew install cmake`
- CoreAudio 由系统框架提供，无需额外安装

### Windows

- [MSYS2](https://www.msys2.org/) 与 MinGW64 工具链
- Ninja 构建工具：`pacman -S mingw-w64-x86_64-ninja`
- CMake：`pacman -S mingw-w64-x86_64-cmake`
- WinMM 由系统提供，无需额外安装

### Nintendo Switch

- [devkitPro](https://devkitpro.org/) 工具链（devkitA64）
- libnx（由 devkitPro 提供）
- 参照 `switchbuild.sh` 配置交叉编译环境

---

## 构建说明

所有平台均使用 CMake 构建，提供各平台专用脚本简化操作。

### Linux

```bash
./linuxbuild.sh
```

产物目录：`build_linux/`

```
build_linux/
├── BKStation            # 主程序
└── mgba_libretro.so     # libretro 核心（构建后自动复制至此）
```

### macOS

```bash
# 普通可执行文件
./macosbuild.sh

# 打包为 .app Bundle
./macosbuild.sh --bundle
```

产物目录：`build_macos/`

```
build_macos/
├── BKStation            # 主程序
└── mgba_libretro.dylib  # libretro 核心
```

### Windows（MSYS2 MinGW64 终端）

```bat
windowsbuild.bat
```

产物目录：`build_windows\`

```
build_windows\
├── BKStation.exe        # 主程序
└── mgba_libretro.dll    # libretro 核心
```

### Nintendo Switch

```bash
./switchbuild.sh
```

产物：`build_switch/BKStation.nro`

---

## 运行方式

将 ROM 文件（`.gba`）放置于任意目录，启动主程序后通过界面文件浏览器选择 ROM 即可开始游戏。

> **注意**：`mgba_libretro.so/.dll/.dylib` 必须与主程序位于同一目录（构建脚本已自动处理）。

---

## 配置参考

配置文件在首次运行时自动生成，路径位于程序工作目录。所有配置项均以 `key = value` 格式存储，支持热重载。

### 画面显示（`display.*`）

| 配置键 | 默认值 | 可选值 | 说明 |
|--------|--------|--------|------|
| `display.mode` | `original` | `fit` / `fill` / `original` / `integer` / `custom` | 画面缩放模式 |
| `display.filter` | `nearest` | `nearest` / `linear` | 纹理过滤模式 |
| `display.integer_scale_mult` | `0` | 整数 ≥ 0 | 整数倍率（`0` = 自动最大整数倍） |
| `display.custom_scale` | `1.0` | 浮点数 | Custom 模式下的缩放倍数 |
| `display.x_offset` | `0.0` | 浮点数（像素） | 相对居中位置的水平偏移 |
| `display.y_offset` | `0.0` | 浮点数（像素） | 相对居中位置的垂直偏移 |
| `display.showFps` | `false` | `true` / `false` | 是否显示 FPS 叠加层 |
| `display.showFfOverlay` | `true` | `true` / `false` | 是否显示快进状态提示 |
| `display.showRewindOverlay` | `true` | `true` / `false` | 是否显示倒带状态提示 |

### 快进（`fastforward.*`）

| 配置键 | 默认值 | 说明 |
|--------|--------|------|
| `fastforward.multiplier` | `4.0` | 快进速度倍率（相对正常速度） |
| `fastforward.mute` | `true` | 快进期间是否静音 |
| `fastforward.mode` | `hold` | `hold`（按住触发）/ `toggle`（切换触发） |

### 倒带（`rewind.*`）

| 配置键 | 默认值 | 说明 |
|--------|--------|------|
| `rewind.enabled` | `false` | 是否启用倒带功能 |
| `rewind.bufferSize` | `3600` | 保存的最大历史帧数 |
| `rewind.step` | `2` | 每次倒带回退的帧数 |
| `rewind.mute` | `false` | 倒带期间是否静音 |
| `rewind.mode` | `hold` | `hold`（按住触发）/ `toggle`（切换触发） |

### 手柄按键映射（`handle.*`）

| 配置键 | 默认值 | 说明 |
|--------|--------|------|
| `handle.fastforward` | `RT` | 触发快进的手柄按键 |
| `handle.rewind` | `LT` | 触发倒带的手柄按键 |
| `handle.<retro_button>` | — | 将手柄按键映射至 libretro 按键（如 `handle.a = A`） |

支持的按键名称：`A`、`B`、`X`、`Y`、`LB`、`RB`、`LT`、`RT`、`START`、`BACK`、`UP`、`DOWN`、`LEFT`、`RIGHT` 等。

### 键盘按键映射（`keyboard.*`）

| 配置键 | 说明 |
|--------|------|
| `keyboard.<retro_button>` | 将键盘按键映射至 libretro 按键（如 `keyboard.a = X`） |
| `keyboard.exit` | 游戏内退出键（如 `keyboard.exit = ESC`） |

支持字符串名称（`A`–`Z`、`0`–`9`、`ENTER`、`ESC`、`TAB`、`SPACE`、`UP`、`DOWN`、`LEFT`、`RIGHT`、`F1`–`F12` 等）或数值形式。

**默认键盘映射：**

| 键盘键 | 模拟器按键 |
|--------|-----------|
| `X` | A |
| `Z` | B |
| `A` | Y |
| `S` | Select |
| `Enter` | Start |
| `Q` | L |
| `W` | R |
| `E` | L2 |
| `R` | R2 |
| 方向键 | 十字键 |

---

## 项目结构

```
BeikLiveStation/
├── main.cpp                    # 程序入口
├── CMakeLists.txt              # 主构建配置
├── linuxbuild.sh               # Linux 构建脚本
├── macosbuild.sh               # macOS 构建脚本
├── windowsbuild.bat            # Windows 构建脚本
├── switchbuild.sh              # Nintendo Switch 构建脚本
├── include/
│   ├── Audio/
│   │   └── AudioManager.hpp   # 跨平台音频管理器接口
│   ├── Game/
│   │   ├── LibretroLoader.hpp # libretro 动态库加载器接口
│   │   ├── DisplayConfig.hpp  # 画面缩放与过滤配置
│   │   └── ShaderChain.hpp    # 后处理着色器链接口
│   ├── UI/
│   │   └── game_view.hpp      # 游戏视图（渲染 + 输入 + 游戏线程）
│   └── Utils/
│       ├── ConfigManager.hpp  # 配置文件读写管理
│       ├── fileUtils.hpp      # 文件路径工具
│       └── strUtils.hpp       # 字符串工具
├── src/
│   ├── Audio/
│   │   └── AudioManager.cpp   # 多平台音频后端实现
│   ├── Game/
│   │   └── LibretroLoader.cpp # 跨平台动态加载实现
│   ├── UI/
│   │   └── game_view.cpp      # 游戏主循环、渲染与输入处理
│   └── XMLUI/                 # borealis XML UI 页面实现
├── resources/                 # 资源文件（字体、图标、XML 布局等）
└── third_party/
    ├── borealis/              # UI 框架
    └── mgba/                  # mGBA 模拟器核心（libretro 接口）
```

---

## 许可证

本项目以 [LICENSE](LICENSE) 文件中声明的许可证发布。所使用的第三方库（borealis、mGBA）各自遵循其原始许可证。

