#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "preparationdialog.h"
#include "netgamecontroller.h"
#include "card.h"
#include "gamecontroller.h"
#include <algorithm>

bool cardCmp(QString a,QString b){
    return gamecontroller::getCardNumb(a)<gamecontroller::getCardNumb(b);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    socket = new QTcpSocket ();
    game = new gamecontroller(this,socket,this);
    netGameController* network = new netGameController;
    QThread * thread = new QThread;
    thread->start();
    network->moveToThread(thread);

    scene = new QGraphicsScene(this);
    constScene = new QGraphicsScene(this);
    ui->graphicsView->setScene(scene);
    ui->graphicsView_2->setScene(constScene);
    ui->onBoardView->setScene(new QGraphicsScene);
    setButtonsVisibility();

    QImage profile (tr(":/profile/human-icon.png"));
    ui->profile1->setPixmap(QPixmap::fromImage(profile.scaledToWidth(ui->profile1->width())));
    ui->profile2->setPixmap(QPixmap::fromImage(profile.scaledToWidth(ui->profile1->width())));
    playerInfoDisp[0] = ui->player1Message;
    playerInfoDisp[1] = ui->player2Message;
    playerBuchuDisp[0] = ui->player1played;
    playerBuchuDisp[1] = ui->player2played;
    playerRemainDisp[0] = ui->player1remain;
    playerRemainDisp[1] = ui->player2remain;

    d = new PreparationDialog(this);
    //
    //network->moveToThread(thread);
    connect(d,&PreparationDialog::buttonClicked,network,&netGameController::handleConnection);
    connect(network,&netGameController::bothConnected,d,&PreparationDialog::accept);
    //network->moveToThread(thread);

    if(d->exec() == QDialog::Accepted){
        game->socket->connectToHost(tr("127.0.0.1"),6665);
        connect(game->socket,&QTcpSocket::connected,[=](){
            setWindowTitle(tr("连接成功"));
        });
    }
    connect(ui->yesButton,&QPushButton::clicked,this,&MainWindow::yes);
    connect(ui->noButton,&QPushButton::clicked,this,&MainWindow::no);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintHandCards(QStringList a)
{
    //qDebug()<<a;
    cards.clear();//记得记录容器的初始化
    std::sort(a.begin(),a.end(),cardCmp);
    for(int i=0;i<a.size();++i){
        card* newCard = new card(a[i],i,true);
        //qDebug()<<newCard;
        scene->addItem(newCard);
        cards.insert(newCard);
    }
}

void MainWindow::paintConstCards(QStringList a){
    std::sort(a.begin(),a.end(),cardCmp);
    constScene->clear();
    for(int i=0;i<a.size();++i){
        card* newCard = new card(a[i],2.6*i);
        newCard->setPos(i*CARD_WIDTH,0);
        constScene->addItem(newCard);
    }
}

void MainWindow::paintOnboardCards(QStringList a){
    std::sort(a.begin(),a.end(),cardCmp);
    ui->onBoardView->scene()->clear();
    for(int i=0;i<a.size();++i){
        card* newCard = new card(a[i],i);
        newCard->setPos(i*CARD_WIDTH,0);
        ui->onBoardView->scene()->addItem(newCard);
    }
}

void MainWindow::yes(){
    switch (game->status) {
        case gamecontroller::stage::playing:
            selected_cards.clear();
            foreach(card* Card,cards){
                if(Card->selected)
                    selected_cards.insert(Card->name);
            }
            if(game->sendCards(selected_cards)){
                game->status = gamecontroller::stage::waiting;
                setButtonsVisibility();
            }
            break;
        case gamecontroller::stage::callingForLandlord:{
            QByteArray toSend ("yes");
            game->send(toSend);
            //发送信息后调整按钮
            ui->landlordOrNot->setText(tr("叫地主"));
            ui->yesButton->setVisible(false);
            ui->noButton->setVisible(false);
            break;
        }
        case gamecontroller::stage::end:{
            scene->clear();
            constScene->clear();
            ui->onBoardView->scene()->clear();
            for(int i=0;i<2;++i){
                playerBuchuDisp[i]->setText(tr(""));
                playerRemainDisp[i]->setText(tr(""));
            }
            game->handleGameAgain();
        }
        default:
            break;
    }
    //ui->yesButton->setEnabled(false);
}

void MainWindow::no(){
    switch (game->status) {
        case gamecontroller::stage::callingForLandlord:
        {
            QByteArray toSend ("no");
            game->send(toSend);
            //发送信息后调整按钮
            ui->landlordOrNot->setText(tr("不叫"));
            ui->yesButton->setVisible(false);
            ui->noButton->setVisible(false);
            break;
        }
        case gamecontroller::stage::playing:
        {
            game->send("null");
            game->status = gamecontroller::stage::waiting;
            setButtonsVisibility();
            break;
        }
        case gamecontroller::stage::end:
        {
            game->socket->write(QByteArray("no"));
            game->socket->flush();
            break;
        }
        default:
            break;
    }
    //ui->noButton->setDisabled(true);
}

void MainWindow::setButtonsVisibility(bool ifNoEnabled){
    if(game->status == gamecontroller::stage::callingForLandlord){
        ui->yesButton->setVisible(true);
        ui->yesButton->setText(tr("叫地主"));
        ui->noButton->setVisible(true);
        ui->noButton->setText(tr("不叫"));
    }
    else if(game->status == gamecontroller::stage::playing){
        ui->yesButton->setVisible(true);
        ui->yesButton->setText(tr("出牌"));
        ui->noButton->setVisible(true);
        ui->noButton->setEnabled(ifNoEnabled);
        ui->noButton->setText(tr("不出"));
    }
    else if(game->status == gamecontroller::stage::end){
        ui->yesButton->setVisible(true);
        ui->yesButton->setText(tr("再来一局"));
        ui->noButton->setVisible(true);
        ui->noButton->setText(tr("退出"));
    }
    else{
        ui->yesButton->setVisible(false);
        ui->noButton->setVisible(false);
    }
}

void MainWindow::setStatus(gamecontroller::stage a){
    game->status = a;
}

void MainWindow::displayIdentity(){
    for(int i=0;i<3;++i){
        if(i == game->userId){
            if(game->isLandlord){
                ui->landlordOrNot->setText(tr("地主"));
            }else{
                ui->landlordOrNot->setText(tr("农民"));
            }
        }else{
            int tmp = game->idToLocalPlayer[i];
            if(game->players[tmp].isLandlord){
                playerInfoDisp[tmp]->setText(tr("地主"));
            }else{
                playerInfoDisp[tmp]->setText(tr("农民"));
            }
        }
    }
}

void MainWindow::displayBuchu(int id,bool played){
    if(id == game->userId)
        return;
    int localId = game->idToLocalPlayer[id];
    QLabel* tmp = playerBuchuDisp[localId];
    if(played){
        tmp->setText(tr(""));
    }else{
        tmp->setText(tr("不出"));
    }
}
