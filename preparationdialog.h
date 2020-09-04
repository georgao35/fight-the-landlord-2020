#ifndef PREPARATIONDIALOG_H
#define PREPARATIONDIALOG_H

#include "mainwindow.h"
#include <QDialog>

namespace Ui {
class PreparationDialog;
}

class PreparationDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreparationDialog(QWidget *parent = nullptr);
    ~PreparationDialog();
    //void handleConnection();
signals:
    void buttonClicked();
private:
    Ui::PreparationDialog *ui;

/*
    netGameController* network;
    QTcpServer* server;
    QList<QTcpSocket*> sockets;
    int connected_sockets = 0;*/

    friend class MainWindow;
};

#endif // PREPARATIONDIALOG_H
