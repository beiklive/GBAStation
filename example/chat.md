`RETRO_MEMORY_RTC` 是 **libretro API** 中定义的一种特殊内存类型，用于让前端（如 RetroArch）访问核心中的 **RTC（Real Time Clock）数据**。典型用途：

* 宝可梦等游戏的 **实时时钟**
* 模拟器 RTC 状态保存
* 支持 **savestate / rewind / netplay** 时同步 RTC

它通过 `retro_get_memory_data()` 和 `retro_get_memory_size()` 暴露给前端。

---

## 一、RETRO_MEMORY_RTC 的定义

在 `libretro.h` 中：

```c
#define RETRO_MEMORY_RTC 6
```

前端会调用：

```c
void* retro_get_memory_data(unsigned id);
size_t retro_get_memory_size(unsigned id);
```

当 `id == RETRO_MEMORY_RTC` 时，你需要返回 **RTC 内存指针和大小**。

---

## 二、核心实现方式

### 1 定义 RTC 结构

一般模拟器都会有 RTC 状态，例如：

```c
struct CoreRTC {
    uint64_t unix_time;
    uint32_t subsecond;
    uint8_t control;
};

static struct CoreRTC g_rtc;
```

---

### 2 实现 memory 接口

```c
void* retro_get_memory_data(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_RTC:
            return &g_rtc;

        default:
            return NULL;
    }
}

size_t retro_get_memory_size(unsigned id)
{
    switch (id)
    {
        case RETRO_MEMORY_RTC:
            return sizeof(g_rtc);

        default:
            return 0;
    }
}
```

这样 **前端就能访问 RTC 数据**。

---

## 三、运行时更新 RTC

核心需要在每帧更新 RTC，例如：

```c
void rtc_update(void)
{
    g_rtc.unix_time = time(NULL);
}
```

在 `retro_run()` 中：

```c
void retro_run(void)
{
    rtc_update();

    emulate_frame();
}
```

---

## 四、RetroArch 使用 RTC 的用途

如果核心实现了 `RETRO_MEMORY_RTC`：

RetroArch 可以：

1. **保存状态**
2. **回放 rewind**
3. **网络同步**
4. **RTC 校正**

否则 RTC 可能在 **load state 后错误**。

---

## 五、mGBA / melonDS 的做法

例如 **mGBA libretro**：

```c
case RETRO_MEMORY_RTC:
    return gba->rtc.sram;
```

size:

```c
case RETRO_MEMORY_RTC:
    return sizeof(gba->rtc.sram);
```

melonDS 也是类似实现。

---

## 六、Switch 自制前端（你这种情况）

如果你在 **Switch 上自己写前端 + libretro 核心**：

你也可以主动读取 RTC：

```c
void* rtc = core.retro_get_memory_data(RETRO_MEMORY_RTC);
size_t rtc_size = core.retro_get_memory_size(RETRO_MEMORY_RTC);
```

这样可以：

* 保存 RTC
* 写入 RTC
* 修正 RTC

---

## 七、常见错误

### 1 没实现接口

前端会得到：

```
NULL
size = 0
```

RTC 不工作。

---

### 2 RTC 不参与 savestate

RTC 应该 **参与状态保存**，否则：

* load state
* 时间会跳变

---

### 3 RTC 大小不固定

RTC memory 必须 **固定大小结构体**。

---

## 八、推荐 RTC 结构

推荐这样：

```c
typedef struct
{
    int64_t unix_time;
    int32_t offset;
    uint8_t flags;
} RetroRTC;
```

然后：

```
RETRO_MEMORY_RTC -> &rtc
```

---

## 九、如果你是做 Switch + libretro + mGBA

你 **其实不用自己实现 RTC**，因为：

mGBA 核心已经实现：

```
RETRO_MEMORY_RTC
```

你只需要：

```c
core.retro_get_memory_data(RETRO_MEMORY_RTC);
```

即可。

---

如果你愿意，我可以再给你一份 **完整的 libretro 内存接口实现模板（含 SAVE / SYSTEM / RTC）**，这是 **所有 libretro 核心通用的标准实现**，直接复制就能用。
