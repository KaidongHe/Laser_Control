#ifndef OPERATORFORM_H
#define OPERATORFORM_H

#include <QWidget>

namespace Ui {
class operatorForm;
}

class operatorForm : public QWidget
{
    Q_OBJECT

public:
    explicit operatorForm(QWidget *parent = nullptr);
    ~operatorForm();

private:
    Ui::operatorForm *ui;
};

#endif // OPERATORFORM_H
