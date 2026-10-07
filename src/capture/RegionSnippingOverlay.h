#pragma once

#include <QWidget>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QPainter>
#include <QKeyEvent>
#include <QMouseEvent>

class RegionSnippingOverlay : public QWidget {
    Q_OBJECT

public:
    explicit RegionSnippingOverlay(QWidget* parent = nullptr);
    void startSnipping();
    QRect selectedRect() const { return m_selectedPhysRect; }

signals:
    void regionCaptured(const QPixmap& pixmap);
    void snippingCancelled();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void drawMagnifier(QPainter& painter, const QPoint& pos);

    QPixmap m_screenGrab;
    QPoint m_startPos;
    QPoint m_currentPos;
    QRect m_selectedRect;
    QRect m_selectedPhysRect;
    bool m_isSelecting = false;
    bool m_selectionDone = false;
};
