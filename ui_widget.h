/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 5.14.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPlainTextEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include "laserchart.h"

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QVBoxLayout *rootLayout;
    QWidget *titleBar;
    QGroupBox *card1;
    QHBoxLayout *card1Layout;
    LaserChart *chartWidget1;
    QWidget *ctrl1Wrap;
    QVBoxLayout *ctrl1Layout;
    QHBoxLayout *status1Row;
    QLabel *ledLabel1;
    QLabel *laser1StatusLabel;
    QSpacerItem *s1;
    QHBoxLayout *big1Row;
    QLabel *bigCurrent1;
    QLabel *unit1;
    QLabel *measuredLabel1;
    QHBoxLayout *step1Row;
    QPushButton *laser1DownBtn;
    QSpinBox *laser1spinbox;
    QPushButton *laser1UpBtn;
    QHBoxLayout *mode1Row;
    QLabel *mode1Title;
    QPushButton *laser1CoarseBtn;
    QPushButton *laser1FineBtn;
    QPushButton *laser1ParamsButton;
    QComboBox *laser1ModeCb;
    QLabel *reasonLabel1;
    QSpacerItem *vs1;
    QGroupBox *card2;
    QHBoxLayout *card2Layout;
    LaserChart *chartWidget2;
    QWidget *ctrl2Wrap;
    QVBoxLayout *ctrl2Layout;
    QHBoxLayout *status2Row;
    QLabel *ledLabel2;
    QLabel *laser2StatusLabel;
    QSpacerItem *s2;
    QHBoxLayout *big2Row;
    QLabel *bigCurrent2;
    QLabel *unit2;
    QLabel *measuredLabel2;
    QHBoxLayout *step2Row;
    QPushButton *laser2DownBtn;
    QSpinBox *laser2spinbox;
    QPushButton *laser2UpBtn;
    QHBoxLayout *mode2Row;
    QLabel *mode2Title;
    QPushButton *laser2CoarseBtn;
    QPushButton *laser2FineBtn;
    QPushButton *laser2ParamsButton;
    QComboBox *laser2ModeCb;
    QLabel *reasonLabel2;
    QSpacerItem *vs2;
    QGroupBox *card3;
    QHBoxLayout *card3Layout;
    LaserChart *chartWidget3;
    QWidget *ctrl3Wrap;
    QVBoxLayout *ctrl3Layout;
    QHBoxLayout *status3Row;
    QLabel *ledLabel3;
    QLabel *laser3StatusLabel;
    QSpacerItem *s3;
    QHBoxLayout *big3Row;
    QLabel *bigCurrent3;
    QLabel *unit3;
    QLabel *measuredLabel3;
    QHBoxLayout *step3Row;
    QPushButton *laser3DownBtn;
    QSpinBox *laser3spinbox;
    QPushButton *laser3UpBtn;
    QHBoxLayout *mode3Row;
    QLabel *mode3Title;
    QSpacerItem *s3b;
    QPushButton *laser3ParamsButton;
    QComboBox *laser3ModeCb;
    QLabel *reasonLabel3;
    QSpacerItem *vs3;
    QPlainTextEdit *receiveEdit;
    QLabel *developerTemperatureBypassWarningLabel;
    QHBoxLayout *serialRow;
    QSpacerItem *sp1;
    QLabel *label;
    QComboBox *serialCb;
    QPushButton *openBt;
    QPushButton *closeBt;
    QPushButton *clearBt;
    QPushButton *temperatureBypassButton;
    QPushButton *sendBt;
    QPushButton *tryButton;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName(QString::fromUtf8("Widget"));
        Widget->resize(1180, 832);
        QFont font;
        font.setFamily(QString::fromUtf8("Arial"));
        Widget->setFont(font);
        rootLayout = new QVBoxLayout(Widget);
        rootLayout->setSpacing(6);
        rootLayout->setContentsMargins(11, 11, 11, 11);
        rootLayout->setObjectName(QString::fromUtf8("rootLayout"));
        rootLayout->setContentsMargins(8, 8, 8, 8);
        titleBar = new QWidget(Widget);
        titleBar->setObjectName(QString::fromUtf8("titleBar"));
        titleBar->setVisible(false);

        rootLayout->addWidget(titleBar);

        card1 = new QGroupBox(Widget);
        card1->setObjectName(QString::fromUtf8("card1"));
        card1Layout = new QHBoxLayout(card1);
        card1Layout->setSpacing(8);
        card1Layout->setContentsMargins(11, 11, 11, 11);
        card1Layout->setObjectName(QString::fromUtf8("card1Layout"));
        chartWidget1 = new LaserChart(card1);
        chartWidget1->setObjectName(QString::fromUtf8("chartWidget1"));
        chartWidget1->setMinimumSize(QSize(0, 180));

        card1Layout->addWidget(chartWidget1);

        ctrl1Wrap = new QWidget(card1);
        ctrl1Wrap->setObjectName(QString::fromUtf8("ctrl1Wrap"));
        ctrl1Wrap->setMinimumSize(QSize(320, 0));
        ctrl1Wrap->setMaximumSize(QSize(360, 16777215));
        ctrl1Layout = new QVBoxLayout(ctrl1Wrap);
        ctrl1Layout->setSpacing(4);
        ctrl1Layout->setContentsMargins(11, 11, 11, 11);
        ctrl1Layout->setObjectName(QString::fromUtf8("ctrl1Layout"));
        status1Row = new QHBoxLayout();
        status1Row->setSpacing(6);
        status1Row->setObjectName(QString::fromUtf8("status1Row"));
        ledLabel1 = new QLabel(ctrl1Wrap);
        ledLabel1->setObjectName(QString::fromUtf8("ledLabel1"));

        status1Row->addWidget(ledLabel1);

        laser1StatusLabel = new QLabel(ctrl1Wrap);
        laser1StatusLabel->setObjectName(QString::fromUtf8("laser1StatusLabel"));

        status1Row->addWidget(laser1StatusLabel);

        s1 = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        status1Row->addItem(s1);


        ctrl1Layout->addLayout(status1Row);

        big1Row = new QHBoxLayout();
        big1Row->setSpacing(6);
        big1Row->setObjectName(QString::fromUtf8("big1Row"));
        bigCurrent1 = new QLabel(ctrl1Wrap);
        bigCurrent1->setObjectName(QString::fromUtf8("bigCurrent1"));
        bigCurrent1->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        big1Row->addWidget(bigCurrent1);

        unit1 = new QLabel(ctrl1Wrap);
        unit1->setObjectName(QString::fromUtf8("unit1"));

        big1Row->addWidget(unit1);


        ctrl1Layout->addLayout(big1Row);

        measuredLabel1 = new QLabel(ctrl1Wrap);
        measuredLabel1->setObjectName(QString::fromUtf8("measuredLabel1"));

        ctrl1Layout->addWidget(measuredLabel1);

        step1Row = new QHBoxLayout();
        step1Row->setSpacing(6);
        step1Row->setObjectName(QString::fromUtf8("step1Row"));
        laser1DownBtn = new QPushButton(ctrl1Wrap);
        laser1DownBtn->setObjectName(QString::fromUtf8("laser1DownBtn"));
        laser1DownBtn->setMinimumSize(QSize(40, 32));

        step1Row->addWidget(laser1DownBtn);

        laser1spinbox = new QSpinBox(ctrl1Wrap);
        laser1spinbox->setObjectName(QString::fromUtf8("laser1spinbox"));
        laser1spinbox->setAlignment(Qt::AlignHCenter);

        step1Row->addWidget(laser1spinbox);

        laser1UpBtn = new QPushButton(ctrl1Wrap);
        laser1UpBtn->setObjectName(QString::fromUtf8("laser1UpBtn"));
        laser1UpBtn->setMinimumSize(QSize(40, 32));

        step1Row->addWidget(laser1UpBtn);


        ctrl1Layout->addLayout(step1Row);

        mode1Row = new QHBoxLayout();
        mode1Row->setSpacing(6);
        mode1Row->setObjectName(QString::fromUtf8("mode1Row"));
        mode1Title = new QLabel(ctrl1Wrap);
        mode1Title->setObjectName(QString::fromUtf8("mode1Title"));

        mode1Row->addWidget(mode1Title);

        laser1CoarseBtn = new QPushButton(ctrl1Wrap);
        laser1CoarseBtn->setObjectName(QString::fromUtf8("laser1CoarseBtn"));
        laser1CoarseBtn->setCheckable(true);

        mode1Row->addWidget(laser1CoarseBtn);

        laser1FineBtn = new QPushButton(ctrl1Wrap);
        laser1FineBtn->setObjectName(QString::fromUtf8("laser1FineBtn"));
        laser1FineBtn->setCheckable(true);

        mode1Row->addWidget(laser1FineBtn);

        laser1ParamsButton = new QPushButton(ctrl1Wrap);
        laser1ParamsButton->setObjectName(QString::fromUtf8("laser1ParamsButton"));

        mode1Row->addWidget(laser1ParamsButton);

        laser1ModeCb = new QComboBox(ctrl1Wrap);
        laser1ModeCb->setObjectName(QString::fromUtf8("laser1ModeCb"));
        laser1ModeCb->setVisible(false);

        mode1Row->addWidget(laser1ModeCb);


        ctrl1Layout->addLayout(mode1Row);

        reasonLabel1 = new QLabel(ctrl1Wrap);
        reasonLabel1->setObjectName(QString::fromUtf8("reasonLabel1"));
        reasonLabel1->setWordWrap(true);

        ctrl1Layout->addWidget(reasonLabel1);

        vs1 = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        ctrl1Layout->addItem(vs1);


        card1Layout->addWidget(ctrl1Wrap);


        rootLayout->addWidget(card1);

        card2 = new QGroupBox(Widget);
        card2->setObjectName(QString::fromUtf8("card2"));
        card2Layout = new QHBoxLayout(card2);
        card2Layout->setSpacing(8);
        card2Layout->setContentsMargins(11, 11, 11, 11);
        card2Layout->setObjectName(QString::fromUtf8("card2Layout"));
        chartWidget2 = new LaserChart(card2);
        chartWidget2->setObjectName(QString::fromUtf8("chartWidget2"));
        chartWidget2->setMinimumSize(QSize(0, 180));

        card2Layout->addWidget(chartWidget2);

        ctrl2Wrap = new QWidget(card2);
        ctrl2Wrap->setObjectName(QString::fromUtf8("ctrl2Wrap"));
        ctrl2Wrap->setMinimumSize(QSize(320, 0));
        ctrl2Wrap->setMaximumSize(QSize(360, 16777215));
        ctrl2Layout = new QVBoxLayout(ctrl2Wrap);
        ctrl2Layout->setSpacing(4);
        ctrl2Layout->setContentsMargins(11, 11, 11, 11);
        ctrl2Layout->setObjectName(QString::fromUtf8("ctrl2Layout"));
        status2Row = new QHBoxLayout();
        status2Row->setSpacing(6);
        status2Row->setObjectName(QString::fromUtf8("status2Row"));
        ledLabel2 = new QLabel(ctrl2Wrap);
        ledLabel2->setObjectName(QString::fromUtf8("ledLabel2"));

        status2Row->addWidget(ledLabel2);

        laser2StatusLabel = new QLabel(ctrl2Wrap);
        laser2StatusLabel->setObjectName(QString::fromUtf8("laser2StatusLabel"));

        status2Row->addWidget(laser2StatusLabel);

        s2 = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        status2Row->addItem(s2);


        ctrl2Layout->addLayout(status2Row);

        big2Row = new QHBoxLayout();
        big2Row->setSpacing(6);
        big2Row->setObjectName(QString::fromUtf8("big2Row"));
        bigCurrent2 = new QLabel(ctrl2Wrap);
        bigCurrent2->setObjectName(QString::fromUtf8("bigCurrent2"));
        bigCurrent2->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        big2Row->addWidget(bigCurrent2);

        unit2 = new QLabel(ctrl2Wrap);
        unit2->setObjectName(QString::fromUtf8("unit2"));

        big2Row->addWidget(unit2);


        ctrl2Layout->addLayout(big2Row);

        measuredLabel2 = new QLabel(ctrl2Wrap);
        measuredLabel2->setObjectName(QString::fromUtf8("measuredLabel2"));

        ctrl2Layout->addWidget(measuredLabel2);

        step2Row = new QHBoxLayout();
        step2Row->setSpacing(6);
        step2Row->setObjectName(QString::fromUtf8("step2Row"));
        laser2DownBtn = new QPushButton(ctrl2Wrap);
        laser2DownBtn->setObjectName(QString::fromUtf8("laser2DownBtn"));
        laser2DownBtn->setMinimumSize(QSize(40, 32));

        step2Row->addWidget(laser2DownBtn);

        laser2spinbox = new QSpinBox(ctrl2Wrap);
        laser2spinbox->setObjectName(QString::fromUtf8("laser2spinbox"));
        laser2spinbox->setAlignment(Qt::AlignHCenter);

        step2Row->addWidget(laser2spinbox);

        laser2UpBtn = new QPushButton(ctrl2Wrap);
        laser2UpBtn->setObjectName(QString::fromUtf8("laser2UpBtn"));
        laser2UpBtn->setMinimumSize(QSize(40, 32));

        step2Row->addWidget(laser2UpBtn);


        ctrl2Layout->addLayout(step2Row);

        mode2Row = new QHBoxLayout();
        mode2Row->setSpacing(6);
        mode2Row->setObjectName(QString::fromUtf8("mode2Row"));
        mode2Title = new QLabel(ctrl2Wrap);
        mode2Title->setObjectName(QString::fromUtf8("mode2Title"));

        mode2Row->addWidget(mode2Title);

        laser2CoarseBtn = new QPushButton(ctrl2Wrap);
        laser2CoarseBtn->setObjectName(QString::fromUtf8("laser2CoarseBtn"));
        laser2CoarseBtn->setCheckable(true);

        mode2Row->addWidget(laser2CoarseBtn);

        laser2FineBtn = new QPushButton(ctrl2Wrap);
        laser2FineBtn->setObjectName(QString::fromUtf8("laser2FineBtn"));
        laser2FineBtn->setCheckable(true);

        mode2Row->addWidget(laser2FineBtn);

        laser2ParamsButton = new QPushButton(ctrl2Wrap);
        laser2ParamsButton->setObjectName(QString::fromUtf8("laser2ParamsButton"));

        mode2Row->addWidget(laser2ParamsButton);

        laser2ModeCb = new QComboBox(ctrl2Wrap);
        laser2ModeCb->setObjectName(QString::fromUtf8("laser2ModeCb"));
        laser2ModeCb->setVisible(false);

        mode2Row->addWidget(laser2ModeCb);


        ctrl2Layout->addLayout(mode2Row);

        reasonLabel2 = new QLabel(ctrl2Wrap);
        reasonLabel2->setObjectName(QString::fromUtf8("reasonLabel2"));
        reasonLabel2->setWordWrap(true);

        ctrl2Layout->addWidget(reasonLabel2);

        vs2 = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        ctrl2Layout->addItem(vs2);


        card2Layout->addWidget(ctrl2Wrap);


        rootLayout->addWidget(card2);

        card3 = new QGroupBox(Widget);
        card3->setObjectName(QString::fromUtf8("card3"));
        card3Layout = new QHBoxLayout(card3);
        card3Layout->setSpacing(8);
        card3Layout->setContentsMargins(11, 11, 11, 11);
        card3Layout->setObjectName(QString::fromUtf8("card3Layout"));
        chartWidget3 = new LaserChart(card3);
        chartWidget3->setObjectName(QString::fromUtf8("chartWidget3"));
        chartWidget3->setMinimumSize(QSize(0, 180));

        card3Layout->addWidget(chartWidget3);

        ctrl3Wrap = new QWidget(card3);
        ctrl3Wrap->setObjectName(QString::fromUtf8("ctrl3Wrap"));
        ctrl3Wrap->setMinimumSize(QSize(320, 0));
        ctrl3Wrap->setMaximumSize(QSize(360, 16777215));
        ctrl3Layout = new QVBoxLayout(ctrl3Wrap);
        ctrl3Layout->setSpacing(4);
        ctrl3Layout->setContentsMargins(11, 11, 11, 11);
        ctrl3Layout->setObjectName(QString::fromUtf8("ctrl3Layout"));
        status3Row = new QHBoxLayout();
        status3Row->setSpacing(6);
        status3Row->setObjectName(QString::fromUtf8("status3Row"));
        ledLabel3 = new QLabel(ctrl3Wrap);
        ledLabel3->setObjectName(QString::fromUtf8("ledLabel3"));

        status3Row->addWidget(ledLabel3);

        laser3StatusLabel = new QLabel(ctrl3Wrap);
        laser3StatusLabel->setObjectName(QString::fromUtf8("laser3StatusLabel"));

        status3Row->addWidget(laser3StatusLabel);

        s3 = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        status3Row->addItem(s3);


        ctrl3Layout->addLayout(status3Row);

        big3Row = new QHBoxLayout();
        big3Row->setSpacing(6);
        big3Row->setObjectName(QString::fromUtf8("big3Row"));
        bigCurrent3 = new QLabel(ctrl3Wrap);
        bigCurrent3->setObjectName(QString::fromUtf8("bigCurrent3"));
        bigCurrent3->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        big3Row->addWidget(bigCurrent3);

        unit3 = new QLabel(ctrl3Wrap);
        unit3->setObjectName(QString::fromUtf8("unit3"));

        big3Row->addWidget(unit3);


        ctrl3Layout->addLayout(big3Row);

        measuredLabel3 = new QLabel(ctrl3Wrap);
        measuredLabel3->setObjectName(QString::fromUtf8("measuredLabel3"));

        ctrl3Layout->addWidget(measuredLabel3);

        step3Row = new QHBoxLayout();
        step3Row->setSpacing(6);
        step3Row->setObjectName(QString::fromUtf8("step3Row"));
        laser3DownBtn = new QPushButton(ctrl3Wrap);
        laser3DownBtn->setObjectName(QString::fromUtf8("laser3DownBtn"));
        laser3DownBtn->setMinimumSize(QSize(40, 32));

        step3Row->addWidget(laser3DownBtn);

        laser3spinbox = new QSpinBox(ctrl3Wrap);
        laser3spinbox->setObjectName(QString::fromUtf8("laser3spinbox"));
        laser3spinbox->setAlignment(Qt::AlignHCenter);

        step3Row->addWidget(laser3spinbox);

        laser3UpBtn = new QPushButton(ctrl3Wrap);
        laser3UpBtn->setObjectName(QString::fromUtf8("laser3UpBtn"));
        laser3UpBtn->setMinimumSize(QSize(40, 32));

        step3Row->addWidget(laser3UpBtn);


        ctrl3Layout->addLayout(step3Row);

        mode3Row = new QHBoxLayout();
        mode3Row->setSpacing(6);
        mode3Row->setObjectName(QString::fromUtf8("mode3Row"));
        mode3Title = new QLabel(ctrl3Wrap);
        mode3Title->setObjectName(QString::fromUtf8("mode3Title"));

        mode3Row->addWidget(mode3Title);

        s3b = new QSpacerItem(20, 10, QSizePolicy::Expanding, QSizePolicy::Minimum);

        mode3Row->addItem(s3b);

        laser3ParamsButton = new QPushButton(ctrl3Wrap);
        laser3ParamsButton->setObjectName(QString::fromUtf8("laser3ParamsButton"));

        mode3Row->addWidget(laser3ParamsButton);

        laser3ModeCb = new QComboBox(ctrl3Wrap);
        laser3ModeCb->setObjectName(QString::fromUtf8("laser3ModeCb"));
        laser3ModeCb->setVisible(false);

        mode3Row->addWidget(laser3ModeCb);


        ctrl3Layout->addLayout(mode3Row);

        reasonLabel3 = new QLabel(ctrl3Wrap);
        reasonLabel3->setObjectName(QString::fromUtf8("reasonLabel3"));
        reasonLabel3->setWordWrap(true);

        ctrl3Layout->addWidget(reasonLabel3);

        vs3 = new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding);

        ctrl3Layout->addItem(vs3);


        card3Layout->addWidget(ctrl3Wrap);


        rootLayout->addWidget(card3);

        receiveEdit = new QPlainTextEdit(Widget);
        receiveEdit->setObjectName(QString::fromUtf8("receiveEdit"));
        receiveEdit->setMinimumSize(QSize(0, 70));
        receiveEdit->setMaximumSize(QSize(16777215, 90));
        receiveEdit->setReadOnly(true);

        rootLayout->addWidget(receiveEdit);

        developerTemperatureBypassWarningLabel = new QLabel(Widget);
        developerTemperatureBypassWarningLabel->setObjectName(QString::fromUtf8("developerTemperatureBypassWarningLabel"));
        developerTemperatureBypassWarningLabel->setVisible(false);
        developerTemperatureBypassWarningLabel->setAlignment(Qt::AlignCenter);
        developerTemperatureBypassWarningLabel->setWordWrap(true);

        rootLayout->addWidget(developerTemperatureBypassWarningLabel);

        serialRow = new QHBoxLayout();
        serialRow->setSpacing(6);
        serialRow->setObjectName(QString::fromUtf8("serialRow"));
        sp1 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        serialRow->addItem(sp1);

        label = new QLabel(Widget);
        label->setObjectName(QString::fromUtf8("label"));

        serialRow->addWidget(label);

        serialCb = new QComboBox(Widget);
        serialCb->setObjectName(QString::fromUtf8("serialCb"));

        serialRow->addWidget(serialCb);

        openBt = new QPushButton(Widget);
        openBt->setObjectName(QString::fromUtf8("openBt"));

        serialRow->addWidget(openBt);

        closeBt = new QPushButton(Widget);
        closeBt->setObjectName(QString::fromUtf8("closeBt"));

        serialRow->addWidget(closeBt);

        clearBt = new QPushButton(Widget);
        clearBt->setObjectName(QString::fromUtf8("clearBt"));

        serialRow->addWidget(clearBt);

        temperatureBypassButton = new QPushButton(Widget);
        temperatureBypassButton->setObjectName(QString::fromUtf8("temperatureBypassButton"));
        temperatureBypassButton->setMinimumSize(QSize(120, 32));

        serialRow->addWidget(temperatureBypassButton);

        sendBt = new QPushButton(Widget);
        sendBt->setObjectName(QString::fromUtf8("sendBt"));
        sendBt->setVisible(false);

        serialRow->addWidget(sendBt);

        tryButton = new QPushButton(Widget);
        tryButton->setObjectName(QString::fromUtf8("tryButton"));
        tryButton->setMinimumSize(QSize(120, 32));

        serialRow->addWidget(tryButton);


        rootLayout->addLayout(serialRow);


        retranslateUi(Widget);

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "\346\277\200\345\205\211\345\231\250\346\216\247\345\210\266\347\263\273\347\273\237", nullptr));
        Widget->setStyleSheet(QCoreApplication::translate("Widget", "QWidget { background-color: #1a1a1a; color: #E0E0E0; }\n"
"QPushButton { background-color: #444444; color: #E0E0E0; border: 1px solid #555555; border-radius: 3px; padding: 4px 12px; font-weight: bold; }\n"
"QPushButton:hover { background-color: #CC5500; color: white; }\n"
"QPushButton:disabled { background-color: #2a2a2a; color: #666666; border-color: #333333; }\n"
"QGroupBox { color: #FFFFFF; border: 1px solid #555555; border-radius: 5px; margin-top: 10px; padding-top: 16px; font-weight: bold; }\n"
"QGroupBox::title { subcontrol-origin: margin; left: 12px; padding: 0 6px; }\n"
"QGroupBox#card1 { border: 1px solid #00C8FF; }\n"
"QGroupBox#card2 { border: 1px solid #FFA040; }\n"
"QGroupBox#card3 { border: 1px solid #C080FF; }\n"
"QGroupBox#card1::title { color: #00C8FF; }\n"
"QGroupBox#card2::title { color: #FFA040; }\n"
"QGroupBox#card3::title { color: #C080FF; }\n"
"QComboBox { background-color: #333333; color: #E0E0E0; border: 1px solid #555555; padding: 2px 6px; }\n"
"QSpinBox { background-color: #333333; c"
                        "olor: #E0E0E0; border: 1px solid #555555; padding: 2px; }\n"
"QPlainTextEdit { background-color: #111111; color: #00CC66; border: 1px solid #555555; }\n"
"QLabel { color: #CCCCCC; }\n"
"QLabel#bigCurrent1 { color: #00C8FF; font-size: 28px; font-weight: bold; }\n"
"QLabel#bigCurrent2 { color: #FFA040; font-size: 28px; font-weight: bold; }\n"
"QLabel#bigCurrent3 { color: #C080FF; font-size: 28px; font-weight: bold; }\n"
"QLabel#measuredLabel1, QLabel#measuredLabel2, QLabel#measuredLabel3 { color: #FFC840; font-size: 11px; }\n"
"QLabel#reasonLabel1, QLabel#reasonLabel2, QLabel#reasonLabel3 { color: #FF6060; font-size: 11px; font-style: italic; }\n"
"QLabel#ledLabel1, QLabel#ledLabel2, QLabel#ledLabel3 { font-size: 18px; font-weight: bold; }\n"
"QLabel#developerTemperatureBypassWarningLabel { color: #FFDFA3; background-color: #3A2A12; border: 1px solid #D69A1E; border-radius: 4px; padding: 5px 10px; font-size: 12px; font-weight: bold; }\n"
"QPushButton#tryButton { background-color: #2196F3; }\n"
"QPushButton#tryBu"
                        "tton:hover { background-color: #CC5500; }", nullptr));
        card1->setTitle(QCoreApplication::translate("Widget", "\346\277\200\345\205\211\345\231\250 1", nullptr));
        ledLabel1->setStyleSheet(QCoreApplication::translate("Widget", "color: #FF4040;", nullptr));
        ledLabel1->setText(QCoreApplication::translate("Widget", "\342\227\217", nullptr));
        laser1StatusLabel->setText(QCoreApplication::translate("Widget", "\346\234\252\345\260\261\347\273\252", nullptr));
        bigCurrent1->setText(QCoreApplication::translate("Widget", "0", nullptr));
        unit1->setStyleSheet(QCoreApplication::translate("Widget", "font-size:14px;", nullptr));
        unit1->setText(QCoreApplication::translate("Widget", "mA", nullptr));
        measuredLabel1->setText(QCoreApplication::translate("Widget", "\345\256\236\346\265\213: --", nullptr));
        laser1DownBtn->setText(QCoreApplication::translate("Widget", "\342\210\222", nullptr));
        laser1UpBtn->setText(QCoreApplication::translate("Widget", "\357\274\213", nullptr));
        mode1Title->setText(QCoreApplication::translate("Widget", "\346\250\241\345\274\217", nullptr));
        laser1CoarseBtn->setText(QCoreApplication::translate("Widget", "\347\262\227\350\260\203 \302\26110", nullptr));
        laser1FineBtn->setText(QCoreApplication::translate("Widget", "\347\273\206\350\260\203 \302\2611", nullptr));
        laser1ParamsButton->setText(QCoreApplication::translate("Widget", "\345\217\202\346\225\260", nullptr));
        reasonLabel1->setText(QString());
        card2->setTitle(QCoreApplication::translate("Widget", "\346\277\200\345\205\211\345\231\250 2", nullptr));
        ledLabel2->setStyleSheet(QCoreApplication::translate("Widget", "color: #FF4040;", nullptr));
        ledLabel2->setText(QCoreApplication::translate("Widget", "\342\227\217", nullptr));
        laser2StatusLabel->setText(QCoreApplication::translate("Widget", "\346\234\252\345\260\261\347\273\252", nullptr));
        bigCurrent2->setText(QCoreApplication::translate("Widget", "0", nullptr));
        unit2->setStyleSheet(QCoreApplication::translate("Widget", "font-size:14px;", nullptr));
        unit2->setText(QCoreApplication::translate("Widget", "mA", nullptr));
        measuredLabel2->setText(QCoreApplication::translate("Widget", "\345\256\236\346\265\213: --", nullptr));
        laser2DownBtn->setText(QCoreApplication::translate("Widget", "\342\210\222", nullptr));
        laser2UpBtn->setText(QCoreApplication::translate("Widget", "\357\274\213", nullptr));
        mode2Title->setText(QCoreApplication::translate("Widget", "\346\250\241\345\274\217", nullptr));
        laser2CoarseBtn->setText(QCoreApplication::translate("Widget", "\347\262\227\350\260\203 \302\26110", nullptr));
        laser2FineBtn->setText(QCoreApplication::translate("Widget", "\347\273\206\350\260\203 \302\2611", nullptr));
        laser2ParamsButton->setText(QCoreApplication::translate("Widget", "\345\217\202\346\225\260", nullptr));
        reasonLabel2->setText(QString());
        card3->setTitle(QCoreApplication::translate("Widget", "\346\277\200\345\205\211\345\231\250 3", nullptr));
        ledLabel3->setStyleSheet(QCoreApplication::translate("Widget", "color: #FF4040;", nullptr));
        ledLabel3->setText(QCoreApplication::translate("Widget", "\342\227\217", nullptr));
        laser3StatusLabel->setText(QCoreApplication::translate("Widget", "\346\234\252\345\260\261\347\273\252", nullptr));
        bigCurrent3->setText(QCoreApplication::translate("Widget", "800", nullptr));
        unit3->setStyleSheet(QCoreApplication::translate("Widget", "font-size:14px;", nullptr));
        unit3->setText(QCoreApplication::translate("Widget", "mA", nullptr));
        measuredLabel3->setText(QCoreApplication::translate("Widget", "\345\256\236\346\265\213: --", nullptr));
        laser3DownBtn->setText(QCoreApplication::translate("Widget", "\342\210\222", nullptr));
        laser3UpBtn->setText(QCoreApplication::translate("Widget", "\357\274\213", nullptr));
        mode3Title->setText(QCoreApplication::translate("Widget", "\346\255\245\351\225\277 100 mA", nullptr));
        laser3ParamsButton->setText(QCoreApplication::translate("Widget", "\345\217\202\346\225\260", nullptr));
        reasonLabel3->setText(QString());
        developerTemperatureBypassWarningLabel->setText(QCoreApplication::translate("Widget", "\346\270\251\345\272\246\346\227\201\350\267\257\345\267\262\345\274\200\345\220\257\357\274\232\344\270\212\344\275\215\346\234\272\346\234\252\351\252\214\350\257\201\346\270\251\345\272\246\345\260\261\347\273\252\357\274\214\346\234\200\347\273\210\344\277\235\346\212\244\344\276\235\350\265\226\344\270\213\344\275\215\346\234\272", nullptr));
        label->setText(QCoreApplication::translate("Widget", "\344\270\262\345\217\243\345\217\267", nullptr));
        openBt->setText(QCoreApplication::translate("Widget", "\346\211\223\345\274\200\344\270\262\345\217\243", nullptr));
        closeBt->setText(QCoreApplication::translate("Widget", "\345\205\263\351\227\255\344\270\262\345\217\243", nullptr));
        clearBt->setText(QCoreApplication::translate("Widget", "\346\270\205\347\251\272\346\226\207\346\234\254", nullptr));
        temperatureBypassButton->setText(QCoreApplication::translate("Widget", "\346\270\251\345\272\246\346\227\201\350\267\257: \345\205\263\351\227\255", nullptr));
        sendBt->setText(QCoreApplication::translate("Widget", "\345\217\221\351\200\201\346\225\260\346\215\256", nullptr));
        tryButton->setText(QCoreApplication::translate("Widget", "\345\205\250\346\256\265\346\211\253\346\217\217 (TRY)", nullptr));
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
