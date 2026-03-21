# BeikLiveStation

基于 [libretro](https://www.libretro.com/) 核心与 [borealis](https://github.com/xfangfang/borealis) UI 框架构建的跨平台模拟器前端，当前内置 mGBA（Game Boy Advance）libretro 核心，支持 Linux、macOS、Windows 及 Nintendo Switch 平台。

---

## 目录

- [功能特性](#功能特性)
- [项目结构](#项目结构)

---

## 功能特性

| 功能 | 说明 |
|------|------|
| **libretro 核心加载** | 运行时动态加载（Linux/macOS/Windows），Switch 平台静态链接 |
| **游戏画面渲染** | OpenGL 纹理上传 + NanoVG 绘制，支持 XRGB8888 / RGB565 / RGB1555 格式 |
| **多种画面缩放模式** | Fit / Fill / Original / IntegerScale / Custom |
| **纹理过滤模式** | Nearest（像素风格）/ Linear（平滑插值） |
| **画面遮罩叠加** | 可为每款游戏单独配置 PNG 遮罩图层，叠加在游戏画面之上 |
| **跨平台音频输出** | ALSA（Linux）/ WinMM（Windows）/ CoreAudio（macOS）/ audout（Switch） |
| **手柄输入** | 通过 borealis ControllerState 统一抽象，支持自定义按键映射 |
| **热键系统** | 手柄键可绑定模拟器功能键 |
| **快进（Fast-Forward）** | 可配置倍率、静音、按住或切换模式 |
| **倒带（Rewind）** | 可选功能，支持配置缓冲帧数、倒带步长及静音 |
| **快速存档 / 读档** | 多存档槽位，热键触发即时保存与读取游戏状态 |
| **SRAM 电池存档** | 游戏退出时自动保存电池存档，下次加载时自动恢复 |
| **GB MBC3 RTC 存档** | Game Boy MBC3 实时时钟数据随 SRAM 一同持久化存储 |
| **金手指（.cht 文件）** | 启动游戏时自动加载 RetroArch 格式 .cht 文件并应用金手指代码 |
| **FPS 显示** | 可在画面上叠加实时帧率 |
| **状态叠加层** | 快进和倒带时显示状态提示；存档操作后短暂显示提示信息 |
| **后处理着色器** | 支持 RetroArch GLSL 着色器预设（.glslp），含多通道渲染链、外部纹理与历史帧引用 |
| **游戏库管理** | 扫描并管理本地 ROM 收藏，支持封面图、自定义名称及游玩数据记录 |
| **设置 UI** | 内置图形化设置界面，涵盖画面、音频、游戏、按键绑定及调试选项卡 |
| **配置文件管理** | 基于 ConfigManager，所有参数持久化存储，首次运行自动生成默认配置 |

---

## 项目结构

```
BeikLiveStation/
├── main.cpp                    # 程序入口
├── CMakeLists.txt              # 主构建配置
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
│   │   └── RenderChain.hpp    # 渲染链框架接口
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

