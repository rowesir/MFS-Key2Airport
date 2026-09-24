#include "dialogtest.h"
#include "ui_dialogtest.h"

#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>

DialogTest::DialogTest(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogTest)
{
    ui->setupUi(this);
    setWindowFlag(Qt::WindowStaysOnTopHint, true);

    ui->lnHash->setMaxLength(32);
    ui->lnHash->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[0-9]{0,32}")), ui->lnHash));
}

DialogTest::~DialogTest()
{
    delete ui;
}

void DialogTest::on_btnGet_clicked()
{
    const QString hashText = ui->lnHash->text();
    if (hashText.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Invalid Hash"),
                             QStringLiteral("Hash cannot be empty."));
        return;
    }

    bool ok = false;
    const quint64 hash = hashText.toULongLong(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, QStringLiteral("Invalid Hash"),
                             QStringLiteral("Hash is invalid."));
        return;
    }

    emit getRequested(hash);
}

void DialogTest::on_btnSend_clicked()
{
    const QString hashText = ui->lnHash->text();
    if (hashText.isEmpty())
    {
        QMessageBox::warning(this, QStringLiteral("Invalid Hash"),
                             QStringLiteral("Hash cannot be empty."));
        return;
    }

    bool ok = false;
    const quint64 hash = hashText.toULongLong(&ok);
    if (!ok)
    {
        QMessageBox::warning(this, QStringLiteral("Invalid Hash"),
                             QStringLiteral("Hash is invalid."));
        return;
    }

    emit sendRequested(hash, ui->sbValue->value());
}

void DialogTest::setInputEventValue(quint64 hash, double value)
{
    bool ok = false;
    const quint64 currentHash = ui->lnHash->text().toULongLong(&ok);
    if (ok && currentHash == hash)
        ui->sbValue->setValue(value);
}

void DialogTest::showInputEventValueUnavailable(quint64 hash)
{
    if (!isVisible())
        return;

    bool ok = false;
    const quint64 currentHash = ui->lnHash->text().toULongLong(&ok);
    if (!ok || currentHash != hash)
        return;

    QMessageBox::warning(this, QStringLiteral("Cannot Get Value"),
                         QStringLiteral("Unable to get value because the InputEvent is a string."));
}
