/********************************************************************************
** Form generated from reading UI file 'dialogtest.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOGTEST_H
#define UI_DIALOGTEST_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>

QT_BEGIN_NAMESPACE

class Ui_DialogTest
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QLineEdit *lnHash;
    QLabel *label_2;
    QDoubleSpinBox *sbValue;
    QHBoxLayout *horizontalLayout;
    QPushButton *btnGet;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnSend;

    void setupUi(QDialog *DialogTest)
    {
        if (DialogTest->objectName().isEmpty())
            DialogTest->setObjectName("DialogTest");
        DialogTest->resize(360, 130);
        DialogTest->setMinimumSize(QSize(360, 130));
        DialogTest->setMaximumSize(QSize(360, 130));
        gridLayout = new QGridLayout(DialogTest);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(6, 6, 6, 6);
        label = new QLabel(DialogTest);
        label->setObjectName("label");
        label->setMinimumSize(QSize(60, 32));
        label->setMaximumSize(QSize(60, 32));
        QFont font;
        font.setPointSize(12);
        label->setFont(font);

        gridLayout->addWidget(label, 0, 0, 1, 1);

        lnHash = new QLineEdit(DialogTest);
        lnHash->setObjectName("lnHash");
        lnHash->setMinimumSize(QSize(280, 32));
        lnHash->setMaximumSize(QSize(280, 32));
        lnHash->setFont(font);

        gridLayout->addWidget(lnHash, 0, 1, 1, 1);

        label_2 = new QLabel(DialogTest);
        label_2->setObjectName("label_2");
        label_2->setMinimumSize(QSize(60, 32));
        label_2->setMaximumSize(QSize(60, 25));
        label_2->setFont(font);

        gridLayout->addWidget(label_2, 1, 0, 1, 1);

        sbValue = new QDoubleSpinBox(DialogTest);
        sbValue->setObjectName("sbValue");
        sbValue->setMinimumSize(QSize(280, 32));
        sbValue->setMaximumSize(QSize(280, 32));
        sbValue->setFont(font);
        sbValue->setMinimum(-9999999.000000000000000);
        sbValue->setMaximum(9999999.000000000000000);

        gridLayout->addWidget(sbValue, 1, 1, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        btnGet = new QPushButton(DialogTest);
        btnGet->setObjectName("btnGet");
        btnGet->setMinimumSize(QSize(80, 32));
        btnGet->setMaximumSize(QSize(80, 32));
        btnGet->setFont(font);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/Resoure/Refresh.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        btnGet->setIcon(icon);
        btnGet->setIconSize(QSize(22, 22));

        horizontalLayout->addWidget(btnGet);

        horizontalSpacer = new QSpacerItem(158, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnSend = new QPushButton(DialogTest);
        btnSend->setObjectName("btnSend");
        btnSend->setMinimumSize(QSize(80, 32));
        btnSend->setMaximumSize(QSize(80, 32));
        btnSend->setFont(font);
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/Resoure/Send.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        btnSend->setIcon(icon1);

        horizontalLayout->addWidget(btnSend);


        gridLayout->addLayout(horizontalLayout, 2, 0, 1, 2);


        retranslateUi(DialogTest);

        QMetaObject::connectSlotsByName(DialogTest);
    } // setupUi

    void retranslateUi(QDialog *DialogTest)
    {
        DialogTest->setWindowTitle(QCoreApplication::translate("DialogTest", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("DialogTest", "Hash:", nullptr));
        lnHash->setText(QString());
        label_2->setText(QCoreApplication::translate("DialogTest", "Value:", nullptr));
        btnGet->setText(QCoreApplication::translate("DialogTest", "Get", nullptr));
        btnSend->setText(QCoreApplication::translate("DialogTest", "Set", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DialogTest: public Ui_DialogTest {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOGTEST_H
