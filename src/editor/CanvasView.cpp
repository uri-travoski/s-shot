#include "CanvasView.h"
#include <QWheelEvent>
#include <QScrollBar>
#include <QGraphicsPixmapItem>

CanvasView::CanvasView(CanvasScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
    setStyleSheet("border: none;");

    m_areaActionWidget = new QWidget(this);
    QHBoxLayout* actLayout = new QHBoxLayout(m_areaActionWidget);
    actLayout->setContentsMargins(4, 4, 4, 4);
    actLayout->setSpacing(4);

    m_copyBtn = new QPushButton(tr("📋 Copy"), m_areaActionWidget);
    m_cutBtn = new QPushButton(tr("✂ Cut"), m_areaActionWidget);
    m_deleteBtn = new QPushButton(tr("🗑 Delete"), m_areaActionWidget);
    m_cropBtn = new QPushButton(tr("⛶ Crop"), m_areaActionWidget);

    connect(m_copyBtn, &QPushButton::clicked, m_scene, &CanvasScene::copySelectedArea);
    connect(m_cutBtn, &QPushButton::clicked, m_scene, &CanvasScene::cutSelectedArea);
    connect(m_deleteBtn, &QPushButton::clicked, m_scene, &CanvasScene::deleteSelectedArea);
    connect(m_cropBtn, &QPushButton::clicked, m_scene, &CanvasScene::cropToSelectedArea);

    actLayout->addWidget(m_copyBtn);
    actLayout->addWidget(m_cutBtn);
    actLayout->addWidget(m_deleteBtn);
    actLayout->addWidget(m_cropBtn);

    m_areaActionWidget->hide();

    connect(m_scene, &CanvasScene::areaSelectionChanged, this, &CanvasView::onAreaSelectionChanged);

    applyTheme(false);
}

void CanvasView::applyTheme(bool isLight) {
    if (isLight) {
        setBackgroundBrush(QBrush(QColor(185, 185, 185)));
        m_areaActionWidget->setStyleSheet(
            "QWidget { background-color: #e4e4e4; border: 1px solid #bbbbbb; border-radius: 6px; padding: 2px; }"
            "QPushButton { background-color: #ffffff; color: #222222; border: 1px solid #c0c0c0; border-radius: 4px; padding: 4px 8px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background-color: #f0f0f0; border-color: #2e7d32; color: #2e7d32; }"
        );
    } else {
        setBackgroundBrush(QBrush(QColor(36, 36, 36)));
        m_areaActionWidget->setStyleSheet(
            "QWidget { background-color: #2b2b2b; border: 1px solid #484848; border-radius: 6px; padding: 2px; }"
            "QPushButton { background-color: #383838; color: #ffffff; border: 1px solid #505050; border-radius: 4px; padding: 4px 8px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background-color: #444444; border-color: #30e500; color: #30e500; }"
        );
    }
}

void CanvasView::applyZoom(qreal factor) {
    factor = qBound(0.1, factor, 10.0);
    m_zoomFactor = factor;
    setTransform(QTransform::fromScale(m_zoomFactor, m_zoomFactor));
    updateFloatingBarPosition();
    emit zoomChanged(m_zoomFactor);
}

void CanvasView::zoomIn() {
    applyZoom(m_zoomFactor * 1.25);
}

void CanvasView::zoomOut() {
    applyZoom(m_zoomFactor / 1.25);
}

void CanvasView::zoomActual() {
    applyZoom(1.0);
}

void CanvasView::zoomFit() {
    if (m_scene->sceneRect().isEmpty()) return;
    qreal wFactor = (viewport()->width() - 30) / m_scene->sceneRect().width();
    qreal hFactor = (viewport()->height() - 30) / m_scene->sceneRect().height();
    applyZoom(qMin(wFactor, hFactor));
}

void CanvasView::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else if (event->angleDelta().y() < 0) {
            zoomOut();
        }
        event->accept();
    } else {
        QGraphicsView::wheelEvent(event);
    }
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton || (event->button() == Qt::LeftButton && (event->modifiers() & Qt::AltModifier))) {
        m_isPanning = true;
        m_panStart = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    QGraphicsView::mousePressEvent(event);
}

void CanvasView::mouseMoveEvent(QMouseEvent* event) {
    if (m_isPanning) {
        QPoint delta = event->pos() - m_panStart;
        m_panStart = event->pos();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta.x());
        verticalScrollBar()->setValue(verticalScrollBar()->value() - delta.y());
        event->accept();
        return;
    }

    QPointF sPos = mapToScene(event->pos());
    emit mouseMovedTo(sPos.toPoint());
    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isPanning) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
    updateFloatingBarPosition();
}

void CanvasView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    updateFloatingBarPosition();
}

void CanvasView::onAreaSelectionChanged(const QRectF& area, bool active) {
    m_currentArea = area;
    if (active && area.width() > 10 && area.height() > 10) {
        updateFloatingBarPosition();
        m_areaActionWidget->show();
        m_areaActionWidget->raise();
    } else {
        m_areaActionWidget->hide();
    }
}

void CanvasView::updateFloatingBarPosition() {
    if (!m_areaActionWidget->isVisible() && !m_scene->hasAreaSelection()) return;

    QRect viewRect = mapFromScene(m_currentArea).boundingRect();
    int barW = m_areaActionWidget->sizeHint().width();
    int barH = m_areaActionWidget->sizeHint().height();

    int posX = viewRect.center().x() - barW / 2;
    int posY = viewRect.bottom() + 8;

    if (posY + barH > height() - 10) {
        posY = viewRect.top() - barH - 8;
    }

    posX = qBound(10, posX, width() - barW - 10);
    posY = qBound(10, posY, height() - barH - 10);

    m_areaActionWidget->move(posX, posY);
}
