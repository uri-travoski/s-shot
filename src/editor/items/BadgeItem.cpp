#include "BadgeItem.h"
#include <QPainter>
#include <QPainterPath>
#include <QFont>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <QInputDialog>
#include <QColorDialog>
#include <QAction>
#include <QTimer>
#include "../CanvasScene.h"

BadgeItem::BadgeItem(int number, const QPointF& pos)
    : m_number(number)
{
    setPos(pos);
    m_strokeColor = QColor(255, 30, 30);
    m_fillColor = QColor(255, 30, 30);
}

QRectF BadgeItem::boundingRect() const {
    qreal r = m_radius + 4;
    return QRectF(-r, -r, r * 2, r * 2);
}

QPainterPath BadgeItem::shape() const {
    QPainterPath p;
    p.addEllipse(QPointF(0, 0), m_radius, m_radius);
    return p;
}

void BadgeItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    // Draw circular badge background
    painter->setBrush(m_fillColor);
    painter->setPen(QPen(Qt::white, 2.0));
    painter->drawEllipse(QPointF(0, 0), m_radius, m_radius);

    // Draw number with dynamic size for multi-digit numbers
    int fontSize = (m_number >= 100) ? 9 : ((m_number >= 10) ? 10 : 11);
    QFont font("Sans", fontSize, QFont::Bold);
    painter->setFont(font);
    painter->setPen(Qt::white);
    painter->drawText(boundingRect(), Qt::AlignCenter, QString::number(m_number));

    paintSelectionBorder(painter, boundingRect().adjusted(2, 2, -2, -2));
    painter->restore();
}

void BadgeItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* actReset = menu.addAction(QObject::tr("Reset Number to 1"));
    QAction* actChange = menu.addAction(QObject::tr("Set Number..."));
    menu.addSeparator();
    QAction* actResetStepper = menu.addAction(QObject::tr("Reset Next Stepper Counter to 1"));
    menu.addSeparator();
    QAction* actColor = menu.addAction(QObject::tr("Change Badge Color..."));
    menu.addSeparator();
    QAction* actDelete = menu.addAction(QObject::tr("Delete"));

    QAction* chosen = menu.exec(event->screenPos());
    if (chosen == actReset) {
        auto* cs = dynamic_cast<CanvasScene*>(scene());
        if (cs) {
            cs->modifyBadgeNumber(this, 1);
        } else {
            setNumber(1);
        }
    } else if (chosen == actChange) {
        bool ok = false;
        int n = QInputDialog::getInt(nullptr, QObject::tr("Set Badge Number"), QObject::tr("Badge Number:"), m_number, 1, 9999, 1, &ok);
        if (ok) {
            auto* cs = dynamic_cast<CanvasScene*>(scene());
            if (cs) {
                cs->modifyBadgeNumber(this, n);
            } else {
                setNumber(n);
            }
        }
    } else if (chosen == actResetStepper) {
        if (auto* cs = dynamic_cast<CanvasScene*>(scene())) {
            cs->resetBadgeCounter();
        }
    } else if (chosen == actColor) {
        QColor c = QColorDialog::getColor(m_fillColor, nullptr, QObject::tr("Choose Badge Color"), QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            setStrokeColor(c);
            setFillColor(c);
            if (auto* cs = dynamic_cast<CanvasScene*>(scene())) {
                emit cs->sceneModified();
            }
        }
    } else if (chosen == actDelete) {
        if (scene()) {
            auto* cs = dynamic_cast<CanvasScene*>(scene());
            scene()->removeItem(this);
            if (cs) {
                emit cs->sceneModified();
            }
            setVisible(false);
            QTimer::singleShot(0, [this]() { delete this; });
        }
    }
    event->accept();
}

void BadgeItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    bool ok = false;
    int n = QInputDialog::getInt(nullptr, QObject::tr("Edit Badge Number"), QObject::tr("Badge Number:"), m_number, 1, 9999, 1, &ok);
    if (ok) {
        auto* cs = dynamic_cast<CanvasScene*>(scene());
        if (cs) {
            cs->modifyBadgeNumber(this, n);
        } else {
            setNumber(n);
        }
    }
    event->accept();
}
