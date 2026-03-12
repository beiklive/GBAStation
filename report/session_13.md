# 着色器白屏修复 & 参数存储 API 报告

## 任务背景

前次任务中，passthrough.glsl / sharpen.glsl / scanlines.glsl 三个着色器效果正常，
但使用其他 RetroArch 着色器（如 phosphor-line v2.0）时，游戏画面仍然全白。

本次任务解决两个需求：

1. **修复白屏**：找出 phosphor-line 等 RA 着色器白屏的根本原因并修复
2. **参数存储 API**：将所有可调参数（含完整元数据）保存到着色器链中，为后续实时调整面板提供支持

---

## 一、白屏根本原因分析

### 问题：`OrigInputSize` 未绑定

`phosphor-line.glsl`（及 phosphor-line v2.0 系列的多个 pass）使用了
RetroArch 的 `OrigInputSize` uniform：

```glsl
uniform COMPAT_PRECISION vec2 OrigInputSize;

void main() {
    vec4 q = vec4(OrigInputSize, 1.0/OrigInputSize);  // 1/0 = INF！
    float multi_y0 = OutputSize.y * q.w;              // OutputSize.y * INF = INF
    float multi_y1 = multi_y0 * PIC_SCALE_Y;          // INF
    vParameter2.x = max(floor(multi_y1 * ...), 1.0);  // floor(INF) = INF
    vParameter.y  = multi_y0 / vParameter2.x;         // INF / INF = NaN
    ...
}
```

`OrigInputSize` 代表着色器链入前的原始视频分辨率（即 emulator 输出尺寸）。
在我们的实现中，该 uniform 从未被设置，GLSL uniform 默认值为 `(0,0)`，
导致 `1.0/OrigInputSize = (INF, INF)`，进而使顶点着色器传递给片段着色器的
vParameter/vParameter2 包含 NaN/INF → UV 坐标错误 → **全白画面**。

### 受影响的着色器

phosphor-line v2.0 系列中以下 pass 文件均使用 `OrigInputSize`：

| 文件 | 用途 |
|------|------|
| `phosphor-line.glsl` | `vec4 q = vec4(OrigInputSize, 1.0/OrigInputSize)` |
| `PP-reflex.glsl` | `#define bbb vec4(OrigInputSize, 1.0/OrigInputSize)` |
| `PP-diffusion.glsl` | `#define ccc vec4(OrigInputSize, 1.0/OrigInputSize)` |
| `post-process.glsl` | `vec4 b = vec4(OrigInputSize, 1.0/OrigInputSize)` |
| `shadowmask.glsl` | `#define OriginalSize vec4(OrigInputSize, 1.0/OrigInputSize)` |
| `pl-hc.glsl` | `vec4 n = vec4(OrigInputSize, 1.0/OrigInputSize)` |
| `CA-sharpen.glsl` | `OrigInputSize.x/OrigInputSize.y` 计算宽高比 |

---

## 二、修复方案

### 1. 添加 `origInputSizeLoc` 字段（`ShaderPass`）

在 `include/Game/ShaderChain.hpp` 的 `ShaderPass` 中新增：

```cpp
GLint  origInputSizeLoc  = -1; ///< uniform vec2 OrigInputSize（原始视频分辨率）
```

### 2. 查找 `OrigInputSize` uniform 位置（`_lookupUniforms`）

```cpp
p.origInputSizeLoc = glGetUniformLocation(p.program, "OrigInputSize");
```

### 3. 每帧设置 `OrigInputSize` uniform（`run()`）

```cpp
if (p.origInputSizeLoc >= 0)
    glUniform2f(p.origInputSizeLoc, (float)videoW, (float)videoH);
```

`OrigInputSize` 始终等于原始视频尺寸（即着色器链的输入 `videoW × videoH`），
对所有通道均如此——与 RetroArch 行为一致。

---

## 三、参数存储 API

### 新增 `ShaderParamDef` 结构体

```cpp
struct ShaderParamDef {
    std::string name;        // 参数名（对应 GLSL uniform）
    std::string desc;        // 用户可读描述
    float       defaultVal;  // 着色器文件中声明的默认值
    float       minVal;      // 最小值
    float       maxVal;      // 最大值
    float       step;        // UI 步进值
    float       currentVal;  // 当前运行时值
};
```

### `ShaderChain` 新增 API

| 方法 | 说明 |
|------|------|
| `const std::vector<ShaderParamDef>& params() const` | 获取所有参数定义（只读） |
| `void setParamDefs(const std::vector<ShaderParamDef>& defs)` | 批量设置参数定义（由 GlslpLoader 调用） |
| `bool setParam(const std::string& name, float val)` | 设置单个参数值，立即更新所有通道的 uniform |

### `GlslpLoader` 改进

- 将 `extractParamDefaults()` 替换为 `extractParamDefs()`，返回完整 `ShaderParamDef` 列表
- `loadGlslpIntoChain()`：第一轮收集时同时存储完整元数据；加载完成后调用 `chain.setParamDefs(allParamDefs)`
- `loadGlslIntoChain()`：同样提取完整定义并调用 `chain.setParamDefs()`

### 参数合并策略

| 情况 | 策略 |
|------|------|
| 同一参数在多个 pass 文件中声明 | **第一次出现**的定义为准（保留完整元数据） |
| 预设文件的全局覆盖（`CORNER = "0.3"` 等） | 更新 `currentVal`，最高优先级 |
| `addPass()` 的 uniform 初始化 | 使用 `currentVal`（含预设覆盖） |

---

## 四、修改文件清单

| 文件 | 修改内容 |
|------|---------|
| `include/Game/ShaderChain.hpp` | 新增 `ShaderParamDef` 结构体；`ShaderPass` 新增 `origInputSizeLoc`；`ShaderChain` 新增 `m_params`、`params()`、`setParamDefs()`、`setParam()` |
| `src/Game/ShaderChain.cpp` | `_lookupUniforms()` 新增 `OrigInputSize` 查找；`run()` 中新增 `OrigInputSize` uniform 设置；新增 `setParam()` 实现 |
| `src/Game/GlslpLoader.cpp` | `extractParamDefaults()` 替换为 `extractParamDefs()`（返回完整 `ShaderParamDef`）；`loadGlslpIntoChain()` 和 `loadGlslIntoChain()` 在加载完成后调用 `chain.setParamDefs()` |

---

## 五、`setParam()` 使用示例

```cpp
// 获取所有可调参数（用于 UI 面板渲染）
const auto& params = m_shaderChain.params();
for (const auto& p : params) {
    // 渲染一个滑块
    renderSlider(p.name, p.desc, p.currentVal, p.minVal, p.maxVal, p.step);
}

// 用户修改参数后立即生效（下一帧自动应用）
m_shaderChain.setParam("GAMMA", 2.4f);
m_shaderChain.setParam("LINE_THICKNESS", 0.8f);
```

---

## 六、验证

- **构建**：Linux Release 模式编译通过，无新增错误
- **逻辑验证**：
  - `OrigInputSize` 现在始终设置为原始视频尺寸 → `1/OrigInputSize` 不再为 INF → 顶点着色器计算正确
  - `ShaderParamDef` 完整存储了 `#pragma parameter` 的所有字段
  - `setParam()` 会更新 `m_params` 中的 `currentVal` 并立即对所有通道调用 `glUniform1f`
  - 预设文件中的 `CORNER = "0.3"` 等全局覆盖值会更新 `currentVal` 并通过 `addPass()` 的 `paramDefaults` 映射应用

---

## 七、已知局限（可后续改进）

1. `OriginalAspect` uniform（color-adjust.glsl 中使用）未设置，但有 fallback 逻辑 `OriginalAspect < 0.0001` → 回退到 `OrigInputSize.x/OrigInputSize.y`（已通过本次修复正确设置 OrigInputSize，此 fallback 路径可正常工作）
2. `setParam()` 直接调用 `glGetUniformLocation` 查询位置，可通过缓存优化
3. `m_params` 在 `clearPasses()` 时未清空（可考虑是否需要清空）
