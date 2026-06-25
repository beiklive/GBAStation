

# 一、当前 GridItem 布局分析

你当前布局实际上是：

---

# 普通模式

```text id="j9h3ji"
┌──────────────────────────────┐
│ [IMAGE]  [BADGE] TITLE       │
│          PLAY TIME           │
│          SUB TEXT            │
└──────────────────────────────┘
```

---

# 空状态

```text id="lfh3oj"
┌──────────────────────────────┐
│         EMPTY LABEL          │
└──────────────────────────────┘
```

---

# 当前接口设计分析

你当前：

```cpp id="10f7li"
setImagePath()
setBadge()
setTitle()
setSubText()
setPlayTime()
setEmpty()
```

这是：

```text id="f0u7l6"
View 驱动接口
```

问题：

```text id="jlwm8l"
状态分散
draw 状态依赖控件树
```

不适合：

```text id="mwvlbl"
虚拟列表
```

---

# 二、重构目标

重构后：

```text id="w6n90n"
GridDrawItem
=
纯数据
```

例如：

```cpp id="u7w4al"
struct GridDrawItem
{
};
```

不再：

```text id="iv4d4o"
继承 View
```

---

# 三、新 GridDrawItem 设计

# 推荐：

```cpp id="ljhzlc"
struct GridDrawItem
{
    uint64_t gameId;

    bool empty = false;

    std::string title;
    std::string subText;
    std::string playTime;

    PlatformBadgeColor badgeColor;
    std::string badgeText;

    std::string imagePath;
    std::string imageLayerPath;

    bool imageLayerVisible = false;

    bool favorite = false;

    float marqueeOffset = 0.f;

    float focusScale = 1.f;

    float focusGlow = 0.f;

    uint64_t textureHandle = 0;

    uint64_t imageLayerHandle = 0;

    bool textureReady = false;

    bool selected = false;
};
```

---

# 四、核心变化（非常重要）

# 旧方案：

```text id="pvxg4t"
GridItem 自己 draw 自己
```

---

# 新方案：

```text id="cgjlwm"
GridView 统一 draw 所有 item
```

也就是说：

```text id="z3d7i0"
item 不负责 draw
```

而：

```text id="vjlwm5"
Renderer 负责 draw
```

---

# 五、新布局方案

# GridDrawItem 最终布局

---

# 普通模式

```text id="2wyjlwm"
┌──────────────────────────────┐
│ ┌─────┐                      │
│ │ IMG │ [GBA] TITLEEEEEEEE   │
│ │     │ PLAY TIME            │
│ └─────┘ LAST PLAYED          │
└──────────────────────────────┘
```

---

# 空状态

```text id="96y2yl"
┌──────────────────────────────┐
│         EMPTY SLOT           │
└──────────────────────────────┘
```

---

# 六、真正的 draw 架构（重要）

最终：

```text id="jlwm6t"
GameGridView
    ↓
drawVisibleItems()
    ↓
drawGridItem()
```

---

# 七、GridDrawRenderer（推荐）

推荐：

```cpp id="1kzjlwm"
class GridDrawRenderer
{
public:

    static void drawItem(...);
};
```

统一管理：

* item draw
* 高亮
* 图片
* 文本
* 动画

---

# 八、drawItem() 设计

# 参数：

```cpp id="h5zjlwm"
drawItem(
    NVGcontext* vg,
    GridDrawItem& item,
    Rect rect,
    bool focused
)
```

---

# 九、draw 流程（核心）

```text id="jlwm0q"
1. draw background
2. draw focus glow
3. draw image
4. draw overlay
5. draw badge
6. draw title
7. draw play time
8. draw sub text
9. draw favorite icon
```

---

# 十、背景绘制

替代：

```cpp id="3rjlwm"
Box::draw()
```

直接：

```cpp id="jlwm6v"
nvgBeginPath(vg);
nvgRoundedRect(...);
nvgFillColor(...);
nvgFill(vg);
```

---

# 十一、高亮框绘制

焦点：

```cpp id="jlwmk0"
if (focused)
```

绘制：

```text id="jlwm8u"
外发光
高亮边框
缩放
```

---

# 推荐效果

```cpp id="jlwm8d"
nvgStrokeWidth(vg, 2.f);

nvgStrokeColor(
    vg,
    nvgRGBA(80, 180, 255, 255)
);
```

---

# 十二、缩放动画

# draw 时：

```cpp id="jlwm9c"
float scale = item.focusScale;
```

---

# Frame：

```cpp id="jlwmh8"
item.focusScale +=
    (target - current)
    * 0.15f;
```

---

# 十三、图片区域布局

# 左侧固定：

```cpp id="2rjlwm"
imageSize =
    rect.h - 10;
```

---

# 图片位置：

```cpp id="jlwmr5"
imageX = rect.x + 5;
imageY = rect.y + 5;
```

---

# 十四、图片绘制

# 不再：

```cpp id="vjlwmx"
brls::Image
```

---

# 直接：

```cpp id="9djlwm"
nvgImagePattern(...)
```

---

# draw image

```cpp id="v4jlwm"
NVGpaint paint =
    nvgImagePattern(
        vg,
        imageX,
        imageY,
        imageSize,
        imageSize,
        0.f,
        textureId,
        1.f
    );
```

---

# 十五、图片圆角

```cpp id="6jjlwm"
nvgRoundedRect(
    vg,
    imageX,
    imageY,
    imageSize,
    imageSize,
    4.f
);
```

---

# 十六、标题区域

# 右侧：

```cpp id="wqjlwm"
textX =
    imageX + imageSize + 10;
```

---

# title

```cpp id="6rjlwm"
titleY =
    rect.y + 22;
```

---

# playtime

```cpp id="jlwm2f"
playY =
    rect.y + 42;
```

---

# subtext

```cpp id="jlwm1p"
subY =
    rect.y + 62;
```

---

# 十七、Badge 绘制

替代：

```cpp id="1jlwm4"
badgeBox
```

直接：

```cpp id="jlwm0b"
nvgRoundedRect(...)
```

---

# badge size

```cpp id="k8jlwm"
badgeW = 36;
badgeH = 20;
```

---

# 十八、Badge 颜色

沿用：

```cpp id="yjlwmf"
_getBadgeColor()
```

即可。

---

# 十九、文本系统（重要）

# 不再：

```cpp id="9jlwmq"
brls::Label
```

---

# 直接：

```cpp id="x7jlwm"
nvgText()
```

---

# 二十、标题跑马灯（核心）

# 只对 focus item：

```cpp id="3mjlwm"
if (focused)
```

---

# Frame：

```cpp id="jlwmmv"
item.marqueeOffset +=
    delta * 30.f;
```

---

# Draw：

```cpp id="jlwmkk"
nvgScissor(vg, textRect);

nvgText(
    vg,
    textX - marqueeOffset,
    titleY,
    title.c_str(),
    nullptr
);
```

---

# 二十一、空状态绘制

# empty item

```cpp id="jlwm7s"
if (item.empty)
```

---

# draw：

```cpp id="jlwmnh"
draw centered text
```

即可。

---

# 二十二、Frame() 中 item 更新

# Frame 不再更新 View

而：

```text id="jlwm3x"
更新 item 状态
```

---

# frame 处理：

```text id="9jjlwm"
1. focus animation
2. marquee
3. image upload
4. hover glow
5. scroll animation
```

---

# 二十三、focus 动画

```cpp id="jlwmu8"
float target =
    focused ? 1.05f : 1.f;

item.focusScale +=
    (target - item.focusScale)
    * 0.15f;
```

---

# 二十四、Glow 动画

```cpp id="jlwm0m"
item.focusGlow +=
    ((focused ? 1.f : 0.f)
    - item.focusGlow)
    * 0.15f;
```

---

# 二十五、Grid 中如何使用（核心）

# 原来：

```cpp id="jlwm1w"
GridItem* item
```

---

# 现在：

```cpp id="jlwmx5"
std::vector<GridDrawItem>
```

---

# Grid draw

```cpp id="jlwm9d"
for (visible items)
{
    renderer.drawItem(...);
}
```

---

# 二十六、Grid 中的 focus

# Grid 维护：

```cpp id="jlwmr1"
selectedIndex
```

---

# draw 时：

```cpp id="2vjlwm"
bool focused =
    (i == selectedIndex);
```

---

# 二十七、Grid 中的 frame

```cpp id="jlwmjq"
for (visible items)
{
    updateItemAnimation();
}
```

---

# 二十八、Grid 中的图片请求

# visible item：

```cpp id="jlwm4g"
if (!textureReady)
{
    requestTexture(path);
}
```

---

# 二十九、GridDrawItem 不允许做的事

# 不允许：

```text id="jlwmq3"
不要:
View
Box
Label
Image
```

---

# 不允许：

```text id="2djlwm"
Yoga
```

---

# 不允许：

```text id="wjlwm0"
自动布局
```

---

# 三十、推荐最终结构

```text id="jlwmu3"
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
└── FocusController
```

---

# 三十一、最终 draw 生命周期

```text id="jlwm1m"
frame()
    ↓
update visible range
    ↓
update animation
    ↓
request draw
    ↓
draw()
    ↓
draw visible items
```

---

# 三十二、最终性能提升

你当前：

```text id="jlwm0s"
每 item:
10~15 View
```

---

# 重构后：

```text id="jlwmw7"
每 item:
0 View
```

---

# 最终效果

| 项目        | 当前  | 重构后 |
| --------- | --- | --- |
| 100 item  | 正常  | 流畅  |
| 1000 item | 开始卡 | 流畅  |
| 5000 item | 很卡  | 流畅  |
| 内存        | 高   | 极低  |
| draw call | 多   | 极少  |
| layout 成本 | 极高  | 几乎无 |

---

# 三十三、给其他模型的最终实施要求（最重要）

实施时必须：

---

## 1

```text id="0jlwmz"
GridDrawItem 是纯数据结构
```

---

## 2

```text id="zjlwmn"
所有 item 完全 draw-only
```

---

## 3

```text id="jlwm6n"
GridView 统一管理 draw
```

---

## 4

```text id="jlwmw0"
GridView 统一管理 frame
```

---

## 5

```text id="jlwmqf"
图片异步 decode
主线程 upload
```

---

## 6

```text id="jlwmx0"
只绘制 visible range
```

---

## 7

```text id="xjlwm7"
focus 只保存 index/gameId
```

---

## 8

```text id="yjlwm4"
不要任何 Borealis 子 View
```

这才是真正适合：

```text id="jlwm0r"
Switch 游戏封面墙
```

的大规模虚拟化方案。
