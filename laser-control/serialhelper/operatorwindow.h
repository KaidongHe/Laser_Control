#ifndef OPERATORWINDOW_H
#define OPERATORWINDOW_H

#include <QWidget>

class QPushButton;
class QSpinBox;
class Widget;

class OperatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OperatorWindow(QWidget *parent = nullptr);
    ~OperatorWindow();

private slots:
    void toggleSeed();
    void togglePreRelease();
    void openDeveloperWindow();
    void handleDeveloperWindowClosed();

private:
    void setupUi();
    void updateToggleButton(QPushButton *button, bool enabled, const QString &label);

    QPushButton *seedButton = nullptr;
    QPushButton *preReleaseButton = nullptr;
    QSpinBox *powerSpinBox = nullptr;
    QPushButton *developerButton = nullptr;
    Widget *developerWindow = nullptr;
    bool seedEnabled = false;
    bool preReleaseEnabled = false;
};

#endif // OPERATORWINDOW_H
