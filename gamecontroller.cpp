#include "gamecontroller.h"
#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>

gamecontroller::gamecontroller(QObject *parent, QTcpSocket *_socket, MainWindow *_window) :
    QObject(parent),
    socket(_socket),
    window(_window)
{
    qDebug()<<socket;
    connect(socket, &QTcpSocket::readyRead, this, &gamecontroller::handleInput);
    connect(this,&gamecontroller::gameover,this,&gamecontroller::finish);
}
void gamecontroller::handleInput()
{
    QString raw = socket->readAll();
    QStringList rawlist = raw.split(QLatin1Char('@'),Qt::SkipEmptyParts);
    foreach(QString str,rawlist){
        str = str.trimmed();
        qDebug()<<str;
        switch (status)
        {
            case dialog:
                socket->write(QByteArray("received"));
                status = uninitialized;
                break;
            case uninitialized:{
                setInitial(str);
                socket->flush();
                socket->write(QByteArray("initialized"));
                status = callingForLandlord;
                break;
            }
            case callingForLandlord:{
                if(str == "startcalling"){
                    window->setButtonsVisibility();
                    break;
                }
                qDebug()<<str;
                int user_from = str.split(QLatin1Char(':'),Qt::SkipEmptyParts)[0].toInt();
                QString userChoice = str.split(QLatin1Char(':'),Qt::SkipEmptyParts)[1].trimmed();
                if(userChoice.split(QLatin1Char(' ')).size() == 1){
                    if(userChoice == "yes")
                        window->playerInfoDisp[idToLocalPlayer[user_from]]->setText(tr("叫地主"));
                    else
                        window->playerInfoDisp[idToLocalPlayer[user_from]]->setText(tr("不叫"));
                }else{
                    setTheThreeCards(str,user_from);
                    status = waiting;
                    window->setButtonsVisibility();
                }
                break;
            }
            case waiting:{
                if(str == "yourturn"){
                    status = playing;
                    if(onBoardCardsFrom == userId)//有牌权，不能不出
                    {   qDebug()<<userId<<onBoardCardsFrom;
                        window->setButtonsVisibility(false);
                    }else{
                        qDebug()<<userId<<onBoardCardsFrom;
                        window->setButtonsVisibility();//参数缺省值为true
                    }
                }else{
                    setOnboardCards(str);
                }
                break;
            }
            case playing:
                break;
            case end:{
                if(str == "yes"){
                    status = uninitialized;
                    window->setButtonsVisibility();
                    socket->write(QByteArray("received"));
                    socket->flush();
                }else if(str == "no"){
                    QTimer::singleShot(200,QApplication::instance(),&QApplication::quit);
                }
                break;
            }
            default:
                break;
        }
    }
}

//17张起始牌,以传递过来的字符串为参数
void gamecontroller::setInitial(QString str)
{
    QString user = str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[0];
    userId = user.toInt();
    if(str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[1].trimmed() == "null"){
    }else{
        QStringList cards = str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[1].split(QLatin1Char(' '), Qt::SkipEmptyParts);
        currentCards = QSet<QString>(cards.begin(), cards.end());
        window->paintHandCards(currentCards.values());
        int count = 0;
        for(int i=1;i<3;++i){
            int current = (userId +3 - i)%3;
            idToLocalPlayer.insert(current,count++);
        }
    }
}
//三张牌的字符串,预计字符形式为 地主: XX XX XX;同时确定游戏的地主
void gamecontroller::setTheThreeCards(QString str,int landlord)
{
    QStringList cards = str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[1].split(QLatin1Char(' '), Qt::SkipEmptyParts);
    window->paintConstCards(cards);
    for(int i=0;i<3;++i){
        if(i == userId){
            if(i == landlord){
                isLandlord = true;
                foreach(QString card,cards){
                    currentCards.insert(card);
                    window->paintHandCards(currentCards.values());
                }
            }else{
                isLandlord = false;
            }
        }else{
            if(i == landlord){
                players[idToLocalPlayer[i]].isLandlord = true;
                players[idToLocalPlayer[i]].cardRemain = 20;
                players[idToLocalPlayer[i]].id = i;
                window->playerRemainDisp[idToLocalPlayer[i]]->setNum(20);
            }else{
                players[idToLocalPlayer[i]].isLandlord = false;
                players[idToLocalPlayer[i]].cardRemain = 17;
                players[idToLocalPlayer[i]].id = i;
                window->playerRemainDisp[idToLocalPlayer[i]]->setNum(17);
            }
        }
    }
    onBoardCardsFrom = landlord;
    window->displayIdentity();
}
//设置、记录并展示打出来的牌，由于在本地已经检测过，所以一定是合法的;预计形式为 1: XX XX XX ，下一步是否打牌将有服务器发送指令
void gamecontroller::setOnboardCards(QString str)
{
    str = str.split(QLatin1Char('@'),Qt::SkipEmptyParts)[0];
    if(str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[1].trimmed() != QString("null")){
        QStringList cards = str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[1].split(QLatin1Char(' '), Qt::SkipEmptyParts);
        onBoardCards = QSet<QString>(cards.begin(), cards.end());
        window->paintOnboardCards(onBoardCards.values());
        now = getCardsType(onBoardCards);
        onBoardCardsFrom = str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[0].toUInt();
        window->displayBuchu(onBoardCardsFrom,true);//将不出的信息删去
        if(onBoardCardsFrom != userId){
            players[idToLocalPlayer[onBoardCardsFrom]].cardRemain -= cards.size();
            window->playerRemainDisp[idToLocalPlayer[onBoardCardsFrom]]->setNum(
                    players[idToLocalPlayer[onBoardCardsFrom]].cardRemain);
        }
        if(players[idToLocalPlayer[onBoardCardsFrom]].cardRemain == 0){
            status = stage::end;
            emit gameover();
        }
    }else{
        int i = str.split(QLatin1Char(':'), Qt::SkipEmptyParts)[0].toUInt();
        window->displayBuchu(i,false);
    }
}

bool gamecontroller::legalCardsComb(QSet<QString> a)
{
    cardType tmp = getCardsType(a);
    if(now.type == gamecontroller::comboType::error && tmp.type != error)
        return true;
    if(onBoardCardsFrom == userId && tmp.type != error)
        return true;
    bool legal = true;
    switch (tmp.type) {
        case error:
            legal = false;
            break;
        case kingbomb:
            legal = true;
            break;
        case bomb:{
            if(now.type == kingbomb){
                legal = false;
            }else if(now.type == bomb){
                legal = tmp.base > now.base;
            }else{
                legal = true;
            }
        }
            break;
        default:
            legal = (tmp.type == now.type) and (tmp.base > now.base) and (tmp.extra == now.extra);
            break;

    }
    if(!legal){
        QMessageBox::information(nullptr, tr("!"), tr("牌型错误"));
    }
    return legal;
}

cardType gamecontroller::getCardsType(QSet<QString> a)
{
    QMap<int, int> statistics;
    foreach (QString card, a)
    {
        if (statistics.contains(getCardNumb(card)))
            statistics[getCardNumb(card)]++;
        else
            statistics.insert(getCardNumb(card), 1);
    }
    cardType tmp{error, 0, 0};
    switch (a.size())
    {
    case 1:
    {
        qDebug() << "单张" << getCardNumb(a.values()[0]);
        tmp = cardType{single, getCardNumb(a.values()[0]), 0};
        break;
    }
    case 2:
    {
        if (a.values()[0][1] == a.values()[1][1])
        {
            if (getCardNumb(a.values()[0]) >= 16 and getCardNumb(a.values()[1]) >= 16)
            {
                qDebug() << "王炸";
                tmp = cardType{kingbomb, 16, 0};
            }
            else
            {
                qDebug() << "对子";
                tmp = cardType{couple, getCardNumb(a.values()[0]), 0};
            }
        }
        else
        {
            qDebug() << "错误";
        }
        break;
    }
    case 3:
    {
        if (statistics.size() == 1)
        {
            tmp = cardType{threewith, statistics.firstKey(), 0};
        }
    }
    break;
    case 4:
    {
        switch (statistics.size())
        {
        case 1:
            tmp = cardType{bomb, statistics.values()[0], 0};
            break;
        case 2:
            if (statistics[statistics.firstKey()] == 2)
            {
                qDebug() << "错误类型";
            }
            else
            {
                qDebug() << "三带一";
                tmp = cardType{threewith, statistics[statistics.firstKey()] == 3 ? statistics.firstKey() : statistics.lastKey(), 1};
            }
            break;
        default:
            qDebug() << "错误";
            break;
        }
    }
    break;
    case 5:
    {
        switch (statistics.size())
        {
        case 5:
        {
            bool isSequence = true;
            for (int i = 0; i < 5; ++i)
            {
                if (!statistics.contains(i + statistics.firstKey()))
                {
                    isSequence = false;
                    break;
                }
            }
            if (isSequence)
                tmp = cardType{sequence, statistics.firstKey(), 5};
            break;
        }
        case 3:
            break;
        case 2:
        {
            if (statistics.values().contains(3))
            {
                qDebug() << "三带二";
                tmp = cardType{threewith, statistics[statistics.firstKey()] == 3 ? statistics.firstKey() : statistics.lastKey(), 2};
            }
            break;
        }
        default:
            break;
        }
    }
    break;
    default:
    {
        qDebug()<<statistics<<statistics.values()<<a.size();
        //检查顺子
        if (statistics.size() == a.size())
        {
            qDebug()<<"检查顺子";
            for (int i = 0; i < a.size(); ++i)
            {
                if (!statistics.contains(i + statistics.firstKey())){
                    qDebug()<<"顺子错误";
                    return tmp;
                }
            }
            qDebug() << "顺子";
            tmp = cardType{sequence, statistics.firstKey(), statistics.lastKey() - statistics.firstKey() + 1};
        }
        //检查连对
        else if (statistics.size() * 2 == a.size())
        {
            qDebug()<<"进入连对检查";
            for (int i = 0; i < statistics.size(); ++i)
            {
                if (!statistics.contains(i + statistics.firstKey()) or !(statistics[i + statistics.firstKey()] == 2))
                {
                    return tmp;
                }
            }
            qDebug() << "连对";
            return cardType{sequentialCouples, statistics.firstKey(), statistics.lastKey() - statistics.firstKey() + 1};
        }
        //四带二
        else if (statistics.values().contains(4))
        {
                qDebug()<<"四带二检查";
            if (a.size() == 6)
            {
                foreach (QString card, a)
                {
                    if (statistics[getCardNumb(card)] == 4)
                        tmp = cardType{fourwith, getCardNumb(card), 2};
                }
            }
            else if (a.size() == 8 and
                     statistics.size() == 3 and
                     statistics.values().contains(2))
            {
                foreach (QString card, a)
                {
                    if (statistics[getCardNumb(card)] == 4)
                        tmp = cardType{fourwith, getCardNumb(card), 4};
                }
                qDebug() << "四带两对";
            }
        }
        //检查飞机
        else
        {
            qDebug()<<"飞机检查";
            int j = statistics.firstKey();
            while (statistics[j] != 3)
            {
                j++;
            }
            if (statistics[j + 1] == 3)
            {
                switch (statistics.size())
                {
                case 2: //flight without wing
                {
                    qDebug() << "flight";
                    tmp = cardType{flight, j, 0};
                    break;
                }
                case 4: //flight with wings
                {
                    if (a.size() == 8)
                    {
                        qDebug() << "flight small wing";
                        tmp = cardType{flight, j, 1};
                    }
                    else if (a.size() == 10)
                    {
                        if (statistics.values().toSet().size() == 2)
                        {
                            qDebug() << "flight big wings";
                            tmp = cardType{flight, j, 2};
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    break;
    }
    return tmp;
}

int gamecontroller::getCardNumb(QString card)
{
    if (card == "BJ")
        return 16;
    else if(card == "RJ")
        return 17;
    else if (card[1] == 'J')
        return 11;
    else if (card[1] == 'Q')
        return 12;
    else if (card[1] == 'K')
        return 13;
    else if (card[1] == '1' and card.size() == 2)
        return 14;
    else if (card[1] == '1' and card.size() == 3)//注意A和10的区别，10在这里是唯一的三个长度的卡牌名称
        return 10;
    else if (card[1] == '2')
        return 15;
    else{
        return card[1].toLatin1() - '0';
    }
}

bool gamecontroller::sendCards(QSet<QString> a)
{
    if (legalCardsComb(a))
    {
        currentCards.subtract(a);
        window->scene->clear(); //应该重新定义这个接口；同时也可以加到paint界面中去
        window->paintHandCards(currentCards.values());
        QByteArray toWrite ("@");
        toWrite += QByteArray::number(userId) + QByteArray(": ");
        foreach(QString card, a){
            toWrite += (QString(tr(" "))+card);
        }
        toWrite += QByteArray("@");
        setOnboardCards(toWrite);
        socket->write(toWrite);
        socket->flush();
        if(currentCards.isEmpty()){
            status = stage::end;
            emit gameover();
            return false;
        }
        return true;
    }
    return false;
}

void gamecontroller::send(QByteArray s){
    //QDataStream out (socket);
    //out<<QString::number(userId)<<" "<<s;
    QByteArray tmp = "@"+QByteArray::number(userId);
    tmp.append(':').append(' ');
    tmp.append(s);
    tmp += " @";
    socket->write(tmp);
}

gamecontroller::stage gamecontroller::currentStatus(){
    return status;
}

void gamecontroller::finish(){
    if(currentCards.isEmpty()){
        QMessageBox::information(nullptr,tr("恭喜！"),tr("您赢了！"));
    }
    else{
        for(int i=0;i<2;++i){
            if(players[i].cardRemain == 0){
                if(players[i].isLandlord == isLandlord){
                    QMessageBox::information(nullptr,tr("恭喜！"),tr("您赢了！"));
                }else{
                    QMessageBox::information(nullptr,tr(""),tr("您输了"));
                }
                break;
            }
        }
    }
    status = end;
    window->setButtonsVisibility();
}

void gamecontroller::handleGameAgain(){
    currentCards.clear();
    onBoardCards.clear();
    the3cards.clear();
    idToLocalPlayer.clear();
    socket->write(QByteArray("yes"));
    socket->flush();
}
