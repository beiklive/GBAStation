# Borealis Core API 接口手册

> 源文件: `view.hpp` / `box.hpp`  
> 基类层次: `brls::View` -> `brls::Box`

---

## 一、枚举类型

### 1.1 ViewBackground - 背景样式

| 值 | 含义 |
|---|------|
| `NONE` | 无背景 |
| `SIDEBAR` | 侧边栏背景 |
| `BACKDROP` | 遮罩/弹窗背景 |
| `SHAPE_COLOR` | 纯色形状背景（配合 `setBackgroundColor` 使用） |
| `VERTICAL_LINEAR` | 纵向渐变背景 |

### 1.2 Visibility - 可见性

| 值 | 含义 |
|---|------|
| `VISIBLE` | 完全可见 |
| `INVISIBLE` | 不可见但保留布局空间 |
| `GONE` | 不可见且不占空间 |

### 1.3 PositionType - 定位模式

| 值 | 含义 |
|---|------|
| `RELATIVE` | 基于布局位置偏移（默认） |
| `ABSOLUTE` | 绝对定位，自由摆放 |

### 1.4 ShadowType - 阴影类型

| 值 | 含义 |
|---|------|
| `NONE` | 无阴影 |
| `GENERIC` | 通用阴影 |
| `CUSTOM` | 自定义阴影 |

### 1.5 TransitionAnimation - 过渡动画

| 值 | 效果 |
|---|------|
| `FADE` | 淡入淡出 |
| `SLIDE_LEFT` | 向左滑出 / 从右滑入 |
| `SLIDE_RIGHT` | 向右滑出 / 从左滑入 |
| `NONE` | 无动画 |
| `LINEAR` | 线性过渡 |

### 1.6 Axis - Flex 主轴方向 (Box)

| 值 | 含义 |
|---|------|
| `ROW` | 水平排列 |
| `COLUMN` | 垂直排列 |

### 1.7 JustifyContent - 主轴对齐 (Box)

| 值 | 含义 |
|---|------|
| `FLEX_START` | 靠左/靠上 |
| `CENTER` | 居中 |
| `FLEX_END` | 靠右/靠下 |
| `SPACE_BETWEEN` | 两端对齐，间距均分 |
| `SPACE_AROUND` | 两端半间距，项间均分 |
| `SPACE_EVENLY` | 等距分布 |

### 1.8 AlignItems - 交叉轴对齐 (Box)

| 值 | 含义 |
|---|------|
| `AUTO` | 自动 |
| `FLEX_START` | 靠左/靠上 |
| `CENTER` | 居中 |
| `FLEX_END` | 靠右/靠下 |
| `STRETCH` | 拉伸填充 |
| `BASELINE` | 基线对齐 |
| `SPACE_BETWEEN` | 两端对齐 |
| `SPACE_AROUND` | 均布对齐 |

### 1.9 Direction - 方向 (Box)

| 值 | 含义 |
|---|------|
| `INHERIT` | 继承父容器（默认） |
| `LEFT_TO_RIGHT` | 从左到右 |
| `RIGHT_TO_LEFT` | 从右到左 |

### 1.10 FocusDirection - 焦点方向

| 值 |
|---|
| `UP` |
| `DOWN` |
| `LEFT` |
| `RIGHT` |

### 1.11 AlignSelf - 子项自身对齐

| 值 | 含义 |
|---|------|
| `AUTO` | 继承父容器 |
| `FLEX_START` | 靠左/靠上 |
| `CENTER` | 居中 |
| `FLEX_END` | 靠右/靠下 |
| `STRETCH` | 拉伸填充 |
| `BASELINE` | 基线对齐 |

---

## 二、brls::View 接口

### 2.1 静态常量

| 常量 | 值 | 用途 |
|------|-----|------|
| `View::AUTO` | `NAN` | 自动尺寸，用于 `setWidth/setHeight/setMargins` |

### 2.2 生命周期 (Virtual, 可重写)

| 方法 | 签名 | 说明 | 调用时机 |
|------|------|------|---------|
| `frame` | `void frame(FrameContext* ctx)` | 每帧逻辑更新（**不**用于绘制） | 每帧自动调用，先于 `draw()` |
| `draw` | `void draw(NVGcontext* vg, float x, float y, float w, float h, Style style, FrameContext* ctx) = 0` | **纯虚函数**, 每帧渲染绘制 | `frame()` 之后自动调用 |
| `willAppear` | `void willAppear(bool resetState = false)` | 视图即将显示 | Activity push / Tab 切换 / 从隐藏变为可见 |
| `willDisappear` | `void willDisappear(bool resetState = false)` | 视图即将隐藏 | Activity pop / Tab 切换 / 从可见变为隐藏 |
| `onLayout` | `void onLayout()` | 布局完成后回调 | 每次布局 pass 结束 |
| `onShowAnimationEnd` | `void onShowAnimationEnd()` | 显示动画结束后 | `show()` 动效完成时 |
| `onWindowSizeChanged` | `void onWindowSizeChanged()` | 窗口尺寸变化后 | resize 事件后 |

> **注意:** `willAppear`/`willDisappear` 在生命周期内可能被调用零次或多次（如 TabLayout 场景）。
> 这些是框架自动调用的，切勿手动调用。

### 2.3 尺寸与布局

| 方法 | 参数 | 说明 |
|------|------|------|
| `setWidth(float)` | 像素值 / `View::AUTO` | 设置首选宽度，非 AUTO 时保证不小于此值 |
| `setHeight(float)` | 像素值 / `View::AUTO` | 设置首选高度 |
| `setDimensions(float w, float h)` | 像素值 | 同时设置宽高（比分别设置少一次 layout pass） |
| `setSize(Size size)` | Size 结构体 | 同时设置宽高 |
| `setWidthPercentage(float)` | 0-100 | 按父容器宽度百分比设置宽度 |
| `setHeightPercentage(float)` | 0-100 | 按父容器高度百分比设置高度 |
| `setMinWidth(float)` | 像素值 | 最小宽度约束 |
| `setMinHeight(float)` | 像素值 | 最小高度约束 |
| `setMinWidthPercentage(float)` | 0-100 | 最小宽度（百分比） |
| `setMinHeightPercentage(float)` | 0-100 | 最小高度（百分比） |
| `setMaxWidth(float)` | 像素值 | 最大宽度约束 |
| `setMaxHeight(float)` | 像素值 | 最大高度约束 |
| `setMaxWidthPercentage(float)` | 0-100 | 最大宽度（百分比） |
| `setMaxHeightPercentage(float)` | 0-100 | 最大高度（百分比） |
| `setGrow(float)` | 增长因子 | 剩余空间的分配比例，默认为 0 |
| `setShrink(float)` | 收缩因子 | 空间不足时允许收缩的比例，默认为 1 |

### 2.4 边距 (Margin)

| 方法 | 参数 | 说明 |
|------|------|------|
| `setMargins(float top, float right, float bottom, float left)` | 像素值 | 一次性设置四边边距（仅一次 layout pass） |
| `setMarginTop(float)` | 像素值 | 上边距 |
| `setMarginRight(float)` | 像素值 | 右边距 |
| `setMarginBottom(float)` | 像素值 | 下边距 |
| `setMarginLeft(float)` | 像素值 | 左边距 |
| `getMarginRight()` | - | 获取右边距 |
| `getMarginLeft()` | - | 获取左边距 |

### 2.5 定位 (Position)

| 方法 | 参数 | 说明 |
|------|------|------|
| `setPositionType(PositionType)` | `RELATIVE` / `ABSOLUTE` | 定位模式（默认 RELATIVE） |
| `setPositionTop(float)` | 像素值 | 上位置 |
| `setPositionRight(float)` | 像素值 | 右位置 |
| `setPositionBottom(float)` | 像素值 | 下位置 |
| `setPositionLeft(float)` | 像素值 | 左位置 |
| `setPositionTopPercentage(float)` | 0-100 | 上位置（百分比） |
| `setPositionRightPercentage(float)` | 0-100 | 右位置（百分比） |
| `setPositionBottomPercentage(float)` | 0-100 | 下位置（百分比） |
| `setPositionLeftPercentage(float)` | 0-100 | 左位置（百分比） |

### 2.6 平移 (Translation)

| 方法 | 参数 | 说明 |
|------|------|------|
| `setTranslationX(float)` | 像素值 | X 轴平移（layout 之后应用） |
| `setTranslationY(float)` | 像素值 | Y 轴平移（layout 之后应用） |

### 2.7 可见性与显隐动画

| 方法 | 参数 | 说明 |
|------|------|------|
| `setVisibility(Visibility)` | `VISIBLE` / `INVISIBLE` / `GONE` | 设置可见状态 |
| `getVisibility()` | - | 获取可见状态 |
| `show(cb)` | `void()` 回调 | 淡入显示 |
| `show(cb, bool animate, float duration)` | 回调 + 是否动画 + 时长 | 淡入显示（可控参数） |
| `hide(cb)` | `void()` 回调 | 淡出隐藏 |
| `hide(cb, bool animate, float duration)` | 回调 + 是否动画 + 时长 | 淡出隐藏（可控参数） |
| `collapse(bool animated = true)` | 是否动画 | 折叠隐藏（收起动画） |
| `expand(bool animated = true)` | 是否动画 | 展开显示（collapse 的反向） |
| `isHidden()` | - | 是否隐藏中 |
| `isCollapsed()` | - | 是否折叠中 |

### 2.8 样式

| 方法 | 参数 | 说明 |
|------|------|------|
| `setBackground(ViewBackground)` | 枚举 | 设置背景类型 |
| `setBackgroundColor(NVGcolor)` | `nvgRGBA(...)` | 设置纯色背景（自动切换为 SHAPE_COLOR 模式） |
| `setBackgroundCornerRadii(float tl, float tr, float br, float bl)` | 四个圆角半径 | 设置渐变背景的四角圆角 |
| `setBorderColor(NVGcolor)` | `nvgRGBA(...)` | 边框颜色 |
| `setBorderThickness(float)` | 像素值 | 边框粗细 |
| `getBorderThickness()` | - | 获取边框粗细 |
| `setCornerRadius(float)` | 像素值 | 形状圆角半径（0 = 无圆角） |
| `getCornerRadius()` | - | 获取圆角半径 |
| `setShadowType(ShadowType)` | `NONE` / `GENERIC` / `CUSTOM` | 阴影类型（默认 NONE） |
| `setShadowVisibility(bool)` | true/false | 阴影可见性 |
| `setAlpha(float)` | 0.0-1.0 | 透明度 |
| `setClipsToBounds(bool)` | true/false | 是否裁剪超出边界的内容（Image 构造中自动启用） |
| `overrideTheme(Theme*)` | 主题指针 | 强制此视图及其子视图使用指定主题 |

### 2.9 分隔线 (Line)

| 方法 | 参数 | 说明 |
|------|------|------|
| `setLineColor(NVGcolor)` | `nvgRGBA(...)` | 四边线的颜色 |
| `setLineTop(float)` | 像素值 | 上边线粗细 |
| `setLineRight(float)` | 像素值 | 右边线粗细 |
| `setLineBottom(float)` | 像素值 | 下边线粗细（常用于分隔线） |
| `setLineLeft(float)` | 像素值 | 左边线粗细 |

### 2.10 焦点高亮样式

| 方法 | 参数 | 说明 |
|------|------|------|
| `setHideHighlightBackground(bool)` | true/false | 隐藏焦点高亮背景（背后白矩形） |
| `setHideHighlightBorder(bool)` | true/false | 隐藏焦点高亮边框 |
| `setHideHighlight(bool)` | true/false | 隐藏全部高亮 |
| `setHideClickAnimation(bool)` | true/false | 隐藏点击动画 |
| `setHighlightPadding(float)` | 像素值 | 高亮矩形与视图之间的间距 |
| `setHighlightCornerRadius(float)` | 像素值 | 高亮矩形圆角 |

### 2.11 焦点与导航 (可重写)

| 方法 | 签名 | 说明 |
|------|------|------|
| `setFocusable(bool)` | true/false | 是否可获取焦点（需要为 true 才能响应按键） |
| `isFocusable()` | - | 是否可获焦 |
| `isFocused()` | - | 当前是否已获焦 |
| `getDefaultFocus()` | `virtual View*` | 首次进入时的默认焦点视图，返回 nullptr 表示不可获焦 |
| `getNextFocus(direction, currentView)` | `virtual View*` | 返回指定方向的下一个焦点视图 |
| `hitTest(Point point)` | `virtual View*` | 返回屏幕坐标对应的视图 |
| `onFocusGained()` | `virtual void` | 获得焦点时回调 |
| `onFocusLost()` | `virtual void` | 失去焦点时回调 |
| `onParentFocusGained(View* focusedView)` | `virtual void` | 父视图获得焦点时回调 |
| `onParentFocusLost(View* focusedView)` | `virtual void` | 父视图失去焦点时回调 |
| `setFocusSound(Sound)` | 枚举 | 设置获取焦点时的音效 |
| `getFocusSound()` | `virtual Sound` | 获取焦点音效 |

### 2.12 自定义导航路由

| 方法 | 参数 | 说明 |
|------|------|------|
| `setCustomNavigationRoute(FocusDirection, View*)` | 方向 + 目标 | 设置自定义焦点导航路由 |
| `setCustomNavigationRoute(FocusDirection, std::string)` | 方向 + 目标ID | 按 ID 设置路由（运行时解析） |
| `hasCustomNavigationRouteByPtr(direction)` | - | 是否已设置指针对应的路由 |
| `hasCustomNavigationRouteById(direction)` | - | 是否已设置 ID 对应的路由 |
| `getCustomNavigationRoutePtr(direction)` | - | 获取路由目标指针 |
| `getCustomNavigationRouteId(direction)` | - | 获取路由目标 ID |

### 2.13 按键注册 (Actions)

| 方法 | 签名 | 说明 |
|------|------|------|
| `registerAction(hintText, ControllerButton, ActionListener, hidden, allowRepeating, sound)` | 动作注册 | 注册手柄按键动作，返回 ActionIdentifier |
| `registerAction(BrlsKeyCombination, ActionListener, allowRepeating)` | 键盘组合键注册 | 注册键盘快捷键 |
| `registerClickAction(ActionListener)` | 快捷方法 | 等价于 `registerAction("", BUTTON_A, ..., SOUND_CLICK)` |
| `unregisterAction(ActionIdentifier)` | 注销动作 | 按 ID 移除已注册的动作 |
| `updateActionHint(ControllerButton, hintText)` | 更新提示文字 | 修改底部提示文本 |
| `setActionAvailable(ControllerButton, bool)` | 单个按钮可用性 | 设置某按键是否可用 |
| `setActionsAvailable(bool)` | 全部按钮可用性 | 批量设置所有按键可用性 |

**ActionListener 签名:** `std::function<bool(View*)>`
返回 `true` 表示已消费，返回 `false` 表示未消费。

**常用 ControllerButton 枚举值:**
```
BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y
BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT
BUTTON_LB, BUTTON_RB, BUTTON_LT, BUTTON_RT
BUTTON_START, BUTTON_BACK
BUTTON_LSB, BUTTON_RSB
```

### 2.14 点击动画

| 方法 | 参数 | 说明 |
|------|------|------|
| `resetClickAnimation()` | - | 重置点击动画状态 |
| `playClickAnimation(bool reverse, bool animateBack, bool force)` | 是否反向/弹回/强制 | 播放点击动画 |

### 2.15 手势识别

| 方法 | 参数 | 说明 |
|------|------|------|
| `addGestureRecognizer(GestureRecognizer*)` | 手势识别器 | 添加手势 |
| `interruptGestures(bool onlyIfUnsureState)` | 是否仅中断 UNSURE 状态 | 中断手势识别 |
| `getGestureRecognizers()` | - | 获取所有手势识别器 |

### 2.16 视图树查询

| 方法 | 签名 | 说明 |
|------|------|------|
| `getView(std::string id)` | `virtual View*` | 在自身及子视图中递归查找指定 ID 的视图 |
| `getNearestView(std::string id)` | `virtual View*` | 向上遍历树查找最近匹配 ID 的视图 |
| `getParent()` / `hasParent()` | `Box*` / `bool` | 获取父 Box / 是否有父视图 |
| `setParent(Box*, void* userdata)` | 父视图 + 用户数据 | 设置父视图 |
| `getParentUserData()` | `void*` | 获取父视图附加的用户数据 |

### 2.17 空间信息获取

| 方法 | 返回值 | 说明 |
|------|--------|------|
| `getFrame()` | `Rect` | 全局帧矩形（相对于窗口） |
| `getX()` / `getY()` | `float` | 全局 X / Y 坐标 |
| `getLocalFrame()` | `Rect` | 本地帧矩形 |
| `getLocalX()` / `getLocalY()` | `float` | 本地 X / Y 坐标 |
| `getWidth()` | `float` | 视图宽度 |
| `getHeight(bool includeCollapse = true)` | `float` | 视图高度（可选包含折叠状态） |

### 2.18 其他

| 方法 | 参数 | 说明 |
|------|------|------|
| `setId(std::string)` | 字符串 | 设置视图 ID（用于 `getView()` / 导航路由） |
| `setAlignSelf(AlignSelf)` | 枚举 | 覆盖父 Box 的 align items |
| `setAspectRatio(float)` | 宽高比 | 设置宽高比约束 |
| `invalidate()` | - | 触发整个视图树重新布局 |
| `removeFromSuperView(bool free = true)` | 是否释放 | 从父视图中移除 |
| `setWireframeEnabled(bool)` | true/false | 开启线框模式（调试用，显示视图边界） |
| `setCulled(bool)` | true/false | 设置是否可被父 Box 裁剪 |
| `detach()` | - | 将视图从 Yoga 布局节点分离（不可逆） |
| `setDetachedPosition(x, y)` / `setDetachedPositionX(x)` / `setDetachedPositionY(y)` | 坐标 | 设置分离视图的位置 |
| `setInFadeAnimation(bool)` | true/false | 强制视图为半透明（用于淡入动画） |
| `freeView()` | - | 安全释放视图（检查 ptr lock） |
| `getAlpha(bool child = false)` | `virtual float` | 获取透明度 |
| `present(View*)` | 要呈现的视图 | 在当前视图上 present 新视图 |
| `dismiss(cb)` | 回调 | 关闭 present 的视图 |
| `getAppletFrame()` | `virtual AppletFrame*` | 获取 AppletFrame |

### 2.19 异步安全

| 宏 / 方法 | 说明 |
|-----------|------|
| `ASYNC_RETAIN` | 在异步回调开头使用，防止视图被提前释放 |
| `ASYNC_RELEASE` | 在异步回调开头使用（与 RETAIN 配对），视图已释放时直接 return |
| `ASYNC_TOKEN` | 传递给异步回调的参数列表 |
| `ptrLock()` / `ptrUnlock()` / `isPtrLocked()` | 指针锁机制 |

### 2.20 颜色工具 (Protected)

```cpp
NVGcolor a(NVGcolor color);     // 应用视图 alpha 到颜色
NVGcolor RGB(r, g, b);          // RGB + 自动 alpha
NVGcolor RGBA(r, g, b, a);      // RGBA + 自动 alpha
NVGcolor RGBf(r, g, b);         // 浮点 RGB + 自动 alpha
NVGcolor RGBAf(r, g, b, a);     // 浮点 RGBA + 自动 alpha
```

### 2.21 宏定义

| 宏 | 作用 |
|----|------|
| `BRLS_REGISTER_ENUM_XML_ATTRIBUTE(name, enumType, method, ...)` | 注册枚举类型的 XML 属性 |
| `BRLS_REGISTER_CLICK_BY_ID(id, method)` | 按 ID 注册 A 键点击动作 |
| `ASYNC_RETAIN` | 异步安全：创建删除令牌 |
| `ASYNC_RELEASE` | 异步安全：检查令牌，已删除则 return |
| `ASYNC_TOKEN` | 异步安全：传递给异步回调的参数列表 |

### 2.22 事件类型

| 类型 | 定义 |
|------|------|
| `GenericEvent` | `Event<View*>` |
| `VoidEvent` | `Event<>` |

用法:
```cpp
dialog->getSubmitEvent()->subscribe([]() { ... });
dialog->getCancelEvent()->subscribe([]() { ... });
```

---

## 三、brls::Box 接口

Box 继承自 View，是一个使用 FlexBox (Yoga) 布局引擎的容器。

### 3.1 构造

```cpp
Box();                      // 默认 ROW 方向
Box(Axis flexDirection);    // ROW 或 COLUMN
```

### 3.2 子视图管理

| 方法 | 签名 | 说明 |
|------|------|------|
| `addView(View*)` | 添加到末尾 | 添加子视图 |
| `addView(View*, size_t position)` | 指定位置插入 | 在指定索引处插入子视图 |
| `removeView(View*, bool free = true)` | 移除 + 释放 | 移除并可选释放子视图 |
| `clearViews(bool free = true)` | 清空 | 移除并可选释放所有子视图 |
| `getChildren()` | `vector<View*>&` | 获取所有子视图引用 |

### 3.3 内边距 (Padding)

| 方法 | 参数 | 说明 |
|------|------|------|
| `setPadding(float)` | 像素值 | 四边统一内边距 |
| `setPadding(float top, float right, float bottom, float left)` | 前后左右 | 分别设置（一次 layout pass） |
| `setPaddingTop(float)` | 像素值 | 上内边距 |
| `setPaddingRight(float)` | 像素值 | 右内边距 |
| `setPaddingBottom(float)` | 像素值 | 下内边距 |
| `setPaddingLeft(float)` | 像素值 | 左内边距 |
| `getPaddingTop/Right/Bottom/Left()` | - | 获取各方向内边距 |

### 3.4 Flex 布局属性

| 方法 | 参数 | 说明 |
|------|------|------|
| `setAxis(Axis)` | `ROW` / `COLUMN` | 主轴方向 |
| `getAxis()` | - | 获取主轴方向 |
| `setJustifyContent(JustifyContent)` | 枚举 | 主轴上的子项分布（默认 FLEX_START） |
| `setAlignItems(AlignItems)` | 枚举 | 交叉轴上的子项对齐（默认 AUTO） |
| `setDirection(Direction)` | 枚举 | 排列方向（LTR/RTL，默认 INHERIT） |

### 3.5 子视图焦点回调 (可重写)

| 方法 | 签名 | 说明 |
|------|------|------|
| `onChildFocusGained(View* directChild, View* focusedView)` | `virtual void` | 某个后代视图获焦 |
| `onChildFocusLost(View* directChild, View* focusedView)` | `virtual void` | 某个后代视图失焦 |
| `isChildFocused()` | `virtual bool` | 是否有子视图获焦 |

> `directChild` 保证是 Box 的直接子视图。`focusedView` 是实际获焦的视图（可能为 directChild 的子代）。

### 3.6 焦点记忆

| 方法 | 参数 | 说明 |
|------|------|------|
| `setLastFocusedView(View*)` | 视图指针 | 记录上次获焦的子视图（用于返回焦点） |
| `getLastFocusedView()` | 返回值 | 获取上次获焦的子视图 |
| `setDefaultFocusedIndex(int)` | 索引 | 设置默认焦点子视图的索引 |
| `getDefaultFocusedIndex()` | 返回值 | 获取默认焦点索引 |

### 3.7 XML 膨胀 (Protected)

| 方法 | 参数 | 说明 |
|------|------|------|
| `inflateFromXMLString(string_view xml)` | XML 字符串 | 从 XML 字符串创建子视图 |
| `inflateFromXMLElement(XMLElement*)` | XML 元素 | 从 XML 元素创建子视图 |
| `inflateFromXMLRes(const string& res)` | 资源名 | 从 XML 资源创建子视图 |
| `inflateFromXMLFile(const string& path)` | 文件路径 | 从 XML 文件创建子视图 |

> 根元素必须是 `<brls:Box>`，属性会应用到此 Box。

### 3.8 XML 属性转发

| 方法 | 参数 | 说明 |
|------|------|------|
| `forwardXMLAttribute(string attrName, View* target)` | 属性名 + 目标 | 将 XML 属性转发到子视图 |
| `forwardXMLAttribute(string attrName, View* target, string targetAttrName)` | 属性名 + 目标 + 重命名 | 转发并重命名 |

### 3.9 其他

| 方法 | 参数 | 说明 |
|------|------|------|
| `getParentNavigationDecision(from, newFocus, direction)` | `virtual View*` | 父级导航决策 |
| `getCullingBounds(t, r, b, l)` | `virtual void` | 获取裁剪边界 |
| `create()` | `static View*` | 创建 Box 实例（用于 XML 注册表） |

---

## 四、brls::Padding 类

继承自 `brls::View`，是一个空的占位视图。

```cpp
Padding();       // 构造：auto x auto 尺寸 + grow=1.0
                 // 效果：将同一 Box 中后续的子视图推向底部或右侧
```

---

## 五、视图生命周期总结

```
new View()                          -- 用户创建
  |
  +-- willAppear(true)              -- Activity push 时自动调用
  +-- onLayout()                    -- 布局完成后自动调用
  +-- onShowAnimationEnd()          -- show() 动画完成后
  |
  +-- [frame() -> draw()] x N       -- 每帧循环
  |
  +-- willDisappear(true)           -- Activity pop 时自动调用
  |
  +-- willAppear(false)             -- 从栈中恢复时（可选多次）
  +-- willDisappear(false)
  
delete view                         -- 框架释放
```

**焦点导航流程** (按键时):
```
1. 从当前焦点视图的父级开始，向上遍历调用 getNextFocus(direction, currentView)
2. 找到目标后，调用 getDefaultFocus() 确定具体焦点子视图
3. 给目标视图设置焦点
```