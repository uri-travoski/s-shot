#pragma once

#include <QGraphicsItem>
#include <QRectF>
#include <QPainter>
#include <QCursor>

class AreaSelectionItem : public QGraphicsItem {
public:
    enum { Type = UserType + 20 };
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
        Left,
        Inside
    };

    explicit AreaSelectionItem(QGraphicsItem* parent = nullptr);

    void setRect(const QRectF& rect);
    QRectF rect() const { return m_rect; }

    void setCanvasRect(const QRectF& cRect);
    QRectF canvasRect() const { return m_canvasRect; }

    void setCropMode(bool cropMode);
    bool isCropMode() const { return m_isCropMode; }

    Handle handleAt(const QPointF& scenePos) const;
    QRectF handleRect(Handle h) const;
    static Qt::CursorShape cursorForHandle(Handle h);

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

private:
    QRectF m_rect;
    QRectF m_canvasRect;
    bool m_isCropMode = false;
};
