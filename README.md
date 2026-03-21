# BeikLiveStation

基于 [libretro](https://www.libretro.com/) 核心与 [borealis](https://github.com/xfangfang/borealis) UI 框架构建的跨平台模拟器前端，当前内置 mGBA（Game Boy Advance）libretro 核心，支持 Linux、macOS、Windows 及 Nintendo Switch 平台。

---

## 目录

- [功能概览](#功能概览)
- [待实现功能](#待实现功能)
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
| **游戏画面渲染** | OpenGL 纹理上传 + NanoVG 绘制，支持 XRGB8888 / RGB565 / RGB1555 格式 |
| **多种画面缩放模式** | Fit / Fill / Original / IntegerScale / Custom |
| **纹理过滤模式** | Nearest（像素风格）/ Linear（平滑插值） |
| **画面遮罩叠加** | 可为每款游戏单独配置 PNG 遮罩图层，叠加在游戏画面之上 |
| **跨平台音频输出** | ALSA（Linux）/ WinMM（Windows）/ CoreAudio（macOS）/ audout（Switch） |
| **手柄输入** | 通过 borealis ControllerState 统一抽象，支持自定义按键映射 |
| **键盘输入** | 自动检测键盘模式，支持字符串名称或数值方式配置 |
| **热键系统** | 键盘组合键（支持 Ctrl/Shift/Alt 修饰符）与手柄键均可绑定模拟器功能键 |
| **快进（Fast-Forward）** | 可配置倍率、静音、按住或切换模式 |
| **倒带（Rewind）** | 可选功能，支持配置缓冲帧数、倒带步长及静音 |
| **快速存档 / 读档** | 多存档槽位，热键触发即时保存与读取游戏状态 |
| **SRAM 电池存档** | 游戏退出时自动保存电池存档，下次加载时自动恢复 |
| **GB MBC3 RTC 存档** | Game Boy MBC3 实时时钟数据随 SRAM 一同持久化存储 |
| **金手指（.cht 文件）** | 启动游戏时自动加载 RetroArch 格式 .cht 文件并应用金手指代码 |
| **FPS 显示** | 可在画面上叠加实时帧率 |
| **状态叠加层** | 快进和倒带时显示状态提示；存档操作后短暂显示提示信息 |
| **渲染链框架** | RenderChain 占位框架已就位，供后续接入后处理着色器；当前为直通模式，不做额外处理 |
| **设置 UI** | 内置图形化设置界面，涵盖画面、音频、游戏、按键绑定及调试选项卡 |
| **配置文件管理** | 基于 ConfigManager，所有参数持久化存储，首次运行自动生成默认配置 |

---

## 待实现功能

以下功能已规划或部分搭建基础结构，尚未完整实现：

| 功能 | 当前状态 | 说明 |
|------|----------|------|
| **游戏时间同步** | 🚧 未实现 | GB MBC3 RTC 已可从磁盘恢复，但尚不支持将游戏内时钟与主机系统时间自动同步 |
| **金手指菜单 UI** | 🚧 未实现 | 从 .cht 文件加载金手指（作弊码）已支持，但游戏运行中的金手指浏览与编辑界面尚未完成 |
| **后处理着色器** | 🚧 未实现 | RenderChain 框架已就位（直通模式），OpenGL 着色器通道尚未实现 |
| **暂停 / 截屏 / 静音热键** | 🚧 未实现 | 热键配置项已定义（`hotkey.pause`、`hotkey.screenshot`、`hotkey.mute`），对应功能逻辑尚待接入 |

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
├── GBAStation            # 主程序
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
├── GBAStation            # 主程序
└── mgba_libretro.dylib  # libretro 核心
```

### Windows（MSYS2 MinGW64 终端）

```bat
windowsbuild.bat
```

产物目录：`build_windows\`

```
build_windows\
├── GBAStation.exe        # 主程序
└── mgba_libretro.dll    # libretro 核心
```

### Nintendo Switch

```bash
./switchbuild.sh
```

产物：`build_switch/GBAStation.nro`

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
| `handle.fastforward` | `RT` | 触发快进（保持模式）的手柄按键 |
| `handle.rewind` | `LT` | 触发倒带（保持模式）的手柄按键 |
| `handle.<retro_button>` | — | 将手柄按键映射至 libretro 按键（如 `handle.a = A`） |

支持的按键名称：`A`、`B`、`X`、`Y`、`LB`、`RB`、`LT`、`RT`、`START`、`BACK`、`UP`、`DOWN`、`LEFT`、`RIGHT` 等。

### 热键（`hotkey.*`）

模拟器功能热键均支持键盘（`hotkey.<name>.kbd`）和手柄（`hotkey.<name>.pad`）两种绑定方式。值为 `none` 表示未绑定。

> **注意**：快进保持模式和倒带保持模式沿用旧版配置键（`keyboard.fastforward` / `handle.fastforward` 等）以保持向后兼容；其余功能统一采用 `hotkey.*` 格式。

| 热键配置键 | 默认键盘 | 默认手柄 | 说明 |
|-----------|---------|---------|------|
| `hotkey.fastforward_toggle.kbd/pad` | `none` | `none` | 快进（切换模式） |
| `keyboard.fastforward` / `handle.fastforward` | `TAB` | `RT` | 快进（保持模式，兼容旧配置键） |
| `hotkey.rewind_toggle.kbd/pad` | `none` | `none` | 倒带（切换模式） |
| `keyboard.rewind` / `handle.rewind` | `GRAVE` | `LT` | 倒带（保持模式，兼容旧配置键） |
| `hotkey.quicksave.kbd/pad` | `F5` | `none` | 快速保存 |
| `hotkey.quickload.kbd/pad` | `F8` | `none` | 快速读取 |
| `keyboard.exit` / `hotkey.exit_game.pad` | `ESC` | `none` | 退出游戏 |

键盘绑定支持修饰符前缀，如 `CTRL+F5`、`SHIFT+Z`。

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
│   │   ├── AudioManager.hpp   # 跨平台音频管理器接口
│   │   └── BKAudioPlayer.hpp  # 音频播放器封装接口
│   ├── Control/
│   │   ├── GameInputController.hpp  # 游戏手柄热键控制器
│   │   └── InputMapping.hpp         # 按键映射与热键配置
│   ├── Game/
│   │   └── game_view.hpp      # 游戏视图（渲染 + 输入 + 游戏线程）
│   ├── Retro/
│   │   └── LibretroLoader.hpp # libretro 动态库加载器接口
│   ├── UI/
│   │   ├── Pages/             # 各 UI 页面头文件
│   │   ├── Utils/             # UI 工具类头文件
│   │   └── StartPageView.hpp  # 启动页视图接口
│   ├── Video/
│   │   ├── DisplayConfig.hpp  # 画面缩放与过滤配置
│   │   └── RenderChain.hpp    # 渲染链框架接口（当前为直通占位符）
│   └── Utils/
│       ├── ConfigManager.hpp  # 配置文件读写管理
│       ├── fileUtils.hpp      # 文件路径工具
│       └── strUtils.hpp       # 字符串工具
├── src/
│   ├── Audio/
│   │   ├── AudioManager.cpp   # 多平台音频后端实现
│   │   └── BKAudioPlayer.cpp  # 音频播放器实现
│   ├── Control/
│   │   ├── GameInputController.cpp  # 游戏手柄热键控制器实现
│   │   └── InputMapping.cpp         # 按键映射与热键配置实现
│   ├── Game/
│   │   └── game_view.cpp      # 游戏主循环、渲染与输入处理
│   ├── Retro/
│   │   └── LibretroLoader.cpp # 跨平台动态加载实现
│   ├── UI/
│   │   ├── Pages/             # 各 UI 页面实现
│   │   └── Utils/             # UI 工具类实现
│   ├── Video/
│   │   ├── DisplayConfig.cpp  # 画面配置实现
│   │   └── RenderChain.cpp    # 渲染链实现
│   └── Utils/
│       ├── ConfigManager.cpp  # 配置文件实现
│       ├── fileUtils.cpp      # 文件路径工具实现
│       └── strUtils.cpp       # 字符串工具实现
├── resources/                 # 资源文件（字体、图标、XML 布局等）
└── third_party/
    ├── borealis/              # UI 框架
    └── mgba/                  # mGBA 模拟器核心（libretro 接口）
```

---

## 许可证

本项目以 [LICENSE](LICENSE) 文件中声明的许可证发布。所使用的第三方库（borealis、mGBA）各自遵循其原始许可证。

