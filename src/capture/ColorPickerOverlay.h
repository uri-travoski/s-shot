#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QColor>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>

class ColorPickerOverlay : public QWidget {
    Q_OBJECT

public:
    explicit ColorPickerOverlay(QWidget* parent = nullptr);
    void startPicking();

signals:
    void colorPicked(const QColor& color, const QString& hexCode);
    void pickingCancelled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    QPixmap m_screenGrab;
    QPoint m_currentPos;
};
