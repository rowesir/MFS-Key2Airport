#include "dialogenum.h"
#include "ui_dialogenum.h"

#include <QHeaderView>
#include <QDateTime>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QStringList>
#include <QTableWidget>
#include <QTableWidgetItem>

DialogEnum::DialogEnum(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogEnum)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);
    initUI();
}

DialogEnum::~DialogEnum()
{
    delete ui;
}

void DialogEnum::initUI()
{
    enumAllItems.clear();
    enumParams.clear();
    listenRates.clear();

    ui->TWEnumAll->setColumnCount(3);
    ui->TWEnumAll->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Name")
                                                           << QStringLiteral("Hash")
                                                           << QStringLiteral("eType"));

    ui->TWListem->setColumnCount(6);
    ui->TWListem->setHorizontalHeaderLabels(QStringList() << QStringLiteral("Hash")
                                                          << QStringLiteral("eType")
                                                          << QStringLiteral("Value")
                                                          << QStringLiteral("Param")
                                                          << QStringLiteral("Size")
                                                          << QStringLiteral("Time"));

    const QList<QTableWidget *> tables = {ui->TWEnumAll, ui->TWListem};
    for (QTableWidget *table : tables) {
        table->setRowCount(0);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSortingEnabled(false);
        table->horizontalHeader()->setSectionsMovable(false);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    ui->leFiltra->clear();
    ui->leFiltra->setMaxLength(20);
    if (!ui->leFiltra->validator()) {
        ui->leFiltra->setValidator(new QRegularExpressionValidator(
            QRegularExpression(QStringLiteral("[\\x21-\\x7E]{0,20}")), ui->leFiltra));
    }
}

void DialogEnum::addEnumAll(const QString &name, quint64 hash, const QString &eType)
{
    enumAllItems.append({name, hash, eType});

    const QString filter = ui->leFiltra->text();
    if (!filter.isEmpty() && !name.contains(filter, Qt::CaseInsensitive)) return;

    const int row = ui->TWEnumAll->rowCount();
    ui->TWEnumAll->insertRow(row);
    ui->TWEnumAll->setItem(row, 0, new QTableWidgetItem(name));
    ui->TWEnumAll->setItem(row, 1, new QTableWidgetItem(QString::number(hash)));
    ui->TWEnumAll->setItem(row, 2, new QTableWidgetItem(eType));
}

void DialogEnum::refreshEnumAll()
{
    const QString filter = ui->leFiltra->text();

    ui->TWEnumAll->setRowCount(0);
    for (const EnumAllItem &item : enumAllItems) {
        if (!filter.isEmpty() && !item.name.contains(filter, Qt::CaseInsensitive)) continue;

        const int row = ui->TWEnumAll->rowCount();
        ui->TWEnumAll->insertRow(row);
        ui->TWEnumAll->setItem(row, 0, new QTableWidgetItem(item.name));
        ui->TWEnumAll->setItem(row, 1, new QTableWidgetItem(QString::number(item.hash)));
        ui->TWEnumAll->setItem(row, 2, new QTableWidgetItem(item.eType));
    }
}

void DialogEnum::on_leFiltra_textChanged(const QString &text)
{
    Q_UNUSED(text)
    refreshEnumAll();
}

void DialogEnum::removeListenRows(quint64 hash)
{
    for (int row = ui->TWListem->rowCount() - 1; row >= 0; --row) {
        QTableWidgetItem *hashItem = ui->TWListem->item(row, 0);
        if (!hashItem)
            continue;

        bool ok = false;
        const quint64 rowHash = hashItem->text().toULongLong(&ok);
        if (ok && rowHash == hash)
            ui->TWListem->removeRow(row);
    }
}

int DialogEnum::findListenRow(quint64 hash) const
{
    for (int row = 0; row < ui->TWListem->rowCount(); ++row) {
        QTableWidgetItem *hashItem = ui->TWListem->item(row, 0);
        if (!hashItem)
            continue;

        bool ok = false;
        const quint64 rowHash = hashItem->text().toULongLong(&ok);
        if (ok && rowHash == hash)
            return row;
    }

    return -1;
}

void DialogEnum::writeListenRow(int row, quint64 hash, const QString &eType, const QString &value,
                                const QString &param, int size, const QString &time)
{
    ui->TWListem->setItem(row, 0, new QTableWidgetItem(QString::number(hash)));
    ui->TWListem->setItem(row, 1, new QTableWidgetItem(eType));
    ui->TWListem->setItem(row, 2, new QTableWidgetItem(value));
    ui->TWListem->setItem(row, 3, new QTableWidgetItem(param));
    ui->TWListem->setItem(row, 4,
                          new QTableWidgetItem(size < 0 ? QString() : QString::number(size)));
    ui->TWListem->setItem(row, 5, new QTableWidgetItem(time));
}

void DialogEnum::addListen(quint64 hash, const QString &eType, const QString &value,
                           const QString &param, int size)
{
    QString displayParam = param;
    int displaySize = size;
    if (!param.isEmpty() || size >= 0) {
        enumParams.insert(hash, qMakePair(param, size));
    } else {
        const auto cachedParam = enumParams.constFind(hash);
        if (cachedParam != enumParams.constEnd()) {
            displayParam = cachedParam->first;
            displaySize = cachedParam->second;
        }
    }

    const QDateTime now = QDateTime::currentDateTime();
    const qint64 nowMs = now.toMSecsSinceEpoch();
    const QString time = now.time().toString(QStringLiteral("HH:mm:ss"));

    ListenRate &rate = listenRates[hash];
    while (!rate.timestamps.isEmpty() && nowMs - rate.timestamps.first() >= 1000)
        rate.timestamps.removeFirst();
    rate.timestamps.append(nowMs);

    if (!rate.folded && rate.timestamps.size() >= 8) {
        rate.folded = true;
        removeListenRows(hash);

        ui->TWListem->insertRow(0);
        writeListenRow(0, hash, eType, value, displayParam, displaySize, time);
        return;
    }

    if (rate.folded && rate.timestamps.size() <= 4) {
        rate.folded = false;
        removeListenRows(hash);
    }

    if (rate.folded) {
        int row = findListenRow(hash);
        if (row < 0) {
            ui->TWListem->insertRow(0);
            row = 0;
        }
        writeListenRow(row, hash, eType, value, displayParam, displaySize, time);
    } else {
        ui->TWListem->insertRow(0);
        writeListenRow(0, hash, eType, value, displayParam, displaySize, time);
    }
}

void DialogEnum::setEnumParam(quint64 hash, const QString &param, int size)
{
    enumParams.insert(hash, qMakePair(param, size));

    for (int row = 0; row < ui->TWListem->rowCount(); ++row) {
        QTableWidgetItem *hashItem = ui->TWListem->item(row, 0);
        if (!hashItem)
            continue;

        bool ok = false;
        const quint64 rowHash = hashItem->text().toULongLong(&ok);
        if (!ok || rowHash != hash)
            continue;

        ui->TWListem->setItem(row, 3, new QTableWidgetItem(param));
        ui->TWListem->setItem(row, 4,
                              new QTableWidgetItem(size < 0 ? QString() : QString::number(size)));
    }
}
