#include "operatorwindow.h"
#include "widget.h"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStyle>
#include <QVBoxLayout>

namespace {
const char *DEVELOPER_PASSWORD = "laser2026";
}

OperatorWindow::OperatorWindow(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

OperatorWindow::~OperatorWindow()
{
    if (developerWindow) {
        developerWindow->deleteLater();
        developerWindow = nullptr;
    }
}

void OperatorWindow::setupUi()
{
}

void OperatorWindow::toggleSeed()
{
}

void OperatorWindow::togglePreRelease()
{
}

void OperatorWindow::openDeveloperWindow()
{
}

void OperatorWindow::handleDeveloperWindowClosed()
{
}

void OperatorWindow::updateToggleButton(QPushButton *button, bool enabled, const QString &label)
{
    if (!button) return;
    button->setText(QString("%1\n%2").arg(label, enabled ? QStringLiteral("已开启") : QStringLiteral("已关闭")));
    button->setProperty("active", enabled);
    button->style()->unpolish(button);
    button->style()->polish(button);
}
