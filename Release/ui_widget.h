/********************************************************************************
** Form generated from reading UI file 'widget.ui'
**
** Created by: Qt User Interface Compiler version 6.7.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_WIDGET_H
#define UI_WIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFrame>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLCDNumber>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_Widget
{
public:
    QGridLayout *gridLayout_4;
    QGroupBox *groupBox;
    QGridLayout *gridLayout;
    QComboBox *cbConfig;
    QPushButton *pbtnFolder;
    QCheckBox *ckbAuto;
    QLabel *lbInfo;
    QPushButton *pbtnConnect;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *lbDetail;
    QFrame *line;
    QPushButton *pbtnEnum;
    QSpacerItem *horizontalSpacer;
    QGroupBox *groupBox_3;
    QGridLayout *gridLayout_3;
    QHBoxLayout *horizontalLayout;
    QCheckBox *ckbRA;
    QCheckBox *ckbLR;
    QHBoxLayout *horizontalLayout_2;
    QLCDNumber *lcdRA;
    QSpacerItem *horizontalSpacer_2;
    QLCDNumber *lcdLR;
    QHBoxLayout *horizontalLayout_3;
    QLabel *label_3;
    QLabel *lbPage;
    QSpacerItem *horizontalSpacer_3;

    void setupUi(QWidget *Widget)
    {
        if (Widget->objectName().isEmpty())
            Widget->setObjectName("Widget");
        Widget->resize(388, 394);
        Widget->setMinimumSize(QSize(388, 394));
        Widget->setMaximumSize(QSize(388, 394));
        gridLayout_4 = new QGridLayout(Widget);
        gridLayout_4->setObjectName("gridLayout_4");
        groupBox = new QGroupBox(Widget);
        groupBox->setObjectName("groupBox");
        groupBox->setMinimumSize(QSize(370, 130));
        groupBox->setMaximumSize(QSize(370, 130));
        QFont font;
        font.setPointSize(10);
        groupBox->setFont(font);
        gridLayout = new QGridLayout(groupBox);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(6, 6, 6, 8);
        cbConfig = new QComboBox(groupBox);
        cbConfig->addItem(QString());
        cbConfig->setObjectName("cbConfig");
        cbConfig->setMinimumSize(QSize(240, 32));
        cbConfig->setMaximumSize(QSize(240, 32));
        QFont font1;
        font1.setPointSize(12);
        cbConfig->setFont(font1);

        gridLayout->addWidget(cbConfig, 0, 0, 1, 1);

        pbtnFolder = new QPushButton(groupBox);
        pbtnFolder->setObjectName("pbtnFolder");
        pbtnFolder->setMinimumSize(QSize(108, 32));
        pbtnFolder->setMaximumSize(QSize(108, 32));
        pbtnFolder->setFont(font1);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/Resoure/OpenFile.png"), QSize(), QIcon::Normal, QIcon::Off);
        pbtnFolder->setIcon(icon);
        pbtnFolder->setIconSize(QSize(20, 20));

        gridLayout->addWidget(pbtnFolder, 0, 1, 1, 1);

        ckbAuto = new QCheckBox(groupBox);
        ckbAuto->setObjectName("ckbAuto");
        QFont font2;
        font2.setPointSize(11);
        ckbAuto->setFont(font2);

        gridLayout->addWidget(ckbAuto, 1, 0, 1, 2);

        lbInfo = new QLabel(groupBox);
        lbInfo->setObjectName("lbInfo");
        lbInfo->setFont(font1);

        gridLayout->addWidget(lbInfo, 2, 0, 1, 1);

        pbtnConnect = new QPushButton(groupBox);
        pbtnConnect->setObjectName("pbtnConnect");
        pbtnConnect->setMinimumSize(QSize(108, 32));
        pbtnConnect->setMaximumSize(QSize(108, 32));
        pbtnConnect->setFont(font1);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/Resoure/CoilBalck.png"), QSize(), QIcon::Normal, QIcon::Off);
        pbtnConnect->setIcon(icon1);
        pbtnConnect->setIconSize(QSize(18, 18));

        gridLayout->addWidget(pbtnConnect, 2, 1, 1, 1);


        gridLayout_4->addWidget(groupBox, 0, 0, 1, 1);

        groupBox_2 = new QGroupBox(Widget);
        groupBox_2->setObjectName("groupBox_2");
        groupBox_2->setMinimumSize(QSize(370, 100));
        groupBox_2->setMaximumSize(QSize(370, 100));
        groupBox_2->setFont(font);
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName("gridLayout_2");
        gridLayout_2->setContentsMargins(6, 6, 6, 5);
        lbDetail = new QLabel(groupBox_2);
        lbDetail->setObjectName("lbDetail");
        QFont font3;
        font3.setPointSize(12);
        font3.setBold(true);
        lbDetail->setFont(font3);

        gridLayout_2->addWidget(lbDetail, 0, 0, 1, 2);

        line = new QFrame(groupBox_2);
        line->setObjectName("line");
        line->setFrameShape(QFrame::Shape::HLine);
        line->setFrameShadow(QFrame::Shadow::Sunken);

        gridLayout_2->addWidget(line, 1, 0, 1, 2);

        pbtnEnum = new QPushButton(groupBox_2);
        pbtnEnum->setObjectName("pbtnEnum");
        pbtnEnum->setMinimumSize(QSize(80, 30));
        pbtnEnum->setMaximumSize(QSize(80, 30));
        pbtnEnum->setFont(font1);
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/Resoure/com.png"), QSize(), QIcon::Normal, QIcon::Off);
        pbtnEnum->setIcon(icon2);
        pbtnEnum->setIconSize(QSize(20, 20));

        gridLayout_2->addWidget(pbtnEnum, 2, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(263, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        gridLayout_2->addItem(horizontalSpacer, 2, 1, 1, 1);


        gridLayout_4->addWidget(groupBox_2, 1, 0, 1, 1);

        groupBox_3 = new QGroupBox(Widget);
        groupBox_3->setObjectName("groupBox_3");
        groupBox_3->setMinimumSize(QSize(370, 0));
        groupBox_3->setMaximumSize(QSize(370, 16777215));
        groupBox_3->setFont(font);
        gridLayout_3 = new QGridLayout(groupBox_3);
        gridLayout_3->setObjectName("gridLayout_3");
        gridLayout_3->setContentsMargins(6, 5, 6, 6);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        ckbRA = new QCheckBox(groupBox_3);
        ckbRA->setObjectName("ckbRA");
        ckbRA->setMinimumSize(QSize(220, 20));
        ckbRA->setMaximumSize(QSize(220, 20));
        ckbRA->setFont(font2);

        horizontalLayout->addWidget(ckbRA);

        ckbLR = new QCheckBox(groupBox_3);
        ckbLR->setObjectName("ckbLR");
        ckbLR->setMinimumSize(QSize(120, 20));
        ckbLR->setMaximumSize(QSize(120, 20));
        ckbLR->setFont(font2);

        horizontalLayout->addWidget(ckbLR);


        gridLayout_3->addLayout(horizontalLayout, 0, 0, 1, 1);

        horizontalLayout_2 = new QHBoxLayout();
        horizontalLayout_2->setObjectName("horizontalLayout_2");
        lcdRA = new QLCDNumber(groupBox_3);
        lcdRA->setObjectName("lcdRA");
        lcdRA->setMinimumSize(QSize(90, 40));
        lcdRA->setMaximumSize(QSize(90, 40));
        lcdRA->setSizeIncrement(QSize(120, 40));
        QFont font4;
        font4.setPointSize(11);
        font4.setBold(false);
        lcdRA->setFont(font4);
        lcdRA->setSmallDecimalPoint(true);
        lcdRA->setDigitCount(4);
        lcdRA->setMode(QLCDNumber::Mode::Dec);
        lcdRA->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdRA->setProperty("value", QVariant(2500.000000000000000));
        lcdRA->setProperty("intValue", QVariant(2500));

        horizontalLayout_2->addWidget(lcdRA);

        horizontalSpacer_2 = new QSpacerItem(118, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_2->addItem(horizontalSpacer_2);

        lcdLR = new QLCDNumber(groupBox_3);
        lcdLR->setObjectName("lcdLR");
        lcdLR->setMinimumSize(QSize(120, 40));
        lcdLR->setMaximumSize(QSize(120, 40));
        lcdLR->setSizeIncrement(QSize(120, 40));
        lcdLR->setFont(font4);
        lcdLR->setSmallDecimalPoint(true);
        lcdLR->setDigitCount(5);
        lcdLR->setMode(QLCDNumber::Mode::Dec);
        lcdLR->setSegmentStyle(QLCDNumber::SegmentStyle::Flat);
        lcdLR->setProperty("value", QVariant(-1234.000000000000000));
        lcdLR->setProperty("intValue", QVariant(-1234));

        horizontalLayout_2->addWidget(lcdLR);


        gridLayout_3->addLayout(horizontalLayout_2, 1, 0, 1, 1);


        gridLayout_4->addWidget(groupBox_3, 2, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName("horizontalLayout_3");
        label_3 = new QLabel(Widget);
        label_3->setObjectName("label_3");
        label_3->setMinimumSize(QSize(100, 20));
        label_3->setMaximumSize(QSize(100, 20));
        label_3->setFont(font4);

        horizontalLayout_3->addWidget(label_3);

        lbPage = new QLabel(Widget);
        lbPage->setObjectName("lbPage");
        lbPage->setMinimumSize(QSize(60, 20));
        lbPage->setMaximumSize(QSize(60, 20));
        lbPage->setSizeIncrement(QSize(90, 20));

        horizontalLayout_3->addWidget(lbPage);

        horizontalSpacer_3 = new QSpacerItem(178, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout_3->addItem(horizontalSpacer_3);


        gridLayout_4->addLayout(horizontalLayout_3, 3, 0, 1, 1);


        retranslateUi(Widget);

        QMetaObject::connectSlotsByName(Widget);
    } // setupUi

    void retranslateUi(QWidget *Widget)
    {
        Widget->setWindowTitle(QCoreApplication::translate("Widget", "Widget", nullptr));
        groupBox->setTitle(QCoreApplication::translate("Widget", "Setting", nullptr));
        cbConfig->setItemText(0, QCoreApplication::translate("Widget", "Cessna 172 Cessna Cessna", nullptr));

        pbtnFolder->setText(QCoreApplication::translate("Widget", "Folder", nullptr));
        ckbAuto->setText(QCoreApplication::translate("Widget", "Auto-Switching Configuration", nullptr));
        lbInfo->setText(QCoreApplication::translate("Widget", "Standby...", nullptr));
        pbtnConnect->setText(QCoreApplication::translate("Widget", "Connect", nullptr));
        groupBox_2->setTitle(QCoreApplication::translate("Widget", "Detail", nullptr));
        lbDetail->setText(QCoreApplication::translate("Widget", "CTRL+E", nullptr));
        pbtnEnum->setText(QCoreApplication::translate("Widget", "Enum", nullptr));
        groupBox_3->setTitle(QCoreApplication::translate("Widget", "Tools", nullptr));
        ckbRA->setText(QCoreApplication::translate("Widget", "Radio Altitude Callouts", nullptr));
        ckbLR->setText(QCoreApplication::translate("Widget", "Landing Rate", nullptr));
        label_3->setText(QCoreApplication::translate("Widget", "Config Page", nullptr));
        lbPage->setText(QString());
    } // retranslateUi

};

namespace Ui {
    class Widget: public Ui_Widget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_WIDGET_H
