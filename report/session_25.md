# 音效系统修复工作报告

**日期**: 2026-03-10  
**任务**: 修复 borealis 内置音效失效问题，完善 AudioManager 系统，添加桌面平台音效播放接口

---

## 问题分析

经代码检查，发现以下两处导致音效无法播放的根本原因：

### 问题 1：Nintendo Switch 平台 —— 播放代码被注释

文件：`third_party/borealis/library/lib/platforms/switch/switch_audio.cpp`

`SwitchAudioPlayer::play()` 函数中，实际调用 `plsrPlayerPlay()` 的代码被全部注释掉，函数直接返回 `true` 而不产生任何声音：

```cpp
// 修复前：全部播放逻辑均被 // 注释
bool SwitchAudioPlayer::play(enum Sound sound, float pitch) {
    if (!this->init) return false;
    if (sound == SOUND_NONE) return true;
    // ... 13 行播放代码被注释 ...
    return true;  // 什么都没发生
}
```

**修复**：恢复注释掉的播放代码，使 Switch 平台能够正常通过 libpulsar 播放来自 qlaunch.bfsar 的系统音效。

---

### 问题 2：桌面平台 —— 使用空实现 `NullAudioPlayer`

borealis 的 GLFW 平台（桌面默认）和 SDL 平台均使用 `NullAudioPlayer`，其 `load()` 和 `play()` 方法直接返回 `false`，没有任何音效输出。

---

## 修复内容

### 1. 修复 Nintendo Switch 音效播放

**文件**：`third_party/borealis/library/lib/platforms/switch/switch_audio.cpp`

恢复了 `play()` 函数中被注释的播放逻辑：
- 按需通过 `load()` 加载音效
- 调用 `plsrPlayerSetPitch()` 设置音调
- 调用 `plsrPlayerPlay()` 播放音效
- 错误时记录日志并返回 `false`

---

### 2. 为 borealis Application 添加自定义音频播放器注入接口

**文件**：
- `third_party/borealis/library/include/borealis/core/application.hpp`
- `third_party/borealis/library/lib/core/application.cpp`

新增 `Application::setAudioPlayer(AudioPlayer* player)` 静态方法：
- 允许上层应用（BeikLiveStation）替换 borealis 默认的 `NullAudioPlayer`
- 设置后自动预加载常用音效（focus_change、click、back 等）
- `getAudioPlayer()` 优先返回自定义播放器；未设置时退回平台默认实现

---

### 3. 新建 `BKAudioPlayer`（桌面平台音效播放器）

**文件**：
- `include/Audio/BKAudioPlayer.hpp`
- `src/Audio/BKAudioPlayer.cpp`

功能特性：
- 继承 `brls::AudioPlayer`，实现 `load()` 和 `play()` 接口
- 启动时从 `resources/sounds/` 目录加载 WAV 文件
- 支持三大桌面平台原生音频 API：
  - **Linux**：ALSA（`snd_pcm_open` / `snd_pcm_writei`）
  - **Windows**：WinMM（构建内存 WAV 文件后调用 `PlaySoundA`）
  - **macOS**：CoreAudio（`AudioUnit` + 渲染回调）
  - **无音频环境**：静默失败
- 后台播放线程，"最新优先"队列（若有未播放的待播音效，新音效会替换旧音效）
- 文件缺失时静默跳过，不影响程序运行

**音效文件映射**：

| 枚举值                  | 文件名               |
|------------------------|---------------------|
| `SOUND_FOCUS_CHANGE`   | `focus_change.wav`  |
| `SOUND_FOCUS_ERROR`    | `focus_error.wav`   |
| `SOUND_CLICK`          | `click.wav`         |
| `SOUND_BACK`           | `back.wav`          |
| `SOUND_FOCUS_SIDEBAR`  | `focus_sidebar.wav` |
| `SOUND_CLICK_ERROR`    | `click_error.wav`   |
| `SOUND_HONK`           | `honk.wav`          |
| `SOUND_CLICK_SIDEBAR`  | `click_sidebar.wav` |
| `SOUND_TOUCH_UNFOCUS`  | `touch_unfocus.wav` |
| `SOUND_TOUCH`          | `touch.wav`         |
| `SOUND_SLIDER_TICK`    | `slider_tick.wav`   |
| `SOUND_SLIDER_RELEASE` | `slider_release.wav`|

---

### 4. 在 `main.cpp` 中初始化并注入音效播放器

程序启动时（`createWindow()` 后）在非 Switch 平台上自动：
1. 创建 `BKAudioPlayer` 实例
2. 通过 `Application::setAudioPlayer()` 接管 borealis 音效系统
3. 音效文件在此时预加载

---

### 5. 新建 `resources/sounds/` 目录

存放桌面平台 WAV 音效文件，包含 `README.md` 说明所需文件格式和列表。

---

## 缺少的音效文件

以下 WAV 文件需要由开发者提供并放置到 `resources/sounds/` 目录：

| 文件名               | 用途                     |
|---------------------|--------------------------|
| `focus_change.wav`  | 焦点移动到其他控件时       |
| `focus_error.wav`   | 无法移动焦点时（高亮抖动） |
| `click.wav`         | 按钮点击确认               |
| `back.wav`          | 返回/取消                  |
| `focus_sidebar.wav` | 焦点移动到侧边栏           |
| `click_error.wav`   | 点击禁用按钮               |
| `honk.wav`          | 特殊提示音                 |
| `click_sidebar.wav` | 侧边栏点击确认             |
| `touch_unfocus.wav` | 触摸焦点中断               |
| `touch.wav`         | 通用触摸                   |
| `slider_tick.wav`   | 滑块每步变化               |
| `slider_release.wav`| 滑块松开                   |

**文件要求**：PCM 16-bit，单声道或立体声，44100 Hz 或 48000 Hz 采样率。

> 注意：文件缺失时 `BKAudioPlayer` 会静默跳过，不影响程序运行。

---

## 修改的文件列表

| 文件 | 操作 | 说明 |
|------|------|------|
| `third_party/borealis/library/lib/platforms/switch/switch_audio.cpp` | 修改 | 恢复被注释的 Switch 音效播放代码 |
| `third_party/borealis/library/include/borealis/core/application.hpp` | 修改 | 新增 `setAudioPlayer()` 声明和 `customAudioPlayer` 成员 |
| `third_party/borealis/library/lib/core/application.cpp` | 修改 | 实现 `setAudioPlayer()`，修改 `getAudioPlayer()` 支持自定义播放器 |
| `include/Audio/BKAudioPlayer.hpp` | 新建 | 桌面音效播放器头文件 |
| `src/Audio/BKAudioPlayer.cpp` | 新建 | 桌面音效播放器实现（ALSA/WinMM/CoreAudio） |
| `main.cpp` | 修改 | 初始化并注入 `BKAudioPlayer` |
| `resources/sounds/README.md` | 新建 | 音效文件说明文档 |
