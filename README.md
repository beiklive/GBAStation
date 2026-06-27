# GBAStation

基于 [borealis](https://github.com/xfangfang/borealis) UI 框架构建的跨平台模拟器前端，整合 libretro 与 melonDS 核心，当前支持 GB、GBC、GBA、FC、SFC、NDS 等机型，可运行于 Nintendo Switch 以及桌面平台。

![](resources/img/mgba.png)

![](resources/img/borealis_96.png)


---

## 功能特性

### 支持机型与核心

| 机型 | 核心 |
|------|------|
| **GB / GBC / GBA** | mGBA |
| **FC** | Nestopia、FCEUmm |
| **SFC** | Snes9x 2005、Snes9x |
| **NDS** | melonDS |

### 主要功能

| 功能 | 说明 |
|------|------|
| **游戏库管理** | 自动入库、最近游玩、收藏、分类筛选、搜索、拼音排序、批量删除 |
| **游戏导入** | 支持目录扫描、RetroArch `lpl` 导入、移除无效游戏、清空游戏库 |
| **Web 管理** | 可在同一局域网中上传 ROM、导入存档、修改封面并管理游戏库 |
| **多核心切换** | FC / SFC 可在游戏主页或游戏库中切换不同核心 |
| **画面模式** | Fit / Fill / Original / 4:3 / Integer / Custom |
| **NDS 画面设置** | 支持双屏布局、屏幕旋转、上下屏单独调整与内部分辨率切换 |
| **遮罩** | 支持按游戏配置 PNG 遮罩 |
| **RetroArch 着色器** | 支持 GLSL 着色器预设（`.glslp`）及参数调整，并可同步到同平台游戏 |
| **快进 / 倒带** | 支持倍率、静音、按住或切换模式，倒带可配置缓冲帧数与步长 |
| **即时存档 / 自动存档** | 多槽位即时存档、自动读取、退出自动保存，并保存缩略图 |
| **电池存档与 RTC** | 支持 SRAM 自动保存恢复，以及 RTC 相关游戏数据持久化 |
| **金手指** | 支持 RetroArch 格式 `cht` 文件、启用切换与条目编辑，NDS支持usrcheat.dat |
| **按键映射** | 按机型独立配置按键，支持 A / B 连发、截图等热键 |
| **运行状态显示** | 支持 FPS、快进、倒带、静音等状态叠加显示 |
| **更新与资源** | 内置版本检测，以及 NDS 固件 / 金手指资源下载入口 |

---



## 许可证

本项目以 [LICENSE](LICENSE) 文件中声明的许可证发布。所使用的第三方库（borealis、mGBA）各自遵循其原始许可证。

## ⭐ 支持作者

如果这个项目对你有帮助，欢迎支持：

- ⭐ Star 项目
- 🧾 提交 Issue / PR
- ☕ 打赏支持


![pay](./assets/pay.png)
