```text id="0qk7kx"
Grid 数据重构（Rebuild）
```

因为游戏库最容易卡顿的地方其实不是 draw：

而是：

* 排序
* 分类
* 搜索
* 过滤
* 收藏夹切换
* 平台切换
* 数据刷新

如果设计不好：

```text id="crr4hj"
会瞬间卡死几百毫秒
```

尤其：

```text id="c1qk2m"
5000+ ROM
```

时。

下面给你一套：

```text id="xgphgo"
高性能 Grid 数据重构架构
```

这个非常重要。

---

# 一、核心思想（最重要）

# 不要：

```text id="p24j95"
修改 UI
```

而是：

```text id="1m53xn"
修改“数据视图”
```

这是整个系统最关键思想。

---

# 二、正确的数据结构

# 原始数据

```cpp id="l1b16x"
std::vector<GameItemData> allGames;
```

永远不变。

---

# 显示数据

```cpp id="kofcrr"
std::vector<int> visibleIndices;
```

这里：

```text id="z5mn4g"
保存的是索引
不是 GameItemData
```

例如：

```cpp id="hn5d3w"
visibleIndices =
{
    8,
    3,
    100,
    25
};
```

代表：

```text id="pvtwqk"
显示顺序
```

---

# 三、为什么这样设计

因为：

```text id="34m1t9"
排序/分类时
不移动真实数据
```

只修改：

```cpp id="s6c9e5"
visibleIndices
```

性能会高很多。

---

# 四、Grid 真正读取的数据

draw 时：

不要：

```cpp id="a16x7l"
items[i]
```

而是：

```cpp id="o95z6r"
items[visibleIndices[i]]
```

---

# 五、排序（核心）

例如：

```text id="g7i93u"
按名称
按时间
按收藏
按评分
按平台
```

---

# 六、正确排序流程

# 不要：

```cpp id="94vqdz"
sort(allGames)
```

---

# 正确：

```cpp id="7dn7ig"
sort(visibleIndices)
```

---

# 示例

```cpp id="jjlwmx"
std::sort(
    visibleIndices.begin(),
    visibleIndices.end(),
    [&](int a, int b)
    {
        return allGames[a].title
             < allGames[b].title;
    });
```

---

# 七、为什么性能高

因为：

```text id="q84ndm"
移动 int
远比移动结构体快
```

尤其：

```cpp id="wmd5lf"
GameItemData
```

里面可能有：

* string
* metadata
* path
* texture info

很重。

---

# 八、分类系统（推荐）

推荐：

```cpp id="9mwmpn"
enum class GameCategory
{
    All,
    Favorite,
    GBA,
    GB,
    GBC,
    Recent,
};
```

---

# 九、分类不要重新生成数据

# 不要：

```cpp id="d7qzkd"
favoriteGames vector
gbaGames vector
gbGames vector
```

否则：

```text id="dxryjq"
数据复制
同步困难
内存浪费
```

---

# 十、正确分类方案

切换分类：

重新构建：

```cpp id="vtr5v8"
visibleIndices
```

即可。

---

# 十一、RebuildVisibleList()

核心函数：

```cpp id="z4t7m2"
void rebuildVisibleList()
```

负责：

```text id="s1zru7"
过滤
排序
搜索
分类
```

---

# 十二、完整 rebuild 流程

```text id="t4qlyx"
clear visibleIndices
    ↓
filter
    ↓
search
    ↓
category
    ↓
sort
    ↓
recalculate layout
    ↓
fix focus
    ↓
refresh visible range
```

---

# 十三、完整 rebuild 示例

```cpp id="k5a2y4"
void rebuildVisibleList()
{
    visibleIndices.clear();

    for (int i = 0; i < allGames.size(); i++)
    {
        auto& game = allGames[i];

        if (!matchCategory(game))
            continue;

        if (!matchSearch(game))
            continue;

        visibleIndices.push_back(i);
    }

    applySort();

    refreshLayout();

    validateSelection();

    updateVisibleRange();

    requestDraw();
}
```

---

# 十四、搜索优化（非常重要）

# 不要：

```text id="62w2r5"
每帧搜索
```

---

# 正确：

只有：

```text id="4j1ojj"
输入变化时
```

才 rebuild。

---

# 十五、大数据搜索优化

5000+ ROM 时：

推荐：

```text id="xb83vn"
预生成 lowercase name
```

---

# GameItemData

```cpp id="ph4gb0"
std::string lowerTitle;
```

初始化时：

```cpp id="h7b9r0"
lowerTitle = toLower(title);
```

搜索时：

避免：

```text id="3pkn1i"
每次动态 lowercase
```

---

# 十六、排序缓存（高级优化）

# 不要每次：

```cpp id="ml5yzv"
std::sort(...)
```

---

# 推荐：

预生成：

```cpp id="vjl7ny"
sortedByName
sortedByFavorite
sortedByRecent
```

---

# 结构

```cpp id="eah60w"
unordered_map<SortMode, vector<int>>
```

---

# 切换排序时：

直接：

```cpp id="w99qv0"
visibleIndices = sortedCache[mode];
```

性能极高。

---

# 十七、网格重构（核心）

分类切换后：

```text id="n2c4cc"
Grid 会发生:
行数变化
位置变化
scroll 变化
focus 变化
```

这部分必须专门设计。

---

# 十八、Focus 修复（极重要）

# 不要：

```text id="k4b2ff"
selectedIndex 保持原值
```

因为：

```text id="ccpw1s"
数据顺序变了
```

会：

```text id="rr2kq6"
焦点跳错
```

---

# 十九、正确方案

保存：

```cpp id="cf7kh6"
selectedGameId
```

而不是：

```cpp id="uzvk3i"
selectedIndex
```

---

# rebuild 后：

重新查找：

```cpp id="bkgr5r"
selectedIndex =
    findGameIndexById(selectedGameId);
```

---

# 二十、如果游戏不存在

例如：

```text id="p3jnyj"
切换分类后被过滤
```

则：

```cpp id="kzyqkk"
selectedIndex = 0;
```

---

# 二十一、Scroll 修复

排序后：

```text id="8h2p6f"
scrollY 可能已经无效
```

---

# 必须：

```cpp id="cs4kdb"
scrollY =
    clamp(scrollY, 0, maxScroll);
```

---

# 二十二、超大数据 rebuild 卡顿问题

如果：

```text id="nv20o6"
10000+
```

排序可能卡。

---

# 推荐方案：

后台线程排序。

---

# 二十三、后台排序架构

主线程：

```text id="52hm26"
请求 rebuild
```

后台：

```text id="lhm0ix"
生成新 visibleIndices
```

完成：

```text id="hgtf7v"
主线程 swap
```

---

# 二十四、线程安全方案

# 不要：

```cpp id="bls4n8"
后台线程直接改 visibleIndices
```

---

# 正确：

后台：

```cpp id="65m3wy"
std::vector<int> pendingList;
```

主线程：

```cpp id="spx3zm"
visibleIndices.swap(pendingList);
```

---

# 二十五、Grid 重建动画（高级）

很多前端会：

```text id="i3w2ub"
瞬间跳变
```

很生硬。

---

# 推荐：

```text id="e0n24u"
位置过渡动画
```

---

# 二十六、Item 动画位置缓存

```cpp id="uw08yc"
unordered_map<uint64_t, AnimatedRect>
```

key：

```text id="k9l5n7"
gameId
```

---

# rebuild 后：

记录：

```text id="tq07sq"
旧位置
新位置
```

---

# Draw 时：

```cpp id="z8fpr0"
current =
    lerp(old, target, anim);
```

实现：

```text id="9fk6es"
丝滑网格重排动画
```

---

# 二十七、分页式 rebuild（高级）

如果：

```text id="2t1r6q"
20000 ROM
```

甚至后台排序也可能重。

---

# 推荐：

```text id="j8o4tz"
分帧 rebuild
```

例如：

每帧：

```cpp id="ud2hpc"
处理 200 个 item
```

逐渐完成。

---

# 二十八、数据脏标记系统

推荐：

```cpp id="nfe4n9"
bool visibleListDirty;
bool sortDirty;
bool layoutDirty;
```

---

# Frame 中：

```cpp id="up9ddm"
if (visibleListDirty)
{
    rebuildVisibleList();
}
```

避免：

```text id="mjlwmg"
重复 rebuild
```

---

# 二十九、布局缓存（非常重要）

# 不要：

```text id="kzhmsb"
每次 rebuild 重算所有 rect
```

---

# 正确：

item rect：

```text id="h18u3y"
公式实时推导
```

因为：

```text id="gl3ehx"
Grid 是规则结构
```

不需要存储。

---

# 三十、动态列数变化（重点）

例如：

```text id="7pvbvi"
3列 -> 5列
```

---

# 必须：

```text id="t5rm2t"
保持当前焦点 item 可见
```

---

# 正确做法

记录：

```cpp id="ff4jxg"
selectedGameId
```

重新布局后：

```cpp id="v3fwb2"
重新定位 index
```

然后：

```cpp id="d7q8rp"
ensureVisible(selectedIndex)
```

---

# 三十一、推荐最终数据流

```text id="3v0nc4"
ROM Database
    ↓
allGames
    ↓
rebuildVisibleList()
    ↓
visibleIndices
    ↓
visibleRange
    ↓
drawVisibleItems()
```

---

# 三十二、最终推荐模块结构

```text id="k5m2z4"
GameGridView
│
├── DataSource
│   ├── allGames
│   ├── visibleIndices
│   └── sort cache
│
├── LayoutEngine
│
├── ScrollController
│
├── FocusController
│
├── AsyncImageLoader
│
├── TextureCache
│
├── AnimationSystem
│
└── Renderer
```

---

# 三十三、给其他模型的核心要求（最重要）

实施时必须遵守：

---

## 1

```text id="u0j9n0"
不要创建 item View
```

---

## 2

```text id="9uh4kp"
Grid 完全 draw-only
```

---

## 3

```text id="qg26uj"
visibleIndices 控制显示顺序
```

---

## 4

```text id="nykn5i"
排序/分类只修改 visibleIndices
```

---

## 5

```text id="w4oh5j"
draw 只绘制 visible range
```

---

## 6

```text id="ubdyca"
后台线程只 decode 图片
```

---

## 7

```text id="ym4tr3"
GPU 上传必须在主线程
```

---

## 8

```text id="ec4h0z"
focus 必须基于 gameId 修复
```

---

# 三十四、最终效果

最终会得到：

```text id="3nmy0z"
Switch 级别
高性能
虚拟化
无限滚动
游戏封面墙
```

效果会非常接近：

* Nintendo eShop
* Steam Big Picture
* Playnite
* RetroArch

这一类大型游戏库前端。
