#pragma once

#include "BaseAnnotationItem.h"
#include <QRectF>
#include <QPixmap>

class BlurItem : public BaseAnnotationItem {
public:
    enum { Type = UserType + 7 };
    int type() const override { return Type; }

    BlurItem(const QRectF& rect = QRectF(), const QPixmap& sourcePixmap = QPixmap(), int blurLevel = 5);

    void setRect(const QRectF& r);
    QRectF rect() const { return m_rect; }

    int blurLevel() const { return m_blurLevel; }
    void setBlurLevel(int level);

    void updateEffect(const QPixmap& sourcePixmap);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    void applyBlur(const QPixmap& sourcePixmap);

    QRectF m_rect;
    QPixmap m_blurredPixmap;
    QPixmap m_sourceCache;
    int m_blurLevel = 5;
};
