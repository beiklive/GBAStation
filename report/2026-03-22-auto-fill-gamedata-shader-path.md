# 工作汇报：打开 GameView 时自动填充 gamedata 中的 shader.path

## 任务目标

在打开 GameView 时，若游戏的 gamedata 中 `shader.path` 字段为空，
则将对应平台的着色器路径（来自全局平台配置）回写到 gamedata。

---

## 任务分析

### 现有行为

`game_view.cpp` 初始化渲染链时（lines 375-391）：

1. 读取 gamedata 中的 `shader.path`（GAMEDATA_FIELD_SHADER_PATH）
2. 若为空，根据 ROM 扩展名（gba/gbc/gb）从全局配置读取平台专属路径
3. 若平台路径也为空，回退到全局通用着色器路径
4. **但不会将解析结果写回 gamedata**

### 问题

每次打开同一款游戏，`shader.path` 始终为空，总是从全局平台配置重新读取，
GameMenu 中显示的着色器路径也始终为"未设置"（因为 gamedata 里确实没有路径）。

### 修复方案

在找到平台对应的着色器路径（非空）后，立即调用 `setGameDataStr()` 将路径回写到
gamedata 的 `shader.path` 字段，使游戏拥有专属路径记录。

- **仅在平台路径非空时**写回（避免将空字符串写入 gamedata 造成"伪空"问题）
- **不写回**全局通用着色器路径（KEY_DISPLAY_SHADER_PATH），
  因为通用路径属于全局回退逻辑，不应绑定到单个游戏
- 写回后，下次打开同一游戏时 gamedata 已有路径，跳过平台解析逻辑

---

## 修改内容

### 修改文件

| 文件 | 修改内容 |
|------|---------|
| `src/Game/game_view.cpp` | 在平台路径解析成功后，调用 `setGameDataStr()` 将路径回写到 gamedata |

### 关键变更

```cpp
// 找到平台对应的着色器路径后（修改后新增）
if (!shaderPath.empty()) {
    setGameDataStr(m_romFileName, GAMEDATA_FIELD_SHADER_PATH, shaderPath);
}
```

---

## 验证

- Linux 平台编译通过（无新增编译错误）
- 逻辑：首次打开游戏 → gamedata shader.path 为空 → 从平台配置读取 → 回写 gamedata
- 再次打开：gamedata shader.path 非空 → 直接使用，无需再次解析
