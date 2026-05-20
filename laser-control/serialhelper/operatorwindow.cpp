#include "operatorwindow.h"
#include "widget.h"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
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
    setWindowTitle(QStringLiteral("激光控制"));
    resize(760, 620);
    setMinimumSize(640, 580);

    setStyleSheet(
        "OperatorWindow { background: #eef2f7; }"
        "QFrame#panel {"
        "  background: #ffffff;"
        "  border: 1px solid #cbd6e6;"
        "  border-radius: 8px;"
        "}"
        "QLabel#titleLabel {"
        "  color: #22304f;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 24px;"
        "  font-weight: 600;"
        "}"
        "QLabel#subtitleLabel {"
        "  color: #6d7890;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 13px;"
        "}"
        "QPushButton#mainButton {"
        "  background: #2f6fd6;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 8px;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 24px;"
        "  font-weight: 600;"
        "  min-height: 86px;"
        "}"
        "QPushButton#mainButton:hover { background: #245fbd; }"
        "QPushButton#mainButton[active='true'] { background: #1f8a5b; }"
        "QPushButton#mainButton[active='true']:hover { background: #18754c; }"
        "QSpinBox#powerSpinBox {"
        "  background: #f8fbff;"
        "  color: #2b5fb7;"
        "  border: 1px solid #b7c6e8;"
        "  border-radius: 8px;"
        "  padding: 8px 12px;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 28px;"
        "  font-weight: 600;"
        "  min-height: 68px;"
        "}"
        "QPushButton#developerButton {"
        "  background: transparent;"
        "  color: #7b879b;"
        "  border: 1px solid #d1d8e5;"
        "  border-radius: 4px;"
        "  padding: 5px 12px;"
        "  font-family: 'Microsoft YaHei';"
        "  font-size: 12px;"
        "}"
        "QPushButton#developerButton:hover {"
        "  color: #2f6fd6;"
        "  border-color: #9db7e8;"
        "}"
    );

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(36, 36, 36, 36);
    rootLayout->setAlignment(Qt::AlignCenter);

    QFrame *panel = new QFrame(this);
    panel->setObjectName("panel");
    panel->setMinimumSize(430, 500);
    panel->setMaximumWidth(520);

    QVBoxLayout *panelLayout = new QVBoxLayout(panel);
    panelLayout->setContentsMargins(42, 34, 42, 24);
    panelLayout->setSpacing(22);

    QLabel *titleLabel = new QLabel(QStringLiteral("激光控制"), panel);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    QLabel *subtitleLabel = new QLabel(QStringLiteral("普通操作界面"), panel);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    seedButton = new QPushButton(panel);
    seedButton->setObjectName("mainButton");
    seedButton->setCheckable(false);
    seedButton->setCursor(Qt::PointingHandCursor);

    preReleaseButton = new QPushButton(panel);
    preReleaseButton->setObjectName("mainButton");
    preReleaseButton->setCheckable(false);
    preReleaseButton->setCursor(Qt::PointingHandCursor);

    QLabel *powerLabel = new QLabel(QStringLiteral("功率调整"), panel);
    powerLabel->setObjectName("subtitleLabel");
    powerLabel->setAlignment(Qt::AlignCenter);

    powerSpinBox = new QSpinBox(panel);
    powerSpinBox->setObjectName("powerSpinBox");
    powerSpinBox->setRange(2, 100);
    powerSpinBox->setValue(2);
    powerSpinBox->setSuffix(QStringLiteral(" %"));
    powerSpinBox->setAlignment(Qt::AlignCenter);
    powerSpinBox->setButtonSymbols(QAbstractSpinBox::PlusMinus);

    developerButton = new QPushButton(QStringLiteral("开发者"), panel);
    developerButton->setObjectName("developerButton");
    developerButton->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *developerRow = new QHBoxLayout();
    developerRow->addStretch();
    developerRow->addWidget(developerButton);

    panelLayout->addWidget(titleLabel);
    panelLayout->addWidget(subtitleLabel);
    panelLayout->addSpacing(8);
    panelLayout->addWidget(seedButton);
    panelLayout->addWidget(preReleaseButton);
    panelLayout->addWidget(powerLabel);
    panelLayout->addWidget(powerSpinBox);
    panelLayout->addStretch();
    panelLayout->addLayout(developerRow);

    rootLayout->addWidget(panel);

    updateToggleButton(seedButton, seedEnabled, QStringLiteral("L1 开/关（种子）"));
    updateToggleButton(preReleaseButton, preReleaseEnabled, QStringLiteral("预放开/关"));

    connect(seedButton, &QPushButton::clicked, this, &OperatorWindow::toggleSeed);
    connect(preReleaseButton, &QPushButton::clicked, this, &OperatorWindow::togglePreRelease);
    connect(developerButton, &QPushButton::clicked, this, &OperatorWindow::openDeveloperWindow);
}

void OperatorWindow::toggleSeed()
{
    seedEnabled = !seedEnabled;
    updateToggleButton(seedButton, seedEnabled, QStringLiteral("L1 开/关（种子）"));
}

void OperatorWindow::togglePreRelease()
{
    preReleaseEnabled = !preReleaseEnabled;
    updateToggleButton(preReleaseButton, preReleaseEnabled, QStringLiteral("预放开/关"));
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
        QStringLiteral("开发者验证"),
        QStringLiteral("请输入开发者密码："),
        QLineEdit::Password,
        QString(),
        &ok);
    if (!ok) {
        return;
    }

    if (password != QString::fromLatin1(DEVELOPER_PASSWORD)) {
        QMessageBox::warning(this, QStringLiteral("密码错误"), QStringLiteral("开发者密码不正确。"));
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
    developerWindow = nullptr;
    show();
    raise();
    activateWindow();
}

void OperatorWindow::updateToggleButton(QPushButton *button, bool enabled, const QString &label)
{
    if (!button) return;
    button->setText(QStringLiteral("%1\n%2").arg(label, enabled ? QStringLiteral("已开启") : QStringLiteral("已关闭")));
    button->setProperty("active", enabled);
    button->style()->unpolish(button);
    button->style()->polish(button);
}
