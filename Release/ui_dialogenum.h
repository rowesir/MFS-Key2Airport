/********************************************************************************
** Form generated from reading UI file 'dialogenum.ui'
**
** Created by: Qt User Interface Compiler version 6.8.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOGENUM_H
#define UI_DIALOGENUM_H

#include <QtCore/QVariant>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QTableWidget>

QT_BEGIN_NAMESPACE

class Ui_DialogEnum
{
public:
    QGridLayout *gridLayout;
    QHBoxLayout *horizontalLayout;
    QLabel *label;
    QSpacerItem *horizontalSpacer;
    QPushButton *btnTest;
    QLabel *label_3;
    QLineEdit *leFiltra;
    QTableWidget *TWEnumAll;
    QLabel *label_2;
    QTableWidget *TWListem;

    void setupUi(QDialog *DialogEnum)
    {
        if (DialogEnum->objectName().isEmpty())
            DialogEnum->setObjectName("DialogEnum");
        DialogEnum->resize(469, 469);
        DialogEnum->setMinimumSize(QSize(469, 469));
        gridLayout = new QGridLayout(DialogEnum);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setHorizontalSpacing(6);
        gridLayout->setVerticalSpacing(2);
        gridLayout->setContentsMargins(6, 6, 6, 6);
        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName("horizontalLayout");
        label = new QLabel(DialogEnum);
        label->setObjectName("label");
        label->setMinimumSize(QSize(60, 0));
        label->setMaximumSize(QSize(60, 16777215));

        horizontalLayout->addWidget(label);

        horizontalSpacer = new QSpacerItem(178, 20, QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        btnTest = new QPushButton(DialogEnum);
        btnTest->setObjectName("btnTest");
        btnTest->setMinimumSize(QSize(80, 27));
        btnTest->setMaximumSize(QSize(80, 27));
        QFont font;
        font.setPointSize(10);
        btnTest->setFont(font);
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/Resoure/Send.png"), QSize(), QIcon::Mode::Normal, QIcon::State::Off);
        btnTest->setIcon(icon);

        horizontalLayout->addWidget(btnTest);

        label_3 = new QLabel(DialogEnum);
        label_3->setObjectName("label_3");
        label_3->setMinimumSize(QSize(40, 0));
        label_3->setMaximumSize(QSize(40, 16777215));
        label_3->setAlignment(Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignTrailing|Qt::AlignmentFlag::AlignVCenter);

        horizontalLayout->addWidget(label_3);

        leFiltra = new QLineEdit(DialogEnum);
        leFiltra->setObjectName("leFiltra");
        leFiltra->setMinimumSize(QSize(150, 0));
        leFiltra->setMaximumSize(QSize(150, 16777215));
        leFiltra->setLayoutDirection(Qt::LayoutDirection::LeftToRight);
        leFiltra->setAlignment(Qt::AlignmentFlag::AlignRight|Qt::AlignmentFlag::AlignTrailing|Qt::AlignmentFlag::AlignVCenter);

        horizontalLayout->addWidget(leFiltra);


        gridLayout->addLayout(horizontalLayout, 0, 0, 1, 1);

        TWEnumAll = new QTableWidget(DialogEnum);
        TWEnumAll->setObjectName("TWEnumAll");

        gridLayout->addWidget(TWEnumAll, 1, 0, 1, 1);

        label_2 = new QLabel(DialogEnum);
        label_2->setObjectName("label_2");

        gridLayout->addWidget(label_2, 2, 0, 1, 1);

        TWListem = new QTableWidget(DialogEnum);
        TWListem->setObjectName("TWListem");

        gridLayout->addWidget(TWListem, 3, 0, 1, 1);


        retranslateUi(DialogEnum);

        QMetaObject::connectSlotsByName(DialogEnum);
    } // setupUi

    void retranslateUi(QDialog *DialogEnum)
    {
        DialogEnum->setWindowTitle(QCoreApplication::translate("DialogEnum", "Dialog", nullptr));
        label->setText(QCoreApplication::translate("DialogEnum", "Enum All:", nullptr));
        btnTest->setText(QCoreApplication::translate("DialogEnum", "Test", nullptr));
        label_3->setText(QCoreApplication::translate("DialogEnum", "Filtra: ", nullptr));
        leFiltra->setText(QString());
        label_2->setText(QCoreApplication::translate("DialogEnum", "Listen:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DialogEnum: public Ui_DialogEnum {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOGENUM_H
