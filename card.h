#ifndef CARD_H
#define CARD_H

#include "constant.h"
#include <QGraphicsItem>

class card : public QGraphicsItem
{
public:
    card();
    card(QString a,int id,bool _canClick = false);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
private:
    friend class MainWindow;

    QString name;
    int id;
    bool selected = false;
    bool canClick;
};

#endif // CARD_H
