#include "dialogenum.h"
#include "ui_dialogenum.h"

#include <QHeaderView>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>

DialogEnum::DialogEnum(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogEnum)
{
    ui->setupUi(this);
    initUI();
}

DialogEnum::~DialogEnum()
{
    delete ui;
}

void DialogEnum::initUI()
{
    ui->TWEnumAll->setColumnCount(3);
    ui->TWEnumAll->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Name")
                                                           << QStringLiteral("Hash")
                                                           << QStringLiteral("eType"));

    ui->TWListem->setColumnCount(5);
    ui->TWListem->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Hash")
                                                          << QStringLiteral("eType")
                                                          << QStringLiteral("Value")
                                                          << QStringLiteral("Param")
                                                          << QStringLiteral("Size"));

    const QList<QTableWidget *> tables = {ui->TWEnumAll, ui->TWListem};
    for (QTableWidget *table : tables) {
        table->setRowCount(0);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSortingEnabled(false);
        table->horizontalHeader()->setSectionsMovable(false);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }
}

void DialogEnum::addEnumAll(const QString &name, quint64 hash, const QString &eType)
{
    const int row = ui->TWEnumAll->rowCount();
    ui->TWEnumAll->insertRow(row);
    ui->TWEnumAll->setItem(row, 0, new QTableWidgetItem(name));
    ui->TWEnumAll->setItem(row, 1, new QTableWidgetItem(QString::number(hash)));
    ui->TWEnumAll->setItem(row, 2, new QTableWidgetItem(eType));
}

void DialogEnum::addListen(quint64 hash, const QString &eType, const QString &value,
                           const QString &param, int size)
{
    ui->TWListem->insertRow(0);
    ui->TWListem->setItem(0, 0, new QTableWidgetItem(QString::number(hash)));
    ui->TWListem->setItem(0, 1, new QTableWidgetItem(eType));
    ui->TWListem->setItem(0, 2, new QTableWidgetItem(value));
    ui->TWListem->setItem(0, 3, new QTableWidgetItem(param));
    ui->TWListem->setItem(0, 4, new QTableWidgetItem(size < 0 ? QString() : QString::number(size)));
}
