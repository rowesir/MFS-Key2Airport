#ifndef DIALOGENUM_H
#define DIALOGENUM_H

#include <QDialog>
#include <QHash>
#include <QString>
#include <QVector>

namespace Ui {
class DialogEnum;
}

class DialogEnum : public QDialog
{
    Q_OBJECT

    struct EnumAllItem
    {
        QString name;
        quint64 hash;
        QString eType;
    };

    struct ListenRate
    {
        QVector<qint64> timestamps;
        bool folded = false;
    };

public:
    explicit DialogEnum(QWidget *parent = nullptr);
    ~DialogEnum();

    void initUI();

    void addEnumAll(const QString &name, quint64 hash, const QString &eType);

    void addListen(quint64 hash, const QString &eType, const QString &value,
                   const QString &param, int size);

    void setEnumParam(quint64 hash, const QString &param, int size);

signals:
    void testRequested();

private slots:
    void on_btnTest_clicked();
    void on_leFiltra_textChanged(const QString &text);

private:
    void refreshEnumAll();
    void removeListenRows(quint64 hash);
    int findListenRow(quint64 hash) const;
    void writeListenRow(int row, quint64 hash, const QString &eType, const QString &value,
                        const QString &param, int size, const QString &time);

    Ui::DialogEnum *ui;
    QVector<EnumAllItem> enumAllItems;
    QHash<quint64, QPair<QString, int>> enumParams;
    QHash<quint64, ListenRate> listenRates;
};

#endif // DIALOGENUM_H
