#include "BaseAnnotationItem.h"
#include <QPainter>
#include <QGraphicsScene>

BaseAnnotationItem::BaseAnnotationItem() {
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
}

QVariant BaseAnnotationItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        QRectF sr = scene()->sceneRect();
        QRectF br = boundingRect();
        qreal minX = sr.left() - br.left();
        qreal maxX = sr.right() - br.right();
        qreal minY = sr.top() - br.top();
        qreal maxY = sr.bottom() - br.bottom();
        if (maxX >= minX) {
            newPos.setX(qBound(minX, newPos.x(), maxX));
        }
        if (maxY >= minY) {
            newPos.setY(qBound(minY, newPos.y(), maxY));
        }
        return newPos;
    }
    return QGraphicsItem::itemChange(change, value);
}

void BaseAnnotationItem::paintSelectionBorder(QPainter* painter, const QRectF& rect) {
    if (isSelected()) {
        painter->save();
        QPen p(QColor(48, 229, 0), 1.5, Qt::DashLine);
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(rect.adjusted(-2, -2, 2, 2));

        // Corner handles
        painter->setBrush(QColor(48, 229, 0));
        painter->setPen(Qt::white);
        qreal hs = 5.0;
        painter->drawRect(QRectF(rect.left() - hs/2, rect.top() - hs/2, hs, hs));
        painter->drawRect(QRectF(rect.right() - hs/2, rect.top() - hs/2, hs, hs));
        painter->drawRect(QRectF(rect.left() - hs/2, rect.bottom() - hs/2, hs, hs));
        painter->drawRect(QRectF(rect.right() - hs/2, rect.bottom() - hs/2, hs, hs));
        painter->restore();
    }
}
