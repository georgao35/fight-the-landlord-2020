#ifndef GAMECONTROLLER_H
#define GAMECONTROLLER_H

#include <QObject>
#include <QString>
#include <QtNetwork>

struct cardType;
class MainWindow;

struct player{
    int id;
    int cardRemain;
    bool isLandlord;
};

class gamecontroller: public QObject
{
    Q_OBJECT
public:
    enum stage{
        dialog,uninitialized,callingForLandlord,playing,waiting,end,
    };
    enum comboType{
        single,couple,threewith,sequence,sequentialCouples,fourwith,flight,bomb,kingbomb,
        error,
    };

    explicit gamecontroller(QObject* parent = nullptr,
                            QTcpSocket* _socket = nullptr,
                            MainWindow* _window = nullptr);

    stage currentStatus();
    /*-逻辑判断函数-*/
    //bool legalCardsComb(QString a){return true;}
    bool legalCardsComb(QSet<QString> a);
    cardType getCardsType(QSet<QString> a);
    comboType getCardsCombType(QSet<QString> a);
    static int getCardNumb(QString card);
    /*-通信函数-*/
    //void sendCards(QString a){}
    bool sendCards(QSet<QString> a);//是否成功发送
    void handleInput();
    void send(QByteArray s);
    /*操作函数*/
    void initGame();
    void setInitial(QString str);
    void setTheThreeCards(QString str,int landlord);
    void setOnboardCards(QString str);
    void finish();
    void handleGameAgain();
    //QSet<QString> getCardsSetFromQString(QString a);
signals:
    void gameover();
private:
    QTcpSocket* socket;
    MainWindow* window;
    friend class MainWindow;

    stage status = dialog;
    int userId;
    bool isLandlord;
    QSet<QString> currentCards;//手牌

    QSet<QString> onBoardCards;//打出的牌
    int onBoardCardsFrom;
    QString the3cards;//三张地主牌
    player players[2];
    QMap<int,int> idToLocalPlayer;
};

struct cardType{
    gamecontroller::comboType type;
    int base;
    int extra;//三带的几，顺子多长等等
};

static cardType now = {gamecontroller::comboType::error,0,0};

#endif // GAMECONTROLLER_H
