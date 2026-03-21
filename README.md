# BeikLiveStation

基于 [libretro](https://www.libretro.com/) 核心与 [borealis](https://github.com/xfangfang/borealis) UI 框架构建的跨平台模拟器前端，当前内置 mGBA（Game Boy Advance）libretro 核心，支持 Nintendo Switch 平台。

![](resources/img/mgba.png)

![](resources/img/borealis_96.png)


---

## 功能特性

| 功能 | 说明 |
|------|------|
| **多种画面缩放模式** | Fit / Fill / Original / IntegerScale / Custom |
| **纹理过滤模式** | Nearest（像素风格）/ Linear（平滑插值） |
| **画面遮罩** | 可为每款游戏单独配置 PNG 遮罩图层 |
| **快进（Fast-Forward）** | 可配置倍率、静音、按住或切换模式 |
| **倒带（Rewind）** | 可选功能，支持配置缓冲帧数、倒带步长及静音 |
| **快速存档 / 读档** | 多存档槽位，热键触发即时保存与读取游戏状态 |
| **SRAM 电池存档** | 游戏退出时自动保存电池存档，下次加载时自动恢复 |
| **金手指（.cht 文件）** | 启动游戏时自动加载 RetroArch 格式 .cht 文件并应用金手指代码 |
| **FPS 显示** | 可在画面上叠加实时帧率 |
| **着色器** | 支持 RetroArch GLSL 着色器预设（.glslp），可自由配置参数|
| **设置 UI** | 内置图形化设置界面，涵盖画面、音频、游戏、按键绑定及调试选项卡 |

---



## 许可证

本项目以 [LICENSE](LICENSE) 文件中声明的许可证发布。所使用的第三方库（borealis、mGBA）各自遵循其原始许可证。

## ⭐ 支持作者

如果这个项目对你有帮助，欢迎支持：

- ⭐ Star 项目
- 🧾 提交 Issue / PR
- ☕ 打赏支持


![pay](./assets/pay.png)