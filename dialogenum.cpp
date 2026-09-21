#include "dialogenum.h"
#include "ui_dialogenum.h"

DialogEnum::DialogEnum(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogEnum)
{
    ui->setupUi(this);
}

DialogEnum::~DialogEnum()
{
    delete ui;
}
