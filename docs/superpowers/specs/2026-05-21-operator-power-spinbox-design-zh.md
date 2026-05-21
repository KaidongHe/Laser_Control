# 普通操作员功率 SpinBox 自定义样式设计

## 背景

普通操作员页面 `operatorForm` 中的 `powerSpinBox` 当前使用标准 Qt Widgets `QSpinBox` 样式，并通过 `.ui` 文件里的 stylesheet 做基础外观控制。用户希望参考一个网上找到的 Qt Quick/QML 示例，将 SpinBox 做成更现代的圆角胶囊样式，包含左右 `- / +` 区域、居中数值、焦点边框和阴影。

当前项目是 Qt Widgets 工程，不是 Qt Quick/QML 工程。因此不能直接粘贴 QML 的 `SpinBox { background / up.indicator / down.indicator / contentItem }` 写法。实现应在 Qt Widgets 体系内复刻其视觉和交互。

## 目标

- 只修改普通操作员页面的 `powerSpinBox` 外观和交互。
- 开发者界面的三个激光电流 SpinBox 保持现状，不参与本次改动。
- 视觉上接近用户提供的 QML 示例：
  - 胶囊形圆角背景。
  - 左侧为 `-` 区域，右侧为 `+` 区域。
  - 中间数字居中。
  - 普通态白色背景、灰色边框。
  - focus 时蓝色边框。
  - 按下 `- / +` 区域时有浅灰反馈。
  - 禁用态为浅灰背景。
  - 控件带轻微阴影。
- 保留 `QSpinBox` 原有数值能力：
  - 范围仍为 `2-100`。
  - 后缀仍为 `%`。
  - 仍可键盘输入。
  - 仍受 Qt 的 validator 和范围限制保护。

## 非目标

- 本次不修改开发者窗口 `Widget` 中的 `laser1spinbox`、`laser2spinbox`、`laser3spinbox`。
- 本次不改变 `powerSpinBox` 的业务含义。
- 本次不新增串口命令或硬件控制逻辑。
- 本次不把项目迁移到 Qt Quick。
- 本次不引入 `QtQuickWidgets` 或 QML 运行时依赖。

## 方案

新增一个 Qt Widgets 自定义控件类，暂定名为 `PillSpinBox`，继承自 `QSpinBox`。

`PillSpinBox` 的职责是封装普通操作员页面需要的胶囊式数值输入样式。它只处理控件外观和局部鼠标交互，不承载激光控制业务逻辑。

实现要点：

- 在构造函数中关闭默认按钮外观，例如使用 `setButtonSymbols(QAbstractSpinBox::NoButtons)`。
- 设置合适的固定或最小高度，让控件在普通操作员页面中保持大号触控友好尺寸。
- 通过 stylesheet 或 `paintEvent()` 绘制白色圆角主体、边框、左右点击区域和 `- / +` 文本。
- 重写 `mousePressEvent()`、`mouseReleaseEvent()`：
  - 点击左侧区域时调用 `stepDown()`。
  - 点击右侧区域时调用 `stepUp()`。
  - 中间区域继续交给 `QSpinBox` 处理，以保留文本编辑能力。
- 根据 focus、enabled、pressedSide 状态刷新绘制。
- 在 `operatorform.cpp` 中给 `powerSpinBox` 加 `QGraphicsDropShadowEffect`，模拟 QML 示例中的轻微阴影。

## UI 集成

`operatorform.ui` 中保留对象名 `powerSpinBox`，但将其从普通 `QSpinBox` 提升为 `PillSpinBox`。

Qt Designer 集成方式：

- 在 `.ui` 的 custom widget 区域注册：

```xml
<customwidget>
 <class>PillSpinBox</class>
 <extends>QSpinBox</extends>
 <header>pillspinbox.h</header>
</customwidget>
```

- `powerSpinBox` 仍使用 `name="powerSpinBox"`。
- 现有范围、后缀和值继续保留在 `.ui` 中，或者在 `operatorform.cpp` 初始化时设置。

`serialhelper.pro` 需要加入：

```pro
SOURCES += \
        pillspinbox.cpp

HEADERS += \
        pillspinbox.h
```

## 行为保持

- 程序启动仍显示普通操作员页面。
- `powerSpinBox` 默认值仍为 `2%`。
- `powerSpinBox` 最小值仍为 `2%`，最大值仍为 `100%`。
- 点击左侧 `-` 区域时减少一步。
- 点击右侧 `+` 区域时增加一步。
- 点击中间数字区域时仍可编辑数值。
- 键盘输入、Tab focus、上下键调整等 `QSpinBox` 基本能力尽量保留。
- 两个开关按钮和开发者入口逻辑不变。

## 样式细节

推荐的初始视觉参数：

- 控件高度：约 `68 px`，与当前普通页面的大号输入框接近。
- 左右按钮区宽度：约 `52-60 px`。
- 圆角：高度的一半，形成胶囊。
- 普通边框：`#b7c6e8` 或 `#bdbdbd`。
- focus 边框：`#2196F3`，宽度可略加粗。
- 主体背景：enabled 时 `#ffffff`，disabled 时 `#f0f0f0`。
- 按下区域背景：`#e0e0e0`。
- 数字文本：`#212121`。
- `- / +` 文本：普通态 `#757575`，按下时 `#1976D2`。
- 阴影：透明黑色、轻量模糊，避免在浅色面板上显得过重。

## 风险与约束

- Qt Widgets 的 stylesheet 不支持 QML 那种 `up.indicator` / `down.indicator` 结构，因此需要自定义控件来保持接近效果。
- 如果完全重写 `paintEvent()`，需要避免破坏文本编辑时的光标、选区和输入行为。
- 因为只用于普通操作员页面，控件实现应保持小而独立，避免影响开发者窗口。
- 如果 Qt Designer 对 promoted widget 显示不完整，运行时仍应正常；必要时可在 `.ui` 保持普通 `QSpinBox`，在 C++ 中动态替换为 `PillSpinBox`，但优先使用 promoted widget。

## 测试

需要验证：

- `qmake serialhelper.pro` 成功。
- Build 通过。
- 程序启动后普通操作员页面能正常显示。
- `powerSpinBox` 显示为胶囊式样式。
- `powerSpinBox` 默认显示 `2 %` 或等价百分比显示。
- 点击左侧 `-` 不会低于 `2%`。
- 点击右侧 `+` 不会高于 `100%`。
- 中间区域可编辑输入。
- focus 时边框变化正常。
- 禁用态如未来出现时视觉清楚。
- 开发者窗口三个激光电流 SpinBox 没有样式变化。
