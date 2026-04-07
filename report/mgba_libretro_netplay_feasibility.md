# 基于 mgba libretro 实现联机功能可行性分析报告

> 日期：2026-04-07  
> 作者：Copilot Agent  

---

## 一、背景与目标

GBAStation 是一款运行在 Nintendo Switch 和 Windows 上的 GBA/GB/GBC 模拟器前端，
底层核心为 [mgba](https://mgba.io/)，接口层使用 libretro API。  
本报告评估在当前架构下实现"联机对战/联机游玩"功能的技术可行性。

---

## 二、GBA 联机的硬件基础

### 2.1 GBA 实体联机方式

GBA 通过**串行通信口（SIO）**实现多人游戏：

| 通信模式 | 说明 | 典型应用 |
|----------|------|---------|
| **Multiplayer（多人）** | 最多 4 台 GBA，主从模式，速率 115200bps | 宝可梦对战/交换、口袋妖怪双打 |
| **Normal（普通）** | 点对点，8/32 位传输 | 少数游戏 |
| **UART** | 全双工串行 | GameBoy Player 等 |
| **JOY BUS** | Nintendo GameCube 链接线 | GBA–GameCube 连接游戏 |

联机游戏使用最多的是 **SIO_MULTI（多人模式）**，主机控制传输时序，所有从机同步接收。

### 2.2 mgba 的 SIO 驱动架构

mgba 通过可插拔的 `GBASIODriver` 接口抽象底层 SIO 传输：

```c
// third_party/mgba/include/mgba/gba/interface.h
struct GBASIODriver {
    struct GBASIO* p;
    bool (*init)(struct GBASIODriver* driver);
    void (*deinit)(struct GBASIODriver* driver);
    bool (*load)(struct GBASIODriver* driver);
    bool (*unload)(struct GBASIODriver* driver);
    uint16_t (*writeRegister)(struct GBASIODriver* driver, uint32_t address, uint16_t value);
};
```

mgba 内置了**锁步（Lockstep）驱动** `GBASIOLockstep`，用于在同一进程内或同一机器上模拟多台 GBA 互联：

```c
// third_party/mgba/include/mgba/internal/gba/sio/lockstep.h
struct GBASIOLockstep {
    struct mLockstep d;              // 锁步基础结构（含 signal/wait）
    struct GBASIOLockstepNode* players[MAX_GBAS];  // 最多 MAX_GBAS 台 GBA
    uint16_t multiRecv[MAX_GBAS];   // 多人模式接收缓冲
    uint32_t normalRecv[MAX_GBAS];  // 普通模式接收缓冲
};
```

`mLockstep` 提供了以下核心回调：

| 回调 | 用途 |
|------|------|
| `lock` / `unlock` | 保护传输状态的互斥锁 |
| `signal` | 通知其他节点进入下一传输阶段 |
| `wait` | 等待其他节点响应 |
| `addCycles` / `useCycles` | 多核之间的时序对齐 |

---

## 三、实现联机的两种技术路线

### 方案 A：基于 GBASIOLockstep 的网络延伸（真实 SIO 仿真）

**核心思路**：将 `mLockstep` 的 `signal`/`wait` 替换为网络 Socket 实现，
让运行在不同机器上的 mgba 核心共享同一个"虚拟 GBA Link Cable"。

```
[设备 A：GBAStation]                [设备 B：GBAStation]
  GBA Core 1 (主)                     GBA Core 2 (从)
     ↓                                     ↓
GBASIOLockstepNode                 GBASIOLockstepNode
     ↓                                     ↓
  [Network SIO Driver]  ←TCP/UDP→  [Network SIO Driver]
```

**实现步骤**：
1. 基于 `mLockstep` 实现一个网络驱动 `mNetworkLockstep`：
   - `signal()` → 向对端 Socket 发送"ready"包
   - `wait()` → 阻塞读取对端"ready"包
   - `addCycles` / `useCycles` → 本地帧计数补偿
2. 将 `GBASIOLockstepNode` 挂接到网络驱动的 `GBASIOLockstep`
3. 两台设备通过 TCP/UDP 互联，同步 SIO 帧数据

**优点**：
- 游戏兼容性高（与真实 GBA 链接行为一致）
- 支持宝可梦对战/交换等 SIO_MULTI 类游戏
- mgba 已提供 socket 工具库（`mgba-util/socket.h`），跨平台封装完善

**缺点**：
- 需要严格的时序同步（GBA SIO 对延迟极敏感，>16ms 可能导致超时断线）
- 需要实现网络时钟对齐和帧步进补偿
- 需要修改 mgba 核心的 lockstep 驱动（核心层改动）

---

### 方案 B：libretro 回滚联机（Rollback Netplay）

**核心思路**：在 libretro 层面实现输入共享与状态回滚，无需修改 mgba 核心。
RetroArch 的 Netplay 模块（`libretro_netplay`）就是此类方案。

```
[设备 A：GBAStation]                [设备 B：GBAStation]
   每帧运行核心                         每帧运行核心
   发送本地输入  ←──── TCP ────→   发送本地输入
   接收对端输入                         接收对端输入
   如输入帧延迟 → 回滚到存档点重演      如输入帧延迟 → 回滚到存档点重演
```

**实现步骤**：
1. 在 `GameView::_gameLoop()` 中，每帧：
   - 将本地手柄输入通过 Socket 发送到对端
   - 接收对端输入（若未到达则预测/延迟一帧）
2. 使用 `Serialize`/`Unserialize`（已实现）定期保存状态点
3. 当对端输入到达且与预测不一致时，从最近存档回滚并重演

**优点**：
- 不修改 mgba 核心，维护成本低
- 通用性好（适用于 GB/GBC/GBA 所有游戏，不依赖 SIO）
- GBAStation 已有 Serialize/Unserialize 支持（倒带功能基础）

**缺点**：
- 只支持"相同输入 → 确定性相同状态"的游戏（GBA 大多数游戏满足）
- 对于不同步 SIO 的联机游戏（如宝可梦对战），联机功能无意义（只是画面同步，实际 SIO 不连通）
- 回滚深度与延迟直接影响用户体验

---

## 四、现有代码基础评估

| 功能需求 | 现有基础 | 完成度 |
|----------|----------|--------|
| GBA 核心接口封装 | `CoreMgba` / `LibretroLoader` | ✅ 完善 |
| 序列化/反序列化 | `Serialize`/`Unserialize`（倒带） | ✅ 完善 |
| 网络 Socket 工具 | `mgba-util/socket.h`（跨平台） | ✅ 可用 |
| SIO 驱动接口 | `GBASIODriver`/`GBASIOLockstep` | ✅ 存在 |
| 输入系统 | `GameInputManager` + `GameSignal` | ✅ 完善 |
| Switch 网络权限 | 需要 `nifm` 服务（已有 Wireless widget） | ✅ 框架已有 |
| 联机 UI | 无 | ❌ 待开发 |
| 网络传输层 | 无 | ❌ 待开发 |

---

## 五、技术挑战

### 5.1 Switch 平台网络限制

- Switch 需要申请网络权限（`nifmInitialize()`，borealis wireless widget 已使用）
- P2P 直连需要双方在同一局域网或支持 NAT 穿透
- Switch 无法轻松开放端口，广域网对战需要中继服务器

### 5.2 延迟与同步

- GBA 以 ~59.7fps 运行，每帧约 16.7ms
- SIO Multiplayer 模式的超时通常在 2~8 个 CPU 帧（约 0.5ms~2ms）
- 方案 A 对延迟极敏感，局域网（<1ms）可行，广域网（>50ms）基本不可行
- 方案 B 回滚联机容忍 30~60ms 延迟，广域网可行

### 5.3 游戏确定性

- GBA 模拟本身是确定性的（相同输入→相同状态）
- 但音频/定时器浮点差异可能在不同机器上产生微小差异
- mgba 的 Serialize/Unserialize 可解决此问题（双方同步状态）

---

## 六、推荐方案与实现优先级

### 推荐：方案 B（libretro 回滚联机）+ 有限 SIO 仿真

**第一阶段（局域网对战，较快实现）**：
1. 实现基于 UDP 的输入共享协议（每帧发送输入帧号 + 按键状态）
2. 在 `_gameLoop` 中集成帧同步逻辑（延迟帧或回滚）
3. 开发联机房间 UI（创建/加入房间）

**第二阶段（SIO 联机，支持宝可梦等）**：
1. 实现 `mNetworkLockstep` 驱动（基于 mgba socket 工具）
2. 通过 `GBASIOSetDriverSet` 挂接网络 SIO 驱动
3. 仅在局域网环境下支持（广域网延迟不满足 SIO 时序要求）

### 工作量估算

| 任务 | 工作量 |
|------|--------|
| 网络传输层（UDP 帧同步） | 1~2 周 |
| 回滚逻辑集成 | 1 周 |
| 联机 UI（房间/匹配） | 1 周 |
| SIO 网络驱动实现 | 2~3 周 |
| Switch NAT 穿透/中继 | 2~4 周（可选）|

---

## 七、结论

| 维度 | 评估 |
|------|------|
| **技术可行性** | ✅ 可行，mgba 有完善的 SIO/Lockstep 接口和 Socket 工具 |
| **libretro 层回滚联机** | ✅ 可行，难度较低，不修改核心 |
| **真实 SIO 网络仿真** | ⚠️ 可行但复杂，需修改 mgba 核心，仅适合局域网 |
| **广域网支持** | ⚠️ 需要 STUN/中继服务器，复杂度高 |
| **推荐优先级** | 先实现方案 B（回滚联机），再考虑方案 A（SIO 仿真） |

基于 mgba libretro 实现联机功能是**技术上完全可行的**。
建议以"局域网回滚输入共享"作为第一个里程碑，逐步迭代到广域网对战和真实 SIO 仿真。

---

## 参考资料

- mgba 源码：`third_party/mgba/include/mgba/internal/gba/sio/lockstep.h`
- mgba SIO 驱动接口：`third_party/mgba/include/mgba/gba/interface.h`
- mgba socket 工具：`third_party/mgba/include/mgba-util/socket.h`
- RetroArch Netplay 文档：https://docs.libretro.com/development/cores/developing-cores/#netplay
- GGPO 回滚联机框架：https://www.ggpo.net/
