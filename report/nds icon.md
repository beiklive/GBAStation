NDS 游戏图标直接内置在 ROM 里，不需要模拟启动游戏。图标和标题信息放在 `.nds` 文件头指定的 Banner 区域（Banner/Icon Data）中。

流程是：

```text
NDS ROM
│
├─ ROM Header (0x0000~0x01FF)
│      ↓
│      Banner Offset (0x68)
│
└─ Banner Data
       ├─ 图标 (32×32，4bpp)
       ├─ 调色板（16色）
       └─ 多语言标题
```

图标固定：

* 分辨率：`32×32`
* 色深：`4bpp`
* 调色板：16色（第0色透明）
* 存储大小：

```text
32×32×4bit = 512 Byte
```

Banner 数据结构（常用部分）：

```cpp
struct NDSBanner {
    uint16_t version;          // 0x00
    uint16_t crc;              // 0x02

    uint8_t icon[512];         // 0x20
    uint16_t palette[16];      // 0x220

    wchar_t titleJP[128];
    wchar_t titleEN[128];
    wchar_t titleFR[128];
    wchar_t titleDE[128];
    wchar_t titleIT[128];
    wchar_t titleES[128];
};
```

读取方法：

```cpp
std::ifstream f(path,std::ios::binary);

// Header中的Banner偏移
f.seekg(0x68);

uint32_t bannerOffset;
f.read((char*)&bannerOffset,4);

// 跳到Banner
f.seekg(bannerOffset);

NDSBanner banner;
f.read((char*)&banner,sizeof(NDSBanner));
```

然后把图标数据解码：

```cpp
for (int y=0;y<32;y++)
{
    for (int x=0;x<32;x++)
    {
        int index=(y*32+x);

        // 每字节两个像素
        uint8_t b=banner.icon[index/2];

        uint8_t colorIndex=
            (index&1)?
            (b>>4):
            (b&0xF);

        // 转RGB555
        uint16_t c=
            banner.palette[colorIndex];

        int r=(c&31)<<3;
        int g=((c>>5)&31)<<3;
        int b=((c>>10)&31)<<3;
    }
}
```

但还有一个坑：图标不是按行存储，而是按 **8×8 Tile 排列**。

真实顺序：

```text
32x32
┌──┬──┬──┬──┐
│T0│T1│T2│T3│
├──┼──┼──┼──┤
│T4│T5│T6│T7│
├──┼──┼──┼──┤
│T8│T9│...  │
└──┴──┴─────┘
```

需要先按 Tile 解码：

```cpp
for(int tileY=0;tileY<4;tileY++)
for(int tileX=0;tileX<4;tileX++)
for(int py=0;py<8;py++)
for(int px=0;px<8;px++)
{
    int tileIndex=tileY*4+tileX;

    int offset=
        tileIndex*32+
        py*4+
        px/2;

    uint8_t b=banner.icon[offset];

    int ci=
        (px&1)?
        (b>>4):
        (b&0xF);

    int x=tileX*8+px;
    int y=tileY*8+py;

    image[y][x]=palette[ci];
}
```

如果你做自己的模拟器/游戏列表（你前面一直在做 Switch 模拟器前端），建议启动扫描 ROM 时直接缓存：

```cpp
struct RomMeta {
    std::string title;
    std::string gameCode;
    TextureHandle icon;
};
```

首次扫描：

```text
ROM → Header → Banner → 图标解码 → RGBA → PNG缓存
```

缓存到：

```text
sdmc:/GBAStation/cache/nds_icons/ABCD.png
```

这样列表滚动时不用反复解析 `.nds` 文件。
