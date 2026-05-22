#include "lasercontroller.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSerialPortInfo>
#include <QSettings>
#include <QTextCodec>
#include <QTimer>
#include <QtGlobal>

namespace {
int normalizeStepChoice(int stepMa)
{
    return (qAbs(stepMa - 1) <= qAbs(stepMa - 10)) ? 1 : 10;
}
}

LaserController::LaserController(QObject *parent)
    : QObject(parent),
      serialPort(new QSerialPort(this)),
      serialCheckTimer(new QTimer(this)),
      autoReconnectTimer(new QTimer(this)),
      statusCheckTimer(new QTimer(this)),
      rampTimer(new QTimer(this))
{
    // 程序启动时先确保配置文件存在，再把阈值、步长和缓升时间加载到控制核心。
    saveDefaultConfigIfMissing();
    loadConfig();

    currentLaser3mA = cfg.l3SafeOffMa;
    rampIntervalMs = cfg.defaultRampIntervalMs;
    minSendIntervalMs = cfg.minManualSendIntervalMs;

    serialCheckTimer->start(1000);
    statusCheckTimer->start(3000);
    rampTimer->setSingleShot(false);
    rampTimer->setInterval(rampIntervalMs);

    connect(serialCheckTimer, &QTimer::timeout, this, &LaserController::checkSerialPorts);
    connect(autoReconnectTimer, &QTimer::timeout, this, &LaserController::autoReconnectSerialPort);
    connect(statusCheckTimer, &QTimer::timeout, this, &LaserController::checkLaserStatus);
    connect(rampTimer, &QTimer::timeout, this, &LaserController::processRampStep);
    connect(serialPort, &QSerialPort::readyRead, this, &LaserController::serialPortReadyRead);
    connect(serialPort, &QSerialPort::errorOccurred, this, &LaserController::handleSerialError);

#ifdef DEBUG_MODE
    // Debug 模式默认模拟已具备控制通道，便于普通页面不打开开发者窗口也能走完整联锁流程。
    lastOpenedPortName = QStringLiteral("DEBUG");
    wasOpenedBefore = true;
    laser1RawReady = true;
    laser2RawReady = true;
    laser3RawReady = true;
    updateLaserDependencies();
#endif
}

QString LaserController::configFilePath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("laser_config.ini"));
}

void LaserController::saveDefaultConfigIfMissing() const
{
    const QString path = configFilePath();
    if (QFile::exists(path)) return;

    QSettings s(path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    s.setIniCodec("UTF-8");
#endif

    // 首次运行时生成默认配置，后续只读取用户修改后的 ini，不在这里覆盖。
    LaserConfig defaults;
    s.setValue(QStringLiteral("Interlock/L1EnableL2Ma"), defaults.l1EnableL2Ma);
    s.setValue(QStringLiteral("Interlock/L2EnableL3Ma"), defaults.l2EnableL3Ma);
    s.setValue(QStringLiteral("Interlock/L2SafeOffMa"), defaults.l2SafeOffMa);
    s.setValue(QStringLiteral("Interlock/L3SafeOffMa"), defaults.l3SafeOffMa);

    s.setValue(QStringLiteral("Range/Laser12MaxMa"), defaults.laser12MaxMa);
    s.setValue(QStringLiteral("Range/Laser3MaxMa"), defaults.laser3MaxMa);

    // L1/L2 启动曲线独立保存；普通操作员启动和开发者 TRY 前段共用这两组参数。
    s.setValue(QStringLiteral("StartupL1/HighMa"), defaults.startupL1HighMa);
    s.setValue(QStringLiteral("StartupL1/MiddleMa"), defaults.startupL1MiddleMa);
    s.setValue(QStringLiteral("StartupL1/FinalMa"), defaults.startupL1FinalMa);
    s.setValue(QStringLiteral("StartupL1/RiseDurationMs"), defaults.startupL1RiseDurationMs);
    s.setValue(QStringLiteral("StartupL1/FallMiddleDurationMs"), defaults.startupL1FallMiddleDurationMs);
    s.setValue(QStringLiteral("StartupL1/FallFinalDurationMs"), defaults.startupL1FallFinalDurationMs);
    s.setValue(QStringLiteral("StartupL1/StepMa"), defaults.startupL1StepMa);

    s.setValue(QStringLiteral("StartupL2/HighMa"), defaults.startupL2HighMa);
    s.setValue(QStringLiteral("StartupL2/MiddleMa"), defaults.startupL2MiddleMa);
    s.setValue(QStringLiteral("StartupL2/FinalMa"), defaults.startupL2FinalMa);
    s.setValue(QStringLiteral("StartupL2/RiseDurationMs"), defaults.startupL2RiseDurationMs);
    s.setValue(QStringLiteral("StartupL2/FallMiddleDurationMs"), defaults.startupL2FallMiddleDurationMs);
    s.setValue(QStringLiteral("StartupL2/FallFinalDurationMs"), defaults.startupL2FallFinalDurationMs);
    s.setValue(QStringLiteral("StartupL2/StepMa"), defaults.startupL2StepMa);

    // 兼容旧版本配置键：新代码不再以 OperatorSoftStart 作为主配置源。
    s.setValue(QStringLiteral("OperatorSoftStart/L1FinalMa"), defaults.startupL1FinalMa);
    s.setValue(QStringLiteral("OperatorSoftStart/L2FinalMa"), defaults.startupL2FinalMa);

    s.setValue(QStringLiteral("Step/L1CoarseStepMa"), defaults.l1CoarseStepMa);
    s.setValue(QStringLiteral("Step/L1FineStepMa"), defaults.l1FineStepMa);
    s.setValue(QStringLiteral("Step/L2CoarseStepMa"), defaults.l2CoarseStepMa);
    s.setValue(QStringLiteral("Step/L2FineStepMa"), defaults.l2FineStepMa);
    s.setValue(QStringLiteral("Step/L3StepMa"), defaults.l3StepMa);

    s.setValue(QStringLiteral("L3OperatorPower/MinPercent"), defaults.l3MinPercent);
    s.setValue(QStringLiteral("L3OperatorPower/MaxPercent"), defaults.l3MaxPercent);
    s.setValue(QStringLiteral("L3OperatorPower/MinMa"), defaults.l3MinMa);
    s.setValue(QStringLiteral("L3OperatorPower/MaxMa"), defaults.l3MaxMa);

    s.setValue(QStringLiteral("Ramp/DefaultIntervalMs"), defaults.defaultRampIntervalMs);
    s.setValue(QStringLiteral("Ramp/MinIntervalMs"), defaults.minRampIntervalMs);
    s.setValue(QStringLiteral("Ramp/MinManualSendIntervalMs"), defaults.minManualSendIntervalMs);

    // 临时温度旁路默认关闭；只有开发者确认后才允许忽略上位机 rawReady。
    s.setValue(QStringLiteral("Temperature/BypassReadyCheck"), defaults.temperatureReadyBypass);

    // 兼容旧版本 TRY 的 L1 字段；真实运行时以 StartupL1 为准。
    s.setValue(QStringLiteral("DeveloperTry/L1Phase1TimeSec"), defaults.startupL1RiseDurationMs / 1000);
    s.setValue(QStringLiteral("DeveloperTry/L1Phase2TimeSec"), defaults.startupL1FallMiddleDurationMs / 1000);
    s.setValue(QStringLiteral("DeveloperTry/L1Phase3TimeSec"), defaults.startupL1FallFinalDurationMs / 1000);
    s.setValue(QStringLiteral("DeveloperTry/L1StepMa"), defaults.startupL1StepMa);
    s.setValue(QStringLiteral("DeveloperTry/L1FinalMa"), defaults.startupL1FinalMa);
    s.setValue(QStringLiteral("DeveloperTry/L2TargetMa"), defaults.tryL2TargetMa);
    s.setValue(QStringLiteral("DeveloperTry/L2StepMa"), defaults.tryL2StepMa);
    s.setValue(QStringLiteral("DeveloperTry/L2TimeSec"), defaults.tryL2TimeSec);
    s.setValue(QStringLiteral("DeveloperTry/L3TargetMa"), defaults.tryL3TargetMa);
    s.setValue(QStringLiteral("DeveloperTry/L3StepMa"), defaults.tryL3StepMa);
    s.setValue(QStringLiteral("DeveloperTry/L3TimeSec"), defaults.tryL3TimeSec);
    s.sync();
}

void LaserController::loadConfig()
{
    const QString path = configFilePath();
    QSettings s(path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    s.setIniCodec("UTF-8");
#endif

    LaserConfig defaults;

    cfg.l1EnableL2Ma = s.value(QStringLiteral("Interlock/L1EnableL2Ma"), defaults.l1EnableL2Ma).toInt();
    cfg.l2EnableL3Ma = s.value(QStringLiteral("Interlock/L2EnableL3Ma"), defaults.l2EnableL3Ma).toInt();
    cfg.l2SafeOffMa = s.value(QStringLiteral("Interlock/L2SafeOffMa"), defaults.l2SafeOffMa).toInt();
    cfg.l3SafeOffMa = s.value(QStringLiteral("Interlock/L3SafeOffMa"), defaults.l3SafeOffMa).toInt();

    cfg.laser12MaxMa = s.value(QStringLiteral("Range/Laser12MaxMa"), defaults.laser12MaxMa).toInt();
    cfg.laser3MaxMa = s.value(QStringLiteral("Range/Laser3MaxMa"), defaults.laser3MaxMa).toInt();

    const int oldL1Phase1Ms = s.value(QStringLiteral("DeveloperTry/L1Phase1TimeSec"),
                                      defaults.startupL1RiseDurationMs / 1000).toInt() * 1000;
    const int oldL1Phase2Ms = s.value(QStringLiteral("DeveloperTry/L1Phase2TimeSec"),
                                      defaults.startupL1FallMiddleDurationMs / 1000).toInt() * 1000;
    const int oldL1Phase3Ms = s.value(QStringLiteral("DeveloperTry/L1Phase3TimeSec"),
                                      defaults.startupL1FallFinalDurationMs / 1000).toInt() * 1000;
    const int oldL2StepMa = s.value(QStringLiteral("OperatorSoftStart/L2CoarseMode"), true).toBool() ? 10 : 1;

    // StartupL1/StartupL2 是新的统一启动曲线。读取时兼容旧的 OperatorSoftStart 和 DeveloperTry/L1*。
    cfg.startupL1HighMa = s.value(QStringLiteral("StartupL1/HighMa"),
                                  s.value(QStringLiteral("OperatorSoftStart/HighMa"), defaults.startupL1HighMa)).toInt();
    cfg.startupL1MiddleMa = s.value(QStringLiteral("StartupL1/MiddleMa"),
                                    s.value(QStringLiteral("OperatorSoftStart/MiddleMa"), defaults.startupL1MiddleMa)).toInt();
    cfg.startupL1FinalMa = s.value(QStringLiteral("StartupL1/FinalMa"),
                                   s.value(QStringLiteral("DeveloperTry/L1FinalMa"),
                                           s.value(QStringLiteral("OperatorSoftStart/L1FinalMa"), defaults.startupL1FinalMa))).toInt();
    cfg.startupL1RiseDurationMs = s.value(QStringLiteral("StartupL1/RiseDurationMs"), oldL1Phase1Ms).toInt();
    cfg.startupL1FallMiddleDurationMs = s.value(QStringLiteral("StartupL1/FallMiddleDurationMs"), oldL1Phase2Ms).toInt();
    cfg.startupL1FallFinalDurationMs = s.value(QStringLiteral("StartupL1/FallFinalDurationMs"), oldL1Phase3Ms).toInt();
    cfg.startupL1StepMa = s.value(QStringLiteral("StartupL1/StepMa"),
                                  s.value(QStringLiteral("DeveloperTry/L1StepMa"), defaults.startupL1StepMa)).toInt();

    cfg.startupL2HighMa = s.value(QStringLiteral("StartupL2/HighMa"),
                                  s.value(QStringLiteral("OperatorSoftStart/HighMa"), defaults.startupL2HighMa)).toInt();
    cfg.startupL2MiddleMa = s.value(QStringLiteral("StartupL2/MiddleMa"),
                                    s.value(QStringLiteral("OperatorSoftStart/MiddleMa"), defaults.startupL2MiddleMa)).toInt();
    cfg.startupL2FinalMa = s.value(QStringLiteral("StartupL2/FinalMa"),
                                   s.value(QStringLiteral("OperatorSoftStart/L2FinalMa"), defaults.startupL2FinalMa)).toInt();
    cfg.startupL2RiseDurationMs = s.value(QStringLiteral("StartupL2/RiseDurationMs"),
                                          s.value(QStringLiteral("OperatorSoftStart/RiseDurationMs"), defaults.startupL2RiseDurationMs)).toInt();
    cfg.startupL2FallMiddleDurationMs = s.value(QStringLiteral("StartupL2/FallMiddleDurationMs"),
                                                s.value(QStringLiteral("OperatorSoftStart/FallMiddleDurationMs"), defaults.startupL2FallMiddleDurationMs)).toInt();
    cfg.startupL2FallFinalDurationMs = s.value(QStringLiteral("StartupL2/FallFinalDurationMs"),
                                               s.value(QStringLiteral("OperatorSoftStart/FallFinalDurationMs"), defaults.startupL2FallFinalDurationMs)).toInt();
    cfg.startupL2StepMa = s.value(QStringLiteral("StartupL2/StepMa"), oldL2StepMa).toInt();

    cfg.l1CoarseStepMa = s.value(QStringLiteral("Step/L1CoarseStepMa"), defaults.l1CoarseStepMa).toInt();
    cfg.l1FineStepMa = s.value(QStringLiteral("Step/L1FineStepMa"), defaults.l1FineStepMa).toInt();
    cfg.l2CoarseStepMa = s.value(QStringLiteral("Step/L2CoarseStepMa"), defaults.l2CoarseStepMa).toInt();
    cfg.l2FineStepMa = s.value(QStringLiteral("Step/L2FineStepMa"), defaults.l2FineStepMa).toInt();
    cfg.l3StepMa = s.value(QStringLiteral("Step/L3StepMa"), defaults.l3StepMa).toInt();

    cfg.l3MinPercent = s.value(QStringLiteral("L3OperatorPower/MinPercent"), defaults.l3MinPercent).toInt();
    cfg.l3MaxPercent = s.value(QStringLiteral("L3OperatorPower/MaxPercent"), defaults.l3MaxPercent).toInt();
    cfg.l3MinMa = s.value(QStringLiteral("L3OperatorPower/MinMa"), defaults.l3MinMa).toInt();
    cfg.l3MaxMa = s.value(QStringLiteral("L3OperatorPower/MaxMa"), defaults.l3MaxMa).toInt();

    cfg.defaultRampIntervalMs = s.value(QStringLiteral("Ramp/DefaultIntervalMs"), defaults.defaultRampIntervalMs).toInt();
    cfg.minRampIntervalMs = s.value(QStringLiteral("Ramp/MinIntervalMs"), defaults.minRampIntervalMs).toInt();
    cfg.minManualSendIntervalMs = s.value(QStringLiteral("Ramp/MinManualSendIntervalMs"), defaults.minManualSendIntervalMs).toInt();
    cfg.temperatureReadyBypass = s.value(QStringLiteral("Temperature/BypassReadyCheck"), defaults.temperatureReadyBypass).toBool();

    cfg.tryL1Phase1TimeSec = s.value(QStringLiteral("DeveloperTry/L1Phase1TimeSec"), defaults.tryL1Phase1TimeSec).toInt();
    cfg.tryL1Phase2TimeSec = s.value(QStringLiteral("DeveloperTry/L1Phase2TimeSec"), defaults.tryL1Phase2TimeSec).toInt();
    cfg.tryL1Phase3TimeSec = s.value(QStringLiteral("DeveloperTry/L1Phase3TimeSec"), defaults.tryL1Phase3TimeSec).toInt();
    cfg.tryL1StepMa = s.value(QStringLiteral("DeveloperTry/L1StepMa"), defaults.tryL1StepMa).toInt();
    cfg.tryL1FinalMa = s.value(QStringLiteral("DeveloperTry/L1FinalMa"), defaults.tryL1FinalMa).toInt();
    cfg.tryL2TargetMa = s.value(QStringLiteral("DeveloperTry/L2TargetMa"), defaults.tryL2TargetMa).toInt();
    cfg.tryL2StepMa = s.value(QStringLiteral("DeveloperTry/L2StepMa"), defaults.tryL2StepMa).toInt();
    cfg.tryL2TimeSec = s.value(QStringLiteral("DeveloperTry/L2TimeSec"), defaults.tryL2TimeSec).toInt();
    cfg.tryL3TargetMa = s.value(QStringLiteral("DeveloperTry/L3TargetMa"), defaults.tryL3TargetMa).toInt();
    cfg.tryL3StepMa = s.value(QStringLiteral("DeveloperTry/L3StepMa"), defaults.tryL3StepMa).toInt();
    cfg.tryL3TimeSec = s.value(QStringLiteral("DeveloperTry/L3TimeSec"), defaults.tryL3TimeSec).toInt();

    // 对配置做下限保护，避免用户手动改错 ini 后出现 0 步长、反向量程或除零。
    cfg.l2SafeOffMa = qMax(0, cfg.l2SafeOffMa);
    cfg.l3SafeOffMa = qMax(0, cfg.l3SafeOffMa);
    cfg.laser12MaxMa = qMax(1, cfg.laser12MaxMa);
    cfg.laser3MaxMa = qMax(cfg.l3SafeOffMa + 100, cfg.laser3MaxMa);
    cfg.l1EnableL2Ma = qBound(0, cfg.l1EnableL2Ma, cfg.laser12MaxMa);
    cfg.l2EnableL3Ma = qBound(0, cfg.l2EnableL3Ma, cfg.laser12MaxMa);

    // 启动曲线统一做边界保护，避免配置文件把高点/中点/最终工作点写到量程外。
    cfg.startupL1HighMa = qBound(0, cfg.startupL1HighMa, cfg.laser12MaxMa);
    cfg.startupL1MiddleMa = qBound(0, cfg.startupL1MiddleMa, cfg.laser12MaxMa);
    cfg.startupL1FinalMa = qBound(cfg.l1EnableL2Ma, cfg.startupL1FinalMa, cfg.laser12MaxMa);
    cfg.startupL1RiseDurationMs = qMax(1, cfg.startupL1RiseDurationMs);
    cfg.startupL1FallMiddleDurationMs = qMax(1, cfg.startupL1FallMiddleDurationMs);
    cfg.startupL1FallFinalDurationMs = qMax(1, cfg.startupL1FallFinalDurationMs);
    cfg.startupL1StepMa = normalizeStepChoice(cfg.startupL1StepMa);

    cfg.startupL2HighMa = qBound(0, cfg.startupL2HighMa, cfg.laser12MaxMa);
    cfg.startupL2MiddleMa = qBound(0, cfg.startupL2MiddleMa, cfg.laser12MaxMa);
    cfg.startupL2FinalMa = qBound(cfg.l2EnableL3Ma, cfg.startupL2FinalMa, cfg.laser12MaxMa);
    cfg.startupL2RiseDurationMs = qMax(1, cfg.startupL2RiseDurationMs);
    cfg.startupL2FallMiddleDurationMs = qMax(1, cfg.startupL2FallMiddleDurationMs);
    cfg.startupL2FallFinalDurationMs = qMax(1, cfg.startupL2FallFinalDurationMs);
    cfg.startupL2StepMa = normalizeStepChoice(cfg.startupL2StepMa);

    // 串口协议固定：L1/L2 粗调=10 mA、细调=1 mA；L3 单条指令约 100 mA。
    // 这里不再允许 ini 把协议步长改成其它值，避免软件电流和 STM32 实际输出不同步。
    cfg.l1CoarseStepMa = 10;
    cfg.l1FineStepMa = 1;
    cfg.l2CoarseStepMa = 10;
    cfg.l2FineStepMa = 1;
    cfg.l3StepMa = 100;

    cfg.l3MinPercent = qMax(0, cfg.l3MinPercent);
    cfg.l3MaxPercent = qMax(0, cfg.l3MaxPercent);
    if (cfg.l3MaxPercent <= cfg.l3MinPercent) {
        cfg.l3MaxPercent = cfg.l3MinPercent + 1;
    }
    cfg.l3MinMa = qBound(cfg.l3SafeOffMa, cfg.l3MinMa, cfg.laser3MaxMa - 1);
    cfg.l3MaxMa = qBound(cfg.l3MinMa + 1, cfg.l3MaxMa, cfg.laser3MaxMa);
    cfg.defaultRampIntervalMs = qMax(1, cfg.defaultRampIntervalMs);
    cfg.minRampIntervalMs = qMax(1, cfg.minRampIntervalMs);
    cfg.minManualSendIntervalMs = qMax(0, cfg.minManualSendIntervalMs);

    // 开发者 TRY 参数也从配置读取，但仍按当前量程做夹紧，避免旧 ini 或手动编辑产生越界扫描目标。
    // L1 TRY 前段已经并入 StartupL1，这里只保留镜像值，避免旧调用读到独立参数。
    cfg.tryL1Phase1TimeSec = cfg.startupL1RiseDurationMs / 1000;
    cfg.tryL1Phase2TimeSec = cfg.startupL1FallMiddleDurationMs / 1000;
    cfg.tryL1Phase3TimeSec = cfg.startupL1FallFinalDurationMs / 1000;
    cfg.tryL1StepMa = cfg.startupL1StepMa;
    cfg.tryL1FinalMa = cfg.startupL1FinalMa;
    cfg.tryL2TargetMa = qBound(cfg.startupL2FinalMa, cfg.tryL2TargetMa, cfg.laser12MaxMa);
    cfg.tryL2StepMa = normalizeStepChoice(cfg.tryL2StepMa);
    cfg.tryL2TimeSec = qMax(1, cfg.tryL2TimeSec);
    cfg.tryL3TargetMa = qBound(cfg.l3SafeOffMa, cfg.tryL3TargetMa, cfg.laser3MaxMa);
    // L3 没有 1/10 mA 指令，TRY 步长固定跟随硬件单步 100 mA。
    cfg.tryL3StepMa = cfg.l3StepMa;
    cfg.tryL3TimeSec = qMax(1, cfg.tryL3TimeSec);
}

QStringList LaserController::availablePortNames() const
{
    QStringList ports;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ports << info.portName();
    }
    return ports;
}

bool LaserController::openSerial(const QString &portName)
{
#ifdef DEBUG_MODE
    Q_UNUSED(portName);
    lastOpenedPortName = QStringLiteral("DEBUG");
    wasOpenedBefore = true;
    autoReconnectEnabled = true;
    laser1RawReady = true;
    laser2RawReady = true;
    laser3RawReady = true;
    updateLaserDependencies();
    emit logMessage(QString::fromUtf8(u8"[DEBUG] 模拟串口连接成功"));
    emit transportChanged(true, lastOpenedPortName);
    return true;
#else
    if (serialPort->isOpen()) {
        closeSerial();
        return true;
    }

    if (portName.isEmpty()) {
        emit logMessage(QString::fromUtf8(u8"[WARN] 请先选择串口"));
        return false;
    }

    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud115200);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadWrite)) {
        emit logMessage(QString::fromUtf8(u8"[ERROR] 无法打开串口: ") + serialPort->errorString());
        return false;
    }

    lastOpenedPortName = portName;
    wasOpenedBefore = true;
    autoReconnectEnabled = true;
    rxBuffer.clear();
    emit logMessage(QString::fromUtf8(u8"[INFO] 串口已打开: ") + portName);
    emit transportChanged(true, portName);
    return true;
#endif
}

void LaserController::closeSerial()
{
#ifdef DEBUG_MODE
    cancelActiveRamp(QString::fromUtf8(u8"控制通道已关闭"));
    wasOpenedBefore = false;
    autoReconnectEnabled = false;
    resetLaserStates();
    emit logMessage(QString::fromUtf8(u8"[DEBUG] 模拟串口已关闭"));
    emit transportChanged(false, QString());
    return;
#endif
    if (serialPort->isOpen()) {
        cancelActiveRamp(QString::fromUtf8(u8"串口已关闭"));
        serialPort->close();
        wasOpenedBefore = false;
        autoReconnectEnabled = false;
        resetLaserStates();
        emit logMessage(QString::fromUtf8(u8"[INFO] 串口已关闭"));
        emit transportChanged(false, QString());
    }
}

bool LaserController::hasLaserTransport() const
{
#ifdef DEBUG_MODE
    // Debug 模式跳过真实串口，但不跳过顺序联锁。
    return true;
#else
    return serialPort && serialPort->isOpen();
#endif
}

bool LaserController::isSerialOpen() const
{
#ifdef DEBUG_MODE
    return true;
#else
    return serialPort && serialPort->isOpen();
#endif
}

QString LaserController::currentPortName() const
{
    return lastOpenedPortName;
}

int LaserController::currentLaserMa(int laserIndex) const
{
    switch (laserIndex) {
    case 1: return currentLaser1mA;
    case 2: return currentLaser2mA;
    case 3: return currentLaser3mA;
    default: return 0;
    }
}

double LaserController::measuredLaserMa(int laserIndex) const
{
    switch (laserIndex) {
    case 1: return measuredLaser1mA;
    case 2: return measuredLaser2mA;
    case 3: return measuredLaser3mA;
    default: return -1;
    }
}

bool LaserController::laserReady(int laserIndex) const
{
    switch (laserIndex) {
    case 1: return laser1Ready;
    case 2: return laser2Ready;
    case 3: return laser3Ready;
    default: return false;
    }
}

bool LaserController::laserRawReady(int laserIndex) const
{
    switch (laserIndex) {
    case 1: return laser1RawReady;
    case 2: return laser2RawReady;
    case 3: return laser3RawReady;
    default: return false;
    }
}

bool LaserController::isLaserBusy(int laserIndex) const
{
    return rampTimer && rampTimer->isActive() && rampLaserIndex == laserIndex;
}

bool LaserController::isAnyLaserBusy() const
{
    return rampTimer && rampTimer->isActive();
}

int LaserController::l1EnableL2Ma() const
{
    return cfg.l1EnableL2Ma;
}

int LaserController::l2EnableL3Ma() const
{
    return cfg.l2EnableL3Ma;
}

int LaserController::l1FinalMa() const
{
    return cfg.startupL1FinalMa;
}

int LaserController::l2FinalMa() const
{
    return cfg.startupL2FinalMa;
}

int LaserController::l2SafeOffMa() const
{
    return cfg.l2SafeOffMa;
}

int LaserController::l3SafeOffMa() const
{
    return cfg.l3SafeOffMa;
}

int LaserController::laserMinMa(int laserIndex) const
{
    return (laserIndex == 3) ? cfg.l3SafeOffMa : 0;
}

int LaserController::laserMaxMa(int laserIndex) const
{
    return (laserIndex == 3) ? cfg.laser3MaxMa : cfg.laser12MaxMa;
}

int LaserController::laserStepMa(int laserIndex, bool coarseMode) const
{
    switch (laserIndex) {
    case 1:
        return coarseMode ? cfg.l1CoarseStepMa : cfg.l1FineStepMa;
    case 2:
        return coarseMode ? cfg.l2CoarseStepMa : cfg.l2FineStepMa;
    case 3:
        Q_UNUSED(coarseMode);
        return cfg.l3StepMa;
    default:
        return 1;
    }
}

int LaserController::operatorPowerMinPercent() const
{
    return cfg.l3MinPercent;
}

int LaserController::operatorPowerMaxPercent() const
{
    return cfg.l3MaxPercent;
}

bool LaserController::laserReadyForStartup(int laserIndex) const
{
#ifdef DEBUG_MODE
    Q_UNUSED(laserIndex);
    // Debug 模式没有真实温度回报，只跳过 ready 信号，不跳过电流顺序。
    return true;
#else
    if (cfg.temperatureReadyBypass) {
        Q_UNUSED(laserIndex);
        // 临时旁路只跳过上位机 rawReady 判断；canAdjustLaser() 中的电流顺序和关机顺序仍然继续执行。
        return true;
    }
    return laserRawReady(laserIndex);
#endif
}

bool LaserController::canAdjustLaser(int laserIndex, int direction) const
{
    if (!hasLaserTransport()) return false;

    // 升高/开启必须按 L1 -> L2 -> L3 推进。
    if (direction > 0) {
        switch (laserIndex) {
        case 1:
            return laserReadyForStartup(1)
                   && currentLaser2mA <= cfg.l2SafeOffMa
                   && currentLaser3mA <= cfg.l3SafeOffMa;
        case 2:
            return laserReadyForStartup(1)
                   && laserReadyForStartup(2)
                   && currentLaser1mA >= cfg.l1EnableL2Ma
                   && currentLaser3mA <= cfg.l3SafeOffMa;
        case 3:
            return laserReadyForStartup(1)
                   && laserReadyForStartup(2)
                   && laserReadyForStartup(3)
                   && currentLaser1mA >= cfg.l1EnableL2Ma
                   && currentLaser2mA >= cfg.l2EnableL3Ma;
        default:
            return false;
        }
    }

    // 降低/关断必须按 L3 -> L2 -> L1 回退。
    if (direction < 0) {
        switch (laserIndex) {
        case 3:
            return true;
        case 2:
            return currentLaser3mA <= cfg.l3SafeOffMa;
        case 1:
            return currentLaser2mA <= cfg.l2SafeOffMa
                   && currentLaser3mA <= cfg.l3SafeOffMa;
        default:
            return false;
        }
    }

    return false;
}

QString LaserController::adjustBlockReason(int laserIndex, int direction) const
{
    if (!hasLaserTransport()) return QString::fromUtf8(u8"串口未打开");

    if (direction > 0) {
        switch (laserIndex) {
        case 1:
            if (!laserReadyForStartup(1)) return QString::fromUtf8(u8"等待 L1 温度就绪");
            if (currentLaser2mA > cfg.l2SafeOffMa || currentLaser3mA > cfg.l3SafeOffMa)
                return QString::fromUtf8(u8"L1 调节前请先将 L2 调至 %1 mA，并将 L3 调至 %2 mA 最低/关闭态")
                    .arg(cfg.l2SafeOffMa).arg(cfg.l3SafeOffMa);
            return QString();
        case 2:
            if (currentLaser3mA > cfg.l3SafeOffMa)
                return QString::fromUtf8(u8"L2 调节前请先将 L3 调至 %1 mA 最低/关闭态").arg(cfg.l3SafeOffMa);
            if (!laserReadyForStartup(1)) return QString::fromUtf8(u8"等待 L1 温度就绪");
            if (currentLaser1mA < cfg.l1EnableL2Ma)
                return QString::fromUtf8(u8"请先将 L1 调至 >= %1 mA").arg(cfg.l1EnableL2Ma);
            if (!laserReadyForStartup(2)) return QString::fromUtf8(u8"等待 L2 温度就绪");
            return QString();
        case 3:
            if (!laserReadyForStartup(1)) return QString::fromUtf8(u8"等待 L1 温度就绪");
            if (currentLaser1mA < cfg.l1EnableL2Ma)
                return QString::fromUtf8(u8"请先将 L1 调至 >= %1 mA").arg(cfg.l1EnableL2Ma);
            if (!laserReadyForStartup(2)) return QString::fromUtf8(u8"等待 L2 温度就绪");
            if (currentLaser2mA < cfg.l2EnableL3Ma)
                return QString::fromUtf8(u8"请先将 L2 调至 >= %1 mA").arg(cfg.l2EnableL3Ma);
            if (!laserReadyForStartup(3)) return QString::fromUtf8(u8"等待 L3 就绪");
            return QString();
        default:
            return QString::fromUtf8(u8"未知激光器通道");
        }
    }

    if (direction < 0) {
        switch (laserIndex) {
        case 3:
            return QString();
        case 2:
            if (currentLaser3mA > cfg.l3SafeOffMa)
                return QString::fromUtf8(u8"请先将 L3 调至 %1 mA 最低/关闭态").arg(cfg.l3SafeOffMa);
            return QString();
        case 1:
            if (currentLaser3mA > cfg.l3SafeOffMa || currentLaser2mA > cfg.l2SafeOffMa)
                return QString::fromUtf8(u8"请先按顺序将 L3 调至 %1 mA，并将 L2 调至 %2 mA")
                    .arg(cfg.l3SafeOffMa).arg(cfg.l2SafeOffMa);
            return QString();
        default:
            return QString::fromUtf8(u8"未知激光器通道");
        }
    }

    return QString::fromUtf8(u8"无效调节方向");
}

bool LaserController::adjustLaser(int laserIndex, int direction, bool coarseMode)
{
    if (isAnyLaserBusy()) {
        emit logMessage(QString::fromUtf8(u8"[WARN] 当前正在执行 L%1 缓升/缓降，请完成后再操作").arg(rampLaserIndex));
        return false;
    }

    int step = 0;
    int lo = 0;
    int hi = 0;
    char cmd = 0;

    switch (laserIndex) {
    case 1:
        step = coarseMode ? cfg.l1CoarseStepMa : cfg.l1FineStepMa;
        lo = 0; hi = cfg.laser12MaxMa;
        cmd = coarseMode ? ((direction > 0) ? '1' : '0') : ((direction > 0) ? '3' : '2');
        break;
    case 2:
        step = coarseMode ? cfg.l2CoarseStepMa : cfg.l2FineStepMa;
        lo = 0; hi = cfg.laser12MaxMa;
        cmd = coarseMode ? ((direction > 0) ? '4' : '5') : ((direction > 0) ? '6' : '7');
        break;
    case 3:
        step = cfg.l3StepMa;
        lo = cfg.l3SafeOffMa; hi = cfg.laser3MaxMa;
        cmd = (direction > 0) ? '8' : '9';
        break;
    default:
        return false;
    }

    if (!canAdjustLaser(laserIndex, direction)) {
        emit logMessage(QString::fromUtf8(u8"[WARN] L%1 当前不允许调节：%2")
                        .arg(laserIndex).arg(adjustBlockReason(laserIndex, direction)));
        return false;
    }

    int current = currentLaserMa(laserIndex);
    int newVal = qBound(lo, current + direction * step, hi);
    if (newVal == current) return true;

    if (!sendLaserCommand(laserIndex, cmd)) return false;
    setCurrentLaserMa(laserIndex, newVal);
    return true;
}

bool LaserController::setLaserTarget(int laserIndex, int target, bool coarseMode, int durationMs)
{
    int lo = laserMinMa(laserIndex);
    int hi = laserMaxMa(laserIndex);
    if (laserIndex == 3) {
        // L3 按硬件固定步长对齐，目前默认 100 mA，不跟随 L1/L2 的 1/10 mA 选择。
        const int step = cfg.l3StepMa;
        const int offset = target - lo;
        target = lo + ((offset + step / 2) / step) * step;
    }
    target = qBound(lo, target, hi);

    int current = currentLaserMa(laserIndex);
    if (target == current) return true;

    int direction = (target > current) ? +1 : -1;
    if (!canAdjustLaser(laserIndex, direction)) {
        emit logMessage(QString::fromUtf8(u8"[WARN] L%1 目标值被拒绝：%2")
                        .arg(laserIndex).arg(adjustBlockReason(laserIndex, direction)));
        return false;
    }

    // 目标值调节改为定时器分步执行：Debug 和真实串口都保持同样的缓升/缓降时间流程。
    return startRampToTarget(laserIndex, target, coarseMode, durationMs);
}

bool LaserController::startRampToTarget(int laserIndex, int target, bool coarseMode, int durationMs)
{
    if (isAnyLaserBusy()) {
        emit logMessage(QString::fromUtf8(u8"[WARN] L%1 正在缓升/缓降，请完成后再操作").arg(rampLaserIndex));
        return false;
    }

    const int current = currentLaserMa(laserIndex);
    if (target == current) {
        return true;
    }

    const int direction = (target > current) ? +1 : -1;
    if (!canAdjustLaser(laserIndex, direction)) {
        emit logMessage(QString::fromUtf8(u8"[WARN] L%1 缓升/缓降未启动：%2")
                        .arg(laserIndex).arg(adjustBlockReason(laserIndex, direction)));
        return false;
    }

    rampLaserIndex = laserIndex;
    rampTargetMa = target;
    rampCoarseMode = coarseMode;
    rampIntervalMs = rampIntervalForTarget(laserIndex, target, coarseMode, durationMs);
    rampTimer->start(rampIntervalMs);

    // busy 信号用于界面显示“正在开启/正在关闭”，真正到达目标后才切换成已开启/已关闭。
    emit busyChanged(laserIndex, true);
    emit operationStarted(laserIndex, target);
    emit logMessage(QString::fromUtf8(u8"[INFO] L%1 开始缓%2到 %3 mA，步进间隔 %4 ms")
                    .arg(laserIndex)
                    .arg(direction > 0 ? QString::fromUtf8(u8"升") : QString::fromUtf8(u8"降"))
                    .arg(target)
                    .arg(rampIntervalMs));
    return true;
}

void LaserController::processRampStep()
{
    if (rampLaserIndex < 1 || rampLaserIndex > 3) {
        rampTimer->stop();
        return;
    }

    const int current = currentLaserMa(rampLaserIndex);
    if (current == rampTargetMa) {
        finishRamp(true, QString::fromUtf8(u8"已到达目标电流"));
        return;
    }

    const int direction = (rampTargetMa > current) ? +1 : -1;
    if (!canAdjustLaser(rampLaserIndex, direction)) {
        finishRamp(false, adjustBlockReason(rampLaserIndex, direction));
        return;
    }

    const int diff = qAbs(rampTargetMa - current);
    const int coarseStep = laserStepMa(rampLaserIndex, true);
    // L1/L2 粗调模式在尾数处自动切到 1 mA；L3 仍按固定硬件步长运行。
    bool useCoarse = (rampLaserIndex == 3) ? true : (rampCoarseMode && diff >= coarseStep);

    const int step = rampStepSize(rampLaserIndex, diff, useCoarse);

    const char cmd = commandForStep(rampLaserIndex, direction, useCoarse);
    if (cmd == 0) {
        finishRamp(false, QString::fromUtf8(u8"无效的激光器步进命令"));
        return;
    }

    if (!sendLaserCommand(rampLaserIndex, cmd)) {
        finishRamp(false, QString::fromUtf8(u8"命令发送失败或发送间隔过短"));
        return;
    }

    setCurrentLaserMa(rampLaserIndex, current + direction * step);
    emit operationProgress(rampLaserIndex, currentLaserMa(rampLaserIndex), rampTargetMa);

    if (currentLaserMa(rampLaserIndex) == rampTargetMa) {
        finishRamp(true, QString::fromUtf8(u8"已到达目标电流"));
    }
}

void LaserController::finishRamp(bool success, const QString &message)
{
    if (rampLaserIndex < 1 || rampLaserIndex > 3) {
        if (rampTimer) rampTimer->stop();
        return;
    }

    const int finishedLaser = rampLaserIndex;
    const int finishedTarget = rampTargetMa;
    rampTimer->stop();
    rampLaserIndex = 0;
    rampTargetMa = 0;

    const bool softContinuationExpected = success
            && finishedLaser == operatorSoftLaserIndex
            && (operatorSoftPhase == SoftRiseHigh || operatorSoftPhase == SoftFallMiddle);

    // 用户模式 L1/L2 开启是三段式曲线：中间段完成后继续下一段，不提前通知界面“已开启”。
    if (success && continueOperatorSoftOn(finishedLaser)) {
        emit logMessage(QString::fromUtf8(u8"[INFO] L%1 用户模式启动曲线进入下一段").arg(finishedLaser));
        return;
    }

    bool finishSuccess = success;
    QString finishMessage = message;
    if (softContinuationExpected) {
        finishSuccess = false;
        finishMessage = QString::fromUtf8(u8"用户模式启动曲线下一段启动失败");
    }

    if (!finishSuccess && finishedLaser == operatorSoftLaserIndex) {
        operatorSoftPhase = SoftIdle;
        operatorSoftLaserIndex = 0;
        operatorSoftFinalMa = 0;
    }

    emit busyChanged(finishedLaser, false);
    emit operationFinished(finishedLaser, finishSuccess, finishMessage);
    emit logMessage(QString::fromUtf8(u8"[INFO] L%1 缓升/缓降%2：目标 %3 mA，当前 %4 mA，%5")
                    .arg(finishedLaser)
                    .arg(finishSuccess ? QString::fromUtf8(u8"完成") : QString::fromUtf8(u8"中止"))
                    .arg(finishedTarget)
                    .arg(currentLaserMa(finishedLaser))
                    .arg(finishMessage));
    emit stateChanged();
}

void LaserController::cancelActiveRamp(const QString &message)
{
    if (!isAnyLaserBusy()) return;
    finishRamp(false, message);
}

bool LaserController::startOperatorSoftOn(int laserIndex)
{
    if (laserIndex != 1 && laserIndex != 2) return false;

    const int finalTarget = startupFinalMa(laserIndex);
    const int highTarget = startupHighMa(laserIndex);
    const int middleTarget = startupMiddleMa(laserIndex);
    const int riseDurationMs = startupRiseDurationMs(laserIndex);
    const int fallMiddleDurationMs = startupFallMiddleDurationMs(laserIndex);
    const bool softCoarseMode = startupCoarseMode(laserIndex);
    const int current = currentLaserMa(laserIndex);
    const int direction = (highTarget > current) ? +1 : -1;
    if (!canAdjustLaser(laserIndex, direction)) {
        emit logMessage(QString::fromUtf8(u8"[WARN] L%1 用户模式启动被拒绝：%2")
                        .arg(laserIndex).arg(adjustBlockReason(laserIndex, direction)));
        return false;
    }

    operatorSoftLaserIndex = laserIndex;
    operatorSoftFinalMa = finalTarget;
    operatorSoftPhase = SoftRiseHigh;

    // 用户模式启动保持三段式曲线；每一段目标、时长、步长模式都从 ini 配置读取。
    // 普通页面和开发者 TRY 共享 StartupL1/StartupL2，避免两套启动曲线不一致。
    if (current == highTarget) {
        operatorSoftPhase = SoftFallMiddle;
        const bool ok = startRampToTarget(laserIndex, middleTarget, softCoarseMode, fallMiddleDurationMs);
        if (!ok) {
            operatorSoftPhase = SoftIdle;
            operatorSoftLaserIndex = 0;
            operatorSoftFinalMa = 0;
        }
        return ok;
    }
    const bool ok = startRampToTarget(laserIndex, highTarget, softCoarseMode, riseDurationMs);
    if (!ok) {
        operatorSoftPhase = SoftIdle;
        operatorSoftLaserIndex = 0;
        operatorSoftFinalMa = 0;
    }
    return ok;
}

bool LaserController::continueOperatorSoftOn(int finishedLaser)
{
    if (finishedLaser != operatorSoftLaserIndex || operatorSoftPhase == SoftIdle) {
        return false;
    }

    if (operatorSoftPhase == SoftRiseHigh) {
        operatorSoftPhase = SoftFallMiddle;
        // 第二段从高点缓降到中间电流，目标值和时长由配置文件决定。
        // 第二段从高点回落到中间点，继续使用统一启动曲线配置。
        const bool softCoarseMode = startupCoarseMode(finishedLaser);
        const bool ok = startRampToTarget(finishedLaser,
                                          startupMiddleMa(finishedLaser),
                                          softCoarseMode,
                                          startupFallMiddleDurationMs(finishedLaser));
        if (!ok) {
            operatorSoftPhase = SoftIdle;
            operatorSoftLaserIndex = 0;
            operatorSoftFinalMa = 0;
        }
        return ok;
    }

    if (operatorSoftPhase == SoftFallMiddle) {
        operatorSoftPhase = SoftFallFinal;
        // 第三段回到普通页面最终工作电流，完成后按钮才切换为“已开启”。
        // 第三段回到最终工作电流；完成后普通页面按钮才切换为“已开启”。
        const bool softCoarseMode = startupCoarseMode(finishedLaser);
        const bool ok = startRampToTarget(finishedLaser,
                                          operatorSoftFinalMa,
                                          softCoarseMode,
                                          startupFallFinalDurationMs(finishedLaser));
        if (!ok) {
            operatorSoftPhase = SoftIdle;
            operatorSoftLaserIndex = 0;
            operatorSoftFinalMa = 0;
        }
        return ok;
    }

    operatorSoftPhase = SoftIdle;
    operatorSoftLaserIndex = 0;
    operatorSoftFinalMa = 0;
    return false;
}

char LaserController::commandForStep(int laserIndex, int direction, bool coarseMode) const
{
    switch (laserIndex) {
    case 1:
        return coarseMode ? ((direction > 0) ? '1' : '0') : ((direction > 0) ? '3' : '2');
    case 2:
        return coarseMode ? ((direction > 0) ? '4' : '5') : ((direction > 0) ? '6' : '7');
    case 3:
        return (direction > 0) ? '8' : '9';
    default:
        return 0;
    }
}

int LaserController::rampStepSize(int laserIndex, int diff, bool coarseMode) const
{
    const int step = laserStepMa(laserIndex, coarseMode);
    return qMin(step, diff);
}

int LaserController::rampIntervalForTarget(int laserIndex, int target, bool coarseMode, int durationMs) const
{
    const int current = currentLaserMa(laserIndex);
    const int diff = qAbs(target - current);
    if (diff <= 0) return cfg.defaultRampIntervalMs;

    const int baseStep = laserStepMa(laserIndex, coarseMode);
    const int steps = qMax(1, (diff + baseStep - 1) / baseStep);
    if (durationMs > 0) {
        // 指定时长时，发送间隔由曲线时长反推；最小间隔也放入配置，满足 40s 等目标流程时可直接改 ini。
        return qMax(cfg.minRampIntervalMs, durationMs / steps);
    }

    return cfg.defaultRampIntervalMs;
}

int LaserController::startupHighMa(int laserIndex) const
{
    return (laserIndex == 1) ? cfg.startupL1HighMa : cfg.startupL2HighMa;
}

int LaserController::startupMiddleMa(int laserIndex) const
{
    return (laserIndex == 1) ? cfg.startupL1MiddleMa : cfg.startupL2MiddleMa;
}

int LaserController::startupFinalMa(int laserIndex) const
{
    return (laserIndex == 1) ? cfg.startupL1FinalMa : cfg.startupL2FinalMa;
}

int LaserController::startupRiseDurationMs(int laserIndex) const
{
    return (laserIndex == 1) ? cfg.startupL1RiseDurationMs : cfg.startupL2RiseDurationMs;
}

int LaserController::startupFallMiddleDurationMs(int laserIndex) const
{
    return (laserIndex == 1) ? cfg.startupL1FallMiddleDurationMs : cfg.startupL2FallMiddleDurationMs;
}

int LaserController::startupFallFinalDurationMs(int laserIndex) const
{
    return (laserIndex == 1) ? cfg.startupL1FallFinalDurationMs : cfg.startupL2FallFinalDurationMs;
}

int LaserController::startupStepMa(int laserIndex) const
{
    return (laserIndex == 1) ? cfg.startupL1StepMa : cfg.startupL2StepMa;
}

bool LaserController::startupCoarseMode(int laserIndex) const
{
    // 启动曲线步长只允许 1 mA 或 10 mA；10 mA 对应 STM32 粗调指令，1 mA 对应细调指令。
    return startupStepMa(laserIndex) == laserStepMa(laserIndex, true);
}

int LaserController::operatorPowerPercentToMa(int percent) const
{
    const int minPercent = cfg.l3MinPercent;
    const int maxPercent = cfg.l3MaxPercent;
    const int minMa = cfg.l3MinMa;
    const int maxMa = cfg.l3MaxMa;
    const int clampedPercent = qBound(minPercent, percent, maxPercent);

    // 普通操作员页面使用百分比显示，控制核心按配置统一换算到 L3 的真实 mA 目标值。
    const double ratio = double(clampedPercent - minPercent) / double(maxPercent - minPercent);
    int targetMa = minMa + qRound(ratio * double(maxMa - minMa));

    // 普通页面 L3 功率按硬件固定步长对齐，默认 100 mA。
    const int step = cfg.l3StepMa;
    const int offset = targetMa - minMa;
    targetMa = minMa + ((offset + step / 2) / step) * step;
    return qBound(minMa, targetMa, maxMa);
}

int LaserController::operatorPowerMaToPercent(int currentMa) const
{
    const int minPercent = cfg.l3MinPercent;
    const int maxPercent = cfg.l3MaxPercent;
    const int minMa = cfg.l3MinMa;
    const int maxMa = cfg.l3MaxMa;
    const int clampedMa = qBound(minMa, currentMa, maxMa);

    // 开发者页面或控制核心改变 L3 后，普通页面按同一配置反算百分比，保持显示一致。
    const double ratio = double(clampedMa - minMa) / double(maxMa - minMa);
    return qBound(minPercent, minPercent + qRound(ratio * double(maxPercent - minPercent)), maxPercent);
}

bool LaserController::temperatureReadyBypassEnabled() const
{
    return cfg.temperatureReadyBypass;
}

LaserController::DeveloperRuntimeParams LaserController::developerRuntimeParams() const
{
    DeveloperRuntimeParams params;
    params.operatorL1FinalMa = cfg.startupL1FinalMa;
    params.operatorL2FinalMa = cfg.startupL2FinalMa;
    params.l3OperatorMaxMa = cfg.l3MaxMa;
    params.startupL1HighMa = cfg.startupL1HighMa;
    params.startupL1MiddleMa = cfg.startupL1MiddleMa;
    params.startupL1Phase1TimeSec = cfg.startupL1RiseDurationMs / 1000;
    params.startupL1Phase2TimeSec = cfg.startupL1FallMiddleDurationMs / 1000;
    params.startupL1Phase3TimeSec = cfg.startupL1FallFinalDurationMs / 1000;
    params.startupL1StepMa = cfg.startupL1StepMa;
    params.startupL2HighMa = cfg.startupL2HighMa;
    params.startupL2MiddleMa = cfg.startupL2MiddleMa;
    params.startupL2Phase1TimeSec = cfg.startupL2RiseDurationMs / 1000;
    params.startupL2Phase2TimeSec = cfg.startupL2FallMiddleDurationMs / 1000;
    params.startupL2Phase3TimeSec = cfg.startupL2FallFinalDurationMs / 1000;
    params.startupL2StepMa = cfg.startupL2StepMa;
    // 旧的 L1 TRY 字段镜像 StartupL1，保证尚未改完的界面变量也不会读到另一套参数。
    params.tryL1Phase1TimeSec = params.startupL1Phase1TimeSec;
    params.tryL1Phase2TimeSec = params.startupL1Phase2TimeSec;
    params.tryL1Phase3TimeSec = params.startupL1Phase3TimeSec;
    params.tryL1StepMa = params.startupL1StepMa;
    params.tryL1FinalMa = params.operatorL1FinalMa;
    params.tryL2TargetMa = cfg.tryL2TargetMa;
    params.tryL2StepMa = cfg.tryL2StepMa;
    params.tryL2TimeSec = cfg.tryL2TimeSec;
    params.tryL3TargetMa = cfg.tryL3TargetMa;
    params.tryL3StepMa = cfg.tryL3StepMa;
    params.tryL3TimeSec = cfg.tryL3TimeSec;
    return params;
}

bool LaserController::setTemperatureReadyBypassEnabled(bool enabled, QString *error)
{
    if (isAnyLaserBusy()) {
        if (error) *error = QString::fromUtf8(u8"当前仍在缓升/缓降，不能切换温度旁路状态。");
        return false;
    }

    if (cfg.temperatureReadyBypass == enabled) {
        return true;
    }

    saveDefaultConfigIfMissing();
    const QString path = configFilePath();
    const QString backupPath = path + QStringLiteral(".bak");
    if (QFile::exists(path)) {
        QFile::remove(backupPath);
        if (!QFile::copy(path, backupPath)) {
            if (error) *error = QString::fromUtf8(u8"写入前备份配置文件失败，请检查目录权限。");
            return false;
        }
    }

    QSettings s(path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    s.setIniCodec("UTF-8");
#endif
    s.setValue(QStringLiteral("Temperature/BypassReadyCheck"), enabled);
    s.sync();
    if (s.status() != QSettings::NoError) {
        if (error) *error = QString::fromUtf8(u8"温度旁路配置写入失败，请检查文件是否只读或被占用。");
        return false;
    }

    cfg.temperatureReadyBypass = enabled;
    emit logMessage(enabled
                    ? QString::fromUtf8(u8"[WARN] 温度就绪旁路已开启：上位机忽略 rawReady，仅保留顺序联锁，最终温度保护依赖下位机")
                    : QString::fromUtf8(u8"[INFO] 温度就绪旁路已关闭：上位机重新要求 STM32 温度 ready"));
    emit stateChanged();
    return true;
}

bool LaserController::saveDeveloperLaserParameters(int laserIndex, const DeveloperRuntimeParams &params, QString *error)
{
    if (isAnyLaserBusy()) {
        if (error) *error = QString::fromUtf8(u8"当前仍在缓升/缓降，不能保存会变化的运行参数。");
        return false;
    }

    if (laserIndex < 1 || laserIndex > 3) {
        if (error) *error = QString::fromUtf8(u8"未知激光器通道，未写入配置文件。");
        return false;
    }

    if (laserIndex == 1 && (params.operatorL1FinalMa < cfg.l1EnableL2Ma || params.operatorL1FinalMa > cfg.laser12MaxMa)) {
        if (error) {
            *error = QString::fromUtf8(u8"L1 当前值必须在 %1~%2 mA 之间，才能保存为普通页面最终工作电流。")
                    .arg(cfg.l1EnableL2Ma).arg(cfg.laser12MaxMa);
        }
        return false;
    }
    if (laserIndex == 2 && (params.operatorL2FinalMa < cfg.l2EnableL3Ma || params.operatorL2FinalMa > cfg.laser12MaxMa)) {
        if (error) {
            *error = QString::fromUtf8(u8"L2 当前值必须在 %1~%2 mA 之间，才能保存为普通页面最终工作电流。")
                    .arg(cfg.l2EnableL3Ma).arg(cfg.laser12MaxMa);
        }
        return false;
    }
    if (laserIndex == 3 && (params.l3OperatorMaxMa <= cfg.l3MinMa || params.l3OperatorMaxMa > cfg.laser3MaxMa)) {
        if (error) {
            *error = QString::fromUtf8(u8"L3 当前值必须大于 %1 mA 且不超过 %2 mA，不能把关闭态保存为 100% 功率。")
                    .arg(cfg.l3MinMa).arg(cfg.laser3MaxMa);
        }
        return false;
    }

    const int l3Step = cfg.l3StepMa;
    const int l3Offset = params.l3OperatorMaxMa - cfg.l3MinMa;
    // L3 普通页面 100% 电流按硬件固定步长对齐，避免配置中保存零散尾数。
    const int alignedL3MaxMa = cfg.l3MinMa + ((l3Offset + l3Step / 2) / l3Step) * l3Step;
    if (laserIndex == 3 && (alignedL3MaxMa <= cfg.l3MinMa || alignedL3MaxMa > cfg.laser3MaxMa)) {
        if (error) *error = QString::fromUtf8(u8"L3 当前值按步长对齐后超出允许范围，未写入配置文件。");
        return false;
    }

    const int alignedTryL3TargetMa = cfg.l3SafeOffMa
            + ((params.tryL3TargetMa - cfg.l3SafeOffMa + l3Step / 2) / l3Step) * l3Step;
    const int fixedTryL3StepMa = cfg.l3StepMa;
    // L1/L2 TRY 步长只保存 1 mA 或 10 mA，开发者窗口不能写入任意步长。
    const int normalizedStartupL1StepMa = normalizeStepChoice(params.startupL1StepMa);
    const int normalizedStartupL2StepMa = normalizeStepChoice(params.startupL2StepMa);
    const int normalizedTryL1StepMa = normalizedStartupL1StepMa;
    const int normalizedTryL2StepMa = normalizeStepChoice(params.tryL2StepMa);

    saveDefaultConfigIfMissing();
    const QString path = configFilePath();
    const QString backupPath = path + QStringLiteral(".bak");
    if (QFile::exists(path)) {
        QFile::remove(backupPath);
        if (!QFile::copy(path, backupPath)) {
            if (error) *error = QString::fromUtf8(u8"写入前备份配置文件失败，请检查目录权限。");
            return false;
        }
    }

    QSettings s(path, QSettings::IniFormat);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    s.setIniCodec("UTF-8");
#endif

    // 按通道写入，避免 L1 参数窗口误覆盖 L2/L3 的配置。
    if (laserIndex == 1) {
        // L1 的普通启动和 TRY 前段共用 StartupL1；旧 DeveloperTry/L1* 同步写入只是为了兼容老配置。
        s.setValue(QStringLiteral("StartupL1/HighMa"), qBound(0, params.startupL1HighMa, cfg.laser12MaxMa));
        s.setValue(QStringLiteral("StartupL1/MiddleMa"), qBound(0, params.startupL1MiddleMa, cfg.laser12MaxMa));
        s.setValue(QStringLiteral("StartupL1/FinalMa"), params.operatorL1FinalMa);
        s.setValue(QStringLiteral("StartupL1/RiseDurationMs"), qMax(1, params.startupL1Phase1TimeSec) * 1000);
        s.setValue(QStringLiteral("StartupL1/FallMiddleDurationMs"), qMax(1, params.startupL1Phase2TimeSec) * 1000);
        s.setValue(QStringLiteral("StartupL1/FallFinalDurationMs"), qMax(1, params.startupL1Phase3TimeSec) * 1000);
        s.setValue(QStringLiteral("StartupL1/StepMa"), normalizedStartupL1StepMa);
        s.setValue(QStringLiteral("OperatorSoftStart/L1FinalMa"), params.operatorL1FinalMa);
        s.setValue(QStringLiteral("DeveloperTry/L1Phase1TimeSec"), qMax(1, params.startupL1Phase1TimeSec));
        s.setValue(QStringLiteral("DeveloperTry/L1Phase2TimeSec"), qMax(1, params.startupL1Phase2TimeSec));
        s.setValue(QStringLiteral("DeveloperTry/L1Phase3TimeSec"), qMax(1, params.startupL1Phase3TimeSec));
        s.setValue(QStringLiteral("DeveloperTry/L1StepMa"), normalizedTryL1StepMa);
        s.setValue(QStringLiteral("DeveloperTry/L1FinalMa"), params.operatorL1FinalMa);
    } else if (laserIndex == 2) {
        // L2 同样先走 StartupL2 启动曲线，完成后 TRY 才继续扫描到 DeveloperTry/L2TargetMa。
        s.setValue(QStringLiteral("StartupL2/HighMa"), qBound(0, params.startupL2HighMa, cfg.laser12MaxMa));
        s.setValue(QStringLiteral("StartupL2/MiddleMa"), qBound(0, params.startupL2MiddleMa, cfg.laser12MaxMa));
        s.setValue(QStringLiteral("StartupL2/FinalMa"), params.operatorL2FinalMa);
        s.setValue(QStringLiteral("StartupL2/RiseDurationMs"), qMax(1, params.startupL2Phase1TimeSec) * 1000);
        s.setValue(QStringLiteral("StartupL2/FallMiddleDurationMs"), qMax(1, params.startupL2Phase2TimeSec) * 1000);
        s.setValue(QStringLiteral("StartupL2/FallFinalDurationMs"), qMax(1, params.startupL2Phase3TimeSec) * 1000);
        s.setValue(QStringLiteral("StartupL2/StepMa"), normalizedStartupL2StepMa);
        s.setValue(QStringLiteral("OperatorSoftStart/L2FinalMa"), params.operatorL2FinalMa);
        s.setValue(QStringLiteral("DeveloperTry/L2TargetMa"), qBound(params.operatorL2FinalMa, params.tryL2TargetMa, cfg.laser12MaxMa));
        s.setValue(QStringLiteral("DeveloperTry/L2StepMa"), normalizedTryL2StepMa);
        s.setValue(QStringLiteral("DeveloperTry/L2TimeSec"), qMax(1, params.tryL2TimeSec));
    } else if (laserIndex == 3) {
        s.setValue(QStringLiteral("L3OperatorPower/MaxMa"), alignedL3MaxMa);
        s.setValue(QStringLiteral("DeveloperTry/L3TargetMa"), qBound(cfg.l3SafeOffMa, alignedTryL3TargetMa, cfg.laser3MaxMa));
        // L3 串口协议只有 '8'/'9' 固定约 100 mA 步进，保存时也强制写回 100 mA。
        s.setValue(QStringLiteral("DeveloperTry/L3StepMa"), fixedTryL3StepMa);
        s.setValue(QStringLiteral("DeveloperTry/L3TimeSec"), qMax(1, params.tryL3TimeSec));
    }
    s.sync();

    if (s.status() != QSettings::NoError) {
        if (error) *error = QString::fromUtf8(u8"配置文件写入失败，请检查文件是否只读或被占用。");
        return false;
    }

    loadConfig();
    rampIntervalMs = cfg.defaultRampIntervalMs;
    minSendIntervalMs = cfg.minManualSendIntervalMs;
    emit logMessage(QString::fromUtf8(u8"[INFO] L%1 开发者参数已写入配置文件：%2").arg(laserIndex).arg(path));
    emit stateChanged();
    return true;
}

bool LaserController::requestOperatorSwitch(int laserIndex, bool on, QString *reason)
{
    if (isAnyLaserBusy()) {
        if (reason) *reason = QString::fromUtf8(u8"L%1 正在缓升/缓降，请等待完成").arg(rampLaserIndex);
        emit logMessage(QString::fromUtf8(u8"[WARN] 普通页面操作被拒绝：%1")
                        .arg(reason ? *reason : QString::fromUtf8(u8"正在缓升/缓降")));
        return false;
    }

    int target = 0;
    switch (laserIndex) {
    case 1:
        target = on ? startupFinalMa(1) : 0;
        break;
    case 2:
        target = on ? startupFinalMa(2) : 0;
        break;
    default:
        if (reason) *reason = QString::fromUtf8(u8"普通页面暂只支持 L1 和 L2 开关");
        return false;
    }

    int current = currentLaserMa(laserIndex);
    int direction = (target > current) ? +1 : (target < current ? -1 : 0);
    if (direction != 0 && !canAdjustLaser(laserIndex, direction)) {
        if (reason) *reason = adjustBlockReason(laserIndex, direction);
        emit logMessage(QString::fromUtf8(u8"[WARN] 普通页面 L%1 %2被拒绝：%3")
                        .arg(laserIndex).arg(on ? QString::fromUtf8(u8"开启") : QString::fromUtf8(u8"关闭"))
                        .arg(reason ? *reason : adjustBlockReason(laserIndex, direction)));
        return false;
    }

    bool ok = false;
    if (on && (laserIndex == 1 || laserIndex == 2)) {
        // 普通用户模式开启 L1/L2 时走三段式细调曲线，不再直接跳到 90 mA。
        ok = startOperatorSoftOn(laserIndex);
    } else {
        // 关闭仍是单段缓降到 0 mA；这里使用细调，让按钮不会提前变灰。
        ok = setLaserTarget(laserIndex, target, false);
    }
    if (!ok && reason && reason->isEmpty()) {
        *reason = adjustBlockReason(laserIndex, direction);
    }
    return ok;
}

bool LaserController::requestOperatorPowerPercent(int percent, QString *reason)
{
    if (isAnyLaserBusy()) {
        if (reason) *reason = QString::fromUtf8(u8"L%1 正在缓升/缓降，请等待完成").arg(rampLaserIndex);
        emit logMessage(QString::fromUtf8(u8"[WARN] 普通页面 L3 功率设置被拒绝：%1")
                        .arg(reason ? *reason : QString::fromUtf8(u8"正在缓升/缓降")));
        return false;
    }

    const int target = operatorPowerPercentToMa(percent);
    const int current = currentLaserMa(3);
    const int direction = (target > current) ? +1 : (target < current ? -1 : 0);

    if (direction == 0) {
        return true;
    }

    // 普通页面的功率百分比最终仍然走 L3 顺序联锁，不能绕过 L1/L2 的启动条件。
    if (!canAdjustLaser(3, direction)) {
        const QString blockReason = adjustBlockReason(3, direction);
        if (reason) *reason = blockReason;
        emit logMessage(QString::fromUtf8(u8"[WARN] 普通页面 L3 功率 %1% 被拒绝：%2")
                        .arg(qBound(cfg.l3MinPercent, percent, cfg.l3MaxPercent)).arg(blockReason));
        return false;
    }

    const bool ok = setLaserTarget(3, target, true);
    if (!ok && reason && reason->isEmpty()) {
        *reason = adjustBlockReason(3, direction);
    }
    return ok;
}

bool LaserController::sendLaserCommand(int laserIndex, char cmd)
{
    if (laserIndex < 1 || laserIndex > 3) return false;

    int direction = commandDirectionForLaser(laserIndex, cmd);
    if (direction == 0) {
        emit logMessage(QString::fromUtf8(u8"[WARN] Laser%1 无效命令：%2").arg(laserIndex).arg(QChar(cmd)));
        return false;
    }

    // 控制核心是最后防线：所有界面请求到这里都必须再过一次方向联锁。
    if (!canAdjustLaser(laserIndex, direction)) {
        emit logMessage(QString::fromUtf8(u8"[WARN] Laser%1 未发送：%2")
                        .arg(laserIndex).arg(adjustBlockReason(laserIndex, direction)));
        return false;
    }

#ifdef DEBUG_MODE
    emit logMessage(QString::fromUtf8(u8"[DEBUG:SEND] Laser%1 -> %2 (模拟)").arg(laserIndex).arg(QChar(cmd)));
    updateLaserStatusFromSend(laserIndex);
    return true;
#else
    if (!serialPort || !serialPort->isOpen()) {
        emit logMessage(QString::fromUtf8(u8"[WARN] 串口未打开，未发送命令"));
        return false;
    }
    const bool rampControlledSend = isLaserBusy(laserIndex);
    if (!rampControlledSend
        && lastSentTimers[laserIndex - 1].isValid()
        && lastSentTimers[laserIndex - 1].elapsed() < minSendIntervalMs) {
        return false;
    }
    lastSentTimers[laserIndex - 1].restart();

    QByteArray data(1, cmd);
    qint64 bytesWritten = serialPort->write(data);
    if (bytesWritten == -1) {
        emit logMessage(QString::fromUtf8(u8"[ERROR] 发送命令失败: ") + serialPort->errorString());
        if (serialPort->error() == QSerialPort::ResourceError
            || serialPort->error() == QSerialPort::WriteError) {
            serialPort->close();
            resetLaserStates();
            autoReconnectTimer->start(2000);
            emit transportChanged(false, QString());
        }
        return false;
    }

    emit logMessage(QString::fromUtf8(u8"[SEND] Laser%1 -> %2").arg(laserIndex).arg(QChar(cmd)));
    updateLaserStatusFromSend(laserIndex);
    return true;
#endif
}

int LaserController::commandDirectionForLaser(int laserIndex, char cmd) const
{
    switch (laserIndex) {
    case 1:
        if (cmd == '1' || cmd == '3') return +1;
        if (cmd == '0' || cmd == '2') return -1;
        break;
    case 2:
        if (cmd == '4' || cmd == '6') return +1;
        if (cmd == '5' || cmd == '7') return -1;
        break;
    case 3:
        if (cmd == '8') return +1;
        if (cmd == '9') return -1;
        break;
    default:
        break;
    }
    return 0;
}

void LaserController::serialPortReadyRead()
{
    if (!serialPort->isOpen()) return;
    QByteArray chunk = serialPort->readAll();
    if (chunk.isEmpty()) return;

    rxBuffer.append(chunk);
    while (true) {
        int idx = rxBuffer.indexOf('\n');
        if (idx < 0) break;
        QByteArray lineBytes = rxBuffer.left(idx);
        rxBuffer.remove(0, idx + 1);
        if (!lineBytes.isEmpty() && lineBytes.endsWith('\r')) lineBytes.chop(1);

        QString line = decodeSerialData(lineBytes).trimmed();
        if (line.isEmpty()) continue;

        emit logMessage(line);

        QString cleaned = line;
        cleaned.remove('\r');
        cleaned.replace("\n", "");
        cleaned.replace(" ", "");

        bool changed = false;
        if (cleaned.contains(QString::fromUtf8(u8"Laser1:温度就绪")) || cleaned.contains(QString::fromUtf8(u8"Laser1就绪")) ||
            cleaned.toLower().contains("laser1ready") || cleaned.contains("L1:OK")) {
            if (!laser1RawReady) { laser1RawReady = true; changed = true; }
        } else if (cleaned.contains(QString::fromUtf8(u8"Laser1:温度未就绪")) || cleaned.contains(QString::fromUtf8(u8"Laser1未就绪")) ||
                   cleaned.toLower().contains("laser1notready") || cleaned.contains("L1:NG")) {
            if (laser1RawReady) { laser1RawReady = false; changed = true; }
        }
        if (cleaned.contains(QString::fromUtf8(u8"Laser2:温度就绪")) || cleaned.contains(QString::fromUtf8(u8"Laser2就绪")) ||
            cleaned.toLower().contains("laser2ready") || cleaned.contains("L2:OK")) {
            if (!laser2RawReady) { laser2RawReady = true; changed = true; }
        } else if (cleaned.contains(QString::fromUtf8(u8"Laser2:温度未就绪")) || cleaned.contains(QString::fromUtf8(u8"Laser2未就绪")) ||
                   cleaned.toLower().contains("laser2notready") || cleaned.contains("L2:NG")) {
            if (laser2RawReady) { laser2RawReady = false; changed = true; }
        }
        if (cleaned.contains(QString::fromUtf8(u8"Laser3:温度就绪")) || cleaned.contains(QString::fromUtf8(u8"Laser3就绪")) ||
            cleaned.toLower().contains("laser3ready") || cleaned.contains("L3:OK")) {
            if (!laser3RawReady) { laser3RawReady = true; changed = true; }
        } else if (cleaned.contains(QString::fromUtf8(u8"Laser3:温度未就绪")) || cleaned.contains(QString::fromUtf8(u8"Laser3未就绪")) ||
                   cleaned.toLower().contains("laser3notready") || cleaned.contains("L3:NG")) {
            if (laser3RawReady) { laser3RawReady = false; changed = true; }
        }

        if (changed) updateLaserDependencies();
        parseMeasuredFromLine(line);
    }
}

void LaserController::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        if (serialPort->isOpen()) serialPort->close();
        emit logMessage(QString::fromUtf8(u8"[ERROR] 串口设备错误或已被拔出"));
        cancelActiveRamp(QString::fromUtf8(u8"串口异常，缓升/缓降中止"));
        resetLaserStates();
        autoReconnectEnabled = true;
        autoReconnectTimer->start(2000);
        emit transportChanged(false, QString());
    }
}

void LaserController::checkSerialPorts()
{
#ifdef DEBUG_MODE
    emit transportChanged(true, QStringLiteral("DEBUG"));
    return;
#else
    QStringList currentPorts = availablePortNames();
    if (wasOpenedBefore && !currentPorts.contains(lastOpenedPortName)) {
        if (serialPort->isOpen()) {
            serialPort->close();
            emit logMessage(QString::fromUtf8(u8"[WARN] 串口 ") + lastOpenedPortName + QString::fromUtf8(u8" 已断开连接"));
            resetLaserStates();
            emit transportChanged(false, QString());
        }
    }

    if (autoReconnectEnabled && wasOpenedBefore && !serialPort->isOpen()
        && currentPorts.contains(lastOpenedPortName)) {
        autoReconnectTimer->start(2000);
    }

    emit transportChanged(serialPort->isOpen(), serialPort->isOpen() ? lastOpenedPortName : QString());
#endif
}

void LaserController::autoReconnectSerialPort()
{
#ifdef DEBUG_MODE
    autoReconnectTimer->stop();
    return;
#else
    autoReconnectTimer->stop();
    if (!serialPort->isOpen() && !lastOpenedPortName.isEmpty() && autoReconnectEnabled) {
        if (availablePortNames().contains(lastOpenedPortName)) {
            if (openSerial(lastOpenedPortName)) {
                rxBuffer.clear();
            } else {
                autoReconnectTimer->start(2000);
            }
        }
    }
#endif
}

void LaserController::checkLaserStatus()
{
    if (!hasLaserTransport()) return;
    updateLaserDependencies();
}

QString LaserController::decodeSerialData(const QByteArray &data) const
{
    QString text = QString::fromUtf8(data);
    if (text.contains(QChar(0xFFFD))) {
        QTextCodec *gbk = QTextCodec::codecForName("GBK");
        text = gbk ? gbk->toUnicode(data) : QString::fromLocal8Bit(data);
    }
    return text;
}

void LaserController::parseMeasuredFromLine(const QString &line)
{
    QRegularExpression re(QString::fromUtf8(u8"Laser([123])[^\\n]*?输出电流\\s*=\\s*([0-9]+\\.?[0-9]*)\\s*A"));
    QRegularExpressionMatch match = re.match(line);
    if (!match.hasMatch()) return;

    int index = match.captured(1).toInt();
    double mA = match.captured(2).toDouble() * 1000.0;
    setMeasuredLaserMa(index, mA);
}

void LaserController::resetLaserStates()
{
    laser1RawReady = false;
    laser2RawReady = false;
    laser3RawReady = false;
    laser1Ready = false;
    laser2Ready = false;
    laser3Ready = false;
    emitAllReadyStates();
    emit stateChanged();
}

void LaserController::updateLaserDependencies()
{
    laser1Ready = laser1RawReady;
    laser2Ready = laser1Ready && laser2RawReady;
    laser3Ready = laser1Ready && laser2Ready && laser3RawReady;
    emitAllReadyStates();
    emit stateChanged();
}

void LaserController::updateLaserStatusFromSend(int laserIndex)
{
    // 当前协议会把一次成功发送视为对应激光器“已可控”的辅助依据，保持旧逻辑不变。
    setRawReady(laserIndex, true);
    updateLaserDependencies();
}

void LaserController::setCurrentLaserMa(int laserIndex, int value)
{
    switch (laserIndex) {
    case 1: currentLaser1mA = value; break;
    case 2: currentLaser2mA = value; break;
    case 3: currentLaser3mA = value; break;
    default: return;
    }
    emit currentChanged(laserIndex, value);
    emit stateChanged();
}

void LaserController::setMeasuredLaserMa(int laserIndex, double value)
{
    switch (laserIndex) {
    case 1: measuredLaser1mA = value; break;
    case 2: measuredLaser2mA = value; break;
    case 3: measuredLaser3mA = value; break;
    default: return;
    }
    emit measuredChanged(laserIndex, value);
}

void LaserController::setRawReady(int laserIndex, bool ready)
{
    switch (laserIndex) {
    case 1: laser1RawReady = ready; break;
    case 2: laser2RawReady = ready; break;
    case 3: laser3RawReady = ready; break;
    default: return;
    }
}

void LaserController::emitAllReadyStates()
{
    emit readyChanged(1, laser1Ready, laser1RawReady);
    emit readyChanged(2, laser2Ready, laser2RawReady);
    emit readyChanged(3, laser3Ready, laser3RawReady);
}
