# Borealis Virtualized Grid 实施说明（给 AI 模型）

你现在需要实现一个：

* Borealis
* NanoVG
* Draw-only
* 高性能虚拟化 Grid 游戏库系统

项目架构说明已经拆分为三个 md 文件：

---

# 你必须首先阅读的三个文档

请严格按以下顺序阅读：

---

## 1. 网格总体架构设计.md

该文档描述：

* GameGridView 总体结构
* frame 生命周期
* draw 生命周期
* visible range 虚拟化
* 滚动系统
* focus 系统
* 异步图片系统
* TextureCache
* AsyncImageLoader
* ScrollController
* FocusController
* 动画系统
* draw-only 渲染体系

这是整个系统最核心的文档。

必须先阅读。

---

## 2. rebuild排序分类架构设计.md

该文档描述：

* visibleIndices 架构
* 分类
* 排序
* 搜索
* rebuildVisibleList()
* Grid 数据重构
* focus 修复
* scroll 修复
* 后台排序
* 动画重排

该文档决定：

Grid 如何在：

* 5000+
* 10000+
* 甚至更多数据量

下仍然保持流畅。

该文档与 GridView 强关联。

---

## 3. GridDrawItem 设计方案.md

该文档描述：

* GridDrawItem 数据结构
* item draw 布局
* drawItem()
* 文字绘制
* 图片绘制
* badge 绘制
* marquee
* 高亮框
* focus animation
* item frame 更新

该文档决定：

单个 item 如何：

* 完全 draw-only
* 无 View Tree
* 无 Yoga
* 无 Label
* 无 Image

这是性能核心。

---

# 非常重要：整体设计原则

整个系统必须遵守以下原则。

违反任何一条：

都会导致性能下降。

---

# 1. 禁止创建 Item View

禁止：

* GameCardView
* GridItemView
* brls::Label
* brls::Image
* brls::Box

作为 Grid Item。

Grid Item 必须：

```cpp
struct GridDrawItem
```

即：

纯数据结构。

---

# 2. Grid 必须只有一个 View

必须：

```cpp
class GameGridView : public brls::View
```

整个 Grid：

只能存在一个真正的 Borealis View。

所有 item：

必须完全 draw-only。

---

# 3. 不允许使用 Yoga 布局

禁止：

* setGrow
* setAxis
* addView
* setAlignItems
* setJustifyContent

所有 item 布局：

必须通过：

```cpp
x/y/width/height
```

手动计算。

---

# 4. draw() 只能绘制可见区域

禁止：

```cpp
for (全部 items)
```

必须：

```cpp
for (visible items)
```

visible range：

必须根据：

* scrollY
* viewport
* rowHeight

动态计算。

这是虚拟化核心。

---

# 5. 所有 item draw 必须统一管理

禁止：

```cpp
item.draw()
```

必须：

```cpp
renderer.drawItem(...)
```

由：

```cpp
GridDrawRenderer
```

统一绘制。

---

# 6. 图片必须异步加载

禁止：

* draw 时读取图片
* draw 时 decode 图片
* 主线程 decode 图片

必须：

后台线程 decode。

主线程 upload texture。

---

# 7. GPU 上传必须在主线程

禁止：

后台线程：

* OpenGL
* NanoVG
* Deko3D
* GPU API

后台线程：

只能：

CPU decode 图片。

---

# 8. 排序与分类禁止修改真实数据

禁止：

```cpp
sort(allGames)
```

必须：

```cpp
sort(visibleIndices)
```

visibleIndices：

决定：

Grid 显示顺序。

---

# 9. focus 必须基于 gameId 修复

禁止：

rebuild 后直接保留 selectedIndex。

必须：

```cpp
selectedGameId
```

rebuild 后：

重新查找对应 index。

否则：

分类切换后焦点会错乱。

---

# 10. item 不允许持有 View

GridDrawItem 内：

禁止：

* View*
* Label*
* Image*
* Box*

GridDrawItem：

只能保存：

* 数据
* 动画状态
* texture handle
* marquee
* focus 状态

---

# 推荐最终架构

最终应该类似：

```text
GameGridView
│
├── vector<GridDrawItem>
│
├── GridDrawRenderer
│
├── TextureCache
│
├── AsyncImageLoader
│
├── ScrollController
│
├── FocusController
│
├── AnimationSystem
│
└── LayoutEngine
```

---

# 正确实施顺序（非常重要）

请严格按以下顺序实施：

---

重构对象在src\ui\utils目录
item: 
    RecyclingGridItem
grid:
    RecyclingGridDataSource
    RecyclingGrid
实施完毕后对使用对应类的代码进行修复替换

## Phase 1

先实现：

* GameGridView
* visible range
* 基础 draw
* 基础 scroll

不要先做动画。

不要先做图片。

---

## Phase 2

实现：

* GridDrawItem
* drawItem()
* focus
* 高亮框

---

## Phase 3

实现：

* AsyncImageLoader
* TextureCache
* 主线程 upload

---

## Phase 4

实现：

* rebuildVisibleList()
* 排序
* 分类
* 搜索

---

## Phase 5

实现：

* marquee
* 惯性滚动
* focus animation
* glow
* smooth scroll

---

# 必须优先保证

优先级：

```text
性能
>
虚拟化
>
稳定性
>
动画
>
视觉效果
```

不要为了视觉效果破坏：

* draw-only
* virtualization
* async loading

---

# 最终目标

最终效果应该接近：

* Nintendo eShop
* Steam Big Picture
* RetroArch Ozone
* Playnite
* Pegasus Frontend

这种：

高性能游戏封面墙 UI。

而不是：

传统 Borealis View Tree UI。

---

# 最终要求（最重要）

整个系统必须：

```text
Draw-only
Virtualized
Async Texture
Single View
No Yoga
No Item View
```

这是整个项目最核心的设计原则。
