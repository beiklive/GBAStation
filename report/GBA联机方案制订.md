结合你的项目（**Switch + libnx + Borealis + deko3d + 直接集成 mGBA 源码**），以及你的 UI 设计，我重新整理了一套**可以直接落地实施**的完整方案。

这套方案的设计目标：

* **第三方 mGBA 改动最少（推荐控制在 300 行以内）**
* **支持局域网自动发现**
* **以后可扩展公网联机**
* **以后可扩展 4 人 Link Cable**
* **以后可扩展 GBA Wireless Adapter**
* **网络层与模拟器完全解耦**

---

# 一、总体架构

整个项目新增一个 Network 模块。

```
Project
│
├── app
│
├── emulator
│   │
│   ├── GBAInstance.h
│   ├── GBAInstance.cpp
│   │
│   ├── MGbaCore.cpp
│   ├── MGbaCore.h
│   │
│   └── MGbaLinkAdapter.cpp      <-- mGBA适配层
│
├── network
│   │
│   ├── Packet.h
│   ├── Packet.cpp
│   │
│   ├── UdpSocket.h
│   ├── UdpSocket.cpp
│   │
│   ├── LanDiscovery.h
│   ├── LanDiscovery.cpp
│   │
│   ├── RoomManager.h
│   ├── RoomManager.cpp
│   │
│   ├── NetplayManager.h
│   ├── NetplayManager.cpp
│   │
│   ├── LinkTransport.h
│   ├── LinkTransport.cpp
│   │
│   └── RingQueue.h
│
└── third_party
    └── mgba
```

原则：

> **mGBA 不知道网络。**

> **Network 不知道 mGBA。**

二者仅通过 `MGbaLinkAdapter` 连接。

---

# 二、网络库

**仅使用 libnx BSD Socket。**

```
sys/socket.h

netinet/in.h

arpa/inet.h
```

不使用：

* ENet
* Asio
* Boost
* WebSocket
* libcurl

原因：

你的联机数据只有几十 Byte。

UDP 足够。

---

# 三、线程模型

整个模拟器只有两个线程参与联机。

```
Main Thread

↓

UI

↓

RunFrame()

↓

MGbaLinkAdapter
```

```
Network Thread

↓

recvfrom()

↓

Packet解析

↓

RingQueue
```

发送：

全部由 Main Thread 完成。

接收：

全部由 Network Thread 完成。

这样不会阻塞模拟。

---

# 四、联机状态机

```
Idle

↓

Hosting

↓

WaitingPlayer

↓

Connected

↓

LoadingGame

↓

WaitingReady

↓

Running

↓

Disconnected
```

所有 UI 都读取这一状态。

---

# 五、多人游戏 UI 流程

## 第一次进入

用户设置：

```
昵称

头像
```

保存：

```
Config.json
```

以后默认读取。

---

## 房主流程

```
多人游戏

↓

创建房间

↓

选择游戏

↓

读取GameDB.title

↓

计算CRC32

↓

开始广播房间

↓

等待玩家加入
```

UI：

```
房间名称

玩家名称

头像

游戏

等待中...
```

---

## 客户端流程

```
多人游戏

↓

自动扫描局域网

↓

显示房间列表

↓

点击加入
```

然后：

```
根据title搜索GameDB

↓

找到ROM

↓

计算CRC32

↓

一致

↓

发送Join

↓

加入成功
```

如果：

CRC不同：

```
版本不同

无法加入
```

---

# 六、LAN Discovery

Host：

每秒广播一次。

广播：

```
ROOM_INFO
```

客户端：

一直监听。

维护：

```
std::vector<RoomInfo>
```

超过：

```
3秒
```

没有广播：

自动删除。

---

# 七、RoomInfo

建议：

```
struct RoomInfo
{
    uint64_t roomId;

    char hostName[32];

    uint8_t avatar;

    char title[64];

    uint32_t crc32;

    uint8_t players;

    uint8_t maxPlayers;

    uint32_t ping;
};
```

注意：

不要使用：

```
IP

Port
```

作为唯一标识。

使用：

```
roomId
```

以后支持公网。

---

# 八、加入流程

客户端：

发送：

```
JoinRequest
```

Host：

回复：

```
JoinAccept
```

之后：

双方建立：

UDP通信。

但是：

**还没有启动游戏。**

---

# 九、开始游戏

只有：

Host：

有：

```
开始游戏
```

按钮。

点击以后：

发送：

```
StartGame
```

里面：

```
CRC32

Title

RandomSeed

RTC

PlayerInfo
```

客户端：

收到：

```
StartGame
```

以后：

自动：

```
Load ROM
```

初始化：

```
mGBA
```

完成以后：

发送：

```
READY
```

Host：

等待：

```
双方READY
```

全部：

Ready。

发送：

```
GO
```

双方：

同时：

Resume。

第一帧：

保持同步。

---

# 十、通信流程

真正运行以后：

```
RunFrame()

↓

SIO写寄存器

↓

mGBA Link

↓

MGbaLinkAdapter

↓

NetplayManager

↓

UdpSocket

================

UdpSocket

↓

NetworkThread

↓

ReceiveQueue

↓

MainThread

↓

MGbaLinkAdapter

↓

mGBA Link

↓

SIO

↓

CPU继续执行
```

整个：

UI：

完全不知道。

---

# 十一、Packet设计

统一：

```
PacketHeader
```

```
struct PacketHeader
{
    uint32_t magic;

    uint16_t version;

    uint16_t type;

    uint32_t sequence;

    uint32_t payloadSize;
};
```

Packet：

```
Discover

RoomInfo

JoinRequest

JoinAccept

JoinReject

Leave

StartGame

Ready

Go

Heartbeat

Disconnect

LinkData
```

以后：

所有平台：

NES

GBA

NDS

统一。

---

# 十二、LinkData

真正：

模拟通信：

只发送：

```
struct LinkDataPacket
{
    uint64_t cycle;

    uint16_t data;

    uint8_t flags;
};
```

不要：

同步：

```
CPU

RAM

Save

FrameBuffer
```

全部：

不需要。

---

# 十三、MGbaLinkAdapter

新增：

```
class MGbaLinkAdapter
{
public:

    void SendLinkData();

    void ReceiveLinkData();

};
```

Adapter：

负责：

```
mGBA

↓

Packet

↓

Network
```

反向：

```
Network

↓

mGBA
```

以后：

NDS：

也是：

Adapter。

---

# 十四、Network模块

建议：

职责：

## UdpSocket

只负责：

```
Open

Close

Send

Receive
```

不知道：

Room。

不知道：

mGBA。

---

## LanDiscovery

负责：

```
广播

扫描

维护房间列表
```

---

## RoomManager

负责：

```
创建房间

加入房间

离开房间

房间信息
```

---

## NetplayManager

整个：

联机入口。

负责：

```
StartHost()

Join()

Disconnect()

SendPacket()

PollPacket()
```

UI：

永远：

只操作：

NetplayManager。

---

# 十五、mGBA修改

推荐：

新增：

```
IMGbaLinkCallback
```

例如：

```
class IMGbaLinkCallback
{
public:

    virtual void SendLinkPacket(const void*,size_t)=0;
};
```

mGBA：

所有：

Link：

最终：

调用：

```
callback->SendLinkPacket()
```

收到：

网络：

以后：

调用：

```
InjectPacket()
```

即可。

整个：

third_party/mgba

修改：

控制：

300行以内。

以后：

升级：

mGBA：

非常容易。

---

# 十六、目录建议

```
network
│
├── Packet.*
├── PacketSerializer.*
│
├── UdpSocket.*
│
├── LanDiscovery.*
│
├── RoomManager.*
│
├── NetplayManager.*
│
├── LinkTransport.*
│
├── RingQueue.*
│
└── Heartbeat.*
```

模拟器：

```
emulator
│
├── MGbaCore.*
├── MGbaLinkAdapter.*
└── GBAInstance.*
```

---

# 十七、后续可扩展功能（预留）

这套架构建议在第一版就预留扩展点，后续无需重构：

* **4 人 Link Cable**：`RoomInfo.maxPlayers`、`PlayerId`、`LinkData` 均按最多 4 人设计。
* **GBA Wireless Adapter**：新增 `WirelessTransport`，不影响现有 `LinkTransport`。
* **公网联机**：保留 `UdpSocket` 接口，将来可增加 `RelayTransport` 或 `P2PTransport`，上层 `NetplayManager` 无需修改。
* **自动断线重连**：依赖 `RoomId` 和 `Heartbeat` 即可实现。
* **观战/录像**：控制消息和 Link 数据已经分离，可以单独扩展。

---

## 最终实施流程

```text
进入多人游戏
        │
        ▼
读取昵称/头像
        │
        ├───────────────┐
        │               │
        ▼               ▼
     创建房间        扫描房间
        │               │
        ▼               ▼
    选择 GBA 游戏     显示房间列表
        │               │
        ▼               ▼
读取 GameDB.title    点击加入
计算 CRC32            │
        │             ▼
        │      根据 title 在游戏库匹配
        │             │
        │      计算 CRC32 校验
        │             │
        │      成功后发送 JoinRequest
        │             │
        └──────► 房主 JoinAccept
                      │
                      ▼
                 双方进入 Connected
                      │
                      ▼
              房主点击【开始游戏】
                      │
                      ▼
                发送 StartGame
                      │
                      ▼
          双方自动加载同一 ROM
                      │
                      ▼
                 初始化 mGBA
                      │
                      ▼
               双方发送 READY
                      │
                      ▼
             Host 收到全部 READY
                      │
                      ▼
                  发送 GO
                      │
                      ▼
             双方同时 Resume()
                      │
                      ▼
       mGBA Link Cable ⇄ MGbaLinkAdapter ⇄ UDP Socket
                      │
                      ▼
                开始正常联机通信
```
