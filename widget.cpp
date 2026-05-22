#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QPlainTextEdit>

static const QColor COLOR_L1(0, 200, 255);
static const QColor COLOR_L2(255, 160, 64);
static const QColor COLOR_L3(192, 128, 255);
static const QColor COLOR_MEASURED(255, 200, 64);

static int normalizeDeveloperStepChoice(int stepMa)
{
    // 开发者可选步长只对应 STM32 的细调/粗调两类指令：1 mA 或 10 mA。
    return (stepMa <= 5) ? 1 : 10;
}

static void setupDeveloperStepCombo(QComboBox *combo, int currentStepMa)
{
    if (!combo) return;

    combo->addItem(QString::fromUtf8(u8"1 mA"), 1);
    combo->addItem(QString::fromUtf8(u8"10 mA"), 10);
    const int normalized = normalizeDeveloperStepChoice(currentStepMa);
    const int index = combo->findData(normalized);
    combo->setCurrentIndex(index >= 0 ? index : 0);
}

static int developerStepFromCombo(const QComboBox *combo)
{
    if (!combo) return 1;
    return normalizeDeveloperStepChoice(combo->currentData().toInt());
}

static int tryIntervalForSegment(int currentMa, int targetMa, int stepMa, int timeSec)
{
    const int step = qMax(1, normalizeDeveloperStepChoice(stepMa));
    const int diff = qAbs(targetMa - currentMa);
    const int steps = qMax(1, (diff + step - 1) / step);
    return qMax(1, (qMax(1, timeSec) * 1000) / steps);
}

Widget::Widget(LaserController *sharedController, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    controller(sharedController ? sharedController : new LaserController(this)),
    ownsController(sharedController == nullptr),
    currentLaser1mA(0),
    currentLaser2mA(0),
    currentLaser3mA(LaserController::L3_SAFE_OFF_MA)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("Laser Control System"));

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    visualRefreshTimer = new QTimer(this);
    visualRefreshTimer->setSingleShot(true);
    visualRefreshTimer->setInterval(0);
    // 多个串口/ramp 信号常在同一轮事件循环内连续到达，这里合并成一次真实刷新，减少 +/- 按钮重绘闪烁。
    connect(visualRefreshTimer, &QTimer::timeout, this, &Widget::doUpdateAllLaserVisuals);

    // 开发者窗口现在只作为视图，串口和安全联锁都由 LaserController 统一维护。
    connect(controller, &LaserController::logMessage, ui->receiveEdit, &QPlainTextEdit::appendPlainText);
    connect(controller, &LaserController::currentChanged, this, [this](int laserIndex, int currentMa) {
        QSpinBox *spin = nullptr;
        LaserChart *chart = nullptr;
        if (laserIndex == 1) { currentLaser1mA = currentMa; spin = ui->laser1spinbox; chart = chart1; }
        else if (laserIndex == 2) { currentLaser2mA = currentMa; spin = ui->laser2spinbox; chart = chart2; }
        else if (laserIndex == 3) { currentLaser3mA = currentMa; spin = ui->laser3spinbox; chart = chart3; }
        if (spin) {
            spin->blockSignals(true);
            spin->setValue(currentMa);
            spin->blockSignals(false);
        }
        if (chart) chart->addDataPoint(currentMa);
        updateAllLaserVisuals();
    });
    connect(controller, &LaserController::measuredChanged, this, [this](int laserIndex, double measuredMa) {
        if (laserIndex == 1) { measuredLaser1mA = measuredMa; chart1->addMeasuredPoint(measuredMa); }
        else if (laserIndex == 2) { measuredLaser2mA = measuredMa; chart2->addMeasuredPoint(measuredMa); }
        else if (laserIndex == 3) { measuredLaser3mA = measuredMa; chart3->addMeasuredPoint(measuredMa); }
        updateLaserVisual(laserIndex);
    });
    connect(controller, &LaserController::readyChanged, this, [this](int laserIndex, bool ready, bool rawReady) {
        if (laserIndex == 1) { laser1Ready = ready; laser1RawReady = rawReady; }
        else if (laserIndex == 2) { laser2Ready = ready; laser2RawReady = rawReady; }
        else if (laserIndex == 3) { laser3Ready = ready; laser3RawReady = rawReady; }
        setLaserReady(laserIndex, ready);
        updateAllLaserVisuals();
    });
    connect(controller, &LaserController::stateChanged, this, &Widget::updateAllLaserVisuals);
    connect(controller, &LaserController::operationStarted, this, [this](int laserIndex, int targetMa) {
        // 目标值现在由控制核心按定时器缓慢推进，开发者页同步显示“正在调节”的限制状态。
        ui->receiveEdit->appendPlainText(QString("[RAMP] L%1 开始调节到 %2 mA").arg(laserIndex).arg(targetMa));
        updateAllLaserVisuals();
    });
    connect(controller, &LaserController::operationProgress, this, [this](int, int, int) {
        updateAllLaserVisuals();
    });
    connect(controller, &LaserController::operationFinished, this, [this](int laserIndex, bool success, const QString &message) {
        ui->receiveEdit->appendPlainText(QString("[RAMP] L%1 %2：%3")
                                         .arg(laserIndex)
                                         .arg(success ? QString::fromUtf8(u8"完成") : QString::fromUtf8(u8"中止"))
                                         .arg(message));
        updateAllLaserVisuals();
    });
    connect(controller, &LaserController::transportChanged, this, [this](bool opened, const QString &) {
        if (opened) {
            ui->openBt->setText(QString::fromUtf8(u8"已连接"));
            ui->openBt->setStyleSheet("QPushButton { background-color: green; color: white; } QPushButton:hover { background-color: #CC5500; }");
        } else {
            ui->openBt->setText(QString::fromUtf8(u8"打开串口"));
            ui->openBt->setStyleSheet("");
        }
        updateAllLaserVisuals();
    });

    refreshSerialPortList();

    chart1 = ui->chartWidget1;
    chart1->setTitle("Laser1");
    // 开发者页面量程跟随配置文件，避免 UI 上限和控制核心上限不一致。
    chart1->setYRange(controller->laserMinMa(1), controller->laserMaxMa(1));
    chart1->setLineColor(COLOR_L1);
    chart1->setMeasuredColor(COLOR_MEASURED);
    chart1->setThreshold(controller->l1EnableL2Ma());

    chart2 = ui->chartWidget2;
    chart2->setTitle("Laser2");
    chart2->setYRange(controller->laserMinMa(2), controller->laserMaxMa(2));
    chart2->setLineColor(COLOR_L2);
    chart2->setMeasuredColor(COLOR_MEASURED);
    chart2->setThreshold(controller->l2EnableL3Ma());

    chart3 = ui->chartWidget3;
    chart3->setTitle("Laser3");
    chart3->setYRange(0, controller->laserMaxMa(3));
    chart3->setLineColor(COLOR_L3);
    chart3->setMeasuredColor(COLOR_MEASURED);
    chart3->setThreshold(0, false);

    // SpinBox 范围由控制核心提供；L3 单步由 STM32 协议固定为约 100 mA。
    ui->laser1spinbox->setRange(controller->laserMinMa(1), controller->laserMaxMa(1));
    ui->laser2spinbox->setRange(controller->laserMinMa(2), controller->laserMaxMa(2));
    ui->laser3spinbox->setRange(controller->laserMinMa(3), controller->laserMaxMa(3));
    ui->laser3spinbox->setSingleStep(controller->laserStepMa(3, true));

    currentLaser1mA = controller->currentLaserMa(1);
    currentLaser2mA = controller->currentLaserMa(2);
    // L3 初始值从控制核心读取；控制核心启动时会按配置文件的 L3SafeOffMa 初始化。
    currentLaser3mA = controller->currentLaserMa(3);
    measuredLaser1mA = controller->measuredLaserMa(1);
    measuredLaser2mA = controller->measuredLaserMa(2);
    measuredLaser3mA = controller->measuredLaserMa(3);

    ui->laser1spinbox->setValue(currentLaser1mA);
    ui->laser2spinbox->setValue(currentLaser2mA);
    ui->laser3spinbox->setValue(currentLaser3mA);

    chart1->addDataPoint(currentLaser1mA);
    chart2->addDataPoint(currentLaser2mA);
    chart3->addDataPoint(currentLaser3mA);

    // L1 开发者手动调节默认使用 1 mA 细调，避免一进入页面就使用较大的 10 mA 步进。
    setLaser1Mode(false);
    setLaser2Mode(true);
    // 开发者 TRY 的默认扫描参数来自配置文件，避免每次打开窗口都回到硬编码初值。
    applyDeveloperRuntimeParams(controller->developerRuntimeParams());

    resetLaserStates();
    updateAllLaserVisuals();

    tryTimer = new QTimer(this);
    tryTimer->setSingleShot(false);
    connect(tryTimer, &QTimer::timeout, this, &Widget::tryStep);

    ui->receiveEdit->appendPlainText(QString::fromUtf8(u8"[INFO] 开发者窗口已连接到共享控制核心"));
}

Widget::~Widget()
{
    delete ui;
}

// ===================== 模式切换 =====================
void Widget::setLaser1Mode(bool coarse)
{
    laser1Coarse = coarse;
    // 粗调/细调步长来自控制核心的协议映射，按钮切换时同步 SpinBox 的键盘步进。
    ui->laser1spinbox->setSingleStep(controller->laserStepMa(1, coarse));
    ui->laser1CoarseBtn->setChecked(coarse);
    ui->laser1FineBtn->setChecked(!coarse);
    QString active = "QPushButton { background-color: #CC5500; color: white; }";
    ui->laser1CoarseBtn->setStyleSheet(coarse ? active : "");
    ui->laser1FineBtn->setStyleSheet(coarse ? "" : active);
}

void Widget::setLaser2Mode(bool coarse)
{
    laser2Coarse = coarse;
    // 粗调/细调步长来自控制核心的协议映射，按钮切换时同步 SpinBox 的键盘步进。
    ui->laser2spinbox->setSingleStep(controller->laserStepMa(2, coarse));
    ui->laser2CoarseBtn->setChecked(coarse);
    ui->laser2FineBtn->setChecked(!coarse);
    QString active = "QPushButton { background-color: #CC5500; color: white; }";
    ui->laser2CoarseBtn->setStyleSheet(coarse ? active : "");
    ui->laser2FineBtn->setStyleSheet(coarse ? "" : active);
}

void Widget::applyDeveloperRuntimeParams(const LaserController::DeveloperRuntimeParams &params)
{
    // TRY 前段直接使用 StartupL1/StartupL2，和普通操作员按钮启动曲线保持同源。
    tryL1HighCurrent = params.startupL1HighMa;
    tryL1MiddleCurrent = params.startupL1MiddleMa;
    tryPhase1TimeSec = params.startupL1Phase1TimeSec;
    tryPhase2TimeSec = params.startupL1Phase2TimeSec;
    tryPhase3TimeSec = params.startupL1Phase3TimeSec;
    tryStepSize = params.startupL1StepMa;
    tryFinalCurrent = params.operatorL1FinalMa;
    tryL2HighCurrent = params.startupL2HighMa;
    tryL2MiddleCurrent = params.startupL2MiddleMa;
    tryL2FinalCurrent = params.operatorL2FinalMa;
    tryL2Phase1TimeSec = params.startupL2Phase1TimeSec;
    tryL2Phase2TimeSec = params.startupL2Phase2TimeSec;
    tryL2Phase3TimeSec = params.startupL2Phase3TimeSec;
    tryL2StartupStepSize = params.startupL2StepMa;
    tryL2TargetMA = params.tryL2TargetMa;
    tryL2StepSize = params.tryL2StepMa;
    tryL2TimeSec = params.tryL2TimeSec;
    tryL3TargetMA = params.tryL3TargetMa;
    tryL3StepSize = params.tryL3StepMa;
    tryL3TimeSec = params.tryL3TimeSec;
}

void Widget::on_laser1CoarseBtn_clicked() { setLaser1Mode(true); }
void Widget::on_laser1FineBtn_clicked()   { setLaser1Mode(false); }
void Widget::on_laser2CoarseBtn_clicked() { setLaser2Mode(true); }
void Widget::on_laser2FineBtn_clicked()   { setLaser2Mode(false); }

// ===================== 状态管理 =====================
void Widget::resetLaserStates()
{
    // 状态复位由控制核心负责；开发者窗口只按当前控制核心状态重绘。
    setLaserReady(1, controller->laserReady(1));
    setLaserReady(2, controller->laserReady(2));
    setLaserReady(3, controller->laserReady(3));
    updateAllLaserVisuals();
}

bool Widget::hasLaserTransport() const
{
    return controller->hasLaserTransport();
}

bool Widget::canAdjustLaser(int laserIndex, int direction) const
{
    return controller->canAdjustLaser(laserIndex, direction);
}

QString Widget::adjustBlockReason(int laserIndex, int direction) const
{
    return controller->adjustBlockReason(laserIndex, direction);
}

QString Widget::blockReason(int laserIndex) const
{
    return adjustBlockReason(laserIndex, +1);
}

// ===================== 串口管理 =====================
void Widget::refreshSerialPortList()
{
    QString currentSelection = ui->serialCb->currentText();
    ui->serialCb->clear();
    foreach (const QString &portName, controller->availablePortNames()) {
        ui->serialCb->addItem(portName);
    }
    int index = ui->serialCb->findText(currentSelection);
    if (index != -1) ui->serialCb->setCurrentIndex(index);
}

void Widget::on_openBt_clicked()
{
    // 打开/关闭串口不再由界面直接操作 QSerialPort，而是交给控制核心。
    if (controller->isSerialOpen()) {
        controller->closeSerial();
        return;
    }

    QString portName = ui->serialCb->currentText();
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择串口");
        return;
    }
    if (!controller->openSerial(portName))
        QMessageBox::critical(this, "错误", "无法打开串口，请查看日志。");
}

void Widget::on_closeBt_clicked()
{
    controller->closeSerial();
}

// ===================== 发送 =====================

void Widget::setLaserReady(int laserIndex, bool ready)
{
    QLabel *status = nullptr;
    QLabel *led = nullptr;
    if (laserIndex == 1)      { status = ui->laser1StatusLabel; led = ui->ledLabel1; }
    else if (laserIndex == 2) { status = ui->laser2StatusLabel; led = ui->ledLabel2; }
    else if (laserIndex == 3) { status = ui->laser3StatusLabel; led = ui->ledLabel3; }
    if (!status) return;

    if (ready) {
        status->setText("就绪");
        status->setStyleSheet("color:#40FF40;font-weight:bold;");
        if (led) led->setStyleSheet("color:#40FF40; font-size:18px; font-weight:bold;");
    } else {
        QString reason = blockReason(laserIndex);
        if (reason.contains("等待")) {
            status->setText("等待依赖");
            status->setStyleSheet("color:#FFC840;font-weight:bold;");
            if (led) led->setStyleSheet("color:#FFC840; font-size:18px; font-weight:bold;");
        } else {
            status->setText("未就绪");
            status->setStyleSheet("color:#FF6060;font-weight:bold;");
            if (led) led->setStyleSheet("color:#FF4040; font-size:18px; font-weight:bold;");
        }
    }
}

// ===================== 可视化刷新 =====================
void Widget::updateAllLaserVisuals()
{
    if (!visualRefreshTimer) {
        doUpdateAllLaserVisuals();
        return;
    }

    // 同一条串口发送或 ramp 步进会连续触发多个信号；只排队一次，避免按钮在密集重绘中闪烁。
    if (!visualRefreshTimer->isActive()) {
        visualRefreshTimer->start();
    }
}

void Widget::doUpdateAllLaserVisuals()
{
    // 真正刷新三路联锁状态。调度层会把同一轮事件循环里的重复请求合并到这里。
    updateLaserVisual(1);
    updateLaserVisual(2);
    updateLaserVisual(3);

    const bool canEditParams = canEditDeveloperParams();
    // 每路参数独立保存；ramp/TRY 运行中禁止写入参数，避免保存中间态。
    if (ui->laser1ParamsButton) ui->laser1ParamsButton->setEnabled(canEditParams);
    if (ui->laser2ParamsButton) ui->laser2ParamsButton->setEnabled(canEditParams);
    if (ui->laser3ParamsButton) ui->laser3ParamsButton->setEnabled(canEditParams);
    updateTemperatureBypassUi();
}

void Widget::updateLaserVisual(int laserIndex)
{
    QLabel *big = nullptr;
    QLabel *measured = nullptr;
    QLabel *reason = nullptr;
    QPushButton *upBtn = nullptr, *downBtn = nullptr;
    QSpinBox *spin = nullptr;
    int cur = 0;
    double measuredVal = -1;

    switch (laserIndex) {
    case 1:
        big = ui->bigCurrent1; measured = ui->measuredLabel1; reason = ui->reasonLabel1;
        upBtn = ui->laser1UpBtn; downBtn = ui->laser1DownBtn; spin = ui->laser1spinbox;
        cur = controller->currentLaserMa(1); measuredVal = controller->measuredLaserMa(1);
        break;
    case 2:
        big = ui->bigCurrent2; measured = ui->measuredLabel2; reason = ui->reasonLabel2;
        upBtn = ui->laser2UpBtn; downBtn = ui->laser2DownBtn; spin = ui->laser2spinbox;
        cur = controller->currentLaserMa(2); measuredVal = controller->measuredLaserMa(2);
        break;
    case 3:
        big = ui->bigCurrent3; measured = ui->measuredLabel3; reason = ui->reasonLabel3;
        upBtn = ui->laser3UpBtn; downBtn = ui->laser3DownBtn; spin = ui->laser3spinbox;
        cur = controller->currentLaserMa(3); measuredVal = controller->measuredLaserMa(3);
        break;
    default: return;
    }

    if (big) big->setText(QString::number(cur));

    if (measured) {
        if (measuredVal < 0) measured->setText("实测: --");
        else measured->setText(QString("实测: %1 mA").arg(measuredVal, 0, 'f', 0));
    }

    // 缓升/缓降或 TRY 扫描期间禁止手动插入新命令，避免控制流程被打断。
    const bool anyBusy = controller->isAnyLaserBusy();
    const bool thisBusy = controller->isLaserBusy(laserIndex);
    const bool tryRunning = (tryState != TryIdle);
    const bool manualLocked = anyBusy || tryRunning;

    // 上调和下调分别判断：开机按 L1->L2->L3，关机按 L3->L2->L1。
    // TRY 扫描运行中也保持锁定，避免每段 ramp 间隙里按钮短暂变亮造成闪烁。
    bool canUp = !manualLocked && canAdjustLaser(laserIndex, +1);
    bool canDown = !manualLocked && canAdjustLaser(laserIndex, -1);
    if (upBtn)   upBtn->setEnabled(canUp);
    if (downBtn) downBtn->setEnabled(canDown);
    if (spin) {
        // 非调节期 SpinBox 可输入；ramp/TRY 进行中先锁住，避免覆盖尚未完成的目标。
        spin->setEnabled(hasLaserTransport() && !manualLocked);
        spin->setKeyboardTracking(false);
    }

    QString why;
    if (thisBusy) {
        why = QString::fromUtf8(u8"正在缓升/缓降，请等待完成");
    } else if (anyBusy) {
        why = QString::fromUtf8(u8"其它通道正在缓升/缓降，请等待完成");
    } else if (tryRunning) {
        // TRY 正在自动扫描时，开发者手动控件保持锁定，防止人工命令插入扫描流程。
        why = QString::fromUtf8(u8"TRY 扫描进行中，请等待扫描完成");
    } else if (!canUp && !canDown) {
        why = adjustBlockReason(laserIndex, +1);
    } else if (!canUp) {
        why = QString("上调受限：%1").arg(adjustBlockReason(laserIndex, +1));
    } else if (!canDown) {
        why = QString("下调受限：%1").arg(adjustBlockReason(laserIndex, -1));
    }

    if (reason) reason->setText(why);
}

// ===================== 步进按钮 =====================
void Widget::adjustLaser(int laserIndex, int direction)
{
    bool coarse = (laserIndex == 1) ? laser1Coarse : (laserIndex == 2 ? laser2Coarse : true);
    // 开发者页面的按钮也只请求控制核心执行，避免绕过普通页面共用的安全联锁。
    controller->adjustLaser(laserIndex, direction, coarse);
    updateAllLaserVisuals();
}

void Widget::on_laser1UpBtn_clicked()   { adjustLaser(1, +1); }
void Widget::on_laser1DownBtn_clicked() { adjustLaser(1, -1); }
void Widget::on_laser2UpBtn_clicked()   { adjustLaser(2, +1); }
void Widget::on_laser2DownBtn_clicked() { adjustLaser(2, -1); }
void Widget::on_laser3UpBtn_clicked()   { adjustLaser(3, +1); }
void Widget::on_laser3DownBtn_clicked() { adjustLaser(3, -1); }

// 直接编辑 spinbox 后，按目标差值连续发指令
void Widget::on_laser1spinbox_editingFinished()
{
    int target = ui->laser1spinbox->value();
    // SpinBox 直接输入目标值时交给控制核心执行，失败则回滚到控制核心记录的真实设定值。
    if (!controller->setLaserTarget(1, target, laser1Coarse)) {
        ui->laser1spinbox->blockSignals(true);
        ui->laser1spinbox->setValue(controller->currentLaserMa(1));
        ui->laser1spinbox->blockSignals(false);
    }
    updateAllLaserVisuals();
}

void Widget::on_laser2spinbox_editingFinished()
{
    int target = ui->laser2spinbox->value();
    // SpinBox 直接输入目标值时交给控制核心执行，失败则回滚到控制核心记录的真实设定值。
    if (!controller->setLaserTarget(2, target, laser2Coarse)) {
        ui->laser2spinbox->blockSignals(true);
        ui->laser2spinbox->setValue(controller->currentLaserMa(2));
        ui->laser2spinbox->blockSignals(false);
    }
    updateAllLaserVisuals();
}

void Widget::on_laser3spinbox_editingFinished()
{
    int target = ui->laser3spinbox->value();
    const int step = controller->laserStepMa(3, true);
    const int minMa = controller->laserMinMa(3);
    // L3 手动输入按硬件固定步长对齐，目前为 100 mA。
    target = minMa + ((target - minMa + step / 2) / step) * step;
    target = qBound(minMa, target, controller->laserMaxMa(3));
    // L3 的最低值来自配置，目标对齐后仍由控制核心统一处理。
    if (!controller->setLaserTarget(3, target, true)) {
        ui->laser3spinbox->blockSignals(true);
        ui->laser3spinbox->setValue(controller->currentLaserMa(3));
        ui->laser3spinbox->blockSignals(false);
    }
    updateAllLaserVisuals();
}

void Widget::on_sendBt_clicked()
{
    ui->receiveEdit->appendPlainText("[INFO] 发送功能暂未开放");
}

void Widget::on_clearBt_clicked()
{
    ui->receiveEdit->clear();
}

bool Widget::canEditDeveloperParams() const
{
    return controller && tryState == TryIdle && !controller->isAnyLaserBusy();
}

bool Widget::saveLaserParamsFromDialog(int laserIndex, const LaserController::DeveloperRuntimeParams &params)
{
    QString error;
    if (!controller->saveDeveloperLaserParameters(laserIndex, params, &error)) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"保存失败"),
                             error.isEmpty() ? QString::fromUtf8(u8"参数写入配置文件失败。") : error);
        return false;
    }

    // 写入后重新读取控制核心归一化后的值，确保后续 TRY 使用最终生效配置。
    applyDeveloperRuntimeParams(controller->developerRuntimeParams());
    updateAllLaserVisuals();
    ui->receiveEdit->appendPlainText(QString::fromUtf8(u8"[CONFIG] L%1 参数已保存，下次启动会自动读取。").arg(laserIndex));
    QMessageBox::information(this,
                             QString::fromUtf8(u8"保存完成"),
                             QString::fromUtf8(u8"L%1 参数已写入配置文件，并已生成备份文件。").arg(laserIndex));
    return true;
}

void Widget::updateTemperatureBypassUi()
{
    if (!ui->temperatureBypassButton || !controller) return;

    const bool bypass = controller->temperatureReadyBypassEnabled();
    ui->temperatureBypassButton->setText(bypass
        ? QString::fromUtf8(u8"温度旁路: 开启")
        : QString::fromUtf8(u8"温度旁路: 关闭"));
    // 旁路开关只在静止状态允许切换，避免运行中突然改变联锁判断。
    ui->temperatureBypassButton->setEnabled(canEditDeveloperParams());
    ui->temperatureBypassButton->setToolTip(bypass
        ? QString::fromUtf8(u8"当前已忽略上位机温度 ready，仅保留顺序联锁；最终温度保护依赖 STM32")
        : QString::fromUtf8(u8"开启后将临时忽略上位机温度 ready，仅用于下位机暂不上传 ready 的场景"));
    ui->temperatureBypassButton->setStyleSheet(bypass
        ? "QPushButton { background-color: #D69A1E; color: white; font-weight: bold; } "
          "QPushButton:hover { background-color: #B77D12; color: white; } "
          "QPushButton:disabled { background-color: #8B6A24; color: #E8E0C8; }"
        : "");

    if (ui->developerTemperatureBypassWarningLabel) {
        // 温度旁路属于安全风险提示，单独放在日志区下方；平时隐藏，开启时固定显示。
        const QString warningText = QString::fromUtf8(
            u8"警告：温度就绪旁路已开启。上位机将忽略下位机温度就绪状态，仅保留顺序联锁；最终温度保护依赖下位机。");
        ui->developerTemperatureBypassWarningLabel->setText(warningText);
        ui->developerTemperatureBypassWarningLabel->setToolTip(warningText);
        ui->developerTemperatureBypassWarningLabel->setVisible(bypass);
    }
}

void Widget::on_temperatureBypassButton_clicked()
{
    if (!canEditDeveloperParams()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"暂不能切换"),
                             QString::fromUtf8(u8"请先停止 TRY 扫描，并等待当前缓升/缓降完成。"));
        updateTemperatureBypassUi();
        return;
    }

    const bool targetEnabled = !controller->temperatureReadyBypassEnabled();
    if (targetEnabled) {
        const int ret = QMessageBox::warning(
            this,
            QString::fromUtf8(u8"开启温度旁路"),
            QString::fromUtf8(u8"当前将忽略上位机接收到的温度就绪状态。\n\n"
                              u8"上位机只执行 L1 -> L2 -> L3 顺序联锁和关机顺序联锁。\n"
                              u8"最终温度保护依赖 STM32 下位机。\n\n"
                              u8"该模式只用于下位机暂不上传温度 ready 的临时场景，是否继续？"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);
        if (ret != QMessageBox::Yes) {
            updateTemperatureBypassUi();
            return;
        }
    }

    QString error;
    if (!controller->setTemperatureReadyBypassEnabled(targetEnabled, &error)) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"切换失败"),
                             error.isEmpty() ? QString::fromUtf8(u8"温度旁路状态写入失败。") : error);
        updateTemperatureBypassUi();
        return;
    }

    ui->receiveEdit->appendPlainText(targetEnabled
        ? QString::fromUtf8(u8"[WARN] 温度旁路已开启：上位机未验证温度就绪，最终保护依赖下位机")
        : QString::fromUtf8(u8"[INFO] 温度旁路已关闭：上位机重新要求下位机 ready"));
    updateAllLaserVisuals();
}

void Widget::on_laser1ParamsButton_clicked()
{
    if (!canEditDeveloperParams()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"暂不能修改"),
                             QString::fromUtf8(u8"请先停止 TRY 扫描，并等待当前缓升/缓降完成。"));
        return;
    }

    LaserController::DeveloperRuntimeParams params = controller->developerRuntimeParams();

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8(u8"L1 参数设置"));
    QFormLayout *form = new QFormLayout(&dlg);

    QSpinBox *highMa = new QSpinBox(&dlg);
    highMa->setRange(controller->laserMinMa(1), controller->laserMaxMa(1));
    highMa->setValue(params.startupL1HighMa);
    highMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"启动高点电流"), highMa);

    QSpinBox *middleMa = new QSpinBox(&dlg);
    middleMa->setRange(controller->laserMinMa(1), controller->laserMaxMa(1));
    middleMa->setValue(params.startupL1MiddleMa);
    middleMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"启动中间电流"), middleMa);

    QSpinBox *finalMa = new QSpinBox(&dlg);
    finalMa->setRange(controller->l1EnableL2Ma(), controller->laserMaxMa(1));
    finalMa->setValue(params.operatorL1FinalMa);
    finalMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"普通页面最终工作电流"), finalMa);

    QSpinBox *phase1Time = new QSpinBox(&dlg);
    phase1Time->setRange(1, 9999);
    phase1Time->setValue(params.startupL1Phase1TimeSec);
    phase1Time->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"TRY 阶段1时间"), phase1Time);

    QSpinBox *phase2Time = new QSpinBox(&dlg);
    phase2Time->setRange(1, 9999);
    phase2Time->setValue(params.startupL1Phase2TimeSec);
    phase2Time->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"TRY 阶段2时间"), phase2Time);

    QSpinBox *phase3Time = new QSpinBox(&dlg);
    phase3Time->setRange(1, 9999);
    phase3Time->setValue(params.startupL1Phase3TimeSec);
    phase3Time->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"TRY 阶段3时间"), phase3Time);

    QComboBox *stepMa = new QComboBox(&dlg);
    // L1 TRY 步长只允许选择 STM32 已定义的细调 1 mA 或粗调 10 mA。
    setupDeveloperStepCombo(stepMa, params.startupL1StepMa);
    form->addRow(QString::fromUtf8(u8"TRY 步长"), stepMa);

    QSpinBox *tryFinalMa = new QSpinBox(&dlg);
    tryFinalMa->setRange(controller->laserMinMa(1), controller->laserMaxMa(1));
    tryFinalMa->setValue(params.operatorL1FinalMa);
    tryFinalMa->setEnabled(false);
    tryFinalMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"TRY 最终电流"), tryFinalMa);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    // 参数弹窗的确认动作会写入配置文件，用“保存”比默认 OK 更明确。
    if (QPushButton *saveButton = btnBox->button(QDialogButtonBox::Ok)) saveButton->setText(QString::fromUtf8(u8"保存"));
    form->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    params.operatorL1FinalMa = finalMa->value();
    params.startupL1HighMa = highMa->value();
    params.startupL1MiddleMa = middleMa->value();
    params.startupL1Phase1TimeSec = phase1Time->value();
    params.startupL1Phase2TimeSec = phase2Time->value();
    params.startupL1Phase3TimeSec = phase3Time->value();
    params.startupL1StepMa = developerStepFromCombo(stepMa);
    // 兼容旧字段：L1 TRY 最终点等于统一启动最终工作电流。
    params.tryL1Phase1TimeSec = params.startupL1Phase1TimeSec;
    params.tryL1Phase2TimeSec = params.startupL1Phase2TimeSec;
    params.tryL1Phase3TimeSec = params.startupL1Phase3TimeSec;
    params.tryL1StepMa = params.startupL1StepMa;
    params.tryL1FinalMa = params.operatorL1FinalMa;
    saveLaserParamsFromDialog(1, params);
}

void Widget::on_laser2ParamsButton_clicked()
{
    if (!canEditDeveloperParams()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"暂不能修改"),
                             QString::fromUtf8(u8"请先停止 TRY 扫描，并等待当前缓升/缓降完成。"));
        return;
    }

    LaserController::DeveloperRuntimeParams params = controller->developerRuntimeParams();

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8(u8"L2 参数设置"));
    QFormLayout *form = new QFormLayout(&dlg);

    QSpinBox *highMa = new QSpinBox(&dlg);
    highMa->setRange(controller->laserMinMa(2), controller->laserMaxMa(2));
    highMa->setValue(params.startupL2HighMa);
    highMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"启动高点电流"), highMa);

    QSpinBox *middleMa = new QSpinBox(&dlg);
    middleMa->setRange(controller->laserMinMa(2), controller->laserMaxMa(2));
    middleMa->setValue(params.startupL2MiddleMa);
    middleMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"启动中间电流"), middleMa);

    QSpinBox *finalMa = new QSpinBox(&dlg);
    finalMa->setRange(controller->l2EnableL3Ma(), controller->laserMaxMa(2));
    finalMa->setValue(params.operatorL2FinalMa);
    finalMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"普通页面最终工作电流"), finalMa);

    QSpinBox *phase1Time = new QSpinBox(&dlg);
    phase1Time->setRange(1, 9999);
    phase1Time->setValue(params.startupL2Phase1TimeSec);
    phase1Time->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"启动阶段1时间"), phase1Time);

    QSpinBox *phase2Time = new QSpinBox(&dlg);
    phase2Time->setRange(1, 9999);
    phase2Time->setValue(params.startupL2Phase2TimeSec);
    phase2Time->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"启动阶段2时间"), phase2Time);

    QSpinBox *phase3Time = new QSpinBox(&dlg);
    phase3Time->setRange(1, 9999);
    phase3Time->setValue(params.startupL2Phase3TimeSec);
    phase3Time->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"启动阶段3时间"), phase3Time);

    QComboBox *startupStepMa = new QComboBox(&dlg);
    // L2 启动步长只允许 1 mA 或 10 mA，TRY 额外扫描步长在下面单独设置。
    setupDeveloperStepCombo(startupStepMa, params.startupL2StepMa);
    form->addRow(QString::fromUtf8(u8"启动步长"), startupStepMa);

    QSpinBox *targetMa = new QSpinBox(&dlg);
    targetMa->setRange(controller->laserMinMa(2), controller->laserMaxMa(2));
    targetMa->setValue(params.tryL2TargetMa);
    targetMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"TRY 目标电流"), targetMa);

    QComboBox *stepMa = new QComboBox(&dlg);
    // L2 TRY 步长只允许选择 STM32 已定义的细调 1 mA 或粗调 10 mA。
    setupDeveloperStepCombo(stepMa, params.tryL2StepMa);
    form->addRow(QString::fromUtf8(u8"TRY 步长"), stepMa);

    QSpinBox *timeSec = new QSpinBox(&dlg);
    timeSec->setRange(1, 9999);
    timeSec->setValue(params.tryL2TimeSec);
    timeSec->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"TRY 时间"), timeSec);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    // 参数弹窗的确认动作会写入配置文件，用“保存”比默认 OK 更明确。
    if (QPushButton *saveButton = btnBox->button(QDialogButtonBox::Ok)) saveButton->setText(QString::fromUtf8(u8"保存"));
    form->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    params.operatorL2FinalMa = finalMa->value();
    params.startupL2HighMa = highMa->value();
    params.startupL2MiddleMa = middleMa->value();
    params.startupL2Phase1TimeSec = phase1Time->value();
    params.startupL2Phase2TimeSec = phase2Time->value();
    params.startupL2Phase3TimeSec = phase3Time->value();
    params.startupL2StepMa = developerStepFromCombo(startupStepMa);
    params.tryL2TargetMa = targetMa->value();
    params.tryL2StepMa = developerStepFromCombo(stepMa);
    params.tryL2TimeSec = timeSec->value();
    saveLaserParamsFromDialog(2, params);
}

void Widget::on_laser3ParamsButton_clicked()
{
    if (!canEditDeveloperParams()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"暂不能修改"),
                             QString::fromUtf8(u8"请先停止 TRY 扫描，并等待当前缓升/缓降完成。"));
        return;
    }

    LaserController::DeveloperRuntimeParams params = controller->developerRuntimeParams();
    const int l3Step = controller->laserStepMa(3, true);

    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8(u8"L3 参数设置"));
    QFormLayout *form = new QFormLayout(&dlg);

    QSpinBox *operatorMaxMa = new QSpinBox(&dlg);
    operatorMaxMa->setRange(controller->laserMinMa(3) + l3Step, controller->laserMaxMa(3));
    operatorMaxMa->setSingleStep(l3Step);
    operatorMaxMa->setValue(params.l3OperatorMaxMa);
    operatorMaxMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"普通页面 100% 电流"), operatorMaxMa);

    QSpinBox *targetMa = new QSpinBox(&dlg);
    targetMa->setRange(controller->laserMinMa(3), controller->laserMaxMa(3));
    targetMa->setSingleStep(l3Step);
    targetMa->setValue(params.tryL3TargetMa);
    targetMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"TRY 目标电流"), targetMa);

    QSpinBox *stepMa = new QSpinBox(&dlg);
    // L3 串口只有 '8'/'9' 两条固定约 100 mA 指令，开发者页只显示固定步长，不允许改成 1/10 mA。
    stepMa->setRange(l3Step, l3Step);
    stepMa->setSingleStep(l3Step);
    stepMa->setValue(params.tryL3StepMa);
    stepMa->setEnabled(false);
    stepMa->setSuffix(QString::fromUtf8(u8" mA"));
    form->addRow(QString::fromUtf8(u8"TRY 步长（固定）"), stepMa);

    QSpinBox *timeSec = new QSpinBox(&dlg);
    timeSec->setRange(1, 9999);
    timeSec->setValue(params.tryL3TimeSec);
    timeSec->setSuffix(QString::fromUtf8(u8" 秒"));
    form->addRow(QString::fromUtf8(u8"TRY 时间"), timeSec);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    // 参数弹窗的确认动作会写入配置文件，用“保存”比默认 OK 更明确。
    if (QPushButton *saveButton = btnBox->button(QDialogButtonBox::Ok)) saveButton->setText(QString::fromUtf8(u8"保存"));
    form->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    if (dlg.exec() != QDialog::Accepted) {
        return;
    }

    params.l3OperatorMaxMa = operatorMaxMa->value();
    params.tryL3TargetMa = targetMa->value();
    params.tryL3StepMa = l3Step;
    params.tryL3TimeSec = timeSec->value();
    saveLaserParamsFromDialog(3, params);
}

// ===================== TRY 扫描 =====================
void Widget::on_tryButton_clicked()
{
    if (tryState != TryIdle) {
        tryTimer->stop();
        tryState = TryIdle;
        ui->tryButton->setText("全段扫描 (TRY)");
        ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
        ui->receiveEdit->appendPlainText("[TRY] 扫描已手动停止，当前电流: " + QString::number(tryCurrent) + " mA");
        updateAllLaserVisuals();
        return;
    }

    if (!controller->hasLaserTransport()) {
        QMessageBox::warning(this, "错误", "请先打开串口");
        return;
    }

    QDialog dlg(this);
    dlg.setWindowTitle("扫描参数设置");
    QFormLayout *form = new QFormLayout(&dlg);

    QSpinBox *phase1Time = new QSpinBox(&dlg);
    phase1Time->setRange(1, 9999);
    phase1Time->setValue(tryPhase1TimeSec);
    phase1Time->setSuffix(" 秒");
    form->addRow(QString("L1 阶段1 (当前->%1mA):").arg(tryL1HighCurrent), phase1Time);

    QSpinBox *phase2Time = new QSpinBox(&dlg);
    phase2Time->setRange(1, 9999);
    phase2Time->setValue(tryPhase2TimeSec);
    phase2Time->setSuffix(" 秒");
    form->addRow(QString("L1 阶段2 (%1->%2mA):").arg(tryL1HighCurrent).arg(tryL1MiddleCurrent), phase2Time);

    QSpinBox *phase3Time = new QSpinBox(&dlg);
    phase3Time->setRange(1, 9999);
    phase3Time->setValue(tryPhase3TimeSec);
    phase3Time->setSuffix(" 秒");
    form->addRow(QString("L1 阶段3 (%1->最终):").arg(tryL1MiddleCurrent), phase3Time);

    QComboBox *stepSize = new QComboBox(&dlg);
    // TRY 扫描中的 L1 步长同样只能对应 1 mA 细调或 10 mA 粗调。
    setupDeveloperStepCombo(stepSize, tryStepSize);
    form->addRow("L1 步长:", stepSize);

    QSpinBox *finalCurrent = new QSpinBox(&dlg);
    finalCurrent->setRange(controller->l1EnableL2Ma(), controller->laserMaxMa(1));
    finalCurrent->setValue(tryFinalCurrent);
    finalCurrent->setSuffix(" mA");
    form->addRow("L1 最终电流:", finalCurrent);

    QSpinBox *l2Target = new QSpinBox(&dlg);
    l2Target->setRange(tryL2FinalCurrent, controller->laserMaxMa(2));
    l2Target->setValue(tryL2TargetMA);
    l2Target->setSuffix(" mA");
    form->addRow("L2 终点 (0→):", l2Target);

    QComboBox *l2Step = new QComboBox(&dlg);
    // TRY 扫描中的 L2 步长同样只能对应 1 mA 细调或 10 mA 粗调。
    setupDeveloperStepCombo(l2Step, tryL2StepSize);
    form->addRow("L2 步长:", l2Step);

    QSpinBox *l2Time = new QSpinBox(&dlg);
    l2Time->setRange(1, 9999);
    l2Time->setValue(tryL2TimeSec);
    l2Time->setSuffix(" 秒");
    form->addRow("L2 扫描时长:", l2Time);

    QSpinBox *l3Target = new QSpinBox(&dlg);
    l3Target->setRange(800, 10000);
    l3Target->setSingleStep(100);
    l3Target->setValue(tryL3TargetMA);
    l3Target->setSuffix(" mA");
    form->addRow("L3 终点 (800→):", l3Target);

    QSpinBox *l3Step = new QSpinBox(&dlg);
    const int l3FixedStep = controller->laserStepMa(3, true);
    // L3 与 STM32 协议对齐，单步固定约 100 mA，这里只显示不允许编辑。
    l3Step->setRange(l3FixedStep, l3FixedStep);
    l3Step->setSingleStep(l3FixedStep);
    l3Step->setValue(tryL3StepSize);
    l3Step->setEnabled(false);
    l3Step->setSuffix(" mA");
    form->addRow("L3 步长 (固定):", l3Step);

    QSpinBox *l3Time = new QSpinBox(&dlg);
    l3Time->setRange(1, 9999);
    l3Time->setValue(tryL3TimeSec);
    l3Time->setSuffix(" 秒");
    form->addRow("L3 扫描时长:", l3Time);

    QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
    form->addRow(btnBox);
    connect(btnBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btnBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() != QDialog::Accepted) return;

    tryPhase1TimeSec = phase1Time->value();
    tryPhase2TimeSec = phase2Time->value();
    tryPhase3TimeSec = phase3Time->value();
    tryStepSize = developerStepFromCombo(stepSize);
    tryFinalCurrent = finalCurrent->value();
    tryL2TargetMA = l2Target->value();
    tryL2StepSize = developerStepFromCombo(l2Step);
    tryL2TimeSec = l2Time->value();
    // L3 目标和步长都按固定 100 mA 对齐，避免界面参数和 '8'/'9' 指令含义不一致。
    tryL3TargetMA = controller->laserMinMa(3)
            + ((l3Target->value() - controller->laserMinMa(3) + l3FixedStep / 2) / l3FixedStep) * l3FixedStep;
    tryL3StepSize = l3FixedStep;
    tryL3TimeSec = l3Time->value();

    // TRY 起始阶段可能先升 L1，也可能从高电流开始降 L1，按实际第一步方向做联锁判断。
    int initialDirection = (controller->currentLaserMa(1) < tryL1HighCurrent) ? +1 : -1;
    if (!canAdjustLaser(1, initialDirection)) {
        QMessageBox::warning(this, "错误", QString("TRY 无法启动：%1").arg(adjustBlockReason(1, initialDirection)));
        return;
    }

    tryState = TryPhase1;
    tryCurrent = controller->currentLaserMa(1);

    int remain1 = tryL1HighCurrent - tryCurrent;
    if (remain1 <= 0) {
        tryState = TryPhase2;
        int remain2 = tryCurrent - tryL1MiddleCurrent;
        if (remain2 <= 0) tryState = TryPhase3;
    }

    int numSteps = 0;
    int intervalMs = 1000;
    switch (tryState) {
    case TryPhase1:
        intervalMs = tryIntervalForSegment(tryCurrent, tryL1HighCurrent, tryStepSize, tryPhase1TimeSec);
        break;
    case TryPhase2:
        intervalMs = tryIntervalForSegment(tryCurrent, tryL1MiddleCurrent, tryStepSize, tryPhase2TimeSec);
        break;
    case TryPhase3:
        intervalMs = tryIntervalForSegment(tryCurrent, tryFinalCurrent, tryStepSize, tryPhase3TimeSec);
        break;
    default: break;
    }

    ui->tryButton->setText("停止扫描");
    ui->tryButton->setStyleSheet("QPushButton { background-color: #F44336; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
    ui->receiveEdit->appendPlainText(QString("[TRY] 开始扫描 | 阶段1:%1s 阶段2:%2s 阶段3:%3s | 步长:%4mA 最终:%5mA")
        .arg(tryPhase1TimeSec).arg(tryPhase2TimeSec).arg(tryPhase3TimeSec).arg(tryStepSize).arg(tryFinalCurrent));
    tryTimer->start(intervalMs);
}

void Widget::tryStep()
{
    if (tryState == TryIdle) return;

    if (!controller->hasLaserTransport()) {
        tryTimer->stop();
        tryState = TryIdle;
        ui->tryButton->setText("全段扫描 (TRY)");
        ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
        ui->receiveEdit->appendPlainText("[TRY] 串口断开，扫描中止");
        updateAllLaserVisuals();
        return;
    }

    if (controller->isAnyLaserBusy()) {
        // TRY 也等待控制核心的缓升/缓降完成后再进入下一步，避免扫描节奏和真实发送节奏脱节。
        return;
    }

    tryCurrent = controller->currentLaserMa(1);
    tryL2Current = controller->currentLaserMa(2);
    tryL3Current = controller->currentLaserMa(3);

    int target = 0;
    int direction = 0;

    switch (tryState) {
    case TryPhase1:
        target = tryL1HighCurrent; direction = 1;
        if (tryCurrent >= target) {
            tryState = TryPhase2;
            int intervalMs = tryIntervalForSegment(tryCurrent, tryL1MiddleCurrent, tryStepSize, tryPhase2TimeSec);
            tryTimer->setInterval(intervalMs);
            ui->receiveEdit->appendPlainText("[TRY] 进入 L1 启动阶段2: " + QString::number(tryCurrent) + " -> " + QString::number(tryL1MiddleCurrent) + " mA");
            return;
        }
        break;
    case TryPhase2:
        target = tryL1MiddleCurrent; direction = -1;
        if (tryCurrent <= target) {
            tryState = TryPhase3;
            int intervalMs = tryIntervalForSegment(tryCurrent, tryFinalCurrent, tryStepSize, tryPhase3TimeSec);
            tryTimer->setInterval(intervalMs);
            ui->receiveEdit->appendPlainText("[TRY] 进入 L1 启动阶段3: " + QString::number(tryCurrent) + " -> " + QString::number(tryFinalCurrent) + " mA");
            return;
        }
        break;
    case TryPhase3:
        target = tryFinalCurrent; direction = -1;
        if (tryCurrent <= target) {
            // L1 启动曲线完成后，L2 不再直接扫描到目标，而是先复用 StartupL2 三段式启动曲线。
            tryL2Current = controller->currentLaserMa(2);
            if (tryL2Current < tryL2HighCurrent) {
                tryState = TryPhaseL2Start1;
                tryTimer->setInterval(tryIntervalForSegment(tryL2Current, tryL2HighCurrent, tryL2StartupStepSize, tryL2Phase1TimeSec));
                ui->receiveEdit->appendPlainText(QString("[TRY] L1 完成，进入 L2 启动阶段1: %1 -> %2 mA")
                    .arg(tryL2Current).arg(tryL2HighCurrent));
            } else if (tryL2Current > tryL2MiddleCurrent) {
                tryState = TryPhaseL2Start2;
                tryTimer->setInterval(tryIntervalForSegment(tryL2Current, tryL2MiddleCurrent, tryL2StartupStepSize, tryL2Phase2TimeSec));
                ui->receiveEdit->appendPlainText(QString("[TRY] L1 完成，进入 L2 启动阶段2: %1 -> %2 mA")
                    .arg(tryL2Current).arg(tryL2MiddleCurrent));
            } else {
                tryState = TryPhaseL2Start3;
                tryTimer->setInterval(tryIntervalForSegment(tryL2Current, tryL2FinalCurrent, tryL2StartupStepSize, tryL2Phase3TimeSec));
                ui->receiveEdit->appendPlainText(QString("[TRY] L1 完成，进入 L2 启动阶段3: %1 -> %2 mA")
                    .arg(tryL2Current).arg(tryL2FinalCurrent));
            }
            return;
        }
        break;
    case TryPhaseL2Start1: {
        if (!canAdjustLaser(2, +1)) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 启动阶段1中止：%1").arg(adjustBlockReason(2, +1)));
            updateAllLaserVisuals();
            return;
        }
        if (tryL2Current >= tryL2HighCurrent) {
            tryState = TryPhaseL2Start2;
            tryTimer->setInterval(tryIntervalForSegment(tryL2Current, tryL2MiddleCurrent, tryL2StartupStepSize, tryL2Phase2TimeSec));
            ui->receiveEdit->appendPlainText(QString("[TRY] 进入 L2 启动阶段2: %1 -> %2 mA")
                .arg(tryL2Current).arg(tryL2MiddleCurrent));
            return;
        }
        if (!controller->setLaserTarget(2, qMin(tryL2Current + tryL2StartupStepSize, tryL2HighCurrent), true)) return;
        updateAllLaserVisuals();
        return;
    }
    case TryPhaseL2Start2: {
        if (!canAdjustLaser(2, -1)) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 启动阶段2中止：%1").arg(adjustBlockReason(2, -1)));
            updateAllLaserVisuals();
            return;
        }
        if (tryL2Current <= tryL2MiddleCurrent) {
            tryState = TryPhaseL2Start3;
            tryTimer->setInterval(tryIntervalForSegment(tryL2Current, tryL2FinalCurrent, tryL2StartupStepSize, tryL2Phase3TimeSec));
            ui->receiveEdit->appendPlainText(QString("[TRY] 进入 L2 启动阶段3: %1 -> %2 mA")
                .arg(tryL2Current).arg(tryL2FinalCurrent));
            return;
        }
        if (!controller->setLaserTarget(2, qMax(tryL2Current - tryL2StartupStepSize, tryL2MiddleCurrent), true)) return;
        updateAllLaserVisuals();
        return;
    }
    case TryPhaseL2Start3: {
        const int l2FinalDirection = (tryL2FinalCurrent > tryL2Current) ? +1 : (tryL2FinalCurrent < tryL2Current ? -1 : 0);
        if (l2FinalDirection == 0) {
            tryState = TryPhaseL2;
            tryTimer->setInterval(tryIntervalForSegment(tryL2Current, tryL2TargetMA, tryL2StepSize, tryL2TimeSec));
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 启动完成，进入 L2 额外扫描: %1 -> %2 mA")
                .arg(tryL2Current).arg(tryL2TargetMA));
            return;
        }
        if (!canAdjustLaser(2, l2FinalDirection)) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 启动阶段3中止：%1").arg(adjustBlockReason(2, l2FinalDirection)));
            updateAllLaserVisuals();
            return;
        }
        if ((l2FinalDirection > 0 && tryL2Current >= tryL2FinalCurrent)
            || (l2FinalDirection < 0 && tryL2Current <= tryL2FinalCurrent)) {
            tryState = TryPhaseL2;
            tryTimer->setInterval(tryIntervalForSegment(tryL2Current, tryL2TargetMA, tryL2StepSize, tryL2TimeSec));
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 启动完成，进入 L2 额外扫描: %1 -> %2 mA")
                .arg(tryL2Current).arg(tryL2TargetMA));
            return;
        }
        const int newVal = (l2FinalDirection > 0)
                ? qMin(tryL2Current + tryL2StartupStepSize, tryL2FinalCurrent)
                : qMax(tryL2Current - tryL2StartupStepSize, tryL2FinalCurrent);
        if (!controller->setLaserTarget(2, newVal, true)) return;
        updateAllLaserVisuals();
        return;
    }
    case TryPhaseL2: {
        // L2 扫描只做升高动作，必须满足 L1 达标且 L3 已处于最低/关闭态。
        if (!canAdjustLaser(2, +1)) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 扫描中止：%1").arg(adjustBlockReason(2, +1)));
            updateAllLaserVisuals();
            return;
        }
        if (tryL2Current >= tryL2TargetMA) {
            // L2 完成 → 转入 L3
            tryState = TryPhaseL3;
            tryL3Current = controller->currentLaserMa(3);
            int diff3 = tryL3TargetMA - tryL3Current;
            int n3 = (diff3 > 0) ? (diff3 / tryL3StepSize) : 0;
            if (n3 <= 0) {
                tryTimer->stop();
                tryState = TryIdle;
                ui->tryButton->setText("全段扫描 (TRY)");
                ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
                ui->receiveEdit->appendPlainText("[TRY] 全部扫描完成");
                updateAllLaserVisuals();
                return;
            }
            tryTimer->setInterval((tryL3TimeSec * 1000) / n3);
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 完成，进入 L3 扫描: %1 → %2 mA (步长 %3, 时长 %4s)")
                .arg(tryL3Current).arg(tryL3TargetMA).arg(tryL3StepSize).arg(tryL3TimeSec));
            return;
        }
        // 走 L2 一步：TRY 也通过控制核心设置目标值，不直接发送串口命令。
        int newVal = qMin(tryL2Current + tryL2StepSize, tryL2TargetMA);
        if (!controller->setLaserTarget(2, newVal, true)) return;
        tryL2Current = newVal;
        ui->laser2spinbox->blockSignals(true);
        ui->laser2spinbox->setValue(controller->currentLaserMa(2));
        ui->laser2spinbox->blockSignals(false);
        // TRY 自动推进 L2 后，L3 的上调许可可能发生变化，统一刷新三路 UI。
        updateAllLaserVisuals();
        return;
    }
    case TryPhaseL3: {
        // L3 扫描只做升高动作，必须在 L1/L2 都达到启动阈值后才允许进入。
        if (!canAdjustLaser(3, +1)) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] L3 扫描中止：%1").arg(adjustBlockReason(3, +1)));
            updateAllLaserVisuals();
            return;
        }
        if (tryL3Current >= tryL3TargetMA) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] 全部扫描完成! L3 终点: %1 mA").arg(tryL3Current));
            updateAllLaserVisuals();
            return;
        }
        int newVal = qMin(tryL3Current + tryL3StepSize, tryL3TargetMA);
        // 走 L3 一步：保持通过控制核心执行，避免绕过顺序联锁。
        if (!controller->setLaserTarget(3, newVal, true)) return;
        tryL3Current = newVal;
        ui->laser3spinbox->blockSignals(true);
        ui->laser3spinbox->setValue(controller->currentLaserMa(3));
        ui->laser3spinbox->blockSignals(false);
        // TRY 自动推进 L3 后，L2/L1 的回退许可可能发生变化，统一刷新三路 UI。
        updateAllLaserVisuals();
        return;
    }
    default: return;
    }

    // TRY 的 L1 阶段也必须遵守方向联锁，避免后级未回到安全态时自动回调 L1。
    if (!canAdjustLaser(1, direction)) {
        tryTimer->stop();
        tryState = TryIdle;
        ui->tryButton->setText("全段扫描 (TRY)");
        ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
        ui->receiveEdit->appendPlainText(QString("[TRY] L1 扫描中止：%1").arg(adjustBlockReason(1, direction)));
        updateAllLaserVisuals();
        return;
    }

    int nextTryCurrent = tryCurrent + direction * tryStepSize;
    if (direction > 0) nextTryCurrent = qMin(nextTryCurrent, target);
    else               nextTryCurrent = qMax(nextTryCurrent, target);

    // L1 阶段也通过控制核心推进目标值，避免 TRY 扫描绕过发送层兜底。
    if (!controller->setLaserTarget(1, nextTryCurrent, true)) return;
    tryCurrent = nextTryCurrent;

    ui->laser1spinbox->blockSignals(true);
    ui->laser1spinbox->setValue(controller->currentLaserMa(1));
    ui->laser1spinbox->blockSignals(false);
    // TRY 自动推进 L1 后，L2 的上调许可可能发生变化，统一刷新三路 UI。
    updateAllLaserVisuals();
}
