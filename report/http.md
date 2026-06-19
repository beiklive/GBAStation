

# GBAStation Web 管理服务实施方案

目标：

实现一个内置 HTTP 服务，支持：

* 浏览器访问 Switch
* 上传 ROM 到switch并自动重命名为拼音，并添加到游戏库
* 管理游戏库，浏览库中所有游戏（名称、游玩时间、次数、平台、封面）
* 获取封面
* 删除游戏

要求：

* Switch(libnx) 与 Windows 共用代码
* 非阻塞
* 大文件流式上传
* 不影响模拟器帧循环
* 后期支持 WebSocket

技术选型：

```text
网络层:
Mongoose

数据库:
nlohmann/json

序列化:
nlohmann/json

线程:
独立HTTP线程
```

第三方目录：

```text
third_party/

├── mongoose/
│   ├── mongoose.c
│   └── mongoose.h
```
nlohmann/json目录：
BeikLiveStation\src\core\json.hpp


CMake：

```cmake
add_library(gbastation_http

    network/HttpServer.cpp
    network/ApiRouter.cpp
    network/UploadApi.cpp
    network/GameApi.cpp
    network/SystemApi.cpp
    network/WebSocketApi.cpp

    third_party/mongoose/mongoose.c
)

target_include_directories(
    gbastation_http
    PUBLIC
    network
    third_party/mongoose
)

target_link_libraries(
    gbastation_http
    nx
)
```

# 模块结构

```text
src/

└── network/
   ├── HttpServer.h
   ├── HttpServer.cpp
   │
   ├── ApiRouter.h
   ├── ApiRouter.cpp
   │
   ├── UploadApi.cpp
   ├── GameApi.cpp
   ├── SystemApi.cpp
   ├── WebSocketApi.cpp
   │
   └── HttpTypes.h
resources/
├── web/
    ├── index.html
    ├── app.js
    ├── style.css
```

# HttpServer

接口：

```cpp
class HttpServer
{
public:

    bool Start(
        int port=8080);

    void Stop();

    void Update();

private:

    mg_mgr m_mgr;

};
```

实现：

```cpp
bool HttpServer::Start(int port)
{
    mg_mgr_init(&m_mgr);

    std::string url=
        "http://0.0.0.0:"+
        std::to_string(port);

    mg_http_listen(
        &m_mgr,
        url.c_str(),
        EventHandler,
        this
    );

    return true;
}

void HttpServer::Update()
{
    mg_mgr_poll(
        &m_mgr,
        1
    );
}
```

# Switch 网络初始化

新增：

```cpp
class NetworkManager
{
public:

    bool Initialize();

    void Shutdown();

private:

    void* m_socketBuffer;
};
```

实现：

```cpp
#define SOCKET_BUFFER_SIZE 0x100000

bool NetworkManager::Initialize()
{
    m_socketBuffer=
        memalign(
            0x1000,
            SOCKET_BUFFER_SIZE
        );

    SocketInitConfig cfg={};

    cfg.tcp_tx_buf_size=0x8000;
    cfg.tcp_rx_buf_size=0x10000;

    socketInitialize(
        &cfg
    );

    return true;
}
```

启动：

```cpp
network.Initialize();

http.Start(8080);
```

# API设计

## 获取游戏列表

请求：

```text
GET /api/games
```

返回：
参考build_windows\GBAStation\data\GameData_GBA.json
```json
    {
        "cheatPath": "",
        "crc32": 0,
        "customOffsetX": 0.0,
        "customOffsetY": 0.0,
        "customScale": 1.0,
        "displayMode": 0,
        "favourite": false,
        "integerAspectRatio": 0.0,
        "lastPlayed": "",
        "logoPath": "./resources/img/ui/gba.png",
        "overlayEnabled": true,
        "overlayPath": "",
        "path": "E:\\BaiduNetdiskDownload\\GBA中文游戏黄金典藏版\\gba_zhong_wen_you_xi_huang_jin_dian_cang_ban_8d\\gba_dian_cang_e_mo_cheng_xi_lie_3_zuo_d7\\e_mo_cheng_bai_ye_xie_zou_qu_11.gba",
        "platform": 1,
        "playCount": 0,
        "playTime": 0,
        "savePath": ".\\GBAStation\\saves\\dirms\\e_mo_cheng_bai_ye_xie_zou_qu_11",
        "screenShotPath": "",
        "shaderEnabled": false,
        "shaderParaNames": [],
        "shaderParaValues": [],
        "shaderPath": "",
        "title": "e_mo_cheng_bai_ye_xie_zou_qu_11"
    }
```

---

## 上传游戏

请求：

```text
POST /api/upload
```

上传：

```text
multipart/form-data
```

保存：

```text
sdmc:/GBAStation/roms/
```

实现：

```cpp
void UploadApi::Handle(
    mg_http_message* hm)
{
    while(receive data)
    {
        fwrite(
            chunk,
            size,
            1,
            file
        );
    }
}
```

要求：

禁止：

```cpp
vector<uint8_t> allData;
```

必须：

```cpp
recv
    →
fwrite
```

原因：

Switch内存有限。

---

## 删除游戏

```text
DELETE /api/game/{id}
```

流程：

```text
提示从游戏库中移除（可选是否删除rom文件）

↓

删除数据库记录（删除rom文件）

↓

刷新缓存和GameDB
```

---

## 获取封面

```text
GET /api/cover/{id}
```

返回：

```text
image/png
```

目录：

根据游戏数据的logoPath字段获取图片

---

## 扫描游戏

```text
POST /api/rescan
```

流程：

```text
选择目录，扫描目录

↓

识别文件

↓

生成元数据

↓

更新数据库
```

支持：

```text
.gba
.gb
.gbc
.nes
.sfc
```

---


# 前端

目录：

```text
sdmc:/GBAStation/web/
```

页面：

```text
主页
    ↓
游戏库管理

导入游戏
```

首页布局：

```text
┌─────────────────────────┐
│ GBAStation Web Manager │
├─────────────────────────┤
│ 搜索框                  │
├─────────────────────────┤
│ 游戏封面 游戏名 删除    │
│ 游戏封面 游戏名 删除    │
│ 游戏封面 游戏名 删除    │
├─────────────────────────┤
│ 上传按钮 重新扫描       │
└─────────────────────────┘
```

# 主循环接入

不要阻塞：

错误：

```cpp
while(true)
{
    mg_mgr_poll(
        &mgr,
        1000
    );
}
```

正确：

```cpp
while(appletMainLoop())
{
    http.Update();

    emulator.Update();

    ui.Update();

    renderer.Render();
}
```

# 后续扩展预留

```text
HTTP
    ↓
REST API
    ↓
WebSocket
    ↓
手机控制器
    ↓
网页存档管理
    ↓
在线封面下载
    ↓
网页截图
    ↓
远程启动游戏
```

实施要求：

1. 不允许阻塞模拟器线程
2. 上传必须流式写入
3. 不允许全文件加载内存
4. HTTP模块与平台解耦
5. libnx只负责socket初始化
6. 所有API统一走ApiRouter
7. 所有返回JSON统一封装
8. 后期可直接迁移Windows平台

这一版已经足够让 Codex 直接开始生成工程。
