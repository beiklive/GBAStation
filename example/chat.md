GBA 的 **240×160** 画面在进入渲染链前**最好做一次简单的预处理**，但**不要在 CPU 上缩放**。正确做法是：

**保持原始分辨率上传 GPU → shader pipeline 决定如何放大**。

如果你直接把 `frame.pixels` 丢进 shader pipeline，很多 RetroArch shader 会出现比例或滤波问题，因为它们依赖一些 **正确的尺寸信息（SourceSize / OutputSize）**。

下面是推荐的处理方式。

---

# 一、正确处理原则

### 1 保持原始分辨率

GBA 原始：

```
240 x 160
```

上传 GPU 时保持不变：

```cpp
glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    240,
    160,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    frame.pixels
);
```

不要做：

```
240x160 → 1920x1080
```

这种放大应该由 **shader pipeline 完成**。

---

# 二、渲染链推荐结构

```
frame.pixels (240x160)
        │
        ▼
Game Texture (240x160)
        │
        ▼
Shader Pass0
        │
        ▼
Shader Pass1
        │
        ▼
Final Texture (例如1280x720)
        │
        ▼
NanoVG UI
```

---

# 三、关键处理：比例

GBA 的比例：

```
240 / 160 = 1.5
```

即：

```
3 : 2
```

而屏幕一般：

```
16 : 9
```

所以你需要选择一种策略：

| 策略   | 效果   |
| ---- | ---- |
| 保持比例 | 两侧黑边 |
| 填满屏幕 | 拉伸   |
| 整数缩放 | 最清晰  |

推荐：**整数缩放 + 居中**。

---

# 四、整数缩放计算

假设 Switch 或窗口：

```
1280 x 720
```

计算：

```
1280 / 240 = 5.33
720  / 160 = 4.5
```

取最小整数：

```
scale = 4
```

最终画面：

```
240 * 4 = 960
160 * 4 = 640
```

显示区域：

```
960 x 640
```

居中：

```
x = (1280 - 960)/2 = 160
y = (720 - 640)/2 = 40
```

---

# 五、shader pipeline 输入尺寸

RetroArch shader 依赖以下 uniform：

```
SourceSize
OutputSize
```

例如：

```cpp
glUniform2f(sourceSizeLoc, 240,160);
glUniform2f(outputSizeLoc, 960,640);
```

---

# 六、推荐 FBO 设计

不要让 pass0 直接输出到屏幕。

推荐：

```
GameTexture      240x160
Pass0 FBO        240x160
Pass1 FBO        960x640
FinalTexture     960x640
```

很多 CRT shader 需要：

```
240x160 → 放大 → 再处理
```

---

# 七、GPU 过滤设置

像素游戏必须：

```cpp
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
```

否则会变模糊。

---

# 八、推荐最终渲染链

```
CPU frame (240x160)
        │
        ▼
GameTexture (240x160)
        │
        ▼
RetroShaderPipeline
        │
        ▼
FinalTexture (整数缩放)
        │
        ▼
NanoVG绘制
```

---

# 九、一个非常重要的优化（很多人会忽略）

**不要每帧重新创建 NanoVG image**

错误：

```
nvglCreateImageFromHandleGL3
```

每帧调用。

正确：

```
初始化时创建一次
之后只更新texture
```

否则性能会掉很多。

---

