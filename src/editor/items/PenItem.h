#pragma once

#include "BaseAnnotationItem.h"
#include <QPainterPath>

class PenItem : public BaseAnnotationItem {
public:
    enum { Type = UserType + 2 };
    int type() const override { return Type; }

    PenItem(bool isHighlighter = false);

    void addPoint(const QPointF& pt);
    bool isHighlighter() const { return m_isHighlighter; }
    QPainterPath path() const { return m_path; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    QPainterPath m_path;
    bool m_isHighlighter = false;
};
