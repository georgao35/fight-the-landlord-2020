#include "mainwindow.h"
#include "netgamecontroller.h"
#include "preparationdialog.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
