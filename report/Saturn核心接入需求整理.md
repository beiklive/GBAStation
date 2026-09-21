# Saturn（YabaSanshiro）接入 GBAStation —— 实施方案 v3.1

本版落实你的 5 条决定，并新增**菜单双设置模型（全局 / 独立 + `noSync`）**的完整规格。
**v3.1 已把全部待确认项（原 §8 的 N1–N7）按建议锁定**，并在 §12 追加 M1（启动接线 + 配置路径）逐文件执行清单。
文末附审查结论。**本文不含代码改动。**

* 核心仓库（权威）：`yabasanshiro/yabause`（`gsnx_yabause/yabause` 为旧基座，仅参考）
* 前端仓库：`GBAStation`
* 行号相对 `/Users/beiklive/Code/C++`

---

## 0. 本版变更（v2 → v3）

| # | 变更 | 来源 |
|---|---|---|
| 1 | **`DataRoot` 迁移**：用户可见数据（配置/存档/即时档/截图/BIOS 引用）统一到共享目录，核心私有目录只留 log / cache | 你的第 1 条 |
| 2 | **无 GameDB 记录时**：用回退路径、跳过统计写入、**不新建记录** | 你的第 2 条 |
| 3 | **槽位命名 `.<N>` 系**：`<stem>.ss1…ss10` + `.png` 缩略图 | 你的第 3 条 |
| 4 | **快进**：音频**变调**（不是静音），且**触发方式要改造**（详见 §5.4） | 你的第 4 条 |
| 5 | **菜单改为 7 个左侧 Tab**，新增「独立设置 / 全局设置」双设置模型 + `noSync` 运行时切换 | 你的第 5 条 |
| 6 | **新验证**：前端 GameDB **会丢弃未知字段**（`GBAStation/src/core/game_database.cpp:342-396` 只按固定 `GameEntry` 字段读写）→ 核心**不得**往 `GameData_Saturn.json` 加自定义键；这正好证明"独立设置放 `savePath`"是必需设计 | 本轮验证 |
| 7 | **锁定全部待确认项**：独立文件名 `saturn.settings.cfg`、关开关保留文件、未启用时置灰、旧式 forwarder 不处理、滤镜只做内置档位、做截图、退出前自动存档 | 你的确认「都用建议」 |

---

## 1. 你的 5 条决定 → 落地位置

| 你的决定 | 落地 | 章节 |
|---|---|---|
| 1. 按建议迁移 `DataRoot` | 共享目录承载用户数据；`<DataRoot>` 只放 `log/`、`cache/` | §7.1 |
| 2. 无记录时按建议（回退 + 不写统计 + 不建记录） | §7.2、§6.5-D6 | §7.2 |
| 3. 用 `.ss<N>` | `<savePath>/<stem>.ss1…10` + `<savePath>/<stem>.ss1.png…`；旧 `.state1` 做兼容读取 | §5.3 |
| 4. 快进变调（+ 触发方式改造） | 读 `saturn.handle.fastforward` + `fastforward.mode/multiplier/mute`；变调实现见 §5.4 | §5.4 |
| 5. 7 个 Tab + 独立/全局设置 | §2 全章 | §2 |

---

## 2. 菜单结构与设置模型（本版核心）

### 2.1 左侧 7 个 Tab

| # | Tab | 内容 |
|---|---|---|
| 1 | **返回游戏** | 无内容页，确认直接关闭菜单 |
| 2 | **保存状态** | 10 槽列表：槽号 + 存档时间 + 缩略图；`A` 保存、`X` 删除 |
| 3 | **读取状态** | 同一份 10 槽列表；无档的槽不可选（提示"空"） |
| 4 | **独立设置** | 第一项 = **启动独立设置开关（`noSync`）**；下面是设置项，与「全局设置」**完全相同**；开启时读写 `<savePath>` 下的独立配置文件 |
| 5 | **全局设置** | 同一组设置项；对应 `GameData_Saturn.json` 中该游戏的设置（+ 平台级 `config.cfg` 键） |
| 6 | **重置游戏** | 二次确认后原地重置（重新 init 模拟器并载入当前 ROM，不重启进程） |
| 7 | **退出游戏** | 退出并链回启动器（仅当有 `--return`）；退出前自动存档（读 `save.autoSaveOnExit`，**已定 N7**） |

底部保留提示栏（`A 确定 / B 返回 / X 删除`）；打开方式改为 `saturn.hotkey.menu.pad`（默认 `PAD_LT+PAD_RT`），不再硬编码 ZL+ZR。

### 2.2 两套设置存储

| | 全局设置 | 独立设置 |
|---|---|---|
| 语义 | **启动器也能看到/编辑**的该游戏设置 | **核心私有**的该游戏设置覆盖层 |
| 存储 | `GameData_Saturn.json` 的该游戏字段 + `config.cfg` 的平台级键 | `<savePath>/saturn.settings.cfg`（一份完整快照） |
| 谁可改 | 前端每游戏菜单 + 核心菜单 | 只有核心菜单 |
| 生效条件 | `noSync == 0` | `noSync == 1` |

**独立设置文件**：`<savePath>/saturn.settings.cfg`（**已定 N1**）。格式与 `config.cfg` 同编码 `key=<type>|<value>`（解析/写回代码可直接复用），一行一键，未知键保留。

**为什么必须放 `savePath` 而不是 GameData**（本轮验证）：前端 `GameEntry` 是固定结构体，读进来只保留已知字段、写回时用 `to_json` 重建 JSON（`GBAStation/src/core/game_database.cpp:342-396,647`）→ **核心往 GameData 里加的任何自定义键，下次前端保存时都会被丢掉**。

### 2.3 `noSync` 语义与运行时切换

`noSync` 已是前端既有字段（`GBAStation/src/core/enums.h:133`：「1=锁定本游戏配置，不被同平台同步覆盖」），前端每游戏菜单里有「锁定本游戏配置」开关（`GBAStation/src/ui/view/GameMenuView.cpp:2250-2258`），批量同步会跳过 `noSync != 0` 的游戏（同文件 `:2496,2520,2545`）。

**本方案在其上叠加"使用独立设置"的含义，两者不冲突**：独立设置的游戏本来就不该被同平台同步覆盖。前端无需改动（可选：把标签改成「使用核心独立设置」更准确）。

**状态机**

| 时机 | `noSync=0`（默认） | `noSync=1` |
|---|---|---|
| 启动游戏 | 读 GameData 字段 + `config.cfg` 平台键 → 应用 | 读 `<savePath>/saturn.settings.cfg` → 应用（文件缺失/损坏则用当前全局值生成后应用） |
| 菜单里改设置 | 写回 GameData（每游戏字段）+ `config.cfg`（平台级键） | **只写**独立文件；不碰 GameData 设置字段、不碰 `config.cfg` 设置键 |
| 统计（次数/时长/退出时间） | 照常写 GameData | **照常写 GameData**（统计与设置分离，不受 `noSync` 影响） |
| 独立设置开关 0→1 | — | ①若独立文件不存在，用**当前生效值**（=全局值）初始化并落盘 ②读该文件并立即应用 ③写 `GameData.noSync=1` |
| 独立设置开关 1→0 | ①读 GameData + `config.cfg` 并**立即应用** ②写 `GameData.noSync=0` ③独立文件**保留**（不删，便于来回切） | — |

**独立设置开关未开启时**：该 Tab 下面的设置项**置灰 + 提示「未启用独立设置」**（避免用户以为改的是当前生效值）。（**已定 N3**）

### 2.4 设置项清单（两个 Tab 完全相同）

| 分组 | 设置项 | 全局模式存储 | 独立模式存储 |
|---|---|---|---|
| 画面 | 画面模式（Fit / Fill / 整数倍 / 自定义 / 4:3） | GameData `displayMode` | 独立文件 |
| 画面 | 整数倍倍率 | GameData `integerAspectRatio` | 独立文件 |
| 画面 | 自定义缩放 / X 偏移 / Y 偏移 | GameData `customScale/customOffsetX/customOffsetY` | 独立文件 |
| 画面 | 内部分辨率 | `config.cfg` `core.saturn.resolution_mode` | 独立文件 |
| 画面 | 视频滤镜（无/FXAA/扫描线/双线性） | `config.cfg` `core.saturn.video_filter` | 独立文件 |
| 画面 | 遮罩开关 / 遮罩路径 | GameData `overlayEnabled/overlayPath` | 独立文件 |
| 画面 | 滤镜开关 / 滤镜预设 | GameData `shaderEnabled/shaderPath` | 独立文件 |
| 画面 | 旋转屏幕 | `config.cfg` `core.saturn.rotate_screen` | 独立文件 |
| 显示 | FPS 开关 | `config.cfg` `display.showFps` | 独立文件 |
| 性能 | 快进倍率 / 触发模式 / 快进静音 | `config.cfg` `fastforward.multiplier/mode/mute` | 独立文件 |
| 性能 | 跳帧 / 帧率限制 / CPU 同步 | `config.cfg` `core.saturn.frame_skip/frame_limit/cpu_sync_per_line` | 独立文件 |
| 音频 | 音频引擎 / SCSP 同步模式 / 每帧同步次数 | `config.cfg` `core.saturn.sound_engine/scsp_*` | 独立文件 |
| 系统 | 卡带 / 区域 / 视频制式 / 扩展内存 | `config.cfg` `core.saturn.cartridge/region/video_format/extend_internal_memory` | 独立文件 |
| 系统 | 多边形生成 / RBG 分辨率 | `config.cfg` `core.saturn.polygon_generation/rbg_resolution` | 独立文件 |

⚠️ **一个必须知道的后果**：表中标记为 `config.cfg` 的项**是平台级键** —— 在「全局设置」Tab 里改它们，会同时影响**所有** Saturn 游戏（前端现有的 Saturn 核心设置页也是平台级，行为一致）。若想让某个游戏跟别人不同，就用「独立设置」开关把整组锁到 `savePath`。这也是本方案把独立文件设计成"完整快照"而不是"差异补丁"的原因。

### 2.5 边界情况

| 情况 | 处理 |
|---|---|
| `noSync=1` 但独立文件缺失/损坏 | 用当前全局值重建并落盘，记日志；不阻断启动 |
| 用户在前端改了存档目录（`savePath` 变化） | 独立文件跟随新 `savePath`；旧目录遗留文件不自动迁移，菜单里给一次提示 |
| `savePath` 为空 | 回退 `sdmc:/GBAStation/saves/Saturn/<stem>/` 并**回写 GameData.savePath**（与前端默认一致） |
| 无 GameDB 记录（桌面图标/内置浏览器直接启动） | 用回退路径；设置只读 `config.cfg` + 独立文件（若有）；不写 GameData、不建记录（你的第 2 条） |
| 运行中切开关后崩溃 | 开关状态已在切换瞬间写入 GameData，下次启动按新状态读取，不会错乱 |

### 2.6 验收

- [ ] 7 个 Tab 都能打开、返回、焦点不串
- [ ] `noSync=0`：菜单改设置 → 前端每游戏菜单能看到同一值
- [ ] `noSync=1`：菜单改设置 → 只写 `savePath` 下文件，GameData 设置字段不变
- [ ] 运行中开开关 → 立即切到独立值；关开关 → 立即切回全局值（画面/滤镜/遮罩即时生效）
- [ ] `noSync=1` 时前端"同步到同平台"会跳过该游戏
- [ ] 独立文件删除后再启动，能自动重建且不崩
- [ ] 前端重新保存 GameDB 后，独立设置仍然生效（证明没依赖 GameData 自定义键）

---

## 3. 需求一：三种启动方式

### 3.1 模式与判定规则

| 模式 | 触发 | 行为 |
|---|---|---|
| A 启动器拉起 | argv 含 `--return <nro>` | 直接进游戏；退出链回该 NRO |
| B 桌面/Forwarder | argv 只有 ROM、无 `--return` | 直接进游戏；退出**不链回**，回系统桌面 |
| C 无参数 | 无 ROM | 进内置浏览器；退出回桌面 |

**唯一开关：有 `--return` 才链回。**

### 3.2 修改后的 argv 契约

| 来源 | argv |
|---|---|
| 应用内启动 | `"<GBAStationYabaSanshiroStub.nro>" "<rom>" --return "<returnNro>"` |
| 新式 forwarder | `"<rom>"` |
| 旧式 forwarder | `"<rom>" --return "sdmc:/switch/GBAStation.nro"` |
| hbmenu | 无 |

不再出现 `--gbastation-session`；返回时不再需要 `--external-return`。

前端要改的两处（**6 平台共用，必须按平台收窄**）：
* `GBAStation/src/ui/page/StartPage.cpp:1324-1342`
* `GBAStation/src/main.cpp:258-276`

### 3.3 待做项

- [ ] **A1** `main()` 调 `ReadLaunchInfo(argc, argv)`，取 `rom_path` / `return_nro`。
- [ ] **A2** 有 ROM → 跳过启动页直接进游戏（复用 `nx/main.cpp:1065-1092` 初始化链）。
- [ ] **A3** 无 ROM → 内置浏览器（保持现状）。
- [ ] **A4** 「退出游戏」与窗口关闭都接 `ReturnToLauncher()`，仅 `return_nro` 非空时调用。
- [ ] **A5** `ReturnToLauncher()` 去掉 `--external-return`，只传返回 NRO 路径；**加目标存在性检查**。
- [ ] **A6** 浏览器根目录：先读 `scan.path.saturn`，空则 `sdmc:/GBAStation/roms/saturn`。
- [ ] **A7** 失败路径（缺 BIOS / ROM 不存在 / 初始化失败）不链回。
- [ ] **A8** 菜单热键改为 `saturn.hotkey.menu.pad`；即时存/读档改读 `saturn.hotkey.quicksave.pad` / `quickload.pad`（替换硬编码 `main.cpp:841-855`）。

### 3.4 验收矩阵

| 场景 | 期望 |
|---|---|
| 启动器 → 游戏 → 菜单退出 | 回 GBAStation |
| 启动器 → 游戏 → 关窗口 | 回 GBAStation |
| 桌面图标 → 退出 | 回系统桌面（不进 GBAStation） |
| hbmenu → 浏览器选游戏 → 退出 | 回内置启动页；再退回 hbmenu |
| `--return` 文件不存在 | 不链回，正常退出 |
| 全程结束 | `external_core_session.json` **不存在** |

---

## 4. 需求二：`config.cfg` 与 `GameData_Saturn.json`

### 4.1 `config.cfg` 读取（P0：路径错）

现状候选顺序（`GBAStationPlatform.cpp:84-116`）：① `<DataRoot>/config/config.cfg`（`DataRoot` 现为 `sdmc:/GBAStation/yabasanshiro`）② `/GBAStation/config/config.cfg`（**缺 `sdmc:`**）。
→ 前端维护的 `sdmc:/GBAStation/config/config.cfg` 不是首选。

- [ ] **B1** 顺序改为 `sdmc:/GBAStation/config/config.cfg` → `/GBAStation/config/config.cfg`。
- [ ] **B2** 编码：`key=<type>|<payload>`，仅 `s` 反转义（现逻辑已支持）。
- [ ] **B3** 补齐未消费的键族：`saturn.handle.*`（现在 `ReadButtonMapping()` 是死代码 `:434-445`）、`saturn.hotkey.*`、`fastforward.*`、`save.*`、`display.showFps`、`UI.language`。

### 4.2 `config.cfg` 写入（P0：格式不兼容）

`SaveSettings()`（`Settings.cpp:230-277`）写裸 `key=value`；前端 `DeserializeValue()` 没有 `|` 就**整行丢弃**（`GBAStation/src/core/ConfigManager.cpp:316-330`）→ 核心改的设置前端读不到。

- [ ] **B4** 改为 `key=<type>|<value>`：`s|`/`i|`/`f|`，布尔用 `i|`，`s` 值转义 `\`、`,`、`|`。
- [ ] **B5** 保留未知键（已实现 ✅）。
- [ ] **B6** 所有权：核心只写自己消费的键。

### 4.3 `GameData_Saturn.json`（P0：完全缺失）

要读：`path`（归一化匹配）、`title`、**`savePath`**、`displayMode`（0 Fit/1 Fill/2 整数倍/3 自定义/4 4:3）、`integerAspectRatio`、`customScale/customOffsetX/customOffsetY`、`overlayEnabled/overlayPath`、`shaderEnabled/shaderPath`、`noSync`、`playCount/playTime/lastPlayed`、`screenShotPath`。
要写：`savePath`（补默认时）、`noSync`、`playCount/playTime/lastPlayed`，以及 `noSync=0` 时菜单改动的设置字段。

- [ ] **B7** 按 `path` 读取（归一化、取第一条）。
- [ ] **B8** 写回用**原子写**（`.tmp` → rename）；不要 `ofstream(trunc)` 直接覆盖。
- [ ] **B9** **绝不新增自定义键**（前端会丢弃，见 §0-6）。
- [ ] **B10** 改动即写；统计字段退出时写 + 定期落盘。

### 4.4 验收
- [ ] 前端改 `core.saturn.*` → 重启生效
- [ ] 核心改设置 → 前端读到且**类型正确**
- [ ] 断电一次，JSON 不损坏
- [ ] 前端重新保存 GameDB 后，核心设置仍在

---

## 5. 需求三：菜单功能补完

### 5.1 现状

扁平 4 项（`main.cpp:826` 即时存档/即时读档/设置/退出游戏），无 Tab；设置子页 8 项；ZL+ZR 打开；nanoVG over Vulkan（`NvgUi.cpp` 693 行）；`NvgUiDrawGameMenu` 支持传入 item 数组（`:656-692`）。

### 5.2 画面模式（4 档）

现状只有布尔"保持宽高比"，`ASPECT_RATE_MODE` 4 值只用 0/1（`main.cpp:304-307`、`ygl.h:552-557`）。

- [ ] 实现 Fill（全屏）/ Fit（原比例）/ Integer（整数倍）/ Custom（scale+offset），映射 GameData `displayMode` 0/1/2/3；4:3 走 `displayMode=4` 映射到 `ASPECT_RATE_MODE._4_3`。

### 5.3 保存/读取状态（10 槽 + 时间 + 缩略图）

- [ ] 从单槽 `kQuickSlot=1`（`main.cpp:825-826,886-895`）扩到 10 槽。
- [ ] 文件全部落在 **`savePath`**：

| 文件 | 说明 |
|---|---|
| `<savePath>/<stem>.ram` | 电池备份 RAM |
| `<savePath>/<stem>.cart` | 卡带镜像 |
| `<savePath>/<stem>.ss1 … .ss10` | 10 个即时档 |
| `<savePath>/<stem>.ss1.png …` | 对应缩略图（建议 256×384） |

- [ ] 缩略图：改用 Vulkan 取帧（`vulkan/VIDVulkan.cpp:167 getScreenshot` 已存在但状态路径没用）。
  ⚠️ **不要复用**状态文件内嵌的那段 RGBA"截图"——它在 `#ifdef USE_OPENGL` 内填充而本构建该宏未定义，是**未初始化内存**（`memory.c:1968-1990`）。
- [ ] 兼容旧档：读取时若 `.ss1` 不存在而 `.state1` 存在，按 slot 1 迁移/读取一次。
- [ ] 槽位列表显示：槽号 + 时间（文件 mtime 或状态头内时间）+ 缩略图；`X` 删除。

### 5.4 快进（变调 + 触发方式改造）

**现状**：核心**没有快进触发**，只有 `frame_limit=1`（不限帧）+ 自动跳帧（`VIDVulkan.cpp:2960-2970`、`main.cpp:328-332`）。

**要做的**：

- [ ] 触发方式改成按键：读 `saturn.handle.fastforward`（前端默认 `PAD_RSB`），按 `fastforward.mode`（`hold` 按住 / `toggle` 切换）判定。
- [ ] 倍率读 `fastforward.multiplier`（clamp 0.5–5，与前端一致 `GBAStation/src/core/common.cpp:555`）。
- [ ] 执行方式：快进激活时，一个呈现帧内多跑 `multiplier` 个模拟帧（呈现频率不变）。
- [ ] **音频：变调**（提高重采样率使音高随速度上升）；若 `fastforward.mute=1` 则改为静音。
- [ ] 与 `frame_limit` / 自动跳帧的交互：快进期间临时忽略帧率限制与自动跳帧。
- [ ] 菜单里提供「快进倍率 / 触发模式 / 快进静音」三项（放两个设置 Tab 的"性能"分组）。
- [ ] 菜单热键/提示栏显示当前快进状态（可选 HUD 徽标）。

### 5.5 遮罩（PNG）

- [ ] 现状**缺失**（只有半透明黑底）。实现：PNG 解码 + 叠一层纹理；建议直接用现成的 nanoVG-Vulkan（`nvgCreateImageMem`），不另写 Vulkan pass。
- [ ] 路径与开关取 GameData `overlayPath`/`overlayEnabled`（或在独立模式下取独立文件）。
- [ ] 菜单提供遮罩选择（文件选择器根目录建议 `sdmc:/GBAStation/overlays`）。

### 5.6 滤镜

- [ ] 阶段一：把核心内置 4 档（无/FXAA/扫描线/双线性，`ygl.h:515-521`）在两个设置 Tab 里暴露完整 + 扫描线参数。
- [ ] 阶段二（**已定 N5：本阶段不做**）：外部 shader / 多 Pass 链，另立需求。

### 5.7 重置游戏

- [ ] 新增 Tab：二次确认后原地重置（`NDS::LoadROM` 式的重新 init + 载入当前 ROM），不重启进程、不重载渲染栈。

### 5.8 UI 美化

- [ ] 渐变焦点框（前端用 `border_gradient.png`）、Tab 分区与切换动画、Material 图标（已内嵌）、提示栏、Toast、空态文案。

### 5.9 明确不做

* **倒带**（按你的要求删除）。前端 `rewind.*` 与 `saturn.handle.rewind` 对 Saturn 保持未消费。

---

## 6. 需求四：统计由核心维护

### 6.1 前端要改的只有 2 处（已确认）

| 位置 | 是否作用于 Saturn | 处理 |
|---|---|---|
| `GBAStation/src/core/ExternalCoreSession.cpp:88-92` | ✅ 被下面两处调用 | 由调用点收窄 |
| `GBAStation/src/ui/page/StartPage.cpp:1341` | ✅ | 跳过 |
| `GBAStation/src/main.cpp:275` | ✅ | 跳过 |
| `GBAStation/src/ui/page/GamePage.cpp:231-235`（`updateGameCount`） | ❌ **不受影响**：Saturn 在 `StartPage.cpp:1575-1582/1699-1707`、`main.cpp:334-340` 都提前 return，到不了 `new GamePage(...)` | 不动 |

### 6.2 核心要做

- [ ] **D1** 进游戏 `playCount + 1`；退出写 `playTime`（累加本次会话秒数）与 `lastPlayed`（**退出时间**）。
- [ ] **D2** 计时口径：只在真正运行游戏时累加（菜单暂停/挂起不计）；加限保护（前端是 7 天上限）。
- [ ] **D3** 落盘：退出写一次 + 定期（如每 60 秒）落一次。
- [ ] **D4** 时间戳格式与前端一致：`%y-%m-%d %H-%M-%S`（`GBAStation/src/core/Tools.cpp:457-465`）。
- [ ] **D5** 无 DB 记录时不写统计、不建记录（你的第 2 条）。

### 6.3 验收
- [ ] 玩一次 `playCount` 只 +1
- [ ] `playTime` 随实际时长增长，崩溃不丢太多
- [ ] `lastPlayed` 为退出时刻，前端"上次游玩"显示正确
- [ ] 无 `external_core_session.json`

---

## 7. 需求五：路径约束

### 7.1 统一后的路径表（`DataRoot` 迁移已定）

| 类型 | 位置 |
|---|---|
| 配置（读写） | `sdmc:/GBAStation/config/config.cfg` |
| 每游戏设置 | `GameData_Saturn.json`（全局模式）/ `<savePath>/saturn.settings.cfg`（独立模式） |
| BIOS | `sdmc:/GBAStation/bios/saturn/`（`saturn_bios.bin` / `sega_101.bin` / `mpr-17933.bin`） |
| ROM（模式 C 根） | `scan.path.saturn`，空则 `sdmc:/GBAStation/roms/saturn` |
| 存档 / 卡带 / 即时档 / 缩略图 | **GameData `savePath`**，空则 `sdmc:/GBAStation/saves/Saturn/<stem>/` 并回写 DB |
| 截图 | GameData `screenShotPath`（前端默认 `/GBAStation/screenshots/`） |
| log / cache（核心私有） | `sdmc:/GBAStation/log/saturn/`、`sdmc:/GBAStation/cache/saturn/` |
| 会话文件 | 不再产生 |

### 7.2 待做项

- [ ] **E1** `DataRoot()` 迁移：用户数据不再放 `sdmc:/GBAStation/yabasanshiro/`；保留 log/cache 私有目录。旧数据给一次迁移或明确文档说明。
- [ ] **E2** `savePath` 解析：有值就用；空则回退 + **回写 DB**。
- [ ] **E3** 目录 `mkdir -p`（`EnsureSaturnDirectories()` 按新路径更新，`:348-364`）。
- [ ] **E4** 补截图实现（配合 §5.3 取帧）。
- [ ] **E5** BIOS：保留 `bios/saturn/` 搜索；前端补 BIOS 选择入口（见 §10）。

---

## 8. 决策表

### 已定（你的决定）

| # | 决策 | 结论 |
|---|---|---|
| D1 | `DataRoot` | 迁移到共享目录，log/cache 私有 |
| D2 | 无 GameDB 记录 | 回退路径 + 跳过统计 + 不建记录 |
| D3 | 即时档命名 | `.ss<N>`（+ `.png`），旧 `.state1` 兼容 |
| D4 | 快进音频 | 变调（`fastforward.mute=1` 时静音） |
| D5 | 菜单结构 | 7 Tab（返回/保存/读取/独立设置/全局设置/重置/退出） |
| D6 | 独立设置存储 | `<savePath>/saturn.settings.cfg`，`noSync` 控制 |
| D7 | 链回语义 | 有 `--return` 才链回 |
| D8 | session | Saturn 不传 `--gbastation-session`、不建会话、返回不带 token |
| D9 | 菜单/热键来源 | 全部读 `saturn.handle.*` / `saturn.hotkey.*` |
| D10 | 画面模式字段 | 用 GameData `displayMode` 0–4 |
| D11 | 倒带 | 不做 |

### 已定（补充决定，原待确认项 N1–N7）

| # | 决策 | 结论 |
|---|---|---|
| N1 | 独立设置文件命名 | **`<savePath>/saturn.settings.cfg`** |
| N2 | 关闭独立设置时是否删文件 | **保留**（便于来回切） |
| N3 | 开关未启用时下面的设置项 | **置灰 + 提示「未启用独立设置」** |
| N4 | 旧式 forwarder 行为 | **接受「旧式回启动器」**，前端不改 |
| N5 | 滤镜深度 | **只做内置档位**；外部 shader 另立需求 |
| N6 | 截图 | **做**（复用保存状态的取帧能力） |
| N7 | 退出前自动存档 | **做**（读 `save.autoSaveOnExit`） |

→ **当前无未决项。**

---

## 9. 里程碑

| 阶段 | 内容 | 出口 |
|---|---|---|
| **M1 跑通** | 需求一（A1–A8）+ `config.cfg` 路径修复（B1） | 三种启动正确进出；`core.saturn.*` 与热键生效；无会话文件 |
| **M2 数据** | B2–B10 + 需求四 + 路径统一（E1–E3） | 统计正确；设置双向可读；存档/即时档都在 `savePath`；断电不丢 |
| **M3 菜单** | 7 Tab 框架 + 10 槽（时间+缩略图）+ 画面模式 4 档 + 遮罩 + 重置游戏 | 菜单可用，视觉对齐前端 |
| **M4 双设置** | 独立/全局设置模型 + `noSync` 运行时切换 + 独立文件读写 | §2.6 全部验收通过 |
| **M5 打磨** | 快进（触发+变调）、滤镜档位补全、UI 动效、截图（**已定 N6：做**）、退出前自动存档（**已定 N7**） | 体验补齐（无倒带） |

依赖：M2 统计依赖前端 Saturn 跳过会话（先行）；M3 缩略图依赖取帧改造；M4 依赖 M2 的 GameData 读写；M5 快进依赖音频策略（已定：变调）。

---

## 10. 前端改动清单

| # | 改动 | 位置 | 必要性 |
|---|---|---|---|
| 1 | Saturn 跳过会话（不传 token、不建会话） | `StartPage.cpp:1324-1342`、`main.cpp:258-276` | **必须** |
| 2 | 新增 BIOS 文件选择项（核心已在读 `core.saturn.biosPath`） | `SettingPage.cpp:3466-3594` | 建议 |
| 3 | 「锁定本游戏配置」文案可改为「使用核心独立设置」 | `GameMenuView.cpp:2250-2258` | 可选（语义已兼容） |
| 4 | 补 Saturn 平台级遮罩/滤镜默认键 | `constexpr.h`、`Tools.cpp:615-655` | 可选 |
| 5 | forwarder 一致性（Saturn 加 `--exit-to-home`） | `ForwarderInstaller.cpp:241-244` | **不做**（已定 N4：接受旧式 forwarder 回启动器） |

**明确不需要前端改**：`noSync` 的语义与同步跳过逻辑（现有实现已兼容）。

---

## 11. 风险

1. **独立设置与前端显示不一致**：`noSync=1` 时前端每游戏菜单显示的是 GameData 值，核心实际用独立值。建议在核心菜单显著提示（如 Tab 标题旁标「独立」），并把这一条写进用户文档。
2. **平台级键的连带影响**：`config.cfg` 里的 `core.saturn.*` 在「全局设置」里改动会影响所有 Saturn 游戏（§2.4 已说明）。
3. **不得往 GameData 加字段**：前端 `GameEntry` 固定结构会丢弃未知键（§0-6），所有核心私有数据必须放 `savePath`。
4. **状态内嵌截图是未初始化内存**（`memory.c:1979-1984`，`USE_OPENGL` 未定义）——缩略图必须走 Vulkan 取帧，别复用。
5. **`/GBAStation/config/config.cfg`（无 `sdmc:`）能否解析**未实机验证，改路径时一并验证。
6. **两套保存体系**：`YabSaveStateSlot`（`.yss`，`memory.c:2306/2325`）未使用，nx 用 `.stateN`；做 10 槽时**只保留一套**。
7. **session 收窄的作用域**：`launchExternalCoreNro`/`launchExternalCore` 是 6 平台共用，改错会让 PPSSPP/PS1/DC 的时长统计失效。
8. **`DataRoot` 迁移会改变现有数据位置**，需处理老用户数据（迁移或文档）。
9. **快进变调的音频风险**：需要改动重采样/输出链路，注意与 SCSP 同步设置（`scsp_sync_time_mode` 实时模式）的相互影响。

---

## 12. M1 逐文件执行清单（启动接线 + 配置路径）

**M1 目标**：三种启动方式正确进出；`core.saturn.*` 与热键生效；不产生会话文件。
**M1 不碰**：菜单框架、`GameData_Saturn.json` 读写、统计、`DataRoot` 迁移（都属 M2+）。

### 12.1 前端（`GBAStation`）

| 步 | 文件 | 位置 | 改动 | 验收 |
|---|---|---|---|---|
| F1 | `src/core/ExternalCoreSession.hpp`（或 `src/core/common.h`） | 新增 | 加 `bool platformUsesLauncherStats(int platform)`；Saturn 返回 `false` | 编译通过 |
| F2 | `src/ui/page/StartPage.cpp` | `:1324-1342` | 按 F1 判定：为 `false` 时不生成 token、不传 `--gbastation-session`、不调 `beginExternalCoreSession` | 启动 Saturn 后**无** `external_core_session.json` |
| F3 | `src/main.cpp` | `:258-276` | 同 F2 | 直接启动 Saturn 同样无会话文件 |
| F4 | 回归 | — | PSP / PS1 / DC / Dolphin 仍走会话 | 这 4 个平台会话文件正常创建/清理，`playTime` 增长 |

⚠️ F2 / F3 位于 **6 平台共用**的函数（`launchExternalCoreNro` / `launchExternalCore`），**只加平台分支、不删既有逻辑**。

### 12.2 核心（`yabasanshiro/yabause`）

| 步 | 文件 | 位置 | 改动 | 验收 |
|---|---|---|---|---|
| C1 | `src/gbastation/GBAStationPlatform.cpp` | `:84-116` | 配置候选顺序改为 `sdmc:/GBAStation/config/config.cfg` → `/GBAStation/config/config.cfg` | 改前端 `core.saturn.region` 后核心生效 |
| C2 | 同上 | `:334-346` | `ReturnToLauncher()`：去掉 `--external-return`（已无 token），只传返回 NRO 路径；**加目标文件存在性检查** | 返回值正确；目标缺失时不链回 |
| C3 | 同上 | `:370-377` | 浏览器根目录：先读 `scan.path.saturn`，空则 `sdmc:/GBAStation/roms/saturn` | 改 `scan.path.saturn` 后浏览器根目录跟随 |
| C4 | `src/nx/main.cpp` | `main()` `:1021` | 调 `ReadLaunchInfo(argc, argv)`，把 `rom_path` / `return_nro` 存到运行时状态 | 日志可见解析结果 |
| C5 | 同上 | `:1058-1063` | 有 `rom_path` → 跳过 `RunBootPage()` 直接进游戏；无 → 保持现状 | 启动器发起时直接进游戏，不停在启动页 |
| C6 | 同上 | 退出路径 `:915`、`:995`、`:1100` | 「退出游戏」与窗口关闭都接 `ReturnToLauncher()`（仅 `return_nro` 非空）；失败路径不链回 | §3.4 验收矩阵前 5 行 |
| C7 | `src/gbastation/GBAStationPlatform.cpp` | `:434-445` | 启用 `ReadButtonMapping()`：解析 `saturn.handle.*` / `saturn.hotkey.*`（combo 用 `+`、多绑定用 `|`） | 改前端映射后核心按键跟随 |
| C8 | `src/nx/main.cpp` | `:841-855` | 菜单热键由硬编码 ZL+ZR 改为 `saturn.hotkey.menu.pad`（默认 `PAD_LT+PAD_RT`） | 默认热键与前端一致 |
| C9 | 同上 | `:886-895` | 即时存/读档热键改读 `saturn.hotkey.quicksave.pad` / `quickload.pad` | 默认 `none` 时不触发 |
| C10 | `src/gbastation/GBAStationPlatform.cpp` | `:348-364` | `EnsureSaturnDirectories()` 按 §7.1 建共享目录（本步只建目录；`savePath` 逻辑属 M2） | 首次启动目录齐备 |

### 12.3 M1 出口验收（一次性跑完）

| # | 动作 | 期望 |
|---|---|---|
| 1 | 启动器 → Saturn 游戏 → 菜单退出 | 回 GBAStation，无异常日志 |
| 2 | 启动器 → Saturn 游戏 → 直接关窗口 | 回 GBAStation |
| 3 | 桌面图标（新式 forwarder）→ 退出 | 回系统桌面 |
| 4 | 桌面图标（旧式 forwarder）→ 退出 | 回 GBAStation（已定 N4 接受） |
| 5 | hbmenu 无参数 → 浏览器选游戏 → 退出 | 回内置启动页；再退回 hbmenu |
| 6 | 全流程后检查 | 无 `external_core_session.json`；`config.cfg` 未被核心改写（M1 不写配置） |
| 7 | 改前端 `core.saturn.video_filter` 与 `saturn.hotkey.menu.pad` | 核心行为跟随 |
| 8 | PSP 回归 | 会话统计仍正常 |

### 12.4 明确不在 M1 范围（防范围蔓延）

| 项 | 归属 |
|---|---|
| `config.cfg` 写回类型前缀（B4） | M2 |
| `GameData_Saturn.json` 读写 + 统计（B7–B10、§6） | M2 |
| `DataRoot` 迁移与老数据清理（E1） | M2 |
| 菜单 7 Tab / 10 槽 / 缩略图 / 画面模式 / 遮罩 / 重置游戏 | M3、M4 |
| 独立·全局设置模型（§2） | M4 |
| 快进、滤镜档位、UI 动效、截图 | M5 |

---

## 13. 审查结论

**一致性检查**
- 你的 5 条决定逐条落地（§1 映射表），无遗漏。
- 菜单 7 Tab 与 §2.4 设置项清单一致；两个设置 Tab 项集相同（你的要求）。
- `noSync` 的三种时机（启动 / 运行中开 / 运行中关）在 §2.3 状态机中都有明确行为。
- 即时档路径（`savePath`）与 §7.1 路径表、§5.3 命名一致。
- 快进：触发（`saturn.handle.fastforward`）、模式、倍率、音频策略（变调）四处都已覆盖，并说明了与 `frame_limit` 的关系。

**本轮新发现并按已定决策处理**
- 前端 GameDB 丢弃未知字段 → 独立设置必须放 `savePath`（§0-6、§4.3-B9、§11-3）。
- `GamePage::updateGameCount()` 不影响 Saturn → 前端只需改 2 处（§6.1）。

**已全部闭环**
§8 的 N1–N7 已按建议锁定（见 §8「已定（补充决定）」），当前无未决项。

**下一步**
按 §12 的 M1 清单开工；M2 起按 §9 里程碑推进。

---

## 14. M1 执行记录（本轮已落地）

**状态**：代码已改完，核心 NRO 与前端 NRO 均**编译打包通过**；**实机行为未验证**（需要你在 Switch 上跑 §14.5 的检查项）。
逐项改动见同目录 [Saturn_M1_改动记录.patch](Saturn_M1_改动记录.patch)（只含本轮改动）。

### 14.1 前端（`GBAStation`，4 个文件）

| 步 | 文件 | 改动 |
|---|---|---|
| F1 | `src/core/ExternalCoreSession.hpp/.cpp` | 新增 `beiklive::platformReportsOwnStats(int)`：Saturn 返回 true（含注释说明后续自报统计的核心加在这里） |
| F2 | `src/ui/page/StartPage.cpp`（`launchExternalCoreNro`） | `trackSession` 判定：Saturn 不生成 token、不传 `--gbastation-session`、不调 `beginExternalCoreSession` |
| F3 | `src/main.cpp`（`launchExternalCore` lambda） | 同上 |
| — | PSP / PS1 / DC / Dolphin | **逻辑未动**，仍走会话（回归项） |

### 14.2 核心（`yabasanshiro/yabause/src`，3 个文件）

| 步 | 文件 | 改动 |
|---|---|---|
| C1 | `gbastation/GBAStationPlatform.cpp` | 配置候选顺序改为 `sdmc:/GBAStation/config/config.cfg` → `/GBAStation/config/config.cfg` → `<DataRoot>/config/config.cfg`；记录"实际加载到的路径" |
| C1b | 同上 `ConfigPath()` | 写回**实际加载的那个文件**（原来读一处、写另一处）；无任何文件时回落到启动器路径 |
| B4（提前） | `gbastation/Settings.cpp` | `SaveSettings` 保留每个键原有的 `i\|/f\|/s\|` 类型标签（新键按值推断），字符串按启动器规则转义 |
| C2 | `gbastation/GBAStationPlatform.cpp` | `ReturnToLauncher()` 去掉 `--external-return`/`--resume`，只传带引号的 NRO 路径；**加目标存在性检查**并打日志 |
| C3 | 同上 `DefaultRomsDir()` | 读 `scan.path.saturn`，空则 `sdmc:/GBAStation/roms/saturn` |
| C7 | 同上 | `ParsePadName` 支持 `\|` 多绑定（取第一个可解析者）；`ReadButtonMapping`：键缺失→用调用方默认，"none"→真正不绑定（大小写不敏感） |
| C10 | 同上 `EnsureSaturnDirectories()` | 补 `config/`、`overlays/` 共享目录 |
| C4 | `nx/main.cpp` | `main()` 调 `ReadLaunchInfo(argc, argv)` 并写日志（rom / return / token） |
| C5 | `nx/main.cpp` | 有 ROM 参数 → 跳过内置浏览器，直接进游戏 |
| C6 | `nx/main.cpp` | 退出后：**有参数启动** → `ReturnToLauncher()` 再退出（无 `--return` 则只退出，回系统桌面）；**无参数启动** → 回内置浏览器 |
| C8 | `nx/main.cpp` | 菜单热键改读 `saturn.hotkey.menu.pad`，缺省回落 `ZL+ZR`（保持旧行为） |
| C9 | `nx/main.cpp` | 新增 `saturn.hotkey.quicksave.pad` / `quickload.pad` 触发的即时存/读档（复用菜单路径的 SCSP 静音 + 清队列） |

### 14.3 验证结果

| 验证 | 结果 |
|---|---|
| 三个核心 TU 单独编译 | **0 error / 0 新增 warning**（另有 2 条既有 unused-function warning） |
| 核心完整构建 | **EXIT=0**，产出 `yabause/build-switch/GBAStationYabaSanshiroStub.nro`（26,161,152 B） |
| 前端完整构建 | `GBAStation.elf` 链接成功；符号 `beiklive::platformReportsOwnStats(int)` 已存在；`GBAStation.nro` 打包 **EXIT=0** |
| 未做 | 实机三种启动方式进出、热键、链回、config 写回往返 |

### 14.4 与计划的偏差（均为必要或加固）

1. **B4 从 M2 提前到 M1**：C1 让核心开始读写**启动器的** `config.cfg`，若仍按裸 `key=value` 写回，会把带类型的行降级，前端随后整行丢弃该键 → 设置等于丢失。两者必须同时改。
2. C1 多了第 3 个候选 `/GBAStation/config/config.cfg`（无设备前缀），与 melonDS / PPSSPP 的尝试顺序一致。
3. 额外加固：`|` 多绑定解析、`none` 的准确语义、写回路径跟随加载路径。
4. 已知行为：核心设置菜单保存时会**重排键序并丢弃注释**（原本如此）；值不丢，启动器下次保存会按自己的顺序重写。

### 14.5 需要实机确认的检查项

- [ ] §12.3 的 8 条出口验收（三种启动方式 × 退出行为 + PSP 回归）
- [ ] 核心菜单里改一项设置 → 启动器 Saturn 设置页显示**同一个值**（验证类型标签写回是否被前端接受）
- [ ] 菜单热键跟随 `saturn.hotkey.menu.pad`（改成别的键后 ZL+ZR 不再开菜单）
- [ ] 即时存/读档热键默认 `none` → **不应**触发；显式绑定后能存/读
- [ ] PSP / PS1 / DC 启动后 `external_core_session.json` 仍正常创建与清理（会话未被破坏）

### 14.6 未提交状态说明（重要）

核心仓库在开工前**已有他人未提交改动**（`cs2.c`、`scsp.cpp`、`sndsdl.*`、`vdp2.cpp`、`yabause.c`、`vulkan/VIDVulkan.cpp`、`YabLog.*`、`gbastation/Settings.cpp`、`nx/main.cpp`）。
因此 `Settings.cpp` 与 `nx/main.cpp` 的 `git diff` 是**混合 diff**；本轮改动前的快照留在 `/tmp/saturn_m1_before/`（临时，重启会清），可直接用于区分。前端 4 个文件在改动前是干净的，`git diff` 即本轮改动。

**建议：先把核心仓库里既有的未提交改动提交或 stash，再单独 review 本轮改动。**
