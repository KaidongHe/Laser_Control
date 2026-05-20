#include "widget.h"
#include "ui_widget.h"
#include <QMessageBox>
#include <QTextCodec>
#include <QDebug>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QRegularExpression>

static const QColor COLOR_L1(0, 200, 255);
static const QColor COLOR_L2(255, 160, 64);
static const QColor COLOR_L3(192, 128, 255);
static const QColor COLOR_MEASURED(255, 200, 64);
static const int L_THRESHOLD = 90;

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    serialPort(new QSerialPort(this)),
    currentLaser1mA(0),
    currentLaser2mA(0),
    currentLaser3mA(800)
{
    ui->setupUi(this);

    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    serialCheckTimer = new QTimer(this);
    autoReconnectTimer = new QTimer(this);
    statusCheckTimer = new QTimer(this);
    serialCheckTimer->start(1000);
    statusCheckTimer->start(3000);

    connect(serialCheckTimer, &QTimer::timeout, this, &Widget::checkSerialPorts);
    connect(autoReconnectTimer, &QTimer::timeout, this, &Widget::autoReconnectSerialPort);
    connect(statusCheckTimer, &QTimer::timeout, this, &Widget::checkLaserStatus);
    connect(serialPort, &QSerialPort::errorOccurred, this, &Widget::handleSerialError);

    refreshSerialPortList();

    chart1 = ui->chartWidget1;
    chart1->setTitle("Laser1");
    chart1->setYRange(0, 1000);
    chart1->setLineColor(COLOR_L1);
    chart1->setMeasuredColor(COLOR_MEASURED);
    chart1->setThreshold(L_THRESHOLD);

    chart2 = ui->chartWidget2;
    chart2->setTitle("Laser2");
    chart2->setYRange(0, 1000);
    chart2->setLineColor(COLOR_L2);
    chart2->setMeasuredColor(COLOR_MEASURED);
    chart2->setThreshold(L_THRESHOLD);

    chart3 = ui->chartWidget3;
    chart3->setTitle("Laser3");
    chart3->setYRange(800, 10000);
    chart3->setLineColor(COLOR_L3);
    chart3->setMeasuredColor(COLOR_MEASURED);
    chart3->setThreshold(0, false);

    ui->laser1spinbox->setRange(0, 1000);
    ui->laser2spinbox->setRange(0, 1000);
    ui->laser3spinbox->setRange(800, 10000);
    ui->laser3spinbox->setSingleStep(100);

    ui->laser1spinbox->setValue(currentLaser1mA);
    ui->laser2spinbox->setValue(currentLaser2mA);
    ui->laser3spinbox->setValue(currentLaser3mA);

    chart1->addDataPoint(currentLaser1mA);
    chart2->addDataPoint(currentLaser2mA);
    chart3->addDataPoint(currentLaser3mA);

    setLaser1Mode(true);
    setLaser2Mode(true);

    resetLaserStates();
    updateLaserVisual(1);
    updateLaserVisual(2);
    updateLaserVisual(3);

    tryTimer = new QTimer(this);
    tryTimer->setSingleShot(false);
    connect(tryTimer, &QTimer::timeout, this, &Widget::tryStep);

    connect(serialPort, &QSerialPort::readyRead, this, &Widget::serialPortReadyRead_Slot);

#ifdef DEBUG_MODE
    wasOpenedBefore = true;
    lastOpenedPortName = "DEBUG";
    laser1RawReady = true;
    laser2RawReady = true;
    laser3RawReady = true;
    updateAllLaserStates();
    updateLaserDependencies();
    updateLaserVisual(1);
    updateLaserVisual(2);
    updateLaserVisual(3);
    ui->receiveEdit->appendPlainText("[DEBUG] 调试模式已启用，无需串口连接");
#endif
}

Widget::~Widget()
{
    if (serialPort->isOpen()) serialPort->close();
    delete ui;
}

// ===================== 模式切换 =====================
void Widget::setLaser1Mode(bool coarse)
{
    laser1Coarse = coarse;
    ui->laser1CoarseBtn->setChecked(coarse);
    ui->laser1FineBtn->setChecked(!coarse);
    QString active = "QPushButton { background-color: #CC5500; color: white; }";
    ui->laser1CoarseBtn->setStyleSheet(coarse ? active : "");
    ui->laser1FineBtn->setStyleSheet(coarse ? "" : active);
}

void Widget::setLaser2Mode(bool coarse)
{
    laser2Coarse = coarse;
    ui->laser2CoarseBtn->setChecked(coarse);
    ui->laser2FineBtn->setChecked(!coarse);
    QString active = "QPushButton { background-color: #CC5500; color: white; }";
    ui->laser2CoarseBtn->setStyleSheet(coarse ? active : "");
    ui->laser2FineBtn->setStyleSheet(coarse ? "" : active);
}

void Widget::on_laser1CoarseBtn_clicked() { setLaser1Mode(true); }
void Widget::on_laser1FineBtn_clicked()   { setLaser1Mode(false); }
void Widget::on_laser2CoarseBtn_clicked() { setLaser2Mode(true); }
void Widget::on_laser2FineBtn_clicked()   { setLaser2Mode(false); }

// ===================== 状态管理 =====================
void Widget::resetLaserStates()
{
    laser1Ready = false;
    laser2Ready = false;
    laser3Ready = false;
    laser1RawReady = false;
    laser2RawReady = false;
    laser3RawReady = false;
    setLaserReady(1, false);
    setLaserReady(2, false);
    setLaserReady(3, false);
    updateLaserVisual(1);
    updateLaserVisual(2);
    updateLaserVisual(3);
}

void Widget::tryAutoActivateLasers()
{
    if (!serialPort->isOpen()) return;
    bool anyActivated = false;
    if (!laser1RawReady) { laser1RawReady = true; anyActivated = true;
        ui->receiveEdit->appendPlainText("[INFO] 自动激活激光器1（热插拔恢复）"); }
    if (!laser2RawReady) { laser2RawReady = true; anyActivated = true;
        ui->receiveEdit->appendPlainText("[INFO] 自动激活激光器2（热插拔恢复）"); }
    if (!laser3RawReady) { laser3RawReady = true; anyActivated = true;
        ui->receiveEdit->appendPlainText("[INFO] 自动激活激光器3（热插拔恢复）"); }
    if (anyActivated) {
        updateAllLaserStates();
        updateLaserDependencies();
    }
}

void Widget::updateAllLaserStates()
{
    laser1Ready = laser1RawReady;
    setLaserReady(1, laser1Ready);
    laser2Ready = laser1Ready && laser2RawReady;
    setLaserReady(2, laser2Ready);
    laser3Ready = laser1Ready && laser2Ready && laser3RawReady;
    setLaserReady(3, laser3Ready);
    updateLaserVisual(1);
    updateLaserVisual(2);
    updateLaserVisual(3);
}

void Widget::updateLaserDependencies()
{
    if (laser1Ready) {
        if (laser2RawReady && !laser2Ready) {
            laser2Ready = true;
            setLaserReady(2, true);
        }
        if (laser1Ready && laser2Ready && laser3RawReady && !laser3Ready) {
            laser3Ready = true;
            setLaserReady(3, true);
        }
    }
    if (laser2Ready) {
        if (laser1Ready && laser2Ready && laser3RawReady && !laser3Ready) {
            laser3Ready = true;
            setLaserReady(3, true);
        }
    }
    updateLaserVisual(1);
    updateLaserVisual(2);
    updateLaserVisual(3);
}

void Widget::updateLaserStatusFromSend(int laserIndex)
{
    switch(laserIndex) {
    case 1:
        if (!laser1RawReady) { laser1RawReady = true;
            ui->receiveEdit->appendPlainText("[INFO] Laser1 状态已更新为就绪（发送成功）"); }
        break;
    case 2:
        if (!laser2RawReady) { laser2RawReady = true;
            ui->receiveEdit->appendPlainText("[INFO] Laser2 状态已更新为就绪（发送成功）"); }
        break;
    case 3:
        if (!laser3RawReady) { laser3RawReady = true;
            ui->receiveEdit->appendPlainText("[INFO] Laser3 状态已更新为就绪（发送成功）"); }
        break;
    }
    updateAllLaserStates();
    updateLaserDependencies();
}

bool Widget::canControlLaser(int laserIndex)
{
#ifdef DEBUG_MODE
    Q_UNUSED(laserIndex);
    return true;
#endif
    if (!serialPort->isOpen()) return false;
    switch(laserIndex) {
    case 1: return true;
    case 2: return laser1RawReady && currentLaser1mA > L_THRESHOLD;
    case 3: return laser1RawReady && laser2RawReady && currentLaser2mA > L_THRESHOLD;
    default: return false;
    }
}

QString Widget::blockReason(int laserIndex) const
{
#ifdef DEBUG_MODE
    Q_UNUSED(laserIndex);
    return QString();
#endif
    if (!serialPort->isOpen()) return QString("串口未打开");
    switch(laserIndex) {
    case 1:
        return laser1RawReady ? QString() : QString("等待 L1 温度就绪");
    case 2:
        if (!laser1RawReady) return QString("等待 L1 温度就绪");
        if (currentLaser1mA <= L_THRESHOLD) return QString("等待 L1 输出 > %1 mA").arg(L_THRESHOLD);
        if (!laser2RawReady) return QString("等待 L2 温度就绪");
        return QString();
    case 3:
        if (!laser1RawReady) return QString("等待 L1 温度就绪");
        if (!laser2RawReady) return QString("等待 L2 温度就绪");
        if (currentLaser2mA <= L_THRESHOLD) return QString("等待 L2 输出 > %1 mA").arg(L_THRESHOLD);
        if (!laser3RawReady) return QString("等待 L3 就绪");
        return QString();
    }
    return QString();
}

// ===================== 串口管理 =====================
void Widget::refreshSerialPortList()
{
    QString currentSelection = ui->serialCb->currentText();
    ui->serialCb->clear();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        ui->serialCb->addItem(info.portName());
    }
    int index = ui->serialCb->findText(currentSelection);
    if (index != -1) ui->serialCb->setCurrentIndex(index);
}

bool Widget::isTargetPort(const QString &portName)
{
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        if (info.portName() == portName) return true;
    }
    return false;
}

void Widget::handleSerialError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::ResourceError) {
        if (serialPort->isOpen()) serialPort->close();
        ui->receiveEdit->appendPlainText("[ERROR] 串口设备错误或已被拔出");
        resetLaserStates();
        autoReconnectEnabled = true;
        autoReconnectTimer->start(2000);
    }
}

void Widget::checkSerialPorts()
{
    static QStringList previousPorts;
    QStringList currentPorts;
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts())
        currentPorts << info.portName();

    if (currentPorts != previousPorts) {
        refreshSerialPortList();
        if (wasOpenedBefore && !currentPorts.contains(lastOpenedPortName)) {
            if (serialPort->isOpen()) {
                serialPort->close();
                ui->receiveEdit->appendPlainText("[WARN] 串口 " + lastOpenedPortName + " 已断开连接");
                resetLaserStates();
            }
        }
        if (autoReconnectEnabled && wasOpenedBefore && !serialPort->isOpen() &&
            currentPorts.contains(lastOpenedPortName)) {
            autoReconnectTimer->start(2000);
        }
        previousPorts = currentPorts;
    }

    if (serialPort->isOpen()) {
        ui->openBt->setText("已连接");
        ui->openBt->setStyleSheet("QPushButton { background-color: green; color: white; } QPushButton:hover { background-color: #CC5500; }");
    } else {
        ui->openBt->setText("打开串口");
        ui->openBt->setStyleSheet("");
    }
}

void Widget::autoReconnectSerialPort()
{
    autoReconnectTimer->stop();
    if (!serialPort->isOpen() && !lastOpenedPortName.isEmpty() && autoReconnectEnabled) {
        bool portExists = false;
        foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
            if (info.portName() == lastOpenedPortName) { portExists = true; break; }
        }
        if (portExists) {
            serialPort->setPortName(lastOpenedPortName);
            serialPort->setBaudRate(QSerialPort::Baud115200);
            serialPort->setDataBits(QSerialPort::Data8);
            serialPort->setStopBits(QSerialPort::OneStop);
            serialPort->setParity(QSerialPort::NoParity);
            serialPort->setFlowControl(QSerialPort::NoFlowControl);
            if (serialPort->open(QIODevice::ReadWrite)) {
                ui->receiveEdit->appendPlainText("[INFO] 自动重新连接串口: " + lastOpenedPortName);
                tryAutoActivateLasers();
                rxBuffer.clear();
            } else {
                autoReconnectTimer->start(2000);
            }
        }
    }
}

void Widget::checkLaserStatus()
{
    if (!serialPort->isOpen()) return;
    updateLaserDependencies();
}

QString Widget::decodeSerialData(const QByteArray &data)
{
    QString text = QString::fromUtf8(data);
    if (text.contains(QChar(0xFFFD))) {
        QTextCodec *gbk = QTextCodec::codecForName("GBK");
        text = gbk ? gbk->toUnicode(data) : QString::fromLocal8Bit(data);
    }
    return text;
}

void Widget::on_openBt_clicked()
{
#ifdef DEBUG_MODE
    lastOpenedPortName = "DEBUG";
    wasOpenedBefore = true;
    autoReconnectEnabled = true;
    ui->receiveEdit->appendPlainText("[DEBUG] 模拟串口连接成功");
    resetLaserStates();
    laser1RawReady = true;
    laser2RawReady = true;
    laser3RawReady = true;
    updateAllLaserStates();
    updateLaserDependencies();
    return;
#endif
    if (serialPort->isOpen()) {
        serialPort->close();
        autoReconnectEnabled = false;
        ui->receiveEdit->appendPlainText("[INFO] 串口已关闭");
        wasOpenedBefore = false;
        resetLaserStates();
        return;
    }

    QString portName = ui->serialCb->currentText();
    if (portName.isEmpty()) {
        QMessageBox::warning(this, "错误", "请先选择串口");
        return;
    }
    serialPort->setPortName(portName);
    serialPort->setBaudRate(QSerialPort::Baud115200);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    if (!serialPort->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, "错误", "无法打开串口: " + serialPort->errorString());
        return;
    }
    lastOpenedPortName = portName;
    wasOpenedBefore = true;
    autoReconnectEnabled = true;
    ui->receiveEdit->appendPlainText("[INFO] 串口已打开: " + portName);
    tryAutoActivateLasers();
    rxBuffer.clear();
}

void Widget::on_closeBt_clicked()
{
#ifdef DEBUG_MODE
    wasOpenedBefore = false;
    autoReconnectEnabled = false;
    resetLaserStates();
    ui->receiveEdit->appendPlainText("[DEBUG] 模拟串口已关闭");
    return;
#endif
    if (serialPort->isOpen()) {
        serialPort->close();
        wasOpenedBefore = false;
        autoReconnectEnabled = false;
        ui->receiveEdit->appendPlainText("[INFO] 串口已关闭");
        resetLaserStates();
    }
}

// ===================== 接收 =====================
void Widget::serialPortReadyRead_Slot()
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

        ui->receiveEdit->appendPlainText(line);

        QString cleaned = line;
        cleaned.remove('\r');
        cleaned.replace("\n", "");
        cleaned.replace(" ", "");

        bool stateChanged = false;

        if (cleaned.contains("Laser1:温度就绪") || cleaned.contains("Laser1就绪") ||
            cleaned.toLower().contains("laser1ready") || cleaned.contains("L1:OK")) {
            if (!laser1RawReady) { laser1RawReady = true; stateChanged = true; }
        } else if (cleaned.contains("Laser1:温度未就绪") || cleaned.contains("Laser1未就绪") ||
                   cleaned.toLower().contains("laser1notready") || cleaned.contains("L1:NG")) {
            if (laser1RawReady) { laser1RawReady = false; stateChanged = true; }
        }
        if (cleaned.contains("Laser2:温度就绪") || cleaned.contains("Laser2就绪") ||
            cleaned.toLower().contains("laser2ready") || cleaned.contains("L2:OK")) {
            if (!laser2RawReady) { laser2RawReady = true; stateChanged = true; }
        } else if (cleaned.contains("Laser2:温度未就绪") || cleaned.contains("Laser2未就绪") ||
                   cleaned.toLower().contains("laser2notready") || cleaned.contains("L2:NG")) {
            if (laser2RawReady) { laser2RawReady = false; stateChanged = true; }
        }
        if (cleaned.contains("Laser3:温度就绪") || cleaned.contains("Laser3就绪") ||
            cleaned.toLower().contains("laser3ready") || cleaned.contains("L3:OK")) {
            if (!laser3RawReady) { laser3RawReady = true; stateChanged = true; }
        } else if (cleaned.contains("Laser3:温度未就绪") || cleaned.contains("Laser3未就绪") ||
                   cleaned.toLower().contains("laser3notready") || cleaned.contains("L3:NG")) {
            if (laser3RawReady) { laser3RawReady = false; stateChanged = true; }
        }

        if (stateChanged) {
            updateAllLaserStates();
            updateLaserDependencies();
        }

        parseMeasuredFromLine(line);
    }
}

// 提取实测电流。STM32 输出形如：
// "Laser1: DAC=..., 输入电流=0.123A, 输入电压=..., 输出电流=0.118A, 输出电压=..."
void Widget::parseMeasuredFromLine(const QString &line)
{
    QRegularExpression re(QStringLiteral("Laser([123])[^\\n]*?输出电流\\s*=\\s*([0-9]+\\.?[0-9]*)\\s*A"));
    QRegularExpressionMatch m = re.match(line);
    if (!m.hasMatch()) return;
    int idx = m.captured(1).toInt();
    double amps = m.captured(2).toDouble();
    double mA = amps * 1000.0;
    switch (idx) {
    case 1: measuredLaser1mA = mA; chart1->addMeasuredPoint(mA); updateLaserVisual(1); break;
    case 2: measuredLaser2mA = mA; chart2->addMeasuredPoint(mA); updateLaserVisual(2); break;
    case 3: measuredLaser3mA = mA; chart3->addMeasuredPoint(mA); updateLaserVisual(3); break;
    }
}

// ===================== 发送 =====================
void Widget::sendLaserCommand(int laserIndex, char cmd)
{
#ifdef DEBUG_MODE
    ui->receiveEdit->appendPlainText(QString("[DEBUG:SEND] Laser%1 -> %2 (模拟)").arg(laserIndex).arg(QChar(cmd)));
    updateLaserStatusFromSend(laserIndex);
    return;
#endif
    if (!serialPort || !serialPort->isOpen()) {
        ui->receiveEdit->appendPlainText("[WARN] 串口未打开，未发送命令");
        return;
    }
    if (laserIndex < 1 || laserIndex > 3) return;

    if (!canControlLaser(laserIndex)) {
        ui->receiveEdit->appendPlainText(QString("[WARN] Laser%1 无法发送：%2")
                                         .arg(laserIndex).arg(blockReason(laserIndex)));
        return;
    }
    if (lastSentTimers[laserIndex-1].isValid() && lastSentTimers[laserIndex-1].elapsed() < minSendIntervalMs)
        return;
    lastSentTimers[laserIndex-1].restart();

    QByteArray b(1, cmd);
    qint64 bytesWritten = serialPort->write(b);
    if (bytesWritten == -1) {
        ui->receiveEdit->appendPlainText("[ERROR] 发送命令失败: " + serialPort->errorString());
        if (serialPort->error() == QSerialPort::ResourceError ||
            serialPort->error() == QSerialPort::WriteError) {
            serialPort->close();
            resetLaserStates();
            autoReconnectTimer->start(2000);
        }
    } else {
        ui->receiveEdit->appendPlainText(QString("[SEND] Laser%1 -> %2").arg(laserIndex).arg(QChar(cmd)));
        updateLaserStatusFromSend(laserIndex);
    }
}

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
        cur = currentLaser1mA; measuredVal = measuredLaser1mA;
        break;
    case 2:
        big = ui->bigCurrent2; measured = ui->measuredLabel2; reason = ui->reasonLabel2;
        upBtn = ui->laser2UpBtn; downBtn = ui->laser2DownBtn; spin = ui->laser2spinbox;
        cur = currentLaser2mA; measuredVal = measuredLaser2mA;
        break;
    case 3:
        big = ui->bigCurrent3; measured = ui->measuredLabel3; reason = ui->reasonLabel3;
        upBtn = ui->laser3UpBtn; downBtn = ui->laser3DownBtn; spin = ui->laser3spinbox;
        cur = currentLaser3mA; measuredVal = measuredLaser3mA;
        break;
    default: return;
    }

    if (big) big->setText(QString::number(cur));

    if (measured) {
        if (measuredVal < 0) measured->setText("实测: --");
        else measured->setText(QString("实测: %1 mA").arg(measuredVal, 0, 'f', 0));
    }

    QString why = blockReason(laserIndex);
    bool canCtrl = canControlLaser(laserIndex);
    if (upBtn)   upBtn->setEnabled(canCtrl);
    if (downBtn) downBtn->setEnabled(canCtrl);
    if (spin)    spin->setEnabled(canCtrl);

    if (reason) reason->setText(why);
}

// ===================== 步进按钮 =====================
void Widget::adjustLaser(int laserIndex, int direction)
{
    int *cur = nullptr;
    int step = 0, lo = 0, hi = 0;
    LaserChart *chart = nullptr;
    QSpinBox *spin = nullptr;
    char cmd = 0;

    switch (laserIndex) {
    case 1:
        cur = &currentLaser1mA; lo = 0; hi = 1000;
        step = laser1Coarse ? 10 : 1;
        chart = chart1; spin = ui->laser1spinbox;
        if (laser1Coarse) cmd = (direction > 0) ? '1' : '0';
        else              cmd = (direction > 0) ? '3' : '2';
        break;
    case 2:
        cur = &currentLaser2mA; lo = 0; hi = 1000;
        step = laser2Coarse ? 10 : 1;
        chart = chart2; spin = ui->laser2spinbox;
        if (laser2Coarse) cmd = (direction > 0) ? '4' : '5';
        else              cmd = (direction > 0) ? '6' : '7';
        break;
    case 3:
        cur = &currentLaser3mA; lo = 800; hi = 10000;
        step = 100;
        chart = chart3; spin = ui->laser3spinbox;
        cmd = (direction > 0) ? '8' : '9';
        break;
    default: return;
    }

    if (!canControlLaser(laserIndex)) {
        QString why = blockReason(laserIndex);
        if (!why.isEmpty())
            ui->receiveEdit->appendPlainText(QString("[INFO] L%1 %2").arg(laserIndex).arg(why));
        if (laserIndex >= 2) updateLaserStatusFromSend(1);
        if (laserIndex >= 3) updateLaserStatusFromSend(2);
        updateLaserStatusFromSend(laserIndex);
        updateLaserVisual(laserIndex);
        return;
    }

    int newVal = qBound(lo, *cur + direction * step, hi);
    if (newVal == *cur) return;
    *cur = newVal;

    if (spin) {
        spin->blockSignals(true);
        spin->setValue(newVal);
        spin->blockSignals(false);
    }
    if (chart) chart->addDataPoint(newVal);

    sendLaserCommand(laserIndex, cmd);
    updateLaserVisual(laserIndex);
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
    if (target == currentLaser1mA) return;
    if (!canControlLaser(1)) {
        ui->laser1spinbox->blockSignals(true);
        ui->laser1spinbox->setValue(currentLaser1mA);
        ui->laser1spinbox->blockSignals(false);
        return;
    }
    int dir = (target > currentLaser1mA) ? +1 : -1;
    while (currentLaser1mA != target) {
        int diff = qAbs(target - currentLaser1mA);
        char cmd; int step;
        if (laser1Coarse && diff >= 10) { step = 10; cmd = (dir > 0) ? '1' : '0'; }
        else                            { step = 1;  cmd = (dir > 0) ? '3' : '2'; }
        sendLaserCommand(1, cmd);
        currentLaser1mA += dir * step;
    }
    chart1->addDataPoint(currentLaser1mA);
    updateLaserVisual(1);
}

void Widget::on_laser2spinbox_editingFinished()
{
    int target = ui->laser2spinbox->value();
    if (target == currentLaser2mA) return;
    if (!canControlLaser(2)) {
        ui->laser2spinbox->blockSignals(true);
        ui->laser2spinbox->setValue(currentLaser2mA);
        ui->laser2spinbox->blockSignals(false);
        return;
    }
    int dir = (target > currentLaser2mA) ? +1 : -1;
    while (currentLaser2mA != target) {
        int diff = qAbs(target - currentLaser2mA);
        char cmd; int step;
        if (laser2Coarse && diff >= 10) { step = 10; cmd = (dir > 0) ? '4' : '5'; }
        else                            { step = 1;  cmd = (dir > 0) ? '6' : '7'; }
        sendLaserCommand(2, cmd);
        currentLaser2mA += dir * step;
    }
    chart2->addDataPoint(currentLaser2mA);
    updateLaserVisual(2);
}

void Widget::on_laser3spinbox_editingFinished()
{
    int target = ui->laser3spinbox->value();
    target = (target / 100) * 100;
    target = qBound(800, target, 10000);
    if (target == currentLaser3mA) {
        ui->laser3spinbox->blockSignals(true);
        ui->laser3spinbox->setValue(currentLaser3mA);
        ui->laser3spinbox->blockSignals(false);
        return;
    }
    if (!canControlLaser(3)) {
        ui->laser3spinbox->blockSignals(true);
        ui->laser3spinbox->setValue(currentLaser3mA);
        ui->laser3spinbox->blockSignals(false);
        return;
    }
    int dir = (target > currentLaser3mA) ? +1 : -1;
    while (currentLaser3mA != target) {
        char cmd = (dir > 0) ? '8' : '9';
        sendLaserCommand(3, cmd);
        currentLaser3mA += dir * 100;
    }
    ui->laser3spinbox->blockSignals(true);
    ui->laser3spinbox->setValue(currentLaser3mA);
    ui->laser3spinbox->blockSignals(false);
    chart3->addDataPoint(currentLaser3mA);
    updateLaserVisual(3);
}

void Widget::on_sendBt_clicked()
{
    ui->receiveEdit->appendPlainText("[INFO] 发送功能暂未开放");
}

void Widget::on_clearBt_clicked()
{
    ui->receiveEdit->clear();
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
        return;
    }

    if (!serialPort->isOpen()) {
#ifndef DEBUG_MODE
        QMessageBox::warning(this, "错误", "请先打开串口");
        return;
#endif
    }

    QDialog dlg(this);
    dlg.setWindowTitle("扫描参数设置");
    QFormLayout *form = new QFormLayout(&dlg);

    QSpinBox *phase1Time = new QSpinBox(&dlg);
    phase1Time->setRange(1, 9999);
    phase1Time->setValue(tryPhase1TimeSec);
    phase1Time->setSuffix(" 秒");
    form->addRow("L1 阶段1 (0→850mA):", phase1Time);

    QSpinBox *phase2Time = new QSpinBox(&dlg);
    phase2Time->setRange(1, 9999);
    phase2Time->setValue(tryPhase2TimeSec);
    phase2Time->setSuffix(" 秒");
    form->addRow("L1 阶段2 (850→200mA):", phase2Time);

    QSpinBox *phase3Time = new QSpinBox(&dlg);
    phase3Time->setRange(1, 9999);
    phase3Time->setValue(tryPhase3TimeSec);
    phase3Time->setSuffix(" 秒");
    form->addRow("L1 阶段3 (200→最终):", phase3Time);

    QSpinBox *stepSize = new QSpinBox(&dlg);
    stepSize->setRange(1, 100);
    stepSize->setValue(tryStepSize);
    stepSize->setSuffix(" mA");
    form->addRow("L1 步长:", stepSize);

    QSpinBox *finalCurrent = new QSpinBox(&dlg);
    finalCurrent->setRange(0, 200);
    finalCurrent->setValue(tryFinalCurrent);
    finalCurrent->setSuffix(" mA");
    form->addRow("L1 最终电流:", finalCurrent);

    QSpinBox *l2Target = new QSpinBox(&dlg);
    l2Target->setRange(0, 1000);
    l2Target->setValue(tryL2TargetMA);
    l2Target->setSuffix(" mA");
    form->addRow("L2 终点 (0→):", l2Target);

    QSpinBox *l2Step = new QSpinBox(&dlg);
    l2Step->setRange(1, 100);
    l2Step->setValue(tryL2StepSize);
    l2Step->setSuffix(" mA");
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
    l3Step->setRange(100, 1000);
    l3Step->setSingleStep(100);
    l3Step->setValue(tryL3StepSize);
    l3Step->setSuffix(" mA");
    form->addRow("L3 步长 (硬件最小100):", l3Step);

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
    tryStepSize = stepSize->value();
    tryFinalCurrent = finalCurrent->value();
    tryL2TargetMA = l2Target->value();
    tryL2StepSize = l2Step->value();
    tryL2TimeSec = l2Time->value();
    tryL3TargetMA = (l3Target->value() / 100) * 100;  // 对齐到 100 mA
    tryL3StepSize = (l3Step->value() / 100) * 100;
    if (tryL3StepSize < 100) tryL3StepSize = 100;
    tryL3TimeSec = l3Time->value();

    if (!canControlLaser(1)) {
        QMessageBox::warning(this, "错误", "激光器1未就绪，请先确保温度就绪后再试");
        return;
    }

    tryState = TryPhase1;
    tryCurrent = currentLaser1mA;

    int remain1 = 850 - tryCurrent;
    if (remain1 <= 0) {
        tryState = TryPhase2;
        int remain2 = tryCurrent - 200;
        if (remain2 <= 0) tryState = TryPhase3;
    }

    int numSteps = 0;
    int intervalMs = 1000;
    switch (tryState) {
    case TryPhase1:
        numSteps = (850 - tryCurrent) / tryStepSize;
        if (numSteps <= 0) numSteps = 1;
        intervalMs = (tryPhase1TimeSec * 1000) / numSteps;
        break;
    case TryPhase2:
        numSteps = (tryCurrent - 200) / tryStepSize;
        if (numSteps <= 0) numSteps = 1;
        intervalMs = (tryPhase2TimeSec * 1000) / numSteps;
        break;
    case TryPhase3:
        numSteps = (tryCurrent - tryFinalCurrent) / tryStepSize;
        if (numSteps <= 0) numSteps = 1;
        intervalMs = (tryPhase3TimeSec * 1000) / numSteps;
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

#ifndef DEBUG_MODE
    if (!serialPort->isOpen()) {
        tryTimer->stop();
        tryState = TryIdle;
        ui->tryButton->setText("全段扫描 (TRY)");
        ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
        ui->receiveEdit->appendPlainText("[TRY] 串口断开，扫描中止");
        return;
    }
#endif

    int target = 0;
    int direction = 0;

    switch (tryState) {
    case TryPhase1:
        target = 850; direction = 1;
        if (tryCurrent >= target) {
            tryState = TryPhase2;
            int numSteps = (tryCurrent - 200) / tryStepSize;
            if (numSteps <= 0) numSteps = 1;
            int intervalMs = (tryPhase2TimeSec * 1000) / numSteps;
            tryTimer->setInterval(intervalMs);
            ui->receiveEdit->appendPlainText("[TRY] 进入阶段2: " + QString::number(tryCurrent) + " → 200 mA");
            return;
        }
        break;
    case TryPhase2:
        target = 200; direction = -1;
        if (tryCurrent <= target) {
            tryState = TryPhase3;
            int numSteps = (tryCurrent - tryFinalCurrent) / tryStepSize;
            if (numSteps <= 0) numSteps = 1;
            int intervalMs = (tryPhase3TimeSec * 1000) / numSteps;
            tryTimer->setInterval(intervalMs);
            ui->receiveEdit->appendPlainText("[TRY] 进入阶段3: " + QString::number(tryCurrent) + " → " + QString::number(tryFinalCurrent) + " mA");
            return;
        }
        break;
    case TryPhase3:
        target = tryFinalCurrent; direction = -1;
        if (tryCurrent <= target) {
            // L1 完成 → 转入 L2 阶段
            tryState = TryPhaseL2;
            tryL2Current = currentLaser2mA;
            int diff = tryL2TargetMA - tryL2Current;
            int numSteps = (diff > 0) ? (diff / tryL2StepSize) : 0;
            if (numSteps <= 0) {
                // 已超过目标，直接跳到 L3
                tryState = TryPhaseL3;
                tryL3Current = currentLaser3mA;
                int diff3 = tryL3TargetMA - tryL3Current;
                int n3 = (diff3 > 0) ? (diff3 / tryL3StepSize) : 1;
                if (n3 <= 0) n3 = 1;
                tryTimer->setInterval((tryL3TimeSec * 1000) / n3);
                ui->receiveEdit->appendPlainText(QString("[TRY] L2 已达终点，直接进入 L3: %1 → %2 mA")
                    .arg(tryL3Current).arg(tryL3TargetMA));
            } else {
                tryTimer->setInterval((tryL2TimeSec * 1000) / numSteps);
                ui->receiveEdit->appendPlainText(QString("[TRY] L1 完成，进入 L2 扫描: %1 → %2 mA (步长 %3, 时长 %4s)")
                    .arg(tryL2Current).arg(tryL2TargetMA).arg(tryL2StepSize).arg(tryL2TimeSec));
            }
            return;
        }
        break;
    case TryPhaseL2: {
        // L2 联锁：L1 必须就绪且 L1 输出 > 90 mA
        if (!canControlLaser(2)) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 扫描中止：%1").arg(blockReason(2)));
            return;
        }
        if (tryL2Current >= tryL2TargetMA) {
            // L2 完成 → 转入 L3
            tryState = TryPhaseL3;
            tryL3Current = currentLaser3mA;
            int diff3 = tryL3TargetMA - tryL3Current;
            int n3 = (diff3 > 0) ? (diff3 / tryL3StepSize) : 0;
            if (n3 <= 0) {
                tryTimer->stop();
                tryState = TryIdle;
                ui->tryButton->setText("全段扫描 (TRY)");
                ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
                ui->receiveEdit->appendPlainText("[TRY] 全部扫描完成");
                return;
            }
            tryTimer->setInterval((tryL3TimeSec * 1000) / n3);
            ui->receiveEdit->appendPlainText(QString("[TRY] L2 完成，进入 L3 扫描: %1 → %2 mA (步长 %3, 时长 %4s)")
                .arg(tryL3Current).arg(tryL3TargetMA).arg(tryL3StepSize).arg(tryL3TimeSec));
            return;
        }
        // 走 L2 一步
        int newVal = qMin(tryL2Current + tryL2StepSize, tryL2TargetMA);
        int delta = newVal - tryL2Current;
        // 拼出指令：粗调 ±10 + 细调 ±1
        int remaining = delta;
        while (remaining >= 10) { sendLaserCommand(2, '4'); remaining -= 10; }
        while (remaining > 0)   { sendLaserCommand(2, '6'); remaining -= 1; }
        tryL2Current = newVal;
        currentLaser2mA = newVal;
        ui->laser2spinbox->blockSignals(true);
        ui->laser2spinbox->setValue(newVal);
        ui->laser2spinbox->blockSignals(false);
        chart2->addDataPoint(newVal);
        updateLaserVisual(2);
        return;
    }
    case TryPhaseL3: {
        if (!canControlLaser(3)) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] L3 扫描中止：%1").arg(blockReason(3)));
            return;
        }
        if (tryL3Current >= tryL3TargetMA) {
            tryTimer->stop();
            tryState = TryIdle;
            ui->tryButton->setText("全段扫描 (TRY)");
            ui->tryButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; font-size: 14px; } QPushButton:hover { background-color: #CC5500; color: white; }");
            ui->receiveEdit->appendPlainText(QString("[TRY] 全部扫描完成! L3 终点: %1 mA").arg(tryL3Current));
            return;
        }
        int newVal = qMin(tryL3Current + tryL3StepSize, tryL3TargetMA);
        int delta = newVal - tryL3Current;
        int chunks = delta / 100;
        for (int i = 0; i < chunks; i++) sendLaserCommand(3, '8');
        tryL3Current = newVal;
        currentLaser3mA = newVal;
        ui->laser3spinbox->blockSignals(true);
        ui->laser3spinbox->setValue(newVal);
        ui->laser3spinbox->blockSignals(false);
        chart3->addDataPoint(newVal);
        updateLaserVisual(3);
        return;
    }
    default: return;
    }

    tryCurrent += direction * tryStepSize;
    if (direction > 0) tryCurrent = qMin(tryCurrent, target);
    else               tryCurrent = qMax(tryCurrent, target);

    int remaining = tryStepSize;
    while (remaining >= 10) {
        char cmd = (direction > 0) ? '1' : '0';
        sendLaserCommand(1, cmd);
        remaining -= 10;
    }
    while (remaining > 0) {
        char cmd = (direction > 0) ? '3' : '2';
        sendLaserCommand(1, cmd);
        remaining -= 1;
    }

    ui->laser1spinbox->blockSignals(true);
    ui->laser1spinbox->setValue(tryCurrent);
    ui->laser1spinbox->blockSignals(false);
    currentLaser1mA = tryCurrent;
    chart1->addDataPoint(tryCurrent);
    updateLaserVisual(1);
}
