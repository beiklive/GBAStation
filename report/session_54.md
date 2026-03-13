# Session 54 – 工作报告

## 任务目标

为 mGBA libretro 核心添加以下功能：

1. **游戏存档（SRAM）自动读取与保存** — 在游戏启动时从磁盘加载 `.sav` 文件，退出时自动写回
2. **即时存档/读档** — 通过快捷键（默认 F5/F8）将游戏状态序列化到槽位文件
3. **金手指（Cheat）加载** — 启动游戏时自动加载与 ROM 同名的 `.cht` 金手指文件
4. **存档位置可配置** — SRAM 目录、即时存档目录、金手指目录均可在配置中指定（空字符串 = 与 ROM 同目录）

---

## 变更文件汇总

| 文件 | 操作 |
|------|------|
| `include/Retro/LibretroLoader.hpp` | 添加内存 API、金手指 API 及存档目录接口 |
| `src/Retro/LibretroLoader.cpp` | 实现新接口，修复环境回调中的存档目录返回值 |
| `include/Game/game_view.hpp` | 添加即时存档状态变量及新方法声明 |
| `src/Game/game_view.cpp` | 实现所有存档/读档/金手指逻辑，接入热键系统 |
| `report/session_54.md` | 本报告 |

---

## 详细变更说明

### 1. `LibretroLoader` 扩展

#### 新增方法（`include/Retro/LibretroLoader.hpp`）

**内存 API（用于 SRAM）**
```cpp
void*  getMemoryData(unsigned id) const;  // RETRO_MEMORY_SAVE_RAM
size_t getMemorySize(unsigned id) const;
```

**金手指 API**
```cpp
void cheatReset();
void cheatSet(unsigned index, bool enabled, const std::string& code);
```

**存档目录设置**
```cpp
void setSaveDirectory(const std::string& dir);   // RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY
void setSystemDirectory(const std::string& dir); // RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY
```

#### 实现（`src/Retro/LibretroLoader.cpp`）

- 添加 `fn_cheat_reset`、`fn_cheat_set`、`fn_get_memory_data`、`fn_get_memory_size` 函数指针
- 在 `load()` 中通过 `resolveSymbol()` 解析（可选符号，解析失败不中断加载）
- 在 Switch 静态链接路径中直接赋值对应的 `retro_*` 函数
- 将 `s_environmentCallback` 中的 `GET_SAVE_DIRECTORY` 和 `GET_SYSTEM_DIRECTORY` 分开处理，返回 `m_saveDirectory` / `m_systemDirectory` 的实际内容（非空时）

---

### 2. `GameView` 扩展

#### 新增成员变量（`include/Game/game_view.hpp`）

```cpp
std::atomic<int>  m_pendingQuickSave{-1};   // 待执行的存档槽（-1 = 无）
std::atomic<int>  m_pendingQuickLoad{-1};   // 待执行的读档槽（-1 = 无）
int               m_saveSlot = 0;            // 当前活动槽（0-based）
std::mutex        m_saveMsgMutex;
std::string       m_saveMsg;                 // 存档/读档提示信息
std::chrono::steady_clock::time_point m_saveMsgTime; // 信息显示时间
```

#### 新增方法（`include/Game/game_view.hpp`）

```cpp
static std::string resolveSaveDir(const std::string& romPath, const std::string& customDir);
std::string quickSaveStatePath(int slot) const;
std::string sramSavePath() const;
std::string cheatFilePath() const;
void loadSram();
void saveSram();
void doQuickSave(int slot);
void doQuickLoad(int slot);
void loadCheats();
```

#### 配置项默认值（`src/Game/game_view.cpp: initialize()`）

```ini
save.sramDir   = ""      # 空 = 与 ROM 同目录
save.stateDir  = ""      # 空 = 与 ROM 同目录
save.slotCount = 9       # 槽位数（界面层预留，当前槽固定为 0）
cheat.dir      = ""      # 空 = 与 ROM 同目录
```

#### 存档目录配置到核心

在 `initialize()` 中加载 ROM 之前，根据 `save.sramDir` 设置核心的存档目录（通过 `m_core.setSaveDirectory()`），并在需要时自动创建目录。

#### SRAM 生命周期

- 加载 ROM 后调用 `loadSram()` — 将磁盘上的 `.sav` 文件内容拷贝到核心的 `RETRO_MEMORY_SAVE_RAM` 内存区域
- 退出游戏（`cleanup()`）时调用 `saveSram()` — 将核心的 SRAM 内存区域写到磁盘

#### 即时存档/读档

- `doQuickSave(slot)` — 调用 `m_core.serialize()` 获取完整游戏状态并写入 `.stateN` 文件
- `doQuickLoad(slot)` — 读取 `.stateN` 文件并调用 `m_core.unserialize()` 恢复游戏状态，同时清空倒带缓冲区
- 游戏线程在每帧检测 `m_pendingQuickSave` / `m_pendingQuickLoad` 原子变量，有待处理请求时执行操作

**存档槽位命名规则：**
- slot 0 → `<游戏名>.state0`
- slot 1 → `<游戏名>.state1`
- …

#### 热键接入

**手柄热键（`registerGamepadHotkeys()`）**
```
QuickSave → m_pendingQuickSave.store(m_saveSlot)
QuickLoad → m_pendingQuickLoad.store(m_saveSlot)
```

**键盘热键（`pollInput()`）**
- 上升沿检测（按键刚按下时触发一次），避免长按重复触发
- 默认按键：F5 = 即时保存，F8 = 即时读取（可在配置文件中修改）

#### 金手指加载（`loadCheats()`）

加载 ROM 后自动寻找同名 `.cht` 文件，支持两种格式：

**RetroArch .cht 格式**（推荐）：
```ini
cheats = 2
cheat0_desc = "Infinite Lives"
cheat0_enable = true
cheat0_code = XXXXXXXX+YYYYYYYY
cheat1_desc = "Max Money"
cheat1_enable = false
cheat1_code = ZZZZZZZZ
```

**简单一行一码格式**：
```
# 注释
+XXXXXXXX YYYYYYYY   # + 前缀 = 启用
-XXXXXXXX YYYYYYYY   # - 前缀 = 禁用
XXXXXXXX YYYYYYYY    # 无前缀 = 默认启用
```

#### 存档/读档提示 Overlay

在 `draw()` 中增加半透明提示条，显示操作结果（如 `Saved (slot 0)` / `Loaded (slot 0)` / `No save in slot 0`），2 秒后自动消失。位置：屏幕底部居中。

---

## 文件路径规则

以 ROM `/path/to/game/MyGame.gba` 为例，默认配置（空目录）：

| 文件 | 路径 |
|------|------|
| SRAM | `/path/to/game/MyGame.sav` |
| 即时存档 槽0 | `/path/to/game/MyGame.state0` |
| 即时存档 槽1 | `/path/to/game/MyGame.state1` |
| 金手指 | `/path/to/game/MyGame.cht` |

若设置 `save.sramDir = /custom/saves`：

| 文件 | 路径 |
|------|------|
| SRAM | `/custom/saves/MyGame.sav` |

---

## 安全说明

- 所有文件 I/O 均在游戏线程（存档）或启动/关闭阶段（SRAM/金手指）执行，不会阻塞 UI 线程
- 存档目录在访问前自动创建（使用 `std::filesystem::create_directories` + 错误码，不会抛异常）
- `getMemoryData()` 返回的指针在游戏已卸载后失效，`saveSram()` 仅在 `m_core.isLoaded()` 为真且 ROM 路径非空时调用
- 金手指代码直接透传给 libretro 核心，不做额外处理；代码格式依赖于具体核心（mGBA 支持 GameShark/Action Replay）
- 未发现新增安全漏洞
