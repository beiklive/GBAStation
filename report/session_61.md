# Session 61 工作汇报

## 任务说明

排查 mGBA 核心运行的游戏无法获取系统时钟的原因，并修复相关缺失参数传递问题。

---

## 问题根因分析

### 根因一：Nintendo Switch 未初始化时间服务

**文件**：`third_party/borealis/library/lib/platforms/switch/switch_wrapper.c`

mGBA 的 RTC（实时时钟）实现直接调用 POSIX `time(0)` 函数（见 `third_party/mgba/src/core/interface.c` 的 `_rtcGenericCallback`，以及 `third_party/mgba/src/gba/cart/gpio.c` 的 `_rtcUpdateClock`）。

在 Nintendo Switch 平台上，`time()` 的底层实现依赖 libnx 的时间服务（`time` service）。**若未调用 `timeInitialize()` 显式初始化该服务，`time()` 调用可能返回 0（Unix 纪元时间）或错误值**，导致模拟游戏中的时钟（如宝可梦 RS/Emerald 的 GBA RTC、宝可梦 GSC 的 GB MBC3 RTC）无法正确读取系统时间。

`switch_wrapper.c` 已显式初始化 `romfsInit`、`plInitialize`、`setsysInitialize`、`setInitialize`、`psmInitialize`、`nifmInitialize`、`lblInitialize` 等服务，但**独缺 `timeInitialize()`**，属于遗漏。

### 根因二：libretro 日志接口未实现——RTC 错误被静默丢弃

**文件**：`src/Retro/LibretroLoader.cpp`

`s_environmentCallback` 对 `RETRO_ENVIRONMENT_GET_LOG_INTERFACE`（env ID 27）直接返回 `false`。按照 mGBA libretro 代码逻辑（`libretro.c:497-501`）：

```c
if (environCallback(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log)) {
    logCallback = log.log;
} else {
    logCallback = 0;
}
logger.log = GBARetroLog;
mLogSetDefaultLogger(&logger);
```

`logCallback = 0` 后，`GBARetroLog` 在入口即 `return`，**mGBA 核心产生的所有日志（包括 RTC 相关的警告和错误）均被静默丢弃**，无法通过日志定位时钟问题。

---

## 修复方案

### 修复一：`switch_wrapper.c` 添加时间服务初始化

在 `userAppInit()` 末尾添加 `timeInitialize()` 调用，在 `userAppExit()` 中添加匹配的 `timeExit()` 调用：

```c
// userAppInit() 末尾
timeInitialize();  // System clock: required for POSIX time() / localtime() used by emulator RTC

// userAppExit() 中
timeExit();  // system clock
```

**效果**：确保 Switch 平台的时间服务在游戏运行前已初始化，`time(0)` 能正确返回系统当前时间，GBA/GB RTC 模拟正常工作。

### 修复二：`LibretroLoader.cpp` 实现日志接口

1. 新增头文件 `#include <cstdarg>`
2. 在命名空间外添加静态日志回调函数 `s_coreLogCallback`，将核心消息转发至 `stderr`
3. 在 `s_environmentCallback` 中将 `RETRO_ENVIRONMENT_GET_LOG_INTERFACE` 从 "return false" 列表移出，单独处理并返回回调指针

```cpp
static void RETRO_CALLCONV s_coreLogCallback(enum retro_log_level level,
                                              const char* fmt, ...)
{
    static const char* const levelStr[] = { "DEBUG", "INFO", "WARN", "ERROR" };
    const char* tag = (level >= RETRO_LOG_DEBUG && level <= RETRO_LOG_ERROR)
                      ? levelStr[level] : "?";
    va_list args;
    va_start(args, fmt);
    fprintf(stderr, "[Core/%s] ", tag);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

// 在 s_environmentCallback 中：
case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
    retro_log_callback* log = static_cast<retro_log_callback*>(data);
    if (log) log->log = s_coreLogCallback;
    return true;
}
```

**效果**：mGBA 核心的所有日志信息（包括 RTC 读取失败、BIOS 加载状态、游戏硬件类型检测等）均可输出到 stderr，便于调试和后续问题排查。

---

## 技术细节

### mGBA RTC 调用链（GBA）

```
retro_run()
  → retro_run_impl()
    → core->run(core)
      → GBA CPU step
        → GPIO RTC 访问
          → _rtcUpdateClock(hw)
            → rtc->unixTime(rtc)          ← mRTCGenericSource
              → _rtcGenericCallback()
                → time(0)                  ← 需要 timeInitialize() 保障
              → localtime_r(&t, &date)     ← 将 Unix 时间转换为日历格式
```

### mGBA RTC 调用链（GB/MBC3）

```
gb/mbc.c: _latchRtc()
  → rtc->unixTime(rtc)
    → _rtcGenericCallback()
      → time(0)                            ← 同上
```

---

## 修改文件汇总

| 文件 | 修改类型 | 说明 |
|------|---------|------|
| `third_party/borealis/library/lib/platforms/switch/switch_wrapper.c` | Bug 修复 | 添加 `timeInitialize()` / `timeExit()`，确保 Switch 时间服务可用 |
| `src/Retro/LibretroLoader.cpp` | 功能补全 | 实现 `RETRO_ENVIRONMENT_GET_LOG_INTERFACE`，将核心日志转发至 stderr |

---

## 验证要点

- **Switch 平台**：运行含 RTC 的游戏（如宝可梦 RS/Emerald、宝可梦金银）后，游戏内时钟应显示与系统时间一致的日期/时间
- **所有平台**：启动 mGBA 核心时，stderr 应能看到核心打印的 `[Core/INFO]` 等级别日志，包括游戏硬件类型、BIOS 加载结果等信息
- **功能回归**：现有保存/读档、快进、倒带、按键映射等功能不受影响
