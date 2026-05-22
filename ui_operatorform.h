/********************************************************************************
** Form generated from reading UI file 'operatorform.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_OPERATORFORM_H
#define UI_OPERATORFORM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>
#include "pillspinbox.h"

QT_BEGIN_NAMESPACE

class Ui_operatorForm
{
public:
    QFrame *panel;
    QGridLayout *gridLayout;
    QPushButton *seedButton;
    PillSpinBox *powerSpinBox;
    QHBoxLayout *developerRow;
    QSpacerItem *developerSpacer;
    QLabel *serialStatusLabel;
    QComboBox *serialComboBox;
    QPushButton *serialConnectButton;
    QPushButton *preReleaseButton;
    QLabel *powerLabel;
    QLabel *titleLabel;
    QPushButton *developerButton;

    void setupUi(QWidget *operatorForm)
    {
        if (operatorForm->objectName().isEmpty())
            operatorForm->setObjectName(QString::fromUtf8("operatorForm"));
        operatorForm->resize(555, 635);
        operatorForm->setMinimumSize(QSize(400, 580));
        operatorForm->setStyleSheet(QString::fromUtf8("QWidget#operatorForm { background: #EEF2F7; }\n"
"QFrame#panel {\n"
"  background: #ffffff;\n"
"  border: 1px solid #DCE3EE;\n"
"  border-radius: 10px;\n"
"}\n"
"QLabel#titleLabel {\n"
"  color: #22304f;\n"
"  font-family: 'Microsoft YaHei';\n"
"  font-size: 22px;\n"
"  font-weight: 600;\n"
"}\n"
"QLabel#subtitleLabel, QLabel#powerLabel {\n"
"  color: #7E8794;\n"
"  font-family: 'Microsoft YaHei';\n"
"  font-size: 15px;\n"
"  font-weight: 500;\n"
"}\n"
"QPushButton#seedButton, QPushButton#preReleaseButton {\n"
"  background: #2f6fd6;\n"
"  color: white;\n"
"  border: none;\n"
"  border-radius: 8px;\n"
"  font-family: 'Microsoft YaHei';\n"
"  font-size: 22px;\n"
"  font-weight: 600;\n"
"  min-height: 78px;\n"
"}\n"
"QPushButton#seedButton:hover, QPushButton#preReleaseButton:hover {\n"
"  background: #245fbd;\n"
"}\n"
"QPushButton#seedButton[active='true'], QPushButton#preReleaseButton[active='true'] {\n"
"  background: #197A50;\n"
"}\n"
"QPushButton#seedButton[active='true']:hover, QPushButton#preReleaseButton["
                        "active='true']:hover {\n"
"  background: #166B46;\n"
"}\n"
"QPushButton#seedButton:disabled, QPushButton#preReleaseButton:disabled {\n"
"  background: #C8D0DC;\n"
"  color: #F5F7FA;\n"
"}\n"
"QPushButton#seedButton[busy='true'], QPushButton#preReleaseButton[busy='true'],\n"
"QPushButton#seedButton[busy='true']:disabled, QPushButton#preReleaseButton[busy='true']:disabled {\n"
"  background: #D69A1E;\n"
"  color: white;\n"
"}\n"
"QPushButton#developerButton {\n"
"  background: transparent;\n"
"  color: #9AA4B2;\n"
"  border: none;\n"
"  padding: 6px 10px;\n"
"  font-family: 'Microsoft YaHei';\n"
"  font-size: 12px;\n"
"}\n"
"QPushButton#developerButton:hover {\n"
"  color: #5F6B7A;\n"
"  background: rgba(47, 111, 214, 0.06);\n"
"}\n"
"QComboBox#serialComboBox {\n"
"  background: #F7F9FC;\n"
"  color: #3A4658;\n"
"  border: 1px solid #D6DEE9;\n"
"  border-radius: 6px;\n"
"  padding: 4px 8px;\n"
"  font-family: 'Microsoft YaHei';\n"
"  font-size: 12px;\n"
"}\n"
"QComboBox#serialComboBox:disabled {\n"
"  color: #8C"
                        "97A6;\n"
"  background: #EDF1F6;\n"
"}\n"
"QPushButton#serialConnectButton {\n"
"  background: #2F6FD6;\n"
"  color: white;\n"
"  border: none;\n"
"  border-radius: 6px;\n"
"  padding: 5px 14px;\n"
"  font-family: 'Microsoft YaHei';\n"
"  font-size: 12px;\n"
"  font-weight: 600;\n"
"}\n"
"QPushButton#serialConnectButton:disabled {\n"
"  background: #C8D0DC;\n"
"  color: #F5F7FA;\n"
"}\n"
"QLabel#serialStatusLabel {\n"
"  color: #9AA4B2;\n"
"  font-family: 'Microsoft YaHei';\n"
"  font-size: 12px;\n"
"  font-weight: 600;\n"
"}"));
        panel = new QFrame(operatorForm);
        panel->setObjectName(QString::fromUtf8("panel"));
        panel->setGeometry(QRect(60, 70, 430, 490));
        panel->setMinimumSize(QSize(430, 490));
        panel->setMaximumSize(QSize(520, 16777215));
        gridLayout = new QGridLayout(panel);
        gridLayout->setSpacing(18);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        gridLayout->setContentsMargins(42, 26, 42, 18);
        seedButton = new QPushButton(panel);
        seedButton->setObjectName(QString::fromUtf8("seedButton"));
        seedButton->setCursor(QCursor(Qt::PointingHandCursor));

        gridLayout->addWidget(seedButton, 1, 0, 1, 1);

        powerSpinBox = new PillSpinBox(panel);
        powerSpinBox->setObjectName(QString::fromUtf8("powerSpinBox"));
        powerSpinBox->setAlignment(Qt::AlignCenter);
        powerSpinBox->setButtonSymbols(QAbstractSpinBox::PlusMinus);
        powerSpinBox->setMinimum(2);
        powerSpinBox->setMaximum(100);
        powerSpinBox->setValue(2);

        gridLayout->addWidget(powerSpinBox, 4, 0, 1, 1);

        developerRow = new QHBoxLayout();
        developerRow->setObjectName(QString::fromUtf8("developerRow"));
        developerSpacer = new QSpacerItem(40, 16, QSizePolicy::Expanding, QSizePolicy::Minimum);

        developerRow->addItem(developerSpacer);

        serialStatusLabel = new QLabel(panel);
        serialStatusLabel->setObjectName(QString::fromUtf8("serialStatusLabel"));
        serialStatusLabel->setMinimumSize(QSize(0, 20));
        serialStatusLabel->setAlignment(Qt::AlignCenter);

        developerRow->addWidget(serialStatusLabel);

        serialComboBox = new QComboBox(panel);
        serialComboBox->setObjectName(QString::fromUtf8("serialComboBox"));
        serialComboBox->setMinimumSize(QSize(0, 30));

        developerRow->addWidget(serialComboBox);

        serialConnectButton = new QPushButton(panel);
        serialConnectButton->setObjectName(QString::fromUtf8("serialConnectButton"));
        serialConnectButton->setMinimumSize(QSize(0, 30));
        serialConnectButton->setCursor(QCursor(Qt::PointingHandCursor));

        developerRow->addWidget(serialConnectButton);


        gridLayout->addLayout(developerRow, 6, 0, 1, 1);

        preReleaseButton = new QPushButton(panel);
        preReleaseButton->setObjectName(QString::fromUtf8("preReleaseButton"));
        preReleaseButton->setCursor(QCursor(Qt::PointingHandCursor));

        gridLayout->addWidget(preReleaseButton, 2, 0, 1, 1);

        powerLabel = new QLabel(panel);
        powerLabel->setObjectName(QString::fromUtf8("powerLabel"));
        powerLabel->setTextFormat(Qt::RichText);
        powerLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(powerLabel, 3, 0, 1, 1);

        titleLabel = new QLabel(panel);
        titleLabel->setObjectName(QString::fromUtf8("titleLabel"));
        titleLabel->setTextFormat(Qt::RichText);
        titleLabel->setAlignment(Qt::AlignCenter);

        gridLayout->addWidget(titleLabel, 0, 0, 1, 1);

        developerButton = new QPushButton(operatorForm);
        developerButton->setObjectName(QString::fromUtf8("developerButton"));
        developerButton->setGeometry(QRect(440, 580, 91, 28));
        developerButton->setCursor(QCursor(Qt::PointingHandCursor));

        retranslateUi(operatorForm);

        QMetaObject::connectSlotsByName(operatorForm);
    } // setupUi

    void retranslateUi(QWidget *operatorForm)
    {
        seedButton->setText(QCoreApplication::translate("operatorForm", "L1 \345\274\200/\345\205\263\357\274\210\347\247\215\345\255\220\357\274\211\n"
"\345\267\262\345\205\263\351\227\255", nullptr));
        powerSpinBox->setSuffix(QString());
        serialStatusLabel->setText(QCoreApplication::translate("operatorForm", "\342\227\217 \346\234\252\350\277\236\346\216\245", nullptr));
#if QT_CONFIG(tooltip)
        serialComboBox->setToolTip(QCoreApplication::translate("operatorForm", "\351\200\211\346\213\251\346\231\256\351\200\232\346\250\241\345\274\217\350\246\201\350\277\236\346\216\245\347\232\204\344\270\262\345\217\243", nullptr));
#endif // QT_CONFIG(tooltip)
        serialConnectButton->setText(QCoreApplication::translate("operatorForm", "\350\277\236\346\216\245", nullptr));
        preReleaseButton->setText(QCoreApplication::translate("operatorForm", "\351\242\204\346\224\276\345\274\200/\345\205\263\n"
"\345\267\262\345\205\263\351\227\255", nullptr));
        powerLabel->setText(QCoreApplication::translate("operatorForm", "<div align=\"center\"><span style=\"color:#9AA4B2; font-size:11px; font-weight:600;\">OUTPUT POWER</span><br/><span style=\"color:#7E8794; font-size:15px; font-weight:500;\">\345\212\237\347\216\207\350\260\203\350\212\202</span></div>", nullptr));
        titleLabel->setText(QCoreApplication::translate("operatorForm", "<div align=\"center\"><span style=\"color:#197A50; font-size:18px;\">\342\227\217</span> <span style=\"color:#7E8794; font-size:12px; font-weight:600;\">LASER CONTROL</span><br/><span style=\"color:#22304F; font-size:24px; font-weight:600;\">\346\277\200\345\205\211\346\216\247\345\210\266</span></div>", nullptr));
        developerButton->setText(QCoreApplication::translate("operatorForm", "\345\274\200\345\217\221\350\200\205", nullptr));
        (void)operatorForm;
    } // retranslateUi

};

namespace Ui {
    class operatorForm: public Ui_operatorForm {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_OPERATORFORM_H
