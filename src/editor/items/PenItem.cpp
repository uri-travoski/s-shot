#include "PenItem.h"
#include <QPainter>

PenItem::PenItem(bool isHighlighter)
    : m_isHighlighter(isHighlighter)
{
    if (m_isHighlighter) {
        m_strokeColor = QColor(255, 235, 59, 130); // Yellow translucent
        m_strokeWidth = 14;
    }
}

void PenItem::addPoint(const QPointF& pt) {
    prepareGeometryChange();
    if (m_path.isEmpty()) {
        m_path.moveTo(pt);
    } else {
        m_path.lineTo(pt);
    }
    update();
}

QRectF PenItem::boundingRect() const {
    qreal pad = (m_strokeWidth / 2.0) + 4;
    return m_path.boundingRect().adjusted(-pad, -pad, pad, pad);
}

QPainterPath PenItem::shape() const {
    QPainterPathStroker stroker;
    stroker.setWidth(m_strokeWidth + 4);
    return stroker.createStroke(m_path);
}

void PenItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(m_strokeColor, m_strokeWidth, Qt::SolidLine,
             m_isHighlighter ? Qt::FlatCap : Qt::RoundCap,
             m_isHighlighter ? Qt::MiterJoin : Qt::RoundJoin);

    if (m_isHighlighter) {
        painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawPath(m_path);

    paintSelectionBorder(painter, m_path.boundingRect());
    painter->restore();
}
