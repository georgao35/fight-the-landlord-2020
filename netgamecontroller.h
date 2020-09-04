#ifndef NETGAMECONTROLLER_H
#define NETGAMECONTROLLER_H

#include <QObject>
#include <QtNetwork>

enum stage{
    unconnected,landlordUndecided,landLordDecided,playing,ending,endingDecided,
};
//0号为自身
struct netPlayer{
    uint16_t id;
    uint16_t cards;
    bool identity;//true为地主，false为农民
};

class netGameController: public QObject
{
    Q_OBJECT
public:
    netGameController();
    void handleInput(QTcpSocket* socket);
    void initForAll();
public slots:
    void handleConnection();
    void handleLandlord();//决定地主的方法
    void handleGaming();
    void handleFinish();//开始新游戏
    void broadcast(int from,QString mes);//发送信号
signals:
    //初始化完成等信号
    void bothConnected();
    void finishInitialized();
    void finishLandlordAsking();
    void finishPlaying();
    void newGame();
private:
    QTcpServer* server;//server不能在主线程中创建，否则就是主线程中的变量，
    QVector<QTcpSocket*> sockets;
    QMap<QTcpSocket*,int> socketsToId;

    QStringList cards;//初始化后的牌
    uint16_t landlord;
    int16_t handCard[3] = {17,17,17};
    stage status[3] = {unconnected,unconnected,unconnected};
};

#endif // NETGAMECONTROLLER_H
