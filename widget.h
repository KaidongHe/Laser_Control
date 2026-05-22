#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QTimer>
#include <QPushButton>
#include <QHBoxLayout>
#include "lasercontroller.h"
#include "laserchart.h"

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(LaserController *controller = nullptr, QWidget *parent = 0);
    ~Widget();

    void setLaserReady(int laserIndex, bool ready);

private slots:
    void on_openBt_clicked();
    void on_closeBt_clicked();
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

    void on_laser1ParamsButton_clicked();
    void on_laser2ParamsButton_clicked();
    void on_laser3ParamsButton_clicked();
    void on_temperatureBypassButton_clicked();
    void on_tryButton_clicked();
    void tryStep();

private:
    Ui::Widget *ui;
    LaserController *controller = nullptr;
    bool ownsController = false;

    int currentLaser1mA;
    int currentLaser2mA;
    int currentLaser3mA;

    // 实测电流（来自STM32回报，单位 mA），<0 表示尚未收到
    double measuredLaser1mA = -1;
    double measuredLaser2mA = -1;
    double measuredLaser3mA = -1;

    // 当前模式：true=粗调（步长10），false=细调（步长1）；L1 默认使用 1 mA 细调。
    bool laser1Coarse = false;
    bool laser2Coarse = true;

    bool laser1Ready = false;
    bool laser2Ready = false;
    bool laser3Ready = false;

    bool laser1RawReady = false;
    bool laser2RawReady = false;
    bool laser3RawReady = false;

    // TRY扫描
    QTimer *tryTimer;
    QTimer *visualRefreshTimer = nullptr; // 合并同一轮事件循环内的多次界面刷新，减少扫描时按钮重绘闪烁
    enum TryState {
        TryIdle,
        TryPhase1,
        TryPhase2,
        TryPhase3,
        TryPhaseL2Start1,
        TryPhaseL2Start2,
        TryPhaseL2Start3,
        TryPhaseL2,
        TryPhaseL3
    };
    TryState tryState = TryIdle;
    int tryCurrent = 0;            // L1 扫描的当前电流
    int tryL2Current = 0;          // L2 扫描的当前电流
    int tryL3Current = 800;        // L3 扫描的当前电流
    int tryPhase1TimeSec = 90;
    int tryPhase2TimeSec = 90;
    int tryPhase3TimeSec = 90;
    int tryStepSize = 1;
    int tryL1HighCurrent = 850;
    int tryL1MiddleCurrent = 200;
    int tryFinalCurrent = 98;
    // L2 启动曲线和普通操作员页面共用；完成后才进入 L2 额外扫描目标。
    int tryL2HighCurrent = 850;
    int tryL2MiddleCurrent = 200;
    int tryL2FinalCurrent = LaserController::L2_ENABLE_L3_MA;
    int tryL2Phase1TimeSec = 40;
    int tryL2Phase2TimeSec = 15;
    int tryL2Phase3TimeSec = 5;
    int tryL2StartupStepSize = 10;
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

    void refreshSerialPortList();
    void resetLaserStates();
    bool canAdjustLaser(int laserIndex, int direction) const;       // 按升高/降低方向判断顺序联锁

    // ===== 新增：可视化辅助 =====
    void adjustLaser(int laserIndex, int direction);   // +1 / -1
    void updateLaserVisual(int laserIndex);            // 刷新读数、按钮可用、提示
    void updateAllLaserVisuals();                      // 三路联锁互相影响，电流变化后统一刷新全部按钮和提示
    void setLaser1Mode(bool coarse);
    void doUpdateAllLaserVisuals();                    // 定时器真正执行的刷新入口，避免一次串口/ramp 信号触发多次重绘
    void setLaser2Mode(bool coarse);
    void applyDeveloperRuntimeParams(const LaserController::DeveloperRuntimeParams &params);
    bool canEditDeveloperParams() const;
    bool saveLaserParamsFromDialog(int laserIndex, const LaserController::DeveloperRuntimeParams &params);
    void updateTemperatureBypassUi();
    QString blockReason(int laserIndex) const;
    QString adjustBlockReason(int laserIndex, int direction) const; // 返回更具体的顺序联锁阻塞原因
    bool hasLaserTransport() const;                                 // Debug 模式下模拟串口可用，真实模式下检查串口
};

#endif // WIDGET_H
