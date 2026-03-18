# Session 108 工作汇报

## 任务目标

根据前几个 session 的分析，修复 scalefx 着色器（scale4=3.0）导致游戏画面显示异常（视觉撕裂）的问题。

## 问题分析

### 根因

1. **scalefx 着色器管线**：5 个通道，pass0–3 scale=1.0（source），pass4 scale=3.0（source）
   - GBA 游戏 240×160 → pass4 输出 720×480

2. **NVG 非整数缩放**：`computeRect()` 使用原始游戏尺寸（240×160）计算显示矩形
   - 整数缩放模式 × 4 → 960×640 显示矩形
   - NVG 将 720×480 FBO 纹理映射到 960×640 矩形 → 缩放因子 = 4/3（非整数！）

3. **子像素网格破坏**：scalefx pass4 精心设计的 3×3 子像素网格（near-B / E中心 / near-H 各占一行）
   被 GL_NEAREST 的非均匀采样打乱：某些行占 2 个屏幕像素，某些只占 1 个，
   导致像素行"撕裂"。

### 修复方案

在 `game_view.cpp` 的 NVG 渲染区域，对 source/absolute-scale 着色器（输出尺寸 ≠ 视口大小）
使用着色器实际输出尺寸作为 `computeRect()` 的内容尺寸参考：

- **无着色器 / viewport-scale 着色器**（`shOutW == viewW && shOutH == viewH`）：
  保持原行为，使用原始游戏尺寸（避免 computeRect 算出全屏矩形）。

- **source/absolute-scale 着色器**（如 scalefx 3×，`shOutW ≠ viewW`）：
  使用着色器输出尺寸（如 720×480），computeRect 整数缩放得到 1× = 720×480 矩形，
  NVG 以 1:1 精确映射，子像素网格完整保留。

## 修改文件

- `src/Game/game_view.cpp`：
  - 约第 2283 行附近的 NVG 渲染块
  - 新增 `if (m_renderChain.hasShader())` 条件块，
    检测 shader 输出是否非视口尺寸，若是则覆盖 contentW/contentH

## 效果验证

| 模式 | 修复前（原始 240×160） | 修复后（shader 输出 720×480） |
|------|----------------------|---------------------------|
| 整数缩放 | 4× → 960×640，NVG 4/3×（非整数撕裂） | 1× → 720×480，NVG 1:1（完美对齐） |
| 适应模式 | 4.5× → 1080×720，NVG 1.5× | 1.5× → 1080×720，NVG 1.5×（结果相同） |
| 视口着色器 | 不受影响（shOutW==viewW 走旧路径） | 同左（保持兼容） |

## 安全说明

本次修改不涉及安全敏感代码，无新引入安全漏洞。
