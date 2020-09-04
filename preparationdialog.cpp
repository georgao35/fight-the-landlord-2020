#include "preparationdialog.h"
#include "ui_preparationdialog.h"
#include "netgamecontroller.h"

PreparationDialog::PreparationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreparationDialog)
{
    ui->setupUi(this);

    //handleConnection();
    connect(ui->linkButton,&QPushButton::clicked,[=](){
        emit buttonClicked();
    });
}

PreparationDialog::~PreparationDialog()
{
    delete ui;
}

