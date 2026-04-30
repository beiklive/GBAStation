# Borealis UI 开发完整接口手册

> 源目录: `third_party/borealis/library/include/borealis/`  
> 命名空间: `brls`  
> 继承根: `brls::View` → `brls::Box` → 各种控件

---

## 一、基类 brls::View

所有 UI 控件的超类，提供尺寸、布局、样式、焦点、动作、动画等通用能力。

### 1.1 静态常量

| 常量 | 值 | 用途 |
|------|-----|------|
| `View::AUTO` | `NAN` | 自动尺寸，用于 `setWidth/Height/Margins` |

### 1.2 生命周期 (Virtual)

| 方法 | 说明 | 调用时机 |
|------|------|---------|
| `frame(FrameContext*)` | 每帧逻辑更新（不用于绘制） | 先于 draw() 自动调用 |
| `draw(NVGcontext*, x, y, w, h, Style, FrameContext*) = 0` | **纯虚函数**，每帧渲染 | frame() 之后自动调用 |
| `willAppear(bool resetState=false)` | 视图即将显示 | Activity push / Tab 切换 |
| `willDisappear(bool resetState=false)` | 视图即将隐藏 | Activity pop / Tab 切换 |
| `onLayout()` | 布局完成后回调 | 每次 layout pass 结束 |
| `onShowAnimationEnd()` | 显示动画结束后 | show() 动效完成 |
| `onWindowSizeChanged()` | 窗口尺寸变化后 | resize 事件 |

### 1.3 尺寸与布局

| 方法 | 参数 | 说明 |
|------|------|------|
| `setWidth(float)` | 像素 / AUTO | 首选宽度 |
| `setHeight(float)` | 像素 / AUTO | 首选高度 |
| `setDimensions(w, h)` | 像素 | 同时设宽高（一次 layout pass） |
| `setWidthPercentage(float)` | 0-100 | 按父容器宽度百分比 |
| `setHeightPercentage(float)` | 0-100 | 按父容器高度百分比 |
| `setMinWidth/MaxWidth(float)` | 像素 | 最小/最大宽度约束 |
| `setMinHeight/MaxHeight(float)` | 像素 | 最小/最大高度约束 |
| `setMinWidthPercentage/MaxWidthPercentage(float)` | 0-100 | 百分比约束 |
| `setGrow(float)` | 增长因子 | 剩余空间分配比例，默认 0 |
| `setShrink(float)` | 收缩因子 | 空间不足时收缩比例，默认 1 |

### 1.4 边距 Margin

| 方法 | 说明 |
|------|------|
| `setMargins(t, r, b, l)` | 一次性设四边（一次 layout pass） |
| `setMarginTop/Right/Bottom/Left(float)` | 单边设置 |
| `getMarginRight/Left()` | 获取值 |

### 1.5 定位 Position

| 方法 | 说明 |
|------|------|
| `setPositionType(PositionType)` | `RELATIVE`(默认) / `ABSOLUTE` |
| `setPositionTop/Right/Bottom/Left(float)` | 像素定位 |
| `setPositionTopPercentage/LeftPercentage...(float)` | 百分比定位 |
| `setTranslationX/Y(float)` | 平移（layout 之后应用） |

### 1.6 可见性 Visibility

| 方法 | 说明 |
|------|------|
| `setVisibility(Visibility)` | `VISIBLE` / `INVISIBLE`(占位) / `GONE`(不占位) |
| `getVisibility()` | 获取当前值 |
| `show(cb)` / `show(cb, animate, duration)` | 淡入显示 |
| `hide(cb)` / `hide(cb, animate, duration)` | 淡出隐藏 |
| `collapse(bool animated=true)` | 折叠收起 |
| `expand(bool animated=true)` | 展开恢复 |
| `isHidden()` / `isCollapsed()` | 状态查询 |
| `isTranslucent()` | 是否透明（影响淡入动画） |
| `setAlpha(float)` | 透明度 0-1 |

### 1.7 样式 Style

| 方法 | 参数 | 说明 |
|------|------|------|
| `setBackground(ViewBackground)` | `NONE/SIDEBAR/BACKDROP/SHAPE_COLOR/VERTICAL_LINEAR` | 背景类型 |
| `setBackgroundColor(NVGcolor)` | `nvgRGBA(r,g,b,a)` | 纯色背景 |
| `setBackgroundCornerRadii(tl, tr, br, bl)` | 四个圆角 | 渐变背景圆角 |
| `setBorderColor(NVGcolor)` | `nvgRGBA(...)` | 边框色 |
| `setBorderThickness(float)` | 像素 | 边框粗细 |
| `setCornerRadius(float)` | 像素 | 圆角半径，0=直角 |
| `setShadowType(ShadowType)` | `NONE/GENERIC/CUSTOM` | 阴影类型 |
| `setShadowVisibility(bool)` | | 阴影可见性 |

### 1.8 分隔线 Line

| 方法 | 说明 |
|------|------|
| `setLineColor(NVGcolor)` | 四边线颜色 |
| `setLineTop/Right/Bottom/Left(float)` | 各边线粗细（常用于底部 1px 分隔线） |

### 1.9 焦点高亮 Highlight

| 方法 | 说明 |
|------|------|
| `setHideHighlightBackground(bool)` | 隐藏焦点白色背景 |
| `setHideHighlightBorder(bool)` | 隐藏焦点边框 |
| `setHideHighlight(bool)` | 隐藏全部高亮 |
| `setHideClickAnimation(bool)` | 隐藏点击动画 |
| `setHighlightPadding(float)` | 高亮矩形边距 |
| `setHighlightCornerRadius(float)` | 高亮圆角 |

### 1.10 焦点与导航 Focus

| 方法 | 说明 |
|------|------|
| `setFocusable(bool)` | 是否可获焦 |
| `isFocusable()` / `isFocused()` | 状态查询 |
| `getDefaultFocus()` → `virtual View*` | 默认焦点，nullptr=不可获焦 |
| `getNextFocus(direction, currentView)` → `virtual View*` | 方向导航 |
| `hitTest(Point)` → `virtual View*` | 坐标命中测试 |
| `onFocusGained()` / `onFocusLost()` | 焦点回调 |
| `onParentFocusGained(View*)` / `onParentFocusLost(View*)` | 父级焦点回调 |
| `setFocusSound(Sound)` | 焦点音效 |
| `setCustomNavigationRoute(direction, View*)` | 自定义导航路由(指针) |
| `setCustomNavigationRoute(direction, string)` | 自定义导航路由(ID) |
| `hasCustomNavigationRouteByPtr/ById(direction)` | 查询 |
| `getCustomNavigationRoutePtr/Id(direction)` | 获取 |

### 1.11 按键注册 Actions

| 方法 | 说明 |
|------|------|
| `registerAction(hint, ControllerButton, ActionListener, hidden, repeating, sound)` | 注册手柄按键 |
| `registerAction(BrlsKeyCombination, ActionListener, repeating)` | 注册键盘组合键 |
| `registerClickAction(ActionListener)` | 快捷方式：A键 + SOUND_CLICK |
| `unregisterAction(ActionIdentifier)` | 注销 |
| `updateActionHint(button, hintText)` | 更新底部提示文字 |
| `setActionAvailable(button, bool)` | 设置按键可用性 |
| `setActionsAvailable(bool)` | 批量设置 |

> **ActionListener**: `std::function<bool(View*)>` — 返回 true 表示已消费

**常用 ControllerButton**:
```
BUTTON_A, BUTTON_B, BUTTON_X, BUTTON_Y
BUTTON_UP, BUTTON_DOWN, BUTTON_LEFT, BUTTON_RIGHT
BUTTON_LB, BUTTON_RB, BUTTON_LT, BUTTON_RT
BUTTON_START, BUTTON_BACK
BUTTON_LSB, BUTTON_RSB
```

### 1.12 点击动画

| 方法 | 说明 |
|------|------|
| `resetClickAnimation()` | 重置 |
| `playClickAnimation(reverse, animateBack, force)` | 播放 |

### 1.13 手势 Gesture

| 方法 | 说明 |
|------|------|
| `addGestureRecognizer(GestureRecognizer*)` | 添加手势 |
| `interruptGestures(bool onlyIfUnsure)` | 中断手势 |
| `getGestureRecognizers()` | 获取列表 |

### 1.14 视图树查询

| 方法 | 说明 |
|------|------|
| `getView(string id)` → `virtual View*` | 向下递归查找 |
| `getNearestView(string id)` → `virtual View*` | 向上查找 |
| `getParent()` → `Box*` | 父容器 |
| `hasParent()` → `bool` | 是否有父容器 |
| `setParent(Box*, void* userdata)` | 设置父容器 |
| `removeFromSuperView(bool free=true)` | 从父容器移除 |

### 1.15 空间信息

| 方法 | 返回值 |
|------|--------|
| `getFrame()` | `Rect`(全局坐标) |
| `getX()` / `getY()` | `float`(全局) |
| `getLocalFrame()` | `Rect`(本地) |
| `getLocalX()` / `getLocalY()` | `float`(本地) |
| `getWidth()` | `float` |
| `getHeight(bool includeCollapse=true)` | `float` |

### 1.16 其他

| 方法 | 说明 |
|------|------|
| `setId(string)` | 设置视图 ID |
| `setAlignSelf(AlignSelf)` | 覆盖父 Box 的交叉轴对齐 |
| `setAspectRatio(float)` | 宽高比约束 |
| `invalidate()` | 触发视图树重布局 |
| `setWireframeEnabled(bool)` | 调试线框模式 |
| `setCulled(bool)` | 是否可被裁剪 |
| `detach()` | 从 Yoga 节点分离 |
| `setDetachedPosition(x, y)` | 分离后手动设位置 |
| `setInFadeAnimation(bool)` | 强制半透明(淡入用) |
| `freeView()` | 安全释放 |
| `present(View*)` | 当前视图上呈现新视图 |
| `dismiss(cb)` | 关闭 present 的视图 |
| `getAppletFrame()` → `virtual AppletFrame*` | 获取 AppletFrame |
| `setClipsToBounds(bool)` | 裁剪超出边界内容 |

### 1.17 异步安全

| 宏 | 说明 |
|----|------|
| `ASYNC_RETAIN` | 异步回调开头，防止视图提前释放 |
| `ASYNC_RELEASE` | 与 RETAIN 配对，已释放则 return |
| `ASYNC_TOKEN` | 传递给异步回调的参数列表 |

---

## 二、brls::Box — FlexBox 容器

继承 `View`，Yoga 引擎驱动的 FlexBox 布局容器。

### 2.1 构造

```cpp
Box();                     // 默认 ROW 方向
Box(Axis flexDirection);   // ROW 或 COLUMN
```

### 2.2 子视图管理

| 方法 | 说明 |
|------|------|
| `addView(View*)` | 添加到末尾 |
| `addView(View*, size_t position)` | 指定位置插入 |
| `removeView(View*, bool free=true)` | 移除(可选释放) |
| `clearViews(bool free=true)` | 移除所有 |
| `getChildren()` → `vector<View*>&` | 获取所有子视图 |

### 2.3 内边距 Padding

| 方法 | 说明 |
|------|------|
| `setPadding(float)` | 四边统一 |
| `setPadding(t, r, b, l)` | 分别设置(一次 pass) |
| `setPaddingTop/Right/Bottom/Left(float)` | 单边 |
| `getPaddingTop()/Right()/Bottom()/Left()` | 获取 |

### 2.4 Flex 布局

| 方法 | 枚举值 | 说明 |
|------|--------|------|
| `setAxis(Axis)` | `ROW`/`COLUMN` | 主轴方向 |
| `setJustifyContent(JustifyContent)` | `FLEX_START`/`CENTER`/`FLEX_END`/`SPACE_BETWEEN`/`SPACE_AROUND`/`SPACE_EVENLY` | 主轴分布(默认 FLEX_START) |
| `setAlignItems(AlignItems)` | `AUTO`/`FLEX_START`/`CENTER`/`FLEX_END`/`STRETCH`/`BASELINE` | 交叉轴对齐(默认 AUTO) |
| `setDirection(Direction)` | `INHERIT`/`LEFT_TO_RIGHT`/`RIGHT_TO_LEFT` | 排列方向 |

### 2.5 子视图焦点回调

| 方法 | 说明 |
|------|------|
| `onChildFocusGained(View* directChild, View* focusedView)` | 后代视图获焦 |
| `onChildFocusLost(View* directChild, View* focusedView)` | 后代视图失焦 |
| `isChildFocused()` → `bool` | 是否有子视图获焦 |

### 2.6 焦点记忆

| 方法 | 说明 |
|------|------|
| `setLastFocusedView(View*)` | 记录上次焦点(返回用) |
| `getLastFocusedView()` | 获取 |
| `setDefaultFocusedIndex(int)` | 默认焦点索引 |
| `getDefaultFocusedIndex()` | 获取 |

### 2.7 XML 膨胀 (Protected)

| 方法 | 说明 |
|------|------|
| `inflateFromXMLString(string_view)` | 从 XML 字符串创建 |
| `inflateFromXMLRes(string)` | 从 XML 资源创建 |
| `inflateFromXMLFile(string)` | 从 XML 文件创建 |
| `inflateFromXMLElement(XMLElement*)` | 从 XML 元素创建 |

### 2.8 brls::Padding

```cpp
Padding();  // 空占位视图，auto x auto + grow=1.0
            // 将后续子视图推向底部/右侧
```

---

## 三、核心工具类

### 3.1 brls::Application (静态类)

| 方法 | 说明 |
|------|------|
| `pushActivity(Activity*, animation=FADE)` | 推入新页面(Activity) |
| `popActivity(animation=FADE, cb, free)` | 弹出当前页面 |
| `getActivitiesStack()` | 获取 Activity 栈 |
| `giveFocus(View*)` | 设置焦点 |
| `getCurrentFocus()` | 获取当前焦点 |
| `getStyle()` → `Style&` | 获取样式表 |
| `getTheme()` → `Theme&` | 获取主题色 |
| `getThemeVariant()` → `ThemeVariant` | LIGHT/DARK |
| `getImeManager()` → `ImeManager*` | 输入法管理器 |
| `notify(string)` | 显示通知(顶部 toast) |
| `blockInputs(bool)` | 阻塞/恢复输入 |
| `getControllerState()` → `const ControllerState&` | 手柄状态 |
| `getPlatform()` → `Platform*` | 平台接口 |
| `getAudioPlayer()` | 音频播放器 |
| `getNVGContext()` | NanoVG 上下文 |
| `setFPSStatus(bool)` | 显示 FPS 叠加层 |
| `getFPS()` | 获取帧率 |
| `enableDebuggingView(bool)` | 调试叠加层 |
| `setSwapInputKeys(bool)` | 交换 A/B 确认键 |

**静态事件 (均可 subscribe):**
| 事件 | 说明 |
|------|------|
| `getGlobalFocusChangeEvent()` | 全局焦点变化 |
| `getGlobalHintsUpdateEvent()` | 底部提示更新 |
| `getRunLoopEvent()` | 每帧事件 |
| `getExitEvent()` | 退出前 |
| `getWindowSizeChangedEvent()` | 窗口 resize |

### 3.2 brls::Activity

页面容器，封装一个 View 并管理其生命周期。

```cpp
Activity(View* contentView);
Activity();  // 需重写 createContentView()

void setContentView(View*);
View* getContentView();
View* getView(string id);

void willAppear(bool resetState=false);
void willDisappear(bool resetState=false);
void onPause();
void onResume();

void registerAction(hint, button, listener, ...);
void registerExitAction(ControllerButton = BUTTON_START);  // START 返回
```

### 3.3 brls::Event<Ts...> — 观察者模式

```cpp
Event<string> myEvent;

// 订阅
auto sub = myEvent.subscribe([](string val) { ... });

// 取消订阅
myEvent.unsubscribe(sub);

// 触发
myEvent.fire("hello");  // 返回 true 若有订阅者
```

### 3.4 brls::Animatable — 动画值

```cpp
Animatable(float initialValue = 0.0f);

float getValue();
operator float();              // 隐式转换

void addStep(float target, int32_t durationMs, EasingFunction easing);
void reset(float initialValue);
void reset();                   // 保持当前值不动
```

常用缓动: `tweeny::easing::linear`, `tweeny::easing::backOut`, 等。

### 3.5 线程安全

```cpp
// 从任意线程安全提交到 UI 线程执行
brls::sync([this]() { ... });

// 异步执行
brls::async([this]() { ... });

// 延迟执行（返回 index 可用于取消）
size_t id = brls::delay(1000, [this]() { ... });
brls::cancelDelay(id);
```

### 3.6 日志

```cpp
brls::Logger::debug("fmt {}", args...);
brls::Logger::info("fmt {}", args...);
brls::Logger::warning("fmt {}", args...);
brls::Logger::error("fmt {}", args...);
brls::Logger::setLogLevel(LogLevel::LOG_DEBUG);
```

### 3.7 定时器

```cpp
// 单次定时器
Timer t;
t.start(durationMs);  // 到期后自动停止

// 重复定时器
RepeatingTimer rt;
rt.start(periodMs);
rt.setCallback([]() { ... });
rt.stop();
```

### 3.8 几何类型

```cpp
struct Point { float x, y; };
struct Size  { float w, h; };
struct Rect  { Point origin; Size size; };

// Rect 常用方法
rect.pointInside(Point);
rect.collideWith(other);
rect.getMidX() / getMidY();
rect.getMaxX() / getMaxY();
```

---

## 四、视图控件 Views

### 4.1 brls::Image

```cpp
Image();

// 加载图片
setImageFromFile(path);      // PNG/JPG/BMP/TGA/GIF
setImageFromRes(name);       // 嵌入资源
setImageFromMem(data, size);
setImageAsync(callback);     // 异步加载
clear();                     // 卸载

// 缩放
setScalingType(ImageScalingType);  // FIT(默认)/FILL/STRETCH/CENTER
setImageAlign(ImageAlignment);     // CENTER/TOP/LEFT/...
setInterpolation(ImageInterpolation); // LINEAR(默认)/NEAREST

// 查询
getTexture();
getOriginalImageWidth() / getOriginalImageHeight();
```

### 4.2 brls::Label

```cpp
Label();

setText(string);
setFontSize(float);
setLineHeight(float);
setTextColor(NVGcolor);
setHorizontalAlign(HorizontalAlign);  // LEFT/CENTER/RIGHT
setVerticalAlign(VerticalAlign);      // BASELINE/TOP/CENTER/BOTTOM
setSingleLine(bool);
setAnimated(bool);          // 滚动动画
setAutoAnimate(bool);       // 获焦时自动滚动
setCursor(int);             // 光标位置
getFullText();
setIsWrapping(bool);        // 长文本是否换行
```

### 4.3 brls::Header — 段落标题

```cpp
Header();
setTitle(string);
setSubtitle(string);  // 灰色小字副标题
```

### 4.4 brls::Rectangle — 纯色矩形

```cpp
Rectangle(NVGcolor color);
Rectangle();
setColor(NVGcolor);
```

### 4.5 brls::Button

```cpp
Button();
setText(string);
setFontSize(float);
setTextColor(NVGcolor);
setStyle(&ButtonStyle);     // BUTTONSTYLE_PRIMARY/HIGHLIGHT/DEFAULT/BORDERED/BORDERLESS
setState(ButtonState);      // ENABLED/DISABLED
```

### 4.6 brls::Slider — 滑动条

```cpp
Slider();
setProgress(float);         // 0.0 - 1.0
getProgress();
setStep(float);             // 步进值
getProgressEvent() → Event<float>*  // 值变化事件
```

### 4.7 brls::ProgressSpinner — 加载动画

```cpp
ProgressSpinner(ProgressSpinnerSize size = NORMAL);  // 或 LARGE
animate(bool animate);  // 开始/停止旋转
```

---

## 五、容器与导航

### 5.1 brls::AppletFrame — 带页头页脚的框架

```cpp
AppletFrame();
AppletFrame(View* contentView);

setTitle(string);
setIcon(path);
setHeaderVisibility(Visibility);
setFooterVisibility(Visibility);
pushContentView(View*);       // 入栈新内容
popContentView(callback);     // 出栈返回上一层
getContentView();
getHeader();                  // Box*
getFooter();                  // Box*
```

### 5.2 brls::ScrollingFrame — 纵向滚动容器

```cpp
ScrollingFrame();

setContentView(View*);
setScrollingBehavior(ScrollingBehavior);  // NATURAL/CENTERED
setScrollingIndicatorVisible(bool);
getContentOffsetY();
setContentOffsetY(float, bool animated);
```

### 5.3 brls::HScrollingFrame — 横向滚动容器

接口与 `ScrollingFrame` 相同，方向为横向。

### 5.4 brls::TabFrame — 底部标签页

```cpp
TabFrame();
addTab(string label, function<View*()> creator);  // 添加标签页
focusTab(int position);      // 切换到指定标签
clearTabs();
addSeparator();
```

### 5.5 brls::Dialog — 模态对话框

```cpp
Dialog(string text);
Dialog(Box* contentView);    // 自定义内容

addButton(string label, callback);  // 最多 3 个按钮
setCancelable(bool);         // B 键关闭(默认 true)
open();                      // 显示
close(callback);             // 关闭
```

### 5.6 brls::Dropdown — 下拉菜单

```cpp
Dropdown(string title, vector<string> values, callback, int selected=0, dismissCb);
show();
hide();
isTranslucent() → true;
```

### 5.7 brls::EditTextDialog — 文本输入对话框

```cpp
EditTextDialog();

setText(string);             // 初始文本
setHeaderText(string);       // 标题
setHintText(string);         // 占位提示
open();

// 事件订阅
getSubmitEvent() → Event<>*     // 确认时
getCancelEvent() → Event<>*     // 取消时
getBackspaceEvent() → Event<>*  // 退格
getClipboardEvent() → Event<string>*  // 粘贴
```

### 5.8 brls::Sidebar — 侧边栏

```cpp
Sidebar();
addItem(string label, GenericEvent::Callback focusCallback);
getItem(int position) → SidebarItem*;
addSeparator();
clearItems();
```

---

## 六、设置单元格 Cells

### 6.1 brls::DetailCell — 标题+详情行

```cpp
DetailCell();
setText(string);
setDetailText(string);
setTextColor(NVGcolor);
setDetailTextColor(NVGcolor);
```

### 6.2 brls::BooleanCell — 开关

```cpp
BooleanCell();
init(string title, bool isOn, function<void(bool)> callback);
setOn(bool on, bool animated=true);
isOn();
getEvent() → Event<bool>*;
```

### 6.3 brls::SelectorCell — 选择器

```cpp
SelectorCell();
init(title, vector<string> data, int selected, callback, dismissCb=nullptr);
setSelection(int selection, bool silent=false);
getSelection();
getEvent() → Event<int>*;
```

### 6.4 brls::SliderCell — 滑动条单元格

```cpp
SliderCell();
init(string title, float initialValue, function<void(float)> callback);
getEvent() → Event<float>*;
```

### 6.5 brls::InputCell — 文本输入单元格

```cpp
InputCell();
init(title, value, callback, placeholder, hint, maxLength, kbdDisableBitmask);
setValue(string);
getValue() → string;
getEvent() → Event<string>*;
```

### 6.6 brls::InputNumericCell — 数字输入单元格

```cpp
InputNumericCell();
init(title, long value, callback, hint, maxLength, kbdDisableBitmask);
setValue(long);
getValue() → long;
getEvent() → Event<long>*;
```

### 6.7 brls::RadioCell — 单选按钮

```cpp
RadioCell();
setSelected(bool);
getSelected();
```

---

## 七、列表 RecyclerFrame

用于高性能可复用列表(类似 UITableView/RecyclerView)。

### 7.1 brls::RecyclerCell

```cpp
RecyclerCell();
getIndexPath() → IndexPath;
prepareForReuse();  // 复用时重置
string reuseIdentifier;
```

### 7.2 brls::RecyclerDataSource (纯虚接口)

```cpp
virtual int numberOfSections(RecyclerFrame*);
virtual int numberOfRows(RecyclerFrame*, int section);
virtual RecyclerCell* cellForRow(RecyclerFrame*, IndexPath);
virtual float heightForRow(RecyclerFrame*, IndexPath);  // -1=auto
virtual string titleForHeader(RecyclerFrame*, int section);
virtual void didSelectRowAt(RecyclerFrame*, IndexPath);
```

### 7.3 brls::RecyclerFrame

```cpp
RecyclerFrame();

registerCell(string reuseIdentifier, function<RecyclerCell*()> allocator);
setDataSource(RecyclerDataSource*, bool deleteDataSource=true);
reloadData();
dequeueReusableCell(string identifier) → RecyclerCell*;
selectRowAt(IndexPath, bool animated);
float estimatedRowHeight = 44;
```

---

## 八、输入抽象

### 8.1 brls::ImeManager

```cpp
// 打开文本输入键盘
openForText(
    function<void(string)> callback,
    string headerText = "",
    string subText = "",
    int maxLength = 32,
    string initialText = "",
    int kbdDisableBitmask = KEYBOARD_DISABLE_NONE
) → bool;

// 打开数字输入键盘(仅整数)
openForNumber(
    function<void(long)> callback,
    string headerText = "",
    string subText = "",
    int maxLength = 18,
    string initialText = "",
    string leftButton = "",
    string rightButton = "",
    int kbdDisableBitmask = KEYBOARD_DISABLE_NONE
) → bool;
```

### 8.2 brls::ControllerState

```cpp
struct ControllerState {
    bool buttons[_BUTTON_MAX];     // 按键状态
    float axes[_AXES_MAX];         // 摇杆值(0-1)
};

// 用法
auto state = brls::Application::getControllerState();
int idx = static_cast<int>(brls::BUTTON_LB);
if (state.buttons[idx]) { ... }
```

---

## 九、手势识别器

### 9.1 brls::TapGestureRecognizer

```cpp
// 连接视图的主 Action (A 键)
TapGestureRecognizer(View* targetView);

// 自定义响应
TapGestureRecognizer(View* targetView, function<void()> respond);

// 完整控制
TapGestureRecognizer(TapGestureEvent::Callback respond);
```

### 9.2 brls::PanGestureRecognizer

```cpp
PanGestureRecognizer(PanGestureEvent::Callback respond, PanAxis axis);
// PanAxis: HORIZONTAL / VERTICAL / ANY
```

---

## 十、枚举速查表

### 10.1 Visibility
`VISIBLE` | `INVISIBLE` | `GONE`

### 10.2 PositionType
`RELATIVE` | `ABSOLUTE`

### 10.3 ShadowType
`NONE` | `GENERIC` | `CUSTOM`

### 10.4 TransitionAnimation
`FADE` | `SLIDE_LEFT` | `SLIDE_RIGHT` | `NONE` | `LINEAR`

### 10.5 Axis
`ROW` | `COLUMN`

### 10.6 JustifyContent
`FLEX_START` | `CENTER` | `FLEX_END` | `SPACE_BETWEEN` | `SPACE_AROUND` | `SPACE_EVENLY`

### 10.7 AlignItems
`AUTO` | `FLEX_START` | `CENTER` | `FLEX_END` | `STRETCH` | `BASELINE` | `SPACE_BETWEEN` | `SPACE_AROUND`

### 10.8 ImageScalingType
`FIT` | `FILL` | `STRETCH` | `CENTER`

### 10.9 ImageInterpolation
`LINEAR` | `NEAREST`

### 10.10 HorizontalAlign
`LEFT` | `CENTER` | `RIGHT`

### 10.11 VerticalAlign
`BASELINE` | `TOP` | `CENTER` | `BOTTOM`

### 10.12 FocusDirection
`UP` | `DOWN` | `LEFT` | `RIGHT`

### 10.13 ScrollingBehavior
`NATURAL` | `CENTERED`

### 10.14 Sound
`SOUND_NONE` | `SOUND_CLICK` | `SOUND_FOCUS_CHANGE` | `SOUND_BACK` | `SOUND_FOCUS_ERROR` | `SOUND_SLIDER_TICK`

### 10.15 ButtonState
`ENABLED` | `DISABLED`

### 10.16 ThemeVariant
`LIGHT` | `DARK`

---

## 十一、常用模式速查

### 创建页面

```cpp
class MyPage : public beiklive::Box {
public:
    MyPage() {
        brls::sync([this]() {
            this->showHeader(false);
            this->showFooter(false);
            this->showBackground(false);
            buildUI();
        });
    }
private:
    void buildUI() {
        auto* content = this->getContentBox();
        content->setAxis(brls::Axis::COLUMN);
        // ...
    }
};
```

### 推送/弹出页面

```cpp
brls::Application::pushActivity(new brls::Activity(view));
brls::Application::popActivity();
```

### 主题色/样式

```cpp
GET_THEME_COLOR("brls/text")           // 主文本色
GET_THEME_COLOR("brls/backdrop")       // 背景色
GET_STYLE("brls/applet_frame/padding_sides")  // 边距值
```

### 资源路径

```cpp
BK_RES("img/icon.png")  // 项目自定义宏，解析到实际路径
```

### 高亮快捷宏

```cpp
HIDE_BRLS_HIGHLIGHT(view)   // 等价于 setHideHighlightBackground+setHideHighlightBorder
HIDE_BRLS_BACKGROUND(view)  // setBackground(ViewBackground::NONE)
```