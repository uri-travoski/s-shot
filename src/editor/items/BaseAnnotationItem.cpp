#include "BaseAnnotationItem.h"
#include <QPainter>

BaseAnnotationItem::BaseAnnotationItem() {
    setFlags(ItemIsSelectable | ItemIsMovable);
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
