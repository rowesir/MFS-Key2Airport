#ifndef DIALOGENUM_H
#define DIALOGENUM_H

#include <QDialog>
#include <QString>

namespace Ui {
class DialogEnum;
}

class DialogEnum : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEnum(QWidget *parent = nullptr);
    ~DialogEnum();

    void initUI();

    void addEnumAll(const QString &name, quint64 hash, const QString &eType);

    void addListen(quint64 hash, const QString &eType, const QString &value,
                   const QString &param, int size);

private:
    Ui::DialogEnum *ui;
};

#endif // DIALOGENUM_H
