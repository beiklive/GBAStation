# Session 51 – 图片缓存与异步加载优化

## 任务概述

按照需求完成以下五项优化：
1. 背景图片缓存（复用 brls TextureCache）
2. 文件列表/ImageView 图片缓存 + 打开游戏时清空缓存
3. 移除背景图片 GIF 支持
4. 详细面板和 ImageView 异步加载（加载中...提示）
5. 修复详细面板图片边缘像素拉伸

---

## 变更文件

### 新增

| 文件 | 说明 |
|------|------|
| `include/UI/Utils/ImageFileCache.hpp` | 文件字节缓存（主线程单例）|
| `src/UI/Utils/ImageFileCache.cpp` | 实现 |

### 修改

| 文件 | 主要变更 |
|------|---------|
| `include/common.hpp` | 引入 ImageFileCache.hpp；声明 `clearUIImageCache()` |
| `src/common.cpp` | 引入 borealis cache_helper；移除 ApplyXmbBg 中的 GIF 逻辑；实现 `clearUIImageCache()` |
| `include/UI/Utils/ProImage.hpp` | 添加 `setImageFromFileAsync()` 方法及 async 状态成员 |
| `src/UI/Utils/ProImage.cpp` | 实现 `setImageFromFileAsync`（含 GIF 支持、缓存命中快路径、brls::async 后台读取）；draw() 增加"加载中..."占位文字 |
| `include/UI/Pages/FileListPage.hpp` | `m_detailThumb` 改为 `beiklive::UI::ProImage*` |
| `src/UI/Pages/FileListPage.cpp` | buildDetailPanel 使用 ProImage + `setClipsToBounds(false)` 修复边缘拉伸；updateDetailPanel 改用 `setImageFromFileAsync` |
| `include/UI/Pages/ImageView.hpp` | 添加 async 状态字段 |
| `src/UI/Pages/ImageView.cpp` | 改为 brls::async + ASYNC_RETAIN 后台读取；draw() 显示"加载中..."；利用 ImageFileCache |
| `src/UI/Pages/BackGroundPage.cpp` | 移除 GIF 分支 |
| `src/UI/StartPageView.cpp` | 启动游戏前调用 `clearUIImageCache()` |

---

## 技术方案

### 需求 1 & 2：图片缓存机制

**brls TextureCache（已有）**：  
`brls::Image::setImageFromFile` 内部已通过 `brls::TextureCache`（LRU，容量 200）缓存 NVG 纹理句柄。同路径第二次调用直接命中缓存，无需重读磁盘。背景图使用 `setImageFromFile`，因此自动受益。

**ImageFileCache（新增）**：  
对于通过 `setImageFromFileAsync` 异步加载的图片（详细面板缩略图、ImageView），brls TextureCache 不适用（路径无法关联到 `setImageFromMem` 创建的纹理）。因此新增 `ImageFileCache` 缓存原始文件字节：
- 第一次加载：后台线程读磁盘 → 主线程写入缓存 → 创建 NVG 纹理
- 再次加载同路径：直接从缓存取字节 → 跳过磁盘读取

**打开游戏时清空缓存**：  
`clearUIImageCache()` 执行：
1. `ImageFileCache::instance().clear()` — 释放 RAM 中的文件字节
2. `brls::TextureCache::instance().cache.markAllDirty()` — 使 NVG 纹理缓存失效（不立即删除 GPU 纹理，安全）

在 `StartPageView` 中，所有 GameView 启动路径调用此函数。

---

### 需求 3：移除背景 GIF 支持

- `BackGroundPage::setImagePath`：删除 GIF 分支，始终调用 `setImageFromFile`
- `ApplyXmbBg`（common.cpp）：删除 GIF 分支，始终调用 `setImageFromFile`

GIF 动画仍在 **ImageView** 和 **详细面板**（ProImage 的 `setImageFromFileAsync` 支持 GIF 解码播放）中可见。

---

### 需求 4：异步加载 + 加载中提示

**ProImage::setImageFromFileAsync(path)**：
1. 检查 ImageFileCache（主线程）；命中 → 同步解码/创建 NVG 纹理
2. 缓存未命中 → `m_asyncLoading = true`，调用 `brls::async` 启动后台读取
3. 后台线程读完文件 → 通过 `brls::sync` 回到主线程
4. 主线程：写入 ImageFileCache，解码（或调用 `nvgCreateImageMem`），调用 `innerSetImage`
5. 用代次计数器（m_asyncGen）实现取消：新请求到来时旧结果被丢弃
6. `draw()` 在 `m_asyncLoading == true` 时绘制"加载中..."文字

**ImageView**（constructor + draw）：
- 构造时：检查 ImageFileCache，命中则复制字节；未命中则用 brls::async + ASYNC_RETAIN 后台读取
- draw() 中：消耗 `m_asyncReady` 信号 → 创建 NVG 纹理；`m_asyncLoading` 期间显示"加载中..."

---

### 需求 5：修复详细面板边缘像素拉伸

问题根因：`brls::Image` 在 `getClipsToBounds() == true`（默认）时，绘制区域为整个 widget 矩形（如 180×180），而纹理 paint 仅覆盖实际图片区域（如 180×101）。超出图片区域的像素由于 OpenGL CLAMP_TO_EDGE，显示为边缘像素拉伸。

修复方案：对 `m_detailThumb` 调用 `setClipsToBounds(false)`。此时 `brls::Image::draw` 改为绘制恰好等于图片尺寸的圆角矩形，不覆盖空白区域，边缘拉伸消失。同时 `setCornerRadius(8.f)` 仍然有效（作用于图片矩形本身）。

---

## 构建验证

在 Linux 桌面平台编译（`PLATFORM_DESKTOP=ON`），无编译错误。
