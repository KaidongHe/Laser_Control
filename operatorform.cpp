#include "operatorform.h"
#include "lasercontroller.h"
#include "ui_operatorform.h"
#include "widget.h"

#include <QColor>
#include <QComboBox>
#include <QDebug>
#include <QFontMetrics>
#include <QGraphicsDropShadowEffect>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSizePolicy>
#include <QSpinBox>
#include <QStringList>
#include <QStyle>

namespace {
const char *DEVELOPER_PASSWORD = "laser2026";

void applySoftShadow(QWidget *widget, int blurRadius, int offsetY, int alpha)
{
    if (!widget) return;
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blurRadius);
    shadow->setOffset(0, offsetY);
    shadow->setColor(QColor(0, 0, 0, alpha));
    widget->setGraphicsEffect(shadow);
}

int longestLineWidth(const QFontMetrics &fm, const QString &text)
{
    int width = 0;
    const QStringList lines = text.split('\n');
    for (const QString &line : lines) {
        width = qMax(width, fm.horizontalAdvance(line));
    }
    return width;
}

void keepButtonTextReadable(QPushButton *button, int minWidth)
{
    if (!button) return;
    const int textWidth = longestLineWidth(QFontMetrics(button->font()), button->text());
    button->setMinimumWidth(qMax(button->minimumWidth(), qMax(minWidth, textWidth + 34)));
    QSizePolicy policy = button->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::MinimumExpanding);
    button->setSizePolicy(policy);
}

void keepWidgetReadable(QWidget *widget, int minWidth)
{
    if (!widget) return;
    widget->setMinimumWidth(qMax(widget->minimumWidth(), minWidth));
    QSizePolicy policy = widget->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    widget->setSizePolicy(policy);
}
}

operatorForm::operatorForm(QWidget *parent) :
    operatorForm(nullptr, parent)
{
}

operatorForm::operatorForm(LaserController *sharedController, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::operatorForm),
    controller(sharedController ? sharedController : new LaserController(this)),
    ownsController(sharedController == nullptr)
{
    ui->setupUi(this);
    setWindowTitle(QStringLiteral("Laser Control System"));
    keepButtonTextReadable(ui->seedButton, 260);
    keepButtonTextReadable(ui->preReleaseButton, 260);
    keepButtonTextReadable(ui->serialConnectButton, 82);
    keepWidgetReadable(ui->serialComboBox, 150);
    keepWidgetReadable(ui->powerSpinBox, 220);
    ui->statusDisplayLabel->setWordWrap(true);

    // 串口连接控件已经放在 operatorform.ui 中；这里只做初始化和后续状态刷新，控制仍统一交给 LaserController。
    setupSerialControls();

    // 给普通操作员页面的主要控件加柔和阴影，减少默认 Qt 控件感。
    applySoftShadow(ui->seedButton, 24, 6, 40);
    applySoftShadow(ui->preReleaseButton, 24, 6, 40);
    applySoftShadow(ui->powerSpinBox, 18, 4, 34);
    applySoftShadow(ui->serialConnectButton, 10, 2, 18);
    ui->developerButton->setFlat(true);
    ui->developerButton->setText(QString::fromUtf8(u8"⚙ 开发者"));
    // 百分号由 PillSpinBox 自绘，不放进可编辑文本，避免光标跑到 % 后面。
    ui->powerSpinBox->setSuffix(QString());
    ui->powerSpinBox->setButtonSymbols(QAbstractSpinBox::NoButtons);
    // 普通页面百分比范围来自配置文件，默认仍是 2%~100%。
    ui->powerSpinBox->setRange(controller->operatorPowerMinPercent(), controller->operatorPowerMaxPercent());
    // 普通页面显示当前实际百分比，因此 +/- 一次至少跨过一个 L3 硬件电流档位，避免按下后又回显到原值。
    ui->powerSpinBox->setSingleStep(controller->operatorPowerPercentStep());
    // 关闭键盘实时跟踪：手动输入完成后再提交，点击 +/- 仍会立即提交。
    ui->powerSpinBox->setKeyboardTracking(false);

    ui->powerSpinBox->setStyleSheet(
        "QSpinBox {"
        "    font-family: Arial;"
        "    font-size: 34px;"
        "    font-weight: bold;"
        "    color: #2E5CB8;"
        "    background: white;"
        "    border: 3px solid #B7C9F2;"
        "    border-radius: 36px;"
        "    padding: 8px 24px;"
        "    qproperty-alignment: AlignCenter;"
        "}"
        "QSpinBox:focus {"
        "    border: 3px solid #2E78FF;"
        "}"
        "QLineEdit {"
        "    background: transparent;"
        "    border: none;"
        "    selection-background-color: rgba(46, 92, 184, 35);"
        "    selection-color: #2E5CB8;"
        "}"
    );
    // 按钮“已开启”状态按普通页面最终目标判断，而不是只按联锁阈值提前变绿。
    seedEnabled = controller->currentLaserMa(1) >= controller->l1FinalMa();
    preReleaseEnabled = controller->currentLaserMa(2) >= controller->l2FinalMa();
    updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8(u8"L1 种子开/关"), l1Busy);
    updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8(u8"L2 预放开/关"), l2Busy);
    syncPowerSpinBoxFromLaser3();

    connect(ui->seedButton, &QPushButton::clicked, this, &operatorForm::toggleSeed);
    connect(ui->preReleaseButton, &QPushButton::clicked, this, &operatorForm::togglePreRelease);
    connect(ui->powerSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int) {
        applyPowerPercent();
    });
    connect(ui->developerButton, &QPushButton::clicked, this, &operatorForm::openDeveloperWindow);
    connect(ui->serialConnectButton, &QPushButton::clicked, this, &operatorForm::toggleSerialConnection);
    connect(controller, &LaserController::currentChanged, this, [this](int laserIndex, int currentMa) {
        // 缓升/缓降过程中不提前改变“已开启/已关闭”状态，等 operationFinished 后再切换颜色。
        if (laserIndex == 1) {
            if (!l1Busy) seedEnabled = currentMa >= controller->l1FinalMa();
            updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8(u8"L1 开/关（种子）"), l1Busy);
        } else if (laserIndex == 2) {
            if (!l2Busy) preReleaseEnabled = currentMa >= controller->l2FinalMa();
            updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8(u8"预放开/关"), l2Busy);
        } else if (laserIndex == 3) {
            // 普通页面也显示当前实际设定百分比，L3 ramp 每步变化时都实时回显。
            syncPowerSpinBoxFromLaser3();
        }
        updateOperatorLockState();
    });
    connect(controller, &LaserController::operationStarted, this, [this](int laserIndex, int) {
        if (laserIndex == 1) {
            l1Busy = true;
            updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8(u8"L1 开/关（种子）"), l1Busy);
        } else if (laserIndex == 2) {
            l2Busy = true;
            updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8(u8"预放开/关"), l2Busy);
        } else if (laserIndex == 3) {
            l3Busy = true;
        }
        updateOperatorLockState();
    });
    connect(controller, &LaserController::operationFinished, this, [this](int laserIndex, bool success, const QString &) {
        if (laserIndex == 1) {
            l1Busy = false;
            // 只有成功到达目标后才切换开关颜色；中途失败时保留原状态，避免误判为已关闭。
            if (success) seedEnabled = controller->currentLaserMa(1) >= controller->l1FinalMa();
            updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8(u8"L1 开/关（种子）"), l1Busy);
        } else if (laserIndex == 2) {
            l2Busy = false;
            // 只有成功到达目标后才切换开关颜色；中途失败时保留原状态，避免误判为已关闭。
            if (success) preReleaseEnabled = controller->currentLaserMa(2) >= controller->l2FinalMa();
            updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8(u8"预放开/关"), l2Busy);
        } else if (laserIndex == 3) {
            l3Busy = false;
            Q_UNUSED(success);
            syncPowerSpinBoxFromLaser3();
        }
        updateOperatorLockState();
    });
    connect(controller, &LaserController::stateChanged, this, &operatorForm::updateOperatorLockState);
    connect(controller, &LaserController::transportChanged, this, [this](bool opened, const QString &portName) {
        refreshOperatorSerialPorts();
        updateOperatorSerialUi(opened, portName);
        updateOperatorLockState();
    });
    connect(controller, &LaserController::lowerDeviceStateSyncChanged, this, [this](bool) {
        updateOperatorSerialUi(controller->isSerialOpen(), controller->currentPortName());
        updateOperatorLockState();
    });
#ifdef QT_DEBUG
    // 普通操作员页面在 Qt Debug 构建下同步输出控制核心日志，便于不打开开发者窗口也能看串口收发和联锁提示。
    connect(controller, &LaserController::logMessage, this, [](const QString &msg) {
        qDebug().noquote() << "[SERIAL]" << msg;
    });
#endif
    refreshOperatorSerialPorts();
    updateOperatorSerialUi(controller->isSerialOpen(), controller->currentPortName());
    updateOperatorLockState();
}

operatorForm::~operatorForm()
{
    if (developerWindow) {
        developerWindow->deleteLater();
        developerWindow = nullptr;
    }
    delete ui;
}

void operatorForm::setupSerialControls()
{
    // 连接区现在由 .ui 固定管理，避免 C++ 运行时按 grid 行号插入控件导致后续维护困难。
    ui->serialComboBox->setToolTip(QString::fromUtf8(u8"选择普通模式要连接的串口"));
    ui->serialConnectButton->setCursor(Qt::PointingHandCursor);
    ui->serialStatusLabel->setAlignment(Qt::AlignCenter);
    updateStatusDisplay();
}

void operatorForm::refreshOperatorSerialPorts()
{
    if (!ui->serialComboBox || !controller) return;

    const QString previous = ui->serialComboBox->currentText();
    const QString currentPort = controller->currentPortName();
    const QSignalBlocker blocker(ui->serialComboBox);

    ui->serialComboBox->clear();
    const QStringList ports = controller->availablePortNames();
    for (const QString &portName : ports) {
        ui->serialComboBox->addItem(portName);
    }

    // 已连接的端口即使短暂热插拔不在列表中，也保留显示，便于操作员知道当前状态来自哪一路。
    if (!currentPort.isEmpty() && ui->serialComboBox->findText(currentPort) < 0) {
        ui->serialComboBox->addItem(currentPort);
    }

    int index = ui->serialComboBox->findText(previous);
    if (index < 0 && !currentPort.isEmpty()) {
        index = ui->serialComboBox->findText(currentPort);
    }
    if (index >= 0) {
        ui->serialComboBox->setCurrentIndex(index);
    }
}

void operatorForm::updateOperatorSerialUi(bool opened, const QString &portName)
{
    if (!ui->serialComboBox || !ui->serialConnectButton || !ui->serialStatusLabel) return;

    ui->serialComboBox->setEnabled(!opened);
    ui->serialConnectButton->setText(opened ? QString::fromUtf8(u8"断开") : QString::fromUtf8(u8"连接"));
#ifdef DEBUG_MODE
    ui->serialConnectButton->setEnabled(true);
#else
    // 真实串口模式下没有可选端口时禁用连接按钮，避免操作员误以为已经可以控制硬件。
    ui->serialConnectButton->setEnabled(opened || ui->serialComboBox->count() > 0);
#endif
    ui->serialConnectButton->setToolTip(opened ? QString::fromUtf8(u8"断开当前串口连接")
                                               : (ui->serialComboBox->count() > 0
                                                  ? QString::fromUtf8(u8"连接所选串口")
                                                  : QString::fromUtf8(u8"未检测到可用串口")));
    ui->serialConnectButton->setStyleSheet(opened
        ? "QPushButton { background: #197A50; color: white; border: none; border-radius: 6px; padding: 5px 14px; font-family: 'Microsoft YaHei'; font-size: 12px; font-weight: 600; }"
          "QPushButton:hover { background: #166B46; }"
          "QPushButton:disabled { background: #C8D0DC; color: #F5F7FA; }"
        : "QPushButton { background: #2F6FD6; color: white; border: none; border-radius: 6px; padding: 5px 14px; font-family: 'Microsoft YaHei'; font-size: 12px; font-weight: 600; }"
          "QPushButton:hover { background: #245FBD; }"
          "QPushButton:disabled { background: #C8D0DC; color: #F5F7FA; }");

    const QString displayPort = portName.isEmpty() ? QStringLiteral("-") : portName;
    const QString statusText = opened
            ? QString::fromUtf8(u8"● 已连接 %1").arg(displayPort)
            : QString::fromUtf8(u8"● 未连接");
    ui->serialStatusLabel->setText(statusText);
    ui->serialStatusLabel->setToolTip(opened
        ? QString::fromUtf8(u8"串口已连接；下位机状态显示在状态显示栏")
        : QString());
    ui->serialStatusLabel->setStyleSheet(opened
        ? "QLabel { color: #197A50; font-family: 'Microsoft YaHei'; font-size: 12px; font-weight: 600; }"
        : "QLabel { color: #9AA4B2; font-family: 'Microsoft YaHei'; font-size: 12px; font-weight: 600; }");
}

void operatorForm::toggleSerialConnection()
{
    if (!controller) return;

    if (controller->isSerialOpen()) {
        // 断开串口同样交给控制核心，控制核心会取消正在进行的缓升/缓降并复位状态。
        controller->closeSerial();
        return;
    }

    QString portName = ui->serialComboBox ? ui->serialComboBox->currentText() : QString();
#ifdef DEBUG_MODE
    if (portName.isEmpty()) portName = QStringLiteral("DEBUG");
#else
    if (portName.isEmpty()) {
        QMessageBox::warning(this,
                             QString::fromUtf8(u8"串口未选择"),
                             QString::fromUtf8(u8"请先选择要连接的串口。"));
        return;
    }
#endif

    // 普通模式连接按钮只负责发起连接；成功后的按钮状态由 transportChanged 信号统一刷新。
    if (!controller->openSerial(portName)) {
        QMessageBox::critical(this,
                              QString::fromUtf8(u8"连接失败"),
                              QString::fromUtf8(u8"无法打开串口，请检查端口是否被占用或查看日志。"));
    }
}

void operatorForm::toggleSeed()
{
    if (l1Busy || controller->isAnyLaserBusy()) {
        // 缓升/缓降期间禁止重复点击，按钮文字会持续提示当前操作正在进行。
        return;
    }

    const bool requested = !seedEnabled;
    QString reason;
    // 普通页面只向控制核心发出请求；成功后才更新按钮状态，避免 UI 显示和真实控制状态不一致。
    if (!controller->requestOperatorSwitch(1, requested, &reason)) {
        QMessageBox::warning(this, QString::fromUtf8(u8"L1 操作被拒绝"),
                             reason.isEmpty() ? QString::fromUtf8(u8"当前不满足 L1 操作条件。") : reason);
        updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8(u8"L1 开/关（种子）"), l1Busy);
        updateOperatorLockState();
        return;
    }

    // 请求被受理后等待 operationFinished，再把按钮切到已开启/已关闭。
    updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8(u8"L1 开/关（种子）"), l1Busy);
    updateOperatorLockState();
}

void operatorForm::togglePreRelease()
{
    if (l2Busy || controller->isAnyLaserBusy()) {
        // 任一路正在缓升/缓降时，普通页面不允许再发新的开关请求。
        return;
    }

    if (!seedEnabled && !preReleaseEnabled) {
        // UI 正常会把 L2 按钮灰掉；这里作为键盘快捷键或程序触发时的兜底提示。
        QMessageBox::warning(this, QString::fromUtf8(u8"预放操作被拒绝"),
                             QString::fromUtf8(u8"请先开启 L1，再开启预放 L2。"));
        updateOperatorLockState();
        return;
    }

    const bool requested = !preReleaseEnabled;
    QString reason;
    // 预放开关对应 L2，同样由控制核心统一执行顺序联锁和串口发送。
    if (!controller->requestOperatorSwitch(2, requested, &reason)) {
        QMessageBox::warning(this, QString::fromUtf8(u8"预放操作被拒绝"),
                             reason.isEmpty() ? QString::fromUtf8(u8"当前不满足 L2 操作条件。") : reason);
        updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8(u8"预放开/关"), l2Busy);
        updateOperatorLockState();
        return;
    }

    // 请求被受理后保持原状态颜色，直到 L2 真正缓升/缓降到目标值。
    updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8(u8"预放开/关"), l2Busy);
    updateOperatorLockState();
}

void operatorForm::applyPowerPercent()
{
    QString reason;
    const int percent = ui->powerSpinBox->value();

    // 普通页面只提交百分比请求，百分比到 L3 mA 的换算和顺序联锁都交给控制核心。
    if (!controller->requestOperatorPowerPercent(percent, &reason)) {
        QMessageBox::warning(this, QString::fromUtf8(u8"L3 功率设置被拒绝"),
                             reason.isEmpty() ? QString::fromUtf8(u8"请先按顺序开启 L1 和预放 L2。") : reason);
        syncPowerSpinBoxFromLaser3();
    } else {
        requestedPowerPercent = qBound(controller->operatorPowerMinPercent(),
                                       percent,
                                       controller->operatorPowerMaxPercent());
        QSignalBlocker blocker(ui->powerSpinBox);
        ui->powerSpinBox->setValue(requestedPowerPercent);
    }
    updateOperatorLockState();
}

void operatorForm::openDeveloperWindow()
{
    if (developerWindow) {
        hide();
        developerWindow->show();
        developerWindow->raise();
        developerWindow->activateWindow();
        return;
    }

    bool ok = false;
    const QString password = QInputDialog::getText(
        this,
        QString::fromUtf8(u8"开发者验证"),
        QString::fromUtf8(u8"请输入开发者密码："),
        QLineEdit::Password,
        QString(),
        &ok);
    if (!ok) {
        return;
    }

    if (password != QString::fromLatin1(DEVELOPER_PASSWORD)) {
        QMessageBox::warning(this, QString::fromUtf8(u8"密码错误"), QString::fromUtf8(u8"开发者密码不正确。"));
        return;
    }

    // 开发者窗口复用同一个控制核心，只负责显示和调试，不再自己维护另一套控制状态。
    developerWindow = new Widget(controller);
    developerWindow->setAttribute(Qt::WA_DeleteOnClose);
    developerWindow->setAttribute(Qt::WA_QuitOnClose, false);
    connect(developerWindow, &QObject::destroyed, this, &operatorForm::handleDeveloperWindowClosed);
    hide();
    developerWindow->show();
}

void operatorForm::handleDeveloperWindowClosed()
{
    developerWindow = nullptr;
    show();
    raise();
    activateWindow();
}

void operatorForm::updateToggleButton(QPushButton *button, bool enabled, const QString &label, bool busy)
{
    if (!button) return;

    QString stateText;
    if (busy) {
        // busy 时保留原 active 颜色含义：原本已开就是正在关闭，原本已关就是正在开启。
        stateText = enabled ? QString::fromUtf8(u8"正在关闭...") : QString::fromUtf8(u8"正在开启...");
    } else {
        stateText = enabled ? QString::fromUtf8(u8"已开启") : QString::fromUtf8(u8"已关闭");
    }

    button->setText(QString::fromUtf8(u8"%1\n%2").arg(label, stateText));
    button->setProperty("active", enabled);
    button->setProperty("busy", busy);
    button->style()->unpolish(button);
    button->style()->polish(button);
}

void operatorForm::updateOperatorLockState()
{
    if (!controller) return;

    const bool hasTransport = controller->hasLaserTransport();
    const bool anyBusy = controller->isAnyLaserBusy() || l1Busy || l2Busy || l3Busy;
    updateStatusDisplay();

    // L1 也必须跟随关机顺序联锁：如果 L2/L3 还没有回到安全态，普通页面不能先关闭 L1。
    const bool canRequestL1On = !seedEnabled && controller->canAdjustLaser(1, +1);
    const bool canRequestL1Off = seedEnabled && controller->canAdjustLaser(1, -1);
    const bool l1Allowed = hasTransport && !anyBusy && (canRequestL1On || canRequestL1Off);
    ui->seedButton->setEnabled(l1Allowed);
    ui->seedButton->setCursor(l1Allowed ? Qt::PointingHandCursor : Qt::ForbiddenCursor);
    if (!hasTransport) {
        ui->seedButton->setToolTip(QString::fromUtf8(u8"请先连接串口"));
    } else if (l1Busy) {
        ui->seedButton->setToolTip(seedEnabled ? QString::fromUtf8(u8"L1 正在关闭，请等待完成")
                                               : QString::fromUtf8(u8"L1 正在开启，请等待完成"));
    } else if (anyBusy) {
        ui->seedButton->setToolTip(QString::fromUtf8(u8"当前有激光器正在缓升/缓降，请等待完成"));
    } else if (seedEnabled && !canRequestL1Off) {
        ui->seedButton->setToolTip(controller->adjustBlockReason(1, -1));
    } else if (!seedEnabled && !canRequestL1On) {
        ui->seedButton->setToolTip(controller->adjustBlockReason(1, +1));
    } else {
        ui->seedButton->setToolTip(QString());
    }

    // L2 同样按当前按钮状态区分“请求开启”和“请求关闭”，避免绕过 L3 -> L2 -> L1 的关机顺序。
    const bool canRequestL2On = !preReleaseEnabled && controller->canAdjustLaser(2, +1);
    const bool canRequestL2Off = preReleaseEnabled && controller->canAdjustLaser(2, -1);
    const bool l2Allowed = hasTransport && !anyBusy && (canRequestL2On || canRequestL2Off);
    ui->preReleaseButton->setEnabled(l2Allowed);
    ui->preReleaseButton->setCursor(l2Allowed ? Qt::PointingHandCursor : Qt::ForbiddenCursor);
    if (!hasTransport) {
        ui->preReleaseButton->setToolTip(QString::fromUtf8(u8"请先连接串口"));
    } else if (l2Busy) {
        ui->preReleaseButton->setToolTip(preReleaseEnabled ? QString::fromUtf8(u8"L2 正在关闭，请等待完成")
                                                           : QString::fromUtf8(u8"L2 正在开启，请等待完成"));
    } else if (anyBusy) {
        ui->preReleaseButton->setToolTip(QString::fromUtf8(u8"当前有激光器正在缓升/缓降，请等待完成"));
    } else if (!seedEnabled && !preReleaseEnabled) {
        ui->preReleaseButton->setToolTip(QString::fromUtf8(u8"请先开启 L1"));
    } else if (preReleaseEnabled && !canRequestL2Off) {
        ui->preReleaseButton->setToolTip(controller->adjustBlockReason(2, -1));
    } else if (!preReleaseEnabled && !canRequestL2On) {
        ui->preReleaseButton->setToolTip(controller->adjustBlockReason(2, +1));
    } else {
        ui->preReleaseButton->setToolTip(QString());
    }

    const bool canIncreaseL3 = controller->canAdjustLaser(3, +1);
    const bool canDecreaseL3 = controller->currentLaserMa(3) > controller->l3SafeOffMa()
                               && controller->canAdjustLaser(3, -1);
    const bool powerAllowed = hasTransport && !anyBusy && (canIncreaseL3 || canDecreaseL3);
    ui->powerSpinBox->setEnabled(powerAllowed);
    ui->powerSpinBox->setToolTip(powerAllowed
                                 ? QString()
                                 : (hasTransport
                                    ? (anyBusy
                                       ? QString::fromUtf8(u8"当前有激光器正在缓升/缓降，请等待完成")
                                       : controller->adjustBlockReason(3, +1))
                                    : QString::fromUtf8(u8"请先连接串口")));
}

void operatorForm::updateStatusDisplay()
{
    if (!ui->statusDisplayLabel) return;

    const bool hasTransport = controller && controller->hasLaserTransport();
    const bool stateSynchronized = controller && controller->isLowerDeviceStateSynchronized();
    const bool bypass = stateSynchronized && controller && controller->temperatureReadyBypassEnabled();

    QString statusText;
    QString styleSheet;

    if (!hasTransport) {
        statusText = QString::fromUtf8(u8"未连接串口。");
        styleSheet = QString::fromUtf8(
            u8"QLabel { color: #6B7280; background: #F3F6FA; border: 1px solid #D6DEE9; "
            u8"border-radius: 6px; padding: 5px 8px; font-family: 'Microsoft YaHei'; "
            u8"font-size: 12px; font-weight: 600; }");
    } else if (!stateSynchronized) {
        statusText = QString::fromUtf8(u8"正在查询下位机状态，完成前禁止操作激光器。");
        styleSheet = QString::fromUtf8(
            u8"QLabel { color: #B87900; background: #FFF7DB; border: 1px solid #F0C36A; "
            u8"border-radius: 6px; padding: 5px 8px; font-family: 'Microsoft YaHei'; "
            u8"font-size: 12px; font-weight: 600; }");
    } else if (bypass) {
        statusText = QString::fromUtf8(
            u8"下位机状态已同步。警告：温度就绪旁路已开启。上位机将忽略下位机温度就绪状态，仅保留顺序联锁；最终温度保护依赖下位机。");
        styleSheet = QString::fromUtf8(
            u8"QLabel { color: #B87900; background: #FFF7DB; border: 1px solid #F0C36A; "
            u8"border-radius: 6px; padding: 5px 8px; font-family: 'Microsoft YaHei'; "
            u8"font-size: 12px; font-weight: 600; }");
    } else {
        statusText = QString::fromUtf8(u8"下位机状态已同步。");
        styleSheet = QString::fromUtf8(
            u8"QLabel { color: #197A50; background: #EAF7EF; border: 1px solid #9BD3B1; "
            u8"border-radius: 6px; padding: 5px 8px; font-family: 'Microsoft YaHei'; "
            u8"font-size: 12px; font-weight: 600; }");
    }

    ui->statusDisplayLabel->setText(statusText);
    ui->statusDisplayLabel->setToolTip(statusText);
    ui->statusDisplayLabel->setStyleSheet(styleSheet);
    ui->statusDisplayLabel->setVisible(true);
}

void operatorForm::syncPowerSpinBoxFromLaser3()
{
    if (!controller || !ui->powerSpinBox) return;

    // L3 被开发者页面或控制核心改变时，普通页面显示跟随真实设定电流反算出的百分比。
    // 如果用户刚设置过百分比，且当前 L3 仍在调整中或已经落入该百分比的容差范围，则保持用户选择的档位。
    const int currentMa = controller->currentLaserMa(3);
    int displayPercent = controller->operatorPowerMaToPercent(currentMa);
    if (requestedPowerPercent >= controller->operatorPowerMinPercent()
        && requestedPowerPercent <= controller->operatorPowerMaxPercent()
        && (l3Busy
            || controller->isLaserBusy(3)
            || controller->operatorPowerMaMatchesPercent(currentMa, requestedPowerPercent))) {
        displayPercent = requestedPowerPercent;
    } else {
        requestedPowerPercent = displayPercent;
    }

    QSignalBlocker blocker(ui->powerSpinBox);
    ui->powerSpinBox->setValue(displayPercent);
}
