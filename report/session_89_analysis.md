# Session 89 分析文档：mGBA 状态文件截图信息来源分析

## 问题

> 阅读 mgba 源码 `third_party/mgba/src/feature/gui/gui-runner.c`，告诉我为什么 mgba 的 state 中有截图信息。

---

## 核心结论

mGBA 的状态文件包含截图信息，原因是 mGBA 原生 GUI 在保存状态时传入了 `SAVESTATE_SCREENSHOT` 标志，使状态文件被保存为 **PNG 格式**：游戏当前帧即为 PNG 的像素数据，而游戏核心状态被 zlib 压缩后嵌入到自定义 PNG 块中。这样做的目的是在存档选择菜单中直接显示每个存档槽位的游戏画面预览。

---

## 详细分析

### 1. 触发位置（gui-runner.c）

```c
// gui-runner.c 第 654 行：保存状态
mCoreSaveState(runner->core, item->data.v.u >> 16,
    SAVESTATE_SCREENSHOT | SAVESTATE_SAVEDATA | SAVESTATE_RTC | SAVESTATE_METADATA);
```

传入了 `SAVESTATE_SCREENSHOT` 标志。

### 2. PNG 格式序列化（serialize.c）

```c
// serialize.c：mCoreSaveStateNamed()
if (flags & SAVESTATE_SCREENSHOT) {
    // 调用 _savePNGState()，以 PNG 格式保存
    bool success = _savePNGState(core, vf, &extdata);
    ...
}
```

`_savePNGState()` 的工作流程：

```
┌─────────────────────────────────────────┐
│           PNG 文件结构                  │
├─────────────────────────────────────────┤
│ PNG 标准图像数据 ← 游戏当前帧截图      │
│   (PNGWritePixels)                      │
├─────────────────────────────────────────┤
│ 自定义块 "gbAs" ← 核心状态             │
│   (zlib 压缩后的 core->saveState 数据)  │
├─────────────────────────────────────────┤
│ 自定义块 "gbAx" ← 扩展数据            │
│   (savedata / RTC / cheats 等)          │
└─────────────────────────────────────────┘
```

### 3. 截图用于菜单预览（gui-runner.c `_drawState` 函数）

```c
// gui-runner.c 第 116 行：绘制存档槽位预览背景
static void _drawState(struct GUIBackground* background, void* id) {
    // 打开状态文件，若为 PNG 格式则读取像素数据
    if (vf && isPNG(vf) && pixels) {
        // 直接从状态文件（PNG）读取截图并显示为背景
        PNGReadPixels(png, info, pixels, w, h, w);
        gbaBackground->p->drawScreenshot(...);
    }
    ...
}
```

用户在存档菜单中浏览槽位时，`_drawState` 函数直接将状态文件作为 PNG 打开并读取像素，渲染为菜单背景，形成"所见即所存"的效果。

---

## 两种状态格式对比

| 特征         | mGBA 原生 GUI                          | libretro 接口（本项目）             |
|--------------|---------------------------------------|-------------------------------------|
| 触发函数     | `mCoreSaveState(..., SAVESTATE_SCREENSHOT)` | `retro_serialize()` → `mCoreSaveStateNamed(..., SAVESTATE_SAVEDATA \| SAVESTATE_RTC)` |
| 文件格式     | PNG 文件（截图 + 嵌入状态）            | 二进制文件（仅状态数据）            |
| 包含截图     | ✅ 是（PNG 像素数据即为截图）           | ❌ 否                               |
| 截图来源     | 状态文件本身                           | 独立的 `.ss*.png` 缩略图文件        |

---

## 对本项目的影响与改进

### 原有实现（Session 89）

在 `doQuickSave()` 中使用 `stbi_write_png()` 将当前游戏帧保存为独立的缩略图文件：
- 状态文件：`game.ss1`（二进制）
- 缩略图：`game.ss1.png`（独立 PNG 文件）

### 改进（本 Session）

改进 `stateInfoCallback` 以支持两种格式：

```cpp
// 优先检查状态文件本身是否为 PNG（mGBA 原生格式，状态即截图）
static constexpr uint8_t k_pngSig[8] = {0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A};
{
    std::ifstream sf(statePath, std::ios::binary);
    uint8_t hdr[8] = {};
    if (sf.read(...) && gcount == 8 && memcmp(hdr, k_pngSig, 8) == 0) {
        // 状态文件本身就是含截图的 PNG，直接用作缩略图
        info.thumbPath = statePath;
        return info;
    }
}
// 二进制格式：查找配套缩略图文件
std::string thumbPath = statePath + ".png";
if (exists(thumbPath)) info.thumbPath = thumbPath;
```

这样同时兼容：
- 本项目的 libretro 格式状态（二进制 + 独立缩略图）
- mGBA 原生 GUI 格式状态（PNG 文件本身即截图）
