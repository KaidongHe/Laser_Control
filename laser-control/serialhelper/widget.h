#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTextCodec>
#include <QElapsedTimer>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QTimer>
#include <QPushButton>
#include <QHBoxLayout>
#include "laserchart.h"

// ===== Debug模式宏定义 =====
// 取消注释下面这行以启用Debug模式
#define DEBUG_MODE

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

    void sendLaserCommand(int laserIndex, char cmd);
    void setLaserReady(int laserIndex, bool ready);

private slots:
    void on_openBt_clicked();
    void on_closeBt_clicked();
    void serialPortReadyRead_Slot();
    void on_sendBt_clicked();
    void on_clearBt_clicked();

    // 步进按钮
    void on_laser1UpBtn_clicked();
    void on_laser1DownBtn_clicked();
    void on_laser2UpBtn_clicked();
    void on_laser2DownBtn_clicked();
    void on_laser3UpBtn_clicked();
    void on_laser3DownBtn_clicked();

    // 模式按钮
    void on_laser1CoarseBtn_clicked();
    void on_laser1FineBtn_clicked();
    void on_laser2CoarseBtn_clicked();
    void on_laser2FineBtn_clicked();

    // spinbox 直接编辑后回填
    void on_laser1spinbox_editingFinished();
    void on_laser2spinbox_editingFinished();
    void on_laser3spinbox_editingFinished();

    // 热插拔
    void checkSerialPorts();
    void autoReconnectSerialPort();
    void handleSerialError(QSerialPort::SerialPortError error);
    void checkLaserStatus();
    void on_tryButton_clicked();
    void tryStep();

private:
    Ui::Widget *ui;
    QSerialPort *serialPort;

    int currentLaser1mA;
    int currentLaser2mA;
    int currentLaser3mA;

    // 实测电流（来自STM32回报，单位 mA），<0 表示尚未收到
    double measuredLaser1mA = -1;
    double measuredLaser2mA = -1;
    double measuredLaser3mA = -1;

    // 当前模式：true=粗调（步长10），false=细调（步长1）
    bool laser1Coarse = true;
    bool laser2Coarse = true;

    QElapsedTimer lastSentTimers[3];
    int minSendIntervalMs = 120;

    QByteArray rxBuffer;

    QTimer *serialCheckTimer;
    QTimer *autoReconnectTimer;
    QTimer *statusCheckTimer;
    QString lastOpenedPortName;
    bool wasOpenedBefore = false;
    bool autoReconnectEnabled = true;

    bool laser1Ready = false;
    bool laser2Ready = false;
    bool laser3Ready = false;

    bool laser1RawReady = false;
    bool laser2RawReady = false;
    bool laser3RawReady = false;

    // TRY扫描
    QTimer *tryTimer;
    enum TryState { TryIdle, TryPhase1, TryPhase2, TryPhase3, TryPhaseL2, TryPhaseL3 };
    TryState tryState = TryIdle;
    int tryCurrent = 0;            // L1 扫描的当前电流
    int tryL2Current = 0;          // L2 扫描的当前电流
    int tryL3Current = 800;        // L3 扫描的当前电流
    int tryPhase1TimeSec = 90;
    int tryPhase2TimeSec = 90;
    int tryPhase3TimeSec = 90;
    int tryStepSize = 10;
    int tryFinalCurrent = 98;
    // L2 扫描参数
    int tryL2TargetMA = 460;       // L2 终点电流
    int tryL2StepSize = 10;        // L2 步长
    int tryL2TimeSec = 30;         // L2 扫描时长
    // L3 扫描参数（硬件单步约 100 mA）
    int tryL3TargetMA = 5000;      // L3 终点电流
    int tryL3StepSize = 100;       // L3 步长（硬件最小）
    int tryL3TimeSec = 60;         // L3 扫描时长

    LaserChart *chart1;
    LaserChart *chart2;
    LaserChart *chart3;

    QString decodeSerialData(const QByteArray &data);
    void refreshSerialPortList();
    bool isTargetPort(const QString &portName);
    void resetLaserStates();
    void updateLaserDependencies();
    bool canControlLaser(int laserIndex);
    void updateAllLaserStates();
    void updateLaserStatusFromSend(int laserIndex);
    void tryAutoActivateLasers();

    // ===== 新增：可视化辅助 =====
    void adjustLaser(int laserIndex, int direction);   // +1 / -1
    void updateLaserVisual(int laserIndex);            // 刷新读数、按钮可用、提示
    void setLaser1Mode(bool coarse);
    void setLaser2Mode(bool coarse);
    void parseMeasuredFromLine(const QString &line);
    QString blockReason(int laserIndex) const;
};

#endif // WIDGET_H
