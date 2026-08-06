# GBAStation / BeikLiveStation

GBAStation 是面向 Nintendo Switch 的多核心模拟器前端。主程序负责游戏库、文件识别、配置管理、按键映射、资源更新与链式启动，不再把各模拟器核心作为 libretro 动态核心内嵌到同一个进程中运行。

当前架构采用“主程序 + 独立核心 NRO”的方式：主程序识别 ROM 类型并启动对应的 `GBAStation*Stub.nro`，核心退出后再返回 `sdmc:/switch/GBAStation.nro`。这样可以让 3DS、街机、DC、PSP 等大型核心保持各自独立的构建、依赖和渲染后端。

## 支持平台

| 平台 | 独立核心 |
|------|----------|
| NDS | `GBAStationNDSStub.nro` |
| 3DS | `GBAStation3DSStub.nro` |
| 街机 | `GBAStationFBNeoStub.nro` |
| Dreamcast | `GBAStationFlycastStub.nro` |
| PSP | `GBAStationPPSSPPStub.nro` |

主程序保留文件打开、平台识别、游戏入库、核心选择和启动参数组织逻辑。大型核心的菜单、即时设置、按键映射与运行时功能由各自的 Stub 负责实现。

## 主要功能

| 功能 | 说明 |
|------|------|
| 游戏库管理 | 自动扫描、最近游玩、收藏、搜索、分类筛选、拼音排序、批量删除 |
| 文件识别 | 根据扩展名和平台规则识别 NDS、3DS、街机、DC、PSP 游戏 |
| 链式调用 | 从主程序启动独立核心 NRO，并在核心退出后返回主程序 |
| 外部核心配置 | 可在设置页修改各平台核心路径、返回路径、核心选项与按键映射 |
| GameDB 更新 | 支持游戏数据库更新、封面和元数据维护 |
| Web 管理 | 局域网内上传 ROM、导入存档、修改封面并管理游戏库 |
| 按键映射 | 按平台独立配置，支持单键、多键组合和多组映射 |
| 运行时功能 | 快进、即时存档、读取存档、金手指、画面设置、核心设置等由对应核心菜单提供 |
| 更新发布 | Release 包同时包含主程序和所有核心 NRO |
| 调试日志 | Switch 运行日志统一写入 `sdmc:/GBAStation/debug` |

## SD 卡目录

Release 包解压后应保持以下结构：

```text
sdmc:/switch/GBAStation.nro
sdmc:/GBAStation/core/GBAStationNDSStub.nro
sdmc:/GBAStation/core/GBAStation3DSStub.nro
sdmc:/GBAStation/core/GBAStationFBNeoStub.nro
sdmc:/GBAStation/core/GBAStationFlycastStub.nro
sdmc:/GBAStation/core/GBAStationPPSSPPStub.nro
sdmc:/GBAStation/config/config.cfg
sdmc:/GBAStation/config/cores/
sdmc:/GBAStation/debug/
```

各核心会在 `sdmc:/GBAStation/config/cores/` 下保存自己的核心设置。主程序的按键映射、外部核心路径、显示和功能设置保存在 `sdmc:/GBAStation/config/config.cfg`。

## 构建

### Nintendo Switch

需要 devkitPro / devkitA64 环境：

```bash
cd BeikLiveStation
bash switchbuild.sh
```

本地构建默认从相邻项目目录复制外部核心：

```text
../.example/dekopon/build/switch-codex/src/citra_switch/dekopon.nro
../GBAStation_fbneo/GBAStationFBNeoStub.nro
../GBAStation_flycast/GBAStationFlycastStub.nro
../GBAStation_ppsspp/GBAStationPPSSPPStub.nro
```

也可以显式传入已经准备好的核心目录：

```bash
bash switchbuild.sh --core-dir /path/to/external-cores -j 8
```

`--core-dir` 目录必须包含：

```text
GBAStation3DSStub.nro
GBAStationFBNeoStub.nro
GBAStationFlycastStub.nro
GBAStationPPSSPPStub.nro
```

构建产物位于：

```text
build_switch/GBAStation.nro
build_switch/GBAStation/core/GBAStationNDSStub.nro
build_switch/GBAStation/core/GBAStation3DSStub.nro
build_switch/GBAStation/core/GBAStationFBNeoStub.nro
build_switch/GBAStation/core/GBAStationFlycastStub.nro
build_switch/GBAStation/core/GBAStationPPSSPPStub.nro
```

### Windows

桌面版本用于前端开发和资源调试：

```bat
cd BeikLiveStation
windowsbuild.bat
```

## CI 发布流程

推送 `v*` tag 会触发 Switch Release 构建：

1. 在 CI 中安装 Switch 构建依赖。
2. 从各核心仓库的最新 GitHub Release 下载 NRO。
3. 本地构建主程序和 `GBAStationNDSStub.nro`。
4. 将主程序和五个核心打包到 `GBAStation.zip`。
5. 上传 `GBAStation.zip`、`SHA256SUMS.txt` 和 `CORE-RELEASES.txt`。
6. 自动创建 GitHub Release。

外部核心来源：

| 仓库 | Release 资产 |
|------|--------------|
| `beiklive/GBAStation_3DS` | `GBAStation3DSStub.nro` |
| `beiklive/GBAStation_FBNeo` | `GBAStationFBNeoStub.nro` |
| `beiklive/GBAStation_flycast` | `GBAStationFlycastStub.nro` |
| `beiklive/GBAStation_ppsspp` | `GBAStationPPSSPPStub.nro` |

CI 不使用核心清单，也不要求主程序 tag 与核心 tag 相同；主程序每次发布都会抓取对应核心仓库的最新 Release。

## 许可证

本项目以 [LICENSE](LICENSE) 文件中声明的许可证发布。主程序、独立核心、渲染后端和第三方依赖分别遵循各自的开源许可证。

## 支持作者

如果这个项目对你有帮助，欢迎 Star 项目、提交 Issue / PR，或通过下方二维码支持开发。

![pay](./assets/pay.png)
