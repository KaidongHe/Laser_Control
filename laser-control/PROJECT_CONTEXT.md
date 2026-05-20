# 项目上下文文档（PROJECT_CONTEXT）

> 用途：给后续接手的 AI 模型 / 工程师提供"读完即能动手"的完整上下文。
> 范围：覆盖 README 不写的隐式约定、代码位置、坑点、心智模型、当前进行中的工作。
> 维护：每次架构变动后更新本文件，而不是只更新 README。

---

## 0. 一句话项目定位

**`serialhelper-1.2`** = Qt 5 上位机（PC）+ STM32 下位机（板卡），
通过 **115200 8N1 串口** 控制 **三路激光器（L1 / L2 / L3）** 的电流，并实时回采电压 / 电流。
**协议**：单字符 ASCII（`'0'~'9'`，无校验、无 ACK）。
**联锁规则**：L1 → L2 → L3 单向依赖 + 90 mA 阈值 + 温度就绪 GPIO。

---

## 1. 仓库结构

```
serialhelper-1.2/
├── serialhelper/                # Qt 上位机
│   ├── serialhelper.pro         # qmake 工程，加了 /utf-8、QT += serialport
│   ├── main.cpp                 # 入口：高 DPI、UTF-8 codec
│   ├── widget.h / widget.cpp    # 主窗口逻辑（串口、状态机、TRY、信号槽）
│   ├── widget.ui                # 卡片式暗色 UI（QGroupBox card1/2/3）
│   ├── laserchart.h / .cpp      # 自绘双曲线折线图（不依赖 QtCharts）
│   ├── serialhelp.ico
│   └── README.md                # 用户向说明文档
├── stm32/
│   └── main.c                   # 下位机主程序（HAL，单文件代表性节选）
├── TODO.html                    # 安全/联锁加固 TODO 清单（暗色排版）
└── PROJECT_CONTEXT.md           # 本文件
```

---

## 2. 架构总览

```
                   ┌──────────────────────── PC ─────────────────────┐
                   │                                                  │
                   │   widget.cpp (Qt)                                │
                   │   ├─ QSerialPort  ←→ 115200/8N1 ASCII '0'~'9'    │
                   │   ├─ 状态机 (Ready / 联锁 / TRY)                 │
                   │   ├─ 解析回报 → measured                         │
                   │   ├─ LaserChart × 3（setpoint 虚线 + measured）  │
                   │   └─ 日志区（INFO/WARN/ERROR/SEND/TRY/DEBUG）    │
                   │                                                  │
                   └──────────────────────────┬───────────────────────┘
                                              │ UART (USART2)
                   ┌──────────────────────────┴───────────────────────┐
                   │   STM32 (main.c, HAL)                            │
                   │   ┌─ HAL_UART_Receive 阻塞 100 ms (单字节)       │
                   │   ├─ switch(uart_cmd)                            │
                   │   │    '0'-'3' → L1 DAC1_CH1 (内置)              │
                   │   │    '4'-'7' → L2 DAC1_CH2 (内置)              │
                   │   │    '8'-'9' → L3 MCP4725  (I2C1)              │
                   │   ├─ HAL_DAC_SetValue / MCP4725_SetVoltage       │
                   │   ├─ HAL_Delay (100 / 200 ms 稳定)               │
                   │   ├─ ADC1+DMA 4ch → ADC_Value[0..3]              │
                   │   │    [0] L1 输出, [1] L2 输出                  │
                   │   │    [2] L3 电压(PA6), [3] L3 电流(PA7)        │
                   │   ├─ printf("LaserN: DAC=..., 输出电流=X.XXXA…") │
                   │   ├─ 200 ms 防抖读 GPIO                          │
                   │   │    PB13=L1 ready, PB14=L2 ready              │
                   │   │    PB15=L3 ready (代码已注释，硬编码 1)      │
                   │   └─ enable 输出: PB12 / PA15 / PC4              │
                   └──────────────────────────┬───────────────────────┘
                                              │
                   ┌──────────────────────────┴───────────────────────┐
                   │   激光器物理 × 3 + 温控板                         │
                   └──────────────────────────────────────────────────┘
```

---

## 3. 串口协议（与 STM32 对齐）

| 指令 | 作用 | DAC 增量 | ≈ 电流增量 |
|---|---|---|---|
| `'0'` | L1 粗调降 | −31 | −10 mA |
| `'1'` | L1 粗调升 | +31 | +10 mA |
| `'2'` | L1 细调降 | −3  | −1 mA  |
| `'3'` | L1 细调升 | +3  | +1 mA  |
| `'4'` | L2 粗调升 | +31 | +10 mA |
| `'5'` | L2 粗调降 | −31 | −10 mA |
| `'6'` | L2 细调升 | +3  | +1 mA  |
| `'7'` | L2 细调降 | −3  | −1 mA  |
| `'8'` | L3 升     | +20 | +100 mA |
| `'9'` | L3 降     | −20 | −100 mA |

**协议特性（重要约定）**：

- 一次只发一个字节，无帧头、无长度、无校验、无应答。
- STM32 主循环用 `HAL_UART_Receive(huart2, &c, 1, 100ms)` 阻塞收。
- 上位机每发一次至少间隔 `minSendIntervalMs = 120 ms`（`widget.h` 内）。
- DAC 钳位：L1/L2 内置 12 位 → 上限 3103；L3 MCP4725 → 上限 1861。
- STM32 回报固定模板：
  ```
  Laser1: DAC=124, 输入电流=0.100A, 输入电压=0.250V, 输出电流=0.098A, 输出电压=0.245V
  ```
- 上位机解析正则：`Laser([123])[^\n]*?输出电流\s*=\s*([0-9]+\.?[0-9]*)\s*A`
- 中文回报先按 UTF-8 解，失败回落 GBK；按 `\n` 分行做粘包合并。

---

## 4. 上位机关键代码位置

| 主题 | 文件 / 函数 / 行 |
|---|---|
| DEBUG_MODE 宏 | [widget.h:19](serialhelper/widget.h#L19) — 默认开启，发布前必须注释 |
| 颜色常量 | [widget.cpp:11-15](serialhelper/widget.cpp#L11-L15) — `COLOR_L1/L2/L3/MEASURED` + `L_THRESHOLD=90` |
| 串口热插拔 | `Widget::checkSerialPorts` 每秒一次 |
| 自动重连 | `Widget::autoReconnectSerialPort` (2s 重试) |
| 数据接收 / 解析 | `Widget::serialPortReadyRead_Slot` + `parseMeasuredFromLine` |
| 联锁判断 | `Widget::canControlLaser(int)` — **基于 setpoint，不是 measured** |
| 阻塞原因 | `Widget::blockReason(int)` — 给 UI "等待 L1 输出 > 90 mA" |
| 状态机更新 | `updateAllLaserStates` / `updateLaserDependencies` |
| 步进 | `Widget::adjustLaser(int laser, int dir)` |
| spinbox 直输 | `on_laserNspinbox_editingFinished` — **循环连发指令直到逼近** |
| TRY 扫描 | `on_tryButton_clicked` (启动) + `tryStep` (timer 回调) |
| 自绘曲线 | [serialhelper/laserchart.cpp](serialhelper/laserchart.cpp) — `paintEvent` 全自绘 |

---

## 5. 上位机核心状态机（必须搞懂）

每路激光器有两个独立状态变量：

| 变量 | 含义 |
|---|---|
| `laserNRawReady`  | 该路自身的"温度就绪/STM32 已应答" |
| `laserNReady`     | 综合上游依赖后的"上位机视角可控" |

**派生规则**（`updateAllLaserStates`）：

```
laser1Ready = laser1RawReady
laser2Ready = laser1Ready && laser2RawReady
laser3Ready = laser1Ready && laser2Ready && laser3RawReady
```

**可控判定**（`canControlLaser`）：

```
L1: 总是 true（联锁靠 raw ready）
L2: laser1RawReady && currentLaser1mA  > 90 && (在更新时 laser2RawReady)
L3: laser1RawReady && laser2RawReady && currentLaser2mA > 90
```

**关键陷阱**：`updateLaserStatusFromSend` 会在每次"成功 write 一个字节"后把对应通道 raw ready 置 1。
这意味着即使 STM32 没真的执行（断线、故障），上位机也会显示就绪 —— **见第 7 节风险点 #6**。

---

## 6. 下位机核心约定（[stm32/main.c](stm32/main.c)）

- **温度就绪输入**：`PB13/PB14/PB15`，200 ms 防抖；**PB15 检测被注释**，硬编码 `temp_status_prev3 = 1`（见 [main.c:81](stm32/main.c#L81)、[main.c:232](stm32/main.c#L232)、[main.c:278-294](stm32/main.c#L278-L294)）。
- **enable 输出**：`PB12 / PA15 / PC4`，由 `UpdateLaserEnableStatus` 跟随 priority 翻转。
- **优先级函数**：
  - `CheckLaser1Priority` = `temp_status_prev1`
  - `CheckLaser2Priority` = `prev1 && prev2`
  - `CheckLaser3Priority` = `prev1 && prev2 && prev3`
- **每次设 DAC 后**：`HAL_Delay(100~200ms)` → `UpdateADCValues()` 重启 DMA → `printf` 单条状态。
- **L3 ADC 映射**（线性反插值，见 [main.c:107-122](stm32/main.c#L107-L122)）：
  - 电压：`PA6 (0–3.26V) → 实际 2.0–3.3V`
  - 电流：`PA7 (0–2.71V) → 实际 0.8–10A`
- **不存在的功能**：上位机看门狗 / 喂狗超时归零 / ACK / 校验 / 直接设定目标 DAC。

---

## 7. 已知风险点 / 技术债（见 [TODO.html](TODO.html)）

> 这一节是 README 不会写的"心智地雷"，接手者必读。

| # | 位置 | 风险 |
|---|---|---|
| 1 | [stm32/main.c:81](stm32/main.c#L81) | L3 温度检测注释、硬编码 1。温控失效时上下位机都不会知道 |
| 2 | [widget.cpp `canControlLaser`](serialhelper/widget.cpp) | 用 setpoint 判 90 mA，硬件没跟上时仍放行 L2/L3 |
| 3 | 串口协议 | 单字符无校验无 ACK，电磁噪声会引发误增减 |
| 4 | STM32 主循环 | 无看门狗。上位机崩溃后 DAC 保持现值，不会自动归零 |
| 5 | spinbox 直输 | 误输大数会几秒内连发数百条指令冲到顶 |
| 6 | `updateLaserStatusFromSend` | "写成功 = 已就绪"，存在假就绪 |
| 7 | TRY 扫描 | 不监控 measured 与 setpoint 偏差，驱动饱和仍会按时间表往上爬 |

---

## 8. 当前进行中的工作 / 下一步

> 截至 2026-05-20。当前路线由 brainstorming 会话确定。

**目标**：方向 D — **安全 / 联锁加固**。
**阶段**：用户正在对上面 7 条风险点逐项标注 🔴/🟡/⚪ 优先级；标完即进入"提 2-3 个加固方案 → 设计 → 写实现 plan"。

可能产出（取决于打分结果）：

- 协议升级到带帧头 + CRC + ACK，并加 STM32 端看门狗（如果上位机心跳超时则 DAC 归零）。
- 把上位机联锁阈值从 setpoint 切到 measured，并允许"几秒内 measured 没赶上则中止 TRY"。
- 加急停按钮 + 全局软中止接口。
- 恢复 PB15 温度检测，并定义"温控失效"的 fail-safe 行为。

---

## 9. 调试 / 复现技巧

- **无硬件场景**：保留 [widget.h:19](serialhelper/widget.h#L19) 的 `#define DEBUG_MODE`，所有发送只走日志，UI/TRY/状态机都能跑。
- **检查协议是否生效**：发完一个字节后 STM32 必然 `printf` 一行 `LaserN: DAC=...` —— 没看到说明指令没被识别（可能波特率错或粘包）。
- **验证联锁**：把 L1 setpoint 拉到 91 mA 时 L2 步进按钮应该立刻变可用；拉回 89 mA 应该立刻变灰。
- **L3 表现异常**：先看 `[2]/[3]` 两路 ADC 原始值（[main.c:507-515](stm32/main.c#L507-L515) 的 callback 会打印），再用 `Laser3_ADCtoVoltage/ADCtoCurrent` 反推。
- **中文乱码**：上位机 `decodeSerialData` 已先 UTF-8 后 GBK 自动回退；但是 STM32 注释和 printf 是 GBK 源码，改 main.c 时小心编码。
- **重新编译 STM32**：仓库里只有 `main.c`，完整工程（HAL 头/启动文件）不在仓库，需配套 STM32CubeIDE 工程。

---

## 10. 给下一个模型的"心智模型"提示

读这个项目时容易踩的几个误判：

1. **"setpoint = 实际输出"** —— 不是。代码里用的几乎都是 setpoint，但物理上要看 measured。改安全相关代码时务必区分。
2. **"L3 是普通通道"** —— 不是。L3 用外置 MCP4725 + I2C1，DAC 范围、量程、步长（最小 100 mA）、ADC 映射都和 L1/L2 不同；温度检测还被禁用。
3. **"updateLaserStatusFromSend 表示真的就绪"** —— 不表示。仅仅是"上位机刚发了一个字节"。
4. **"DEBUG_MODE 关掉就 OK"** —— 注意所有的 `#ifdef DEBUG_MODE` 短路逻辑（`canControlLaser` 等），这些短路会让"开发时一切正常，发布后立刻全灰"成为典型回归。
5. **"协议很简单可以随便加新指令"** —— 上位机解析回报的正则、`updateLaserStatusFromSend` 的索引、STM32 的 `if (uart_cmd >= '0' && uart_cmd <= '9')` 边界都要同步改。考虑直接做协议升级（v2）而不是塞进现有空间。
6. **"卡片式 UI 是 QtCharts"** —— 不是，是自绘 [laserchart.cpp](serialhelper/laserchart.cpp)，paintEvent 里手画网格 / 阈值线 / 双曲线。改样式改这个文件，不要去找 QtCharts。
7. **"中文是 UTF-8"** —— 上位机源文件是 UTF-8，STM32 源文件 (`main.c`) 是 **GBK**。改 STM32 注释或 printf 字面量时保持 GBK，否则烧进去看到的就是乱码。

---

## 11. 备查 · 关键参数速览

| 参数 | 值 | 出处 |
|---|---|---|
| 串口波特率 | 115200 | `Widget::on_openBt_clicked` |
| 串口格式 | 8N1 无流控 | 同上 |
| 防抖间隔 | 120 ms / 通道 | `Widget::minSendIntervalMs` |
| 串口扫描 | 1000 ms | `serialCheckTimer` |
| 状态轮询 | 3000 ms | `statusCheckTimer` |
| 自动重连延时 | 2000 ms | `autoReconnectTimer` |
| 联锁阈值 | 90 mA | `L_THRESHOLD` |
| L1/L2 量程 | 0–1000 mA | `widget.cpp` 构造 + spinbox setRange |
| L3 量程 | 800–10000 mA | 同上 |
| L3 spinbox 步长 | 100 mA | `setSingleStep(100)` |
| 曲线时间窗 | 60 s | `LaserChart::m_timeWindowSec` |
| 曲线最大点数 | 600 | `addDataPoint / addMeasuredPoint` 内 |
| 温度防抖 | 200 ms | `TEMP_DEBOUNCE_MS` |
| L3 稳定延时 | 200 ms | `LASER3_SETTLE_DELAY` |
| TRY L1 默认 | 阶段 1/2/3 各 90s，步长 10 mA，终点 98 mA | dialog 默认值 |
| TRY L2 默认 | 终点 460 mA，步长 10 mA，30 s | 同上 |
| TRY L3 默认 | 终点 5000 mA，步长 100 mA，60 s | 同上 |

---

最后维护时间：**2026-05-20**
当前活动：**安全/联锁加固方向 brainstorming，等用户对 [TODO.html](TODO.html) 的 7 条风险点打优先级标签。**
