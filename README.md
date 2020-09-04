# 网络斗地主设计文档

#### 一、系统环境

- 系统：macOS 10.15.6
- 版本：Qt 5.15.0 clang 64bit

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

1.`MainWindow`类

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

在界面中，主要实现了两个按钮，分别表示同意和不同意。这两个按钮将根据不同的游戏状态来设置

- 主要接口函数与实现

```c++
		/*画图*/
    void setButtonsVisibility(bool ifNoEnabled = true);//根据当前的游戏状态
    void paintHandCards(QStringList a);
    void paintConstCards(QStringList a);//绘制地主剩余的三张牌
    void paintOnboardCards(QStringList a);
    void displayIdentity();
    void displayBuchu(int id,bool);//显示不出牌的信息;true代表打出了牌，false为没打
    /*相应函数*/
    void yes();
    void no();
```

**（二）服务器**

#### 四、通信协议



#### 五、规则设计流程



