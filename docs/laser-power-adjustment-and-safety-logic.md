# 激光控制逻辑与安全审查说明（2026-05-21）

本文档基于当前 `laser-control/serialhelper` 工程代码整理，不包含 `debug`、`release` 目录。本文只做静态代码审查与逻辑说明，未进行编译、串口联调或硬件验证。

## 1. 当前总体结论

当前主线控制逻辑已经集中到 `LaserController`，普通操作员页面 `operatorForm` 和开发者页面 `Widget` 都通过同一个控制核心执行串口、联锁、缓升/缓降和目标值判断。

当前代码具备以下软件侧保护：

- L1/L2/L3 顺序联锁集中在 `LaserController::canAdjustLaser()`。
- 最终发送前仍由 `LaserController::sendLaserCommand()` 做方向和联锁兜底。
- 普通页面 L1/L2 开启为三段式缓升/回落流程。
- 普通页面 L3 功率百分比通过配置映射到 mA。
- 配置项读取后会做基础范围保护，避免 0 步长、反向百分比范围和明显越界值。

但当前仍存在几项需要特别注意的安全风险。接真实硬件前，至少应处理 `DEBUG_MODE`、开发者密码、ready 推导策略、配置文件保护和串口输入缓冲边界。

## 2. 当前架构

```text
main.cpp
  └── operatorForm 普通操作员页面
        ├── LaserController 控制核心
        └── Widget 开发者页面（密码进入，复用同一个 LaserController）
```

### 2.1 `LaserController`

文件：

- `lasercontroller.h`
- `lasercontroller.cpp`

职责：

- 读取 `laser_config.ini`。
- 维护三路激光器设定电流和实测电流。
- 管理串口打开、关闭、接收、发送、断开和自动重连。
- 执行顺序联锁。
- 执行定时器分步缓升/缓降。
- 发出日志、电流变化、ready 变化、串口状态变化和操作完成信号。

### 2.2 `operatorForm`

职责：

- 普通操作员界面。
- L1 开关对应 L1。
- 预放开关对应 L2。
- 功率 SpinBox 对应 L3 百分比功率。
- 所有操作只向 `LaserController` 发请求，不直接发串口命令。
- 按钮“已开启”状态按 `L1FinalMa` / `L2FinalMa` 判断，不再只按联锁阈值提前变绿。

### 2.3 `Widget`

职责：

- 开发者调试页面。
- 显示三路激光器设定值、实测值、曲线、ready 和日志。
- 提供手动增减、SpinBox 目标输入、TRY 扫描。
- 开发者页面也通过 `LaserController` 操作，避免绕过普通页面共用的联锁核心。

## 3. 配置文件

配置文件路径由：

```cpp
QCoreApplication::applicationDirPath() + "/laser_config.ini"
```

决定。首次运行时，如果该文件不存在，程序会生成默认配置。

当前配置项包括：

- `Interlock`：L1/L2/L3 联锁阈值和安全关闭态。
- `Range`：L1/L2/L3 设定电流上限。
- `OperatorSoftStart`：普通页面 L1/L2 三段式启动目标和时间。
- `Step`：L1/L2 粗细调步长、L3 步长。
- `L3OperatorPower`：普通页面百分比到 L3 mA 的映射。
- `Ramp`：默认 ramp 间隔、最小 ramp 间隔、手动发送节流。

注意：当前没有把缺失配置项写回老配置文件。老配置文件缺字段时程序会使用默认值，但文件本身不会自动补全。

## 4. 电流与状态

当前三路设定电流由 `LaserController` 维护：

| 通道 | 当前变量 | 默认安全态 |
|---|---|---:|
| L1 | `currentLaser1mA` | `0 mA` |
| L2 | `currentLaser2mA` | `0 mA` |
| L3 | `currentLaser3mA` | `L3_SAFE_OFF_MA`，默认 `800 mA` |

实测电流来自 STM32 串口文本：

```text
Laser1 ... 输出电流 = X.XXX A
Laser2 ... 输出电流 = X.XXX A
Laser3 ... 输出电流 = X.XXX A
```

解析后转换为 mA，并通过 `measuredChanged()` 通知界面。

## 5. 顺序联锁

联锁入口：

```cpp
bool LaserController::canAdjustLaser(int laserIndex, int direction) const
```

其中：

- `direction > 0` 表示升高/开启。
- `direction < 0` 表示降低/关闭。

### 5.1 开机/升高顺序

必须按：

```text
L1 -> L2 -> L3
```

默认规则：

| 操作 | 默认允许条件 |
|---|---|
| 升 L1 | 有控制通道，L1 ready，L2 已到安全关闭态，L3 已到安全关闭态 |
| 升 L2 | 有控制通道，L1/L2 ready，L1 >= `L1EnableL2Ma`，L3 已到安全关闭态 |
| 升 L3 | 有控制通道，L1/L2/L3 ready，L1 >= `L1EnableL2Ma`，L2 >= `L2EnableL3Ma` |

### 5.2 关机/降低顺序

必须按：

```text
L3 -> L2 -> L1
```

默认规则：

| 操作 | 默认允许条件 |
|---|---|
| 降 L3 | 允许 |
| 降 L2 | L3 <= `L3SafeOffMa` |
| 降 L1 | L2 <= `L2SafeOffMa` 且 L3 <= `L3SafeOffMa` |

拒绝原因由：

```cpp
QString LaserController::adjustBlockReason(int laserIndex, int direction) const
```

生成。普通页面用弹窗提示，开发者页面显示到日志或 reason label。

## 6. 普通操作员页面逻辑

### 6.1 L1 开启

入口：

```cpp
operatorForm::toggleSeed()
LaserController::requestOperatorSwitch(1, true)
```

普通页面 L1 开启不是直接跳到最终电流，而是进入 `startOperatorSoftOn(1)` 三段式曲线：

```text
当前值 -> HighMa -> MiddleMa -> L1FinalMa
```

默认：

```text
0 -> 850 -> 200 -> 90
```

按钮在 ramp 期间显示“正在开启...”，只有最终阶段完成后才显示“已开启”。

### 6.2 L2 开启

入口：

```cpp
operatorForm::togglePreRelease()
LaserController::requestOperatorSwitch(2, true)
```

普通页面 L2 开启同样进入三段式曲线：

```text
当前值 -> HighMa -> MiddleMa -> L2FinalMa
```

默认：

```text
0 -> 850 -> 200 -> 90
```

L2 开启前必须满足 L1 已达到 `L1EnableL2Ma`。

### 6.3 L1/L2 关闭

普通页面关闭 L1/L2 时：

- L2 关闭必须等 L3 回到 `L3SafeOffMa`。
- L1 关闭必须等 L2 回到 `L2SafeOffMa` 且 L3 回到 `L3SafeOffMa`。
- 关闭过程也是 ramp，按钮不会提前变灰。

### 6.4 L3 百分比功率

入口：

```cpp
operatorForm::applyPowerPercent()
LaserController::requestOperatorPowerPercent(percent)
```

默认映射：

```text
2%   -> 800 mA
100% -> 5000 mA
```

映射参数来自 `laser_config.ini` 的 `L3OperatorPower`。最终目标会按 L3 步长对齐，默认 100 mA。

## 7. 开发者页面逻辑

开发者页面的手动按钮调用：

```cpp
LaserController::adjustLaser(laserIndex, direction, coarseMode)
```

SpinBox 直接目标输入调用：

```cpp
LaserController::setLaserTarget(laserIndex, target, coarseMode)
```

开发者页面上调/下调按钮分别根据：

```cpp
canAdjustLaser(laserIndex, +1)
canAdjustLaser(laserIndex, -1)
```

独立启用或禁用。SpinBox 在 ramp 期间会被锁住，避免覆盖尚未完成的目标。

## 8. TRY 扫描

TRY 扫描仍主要在 `Widget` 中维护参数和状态机：

```text
TryIdle -> TryPhase1 -> TryPhase2 -> TryPhase3 -> TryPhaseL2 -> TryPhaseL3
```

每一步实际推进时已经改成调用：

```cpp
LaserController::setLaserTarget(...)
```

因此 TRY 不再直接绕过控制核心发送串口命令。

当前注意事项：

- TRY 界面仍有部分目标值、文字和范围写死，例如 `850`、`200`、`800`、`10000`。
- 开发者页面普通调节已基本跟随配置，但 TRY 还没有完全配置化。
- TRY 进入 L2/L3 阶段前仍会调用 `canAdjustLaser()` 做联锁判断。

## 9. 串口与命令

命令仍是单字节 ASCII：

| 通道 | 模式 | 默认步长 | 升高 | 降低 |
|---|---|---:|---|---|
| L1 | 粗调 | `10 mA` | `'1'` | `'0'` |
| L1 | 细调 | `1 mA` | `'3'` | `'2'` |
| L2 | 粗调 | `10 mA` | `'4'` | `'5'` |
| L2 | 细调 | `1 mA` | `'6'` | `'7'` |
| L3 | 固定步进 | `100 mA` | `'8'` | `'9'` |

`sendLaserCommand()` 会先由命令反推方向，然后再次调用 `canAdjustLaser()` 做最终联锁检查。

真实串口模式下还会检查每路手动发送最小间隔 `MinManualSendIntervalMs`。ramp 控制发送由 ramp 定时器统一控制节奏。

## 10. 安全审查结果

### 10.1 高风险：`DEBUG_MODE` 当前在头文件中启用

位置：

- `lasercontroller.h`

风险：

- `hasLaserTransport()` 会直接返回 `true`。
- `openSerial()` 会走模拟连接。
- `sendLaserCommand()` 不写真实串口，只写模拟日志。
- ready 信号默认模拟为就绪。

影响：

- 如果该宏进入真实硬件版本，界面会显示控制通道可用，但实际没有真实硬件发送。
- 容易误判真实串口、ready、错误处理和硬件响应流程。

建议：

- 不要在头文件里默认 `#define DEBUG_MODE`。
- 改为 qmake / CMake 构建参数控制，例如只在 Debug 配置中定义。
- 发布或硬件联调前必须确认没有启用 `DEBUG_MODE`。

### 10.2 高风险：发送成功会推导 ready

位置：

- `LaserController::updateLaserStatusFromSend()`

当前逻辑：

```cpp
setRawReady(laserIndex, true);
```

风险：

- 只要命令发送成功，就会把对应通道推导为 raw ready。
- 这会削弱“温度就绪/硬件就绪”作为安全条件的意义。

影响：

- 如果硬件未真正 ready，但上位机发送过命令，软件侧可能误认为该通道可控。

建议：

- 真实模式下不要通过发送成功推导 ready。
- ready 应只来自 STM32 明确状态帧、硬件 GPIO 状态或更可信的协议字段。
- 如果为了兼容旧调试流程保留该逻辑，应仅限 Debug 模式。

### 10.3 中高风险：配置文件可改变安全阈值和上限

位置：

- `LaserController::configFilePath()`
- `laser_config.ini`

风险：

- 配置文件位于程序运行目录。
- 本地用户可修改 `L1EnableL2Ma`、`L2EnableL3Ma`、`Laser3MaxMa`、`MinIntervalMs`、步长和时间。
- 当前只做范围归一化，不做签名、权限校验或安全上限白名单。

影响：

- 如果运行目录可被普通操作员写入，可能绕过预期启动阈值或扩大电流上限。

建议：

- 部署时保证运行目录和配置文件只有维护人员可写。
- 对关键安全上限增加代码内不可突破的硬限制。
- 可增加配置校验日志，启动时输出最终生效参数。
- 高安全场景可加入配置签名或校验和。

### 10.4 中风险：开发者密码硬编码

位置：

- `operatorform.cpp`

当前逻辑：

```cpp
const char *DEVELOPER_PASSWORD = "laser2026";
```

风险：

- 密码写在源码中，也会出现在二进制可见字符串中。
- 没有锁定次数、审计记录或角色权限。

影响：

- 任何拿到程序或源码的人都能提取开发者密码。

建议：

- 至少改为外部配置并限制文件权限。
- 更稳妥做法是使用维护员登录、一次性维护码或硬件钥匙。
- 增加失败次数限制和操作日志。

### 10.5 中风险：串口 ready 文本容易被伪造

位置：

- `LaserController::serialPortReadyRead()`

当前逻辑会通过文本包含关系识别：

```text
laser1ready
laser2ready
laser3ready
L1:OK
L2:OK
L3:OK
```

风险：

- 只要串口设备输出包含这些文本，就能改变上位机 ready 状态。
- 当前没有帧头、校验、设备身份、序列号或 CRC。

影响：

- 接错串口或串口数据被污染时，软件可能误判硬件状态。

建议：

- STM32 输出改为结构化状态帧。
- 增加帧头、长度、CRC、设备 ID。
- 上位机只接受完整合法帧，不再用宽松字符串包含判断 ready。

### 10.6 中低风险：串口接收缓冲没有最大长度

位置：

- `LaserController::serialPortReadyRead()`

当前逻辑：

```cpp
rxBuffer.append(chunk);
```

如果串口持续输入没有 `\n` 的数据，`rxBuffer` 会持续增长。

影响：

- 异常设备或错误数据可能造成内存增长。

建议：

- 给 `rxBuffer` 设置最大长度，例如 4 KB 或 16 KB。
- 超过上限时清空并记录警告。
- 单行超过最大长度时丢弃该行。

### 10.7 低风险：TRY 扫描仍有硬编码安全参数

位置：

- `Widget::on_tryButton_clicked()`
- `Widget::tryStep()`

风险：

- 普通调节已经配置化，但 TRY 扫描仍有部分硬编码目标和范围。

影响：

- 配置文件调整后，开发者 TRY 扫描可能与主控制配置不一致。

建议：

- 增加 `softHighMa()`、`softMiddleMa()`、`l1FinalMa()`、`laserMinMa()`、`laserMaxMa()` 等 getter 后，把 TRY 的硬编码值替换为配置值。

## 11. 未发现的高危传统安全问题

当前静态检查中没有发现：

- 网络监听端口。
- `system()` / `QProcess` 调用外部命令。
- SQL 或脚本执行。
- 明显的任意文件删除。
- HTML 富文本注入路径，日志显示主要使用 `appendPlainText()`。

这不代表系统没有风险。该项目主要风险不是互联网攻击面，而是本地配置、串口可信度、Debug 构建和工业控制安全边界。

## 12. 建议修复优先级

### P0：真实硬件前必须确认

1. 发布/硬件联调版本关闭 `DEBUG_MODE`。
2. 禁止通过发送成功推导 ready，或仅限 Debug 模式。
3. 确认 `laser_config.ini` 的部署权限，普通操作员不可写。

### P1：下一轮建议修复

1. 开发者密码移出源码，增加失败限制和审计日志。
2. 串口 ready 协议改为结构化帧 + CRC。
3. 给 `rxBuffer` 增加最大长度。
4. TRY 扫描参数完全配置化。

### P2：后续清理

1. 清理 `Widget` 中遗留的旧串口成员和空实现函数。
2. 把 Debug/Release 行为移到构建系统管理。
3. 启动时打印最终生效配置，便于现场排查。

## 13. 通信安全改造执行方案

本节只描述后续代码改造方案，不表示当前代码已经完成这些改造。建议按 P0 -> P1 -> P2 -> P3 顺序推进，每一级完成后都单独联调和记录日志，不要一次性把所有通信协议都重写。

### P0：关闭生产环境 Debug 模式，禁止发送后自动 ready

目标：

- 真实硬件版本必须依赖真实串口和下位机状态。
- 上位机发送命令后不能自行推导 ready。
- ready 只能来自下位机明确回报，或来自真实硬件状态输入。

涉及文件：

- `laser-control/serialhelper/lasercontroller.h`
- `laser-control/serialhelper/lasercontroller.cpp`
- `laser-control/serialhelper/serialhelper.pro`

执行步骤：

1. 将 `lasercontroller.h` 中的 `#define DEBUG_MODE` 移出头文件，避免任何构建默认进入模拟模式。
2. 在 `serialhelper.pro` 中只为 Qt Debug 构建显式定义模拟模式，例如放到 `CONFIG(debug, debug|release)` 分支中。
3. 程序启动时输出当前控制模式日志：
   - Debug 模拟模式：`[BOOT] DEBUG_MODE enabled, serial commands are simulated`
   - 真实硬件模式：`[BOOT] hardware serial mode`
4. 修改 `LaserController::updateLaserStatusFromSend()`：
   - Debug 模式可以保留 `setRawReady(laserIndex, true)`，方便无硬件调试。
   - 真实模式必须禁止通过发送成功推导 ready。
5. 修改 `LaserController::sendLaserCommand()` 后续调用关系：
   - 真实模式下发送成功只代表字节进入串口发送缓冲。
   - 不再因为 `write()` 成功就改变 ready。
6. 检查所有依赖 ready 的入口：
   - `canAdjustLaser()`
   - `adjustBlockReason()`
   - 普通页面按钮锁定逻辑
   - 开发者页面按钮锁定逻辑

验收标准：

- Release/硬件联调构建中搜索不到头文件级 `#define DEBUG_MODE`。
- 未收到下位机 ready 回报时，L1/L2/L3 上调都不能越过 ready 联锁。
- 发送一次命令后，如果下位机不回 ready，界面仍显示未就绪或等待状态。
- Debug 构建仍可按模拟流程运行，但日志必须明确显示当前是模拟模式。

### P1：增加 ACK/NAK、序号、超时重试，ACK 后才更新软件电流

目标：

- 上位机每条控制命令都有唯一序号。
- 下位机必须返回 ACK 或 NAK。
- 上位机只在收到匹配 ACK 后更新 `currentLaserXmA`。
- 超时或 NAK 时停止当前 ramp，并给出明确故障提示。

推荐协议字段：

```text
CMD:
  seq        命令序号，1~255 循环
  laser      1/2/3
  action     UP/DOWN
  stepMa     本次步进 mA
  targetMa   本次期望后的设定值

ACK:
  seq        必须等于 CMD.seq
  laser      必须等于 CMD.laser
  accepted   1
  currentMa  下位机确认后的设定电流或实际输出电流

NAK:
  seq
  laser
  accepted   0
  reason     RANGE / BUSY / NOT_READY / CRC_ERROR / UNKNOWN_CMD
```

涉及文件：

- `laser-control/serialhelper/lasercontroller.h`
- `laser-control/serialhelper/lasercontroller.cpp`
- 下位机 STM32 串口协议代码

上位机执行步骤：

1. 在 `LaserController` 中增加“待确认命令”状态：
   - `pendingSeq`
   - `pendingLaserIndex`
   - `pendingDirection`
   - `pendingStepMa`
   - `pendingTargetMa`
   - `pendingRetryCount`
   - `pendingCommandBytes`
2. 增加 ACK 超时定时器，例如 `ackTimeoutTimer`。
3. `sendLaserCommand()` 不再直接更新电流，只负责发送命令并进入等待 ACK 状态。
4. `processRampStep()` 在存在待确认命令时不能继续发送下一步。
5. `serialPortReadyRead()` 解析 ACK：
   - 序号匹配才接受。
   - 通道匹配才接受。
   - ACK 中的 `currentMa` 必须在本通道允许范围内。
   - 通过后调用 `setCurrentLaserMa(laserIndex, currentMa)`。
6. `serialPortReadyRead()` 解析 NAK：
   - 停止当前 ramp。
   - 写日志：通道、序号、原因。
   - `operationFinished(..., false, reason)` 通知界面。
7. ACK 超时处理：
   - 超时后重发原命令。
   - 默认最多重试 3 次。
   - 3 次仍失败则关闭当前 ramp，保留当前软件状态，不假设硬件已经执行。
8. 对重复 ACK 做幂等处理：
   - 如果 ACK 序号等于最近已完成序号，可以记录日志并忽略。
   - 如果 ACK 序号未知或通道不匹配，丢弃并报警。

下位机配合步骤：

1. 每次收到合法命令后返回 ACK。
2. 命令无法执行时返回 NAK，不要静默丢弃。
3. ACK 中回传下位机确认的当前电流。
4. 下位机忙、未就绪、越界、CRC 错误都要有明确 NAK reason。

验收标准：

- 拔掉或屏蔽下位机回包时，上位机不会继续 ramp。
- 下位机返回 NAK 时，上位机立即停止当前操作并显示原因。
- 下位机 ACK 序号不匹配时，上位机不更新电流。
- 只有收到合法 ACK 后，`currentChanged()` 才触发界面电流变化。
- 日志中可以完整追踪：发送 seq、ACK/NAK seq、重试次数、失败原因。

### P2：增加设备握手、CRC、接收缓冲上限、配置硬上限

目标：

- 防止选错串口。
- 防止串口噪声或脏数据被误识别为 ready/ACK。
- 防止异常输入造成 `rxBuffer` 无限制增长。
- 防止配置文件突破硬件安全边界。

推荐帧格式：

```text
Frame:
  magic       0xAA 0x55
  version     协议版本
  type        HELLO / STATUS / CMD / ACK / NAK / FAULT
  seq         序号
  length      payload 长度
  payload     结构化数据
  crc16       对 version/type/seq/length/payload 计算
```

设备握手执行步骤：

1. `openSerial()` 成功后不要立刻认为控制通道可用。
2. 上位机发送 `HELLO`。
3. 下位机返回：
   - `deviceId = LASER_CTRL`
   - `protocolVersion`
   - `firmwareVersion`
   - 支持的通道数量
   - 每通道安全范围
4. 上位机校验：
   - 设备 ID 必须匹配。
   - 协议版本必须兼容。
   - 下位机声明范围不能超过上位机硬上限。
5. 只有握手成功后，`hasLaserTransport()` 才允许返回 true。
6. 握手失败时关闭串口或进入“已连接但不可控制”状态。

CRC 执行步骤：

1. 上位机所有 CMD 帧增加 CRC16。
2. 下位机所有 ACK/NAK/STATUS/FAULT 帧增加 CRC16。
3. `serialPortReadyRead()` 只处理 CRC 正确的完整帧。
4. CRC 错误帧只记录警告，不改变 ready、电流、busy 或联锁状态。

接收缓冲上限执行步骤：

1. 给 `rxBuffer` 增加硬上限，例如 `RX_BUFFER_MAX_BYTES = 8192`。
2. 给单帧增加硬上限，例如 `MAX_FRAME_BYTES = 256`。
3. `rxBuffer.append(chunk)` 后立即检查长度。
4. 超过上限时：
   - 清空 `rxBuffer`。
   - 记录 `[WARN] RX buffer overflow, buffer cleared`。
   - 不改变任何激光状态。
5. 帧头错误时丢弃帧头前的垃圾数据。

配置硬上限执行步骤：

1. 在代码中定义不可被 `laser_config.ini` 突破的硬限制，例如：
   - `HARD_LASER12_MAX_MA`
   - `HARD_LASER3_MAX_MA`
   - `HARD_L1_L2_MAX_STEP_MA`
   - `HARD_L3_MAX_STEP_MA`
   - `HARD_MIN_ACK_TIMEOUT_MS`
2. `loadConfig()` 读取配置后，先用现有配置归一化，再用硬上限二次夹紧。
3. 启动日志输出“配置值”和“最终生效值”，现场能看出配置是否被夹紧。
4. 如果配置超过硬上限，应输出 `[WARN] config clamped`。
5. 部署时将 `laser_config.ini` 设置为维护人员可写，普通操作员只读。

验收标准：

- 选错串口时，界面不能进入可控制状态。
- 下位机返回 CRC 错误数据时，ready 和 current 不变化。
- 连续输入无换行或垃圾数据时，内存不会持续增长。
- 修改 `laser_config.ini` 到超限值时，程序启动后会夹紧并记录日志。
- 握手失败、CRC 错误、缓冲溢出都不会触发任何激光调节。

### P3：把实测电流纳入安全判断，形成闭环控制

目标：

- 软件设定值、下位机确认值、实测电流三者不能长期不一致。
- ramp 完成不只看上位机设定值，还要看下位机 ACK 和实测电流。
- 实测异常时自动阻止继续上调，并提示操作员。

建议新增概念：

| 名称 | 含义 |
|---|---|
| `targetMa` | 上位机本次目标值 |
| `confirmedSetMa` | 下位机 ACK 确认的设定值 |
| `measuredMa` | 下位机采样回传的实际输出电流 |
| `measuredFresh` | 实测值是否在有效时间窗口内 |
| `faultState` | 通信、执行或实测异常后的保护状态 |

涉及文件：

- `laser-control/serialhelper/lasercontroller.h`
- `laser-control/serialhelper/lasercontroller.cpp`
- `laser-control/serialhelper/operatorform.cpp`
- `laser-control/serialhelper/widget.cpp`
- 下位机 STM32 状态上报逻辑

执行步骤：

1. 下位机周期性上报 STATUS 帧：
   - ready 状态
   - confirmedSetMa
   - measuredMa
   - fault code
2. 上位机为每路记录最近一次实测时间。
3. 增加配置项：
   - `MeasuredToleranceMa`
   - `MeasuredTimeoutMs`
   - `MeasuredStableCount`
   - `AllowDownWhenFault`
4. 每次 ACK 后，不直接认为该步完全安全完成，而是等待实测电流进入允许误差范围。
5. ramp 期间每一步判断：
   - ACK 是否成功。
   - 实测值是否新鲜。
   - 实测值是否接近 confirmedSetMa。
   - 实测值是否超过硬件安全上限。
6. 如果实测值过期：
   - 禁止继续上调。
   - 允许按安全顺序下调。
   - 记录“实测电流超时”。
7. 如果实测值超过目标过多：
   - 停止 ramp。
   - 进入 fault 状态。
   - 普通页面禁止继续上调。
   - 开发者页面显示实测偏差。
8. fault 状态下建议只允许两类操作：
   - 下调/关闭，且必须遵守 L3 -> L2 -> L1。
   - 人工复位 fault，复位前要求所有通道回到安全关闭态。

验收标准：

- 下位机停止上报实测电流时，上位机不能继续上调。
- 实测电流偏离设定值超过阈值时，上位机停止 ramp 并提示。
- L1/L2/L3 按钮状态同时反映联锁、busy、通信状态和 fault 状态。
- ramp 完成日志包含：目标值、ACK 确认值、实测值、偏差。
- fault 状态下仍允许安全下调，但禁止继续升高。

### 推荐落地顺序

1. 先完成 P0，使真实硬件模式不再依赖模拟状态。
2. 再完成 P1，用 ACK/NAK 把“发送成功”和“执行成功”分开。
3. 然后完成 P2，升级协议可信度并限制异常输入。
4. 最后完成 P3，把硬件实测值纳入闭环判断。

每完成一个优先级，都建议保留以下联调记录：

- 上位机发送日志。
- 下位机接收日志。
- ACK/NAK 或 STATUS 原始帧。
- 操作员界面状态截图。
- 异常场景测试结果。

## 14. 当前可用性总结

当前软件侧联锁主线是清晰的：

```text
普通页面 / 开发者页面
  -> LaserController 请求接口
  -> canAdjustLaser 顺序联锁
  -> ramp 定时器分步执行
  -> sendLaserCommand 最终兜底
```

当前普通操作员页面：

- L1 开关控制 L1。
- 预放开关控制 L2。
- 功率百分比控制 L3。
- L1/L2/L3 操作顺序由控制核心统一限制。
- 按钮不会在 ramp 过程中提前变绿或变灰。

当前开发者页面：

- 手动调节和 SpinBox 输入已经通过控制核心。
- TRY 扫描也通过控制核心推进目标值。
- 但 TRY 参数仍需要进一步配置化。

接真实硬件前，必须把本文第 10 节的高风险项纳入联调检查清单。
