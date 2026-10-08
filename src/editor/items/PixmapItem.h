#pragma once

#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>

class PixmapItem : public QGraphicsPixmapItem {
public:
    enum { Type = UserType + 10 };
    int type() const override { return Type; }

    explicit PixmapItem(const QPixmap& pix, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;
};
