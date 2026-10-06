#include "ShapeItem.h"
#include <QPainter>
#include <QPainterPath>

ShapeItem::ShapeItem(bool isEllipse)
    : m_isEllipse(isEllipse)
{
}

void ShapeItem::setRect(const QRectF& r) {
    prepareGeometryChange();
    m_rect = r;
    update();
}

QRectF ShapeItem::boundingRect() const {
    qreal pad = (m_strokeWidth / 2.0) + 4;
    return m_rect.normalized().adjusted(-pad, -pad, pad, pad);
}

QPainterPath ShapeItem::shape() const {
    QPainterPath p;
    QRectF r = m_rect.normalized();
    if (m_isEllipse) {
        p.addEllipse(r);
    } else {
        p.addRect(r);
    }
    return p;
}

void ShapeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(m_strokeColor, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(m_fillColor);

    QRectF r = m_rect.normalized();
    if (m_isEllipse) {
        painter->drawEllipse(r);
    } else {
        painter->drawRect(r);
    }

    paintSelectionBorder(painter, r);
    painter->restore();
}
