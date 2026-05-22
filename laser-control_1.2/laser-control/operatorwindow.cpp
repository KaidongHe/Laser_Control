#include "operatorwindow.h"
#include "ui_operatorwindow.h"
#include "widget.h"

#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStyle>

namespace {
const char *DEVELOPER_PASSWORD = "laser2026";
}

OperatorWindow::OperatorWindow(QWidget *parent)
    : QWidget(parent),
      ui(new Ui::OperatorWindow),
      developerWindow(0),
      seedEnabled(false),
      preReleaseEnabled(false)
{
    ui->setupUi(this);

    updateToggleButton(ui->seedButton, seedEnabled, QString("L1 open"));
    updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString("yufang open"));

    connect(ui->seedButton, &QPushButton::clicked, this, &OperatorWindow::toggleSeed);
    connect(ui->preReleaseButton, &QPushButton::clicked, this, &OperatorWindow::togglePreRelease);
    connect(ui->developerButton, &QPushButton::clicked, this, &OperatorWindow::openDeveloperWindow);
}

OperatorWindow::~OperatorWindow()
{
    if (developerWindow) {
        developerWindow->deleteLater();
        developerWindow = 0;
    }
    delete ui;
}

void OperatorWindow::toggleSeed()
{
    seedEnabled = !seedEnabled;
    updateToggleButton(ui->seedButton, seedEnabled, QString::fromUtf8("L1 开/关（种子）"));
}

void OperatorWindow::togglePreRelease()
{
    preReleaseEnabled = !preReleaseEnabled;
    updateToggleButton(ui->preReleaseButton, preReleaseEnabled, QString::fromUtf8("预放开/关"));
}

void OperatorWindow::openDeveloperWindow()
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
        QString::fromUtf8("开发者验证"),
        QString::fromUtf8("请输入开发者密码："),
        QLineEdit::Password,
        QString(),
        &ok);
    if (!ok) {
        return;
    }

    if (password != QString::fromLatin1(DEVELOPER_PASSWORD)) {
        QMessageBox::warning(this, QString::fromUtf8("密码错误"), QString::fromUtf8("开发者密码不正确。"));
        return;
    }

    developerWindow = new Widget();
    developerWindow->setAttribute(Qt::WA_DeleteOnClose);
    developerWindow->setAttribute(Qt::WA_QuitOnClose, false);
    connect(developerWindow, &QObject::destroyed, this, &OperatorWindow::handleDeveloperWindowClosed);
    hide();
    developerWindow->show();
}

void OperatorWindow::handleDeveloperWindowClosed()
{
    developerWindow = 0;
    show();
    raise();
    activateWindow();
}

void OperatorWindow::updateToggleButton(QPushButton *button, bool enabled, const QString &label)
{
    if (!button) return;
    button->setText(QString::fromUtf8("%1\n%2").arg(label, enabled ? QString::fromUtf8("已开启") : QString::fromUtf8("已关闭")));
    button->setProperty("active", enabled);
    button->style()->unpolish(button);
    button->style()->polish(button);
}
