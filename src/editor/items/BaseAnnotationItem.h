#pragma once

#include <QGraphicsItem>
#include <QColor>
#include <QPen>
#include <QBrush>

class BaseAnnotationItem : public QGraphicsItem {
public:
    enum { Type = UserType + 1 };
    int type() const override { return Type; }

    BaseAnnotationItem();
    virtual ~BaseAnnotationItem() = default;

    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor& c) { m_strokeColor = c; update(); }

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor& c) { m_fillColor = c; update(); }

    int strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(int w) { m_strokeWidth = w; update(); }

protected:
    void paintSelectionBorder(QPainter* painter, const QRectF& rect);

    QColor m_strokeColor = QColor(255, 30, 30);
    QColor m_fillColor = Qt::transparent;
    int m_strokeWidth = 3;
};
