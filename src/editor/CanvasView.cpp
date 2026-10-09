#include "CanvasView.h"
#include <QWheelEvent>
#include <QScrollBar>
#include <QGraphicsPixmapItem>
#include <QTimer>
#include <QPainterPath>

CanvasView::CanvasView(CanvasScene* scene, QWidget* parent)
    : QGraphicsView(scene, parent)
    , m_scene(scene)
{
    setRenderHint(QPainter::Antialiasing, true);
    setRenderHint(QPainter::SmoothPixmapTransform, true);
    setDragMode(QGraphicsView::NoDrag);
    setMouseTracking(true);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorUnderMouse);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    m_areaActionWidget = new QWidget(this);
    QHBoxLayout* actLayout = new QHBoxLayout(m_areaActionWidget);
    actLayout->setContentsMargins(4, 4, 4, 4);
    actLayout->setSpacing(4);

    m_copyBtn = new QPushButton(tr("📋 Copy"), m_areaActionWidget);
    m_cutBtn = new QPushButton(tr("✂ Cut"), m_areaActionWidget);
    m_moveBtn = new QPushButton(tr("✥ Move"), m_areaActionWidget);
    m_deleteBtn = new QPushButton(tr("🗑 Delete"), m_areaActionWidget);
    m_cropBtn = new QPushButton(tr("⛶ Crop"), m_areaActionWidget);

    connect(m_copyBtn, &QPushButton::clicked, m_scene, &CanvasScene::copySelectedArea);
    connect(m_cutBtn, &QPushButton::clicked, m_scene, &CanvasScene::cutSelectedArea);
    connect(m_moveBtn, &QPushButton::clicked, m_scene, &CanvasScene::moveSelectedArea);
    connect(m_deleteBtn, &QPushButton::clicked, m_scene, &CanvasScene::deleteSelectedArea);
    connect(m_cropBtn, &QPushButton::clicked, m_scene, &CanvasScene::cropToSelectedArea);

    actLayout->addWidget(m_copyBtn);
    actLayout->addWidget(m_cutBtn);
    actLayout->addWidget(m_moveBtn);
    actLayout->addWidget(m_deleteBtn);
    actLayout->addWidget(m_cropBtn);

    m_areaActionWidget->hide();

    connect(m_scene, &CanvasScene::areaSelectionChanged, this, &CanvasView::onAreaSelectionChanged);
    connect(m_scene, &QGraphicsScene::sceneRectChanged, this, [this](const QRectF&) {
        updateViewSceneRect();
        viewport()->update();
    });
    if (!m_scene->sceneRect().isEmpty()) {
        updateViewSceneRect();
    }

    applyTheme(false);
}

void CanvasView::updateViewSceneRect() {
    if (!m_scene) return;
    QRectF scRect = m_scene->sceneRect();
    if (scRect.isEmpty()) return;

    qreal scaledW = scRect.width() * m_zoomFactor;
    qreal scaledH = scRect.height() * m_zoomFactor;
    bool exceedsW = scaledW > viewport()->width();
    bool exceedsH = scaledH > viewport()->height();

    if (exceedsW || exceedsH) {
        const qreal margin = 64.0;
        setSceneRect(scRect.adjusted(-margin, -margin, margin, margin));
    } else {
        setSceneRect(scRect);
    }
}

void CanvasView::applyTheme(bool isLight) {
    if (isLight) {
        setBackgroundBrush(QBrush(QColor(185, 185, 185)));
        setStyleSheet(
            "QGraphicsView { border: none; background-color: #b9b9b9; }"
            "QScrollBar:horizontal { background: #d6d6d6; height: 12px; margin: 0px; border: none; }"
            "QScrollBar::handle:horizontal { background: #b0b0b0; min-width: 24px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:horizontal:hover { background: #959595; }"
            "QScrollBar::handle:horizontal:pressed { background: #7c7c7c; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; background: none; border: none; }"
            "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"
            "QScrollBar:vertical { background: #d6d6d6; width: 12px; margin: 0px; border: none; }"
            "QScrollBar::handle:vertical { background: #b0b0b0; min-height: 24px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:vertical:hover { background: #959595; }"
            "QScrollBar::handle:vertical:pressed { background: #7c7c7c; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
            "QScrollBar::corner { background: #d6d6d6; border: none; }"
        );
        m_areaActionWidget->setStyleSheet(
            "QWidget { background-color: #e4e4e4; border: 1px solid #bbbbbb; border-radius: 6px; padding: 2px; }"
            "QPushButton { background-color: #ffffff; color: #222222; border: 1px solid #c0c0c0; border-radius: 4px; padding: 4px 8px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background-color: #f0f0f0; border-color: #888888; color: #111111; }"
        );
    } else {
        setBackgroundBrush(QBrush(QColor(36, 36, 36)));
        setStyleSheet(
            "QGraphicsView { border: none; background-color: #242424; }"
            "QScrollBar:horizontal { background: #202020; height: 12px; margin: 0px; border: none; }"
            "QScrollBar::handle:horizontal { background: #484848; min-width: 24px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:horizontal:hover { background: #606060; }"
            "QScrollBar::handle:horizontal:pressed { background: #787878; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; background: none; border: none; }"
            "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal { background: none; }"
            "QScrollBar:vertical { background: #202020; width: 12px; margin: 0px; border: none; }"
            "QScrollBar::handle:vertical { background: #484848; min-height: 24px; border-radius: 4px; margin: 2px; }"
            "QScrollBar::handle:vertical:hover { background: #606060; }"
            "QScrollBar::handle:vertical:pressed { background: #787878; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; background: none; border: none; }"
            "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { background: none; }"
            "QScrollBar::corner { background: #202020; border: none; }"
        );
        m_areaActionWidget->setStyleSheet(
            "QWidget { background-color: #2b2b2b; border: 1px solid #484848; border-radius: 6px; padding: 2px; }"
            "QPushButton { background-color: #383838; color: #ffffff; border: 1px solid #505050; border-radius: 4px; padding: 4px 8px; font-weight: bold; font-size: 11px; }"
            "QPushButton:hover { background-color: #484848; border-color: #666666; color: #ffffff; }"
        );
    }
    updateToolCursor();
}

void CanvasView::applyZoom(qreal factor) {
    factor = qBound(0.1, factor, 10.0);
    if (qFuzzyCompare(factor, m_zoomFactor)) return;
    m_zoomFactor = factor;
    updateViewSceneRect();
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
    if (!m_scene || m_scene->sceneRect().isEmpty()) return;
    if (viewport()->width() <= 30 || viewport()->height() <= 30) return;
    qreal wFactor = (viewport()->width() - 30) / m_scene->sceneRect().width();
    qreal hFactor = (viewport()->height() - 30) / m_scene->sceneRect().height();
    applyZoom(qMin(wFactor, hFactor));
    centerOn(m_scene->sceneRect().center());
}

void CanvasView::wheelEvent(QWheelEvent* event) {
    if (event->angleDelta().y() == 0 && event->angleDelta().x() == 0) {
        event->accept();
        return;
    }

    if (event->modifiers() & Qt::ShiftModifier) {
        int delta = event->angleDelta().y() != 0 ? event->angleDelta().y() : event->angleDelta().x();
        horizontalScrollBar()->setValue(horizontalScrollBar()->value() - delta);
        event->accept();
        return;
    }

    if (event->angleDelta().y() != 0) {
        QPointF mousePos = event->position();
        QPointF scenePos = mapToScene(mousePos.toPoint());

        qreal factor = (event->angleDelta().y() > 0) ? 1.25 : (1.0 / 1.25);
        qreal newZoom = qBound(0.1, m_zoomFactor * factor, 10.0);
        if (!qFuzzyCompare(newZoom, m_zoomFactor)) {
            m_zoomFactor = newZoom;
            setTransform(QTransform::fromScale(m_zoomFactor, m_zoomFactor));
            QPointF newMousePos = mapFromScene(scenePos);
            QPointF delta = newMousePos - mousePos;
            horizontalScrollBar()->setValue(horizontalScrollBar()->value() + delta.x());
            verticalScrollBar()->setValue(verticalScrollBar()->value() + delta.y());
            updateFloatingBarPosition();
            emit zoomChanged(m_zoomFactor);
        }
    }
    event->accept();
}

void CanvasView::updateToolCursor() {
    if (!m_scene) {
        unsetCursor();
        return;
    }
    switch (m_scene->currentTool()) {
    case ToolType::Pan:
        setCursor(Qt::OpenHandCursor);
        break;
    case ToolType::Select:
        setCursor(Qt::ArrowCursor);
        break;
    case ToolType::Pen:
    case ToolType::Highlighter:
    case ToolType::Line:
    case ToolType::Arrow:
    case ToolType::DoubleArrow:
    case ToolType::Rectangle:
    case ToolType::Ellipse:
    case ToolType::Badge:
    case ToolType::Blur:
    case ToolType::Crop:
        setCursor(Qt::CrossCursor);
        break;
    case ToolType::Text:
        setCursor(Qt::IBeamCursor);
        break;
    case ToolType::BucketFill:
        setCursor(Qt::PointingHandCursor);
        break;
    case ToolType::ColorPicker:
        setCursor(Qt::CrossCursor);
        break;
    }
    if (m_scene && m_scene->currentTool() != ToolType::ColorPicker && m_showColorPickerLoupe) {
        m_showColorPickerLoupe = false;
        viewport()->update();
    }
}

void CanvasView::mousePressEvent(QMouseEvent* event) {
    if (m_scene && m_scene->currentTool() == ToolType::Pan && event->button() == Qt::LeftButton) {
        QPointF scPos = mapToScene(event->pos());
        if (m_scene->handleAt(scPos) == CanvasScene::CanvasHandle::None) {
            m_isPanning = true;
            m_panStart = event->pos();
            setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }
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

    if (m_scene && m_scene->currentTool() == ToolType::ColorPicker) {
        m_showColorPickerLoupe = true;
        m_colorPickerPos = event->pos();
        viewport()->update();
    } else if (m_showColorPickerLoupe) {
        m_showColorPickerLoupe = false;
        viewport()->update();
    }

    QGraphicsView::mouseMoveEvent(event);
}

void CanvasView::mouseReleaseEvent(QMouseEvent* event) {
    if (m_isPanning && (event->button() == Qt::LeftButton || event->button() == Qt::MiddleButton)) {
        m_isPanning = false;
        updateToolCursor();
        event->accept();
        return;
    }
    QGraphicsView::mouseReleaseEvent(event);
    updateFloatingBarPosition();
}

void CanvasView::showEvent(QShowEvent* event) {
    QGraphicsView::showEvent(event);
    if (!m_initialFitDone) {
        m_initialFitDone = true;
        QTimer::singleShot(0, this, [this]() {
            zoomFit();
        });
    }
}

void CanvasView::resizeEvent(QResizeEvent* event) {
    QGraphicsView::resizeEvent(event);
    updateViewSceneRect();
    if (!m_initialFitDone && viewport()->width() > 50 && viewport()->height() > 50) {
        m_initialFitDone = true;
        QTimer::singleShot(0, this, [this]() {
            zoomFit();
        });
    }
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

void CanvasView::leaveEvent(QEvent* event) {
    if (m_showColorPickerLoupe) {
        m_showColorPickerLoupe = false;
        viewport()->update();
    }
    QGraphicsView::leaveEvent(event);
}

void CanvasView::paintEvent(QPaintEvent* event) {
    QGraphicsView::paintEvent(event);

    if (m_showColorPickerLoupe && m_scene && m_scene->currentTool() == ToolType::ColorPicker) {
        QPainter p(viewport());
        drawColorPickerLoupe(p, m_colorPickerPos);
    }
}

void CanvasView::drawColorPickerLoupe(QPainter& p, const QPoint& viewPos) {
    if (!m_scene) return;

    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, false);

    QPointF scPos = mapToScene(viewPos);
    QColor sampledColor = m_scene->colorAt(scPos);
    if (!sampledColor.isValid()) {
        sampledColor = backgroundBrush().color();
    }

    const int span = 11;
    QImage patch = m_scene->imagePatch(scPos, span);

    const int loupeRadius = 42;
    const int loupeSize = loupeRadius * 2;
    int offsetX = 24;
    int offsetY = -loupeSize - 20;

    if (viewPos.x() + offsetX + loupeSize > viewport()->width() - 8) {
        offsetX = -loupeSize - 24;
    }
    if (viewPos.y() + offsetY < 8) {
        offsetY = 24;
    }

    QPoint center = viewPos + QPoint(offsetX + loupeRadius, offsetY + loupeRadius);
    QRect loupeRect(center.x() - loupeRadius, center.y() - loupeRadius, loupeSize, loupeSize);

    p.save();

    // Outer shadow / subtle glow
    p.setPen(QPen(QColor(0, 0, 0, 80), 3));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(loupeRect.adjusted(-1, -1, 1, 1));

    // Circular clip
    QPainterPath clipPath;
    clipPath.addEllipse(loupeRect);
    p.setClipPath(clipPath);

    // Draw magnified patch
    p.drawImage(loupeRect, patch);

    // Draw grid over pixels
    double step = static_cast<double>(loupeSize) / span;
    p.setPen(QPen(QColor(255, 255, 255, 40), 1));
    for (int i = 1; i < span; ++i) {
        int pos = qRound(loupeRect.left() + i * step);
        p.drawLine(pos, loupeRect.top(), pos, loupeRect.bottom());
        pos = qRound(loupeRect.top() + i * step);
        p.drawLine(loupeRect.left(), pos, loupeRect.right(), pos);
    }

    // Highlight center pixel
    int centerIdx = span / 2;
    QRectF centerPixelRect(loupeRect.left() + centerIdx * step,
                           loupeRect.top() + centerIdx * step,
                           step, step);
    p.setPen(QPen(QColor(255, 255, 255, 220), 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRect(centerPixelRect);
    p.setPen(QPen(QColor(0, 0, 0, 200), 1.0));
    p.drawRect(centerPixelRect.adjusted(-1, -1, 1, 1));

    p.restore();

    // Draw loupe outer ring
    p.setPen(QPen(QColor(255, 255, 255), 2.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(loupeRect);
    p.setPen(QPen(QColor(40, 40, 40, 180), 1.0));
    p.drawEllipse(loupeRect.adjusted(1, 1, -1, -1));

    // Draw color info badge below loupe
    QString hexStr = sampledColor.name().toUpper();
    QRect infoRect(center.x() - 44, loupeRect.bottom() + 4, 88, 20);

    // Pill background
    p.setPen(QPen(QColor(255, 255, 255, 120), 1));
    p.setBrush(QColor(24, 24, 24, 230));
    p.drawRoundedRect(infoRect, 4, 4);

    // Color swatch
    QRect swatchRect(infoRect.left() + 4, infoRect.top() + 3, 14, 14);
    p.setPen(QPen(QColor(200, 200, 200), 1));
    p.setBrush(sampledColor);
    p.drawRect(swatchRect);

    // Hex text
    p.setPen(Qt::white);
    QFont font("Monospace", 9, QFont::Bold);
    font.setStyleHint(QFont::Monospace);
    p.setFont(font);
    p.drawText(QRect(infoRect.left() + 22, infoRect.top(), infoRect.width() - 24, infoRect.height()),
               Qt::AlignVCenter | Qt::AlignLeft, hexStr);
}
