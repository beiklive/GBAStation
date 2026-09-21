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

---

## 15. M2 执行记录（进行中）

M2 范围：B2–B10（配置键补全 + GameData 读写）+ 需求四（统计）+ E1–E3（路径统一）。
**已完成第一步**；第二步（E1 `DataRoot` 迁移 / B3 玩家按键映射与 `save.*`/`fastforward.*` 键）待做。

### 15.1 第一步：GameData_Saturn.json 读写 + savePath + 统计（已提交 `6858e54`）

| 项 | 实现 |
|---|---|
| 新模块 | `yabause/src/gbastation/GameDb.{h,cpp}`：定位记录、原子写、路径解析、统计 |
| JSON 库 | **复用仓库既有 jsoncpp**（`src/json/`，早已编入 `yabause` 库），未引入新依赖 |
| 定位记录 | 按 `path` 归一化匹配：`\`→`/`、去 `sdmc:` 前缀、折叠前导 `//` |
| 原子写 | 写 `.tmp` → 保留一代 `.bak` → `rename`；FAT 上 rename 失败则回退为拷贝 |
| 字段所有权 | **只写** `savePath` / `playCount` / `playTime` / `lastPlayed`；其余字段与其它条目原样保留；**不新增键**（启动器按固定结构反序列化，新增键会在其下次保存时被丢弃） |
| 无记录 | 不创建记录、不写统计，游戏照常运行（对应你确认的第 2 条决定） |
| savePath | 优先用启动器值；为空则用 `sdmc:/GBAStation/saves/Saturn/<stem>/` 并**回写数据库**，使启动器页面显示同一目录 |
| 统计 | 进入游戏 `playCount + 1`；退出时 `playTime += 本次秒数`、`lastPlayed = 退出时刻`（`%y-%m-%d %H-%M-%S`，与启动器一致）；游戏内每 60 秒落一次盘；菜单暂停不计时，>5 秒的间隔（挂起/卡顿）丢弃 |
| 存档路径统一 | 备份 RAM → `<savePath>/<stem>.ram`（旧 `saturn/backup/<name>-<hash>.ram` **一次性迁移**）；即时档 → `<savePath>/<stem>.ss<N>`，**读档回退旧路径**，老档不丢 |
| 回归测试 | `yabause/src/gbastation/tests/run_gamedb_tests.sh`：15 项宿主断言（字段保留 / 路径归一化 / 时间戳格式 / UTF-8 往返 / 无记录不建库），**全部通过** |

### 15.2 测试抓到的真实缺陷

1. **jsoncpp 1.8 的 `commentStyle` 只接受 `"All"`/`"None"`**（大小写敏感）。我最初写 `"none"`，`Json::writeString` 会**抛异常**；在设备上就是游戏内写统计时直接终止核心。已修正，并给读写两侧都加了 `try/catch`。
2. 由测试确认的行为差异（非缺陷，已记录）：
   - jsoncpp 输出 `"key" : value` 且**按字母序排列键**，与启动器 nlohmann 的 `"key": value` + 插入序不同。JSON 合法、启动器可正常解析；启动器下次保存时会按自己的风格重写。
   - 中文标题以 **原始 UTF-8** 写入（已把 jsoncpp 的 `\uXXXX` 还原），避免数据库文件不可读。

### 15.3 验证

| 项 | 结果 |
|---|---|
| 宿主回归测试 | **ALL PASS（0 failure）** |
| Switch 完整构建 | **成功**，`GBAStationYabaSanshiroStub.nro` 26,210,304 B |
| 实测（设备） | 未做：需要验 `playCount` 只 +1、`playTime` 增长、`lastPlayed` 为退出时刻、`savePath` 被回写、其它字段未丢 |

### 15.4 M2 剩余（第二步）

- [ ] **E1** `DataRoot` 迁移：用户可见数据（配置/存档/即时档/截图）走共享目录，核心私有只留 `log/`、`cache/`；旧数据迁移或文档说明
- [ ] **B3** 玩家按键映射：`saturn.handle.*` 替换 `main.cpp` 里硬编码的 `PAD_KEY(...)` 表（当前只做了热键）
- [ ] **B3** `save.*`（autoLoadState0 / autoSaveOnExit）与 `fastforward.*` 键的消费（`display.showFps` 建议与 FPS 开关一起放 M5）
- [ ] 统计口径微调：`playTime` 目前按"游戏运行且菜单关闭"的墙钟累计，需实机确认与挂起/唤醒的交互

---

## 16. M2 第二步执行记录（已提交 `dcdd335`）

### 16.1 JSON 库换成与启动器同一份

- vendor 启动器的 **nlohmann/json 3.12.0**（`GBAStation/src/core/json.hpp`）到核心
  `yabause/src/gbastation/third_party/nlohmann/json.hpp`（逐字节相同），替换原 jsoncpp 用法。
- 顺带解决两件事：输出风格与启动器 `dump(4)` **完全一致**（`"key": value`、4 空格缩进、
  原始 UTF-8、键序一致），以及删掉了为 jsoncpp 写的 `\uXXXX` 反转义补丁。
- jsoncpp 仍在仓库里被其它代码使用，未删除。

### 16.2 玩家按键映射：`saturn.handle.*` 真正生效

替换了 `main.cpp` 里硬编码的 `PAD_KEY(...)` 表。**每个 Saturn 按键可由多个配置键供给**
（如 L 同时读 `saturn.handle.l` 与 `saturn.handle.l2`），值的形式全部支持：

| 写法 | 语义 |
|---|---|
| `PAD_A` | 单键 |
| `PAD_LT+PAD_RT` | **组合键**：要求同时按住，整组才算按下 |
| `PAD_A\|PAD_B` | 多绑定：取第一个可解析项 |
| `PAD_A+PAD_B\|PAD_LT` | 组合与多绑定混用 |
| `none` / 空 | 真正不绑定（不是"用默认值"） |

- 已核对启动器的写入格式：多绑定用 `|` 拼接、未绑定写 `none`（`SettingPage.cpp:4447-4460`），
  与实现一致。
- 解析逻辑抽到 **NX 无关的 `gbastation/PadMapping.h`**（`ResolvePadExpression` /
  `ResolvePadCombo` / `IsPadMaskHeld`），因此能在宿主上跑回归测试。
- 键缺失时使用与启动器一致的 Saturn 默认值，保证无 `config.cfg` 的独立启动也能玩。
- 说明：Saturn 手柄没有 Select 键，`saturn.handle.select` 不消费。

### 16.3 路径统一（E1/E3）

| 类别 | 位置 |
|---|---|
| 共享（启动器） | `config/config.cfg`、`data/GameData_Saturn.json`、`bios/saturn/`、`roms/saturn/`、`saves/Saturn/`、`overlays/` |
| 核心私有 | `log/saturn/`（yabause.log、crash.log、mesa.log、video-debug.log）、`cache/saturn/{shaders,pipelines}` |
| 仅用于迁移读取 | `GBAStation/yabasanshiro/**`（旧配置/存档/即时档；`DataRoot()` 保留但注释明确为 legacy） |

### 16.4 自动存读档

新增两个启动器设置键的消费：`save.autoLoadState0`（1 基槽位，进游戏即读）、
`save.autoSaveOnExit`（1 基槽位，退出前写，在核心反初始化之前）。

### 16.5 验证

| 项 | 结果 |
|---|---|
| 回归测试（含新增 14 项映射断言） | **ALL PASS，29/29** |
| Switch 完整构建 | 成功，无新增 warning，`GBAStationYabaSanshiroStub.nro` 26,271,744 B |
| 实机 | 未做 |

### 16.6 M2 收尾状态

已完成：B2、B3（`saturn.handle.*` / `saturn.hotkey.*` / `save.*`）、B5–B10、需求四统计、E1–E3。

按里程碑表仍然留给后续的（**不是遗漏**）：
- `fastforward.*`（倍速）→ **M5**，与快进功能一起做
- `display.showFps` → M5，与 FPS 开关一起做
- `UI.language`（多语言）→ UI 阶段，需要先有语言包与 UI 文本抽取

---

## 17. M3 第一步执行记录（已提交 `2291b31`）

### 17.1 七标签页菜单

| Tab | 内容 |
|---|---|
| 返回游戏 | 信息页，按 A 关闭菜单继续游玩 |
| 保存状态 | 10 个档位列表 + 缩略图预览，A 存、X 删 |
| 读取状态 | 同一份档位列表，A 读（空档给出提示） |
| 独立设置 | 设置列表（当前与"全局设置"同一份内容） |
| 全局设置 | 设置列表（现有运行时生效项） |
| 重置游戏 | 信息页，按 A 原地重开本游戏 |
| 退出游戏 | 信息页，按 A 退出（`--return` 存在时链回启动器） |

操作：左列 Up/Down 换标签，A/Right 进入内容，内容里 B 返回左列，左列 B 关闭菜单，
菜单热键再按一次直接关闭。底部提示栏按上下文显示（档位页显示 "X 删除"）。

绘制新增 `NvgUiDrawTabbedMenu()`（左列标签 + 右内容面板 + 缩略图 + 状态行 + 提示栏），
沿用原有暗色主题与 Switch 图标字体。

### 17.2 10 个存档档位 + 缩略图

- 文件：`<savePath>/<stem>.ss1 … .ss10`（1 基，与菜单"档位 1..10"一致），缩略图同名的 `.png`。
- 列表显示：档位号、**存档时间**（文件 mtime，`%y-%m-%d %H:%M`）、文件大小；空档显示"空"。
- **缩略图 = 存档时画面**：菜单打开时抓一帧（`VIDCore->GetScreenshot`），
  RGBA8 自底向上 → 垂直翻转、alpha 置不透明、按整数倍 box 缩放到 320 宽；
  存档时写成 PNG，列表高亮该档位时显示。
- 新增 `gbastation/Thumb.{h,cpp}`；PNG 写出复用同项目 melonDS stub 的
  `stb_image_write.h`，PNG 读取由 nanovg 自带的 `stb_image` 完成（无新增解码器）。
- 已知取舍：缩略图取的是"打开菜单那一帧"，因此画面不会包含菜单本身；菜单打开期间
  游戏暂停，缩略图与存档时刻的画面一致。

### 17.3 重置游戏

- 不重启进程：`RunGameLoop` 返回值由 `bool` 改为 `enum GameLoopExit { Quit, Reset }`，
  重置时 main 重新 `yabauseinit()` 同一 ROM 并继续会话。
- 两个必须的副作用处理：**`playCount` 不重复计数**（`play_counted` 守卫）、
  **重置不触发"退出自动存档"**（避免覆盖 `save.autoSaveOnExit` 的档位）。

### 17.4 编号统一

`save.autoLoadState0` / `save.autoSaveOnExit` 的槽位号、菜单档位号、文件名编号统一为
**1 基**（`N` ↔ `.ssN`）；快速存读档热键固定使用 1 号档位。

### 17.5 验证

| 项 | 结果 |
|---|---|
| 回归测试 | **ALL PASS，35/35**（新增：box 缩放尺寸/均值/no-op 边界、PNG 签名、空图拒绝） |
| Switch 完整构建 | 成功；仅有 1 条既有的 unused-function warning（`DrawBootFrame`，与本次无关） |
| 实机 | 未做：菜单位图、档位存取、缩略图显示、重置游戏都需要上机验证 |

### 17.6 M3 剩余

- [ ] **画面模式 4 档**：全屏 / 原比例 / 整数倍 / 自定义，映射 GameData `displayMode` 0/1/2/3/4
      + `integerAspectRatio` + `customScale/customOffsetX/Y`（需要改 VIDVulkan 的视口/缩放路径）
- [ ] **遮罩 PNG**：读取 GameData `overlayPath`/`overlayEnabled`，叠在游戏画面上
      （建议走 nanoVG 图像，与缩略图同一套路径）
- [ ] 档位列表的视觉细化（缩略图占位、滚动、长按连续移动）

---

## 18. 全部完成状态与实机验证清单

### 18.1 计划内项目全部落地

| 里程碑 | 内容 | 提交 |
|---|---|---|
| M1 | 三种启动方式接线、config.cfg 路径与类型标签、热键读配置 | `79284cc` |
| M2-1 | GameData_Saturn.json 读写、savePath、游玩统计 | `6858e54` |
| M2-2 | nlohmann/json 3.12（与启动器同一份）、`saturn.handle.*` 组合键映射、路径统一、自动存读档 | `dcdd335` |
| M3-1 | 七标签页菜单、10 个存档档位 + 缩略图、重置游戏 | `2291b31` |
| M4 | 独立设置 / 全局设置双存储 + `noSync` 开关 | `4847c23` |
| M3-2 | 遮罩 PNG 合成 | `7797b25` |
| M5-1 | 快进（触发可配置 + **变调**） | `21de1d5` |
| M3-3 | 画面模式 4 档（全屏 / 原比例 / 整数倍 / 自定义） | `6c91989` |
| — | 回归测试 57 项（GameDb / 按键解析 / 缩略图 / 双设置模型） | 每次提交均通过 |

前端侧只需一处改动（Saturn 跳过会话统计），已在 `1eb2d726` 完成。

### 18.2 实机验证清单（按顺序做，出问题时看日志）

日志位置：`sdmc:/GBAStation/log/saturn/yabause.log`（失败会写到 `crash.log`）。

**A. 三种启动方式**
- [ ] 启动器 → 选 Saturn 游戏 → 直接进游戏（不停在核心自己的列表页）
- [ ] 游戏内菜单 → 退出游戏 → 回到 GBAStation（不是回桌面）
- [ ] 桌面图标（forwarder）→ 退出 → 回桌面，不进 GBAStation
- [ ] hbmenu 无参数启动 → 出现核心自己的列表页 → 选游戏 → 退出 → 回列表页
- [ ] 检查 `sdmc:/GBAStation/config/` 下**没有** `external_core_session.json`

**B. 配置与统计**
- [ ] 前端改 `core.saturn.video_filter` / `region` → 重启核心生效
- [ ] 核心菜单里改一项设置 → 前端页面显示同一个值（验证 `i|/f|/s|` 类型标签）
- [ ] 玩一次后 `GameData_Saturn.json` 的 `playCount` **只 +1**
- [ ] `playTime` 随实际时长增长；`lastPlayed` 是退出时刻，前端"上次游玩"显示正常
- [ ] 存档目录 = GameData 的 `savePath`（空时会自动写入并建目录）

**C. 按键（重点验组合键）**
- [ ] 默认键位可玩：A/B/C/X/Y/Z/L/R/方向/Start
- [ ] 在启动器把某个键改成**组合键**（如 `PAD_LT+PAD_RT`）→ 必须同时按住才触发，单按无效
- [ ] 把某个键加**第二个绑定**（`PAD_A|PAD_B`）→ 两个键都能触发
- [ ] 把某个键设为 `none` → 该键彻底无效（不会退回默认键）

**D. 菜单**
- [ ] 菜单热键（默认 ZL+ZR，可在启动器映射页改）打开/关闭
- [ ] 7 个标签页都能进、B 逐级返回；底部提示栏随上下文变化
- [ ] 保存状态 → 选档位 → 存 → 列表出现**时间与大小**；进 读取状态 → 同一档位能读
- [ ] 选中档位时右侧显示**存档时画面**的缩略图（PNG 在存档目录里）
- [ ] X 删除档位
- [ ] 重置游戏 → 游戏重开且**不退出核心**；`playCount` 不再 +1
- [ ] `save.autoSaveOnExit`（如设 1）→ 退出时写档；**重置不应触发**它

**E. 独立设置 / 全局设置**
- [ ] 独立设置 → 第一项「启动独立设置开关」→ 开启后改一项（如滤镜）→ 存档目录出现 `saturn.settings.cfg`
- [ ] 退出重进 → 独立设置仍生效（GameData `noSync` 为 1）
- [ ] 关闭开关 → 立刻切回全局值；重新开启 → 独立值回来
- [ ] 前端"同步画面设置到同平台"时，该游戏被跳过（`noSync` 语义一致）

**F. 遮罩 / 画面模式 / 快进**
- [ ] 前端给该游戏设遮罩 PNG 并开启 → 游戏画面上叠加遮罩；关闭后消失
- [ ] 画面模式：原比例 / 全屏 / 4:3 各试一次；**整数倍**应无黑边且像素整齐；**自定义**缩放与偏移可调
- [ ] 快进：按住（或按配置切 toggle）→ 速度提升、**音高上升**（变调）而不是静音
- [ ] `fastforward.mute` 设为 1 → 快进时静音

**G. 回归**
- [ ] PSP / PS1 / DC 启动后仍能正常返回，游玩时长正常累计

### 18.3 已知未验证/风险点（按风险排序）

1. **画面模式 2/3（整数倍、自定义）**：改了 `VIDVulkan::updateRenderSize()` 的视口矩形计算，纯逻辑审查、无法离线验证渲染结果。若出现画面错位/黑边，优先回看这一段。
2. **快进变调**：`SNDSDLSetPitch()` 改变了重采样步长；`SOUNDRATIOMIN/MAX` 的时钟校正与 pitch 分离，但快进时环形缓冲的供给节奏需实机确认无断续。
3. **遮罩**：每帧多一个 nanoVG pass（仅在启用时挂载），需确认对帧率的影响。
4. **缩略图**：`VIDCore->GetScreenshot` 在菜单打开瞬间调用，若与 Vulkan 命令缓冲提交冲突可能拿不到帧（此时只是没有缩略图，不崩）。
5. **重置游戏**：`yabauseinit()` 在同一进程内二次初始化，需确认无资源泄漏（连续重置多次观察）。

---

## 19. 日志系统（完善后）

### 19.1 位置与开关

| 项 | 值 |
|---|---|
| 日志文件 | `sdmc:/GBAStation/log/saturn/yabause.log` |
| 崩溃日志 | `sdmc:/GBAStation/log/saturn/crash.log`（异常处理器写 PC/LR/SP/寄存器） |
| 其它 | `mesa.log`、`video-debug.log` 同目录 |
| 开关 | **默认开启**；`YAB_LOG=0`（或 `off`/`no`）关闭 |
| 体积控制 | 超过 2 MB 启动时转存为 `yabause.log.old` |
| 格式 | `[YYYY-MM-DD HH:MM:SS] <内容>`，逐行 flush；每次启动写一条 session 分隔行 |
| 同时输出 | 文件 + stdout（接了 nxlink 时终端也能看到） |

### 19.2 事件标签（便于 grep）

| 标签 | 记录内容 |
|---|---|
| `[argv]` | **完整命令行**（argc 与每一项） |
| `[launch]` | 解析出的 rom / return_nro / session_token / 启动方式（启动器链式 or 内置列表页）；返回启动器前的 return_nro 与 `envHasNextLoad()`；exiting core |
| `[paths]` | DataRoot、LogDir、CacheDir、config.cfg、GameData_Saturn.json、roms 目录 |
| `[game]` | rom / bios（或用模拟 BIOS）/ 电池存档路径 / savePath / **是否启用独立设置及其文件** / 当前使用哪套设置 / 启动成功 / yabauseinit 失败 / 重置 / 会话结束原因 |
| `[browser]` | 内置列表页：根目录与条目数、进入设置、切换目录、选中游戏、退出 |
| `[menu]` | 菜单开/关、标签切换、进入内容页（条目数与当前存储）、重置游戏、退出游戏 |
| `[slots]` | 存/读/删档位（槽位号、路径、成功与否、是否写缩略图） |
| `[setting]` | 设置改动（键、旧值→新值、写入独立还是全局、是否保存成功、是否即时生效）；`noSync` 单独记录 |
| `[input]` | 启动时解析出的每个 Saturn 按键掩码（来自配置 / 默认值） |
| `[hotkey]` | 快捷存/读档（槽位、路径、返回值） |
| `[fastforward]` | 快进开/关（倍率、按住/切换、是否静音） |
| `[overlay]` | 遮罩路径、图像句柄、是否启用 |
| `[auto]` | 自动读档 / 退出自动存档（槽位、路径、结果） |
| `[stats]` | playCount、playTime 定期与退出结算 |
| `[boot]` | 启动分段耗时（覆盖层呈现、config 读取、cart 解析、YabauseInit、显示设置、OSD）、Mesa 着色器缓存开关与目录（见 §20） |
| `[applet]` | appletMainLoop 结束（窗口关闭/系统请求） |

### 19.3 排查用法

```sh
# 这次启动收到了什么参数
grep '\[argv\]' yabause.log
# 菜单里改了什么
grep -E '\[menu\]|\[setting\]|\[slots\]' yabause.log
# 统计是否正确（playCount 只 +1、playTime 增长）
grep '\[stats\]' yabause.log
# 是否成功链回启动器
grep '\[launch\]' yabause.log
# 启动慢在哪一段 / 退出崩在哪一步（见 §20）
grep '\[boot\]' yabause.log
tail -30 yabause.log
```

---

## 20. 实机反馈处理：启动慢与退出崩溃（本轮）

两条来自实机日志的问题，代码侧已加可观测性与防护，**根因待下一份日志确认**。

### 20.1 为什么每次启动都像在"编译着色器"

| 事实 | 依据 |
|---|---|
| 覆盖层文案此前写作"正在编译着色器…"，**与实测不符** | 日志里 `bootpage: nanovg ready` 与 `VIDVulkan::init enter` 相隔约 10 s，而 `VIDVulkan::init` 本身在同一秒内返回；10 s 花的不是视频核心的着色器编译阶段 |
| 驱动侧着色器缓存此前被整体关闭 | `MESA_SHADER_CACHE_DISABLE=1`（随最初的 Switch 移植提交引入，无说明）。NVK 需把 SPIR-V 翻成 GM20B 机器码，关掉磁盘缓存后**每次启动都重做** |
| 核心自己的 glslang 缓存是有效的 | `<CacheDir>/shaders/<glsl hash>.spv`，命中后跳过 glslang；不覆盖驱动那一半 |
| 每次启动都是新进程 | 链式启动（chainload）没有常驻进程，内存里的管线缓存一律不带过来 |

本轮改动（`ae30ea2`）：

1. 文案改为"正在启动游戏…"，不再误导排查方向；
2. `MESA_SHADER_CACHE_DIR` 指向 `sdmc:/GBAStation/cache/saturn/mesa`（目录由 `EnsureSaturnDirectories()` 创建），即**默认重新启用驱动磁盘缓存**；若某固件/驱动组合不接受，在 `config.cfg` 加 `core.saturn.mesa_shader_cache=0` 退回旧行为；
3. 开关状态写入日志：`[boot] mesa shader cache: on dir=... / off`。

### 20.2 启动耗时分段（新增埋点，`fddd2cd`）

`yabauseinit()` 内按阶段打印本段与累计毫秒；`main` 里打印覆盖层呈现耗时：

```
[boot] overlay shown after NNN ms        # 含覆盖层首帧渲染 + present
[boot]   phase config.cfg read           +NNN ms (total NNN ms)
[boot]   phase config + cart resolved    +NNN ms (total NNN ms)
[boot]   phase YabauseInit               +NNN ms (total NNN ms)
[boot]   phase display settings push     +NNN ms (total NNN ms)
[boot]   phase OSD init                  +NNN ms (total NNN ms)
[boot] yabauseinit took NNN ms
```

判读：`overlay shown after` 很大 → 卡在首个 nanovg 帧的驱动管线编译（启用 20.1 的缓存后应显著下降）；该值很小而 `phase YabauseInit` 很大 → 卡在核心初始化内部，再按段细化。

### 20.3 为什么退出游戏会崩溃

旧日志最后一行是 `=== clean exit ===`，**不能**说明崩溃点：当时紧跟着 `YabLogShutdown()` 就把文件关了，之后任何阶段（`YabauseDeInit` → 单例销毁 → Renderer/Window 析构 → 静态析构 → loader 退出）崩溃都写不进来。

已提交的埋点（`fddd2cd`）把这条链全部打开：

| 日志行 | 位置 |
|---|---|
| `main: end of renderer/window scope; destructors next` | 循环结束、`Renderer`/`Window` 析构之前 |
| `VIDVulkan::deInit enter` → `deInit: shader manager freed` → `deinit: scene deinit done` → `deleting singleton (delete this)` → `singleton deleted` | `VIDVulkan::deInit()` |
| `~Renderer: enter / window destroyed / device destroyed / instance destroyed / done` | `Renderer::~Renderer()` |
| `~Window: enter / queue idle / … / OS window / done`（每步一条） | `Window::~Window()` |
| `atexit: main returned; static destructors next` | `main` 返回后、静态析构之前 |

判读规则：

- 最后一行是 `main: end of renderer/window scope…` → 崩在 `Renderer/Window` 析构（再看 `~Window:` 停在哪一步）；
- 最后一行是 `delete this` / `singleton deleted` 之间 → 崩在单例析构；
- 出现 `atexit:` 之后没有别的 → 崩在静态析构或 loader 退出；
- 崩溃寄存器现场在 `crash.log`。

同时做了三处防护性修复（`59523c1`，均不改变正常路径行为）：

1. `VIDVulkanDeInit()` 改走 `VIDVulkan::destroyInstance()`：单例已销毁时不再经 `getInstance()` **复活**一个从未初始化的实例（其 `deInit()` 会解引用已释放的 `pipleLineFactory`）；
2. `VulkanScene::deInit()` 对 `_command_pool` / `_render_complete_semaphore` 加空句柄判断——销毁 `VK_NULL_HANDLE` 在 loaderless NVK 里是非法调用；
3. `VulkanScene::present()` 在 `_renderer == nullptr` 时直接返回。

### 20.4 下一份日志需要采集

1. `sdmc:/GBAStation/log/saturn/yabause.log`（**完整**，特别是末尾 30 行）；
2. `sdmc:/GBAStation/log/saturn/crash.log`（若存在）；
3. `sdmc:/GBAStation/debug/external_core_launch.log`（看有没有 `launcher entry`）；
4. `sdmc:/GBAStation/log/GBAStation.log`（启动器侧，若已开 `debug.logFile`）；
5. `sdmc:/GBAStation/log/saturn/mesa.log`（若启用缓存后启动异常）。

### 20.5 追加实机结论：只有链回启动器时崩

前提事实是**只有**"退出游戏 → 链回启动器"这条路崩，这把范围缩到"退出 / 交接"这一段，
本轮（`70bb3a1` + 前端 `26193eb5`）据此做了四件事：

| 改动 | 作用 |
|---|---|
| 交接探针：包住 loader 的 return function | 日志里出现 `handoff: loader return function entered rc=0` 才说明进程**活着**走到了交接；在这行之前断掉，就是崩在我们自己的收尾里 |
| 提前布防：`main` 解析完 argv 立即 `ReturnToLauncher()` | `envSetNextLoad()` 只写 loader 的 next-load 缓冲，提前调用无副作用；代价是**运行期/收尾期崩溃也能回到启动器**，而不是掉到 HOME Menu 的错误框。函数幂等，退出路径再调只记 `already armed` |
| 异常处理器改走 loader | dump 完 `crash.log` 后若已布防则 `__libnx_exit(0)`，让 loader 的 return function 执行；libnx 默认是 `svcExitProcess()`，**会跳过**它 |
| 启动器进程入口标记 | 前端 `main` 第一条语句往 `external_core_launch.log` 追加 `launcher entry t=<epoch> argc=N argv[...]`，用来区分"core 没回来"和"启动器自己崩" |

参考实现：同工作区的 `GBAStation_DrasticDS/source/error.c`（`fatal_error_set_exit_to_loader`）
与 `source/main.c`（`configure_return_to_launcher` 在 `main` 开头就布防，注释明写
"Scheduling it last (after the whole teardown) lost the return whenever a teardown step hung"），
即这套"提前布防 + 崩溃也走 loader"的做法在本项目里已有验证过的先例。

判读表（下次崩溃时对号入座）：

| 现象 | 结论 |
|---|---|
| `yabause.log` 停在 `main: end of renderer/window scope` 之前 | 崩在我们的收尾（看 `~Renderer:` / `~Window:` 最后一条） |
| 有 `atexit:` 但没有 `handoff:` | 崩在静态析构或 `__libnx_exit` 之前的路径 |
| 有 `handoff:` 且启动器 `launcher entry` 出现 | 交接成功，崩在启动器内部（看前端日志） |
| 有 `handoff:` 但没有 `launcher entry` | 崩在 loader 交接/加载下一个 NRO（不在我们代码里） |
| 有 `CRASH:` 行 | 异常现场以 `crash.log` 为准；若 `chainload_armed=1` 应能回到启动器 |
4. `sdmc:/GBAStation/log/saturn/mesa.log`（若启用缓存后启动异常）。

### 20.6 实机日志（20:35）新增结论

该次日志确认了两件事：

1. **启动慢与着色器编译无关**：`[boot] overlay shown after 69 ms`、`VIDVulkan::init` 自身 <1 s，
   而 `phase YabauseInit +10704 ms` —— 10.7 秒全在 `YabauseInit()` 内、且在视频核心起来**之前**。
   为此在 `yabause.c` 里给 `YabauseInit` 的每一步加了 `  yinit <step> +N ms` 计时
   （`YabThreadInit` / `DebugCleanupOldFiles` / `SH2Init` / `T2MemoryInit` / backup /
   `CartInit` / `MappedMemoryInit` / `VideoInit` / `PerInit` / `Cs2Init(CD/ISO)` / `ScuInit` /
   `M68KInit` / `ScspInit` / `Vdp1Init` / `Vdp2Init` / `SmpcInit`），下次启动即可指名道姓。
2. **退出日志停在 `atexit: main returned`**，且交接探针一行都没有。可能是在静态析构里卡死/崩溃，
   也可能是探针写的是已被 newlib 关闭的流。处理：探针额外写独立文件
   `sdmc:/GBAStation/log/saturn/handoff.log`；并且**已布防时直接 `__libnx_exit(0)`**，
   跳过 C++ 静态析构（正是旧日志再也没走过去的那一段）。

另按要求新增 `[cfg]` 生效项日志：只列核心真正消费的固定键表
（`core.saturn.*` / `save.*` / `fastforward.*` / `saturn.handle.fastforward` / `scan.path.saturn`），
值来自独立设置文件时标注 `[独立设置]`，未设置的键合并成一行——**不 dump 整个 config.cfg**。

```sh
grep 'yinit' yabause.log     # 启动慢在哪一步
grep '\[cfg\]' yabause.log   # 这次实际生效的核心设置
cat handoff.log              # 是否走到了 loader 交接
```
