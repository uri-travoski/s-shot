#include "PixmapItem.h"

PixmapItem::PixmapItem(const QPixmap& pix, QGraphicsItem* parent)
    : QGraphicsPixmapItem(pix, parent)
{
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    setCursor(Qt::SizeAllCursor);
    setZValue(2.0);
}

QRectF PixmapItem::boundingRect() const {
    QRectF r = QGraphicsPixmapItem::boundingRect();
    return r.adjusted(-5, -5, 5, 5);
}

void PixmapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    QStyleOptionGraphicsItem opt = *option;
    opt.state &= ~QStyle::State_Selected;
    opt.state &= ~QStyle::State_HasFocus;
    QGraphicsPixmapItem::paint(painter, &opt, widget);

    if (isSelected()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPen p(QColor(48, 229, 0), 1.5, Qt::DashLine);
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        QRectF rect = QGraphicsPixmapItem::boundingRect();
        painter->drawRect(rect.adjusted(-1, -1, 1, 1));

        // Corner handles
        painter->setBrush(QColor(48, 229, 0));
        painter->setPen(Qt::white);
        qreal hs = 6.0;
        painter->drawRect(QRectF(rect.left() - hs/2, rect.top() - hs/2, hs, hs));
        painter->drawRect(QRectF(rect.right() - hs/2, rect.top() - hs/2, hs, hs));
        painter->drawRect(QRectF(rect.left() - hs/2, rect.bottom() - hs/2, hs, hs));
        painter->drawRect(QRectF(rect.right() - hs/2, rect.bottom() - hs/2, hs, hs));
        painter->restore();
    }
}
