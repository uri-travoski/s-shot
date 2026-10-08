#pragma once

#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QUndoStack>
#include <QUndoCommand>
#include "ToolType.h"
#include "items/BaseAnnotationItem.h"
#include "items/PenItem.h"
#include "items/ArrowItem.h"
#include "items/ShapeItem.h"
#include "items/BadgeItem.h"
#include "items/TextItem.h"
#include "items/BlurItem.h"

class CanvasScene : public QGraphicsScene {
    Q_OBJECT

public:
    explicit CanvasScene(QObject* parent = nullptr);

    void setBasePixmap(const QPixmap& pixmap);
    QPixmap basePixmap() const;
    QPixmap renderToPixmap() const;

    QUndoStack* undoStack() { return &m_undoStack; }

    ToolType currentTool() const { return m_currentTool; }
    void setCurrentTool(ToolType tool);

    QColor strokeColor() const { return m_strokeColor; }
    void setStrokeColor(const QColor& c) { m_strokeColor = c; }

    QColor fillColor() const { return m_fillColor; }
    void setFillColor(const QColor& c) { m_fillColor = c; }

    int strokeWidth() const { return m_strokeWidth; }
    void setStrokeWidth(int w) { m_strokeWidth = w; }

    QFont currentFont() const { return m_font; }
    void setCurrentFont(const QFont& f) { m_font = f; }

    int badgeCounter() const { return m_badgeCounter; }
    void setBadgeCounter(int n);
    void resetBadgeCounter();
    void modifyBadgeNumber(BadgeItem* badge, int newNumber);

    int blurLevel() const { return m_blurLevel; }
    void setBlurLevel(int level) { m_blurLevel = qBound(1, level, 10); }

    // Area selection operations
    QRectF selectedArea() const { return m_selectedArea; }
    void setSelectedArea(const QRectF& rect);
    bool hasAreaSelection() const { return !m_selectedArea.isNull() && m_selectedArea.width() > 2 && m_selectedArea.height() > 2; }
    void clearAreaSelection();

    void copySelectedArea();
    void cutSelectedArea();
    void moveSelectedArea();
    void deleteSelectedArea();
    void cropToArea(const QRectF& rect);
    void cropToSelectedArea();

    // Canvas edge resize handles
    enum class CanvasHandle {
        None,
        Top,
        Bottom,
        Left,
        Right,
        TopLeft,
        TopRight,
        BottomLeft,
        BottomRight
    };

    CanvasHandle handleAt(const QPointF& pos) const;
    QRectF handleRect(CanvasHandle h) const;
    static Qt::CursorShape cursorForHandle(CanvasHandle h);
    CanvasHandle activeHandle() const { return m_activeHandle; }
    CanvasHandle hoveredHandle() const { return m_hoveredHandle; }
    void resizeCanvas(const QRectF& newBounds, const QString& undoText = "Resize Canvas");
    void shiftAnnotationItems(const QPointF& offset);
    bool isSystemItem(QGraphicsItem* item) const;

signals:
    void areaSelectionChanged(const QRectF& area, bool active);
    void sceneModified();
    void toolActionCompleted();
    void badgeCounterChanged(int nextNumber);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

private:
    void createNewItem(const QPointF& pos);
    void updateActiveItem(const QPointF& pos);
    void finishActiveItem();

    QGraphicsPixmapItem* m_basePixmapItem = nullptr;
    QUndoStack m_undoStack;

    ToolType m_currentTool = ToolType::Select;
    QColor m_strokeColor = QColor(255, 30, 30);
    QColor m_fillColor = Qt::transparent;
    int m_strokeWidth = 3;
    QFont m_font = QFont("Sans", 14, QFont::Bold);
    int m_badgeCounter = 1;
    int m_blurLevel = 4;

    // Drawing state
    QPointF m_startPoint;
    BaseAnnotationItem* m_activeItem = nullptr;
    bool m_isDrawing = false;

    // Area selection state
    bool m_isSelectingArea = false;
    QRectF m_selectedArea;
    QGraphicsRectItem* m_areaSelectionRectItem = nullptr;

    // Canvas edge resize state
    QGraphicsItem* m_canvasFrameItem = nullptr;
    QGraphicsRectItem* m_canvasResizeGuideItem = nullptr;
    CanvasHandle m_activeHandle = CanvasHandle::None;
    CanvasHandle m_hoveredHandle = CanvasHandle::None;
    bool m_isResizingCanvas = false;
    QRectF m_resizeOriginalRect;
};
