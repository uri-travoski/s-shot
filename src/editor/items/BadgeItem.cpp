#include "BadgeItem.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>

BadgeItem::BadgeItem(int number, const QPointF& pos)
    : m_number(number)
{
    setPos(pos);
    m_strokeColor = QColor(255, 30, 30);
    m_fillColor = QColor(255, 30, 30);
}

QRectF BadgeItem::boundingRect() const {
    qreal r = m_radius + 4;
    return QRectF(-r, -r, r * 2, r * 2);
}

QPainterPath BadgeItem::shape() const {
    QPainterPath p;
    p.addEllipse(QPointF(0, 0), m_radius, m_radius);
    return p;
}

void BadgeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Draw circular badge background
    painter->setBrush(m_fillColor);
    painter->setPen(QPen(Qt::white, 2.0));
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);

    // Draw number
    QFont font("Sans", 11, QFont::Bold);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect(), Qt::AlignCenter, QString::number(m_number));

    paintSelectionBorder(painter, boundingRect().adjusted(2, 2, -2, -2));
    painter->restore();
}
