# GBAStation

![logo](./resources/icon/default.png)

GBAStation 是面向 Nintendo Switch 的多核心模拟器前端。主程序负责游戏库、文件识别、配置管理、按键映射、资源更新与链式启动，不再依赖 libretro 动态核心加载。

当前架构采用“小型核心主程序内置 + 大型核心独立 NRO”的方式：FC、SFC、MD、GBA、GB、GBC 等核心直接集成在主程序中；NDS、3DS、街机、DC、PSP 等核心以 `GBAStation*Stub.nro` 形式独立运行，核心退出后再返回 `sdmc:/switch/GBAStation.nro`。

这种拆分可以让轻量平台减少启动开销并继续使用 glslp 渲染链，也能让 3DS、DC、PSP 等大型核心保持各自独立的构建、依赖和渲染后端。

## 支持平台

| 平台 | 运行方式 |
|------|----------|
| GB / GBC / GBA | 主程序内置核心 |
| FC | 主程序内置核心 |
| SFC | 主程序内置核心 |
| MD | 主程序内置核心 |
| NDS | 独立项目的 `GBAStationNDS.nro` |
| 3DS | `GBAStation3DSStub.nro` |
| 街机 | `GBAStationFBNeoStub.nro` |
| Dreamcast | `GBAStationFlycastStub.nro` |
| PSP | `GBAStationPPSSPPStub.nro` |

主程序保留文件打开、平台识别、游戏入库、核心选择和启动参数组织逻辑。独立核心的菜单、即时设置、按键映射与运行时功能由各自的 Stub 负责实现。

## 主要功能

| 功能 | 说明 |
|------|------|
| 游戏库管理 | 自动扫描、最近游玩、收藏、搜索、分类筛选、拼音排序、批量删除 |
| 文件识别 | 根据扩展名和平台规则识别各平台游戏 |
| 链式调用 | 从主程序启动独立核心 NRO，并在核心退出后返回主程序 |
| 外部核心配置 | 可在设置页修改各平台核心路径、返回路径、核心选项与按键映射 |
| GameDB 更新 | 支持游戏数据库更新、封面和元数据维护 |
| Web 管理 | 局域网内上传 ROM、导入存档、修改封面并管理游戏库 |
| 按键映射 | 按平台独立配置，支持单键、多键组合和多组映射 |
| 运行时功能 | 快进、即时存档、读取存档、金手指、画面设置、核心设置等由对应核心菜单提供 |
| glslp 着色器 | 内置小核心支持 glslp 渲染链和着色器参数 |
| 更新发布 | Release 包同时包含主程序和所有核心 NRO |

## SD 卡目录

Release 包解压后应保持以下结构：

```text
sdmc:/switch/GBAStation.nro
sdmc:/GBAStation/core/GBAStationNDS.nro
sdmc:/GBAStation/core/GBAStation3DSStub.nro
sdmc:/GBAStation/core/GBAStationFBNeoStub.nro
sdmc:/GBAStation/core/GBAStationFlycastStub.nro
sdmc:/GBAStation/core/GBAStationPPSSPPStub.nro
```

## 构建

### Nintendo Switch

需要 devkitPro / devkitA64 环境和 CMake 3.10 或更高版本。macOS 可执行 `brew install cmake` 安装 CMake：

```bash
cd BeikLiveStation
./switchbuild.sh
```

本地构建默认从相邻项目目录复制外部核心：

```text
../GBAStation_fbneo/GBAStationFBNeoStub.nro
../GBAStation_flycast/GBAStationFlycastStub.nro
../GBAStation_ppsspp/GBAStationPPSSPPStub.nro
../GBAStation_3DS/GBAStation3DSStub.nro
```

NDS 核心由相邻的 `GBAStation_melonds` 独立构建；将其
`build_switch/GBAStationNDS.nro` 放入 `sdmc:/GBAStation/core/`。

构建产物位于：

```text
build_switch/GBAStation.nro
build_switch/GBAStation/core/GBAStation3DSStub.nro
build_switch/GBAStation/core/GBAStationFBNeoStub.nro
build_switch/GBAStation/core/GBAStationFlycastStub.nro
build_switch/GBAStation/core/GBAStationPPSSPPStub.nro
```

### Windows

桌面版本用于前端开发和资源调试，暂时无法运行外置核心：

```bat
cd BeikLiveStation
windowsbuild.bat
```

## 许可证

本项目以 [LICENSE](LICENSE) 文件中声明的许可证发布。主程序、独立核心、渲染后端和第三方依赖分别遵循各自的开源许可证。

## 支持作者

如果这个项目对你有帮助，欢迎 Star 项目、提交 Issue / PR，或通过下方二维码支持开发。

![pay](./assets/pay.png)



## 模拟器截图展示

![alt text](./assets/1.png)
![alt text](./assets/2.png)
![alt text](./assets/3.png)
![alt text](./assets/4.png)
