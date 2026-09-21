/********************************************************************************
** Form generated from reading UI file 'dialogenum.ui'
**
** Created by: Qt User Interface Compiler version 6.7.1
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DIALOGENUM_H
#define UI_DIALOGENUM_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QTableWidget>

QT_BEGIN_NAMESPACE

class Ui_DialogEnum
{
public:
    QGridLayout *gridLayout;
    QLabel *label;
    QTableWidget *TWEnumAll;
    QLabel *label_2;
    QTableWidget *TWListem;

    void setupUi(QDialog *DialogEnum)
    {
        if (DialogEnum->objectName().isEmpty())
            DialogEnum->setObjectName("DialogEnum");
        DialogEnum->resize(469, 452);
        gridLayout = new QGridLayout(DialogEnum);
        gridLayout->setObjectName("gridLayout");
        gridLayout->setContentsMargins(6, 6, 6, 6);
        label = new QLabel(DialogEnum);
        label->setObjectName("label");

        gridLayout->addWidget(label, 0, 0, 1, 1);

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
        label_2->setText(QCoreApplication::translate("DialogEnum", "Listen:", nullptr));
    } // retranslateUi

};

namespace Ui {
    class DialogEnum: public Ui_DialogEnum {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DIALOGENUM_H
