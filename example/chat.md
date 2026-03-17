你现在已经拿到 **`frame.pixels`（游戏原始帧）**，接下来要实现的是一条完整的 **GPU 渲染 + RetroArch Shader Pipeline + UI 显示链**。
整体目标是：**CPU帧 → GPU纹理 → 多Pass Shader → 最终纹理 → NanoVG显示**。

下面给你一条**完整处理链（也是大多数模拟器的架构）**。

---

# 一、完整渲染处理链

```
mGBA / libretro
        │
        ▼
frame.pixels (CPU内存)
        │
        ▼
上传到 OpenGL Texture
        │
        ▼
Game FBO
        │
        ▼
RetroArch Shader Pass0
        │
        ▼
RetroArch Shader Pass1
        │
        ▼
RetroArch Shader PassN
        │
        ▼
Final Texture
        │
        ▼
NanoVG UI / 显示
```

对应模块：

| 阶段              | 功能                |
| --------------- | ----------------- |
| CPU帧            | `frame.pixels`    |
| 纹理上传            | `glTexSubImage2D` |
| 原始FBO           | 游戏画面输入            |
| shader pipeline | RetroArch多pass    |
| 最终纹理            | Shader输出          |
| UI显示            | NanoVG绘制          |

---

# 二、第一步：CPU帧 → GPU纹理

你现在拿到：

```
frame.pixels
width
height
```

创建游戏纹理：

```cpp
GLuint gameTexture;

glGenTextures(1, &gameTexture);
glBindTexture(GL_TEXTURE_2D, gameTexture);

glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGBA,
    width,
    height,
    0,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    nullptr
);

glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
```

每一帧更新：

```cpp
glBindTexture(GL_TEXTURE_2D, gameTexture);

glTexSubImage2D(
    GL_TEXTURE_2D,
    0,
    0,
    0,
    width,
    height,
    GL_RGBA,
    GL_UNSIGNED_BYTE,
    frame.pixels
);
```

---

# 三、第二步：创建 Game FBO

很多 RetroArch shader 需要 **原始帧FBO**。

```cpp
GLuint gameFBO;

glGenFramebuffers(1, &gameFBO);
glBindFramebuffer(GL_FRAMEBUFFER, gameFBO);

glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_COLOR_ATTACHMENT0,
    GL_TEXTURE_2D,
    gameTexture,
    0
);
```

---

# 四、第三步：RetroArch Shader Pipeline

核心结构：

```cpp
struct ShaderPass
{
    GLuint program;

    GLuint fbo;
    GLuint texture;

    int width;
    int height;
};
```

Pipeline：

```cpp
class RetroShaderPipeline
{
public:

    GLuint process(GLuint inputTexture);

    std::vector<ShaderPass> passes;
};
```

---

# 五、第四步：执行 Shader Pass

核心流程：

```
inputTexture = gameTexture

for pass in passes
    bind(pass.fbo)
    bind(pass.program)

    glBindTexture(inputTexture)

    draw_fullscreen_quad()

    inputTexture = pass.texture
```

代码示例：

```cpp
GLuint RetroShaderPipeline::process(GLuint input)
{
    GLuint current = input;

    for(auto& pass : passes)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pass.fbo);

        glUseProgram(pass.program);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, current);

        drawFullscreenQuad();

        current = pass.texture;
    }

    return current;
}
```

---

# 六、第五步：全屏 Quad

shader pass 本质就是：

```
输入纹理 → shader → 输出纹理
```

所以需要一个 full screen quad：

```cpp
void drawFullscreenQuad()
{
    static float quad[] =
    {
        -1, -1, 0,0,
         1, -1, 1,0,
         1,  1, 1,1,
        -1,  1, 0,1
    };

    glDrawArrays(GL_TRIANGLE_FAN,0,4);
}
```

---

# 七、第六步：最终纹理交给 NanoVG

NanoVG 可以直接用 OpenGL 纹理。

```cpp
int image = nvglCreateImageFromHandleGL3(
        vg,
        finalTexture,
        width,
        height,
        NVG_IMAGE_FLIPY
);
```

绘制：

```cpp
NVGpaint paint = nvgImagePattern(
        vg,
        0,
        0,
        width,
        height,
        0,
        image,
        1.0f
);

nvgBeginPath(vg);
nvgRect(vg,0,0,width,height);
nvgFillPaint(vg,paint);
nvgFill(vg);
```

---

# 八、第七步：GLSLP解析

RetroArch shader 实际是：

```
shader.glslp
```

例子：

```
shaders = 2

shader0 = crt-pass0.glsl
shader1 = crt-pass1.glsl

scale_type0 = source
scale0 = 2.0
```

解析后生成：

```
vector<ShaderPass>
```

---

# 九、第八步：Shader Uniform

RetroArch shader 会用到：

常见 uniform：

```
Source
SourceSize
OutputSize
FrameCount
```

例如：

```cpp
glUniform2f(sourceSizeLoc, width, height);
glUniform2f(outputSizeLoc, outW, outH);
glUniform1i(frameCountLoc, frameCount);
```

---

# 十、完整运行流程

每帧：

```
1 CPU生成 frame.pixels
2 上传到 gameTexture
3 shaderPipeline.process(gameTexture)
4 得到 finalTexture
5 NanoVG 显示 finalTexture
```

伪代码：

```cpp
void renderFrame()
{
    updateGameTexture(frame.pixels);

    GLuint finalTexture =
        shaderPipeline.process(gameTexture);

    drawWithNanoVG(finalTexture);
}
```

---

# 十一、Switch开发特别注意

Switch GLES：

```
GLSL ES 300
```

RetroArch shader：

```
GLSL 130
```

必须转换。

例如：

```
#version 130
```

改为：

```
#version 300 es
precision mediump float;
```

---

# 十二、你接下来需要实现的模块

建议顺序：

### 1

游戏纹理上传

### 2

FBO管理

### 3

shader编译器

### 4

glslp解析

### 5

shader pipeline

### 6

NanoVG显示

---

# 十三、推荐最终架构

你的项目结构建议：

```
renderer/

    GameTexture.cpp
    GameTexture.h

    FrameUploader.cpp
    FrameUploader.h

    ShaderCompiler.cpp
    ShaderCompiler.h

    GLSLPParser.cpp
    GLSLPParser.h

    RetroShaderPipeline.cpp
    RetroShaderPipeline.h

    FullscreenQuad.cpp
    FullscreenQuad.h
```

---

如果你愿意，我可以**给你一套完整的 RetroArch Shader Pipeline 实现（约2000行代码）**，包含：

* glslp解析器
* shader编译
* FBO管理
* multi-pass pipeline
* GLES兼容
* NanoVG显示

而且可以 **100% 直接接入你现在的 GameRuntime**。
