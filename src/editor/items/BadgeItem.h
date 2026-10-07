#pragma once

#include "BaseAnnotationItem.h"
#include <QPointF>

class BadgeItem : public BaseAnnotationItem {
public:
    enum { Type = UserType + 5 };
    int type() const override { return Type; }

    BadgeItem(int number = 1, const QPointF& pos = QPointF());

    void setNumber(int n) { m_number = n; update(); }
    int number() const { return m_number; }

    void setRadius(qreal r) { prepareGeometryChange(); m_radius = r; update(); }
    qreal radius() const { return m_radius; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    void contextMenuEvent(class QGraphicsSceneContextMenuEvent* event) override;
    void mouseDoubleClickEvent(class QGraphicsSceneMouseEvent* event) override;

private:
    int m_number = 1;
    qreal m_radius = 16.0;
};
