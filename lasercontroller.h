#ifndef LASERCONTROLLER_H
#define LASERCONTROLLER_H

#include <QObject>
#include <QElapsedTimer>
#include <QSerialPort>
#include <QStringList>

// ===== Debug模式宏定义 =====
// 控制核心也使用同一 Debug 开关，避免界面和控制层对“是否需要真实串口”的理解不一致。
#define DEBUG_MODE

class QTimer;

class LaserController : public QObject
{
    Q_OBJECT

public:
    explicit LaserController(QObject *parent = nullptr);

    enum {
        L1_ENABLE_L2_MA = 90,
        L2_ENABLE_L3_MA = 90,
        L2_SAFE_OFF_MA = 0,
        L3_SAFE_OFF_MA = 800
    };

    struct DeveloperRuntimeParams
    {
        int operatorL1FinalMa = 98;
        int operatorL2FinalMa = 460;
        int l3OperatorMaxMa = 5000;

        int startupL1HighMa = 850;
        int startupL1MiddleMa = 200;
        int startupL1Phase1TimeSec = 90;
        int startupL1Phase2TimeSec = 90;
        int startupL1Phase3TimeSec = 90;
        int startupL1StepMa = 1;
        int startupL2StepMa = 10;
        int startupL2Phase1TimeSec = 30;   // L2 单段缓升总时长

        // L1 TRY 启动参数已经统一到 startupL1*，保留这些字段用于兼容旧界面变量。
        int tryL1Phase1TimeSec = 90;
        int tryL1Phase2TimeSec = 90;
        int tryL1Phase3TimeSec = 90;
        int tryL1StepMa = 1;
        int tryL1FinalMa = 98;
        int tryL2TargetMa = 460;
        int tryL2StepMa = 10;
        int tryL2TimeSec = 30;
        int tryL3TargetMa = 5000;
        int tryL3StepMa = 100;
        int tryL3TimeSec = 60;
    };

    QStringList availablePortNames() const;
    bool openSerial(const QString &portName);
    void closeSerial();
    bool hasLaserTransport() const;
    bool isSerialOpen() const;
    QString currentPortName() const;

    int currentLaserMa(int laserIndex) const;
    double measuredLaserMa(int laserIndex) const;
    bool laserReady(int laserIndex) const;
    bool laserRawReady(int laserIndex) const;
    bool isLaserBusy(int laserIndex) const;
    bool isAnyLaserBusy() const;

    bool canAdjustLaser(int laserIndex, int direction) const;
    QString adjustBlockReason(int laserIndex, int direction) const;

    int l1EnableL2Ma() const;
    int l2EnableL3Ma() const;
    int l1FinalMa() const;
    int l2FinalMa() const;
    int l2SafeOffMa() const;
    int l3SafeOffMa() const;
    int laserMinMa(int laserIndex) const;
    int laserMaxMa(int laserIndex) const;
    int laserStepMa(int laserIndex, bool coarseMode) const;
    int operatorPowerMinPercent() const;
    int operatorPowerMaxPercent() const;
    int operatorPowerPercentToMa(int percent) const;
    int operatorPowerMaToPercent(int currentMa) const;
    bool temperatureReadyBypassEnabled() const;
    DeveloperRuntimeParams developerRuntimeParams() const;

    bool adjustLaser(int laserIndex, int direction, bool coarseMode);
    bool setLaserTarget(int laserIndex, int target, bool coarseMode, int durationMs = -1);
    bool requestOperatorSwitch(int laserIndex, bool on, QString *reason = nullptr);
    bool requestOperatorPowerPercent(int percent, QString *reason = nullptr);
    bool setTemperatureReadyBypassEnabled(bool enabled, QString *error = nullptr);
    bool saveDeveloperLaserParameters(int laserIndex, const DeveloperRuntimeParams &params, QString *error = nullptr);

signals:
    void logMessage(const QString &message);
    void currentChanged(int laserIndex, int currentMa);
    void measuredChanged(int laserIndex, double measuredMa);
    void readyChanged(int laserIndex, bool ready, bool rawReady);
    void stateChanged();
    void transportChanged(bool opened, const QString &portName);
    void busyChanged(int laserIndex, bool busy);
    void operationStarted(int laserIndex, int targetMa);
    void operationProgress(int laserIndex, int currentMa, int targetMa);
    void operationFinished(int laserIndex, bool success, const QString &message);

private slots:
    void serialPortReadyRead();
    void handleSerialError(QSerialPort::SerialPortError error);
    void checkSerialPorts();
    void autoReconnectSerialPort();
    void checkLaserStatus();
    void processRampStep();

private:
    bool sendLaserCommand(int laserIndex, char cmd);
    bool startRampToTarget(int laserIndex, int target, bool coarseMode, int durationMs = -1);
    bool startOperatorSoftOn(int laserIndex);
    bool continueOperatorSoftOn(int finishedLaser);
    void finishRamp(bool success, const QString &message);
    void cancelActiveRamp(const QString &message);
    QString configFilePath() const;
    void saveDefaultConfigIfMissing() const;
    void loadConfig();
    char commandForStep(int laserIndex, int direction, bool coarseMode) const;
    int rampStepSize(int laserIndex, int diff, bool coarseMode) const;
    int rampIntervalForTarget(int laserIndex, int target, bool coarseMode, int durationMs) const;
    int startupHighMa(int laserIndex) const;
    int startupMiddleMa(int laserIndex) const;
    int startupFinalMa(int laserIndex) const;
    int startupRiseDurationMs(int laserIndex) const;
    int startupFallMiddleDurationMs(int laserIndex) const;
    int startupFallFinalDurationMs(int laserIndex) const;
    int startupStepMa(int laserIndex) const;
    bool startupCoarseMode(int laserIndex) const;
    int commandDirectionForLaser(int laserIndex, char cmd) const;
    bool laserReadyForStartup(int laserIndex) const;
    QString decodeSerialData(const QByteArray &data) const;
    void parseMeasuredFromLine(const QString &line);
    void resetLaserStates();
    void updateLaserDependencies();
    void updateLaserStatusFromSend(int laserIndex);
    void setCurrentLaserMa(int laserIndex, int value);
    void setMeasuredLaserMa(int laserIndex, double value);
    void setRawReady(int laserIndex, bool ready);
    void emitAllReadyStates();

    QSerialPort *serialPort = nullptr;
    QTimer *serialCheckTimer = nullptr;
    QTimer *autoReconnectTimer = nullptr;
    QTimer *statusCheckTimer = nullptr;
    QTimer *rampTimer = nullptr;

    struct LaserConfig
    {
        int l1EnableL2Ma = L1_ENABLE_L2_MA;
        int l2EnableL3Ma = L2_ENABLE_L3_MA;
        int l2SafeOffMa = L2_SAFE_OFF_MA;
        int l3SafeOffMa = L3_SAFE_OFF_MA;

        int laser12MaxMa = 1000;
        int laser3MaxMa = 10000;

        int startupL1HighMa = 850;
        int startupL1MiddleMa = 200;
        int startupL1FinalMa = 98;
        int startupL1RiseDurationMs = 90000;
        int startupL1FallMiddleDurationMs = 90000;
        int startupL1FallFinalDurationMs = 90000;
        int startupL1StepMa = 1;

        int startupL2HighMa = 850;
        int startupL2MiddleMa = 200;
        int startupL2FinalMa = 460;
        int startupL2RiseDurationMs = 30000;
        int startupL2FallMiddleDurationMs = 15000;
        int startupL2FallFinalDurationMs = 5000;
        int startupL2StepMa = 10;

        int l1CoarseStepMa = 10;
        int l1FineStepMa = 1;
        int l2CoarseStepMa = 10;
        int l2FineStepMa = 1;
        int l3StepMa = 100;

        int l3MinPercent = 2;
        int l3MaxPercent = 100;
        int l3MinMa = L3_SAFE_OFF_MA;
        int l3MaxMa = 5000;

        int defaultRampIntervalMs = 67;
        int minRampIntervalMs = 67;
        int minManualSendIntervalMs = 67;
        bool temperatureReadyBypass = false;

        int tryL1Phase1TimeSec = 90;
        int tryL1Phase2TimeSec = 90;
        int tryL1Phase3TimeSec = 90;
        int tryL1StepMa = 1;
        int tryL1FinalMa = 98;
        int tryL2TargetMa = 460;
        int tryL2StepMa = 10;
        int tryL2TimeSec = 30;
        int tryL3TargetMa = 5000;
        int tryL3StepMa = 100;
        int tryL3TimeSec = 60;
    };

    // 安全阈值、缓升时间和普通页面 L3 百分比映射从启动配置读取；协议步长会在 loadConfig() 中强制对齐到 STM32 指令表。
    LaserConfig cfg;

    int currentLaser1mA = 0;
    int currentLaser2mA = 0;
    int currentLaser3mA = L3_SAFE_OFF_MA;

    double measuredLaser1mA = -1;
    double measuredLaser2mA = -1;
    double measuredLaser3mA = -1;

    bool laser1RawReady = false;
    bool laser2RawReady = false;
    bool laser3RawReady = false;
    bool laser1Ready = false;
    bool laser2Ready = false;
    bool laser3Ready = false;

    QElapsedTimer lastSentTimers[3];
    QElapsedTimer globalSendTimer; // 全局串口发送硬限速：任意两条真实命令之间至少间隔约 67ms，避免跨通道叠加超 15Hz
    int minSendIntervalMs = 120;
    int rampIntervalMs = 100;
    QByteArray rxBuffer;

    int rampLaserIndex = 0;
    int rampTargetMa = 0;
    bool rampCoarseMode = true;

    enum OperatorSoftPhase {
        SoftIdle,
        SoftRiseHigh,
        SoftFallMiddle,
        SoftFallFinal
    };
    OperatorSoftPhase operatorSoftPhase = SoftIdle;
    int operatorSoftLaserIndex = 0;
    int operatorSoftFinalMa = 0;

    QString lastOpenedPortName;
    bool wasOpenedBefore = false;
    bool autoReconnectEnabled = true;
};

#endif // LASERCONTROLLER_H
