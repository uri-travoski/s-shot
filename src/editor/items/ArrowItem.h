#pragma once

#include "BaseAnnotationItem.h"
#include <QPointF>

enum class ArrowMode {
    LineOnly,
    SingleArrow,
    DoubleArrow
};

class ArrowItem : public BaseAnnotationItem {
public:
    enum { Type = UserType + 3 };
    int type() const override { return Type; }

    ArrowItem(ArrowMode mode = ArrowMode::SingleArrow);

    void setEndpoints(const QPointF& start, const QPointF& end);
    QPointF startPoint() const { return m_start; }
    QPointF endPoint() const { return m_end; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void drawArrowHead(QPainter* painter, const QPointF& from, const QPointF& to);

    QPointF m_start;
    QPointF m_end;
    ArrowMode m_mode = ArrowMode::SingleArrow;
};
