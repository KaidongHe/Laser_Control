# OperatorWindow 使用 .ui 文件实现设计

## 背景

当前普通用户主界面 `OperatorWindow` 的布局和样式主要写在 `operatorwindow.cpp` 的 `setupUi()` 中。这样能运行，但后续如果要调整按钮位置、间距、字体、控件尺寸或整体视觉风格，需要改 C++ 代码，不利于用 Qt Designer 可视化维护。

新的要求是：普通用户主界面页面本身用 `.ui` 文件实现，不再用 C++ 手写布局。

## 目标

- 新增 `operatorwindow.ui`，让普通用户主界面的布局、控件层级、主要样式由 Qt Designer `.ui` 文件承载。
- `operatorwindow.cpp` 只保留窗口逻辑：
  - 初始化 UI
  - 两个开关按钮的状态切换
  - 开发者密码验证
  - 打开/关闭开发者界面的窗口流转
- 保持现有普通用户界面功能不变。
- 保持现有开发者界面 `Widget` 不变。

## 非目标

- 本次不改变三个普通控件的硬件协议映射。
- 本次不改 STM32 固件。
- 本次不重新设计旧开发者界面。
- 本次不改变密码值。

## 架构方案

新增 Qt Designer 文件：

```text
laser-control/serialhelper/operatorwindow.ui
```

`OperatorWindow` 改为使用 Qt 自动生成的 `Ui::OperatorWindow`：

- `operatorwindow.h` 增加 `namespace Ui { class OperatorWindow; }`
- `operatorwindow.h` 持有 `Ui::OperatorWindow *ui`
- `operatorwindow.cpp` include `ui_operatorwindow.h`
- 构造函数中调用 `ui->setupUi(this)`
- 析构函数中释放 `ui`

原来 `operatorwindow.cpp::setupUi()` 里创建控件、布局和样式的代码移动到 `.ui` 文件中。C++ 代码不再 new `QFrame`、`QLabel`、`QPushButton`、`QSpinBox` 来搭建页面。

## UI 控件命名

`.ui` 文件中必须保留这些对象名，供 C++ 逻辑直接访问：

- `seedButton`
- `preReleaseButton`
- `powerSpinBox`
- `developerButton`

其他视觉控件建议命名为：

- `panel`
- `titleLabel`
- `subtitleLabel`
- `powerLabel`

## 行为保持

迁移后行为必须与当前版本一致：

- 程序启动显示普通用户主界面。
- `L1 开/关（种子）` 按钮点击后只切换 UI 状态，不发送串口命令。
- `预放开/关` 按钮点击后只切换 UI 状态，不发送串口命令。
- `powerSpinBox` 范围为 `2-100`，后缀为 `%`。
- 点击 `开发者` 弹出密码框。
- 密码为 `laser2026`。
- 密码错误或取消时仍停留在普通用户主界面。
- 密码正确时隐藏普通用户主界面并打开现有开发者界面 `Widget`。
- 关闭开发者界面后重新显示普通用户主界面。

## 样式要求

`.ui` 文件应延续当前“清爽工业风”：

- 页面浅灰蓝背景。
- 中央白色面板。
- 大号蓝色主按钮。
- 按钮开启状态为绿色。
- 小号低强调 `开发者` 按钮放在面板右下角。
- 中文字体优先使用微软雅黑。

样式可以继续通过窗口或控件的 `styleSheet` 属性写在 `.ui` 中。C++ 中只允许保留与动态状态相关的少量样式刷新逻辑，例如按钮 `active` 属性变化后重新 polish。

## 项目文件

`serialhelper.pro` 需要加入：

```pro
FORMS += \
        widget.ui \
        operatorwindow.ui
```

新增 `.ui` 后，Qt 的 uic 会生成 `ui_operatorwindow.h`。

## 测试

需要验证：

- `qmake serialhelper.pro` 成功。
- Qt Creator 中 `operatorwindow.ui` 能以设计器打开。
- Build 通过。
- 运行后首先显示普通用户主界面。
- 中文不乱码。
- 两个主按钮点击后能在 `已关闭 / 已开启` 间切换。
- 功率控件限制在 `2-100%`。
- 错误密码不会打开开发者界面。
- 正确密码会隐藏普通用户界面并打开开发者界面。
- 关闭开发者界面后普通用户界面重新出现。
