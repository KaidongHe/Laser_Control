#include "operatorform.h"
#include "ui_operatorform.h"

operatorForm::operatorForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::operatorForm)
{
    ui->setupUi(this);
}

operatorForm::~operatorForm()
{
    delete ui;
}
