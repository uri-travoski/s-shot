#include "TextItem.h"
#include <QPainter>
#include <QFontMetricsF>

TextItem::TextItem(const QString& text, const QPointF& pos)
    : m_text(text)
    , m_font("Sans", 14, QFont::Bold)
{
    setPos(pos);
    m_strokeColor = QColor(255, 30, 30);
    calculateBounds();
}

void TextItem::setText(const QString& t) {
    prepareGeometryChange();
    m_text = t;
    calculateBounds();
    update();
}

void TextItem::setFont(const QFont& f) {
    prepareGeometryChange();
    m_font = f;
    calculateBounds();
    update();
}

void TextItem::calculateBounds() {
    QFontMetricsF fm(m_font);
    QRectF br = fm.boundingRect(m_text.isEmpty() ? " " : m_text);
    m_textRect = br.adjusted(-6, -4, 6, 4);
}

QRectF TextItem::boundingRect() const {
    return m_textRect.adjusted(-4, -4, 4, 4);
}

QPainterPath TextItem::shape() const {
    QPainterPath p;
    p.addRect(m_textRect);
    return p;
}

void TextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Optional background if fill is set or slight contrast plate
    if (m_fillColor != Qt::transparent) {
        painter->setBrush(m_fillColor);
        painter->setPen(QPen(m_strokeColor, 1));
        painter->drawRoundedRect(m_textRect, 4, 4);
    }

    painter->setFont(m_font);
    painter->setPen(m_strokeColor);
    painter->drawText(m_textRect, Qt::AlignCenter, m_text);

    paintSelectionBorder(painter, m_textRect);
    painter->restore();
}
