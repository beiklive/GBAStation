# Session 90 任务分析：模拟器目录存档按游戏名建子目录

## 任务目标

当 `save.stateDir`（或 `save.sramDir`）设置为"保存到模拟器目录"时，实际保存目录应为 `模拟器目录/saves/游戏文件名/`，防止存档文件在 saves 根目录堆积。

## 输入

- `save.stateDir` 配置值：空字符串（与ROM同目录）或 `BK_APP_ROOT_DIR + "saves"`（模拟器目录）
- ROM 文件路径（`m_romPath`）：用于提取游戏文件名（不含扩展名）

## 输出

- 存档文件路径应变为 `BK_APP_ROOT_DIR/saves/<游戏文件名>/xxx.ss1`（即时存档）
- SRAM存档文件路径应变为 `BK_APP_ROOT_DIR/saves/<游戏文件名>/xxx.sav`
- RTC存档文件路径也应变为同一子目录

## 关键代码位置

### 现状
`src/Game/game_view.cpp` 中的 `resolveSaveDir` 函数：
```cpp
std::string GameView::resolveSaveDir(const std::string& romPath,
                                      const std::string& customDir)
{
    if (!customDir.empty()) return customDir;  // ← 直接返回，不加子目录
    if (!romPath.empty()) {
        return beiklive::file::getParentPath(romPath);
    }
    return BK_APP_ROOT_DIR + std::string("saves");
}
```

### 问题
当 `customDir = BK_APP_ROOT_DIR + "saves"` 时，所有游戏的存档文件都堆积在同一个 `saves/` 目录下。

## 解决方案

修改 `resolveSaveDir`：当 `customDir` 非空且 `romPath` 非空时，在 `customDir` 后追加 `/<游戏文件名（不含扩展名）>/`，形成每游戏独立子目录。

```cpp
std::string GameView::resolveSaveDir(const std::string& romPath,
                                      const std::string& customDir)
{
    if (!customDir.empty()) {
        // 追加游戏文件名子目录，防止存档文件堆积
        if (!romPath.empty()) {
            std::filesystem::path p(romPath);
            return customDir + "/" + p.stem().string();
        }
        return customDir;
    }
    if (!romPath.empty()) {
        return beiklive::file::getParentPath(romPath);
    }
    return BK_APP_ROOT_DIR + std::string("saves");
}
```

## 影响范围

- `quickSaveStatePath`：即时存档路径（使用 `save.stateDir`）
- `sramSavePath`：SRAM 电池存档路径（使用 `save.sramDir`）
- `rtcSavePath`：RTC 时钟存档路径（复用 `save.sramDir`）

三者均调用 `resolveSaveDir`，此修改一次性覆盖所有存档类型。

## 可能的挑战

- 无需迁移已有存档（用户需要手动移动旧文件，或可接受从头开始）
- 目录创建逻辑已在各 `*Path()` 函数中通过 `create_directories` 处理，子目录会自动创建
