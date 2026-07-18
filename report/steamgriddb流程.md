SteamGridDB API 的完整流程可以设计成：

```
输入游戏名称
      |
      v
请求图片列表(grids/heroes/logos/icons)
      |
      v
分页获取更多图片
      |
      v
选择图片
      |
      v
下载CDN图片
      |
      v
本地缓存
```

下面以 **GBAStation 游戏封面下载** 为例。

---

# 1. API 基础信息

基础地址：

```
https://www.steamgriddb.com/api/v2
```

所有请求 Header：

```http
Authorization: Bearer YOUR_API_KEY
```

例如：

```http
GET /api/v2/search/autocomplete/pokemon
Authorization: Bearer xxxxx
```

---

# 2. 第一步：搜索游戏

接口：

```
GET /search/autocomplete/{term}
```

例如：

搜索：

```
Pokemon Emerald
```

请求：

```http
GET https://www.steamgriddb.com/api/v2/search/autocomplete/Pokemon%20Emerald
```

Header:

```http
Authorization: Bearer YOUR_API_KEY
```

---

返回：

```json
{
    "success": true,
    "data": [
        {
            "id": 12345,
            "name": "Pokémon Emerald",
            "verified": true
        },
        {
            "id": 67890,
            "name": "Pokémon Emerald Randomizer"
        }
    ]
}
```

保存：

```
game_id = 12345
```

后续全部使用：

```
12345
```

---

# 3. 获取 Grid 封面列表

Grid 就是 Steam 游戏墙封面：

例如：

```
600x900
竖版
```

接口：

```
GET /grids/game/{game_id}
```

请求：

```http
GET https://www.steamgriddb.com/api/v2/grids/game/12345
```

---

返回：

```json
{
 "success":true,
 "data":[
   {
    "id":11111,
    "url":
    "https://cdn.cloudflare.steamgriddb.com/grid/xxx.png",

    "width":600,
    "height":900,

    "style":"alternate"
   },
   {
    "id":22222,
    "url":
    "https://cdn.cloudflare.steamgriddb.com/grid/yyyy.png",

    "width":600,
    "height":900
   }
 ]
}
```

得到：

```
图片列表:

[
 grid1.png,
 grid2.png,
 grid3.png
]
```

---

# 4. 获取 Hero 背景列表

接口：

```
GET /heroes/game/{game_id}
```

请求：

```http
GET https://www.steamgriddb.com/api/v2/heroes/game/12345
```

返回：

```json
{
"data":[
 {
   "id":555,
   "url":
   "https://cdn.cloudflare.steamgriddb.com/hero/abc.jpg",

   "width":1920,
   "height":620
 }
]
}
```

用于：

```
游戏详情页背景
```

---

# 5. 获取 Logo 列表

接口：

```
GET /logos/game/{game_id}
```

请求：

```http
GET https://www.steamgriddb.com/api/v2/logos/game/12345
```

返回：

```json
{
"data":[
 {
   "id":888,
   "url":
   "https://cdn.cloudflare.steamgriddb.com/logo/logo.png"
 }
]
}
```

通常：

```
透明PNG
```

适合：

```
Hero背景
+
Logo叠加
```

---

# 6. 获取 Icon

接口：

```
GET /icons/game/{game_id}
```

请求：

```http
GET https://www.steamgriddb.com/api/v2/icons/game/12345
```

返回：

```json
{
"data":[
 {
  "url":
  "https://cdn.cloudflare.steamgriddb.com/icon/a.png"
 }
]
}
```

---

# 7. 图片列表分页

SteamGridDB 图片接口支持分页参数：

参数：

```
page
```

例如：

第一页：

```
?page=1
```

第二页：

```
?page=2
```

---

例如：

获取 Grid 第2页：

```http
GET

https://www.steamgriddb.com/api/v2/grids/game/12345?page=2
```

---

返回：

```json
{
"success":true,

"data":[
 ...
],

"page":2,

"limit":50,

"total":120
}
```

逻辑：

```cpp
int page=1;

while(true)
{
    request(page);

    if(data.empty())
        break;

    append(data);

    page++;
}
```

---

# 8. 限制图片数量

实际模拟器不需要全部下载。

例如：

```
Grid:
获取第一页50张

选择:
1. verified=true
2. 分辨率最大
3. 评分最高
```

保存：

```
grid.png
```

即可。

---

# 9. 完整 C++ 流程

伪代码：

```cpp
GameAsset searchGame(string name)
{

    //1 搜索
    auto games =
       SGDB.search(name);


    if(games.empty())
        return {};


    //2 选择最佳匹配

    auto game =
       games[0];


    int id=game.id;



    //3 获取封面

    auto grids =
       SGDB.getGrids(id);


    //4 获取背景

    auto heroes =
       SGDB.getHeroes(id);



    //5 获取Logo

    auto logos =
       SGDB.getLogos(id);



    //6 下载

    download(
       grids[0].url,
       "cover.png"
    );


}
```

---

# 10. GBAStation 推荐缓存设计

不要每次启动访问 API。

建议：

```
sdmc:/GBAStation/

database/
 |
 + games.json


cache/
 |
 + artwork/
      |
      + BPEE/
          |
          + grid.png
          + hero.jpg
          + logo.png
```

games.json:

```json
{
"BPEE":
{
 "title":"Pokemon Emerald",

 "steamgriddb_id":12345,

 "assets":
 {
  "grid":"cache/artwork/BPEE/grid.png",
  "hero":"cache/artwork/BPEE/hero.jpg",
  "logo":"cache/artwork/BPEE/logo.png"
 }
}
```

---





SteamGridDB 返回的图片列表通常不是“自动给你最佳图片”，而是把社区上传的素材全部返回。因此模拟器前端需要自己做**素材筛选策略**。

对于 GBAStation 这种模拟器，我建议不要简单取第 1 张，而是建立评分系统。

---

## 1. Grid（封面）筛选

Grid 返回的数据类似：

```json
{
    "id":12345,
    "url":"xxx.png",
    "width":600,
    "height":900,
    "style":"alternate",
    "mime":"image/png",
    "language":"en",
    "nsfw":false,
    "humor":false
}
```

你可以根据这些字段筛选。

---

## 第一层：安全过滤

首先排除：

```cpp
if(item.nsfw)
    continue;

if(item.humor)
    continue;
```

避免：

* 恶搞图
* 成人素材
* Meme 图

---

## 第二层：尺寸过滤

模拟器封面建议：

比例：

```
宽 : 高 ≈ 2 : 3
```

例如：

```text
600 x 900
400 x 600
300 x 450
```

计算：

```cpp
float ratio =
(float)width / height;


if(abs(ratio - 0.666) > 0.15)
{
    reject;
}
```

---

## 第三层：分辨率评分

例如：

```cpp
score += width * height / 100000;
```

结果：

| 尺寸      |   分数 |
| ------- | ---: |
| 600×900 |  5.4 |
| 300×450 | 1.35 |
| 200×300 |  0.6 |

优先高清。

---

## 第四层：官方风格优先

SteamGridDB 有：

```json
style
```

例如：

```json
{
"style":"alternate"
}
```

常见：

```
official
alternate
blurred
white_logo
```

建议：

排序：

```
official
 >
alternate
 >
blurred
 >
其他
```

---

## 第五层：语言

你的用户主要中文/日文，可以优先：

```
ja
zh
en
```

例如：

```cpp
if(language=="ja")
    score+=10;

if(language=="zh")
    score+=10;

if(language=="en")
    score+=5;
```

---

# Grid评分示例

```cpp
int scoreGrid(Grid g)
{
    int score=0;


    if(g.nsfw)
        return -1000;


    float ratio =
      (float)g.width/g.height;


    if(abs(ratio-0.666)<0.1)
        score+=30;


    score +=
      g.width*g.height/100000;


    if(g.style=="official")
        score+=50;


    if(g.language=="ja")
        score+=20;


    return score;
}
```

然后：

```cpp
sort(
 grids.begin(),
 grids.end(),
 [](auto&a,auto&b)
 {
    return scoreGrid(a)>scoreGrid(b);
 });
```

取：

```cpp
grids[0]
```

---

# 2. Hero 背景筛选

Hero 和 Grid 不一样。

目标：

```
16:9 或超宽
```

例如：

```
1920×620
3840×1240
```

评分：

```cpp
float ratio =
(float)width/height;
```

目标：

```
3:1左右
```

评分：

```cpp
if(ratio>2.5)
    score+=30;


if(width>=1920)
    score+=30;
```

---

# 3. Logo筛选

Logo 最简单：

要求：

```
PNG
透明背景
宽度大
```

过滤：

```cpp
if(!hasTransparency)
    reject;


if(width<300)
    reject;
```

评分：

```cpp
score += width;

if(language=="en")
    score+=5;
```

---

# 4. 用户选择机制（推荐）

不要完全自动。

第一次下载：

显示：

```
Pokemon Emerald

请选择封面:

[图片1]
[图片2]
[图片3]
[图片4]

确认
```

用户选择后：

保存：

```json
{
 "gamecode":"BPEE",
 "steamgriddb_id":12345,

 "grid_id":88888,
 "hero_id":99999
}
```

以后自动使用。

---

# 5. 更适合 GBAStation 的算法

你的场景不是 Steam：

Steam：

```
一个游戏一个AppID
```

模拟器：

```
一个游戏
|
+ 美版
+ 日版
+ 欧版
+ 重制版
```

所以建议：

数据库：

```json
{
"id":"BPEE",

"title":
"Pokemon Emerald",

"artwork":
{
 "source":"SteamGridDB",

 "grid":
 {
   "id":12345,
   "url":"xxx"
 }
}
}
```

---

# 6. 最终推荐优先级

对于 GBA/NDS：

```
过滤
 |
 +-- NSFW=false
 |
 +-- 正确比例
 |
 +-- 最大分辨率
 |
 +-- official
 |
 +-- 日/中/英语言
 |
 +-- 用户选择
```

自动选择：

```
score最高
```

人工：

```
第一次确认
```

这套逻辑可以直接复用到你的：

* GBA
* NDS
* SNES
* MD
* PSP
* 3DS

所有平台。你现在已经有 ROM Header/GameCode 匹配系统，补上这个 ArtworkManager 基本就是一个完整的 Steam Deck 风格游戏库系统。
