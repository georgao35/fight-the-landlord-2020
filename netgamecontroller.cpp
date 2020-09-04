#include "netgamecontroller.h"

#include <QRandomGenerator>

inline int idToSocketId(int a){
    return (a+2)%3;
}

netGameController::netGameController()
{
    server = new QTcpServer(this);
    cards.push_back(tr(""));cards.push_back(tr(""));cards.push_back(tr(""));cards.push_back(tr(""));
    initForAll();//在重新开始游戏的地方也要重新来
    //connect(this,&netGameController::finishInitialized,this,&netGameController::handleLandlord);
    connect(this,&netGameController::finishInitialized,[=](){
       handleLandlord();
    });
    connect(this,&netGameController::finishLandlordAsking,[=](){
        for(int i=0;i<3;++i)
            status[i] = stage::playing;
        handleGaming();
    });
    connect(this,&netGameController::finishPlaying,[=](){
       for(int i=0;i<3;++i)
           status[i] = stage::ending;
    });
    connect(this,&netGameController::newGame,[=](){
        handleFinish();
    });
}

void netGameController::handleInput(QTcpSocket* socket){
    if(status[socketsToId[socket]] == stage::landlordUndecided)
        return;
    if(status[socketsToId[socket]] == stage::playing)
        return;
    QString src = socket->readAll();
    qDebug()<<src;
    switch (status[socketsToId[socket]]) {
        case unconnected:{
            if(src == "received"){
            QByteArray tmp = "@"+cards[socketsToId[socket]].toUtf8()+"@";
            socket->write(tmp);
            }else if(src == "initialized"){
                //qDebug()<<socketsToId[socket];
                status[socketsToId[socket]] = landlordUndecided;
                bool all = true;
                for(int i = 0;i<3;++i){
                    if(status[i] != landlordUndecided)
                        all = false;
                }
                if(all){
                    emit finishInitialized();
                }
            }
            break;
        }
        case landLordDecided:{
            //do nothing, go to the other function
            break;
        }
        case playing:{
            //do nothing, go to the other function
            break;
        }
        case ending:{
            if(src == "yes"){
                bool all = true;
                status[socketsToId[socket]] = endingDecided;
                socket->write(QByteArray("succes"));
                socket->flush();
                for(int i = 0;i<3;++i){
                    if(status[i] != endingDecided)
                        all = false;
                }
                if(all){
                    emit newGame();
                }
            }else if(src == "no"){
                broadcast(3,tr("no"));
            }
            break;
        }
        default:
            break;
    }
}

void netGameController::handleConnection(){
    server->listen(QHostAddress::Any,6665);
    connect(server,&QTcpServer::newConnection,[=](){
        QTcpSocket* socket = server->nextPendingConnection();
        sockets.push_back(socket);
        socketsToId[socket] = sockets.size()%3;
        connect(socket,&QTcpSocket::readyRead,[=](){
            handleInput(socket);
        });
        qDebug()<<sockets.size();
        if(sockets.size() == 2){
            foreach(QTcpSocket* socket,sockets){
                socket->write(QByteArray("succes"));
            }
            emit bothConnected();
        }
        if(sockets.size() == 3){
            socket->write(QByteArray("success"));
            qDebug()<<sockets;
        }
    });
}

void netGameController::initForAll(){
    QSet<int> allocatedCards;
    for(int i=0;i<3;++i){
        cards[i] = QString::number(i) + ": ";
        for(int j = 0;j<17;++j){
            int tmp = QRandomGenerator::global()->generate()%54 + 1;
            while(allocatedCards.contains(tmp)){
                tmp = QRandomGenerator::global()->generate()%54 + 1;
            }
            allocatedCards.insert(tmp);
            if(tmp == 53)
                cards[i] += "BJ ";
            else if(tmp == 54)
                cards[i] += "RJ ";
            else{
            QString remain;
            switch (tmp%13) {
                case 11:
                    remain = "J ";
                    break;
                case 12:
                    remain = "Q ";
                    break;
                case 0:
                    remain = "K ";
                    break;
                default:
                    remain = QString::number(tmp%13) + " ";
                    break;
            }
            switch ((tmp-1)/13) {
                case 0:{
                    cards[i] += "C";
                    cards[i] += remain;
                    break;
                }
                case 1:
                    cards[i] += "D";
                    cards[i] += remain;
                    break;
                case 2:
                    cards[i] += "H";
                    cards[i] += remain;
                    break;
                case 3:
                    cards[i] += "S";
                    cards[i] += remain;
                    break;
                default:
                    break;
            }
            }
            }
    }
    //取剩余的3个
    cards[3] = "";
    for(int tmp=1;tmp<=54;++tmp){
        int i =3;
        if(!allocatedCards.contains(tmp)){
            QString remain;
            if(tmp == 53)
                cards[i] += "BJ ";
            else if(tmp == 54)
                cards[i] += "RJ ";
            else{
                QString remain;
                switch (tmp%13) {
                    case 11:
                        remain = "J ";
                        break;
                    case 12:
                        remain = "Q ";
                        break;
                    case 0:
                        remain = "K ";
                        break;
                    default:
                        remain = QString::number(tmp%13) + " ";
                        break;
                }
                switch ((tmp-1)/13) {
                    case 0:{
                        cards[i] += "C";
                        cards[i] += remain;
                        break;
                    }
                    case 1:
                        cards[i] += "D";
                        cards[i] += remain;
                        break;
                    case 2:
                        cards[i] += "H";
                        cards[i] += remain;
                        break;
                    case 3:
                        cards[i] += "S";
                        cards[i] += remain;
                        break;
                    default:
                        break;
                }
            }
        }
    }
}
//不包括分割符的mes
void netGameController::broadcast(int from,QString mes){
    mes = "@" + mes + "@";
    for(int i=0;i<3;++i){
        if(i!=from){
            sockets[idToSocketId(i)]->write(mes.toUtf8());
            sockets[idToSocketId(i)]->flush();//当数据太小的时候tcp是不会发送的，如果需要立即同步，则需要用flush
        }
    }
}

void netGameController::handleLandlord(){
    qDebug()<<"entering handleLandlord";
    int first = QRandomGenerator::global()->generate()%3;
    landlord = first;
    int count = 0;
    for(;count<3;count++){
        int current = (first+count)%3;
        qDebug()<<current;
        sockets[idToSocketId(current)]->write(QByteArray("@startcalling@"));//发送开始的指令
        if(sockets[idToSocketId(current)]->waitForReadyRead(30000)){
            QString tmp = sockets[idToSocketId(current)]->readAll();
            tmp = tmp.split(QLatin1Char('@'),Qt::SkipEmptyParts)[0];
            if(!tmp.isEmpty())
            {
                qDebug()<<"netcontroller"<<tmp;
                QString tmp2 = tmp.split(QLatin1Char(':'))[1];
                if(tmp2.trimmed() == "yes"){
                    landlord = current;
                }
                broadcast(current,tmp);
            }else{
                broadcast(current,QString::number(current) + ": no");
            }
        }
    }
    qDebug()<<landlord;
    handCard[landlord] += 3;
    broadcast(3,QString::number(landlord)+": "+cards[3]);
    emit finishLandlordAsking();
}

void netGameController::handleGaming(){
    qDebug()<<"entering handlegaming";
    int first = landlord;
    int current = first;
    bool ended = false;
    while(!ended){
        qDebug()<<current;
        QTcpSocket* socket = sockets[idToSocketId(current)];
        socket->write(QByteArray("@yourturn@"));
        socket->flush();
        qDebug()<<"written";
        if(socket->waitForReadyRead(3000000)){
            QString tmp = socket->readAll();
            tmp = tmp.split(QLatin1Char('@'),Qt::SkipEmptyParts)[0];
            if(!tmp.isEmpty()){
                qDebug()<<tmp;
                broadcast(current,tmp);
                if(tmp.split(QLatin1Char(':'),Qt::SkipEmptyParts)[1].trimmed() != "null"){
                    QStringList cards = tmp.split(QLatin1Char(':'),Qt::SkipEmptyParts)[1].trimmed()
                                        .split(QLatin1Char(' '),Qt::SkipEmptyParts);
                    handCard[current] -= cards.size();
                    if(handCard[current] == 0){
                        ended = true;
                    }
                }
            }else{

            }
        }
        current ++;
        current %= 3;
    }
    emit finishPlaying();
}

void netGameController::handleFinish(){
    for(int i=0;i<3;++i){
        status[i] = unconnected;
        handCard[i] = 17;
    }
    initForAll();
    broadcast(3,tr("yes"));
}
