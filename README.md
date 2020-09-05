# 网络斗地主设计文档

#### 一、系统环境

- 系统：macOS 10.15.6
- 版本：Qt 5.15.0 
- QtCreator: 4.12.2
- kit： Qt 5.15.0 clang 64bit

#### 二、客户端工作流程

1. 准备界面在点击准备界面上的连接后，将尝试连接，当三个客户端互相连接成功后，自动转到游戏界面<img src="/Users/gaojingyue/Library/Application Support/typora-user-images/截屏2020-09-04 下午8.08.04.png" alt="截屏2020-09-04 下午8.08.04" style="zoom: 25%;" />
2. 在进入游戏界面后可以进行叫地主等操作，前面玩家是否叫地主将会展示出来；叫地主和打牌的顺序都遵循下家在右侧的原则；

<img src="/Users/gaojingyue/Library/Application Support/typora-user-images/截屏2020-09-04 下午8.14.42.png" alt="截屏2020-09-04 下午8.14.42" style="zoom: 25%;" />

在叫完地主后，各玩家的身份以及剩余的牌数将会展示在用户的下方。从地主开始轮流出牌，点击牌将向上浮起，点击出牌即可出牌；若出牌不合法，将有提示，并可再次选择。若果不出则将在用户旁边显示“不出”，直到这个用户出牌了；当前用户有牌权时，不出按钮将会失效。

<img src="/Users/gaojingyue/Documents/CST/qt/cardgame/截屏2020-09-04 下午8.38.23.png" alt="截屏2020-09-04 下午8.38.23" style="zoom:25%;" />

当一个玩家打完所有牌时，游戏结束，将有弹窗提示当前玩家的胜负，界面将会出现“重新开始”和“退出”两个按钮；当所有玩家都选择重新开始后，将会重新开始一轮游戏；若有玩家选择退出，应用则将关闭。

#### 三、基本框架

整个应用主要分为互相独立的本地客户端以及负责互相连接的服务端两部分组成，可以分别运行在不同的线程中。

**（一）本地客户端**

本地客户端实现了 `MainWindow` `preparationdialog` `gamecontroller` 和 `card` 四个类，分别负责显示游戏主界面、显示准备界面、逻辑控制与通信、卡片的展示。同时客户端也记录了当前所处的游戏状态，以更好地分析、处理命令。

0.纸牌相关的实现

在`gamecontroller`类及整个游戏中，纸牌通常以`QString`的类型储存，`cardtype`则是进行比较、判断的主要用具。

卡牌类型`cardType`是新定义的一个结构体，包含以下成员；`comboType`是对于牌的类型的枚举，包含以下情况。

```c++
struct cardType{
    gamecontroller::comboType type;//牌的类型，是定义的一个枚举类，如三带、对子等等
    int base;//牌的基数，如三带中的三、顺子连对中的第一个等等，用于对大小进行判断
    int extra;//附加信息，如三带的几，顺子多长等等，用于对上述类型的进一步分类
};
```

```c++
enum comboType{
single,couple,threewith,sequence,sequentialCouples,fourwith,flight,bomb,kingbomb,
        error,
    };
```

1.`MainWindow`类

MainWindow类是设计、操控游戏界面的类，数据成员主要包括了各类部件，并且实现了进行绘图的成员函数。

- 主要数据成员

```c++
		Ui::MainWindow *ui;
    QGraphicsView* view;
    QGraphicsScene* scene;//展示手牌的场景
    QGraphicsScene* constScene;//展示地主牌的场景
		/*展示其他玩家身份、剩余手牌等信息的部件*/
		QLabel* playerInfoDisp[2];
    QLabel* playerBuchuDisp[2];
    QLabel* playerRemainDisp[2];

		PreparationDialog* d;//准备界面
    QTcpSocket* socket;//套接字
    gamecontroller* game;//游戏控制类

    QSet<QString> selected_cards;//在手牌中选中的扑克牌
    QSet<card*> cards;//所有card类部件的集合
```

在界面中，主要实现了两个按钮以及其他的显示部件。两个按钮分别表示同意和不同意。这两个按钮将根据不同的游戏状态来设置点击后的反应；其他的部件包括用来显示玩家身份、剩余手牌等等的部件。

- 主要接口函数与实现

```c++
		/*画图*/
    void setButtonsVisibility(bool ifNoEnabled = true);//根据当前的游戏状态
    void paintHandCards(QStringList a);//绘制手牌
    void paintConstCards(QStringList a);//绘制地主剩余的三张牌
    void paintOnboardCards(QStringList a);//根据接受的信息，绘制打出来的牌
    void displayIdentity();//显示当前玩家的状态信息
    void displayBuchu(int id,bool);//显示不出牌的信息;true代表打出了牌，false为没打
    /*相应函数*/
    void yes();//点击同意按钮（叫地主、出牌、重新开始）后的槽函数
    void no();//点击不同意按钮后的槽函数
```

2.`gamecontroller`类

`gamecontroller`类是游戏中负责逻辑、控制与通信交流的类，负责储存当前游戏状态、判断牌是否合法、收发并分析信息等功能。

- 游戏状态的定义

```c++
enum stage{
  dialog,//准备状态，尚未连接，游戏仍在准备界面
  uninitialized,//已经连接，尚未初始化
  callingForLandlord,//叫地主
  playing,waiting,//游戏过程中的状态，分别代表轮到当前用户出牌、当前用户不在出牌
  end,//游戏分出胜负，待决定是否重来
	};
```

- 主要数据成员

```c++
		QTcpSocket* socket;
    MainWindow* window;
		/*当前玩家的信息*/
    stage status = dialog;//游戏所处的状态
    int userId;//玩家在服务端中的序号
    bool isLandlord;//玩家是否是地主？
    QSet<QString> currentCards;//手牌
		/*本局游戏的信息*/
    QSet<QString> onBoardCards;//打出的牌
    int onBoardCardsFrom;//谁打出的牌，对于牌的合法判断、牌权判断有用
    QString the3cards;//三张地主牌
    player players[2];//其他的玩家信息
    QMap<int,int> idToLocalPlayer;//全局用户序号与本地存储序号之间的转换
```

- 主要函数

```c++
		/*-逻辑判断函数-*/
    //bool legalCardsComb(QString a){return true;}
    bool legalCardsComb(QSet<QString> a);//选中的手牌是否合法
    cardType getCardsType(QSet<QString> a);//获得当前手牌的牌型
    static int getCardNumb(QString card);//获得当前
    /*-通信函数-*/
    //void sendCards(QString a){}
    bool sendCards(QSet<QString> a);//将打出的牌向其他用户发送，并返回是否成功发送
    void handleInput();//接受并处理信息的枢纽，并会根据当前状态调用其他函数
    void send(QByteArray s);//传递特定的信息
    /*操作函数*/
    void setInitial(QString str);//初始化游戏
    void setTheThreeCards(QString str,int landlord);//设置地主，并展示地主牌
    void setOnboardCards(QString str);//对于其他用户打的牌进行处理
    void finish();//游戏结束后的处理
    void handleGameAgain();//如果要重新还是游戏的处理
```

**（二）服务器**

服务器线程只有一个`netgamecontroller`一个类，负责处理用户传递的信息，控制游戏全局，并向用户发送特定的命令。

```c++
public:
    netGameController();
    void handleInput(QTcpSocket* socket);//处理不同套接字发送来的字符
    void initForAll();
public slots:
    void handleConnection();//进行连接
    void handleLandlord();//处理叫地主的函数
    void handleGaming();//处理游戏中出牌的函数
    void handleFinish();//开始新游戏的函数
    void broadcast(int from,QString mes);//向其他客户端广播发送信息
signals:
    void bothConnected();//连接成功
    void finishInitialized();//完成初始化
    void finishLandlordAsking();//完成叫地主
    void finishPlaying();//游戏结束
    void newGame();//已经决定要重新开始
```

#### 四、通信协议

- 连接方式：三个应用中有一个应用作为主要的控制端，与其他两个应用相连，通过在单独的线程`netgamecontroller`来控制整个游戏的进行。

- 通信协议：应用通过`QTcpSocket`进行网络传输，传输的主要载体是`QByteArray`，具体的内容格式约定如下：
  - 传递信息的格式为：`@(extra_info: ) (instruct)@`
  - 信息的开头、结尾用@字符进行标记，区分不同的指令；
  - 指令部分包括success、received、startcalling等等，分别代表成功连接、已经收到等等；
  - 对于初始化、叫地主、出牌等场景，需要明确发出的用户的，将在指令前面加上 “用户序号: “。卡牌将以相隔一个空格的方式连接起来，如“C1 C3 D5 ”等；如果选择不出、不叫等，instruct部分将是"null "
- 在连接成功后，将向所有客户端发送success信号，客户端调用`QDialog::accept`函数，切换到游戏界面，向服务器发送received信号；服务器收到后将向不同服务器发送初始手牌，客户端处理后将向服务器发送initialized信号，服务器调用`handleLandlord`函数，整个游戏将进入叫地主状态。在叫地主函数中，服务器将向轮到叫地主的用户发送指令，并将用户传递过来的选择广播到其他用户。叫地主完成后，发送`finishPlaying`信号，调用`handlePlaying`，轮流向该出牌的用户发送出牌指令，并将出的牌向其他用户广播。游戏结束时，接受用户发过来的信息：若所有用户都选择重新开始，则调用`handleFinish`来开始新游戏；若有一个用户选择退出，则将向所有客户端发送指令，使其退出。

#### 五、规则设计流程

出牌的逻辑主要依靠gamecontroller类中的`legalCardsComb  getCardsType`两个函数来进行。

`getCardsType`根据选中的牌的集合来判断属于什么类型、基数是什么，返回cardType类变量。`legaCardsComb`在获得当前牌的类型后，首先判断当前用户是否具有牌权，如果具有的话只判断选中的牌是否是正规的类型，如果不是错误类型即合法；如果不具有牌权，则将根据牌型、牌的基数等信息判断出的牌是否合法，如果是的话选择的牌合法，返回true，否则返回false。