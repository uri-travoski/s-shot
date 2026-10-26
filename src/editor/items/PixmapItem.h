#pragma once

#include <QGraphicsPixmapItem>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QCursor>

class TestEditorTools;

class PixmapItem : public QGraphicsPixmapItem {
    friend class TestEditorTools;
public:
    enum { Type = UserType + 10 };
    int type() const override { return Type; }

    enum class Handle {
        None,
        TopLeft,
        Top,
        TopRight,
        Right,
        BottomRight,
        Bottom,
        BottomLeft,
        Left
    };

    explicit PixmapItem(const QPixmap& pix, QGraphicsItem* parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

    Handle handleAt(const QPointF& itemPos) const;
    QRectF handleRect(Handle h) const;
    static Qt::CursorShape cursorForHandle(Handle h);

    const QPixmap& originalPixmap() const { return m_originalPixmap; }
    void setOriginalPixmap(const QPixmap& pix);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    QVariant itemChange(GraphicsItemChange change, const QVariant& value) override;

private:
    QPixmap m_originalPixmap;
    Handle m_activeHandle = Handle::None;
    Handle m_hoveredHandle = Handle::None;
    QPointF m_pressScenePos;
    QRectF m_pressRect;
    QPointF m_pressPos;
    qreal m_aspectRatio = 1.0;
};
