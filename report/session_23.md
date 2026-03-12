# example/shaders 使用时画面全黑问题修复报告

## 问题描述

使用 `example/shaders`（即 `resources/shaders/shaders_glsl/`）目录下的着色器预设，
例如 `F00_PhosphorLineReflex(Base).glslp`，游戏运行时画面全黑。
而 `resources/shaders/` 中的着色器（如 `crttt.glslp`、`example.glslp`）可以正常显示。

## 根本原因分析

### 两类着色器格式对比

| 着色器来源 | 格式 | 顶点坐标变换 |
|---|---|---|
| `resources/shaders/*.glsl` | `#pragma stage vertex/fragment` 格式 | `buildPragmaStageSrc()` 注入 `#define MVPMatrix mat4(1.0)` |
| `example/shaders/**/*.glsl` | RetroArch 标准格式 (`#if defined(VERTEX)`) | 声明 `uniform mat4 MVPMatrix;` — **真实 uniform** |

### 黑屏原因

RetroArch 格式的着色器（如 `phosphor-line.glsl`、`color-adjust.glsl`）在顶点着色器中这样计算位置：

```glsl
uniform mat4 MVPMatrix;

void main() {
    gl_Position = VertexCoord.x * MVPMatrix[0]
                + VertexCoord.y * MVPMatrix[1]
                + VertexCoord.z * MVPMatrix[2]
                + VertexCoord.w * MVPMatrix[3];
}
```

`beiklive` 的 `ShaderChain` 从未设置 `MVPMatrix` uniform。OpenGL 规范规定，
未设置的 `uniform mat4` 默认值为**全零矩阵**。

因此：
- `MVPMatrix` 全为零 → `gl_Position = vec4(0, 0, 0, 0)`（w = 0，NDC 中为无穷远点）
- 所有顶点退化到同一点 → 光栅化无输出 → **画面全黑**

`resources/shaders/` 中的着色器使用 `#pragma stage` 格式，`buildPragmaStageSrc()`
会注入 `#define MVPMatrix mat4(1.0)`，将 `MVPMatrix` 替换为宏（编译时常量），
所以它们不受此 bug 影响。

### 受影响范围

`example/shaders/` 目录下所有使用 `#if defined(VERTEX)` 格式且包含
`uniform mat4 MVPMatrix;` 的着色器，经统计约有 **46 个** GLSL 文件受影响。

## 修复方案

### 思路

`beiklive` 的顶点数据 (`k_quadVerts`) 已经是 NDC 空间坐标，
`MVPMatrix` 设为单位矩阵（identity matrix）即可让顶点坐标直通，
效果等同于 `gl_Position = VertexCoord`。

### 具体改动

**`include/Game/ShaderChain.hpp`**
- 在 `ShaderPass` 结构体中新增字段：
  ```cpp
  GLint  mvpMatrixLoc = -1; ///< uniform mat4 MVPMatrix（RetroArch 兼容 MVP 矩阵）
  ```

**`src/Game/ShaderChain.cpp`**

1. 新增全局静态常量（列主序单位矩阵）：
   ```cpp
   static const GLfloat k_mvpIdentity[16] = {
       1.f, 0.f, 0.f, 0.f,
       0.f, 1.f, 0.f, 0.f,
       0.f, 0.f, 1.f, 0.f,
       0.f, 0.f, 0.f, 1.f,
   };
   ```

2. 在 `_lookupUniforms()` 中查询 `MVPMatrix` uniform 位置：
   ```cpp
   p.mvpMatrixLoc = glGetUniformLocation(p.program, "MVPMatrix");
   ```

3. 在 `init()`（pass0）和 `addPass()`（用户通道）链接完成后，若存在 `MVPMatrix` uniform 则初始化为单位矩阵：
   ```cpp
   if (p.mvpMatrixLoc >= 0)
       glUniformMatrix4fv(p.mvpMatrixLoc, 1, GL_FALSE, k_mvpIdentity);
   ```

## 验证

- 项目在 Linux 平台编译通过（`cmake --build . -j$(nproc)`），无新增错误。
- `F00_PhosphorLineReflex(Base).glslp` 包含 5 个通道（`color-adjust.glsl`、
  `phosphor-line.glsl`、`diffusion-h.glsl`、`diffusion-v.glsl`、`PP-reflex.glsl`），
  所有通道均声明 `uniform mat4 MVPMatrix;`，修复后每个通道的 `MVPMatrix`
  都将被初始化为单位矩阵，顶点位置正确传递。

## 文件变更列表

| 文件 | 变更类型 | 说明 |
|---|---|---|
| `include/Game/ShaderChain.hpp` | 修改 | `ShaderPass` 新增 `mvpMatrixLoc` 字段 |
| `src/Game/ShaderChain.cpp` | 修改 | 新增 `k_mvpIdentity` 常量；`_lookupUniforms()` 查询 `MVPMatrix`；`init()` 和 `addPass()` 初始化单位矩阵 |
