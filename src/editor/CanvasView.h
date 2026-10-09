#pragma once

#include <QGraphicsView>
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include "CanvasScene.h"

class CanvasView : public QGraphicsView {
    Q_OBJECT

public:
    explicit CanvasView(CanvasScene* scene, QWidget* parent = nullptr);

    CanvasScene* canvasScene() const { return m_scene; }

    qreal zoomFactor() const { return m_zoomFactor; }
    void zoomIn();
    void zoomOut();
    void zoomActual();
    void zoomFit();

    void applyTheme(bool isLight);
    void updateToolCursor();

signals:
    void zoomChanged(double factor);
    void mouseMovedTo(const QPoint& scenePos);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onAreaSelectionChanged(const QRectF& area, bool active);

private:
    void applyZoom(qreal factor);
    void updateFloatingBarPosition();
    void updateViewSceneRect();

    CanvasScene* m_scene = nullptr;
    qreal m_zoomFactor = 1.0;
    bool m_initialFitDone = false;
    bool m_isPanning = false;
    QPoint m_panStart;

    QWidget* m_areaActionWidget = nullptr;
    QPushButton* m_copyBtn = nullptr;
    QPushButton* m_cutBtn = nullptr;
    QPushButton* m_moveBtn = nullptr;
    QPushButton* m_deleteBtn = nullptr;
    QPushButton* m_cropBtn = nullptr;
    QRectF m_currentArea;
};
