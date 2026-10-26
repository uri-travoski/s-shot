#include "PixmapItem.h"
#include <QGraphicsScene>

PixmapItem::PixmapItem(const QPixmap& pix, QGraphicsItem* parent)
    : QGraphicsPixmapItem(pix, parent)
    , m_originalPixmap(pix)
{
    m_originalPixmap.setDevicePixelRatio(1.0);
    if (pix.width() > 0 && pix.height() > 0) {
        m_aspectRatio = static_cast<qreal>(pix.width()) / pix.height();
    }
    setFlags(ItemIsSelectable | ItemIsMovable | ItemSendsGeometryChanges);
    setAcceptHoverEvents(true);
    setCursor(Qt::SizeAllCursor);
    setZValue(2.0);
}

void PixmapItem::setOriginalPixmap(const QPixmap& pix) {
    m_originalPixmap = pix;
    m_originalPixmap.setDevicePixelRatio(1.0);
    if (pix.width() > 0 && pix.height() > 0) {
        m_aspectRatio = static_cast<qreal>(pix.width()) / pix.height();
    }
    setPixmap(pix);
}

QRectF PixmapItem::boundingRect() const {
    QRectF r = QGraphicsPixmapItem::boundingRect();
    return r.adjusted(-8, -8, 8, 8);
}

QRectF PixmapItem::handleRect(Handle h) const {
    qreal w = pixmap().width();
    qreal hgt = pixmap().height();
    if (w < 2 || hgt < 2) return QRectF();

    qreal hs = 8.0;

    switch (h) {
    case Handle::TopLeft:     return QRectF(-hs / 2.0, -hs / 2.0, hs, hs);
    case Handle::Top:         return QRectF(w / 2.0 - hs / 2.0, -hs / 2.0, hs, hs);
    case Handle::TopRight:    return QRectF(w - hs / 2.0, -hs / 2.0, hs, hs);
    case Handle::Right:       return QRectF(w - hs / 2.0, hgt / 2.0 - hs / 2.0, hs, hs);
    case Handle::BottomRight: return QRectF(w - hs / 2.0, hgt - hs / 2.0, hs, hs);
    case Handle::Bottom:      return QRectF(w / 2.0 - hs / 2.0, hgt - hs / 2.0, hs, hs);
    case Handle::BottomLeft:  return QRectF(-hs / 2.0, hgt - hs / 2.0, hs, hs);
    case Handle::Left:        return QRectF(-hs / 2.0, hgt / 2.0 - hs / 2.0, hs, hs);
    default:                  return QRectF();
    }
}

PixmapItem::Handle PixmapItem::handleAt(const QPointF& itemPos) const {
    if (pixmap().width() < 2 || pixmap().height() < 2) return Handle::None;

    const Handle handles[] = {
        Handle::TopLeft, Handle::TopRight, Handle::BottomRight, Handle::BottomLeft,
        Handle::Top, Handle::Bottom, Handle::Left, Handle::Right
    };

    const qreal hit = 12.0;
    for (Handle h : handles) {
        QRectF r = handleRect(h);
        QRectF hr(r.center().x() - hit / 2.0, r.center().y() - hit / 2.0, hit, hit);
        if (hr.contains(itemPos)) {
            return h;
        }
    }

    return Handle::None;
}

Qt::CursorShape PixmapItem::cursorForHandle(Handle h) {
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
    default:
        return Qt::SizeAllCursor;
    }
}

void PixmapItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (isSelected()) {
        Handle h = handleAt(event->pos());
        m_hoveredHandle = h;
        setCursor(cursorForHandle(h));
    } else {
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsPixmapItem::hoverMoveEvent(event);
}

void PixmapItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    m_hoveredHandle = Handle::None;
    setCursor(Qt::ArrowCursor);
    QGraphicsPixmapItem::hoverLeaveEvent(event);
}

void PixmapItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton && isSelected()) {
        Handle h = handleAt(event->pos());
        if (h != Handle::None) {
            m_activeHandle = h;
            m_pressScenePos = event->scenePos();
            m_pressRect = QRectF(0, 0, pixmap().width(), pixmap().height());
            m_pressPos = pos();
            event->accept();
            return;
        }
    }
    QGraphicsPixmapItem::mousePressEvent(event);
}

void PixmapItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle != Handle::None) {
        QPointF delta = event->scenePos() - m_pressScenePos;
        QRectF newRect = m_pressRect;
        QPointF newPos = m_pressPos;

        qreal minW = 10.0;
        qreal minH = 10.0;

        switch (m_activeHandle) {
        case Handle::Right:
            newRect.setWidth(qMax(minW, m_pressRect.width() + delta.x()));
            break;
        case Handle::Bottom:
            newRect.setHeight(qMax(minH, m_pressRect.height() + delta.y()));
            break;
        case Handle::BottomRight:
            newRect.setWidth(qMax(minW, m_pressRect.width() + delta.x()));
            newRect.setHeight(qMax(minH, m_pressRect.height() + delta.y()));
            break;
        case Handle::Left: {
            qreal diff = qMin(delta.x(), m_pressRect.width() - minW);
            newPos.setX(m_pressPos.x() + diff);
            newRect.setWidth(m_pressRect.width() - diff);
            break;
        }
        case Handle::Top: {
            qreal diff = qMin(delta.y(), m_pressRect.height() - minH);
            newPos.setY(m_pressPos.y() + diff);
            newRect.setHeight(m_pressRect.height() - diff);
            break;
        }
        case Handle::TopLeft: {
            qreal diffX = qMin(delta.x(), m_pressRect.width() - minW);
            qreal diffY = qMin(delta.y(), m_pressRect.height() - minH);
            newPos.setX(m_pressPos.x() + diffX);
            newPos.setY(m_pressPos.y() + diffY);
            newRect.setWidth(m_pressRect.width() - diffX);
            newRect.setHeight(m_pressRect.height() - diffY);
            break;
        }
        case Handle::TopRight: {
            qreal diffY = qMin(delta.y(), m_pressRect.height() - minH);
            newPos.setY(m_pressPos.y() + diffY);
            newRect.setHeight(m_pressRect.height() - diffY);
            newRect.setWidth(qMax(minW, m_pressRect.width() + delta.x()));
            break;
        }
        case Handle::BottomLeft: {
            qreal diffX = qMin(delta.x(), m_pressRect.width() - minW);
            newPos.setX(m_pressPos.x() + diffX);
            newRect.setWidth(m_pressRect.width() - diffX);
            newRect.setHeight(qMax(minH, m_pressRect.height() + delta.y()));
            break;
        }
        default:
            break;
        }

        if (event->modifiers() & Qt::ShiftModifier && m_aspectRatio > 0.0) {
            if (m_activeHandle == Handle::Right || m_activeHandle == Handle::Left) {
                newRect.setHeight(newRect.width() / m_aspectRatio);
            } else {
                newRect.setWidth(newRect.height() * m_aspectRatio);
            }
        }

        int targetW = qMax(10, qRound(newRect.width()));
        int targetH = qMax(10, qRound(newRect.height()));

        setPos(newPos);
        setPixmap(m_originalPixmap.scaled(targetW, targetH, Qt::IgnoreAspectRatio, Qt::FastTransformation));
        event->accept();
        return;
    }
    QGraphicsPixmapItem::mouseMoveEvent(event);
}

void PixmapItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_activeHandle != Handle::None) {
        m_activeHandle = Handle::None;
        int finalW = qMax(10, pixmap().width());
        int finalH = qMax(10, pixmap().height());
        setPixmap(m_originalPixmap.scaled(finalW, finalH, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        update();
        if (scene()) {
            scene()->update();
        }
        event->accept();
        return;
    }
    QGraphicsPixmapItem::mouseReleaseEvent(event);
}

QVariant PixmapItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionChange && scene()) {
        QPointF newPos = value.toPointF();
        QRectF sr = scene()->sceneRect();
        qreal w = pixmap().width();
        qreal h = pixmap().height();
        qreal minX = sr.left();
        qreal maxX = qMax(minX, sr.right() - w);
        qreal minY = sr.top();
        qreal maxY = qMax(minY, sr.bottom() - h);
        newPos.setX(qBound(minX, newPos.x(), maxX));
        newPos.setY(qBound(minY, newPos.y(), maxY));
        return newPos;
    }
    return QGraphicsPixmapItem::itemChange(change, value);
}

void PixmapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    QStyleOptionGraphicsItem opt = *option;
    opt.state &= ~QStyle::State_Selected;
    opt.state &= ~QStyle::State_HasFocus;
    QGraphicsPixmapItem::paint(painter, &opt, widget);

    if (isSelected()) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        QPen p(QColor(48, 229, 0), 1.5, Qt::DashLine);
        painter->setPen(p);
        painter->setBrush(Qt::NoBrush);
        QRectF rect = QGraphicsPixmapItem::boundingRect();
        painter->drawRect(rect.adjusted(-1, -1, 1, 1));

        // 8 Interactive resize handles
        const Handle allHandles[] = {
            Handle::TopLeft, Handle::Top, Handle::TopRight,
            Handle::Right, Handle::BottomRight, Handle::Bottom,
            Handle::BottomLeft, Handle::Left
        };
        for (Handle h : allHandles) {
            QRectF hr = handleRect(h);
            painter->setBrush(Qt::white);
            painter->setPen(QPen(QColor(48, 229, 0), 1.5));
            painter->drawRect(hr);
        }
        painter->restore();
    }
}
