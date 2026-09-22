#ifndef DIALOGTEST_H
#define DIALOGTEST_H

#include <QDialog>

namespace Ui {
class DialogTest;
}

class DialogTest : public QDialog
{
    Q_OBJECT

public:
    explicit DialogTest(QWidget *parent = nullptr);
    ~DialogTest();

signals:
    void sendRequested(quint64 hash, double value);
    void getRequested(quint64 hash);

public slots:
    void setInputEventValue(quint64 hash, double value);
    void showInputEventValueUnavailable(quint64 hash);

private slots:
    void on_btnGet_clicked();
    void on_btnSend_clicked();

private:
    Ui::DialogTest *ui;
};

#endif // DIALOGTEST_H
