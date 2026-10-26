#include "AreaSelectionItem.h"
#include <QPainterPath>

AreaSelectionItem::AreaSelectionItem(QGraphicsItem* parent)
    : QGraphicsItem(parent)
{
    setZValue(10000);
    setAcceptedMouseButtons(Qt::NoButton);
}

void AreaSelectionItem::setRect(const QRectF& rect) {
    prepareGeometryChange();
    m_rect = rect.normalized();
    update();
}

void AreaSelectionItem::setCanvasRect(const QRectF& cRect) {
    prepareGeometryChange();
    m_canvasRect = cRect;
    update();
}

void AreaSelectionItem::setCropMode(bool cropMode) {
    if (m_isCropMode != cropMode) {
        prepareGeometryChange();
        m_isCropMode = cropMode;
        update();
    }
}

QRectF AreaSelectionItem::handleRect(Handle h) const {
    if (m_rect.width() < 2 || m_rect.height() < 2) return QRectF();

    qreal x = m_rect.left();
    qreal y = m_rect.top();
    qreal w = m_rect.width();
    qreal hgt = m_rect.height();

    qreal cornerSize = 10.0;
    qreal edgeLength = 16.0;
    qreal edgeThick = 6.0;

    switch (h) {
    case Handle::TopLeft:
        return QRectF(x - cornerSize / 2.0, y - cornerSize / 2.0, cornerSize, cornerSize);
    case Handle::TopRight:
        return QRectF(x + w - cornerSize / 2.0, y - cornerSize / 2.0, cornerSize, cornerSize);
    case Handle::BottomRight:
        return QRectF(x + w - cornerSize / 2.0, y + hgt - cornerSize / 2.0, cornerSize, cornerSize);
    case Handle::BottomLeft:
        return QRectF(x - cornerSize / 2.0, y + hgt - cornerSize / 2.0, cornerSize, cornerSize);
    case Handle::Top:
        return QRectF(x + w / 2.0 - edgeLength / 2.0, y - edgeThick / 2.0, edgeLength, edgeThick);
    case Handle::Bottom:
        return QRectF(x + w / 2.0 - edgeLength / 2.0, y + hgt - edgeThick / 2.0, edgeLength, edgeThick);
    case Handle::Left:
        return QRectF(x - edgeThick / 2.0, y + hgt / 2.0 - edgeLength / 2.0, edgeThick, edgeLength);
    case Handle::Right:
        return QRectF(x + w - edgeThick / 2.0, y + hgt / 2.0 - edgeLength / 2.0, edgeThick, edgeLength);
    default:
        return QRectF();
    }
}

AreaSelectionItem::Handle AreaSelectionItem::handleAt(const QPointF& scenePos) const {
    if (m_rect.width() < 2 || m_rect.height() < 2) return Handle::None;

    const Handle handles[] = {
        Handle::TopLeft, Handle::TopRight, Handle::BottomRight, Handle::BottomLeft,
        Handle::Top, Handle::Bottom, Handle::Left, Handle::Right
    };

    const qreal hitMargin = 5.0;
    for (Handle h : handles) {
        QRectF hr = handleRect(h).adjusted(-hitMargin, -hitMargin, hitMargin, hitMargin);
        if (hr.contains(scenePos)) {
            return h;
        }
    }

    if (m_rect.contains(scenePos)) {
        return Handle::Inside;
    }

    return Handle::None;
}

Qt::CursorShape AreaSelectionItem::cursorForHandle(Handle h) {
    switch (h) {
    case Handle::Top:
    case Handle::Bottom:
        return Qt::SizeVerCursor;
    case Handle::Left:
    case Handle::Right:
        return Qt::SizeHorCursor;
    case Handle::TopLeft:
    case Handle::BottomRight:
        return Qt::SizeFDiagCursor;
    case Handle::TopRight:
    case Handle::BottomLeft:
        return Qt::SizeBDiagCursor;
    case Handle::Inside:
        return Qt::SizeAllCursor;
    default:
        return Qt::CrossCursor;
    }
}

QRectF AreaSelectionItem::boundingRect() const {
    QRectF b = m_rect;
    if (m_isCropMode && !m_canvasRect.isNull()) {
        b = b.united(m_canvasRect);
    }
    if (b.isNull()) return QRectF();
    return b.adjusted(-16, -16, 16, 16);
}

void AreaSelectionItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    if (m_rect.width() < 2 || m_rect.height() < 2) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // 1. Photoshop-style darkened outer scrim
    if (m_isCropMode && !m_canvasRect.isEmpty()) {
        QPainterPath fullPath;
        fullPath.addRect(m_canvasRect);
        QPainterPath holePath;
        holePath.addRect(m_rect);
        QPainterPath darkMask = fullPath.subtracted(holePath);

        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(0, 0, 0, 140));
        painter->drawPath(darkMask);

        // Rule-of-thirds grid
        if (m_rect.width() > 30 && m_rect.height() > 30) {
            painter->setPen(QPen(QColor(255, 255, 255, 75), 1.0, Qt::DashLine));
            qreal oneThirdW = m_rect.width() / 3.0;
            qreal oneThirdH = m_rect.height() / 3.0;
            // Vertical grid lines
            painter->drawLine(QPointF(m_rect.left() + oneThirdW, m_rect.top()),
                             QPointF(m_rect.left() + oneThirdW, m_rect.bottom()));
            painter->drawLine(QPointF(m_rect.left() + 2 * oneThirdW, m_rect.top()),
                             QPointF(m_rect.left() + 2 * oneThirdW, m_rect.bottom()));
            // Horizontal grid lines
            painter->drawLine(QPointF(m_rect.left(), m_rect.top() + oneThirdH),
                             QPointF(m_rect.right(), m_rect.top() + oneThirdH));
            painter->drawLine(QPointF(m_rect.left(), m_rect.top() + 2 * oneThirdH),
                             QPointF(m_rect.right(), m_rect.top() + 2 * oneThirdH));
        }
    } else {
        // Selection fill tint
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(48, 229, 0, 25));
        painter->drawRect(m_rect);
    }

    // 2. Selection border
    painter->setPen(QPen(QColor(0, 0, 0, 180), 1.0));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(m_rect);

    QPen dashPen(QColor(48, 229, 0), 1.5, Qt::DashLine);
    painter->setPen(dashPen);
    painter->drawRect(m_rect);

    // 3. 8 Interactive Control Handles
    const Handle allHandles[] = {
        Handle::TopLeft, Handle::Top, Handle::TopRight,
        Handle::Right, Handle::BottomRight, Handle::Bottom,
        Handle::BottomLeft, Handle::Left
    };

    for (Handle h : allHandles) {
        QRectF hr = handleRect(h);
        // Outer dark shadow
        painter->setPen(QPen(QColor(0, 0, 0, 160), 1.0));
        painter->setBrush(Qt::white);
        painter->drawRect(hr);

        // Inner green core
        painter->setPen(Qt::NoPen);
        painter->setBrush(QColor(48, 229, 0));
        painter->drawRect(hr.adjusted(1.5, 1.5, -1.5, -1.5));
    }

    painter->restore();
}
