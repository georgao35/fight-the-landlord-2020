#include "card.h"
#include <QString>
#include <QPainter>

card::card()
{

}

card::card(QString a,int id,bool _canClick):name(a),id(id),canClick(_canClick)
{
    setPos(id*INTERVAL_WIDTH,0);
}

QRectF card::boundingRect() const{
    return QRectF(0,0,CARD_WIDTH,CARD_HEIGHT);
}

QPainterPath card::shape() const{
    QPainterPath path;
    path.addRect(0,0,CARD_WIDTH,CARD_HEIGHT);
    return path;
}

void card::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget){
    QPixmap pic;
    pic.load(QString(":/cards/cards/")+name+QString(".png"));
    painter->save();

    QRectF source (0,0,220.0,300.0);
    //painter->drawPath(shape());
    painter->drawPixmap(boundingRect(),pic,source);

    painter->restore();
}

void card::mousePressEvent(QGraphicsSceneMouseEvent *event){
    if(canClick){
        if(selected)
            selected = false;
        else
            selected = true;
        setPos(id*INTERVAL_WIDTH,-10*selected);
    }
}
