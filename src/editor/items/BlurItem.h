#pragma once

#include "BaseAnnotationItem.h"
#include <QRectF>
#include <QPixmap>

class BlurItem : public BaseAnnotationItem {
public:
    enum { Type = UserType + 7 };
    int type() const override { return Type; }

    BlurItem(const QRectF& rect = QRectF(), const QPixmap& sourcePixmap = QPixmap());

    void setRect(const QRectF& r);
    QRectF rect() const { return m_rect; }

    void updateEffect(const QPixmap& sourcePixmap);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void generatePixelatedPixmap(const QPixmap& sourcePixmap);

    QRectF m_rect;
    QPixmap m_pixelatedPixmap;
    int m_blockSize = 12;
};
