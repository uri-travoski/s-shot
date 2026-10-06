#pragma once

#include "BaseAnnotationItem.h"
#include <QRectF>

class ShapeItem : public BaseAnnotationItem {
public:
    enum { Type = UserType + 4 };
    int type() const override { return Type; }

    ShapeItem(bool isEllipse = false);

    void setRect(const QRectF& r);
    QRectF rect() const { return m_rect; }
    bool isEllipse() const { return m_isEllipse; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QRectF m_rect;
    bool m_isEllipse = false;
};
