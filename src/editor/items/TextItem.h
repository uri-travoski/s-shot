#pragma once

#include "BaseAnnotationItem.h"
#include <QFont>
#include <QString>

class TextItem : public BaseAnnotationItem {
public:
    enum { Type = UserType + 6 };
    int type() const override { return Type; }

    TextItem(const QString& text = "", const QPointF& pos = QPointF());

    void setText(const QString& t);
    QString text() const { return m_text; }

    void setFont(const QFont& f);
    QFont font() const { return m_font; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

private:
    void calculateBounds();

    QString m_text;
    QFont m_font;
    QRectF m_textRect;
};
