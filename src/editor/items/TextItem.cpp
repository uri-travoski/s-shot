#include "TextItem.h"
#include <QGraphicsScene>
#include <QTextCursor>

TextItem::TextItem(const QString& text, const QPointF& pos)
    : QGraphicsTextItem()
    , m_strokeColor(QColor(255, 30, 30))
    , m_fillColor(Qt::transparent)
{
    setPos(pos);
    setPlainText(text);
    setFont(QFont("Sans", 14, QFont::Bold));
    setDefaultTextColor(m_strokeColor);
    setFlag(ItemIsMovable, true);
    setFlag(ItemIsSelectable, true);
    setTextInteractionFlags(Qt::NoTextInteraction);
}

void TextItem::setText(const QString& t) {
    setPlainText(t);
    update();
}

void TextItem::setStrokeColor(const QColor& c) {
    m_strokeColor = c;
    setDefaultTextColor(c);
    update();
}

void TextItem::setFillColor(const QColor& c) {
    m_fillColor = c;
    update();
}

void TextItem::startEditing() {
    m_isEditing = true;
    setFlag(ItemIsMovable, false);
    setTextInteractionFlags(Qt::TextEditorInteraction);
    setFocus();
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
    update();
}

void TextItem::finishEditing() {
    if (!m_isEditing && !m_isInitialCreation) return;
    m_isEditing = false;
    setTextInteractionFlags(Qt::NoTextInteraction);
    setFlag(ItemIsMovable, true);
    clearFocus();
    update();

    if (m_isInitialCreation) {
        m_isInitialCreation = false;
        emit initialCreationFinished(!toPlainText().trimmed().isEmpty());
    }
}

QRectF TextItem::boundingRect() const {
    QRectF r = QGraphicsTextItem::boundingRect();
    return r.adjusted(-6, -4, 6, 4);
}

void TextItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
    QRectF r = boundingRect().adjusted(2, 2, -2, -2);

    // 1. Draw textbox background
    if (m_fillColor.isValid() && m_fillColor != Qt::transparent && m_fillColor.alpha() > 0) {
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        painter->setBrush(m_fillColor);
        painter->setPen(QPen(m_strokeColor, 1));
        painter->drawRoundedRect(r, 4, 4);
        painter->restore();
    } else if (m_isEditing) {
        // While actively typing in a transparent textbox, show a subtle dashed guide
        painter->save();
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(48, 229, 0, 200), 1, Qt::DashLine));
        painter->drawRoundedRect(r, 4, 4);
        painter->restore();
    }

    // 2. Draw text content and cursor
    QGraphicsTextItem::paint(painter, option, widget);

    // 3. Selection border in Select tool
    if (isSelected() && !m_isEditing) {
        painter->save();
        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(QColor(48, 229, 0), 1.5, Qt::DashLine));
        painter->drawRect(r);
        painter->restore();
    }
}

void TextItem::focusInEvent(QFocusEvent* event) {
    m_isEditing = true;
    QGraphicsTextItem::focusInEvent(event);
}

void TextItem::focusOutEvent(QFocusEvent* event) {
    finishEditing();
    QGraphicsTextItem::focusOutEvent(event);
}

void TextItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        startEditing();
        event->accept();
        return;
    }
    QGraphicsTextItem::mouseDoubleClickEvent(event);
}

void TextItem::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        finishEditing();
        event->accept();
        return;
    }
    QGraphicsTextItem::keyPressEvent(event);
}

void TextItem::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    QMenu menu;
    QAction* actEdit = menu.addAction(tr("Edit Text..."));
    menu.addSeparator();
    QAction* actBg = menu.addAction(tr("Choose Textbox Background Color..."));
    QAction* actTrans = menu.addAction(tr("Set Transparent Background"));
    menu.addSeparator();
    QAction* actTextColor = menu.addAction(tr("Choose Text Color..."));
    QAction* actFont = menu.addAction(tr("Choose Font..."));
    menu.addSeparator();
    QAction* actDelete = menu.addAction(tr("Delete"));

    QAction* chosen = menu.exec(event->screenPos());
    if (chosen == actEdit) {
        startEditing();
    } else if (chosen == actBg) {
        QColor initColor = (m_fillColor.isValid() && m_fillColor != Qt::transparent) ? m_fillColor : Qt::white;
        QColor c = QColorDialog::getColor(initColor, nullptr, tr("Choose Textbox Background Color"), QColorDialog::ShowAlphaChannel);
        if (c.isValid()) {
            setFillColor(c);
        }
    } else if (chosen == actTrans) {
        setFillColor(Qt::transparent);
    } else if (chosen == actTextColor) {
        QColor c = QColorDialog::getColor(m_strokeColor, nullptr, tr("Choose Text Color"));
        if (c.isValid()) {
            setStrokeColor(c);
        }
    } else if (chosen == actFont) {
        bool ok = false;
        QFont f = QFontDialog::getFont(&ok, font(), nullptr, tr("Choose Font"));
        if (ok) {
            setFont(f);
        }
    } else if (chosen == actDelete) {
        if (scene()) {
            scene()->removeItem(this);
            setVisible(false);
        }
    }
}
