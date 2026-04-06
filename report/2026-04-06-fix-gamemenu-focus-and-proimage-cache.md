# 工作汇报：修复GameMenu焦点记忆与ProImage字节缓存问题

## 任务分析

### 任务目标
修复两个独立的UI/缓存问题：
1. GameMenu的保存/读取状态子面板每次进入时聚焦上次选择的位置，而非第一个item
2. 可视化倒带（DataPage存档面板/ProImage异步加载）每次关闭后缓存会丢失，导致重复打开时item数量少

### 输入
- 用户反馈的两个bug描述
- 涉及文件：`src/UI/Utils/GameMenu.cpp`、`src/UI/Utils/ProImage.cpp`

### 输出
- 两个针对性的代码修复

## 问题1：GameMenu焦点记忆

### 根本原因
borealis的`Box::getDefaultFocus()`有以下优先级：
1. 若`lastFocusedView`不为空，返回上次聚焦的子视图
2. 否则返回`defaultFocusedIndex`对应的子视图（默认第0个）

调用`brls::Application::giveFocus(m_saveStateItemBox)`时，borealis会调用`getDefaultFocus()`，从而返回上次聚焦的槽位按钮，而非第一个按钮。

### 修复方案
在A键/右键处理函数中，`giveFocus`之前调用`setLastFocusedView(nullptr)`重置焦点记忆：
```cpp
m_saveStateItemBox->setLastFocusedView(nullptr);
brls::Application::giveFocus(m_saveStateItemBox);
```

### 涉及位置
- `src/UI/Utils/GameMenu.cpp`：保存状态按钮的A键和右键handler
- `src/UI/Utils/GameMenu.cpp`：读取状态按钮的A键和右键handler

## 问题2：ProImage字节缓存丢失

### 根本原因
`ProImage::setImageFromFileAsync`的异步回调中，`storeBytes`调用位于`ASYNC_RELEASE`宏之后。`ASYNC_RELEASE`宏在检测到视图已被销毁时会提前`return`，导致字节缓存永远无法被填充。

场景：
1. 用户打开包含ProImage的页面（如DataPage存档面板）
2. 图片开始异步加载（后台线程读取文件）
3. 用户快速关闭页面 → ProImage视图被销毁，`deletionToken`被标记为已释放
4. 异步回调在主线程触发 → `ASYNC_RELEASE`检测到视图销毁 → 提前`return`
5. `storeBytes`**从未被调用**，字节缓存为空
6. 用户再次打开页面 → `getBytes`返回nullptr → 重新触发异步加载 → 循环往复
7. 图片始终无法被缓存，每次打开页面图片都需要重新从磁盘加载

### 修复方案
将`storeBytes`移到`ASYNC_RELEASE`检查之前，确保字节无论视图是否存活都能写入缓存：
```cpp
// 字节缓存在视图存活性检查前写入，保证快速路径的缓存命中
if (!bytes.empty())
    beiklive::UI::ImageFileCache::instance().storeBytes(capturedPath, bytes);

ASYNC_RELEASE  // 视图已销毁时，仅跳过NVG纹理创建；缓存已填充
```

### 涉及位置
- `src/UI/Utils/ProImage.cpp`：`setImageFromFileAsync`的异步回调

## 修改说明
- 修改极小，每处只改动2-5行
- 不影响已有功能逻辑，只调整执行顺序
- 无新增依赖或结构性变更
