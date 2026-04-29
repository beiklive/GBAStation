# Borealis 库控件设置与布局接口总结

## 分析范围
本项目 `src/ui` 目录下全部 52 个源码文件，覆盖 `audio/`、`layout/`、`page/`、`utils/` 四个子目录。

---

## 一、视图/控件设置接口（View / Widget Settings）

### 1. 尺寸与位置

| 接口 | 参数类型 | 说明 | 典型使用文件 |
|------|---------|------|-------------|
| `setWidth(float)` | 像素值 | 设置绝对宽度 | 全目录广泛使用 |
| `setHeight(float)` | 像素值 | 设置绝对高度 | 全目录广泛使用 |
| `setWidthPercentage(float)` | 百分比 (0-100) | 按父容器百分比设置宽度 | Box.cpp, GameMenuView.cpp, GridBox.cpp |
| `setHeightPercentage(float)` | 百分比 (0-100) | 按父容器百分比设置高度 | Box.cpp, GameMenuView.cpp |
| `setGrow(float)` | flex 增长因子 | 设置 flex 布局中的权重 | SwitchLayout.cpp, Box.cpp, DynamicBackgroundBox.cpp |
| `setPositionType(brls::PositionType)` | `ABSOLUTE` / `RELATIVE` | 设置定位模式 | Box.cpp |
| `setPositionTop(float)` | 像素值 | 绝对定位上边距 | Box.cpp |
| `setPositionLeft(float)` | 像素值 | 绝对定位左边距 | Box.cpp |

### 2. 边距与内边距

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setMarginTop(float)` | 像素值 | 上边距 |
| `setMarginBottom(float)` | 像素值 | 下边距 |
| `setMarginLeft(float)` | 像素值 | 左边距 |
| `setMarginRight(float)` | 像素值 | 右边距 |
| `setMargins(float, float, float, float)` | 上/右/下/左 | 一次性设置四边边距 |
| `setPadding(float)` | 像素值 | 四边统一内边距 |
| `setPaddingTop(float)` | 像素值 | 顶部内边距 |
| `setPaddingBottom(float)` | 像素值 | 底部内边距 |
| `setPaddingLeft(float)` | 像素值 | 左侧内边距 |
| `setPaddingRight(float)` | 像素值 | 右侧内边距 |

### 3. 可见性

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setVisibility(brls::Visibility)` | `VISIBLE` / `GONE` / `INVISIBLE` | 设置控件可见状态 |

### 4. 焦点与交互样式

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setFocusable(bool)` | true/false | 是否可获取焦点 |
| `setHideHighlightBackground(bool)` | true/false | 隐藏焦点高亮背景 |
| `setHideClickAnimation(bool)` | true/false | 隐藏点击动画 |
| `setHighlightPadding(float)` | 像素值 | 焦点高亮边距 |
| `setHighlightCornerRadius(float)` | 像素值 | 焦点高亮圆角 |
| `setCornerRadius(float)` | 像素值 | 控件圆角半径 |

---

## 二、布局配置接口（Layout Configurations）

### 1. 容器类

| 类名 | 说明 | 典型使用 |
|------|------|---------|
| `brls::Box` | Flex 布局容器，核心布局控件 | 全目录继承/使用 |
| `brls::HScrollingFrame` | 水平滚动容器 | SwitchLayout.cpp, RewindSelectorView.cpp |
| `brls::ScrollingFrame` | 普通滚动容器 | GridBox.cpp |
| `brls::RecyclerFrame` | 可回收列表容器 | FileListView.cpp |
| `brls::Padding` | 空白占位控件 | SwitchLayout.cpp |

### 2. Flex 主轴与对齐

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setAxis(brls::Axis)` | `COLUMN` / `ROW` | 设置 flex 方向（纵向/横向） |
| `setAlignItems(brls::AlignItems)` | `CENTER` / `FLEX_START` / `FLEX_END` | 交叉轴对齐方式 |
| `setJustifyContent(brls::JustifyContent)` | `CENTER` / `SPACE_AROUND` / `FLEX_START` / `SPACE_BETWEEN` | 主轴内容分布 |

### 3. 视图层级管理

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `addView(brls::View*)` | 子视图指针 | 添加子控件 |
| `removeView(brls::View*, bool)` | 视图指针, 是否释放 | 移除子控件 |
| `clearViews(bool)` | 是否释放 | 清空所有子控件 |
| `setContentView(brls::View*)` | 子视图指针 | 设置滚动容器的内容视图 |
| `getChildren()` | — | 获取子视图列表 |

### 4. 滚动行为

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setScrollingBehavior(brls::ScrollingBehavior)` | `CENTERED` 等 | 滚动行为模式 |
| `setScrollingIndicatorVisible(bool)` | true/false | 是否显示滚动指示器 |

---

## 三、样式设置接口（Style Settings）

### 1. 背景与颜色

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setBackground(brls::ViewBackground)` | `NONE` | 移除默认背景 |
| `setBackgroundColor(NVGcolor)` | `nvgRGBA(...)` | 设置纯色背景 |
| `setLineColor(NVGcolor)` | `nvgRGBA(...)` / `GET_THEME_COLOR(...)` | 分隔线颜色 |
| `setLineBottom(float)` | 像素值 | 底部分隔线 |
| `setLineRight(float)` | 像素值 | 右侧分隔线 |
| `setTextColor(NVGcolor)` | `nvgRGBA(...)` / `GET_THEME_COLOR(...)` | 文本颜色（Label） |

### 2. 边框与阴影

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setBorderColor(NVGcolor)` | `nvgRGBA(...)` | 边框颜色 |
| `setBorderThickness(float)` | 像素值 | 边框粗细 |
| `setShadowVisibility(bool)` | true/false | 是否显示阴影 |
| `setShadowType(brls::ShadowType)` | `GENERIC` | 阴影样式 |

### 3. 图像属性

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setScalingType(brls::ImageScalingType)` | `FIT` / `FILL` | 图像缩放模式 |
| `setInterpolation(brls::ImageInterpolation)` | `LINEAR` / `NEAREST` | 图像插值算法 |

### 4. 文本属性（Label）

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `setFontSize(float)` | 像素值 | 字体大小 |
| `setHorizontalAlign(brls::HorizontalAlign)` | `CENTER` / `LEFT` / `RIGHT` | 水平对齐 |
| `setVerticalAlign(brls::VerticalAlign)` | `CENTER` | 垂直对齐 |
| `setSingleLine(bool)` | true/false | 单行模式 |
| `setAnimated(bool)` | true/false | 文本滚动动画 |
| `setAutoAnimate(bool)` | true/false | 自动开始滚动 |

---

## 四、事件处理接口（Event Handling）

| 接口 | 参数类型 | 说明 |
|------|---------|------|
| `registerAction(string, brls::ControllerButton, lambda, bool, bool, bool)` | 名称, 按键, 回调, 隐藏, 重复, 声音 | 注册手柄按键动作 |
| `registerClickAction(lambda)` | `lambda(brls::View*) -> bool` | 注册点击动作 |
| `addGestureRecognizer(brls::TapGestureRecognizer*)` | 手势识别器 | 添加点击手势 |
| `setCustomNavigationRoute(brls::FocusDirection, brls::View*)` | 方向, 目标视图 | 自定义焦点导航 |
| `getDefaultFocus()` | — | 获取默认焦点子视图 |
| `onFocusGained()` / `onFocusLost()` | — | 焦点状态回调（重写） |
| `onChildFocusGained()` / `onChildFocusLost()` | — | 子视图焦点回调（重写） |

---

## 五、应用级接口（Application）

| 接口 | 说明 |
|------|------|
| `brls::Application::pushActivity(brls::Activity*, brls::TransitionAnimation)` | 压入新页面 |
| `brls::Application::popActivity(...)` | 弹出当前页面 |
| `brls::Application::giveFocus(brls::View*)` | 设置焦点到指定视图 |
| `brls::Application::blockInputs(bool)` | 阻塞/恢复输入 |
| `brls::Application::notify(string)` | 显示通知 |
| `brls::sync(lambda)` | UI 线程同步执行 |
| `brls::Logger::info/debug/warning/error(...)` | 日志输出 |

---

## 六、继承关系汇总

本项目自定义控件大量继承自 borealis 基类：

| 自定义类 | 继承自 |
|---------|--------|
| `beiklive::Box` | `brls::Box` |
| `beiklive::ButtonBox` | `brls::Box` |
| `beiklive::DynamicBackgroundBox` | `brls::Box` |
| `beiklive::FileListView` | `brls::Box` |
| `beiklive::GameCard` | `brls::Box` |
| `beiklive::GameMenuView` | `beiklive::Box` → `brls::Box` |
| `beiklive::GameView` | `brls::Box` |
| `beiklive::GridBox` | `brls::Box` |
| `beiklive::GridItem` | `brls::Box` |
| `beiklive::HeaderBar` | `brls::Box` |
| `beiklive::RewindSelectorView` | `brls::Box` |
| `beiklive::RoundButton` | `brls::Box` |
| `beiklive::ListItemCell` | `brls::RecyclerCell` |
| `beiklive::FileListDataSource` | `brls::RecyclerDataSource` |
| `beiklive::MyActivity` | `brls::Activity` |
| `beiklive::BKAudioPlayer` | `brls::AudioPlayer` |
| `beiklive::Layout` / `SwitchLayout` | `brls::Box` |

---

## 七、关键宏定义

| 宏 | 展开内容 | 用途 |
|---|---------|------|
| `HIDE_BRLS_BACKGROUND(this)` | `setBackground(brls::ViewBackground::NONE)` | 隐藏默认背景 |
| `HIDE_BRLS_HIGHLIGHT(this)` | `setHideHighlightBackground(true)` | 隐藏焦点高亮 |
| `GET_STYLE("brls/...")` | 读取主题样式值 | 获取主题配置 |
| `GET_THEME_COLOR("brls/...")` | 读取主题颜色 | 获取主题颜色 |

---

## 八、常用枚举值

| 枚举类型 | 常用值 |
|---------|--------|
| `brls::Axis` | `COLUMN`, `ROW` |
| `brls::AlignItems` | `CENTER`, `FLEX_START`, `FLEX_END` |
| `brls::JustifyContent` | `CENTER`, `SPACE_AROUND`, `FLEX_START`, `SPACE_BETWEEN` |
| `brls::Visibility` | `VISIBLE`, `GONE`, `INVISIBLE` |
| `brls::PositionType` | `ABSOLUTE`, `RELATIVE` |
| `brls::ViewBackground` | `NONE` |
| `brls::ImageScalingType` | `FIT`, `FILL` |
| `brls::ImageInterpolation` | `LINEAR`, `NEAREST` |
| `brls::HorizontalAlign` | `CENTER`, `LEFT`, `RIGHT` |
| `brls::VerticalAlign` | `CENTER` |
| `brls::ShadowType` | `GENERIC` |
| `brls::ScrollingBehavior` | `CENTERED` |
| `brls::FocusDirection` | `UP`, `DOWN`, `LEFT`, `RIGHT` |
| `brls::ControllerButton` | `BUTTON_A`, `BUTTON_B`, `BUTTON_X` |
| `brls::ThemeVariant` | `DARK` |
| `brls::TransitionAnimation` | `FADE` |
