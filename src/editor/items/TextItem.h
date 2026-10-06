#pragma once

#include <QGraphicsTextItem>
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QMenu>
#include <QColorDialog>
#include <QFontDialog>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QFocusEvent>
#include <QKeyEvent>

class TextItem : public QGraphicsTextItem {
    Q_OBJECT

public:
    enum { Type = UserType + 6 };
    int type() const override { return Type; }

    explicit TextItem(const QString& text = "", const QPointF& pos = QPointF());

    void setText(const QString& t);
    QString text() const { return toPlainText(); }

    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor& c);

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor& c);

    bool isEditing() const { return m_isEditing; }
    void startEditing();
    void finishEditing();

    void setInitialCreation(bool b) { m_isInitialCreation = b; }
    bool isInitialCreation() const { return m_isInitialCreation; }

    QRectF boundingRect() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

signals:
    void initialCreationFinished(bool hasText);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QColor m_strokeColor = QColor(255, 30, 30);
    QColor m_fillColor = Qt::transparent; // Transparent by default
    bool m_isEditing = false;
    bool m_isInitialCreation = false;
};
