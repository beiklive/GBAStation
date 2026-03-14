# Session 62 工作汇报

## 任务说明

Issue #63 已合并，但 GBA/GB 游戏仍无法正确获取系统时钟。本次任务重新深入分析 mGBA 源码，定位遗漏的根因并予以修复。

---

## 问题根因分析

### 回顾 PR #63 的修复内容

PR #63 做了两件事：
1. 在 `switch_wrapper.c` 的 `userAppInit()` 末尾添加了 `timeInitialize()` / `timeExit()`
2. 在 `LibretroLoader.cpp` 中实现了 `RETRO_ENVIRONMENT_GET_LOG_INTERFACE`，将核心日志转发至 stderr

这两项修复是必要的，但**并不充分**。Issue 依然未被解决，原因如下：

---

### 根因一：`__nx_time_type` 未设置为 `TimeType_UserSystemClock`（最主要原因）

**文件**：`third_party/borealis/library/lib/platforms/switch/switch_wrapper.c`

在 libnx（Nintendo Switch 官方 homebrew SDK）中，POSIX `time()` 函数依赖一个弱符号（weak symbol）全局变量：

```c
extern TimeType __nx_time_type;
```

该变量决定 `time()` 底层使用哪个时钟源：

| 值 | 含义 |
|----|------|
| `TimeType_UserSystemClock` | 用户可调整的系统时钟（Switch 主页面显示的时钟） |
| `TimeType_NetworkSystemClock` | 网络同步 UTC 时钟 |
| `TimeType_LocalSystemClock` | 硬件本地时钟（可能未经用户调整） |

**mGBA 的独立 Switch 版**（`third_party/mgba/src/platform/switch/main.c`，第 33 行）明确设置：

```c
TimeType __nx_time_type = TimeType_UserSystemClock;
```

**BeikLiveStation 从未设置该变量**，导致 `time(0)` 使用了错误的时钟源，即使 `timeInitialize()` 已被调用。

**mGBA RTC 调用链**（GBA 版）：
```
retro_run() → GBA CPU step → GPIO RTC 访问
  → _rtcUpdateClock(hw)
    → rtc->unixTime(rtc)           // mRTCGenericSource
      → _rtcGenericCallback()
        → time(0)                   // ← 若 __nx_time_type 错误，此处返回错误时间
      → localtime_r(&t, &date)
```

**mGBA RTC 调用链**（GB/MBC3 版）：
```
gb/mbc.c: _latchRtc()
  → rtc->unixTime(rtc)
    → _rtcGenericCallback()
      → time(0)                     // ← 同上
```

**修复**：在 `switch_wrapper.c` 中添加：
```c
TimeType __nx_time_type = TimeType_UserSystemClock;
```

---

### 根因二：GB MBC3 RTC 数据未持久化（时钟在会话间重置）

**文件**：`src/Game/game_view.cpp`

对于使用 GB MBC3 RTC 的 Game Boy Color 游戏（如《宝可梦 金/银/水晶》、《塞尔达传说 神谕系列》等），mGBA 通过两个独立的 libretro 内存区域管理数据：

| libretro 内存 ID | 内容 | 大小 |
|-----------------|------|------|
| `RETRO_MEMORY_SAVE_RAM` | MBC3 SRAM（存档数据） | 可变（最大 32 KB） |
| `RETRO_MEMORY_RTC` | MBC3 RTC 寄存器 + 上次更新的 Unix 时间戳 | `sizeof(GBMBCRTCSaveBuffer)` = 44 字节 |

`GBMBCRTCSaveBuffer` 结构体（`include/mgba/internal/gb/mbc.h`）：
```c
struct GBMBCRTCSaveBuffer {
    uint32_t sec, min, hour, days, daysHi;
    uint32_t latchedSec, latchedMin, latchedHour, latchedDays, latchedDaysHi;
    uint64_t unixTime;   // 上次更新时刻的 Unix 时间戳
};
```

`unixTime` 字段极为关键：mGBA 利用它在游戏加载时计算"本次会话开始距上次保存的时间差"，从而推进 RTC 寄存器。若该字段丢失（未保存），每次加载游戏时 RTC 都将从当前系统时间重新计算，导致游戏内时间出现跳变。

**PR #63 之前的代码**只保存 `RETRO_MEMORY_SAVE_RAM`（`.sav` 文件），**完全忽略** `RETRO_MEMORY_RTC`。

**修复**：
- 新增 `rtcSavePath()` 函数，返回 `<存档目录>/<ROM名称>.rtc` 路径
- 新增 `loadRtc()` 函数，将 `.rtc` 文件内容读入 `RETRO_MEMORY_RTC` 指向的内存
- 新增 `saveRtc()` 函数，将 `RETRO_MEMORY_RTC` 内容写入 `.rtc` 文件
- 在 `loadSram()` 末尾调用 `loadRtc()`（含所有提前返回路径）
- 在 `saveSram()` 末尾调用 `saveRtc()`

---

## 修改文件汇总

| 文件 | 修改类型 | 说明 |
|------|---------|------|
| `third_party/borealis/library/lib/platforms/switch/switch_wrapper.c` | Bug 修复 | 添加 `TimeType __nx_time_type = TimeType_UserSystemClock;`，使 `time(0)` 使用用户系统时钟 |
| `include/Game/game_view.hpp` | 接口扩充 | 新增 `rtcSavePath()`、`loadRtc()`、`saveRtc()` 声明 |
| `src/Game/game_view.cpp` | 功能补全 | 实现上述三个函数；在 `loadSram()` / `saveSram()` 中调用对应 RTC 函数 |

---

## 技术细节

### 为什么 `__nx_time_type` 必须显式声明？

libnx 将 `__nx_time_type` 定义为弱符号，默认值为 `TimeType_LocalSystemClock`（硬件本地时钟）。该时钟：
- 可能没有经过用户在 Switch 系统设置中进行的时区/夏令时调整
- 与用户在 Switch 主界面看到的时间可能不一致

`TimeType_UserSystemClock` 是"用户系统时钟"，与 Switch 主界面显示的时间完全一致，是游戏 RTC 仿真的正确时钟源。

mGBA 的 Switch 独立版（`src/platform/switch/main.c:33`）明确设置了该变量，因此其 RTC 功能正常工作。BeikLiveStation 作为使用 mGBA libretro 核心的宿主，也需要在自己的 Switch 入口文件中声明该变量。

### GB MBC3 RTC 持久化流程（修复后）

```
游戏加载
  → loadSram()       读取 .sav → RETRO_MEMORY_SAVE_RAM
  → loadRtc()        读取 .rtc → RETRO_MEMORY_RTC
      mGBA 内部用 unixTime 计算距上次会话的时间差，推进 RTC 寄存器

游戏运行中
  → game loop: time(0) 通过 UserSystemClock 返回正确系统时间
  → MBC3 latch 命令触发 _latchRtc() → 更新 RTC 寄存器

游戏退出
  → saveSram()       RETRO_MEMORY_SAVE_RAM → .sav
  → saveRtc()        RETRO_MEMORY_RTC → .rtc（含 unixTime 时间戳）
```

### GBA RTC（无需持久化）

GBA（如宝可梦 RS/Emerald）的 RTC 通过 GPIO 引脚访问，每次游戏询问时直接调用 `time(0)`，没有需要持久化的 RTC 状态。因此 GBA 只需确保 `time(0)` 返回正确值即可，无需 `.rtc` 文件。

---

## 验证要点

| 测试场景 | 预期结果 |
|---------|---------|
| Switch 上运行宝可梦 Ruby/Sapphire/Emerald（GBA RTC） | 游戏内时钟与 Switch 系统时间一致 |
| Switch 上运行宝可梦 Gold/Silver/Crystal（GB MBC3 RTC） | 游戏内时钟与 Switch 系统时间一致，且退出再进入后时钟正常推进 |
| 首次加载无 `.rtc` 文件的游戏 | 正常提示"无 RTC 文件，从当前时间开始"，游戏正常运行 |
| 有 `.sav` 无 `.rtc` 文件（旧版存档迁移） | loadRtc 在 loadSram 早返回路径中被调用，游戏时钟从当前时间重新计算 |
| 非 MBC3 游戏（无 RTC 内存区域） | `getMemorySize(RETRO_MEMORY_RTC)` 返回 0，loadRtc/saveRtc 静默跳过 |
| 功能回归：保存/读档、快进、倒带等 | 不受影响 |
