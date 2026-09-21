#ifndef DIALOGENUM_H
#define DIALOGENUM_H

#include <QDialog>

namespace Ui {
class DialogEnum;
}

class DialogEnum : public QDialog
{
    Q_OBJECT

public:
    explicit DialogEnum(QWidget *parent = nullptr);
    ~DialogEnum();

private:
    Ui::DialogEnum *ui;
};

#endif // DIALOGENUM_H
