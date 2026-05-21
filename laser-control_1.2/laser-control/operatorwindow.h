#ifndef OPERATORWINDOW_H
#define OPERATORWINDOW_H

#include <QString>
#include <QWidget>

class QPushButton;
class Widget;

namespace Ui {
class OperatorWindow;
}

class OperatorWindow : public QWidget
{
    Q_OBJECT

public:
    explicit OperatorWindow(QWidget *parent = 0);
    ~OperatorWindow();

private slots:
    void toggleSeed();
    void togglePreRelease();
    void openDeveloperWindow();
    void handleDeveloperWindowClosed();

private:
    void updateToggleButton(QPushButton *button, bool enabled, const QString &label);

    Ui::OperatorWindow *ui;
    Widget *developerWindow;
    bool seedEnabled;
    bool preReleaseEnabled;
};

#endif // OPERATORWINDOW_H
