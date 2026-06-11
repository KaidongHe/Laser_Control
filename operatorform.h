#ifndef OPERATORFORM_H
#define OPERATORFORM_H

#include <QWidget>

class LaserController;
class QPushButton;
class Widget;

namespace Ui {
class operatorForm;
}

class operatorForm : public QWidget
{
    Q_OBJECT

public:
    explicit operatorForm(QWidget *parent = nullptr);
    explicit operatorForm(LaserController *controller, QWidget *parent = nullptr);
    ~operatorForm();

private slots:
    void toggleSeed();
    void togglePreRelease();
    void applyPowerPercent();
    void openDeveloperWindow();
    void handleDeveloperWindowClosed();
    void toggleSerialConnection();

private:
    void setupSerialControls();
    void refreshOperatorSerialPorts();
    void updateOperatorSerialUi(bool opened, const QString &portName);
    void updateToggleButton(QPushButton *button, bool enabled, const QString &label, bool busy = false);
    void updateOperatorLockState();
    void updateStatusDisplay();
    void syncPowerSpinBoxFromLaser3();

    Ui::operatorForm *ui;
    LaserController *controller = nullptr;
    bool ownsController = false;
    Widget *developerWindow = nullptr;
    bool seedEnabled = false;
    bool preReleaseEnabled = false;
    bool l1Busy = false;
    bool l2Busy = false;
    bool l3Busy = false;
    int requestedPowerPercent = -1;
};

#endif // OPERATORFORM_H
