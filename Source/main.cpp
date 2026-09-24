#include "widget.h"

#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", "windows:darkmode=0");

    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));

    Widget w;
    w.show();
    return a.exec();
}
