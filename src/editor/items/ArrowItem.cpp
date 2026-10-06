#include "ArrowItem.h"
#include <QPainter>
#include <QPainterPath>
#include <cmath>

ArrowItem::ArrowItem(ArrowMode mode)
    : m_mode(mode)
{
}

void ArrowItem::setEndpoints(const QPointF& start, const QPointF& end) {
    prepareGeometryChange();
    m_start = start;
    m_end = end;
    update();
}

QRectF ArrowItem::boundingRect() const {
    qreal pad = m_strokeWidth * 3.0 + 10;
    return QRectF(m_start, m_end).normalized().adjusted(-pad, -pad, pad, pad);
}

QPainterPath ArrowItem::shape() const {
    QPainterPath p;
    p.moveTo(m_start);
    p.lineTo(m_end);
    QPainterPathStroker stroker;
    stroker.setWidth(m_strokeWidth + 10);
    return stroker.createStroke(p);
}

void ArrowItem::drawArrowHead(QPainter* painter, const QPointF& from, const QPointF& to) {
    qreal dx = to.x() - from.x();
    qreal dy = to.y() - from.y();
    qreal angle = std::atan2(dy, dx);

    qreal headLength = qMax(12.0, m_strokeWidth * 3.5);
    qreal headAngle = 0.45; // ~26 degrees

    QPointF p1 = to - QPointF(headLength * std::cos(angle - headAngle), headLength * std::sin(angle - headAngle));
    QPointF p2 = to - QPointF(headLength * std::cos(angle + headAngle), headLength * std::sin(angle + headAngle));

    QPainterPath arrowPath;
    arrowPath.moveTo(to);
    arrowPath.lineTo(p1);
    arrowPath.lineTo(p2);
    arrowPath.closeSubpath();

    painter->setBrush(m_strokeColor);
    painter->setPen(Qt::NoPen);
    painter->drawPath(arrowPath);
}

void ArrowItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    QPen pen(m_strokeColor, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);
    painter->drawLine(m_start, m_end);

    if (m_mode == ArrowMode::SingleArrow || m_mode == ArrowMode::DoubleArrow) {
        drawArrowHead(painter, m_start, m_end);
    }
    if (m_mode == ArrowMode::DoubleArrow) {
        drawArrowHead(painter, m_end, m_start);
    }

    paintSelectionBorder(painter, QRectF(m_start, m_end).normalized());
    painter->restore();
}
