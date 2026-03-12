# BeikLiveStation 第4轮修复报告

## 修复概览

本次修复针对以下5个问题：

1. 常态帧率不稳定（PC 约50帧，Switch 约40帧）
2. 快进时 FPS 显示没有升高
3. 键盘模式下没有退出按键
4. 手柄配置文件中配置值为整数，可读性低
5. 快进提示缺失，倒带/快进提示不可通过配置控制

---

## 修改文件

| 文件 | 修改内容 |
|------|---------|
| `include/UI/game_view.hpp` | 新增音频线程成员、键盘退出标志、快进/倒带覆盖显示标志、`<condition_variable>` 引用 |
| `src/UI/game_view.cpp` | 实现所有修复（见下方各节） |
| `report/fix_report_round4.md` | 本报告（新增） |

---

## 问题1：帧率不稳定（PC≈50fps，Switch≈40fps）

### 根本原因

原始实现中，游戏线程直接调用 `AudioManager::pushSamples()`。该函数在音频环形缓冲区满时会**阻塞等待**（通过条件变量 `m_spaceCV`）。当音频输出存在抖动或稍有延迟时，此阻塞会打断游戏线程的精确帧率控制（混合睡眠方案），导致帧率不稳定。

### 修复方案

**新增专用音频推送线程**（`m_audioThread`）：

- 游戏线程将音频样本以非阻塞方式存入队列 `m_audioQueue`（互斥锁 + `std::deque`）
- 专用音频线程从队列取出样本并调用 `AudioManager::pushSamples()`（在此处可阻塞，不影响游戏线程）
- 队列设置最大深度（8批次），防止因音频线程滞后导致无限增长；超出时丢弃最旧批次

**清理顺序**：
```
AudioManager::deinit()  →  停止音频线程（通知条件变量，join）  →  停止游戏线程（join）
```

**效果**：游戏线程的帧率控制不再受音频输出抖动影响，应能稳定达到60fps。

---

## 问题2：快进时 FPS 显示没有升高

### 根本原因

原始 FPS 计数器每次迭代固定 `++fpsCounter`，而快进时每次迭代实际运行了 `round(multiplier)` 帧（如4帧），导致显示 FPS 仍为约60，而非实际的约240。

### 修复方案

统一使用 `framesThisIter` 变量跟踪每次迭代实际运行的帧数：

```cpp
unsigned framesThisIter = 1u;  // 倒带和普通模式
if (ff) {
    framesThisIter = (m_ffMultiplier >= 1.0f)
        ? static_cast<unsigned>(std::round(m_ffMultiplier))
        : 1u;
}
// ... 执行逻辑 ...
fpsCounter += framesThisIter;  // 统一在分支外累加
```

**效果**：以4倍速快进时，FPS 显示约为 240（4 × 60）。

---

## 问题3：键盘模式下没有退出按键

### 修复方案

新增 `keyboard.exit` 配置项（默认 `ESC`）：

1. `initialize()` 中读取 `keyboard.exit` 配置，存入 `m_kbExitKey`（默认 `BRLS_KBD_KEY_ESCAPE`）
2. `pollInput()` 中（游戏线程）检测退出键，置位原子标志 `m_requestExit`
3. `draw()` 中（主线程）检测 `m_requestExit`，调用 `stopGameThread()` 后执行 `brls::Application::popActivity()`

此方案避免了跨线程直接调用 UI 函数。

### 配置项

| 配置键 | 说明 | 默认值 |
|--------|------|--------|
| `keyboard.exit` | 键盘模式退出游戏的按键 | `ESC` |

---

## 问题4：手柄配置值可读性低

### 修复方案

新增按键名称查表 `k_brlsBtnNames[]` 和解析函数 `parseBrlsButton()`，支持用可读名称配置手柄按键：

- 修改前：`handle.a = 13`（需了解 `brls::BUTTON_A` 的枚举整数值）
- 修改后：`handle.a = A`（直接使用按键名称）

`parseBrlsButton()` 同时支持：
- 命名字符串（大小写不敏感）：`"A"`, `"LB"`, `"RT"`, `"START"` 等
- 整数回退（兼容旧配置文件）：`"13"`, `"0"` 等

### 支持的按键名称

| 名称 | 对应按键 | 说明 |
|------|---------|------|
| `LT` | BUTTON_LT | ZL / L2 |
| `LB` | BUTTON_LB | L |
| `LSB` | BUTTON_LSB | 左摇杆按键 |
| `UP/DOWN/LEFT/RIGHT` | 方向键 | |
| `BACK` | BUTTON_BACK | Select / - |
| `GUIDE` | BUTTON_GUIDE | Home |
| `START` | BUTTON_START | Start / + |
| `RSB` | BUTTON_RSB | 右摇杆按键 |
| `Y/B/A/X` | 面部按键 | |
| `RB` | BUTTON_RB | R |
| `RT` | BUTTON_RT | ZR / R2 |

---

## 问题5：快进提示缺失 + 覆盖显示不可配置

### 修复方案

1. **新增快进覆盖提示**：快进激活时在屏幕右上角显示青色 `>> FF` 提示框
2. **倒带覆盖可配置**：原倒带提示 `<<< REWIND` 受 `display.showRewindOverlay` 控制
3. **快进覆盖可配置**：新快进提示受 `display.showFfOverlay` 控制

### 新增配置项

| 配置键 | 说明 | 默认值 |
|--------|------|--------|
| `display.showFfOverlay` | 快进时是否显示提示文字 | `true` |
| `display.showRewindOverlay` | 倒带时是否显示提示文字 | `true` |

---

## 完整配置文件示例

```ini
; ===== 显示配置 =====
display.showFps          = true
display.showFfOverlay    = true
display.showRewindOverlay= true

; ===== 快进配置 =====
fastforward.multiplier = 4.0
fastforward.mute       = true
fastforward.mode       = hold

; ===== 倒带配置 =====
rewind.enabled    = false
rewind.bufferSize = 600
rewind.step       = 2
rewind.mute       = false
rewind.mode       = hold

; ===== 手柄按键映射（使用可读名称）=====
handle.a           = A
handle.b           = B
handle.x           = X
handle.y           = Y
handle.up          = UP
handle.down        = DOWN
handle.left        = LEFT
handle.right       = RIGHT
handle.start       = START
handle.select      = BACK
handle.l           = LB
handle.r           = RB
handle.l2          = LT
handle.r2          = RT
handle.fastforward = RT
handle.rewind      = LT

; ===== 键盘按键映射 =====
keyboard.a           = X
keyboard.b           = Z
keyboard.x           = C
keyboard.y           = A
keyboard.up          = UP
keyboard.down        = DOWN
keyboard.left        = LEFT
keyboard.right       = RIGHT
keyboard.start       = ENTER
keyboard.select      = S
keyboard.l           = Q
keyboard.r           = W
keyboard.l2          = E
keyboard.r2          = R
keyboard.fastforward = TAB
keyboard.rewind      = GRAVE
keyboard.exit        = ESC
```

---

## 架构说明

### 线程模型（修复后）

```
主线程（渲染）:  draw() → 上传纹理 → 绘制覆盖层 → 检测退出请求
游戏线程:       pollInput() → retro_run() × N → 样本入队 → 保存状态 → 帧率控制睡眠
音频线程（新）: 样本出队 → AudioManager::pushSamples()（可阻塞，不影响游戏线程）
音频后台线程:   AudioManager 内部线程 → 平台音频输出
```

游戏线程与音频输出完全解耦，帧率控制仅依赖系统定时器，实现稳定60fps。
