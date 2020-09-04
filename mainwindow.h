#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QMainWindow>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include "gamecontroller.h"

class card;
class netGameController;
class PreparationDialog;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setStatus(gamecontroller::stage a);

    /*画图*/
    void setButtonsVisibility(bool ifNoEnabled = true);
    void paintHandCards(QStringList a);
    void paintConstCards(QStringList a);//绘制地主剩余的三张牌
    void paintOnboardCards(QStringList a);
    void displayIdentity();
    void displayBuchu(int id,bool);//显示不出牌的信息;true代表打出了牌，false为没打
    /*相应函数*/
    void yes();
    void no();
    //bool eventFilter(QObject* watcher,QEvent* event) override;
    PreparationDialog* dialog(){
        return d;
    }

private:
    friend class gamecontroller;
    friend class netGameController;
    friend class PreparationDialog;
    Ui::MainWindow *ui;
    QGraphicsView* view;
    QGraphicsScene* scene;
    QGraphicsScene* constScene;//这个是展示地主牌的场景
    QLabel* playerInfoDisp[2];
    QLabel* playerBuchuDisp[2];
    QLabel* playerRemainDisp[2];


    PreparationDialog* d;
    QTcpSocket* socket;
    gamecontroller* game;

    QSet<QString> selected_cards;
    QSet<card*> cards;
};
#endif // MAINWINDOW_H
