#include "CanvasScene.h"
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsSceneContextMenuEvent>
#include <QMenu>
#include <QAction>
#include <QKeyEvent>
#include <QPainter>
#include <QInputDialog>
#include <QClipboard>
#include <QGuiApplication>
#include <QDebug>
#include "CanvasView.h"
#include "../core/ClipboardHelper.h"
#include "../core/IconManager.h"

// --- Undo Commands ---

class SetBadgeNumberCommand : public QUndoCommand {
public:
    SetBadgeNumberCommand(CanvasScene* scene, BadgeItem* item, int oldNum, int newNum, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_scene(scene), m_item(item), m_oldNum(oldNum), m_newNum(newNum) {
        setText(QString("Change Badge Number %1 -> %2").arg(oldNum).arg(newNum));
    }
    void undo() override {
        if (m_item) {
            m_item->setNumber(m_oldNum);
            emit m_scene->sceneModified();
        }
    }
    void redo() override {
        if (m_item) {
            m_item->setNumber(m_newNum);
            emit m_scene->sceneModified();
        }
    }
private:
    CanvasScene* m_scene;
    BadgeItem* m_item;
    int m_oldNum;
    int m_newNum;
};

class AddItemCommand : public QUndoCommand {
public:
    AddItemCommand(QGraphicsScene* scene, QGraphicsItem* item, bool alreadyInScene = false, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_scene(scene), m_item(item), m_inScene(alreadyInScene) {
        setText("Add Annotation");
    }
    ~AddItemCommand() {
        if (!m_inScene && m_item) {
            delete m_item;
        }
    }
    void undo() override {
        m_scene->removeItem(m_item);
        m_inScene = false;
    }
    void redo() override {
        if (!m_inScene) {
            m_scene->addItem(m_item);
            m_inScene = true;
        }
    }
private:
    QGraphicsScene* m_scene;
    QGraphicsItem* m_item;
    bool m_inScene = false;
};

class RemoveItemCommand : public QUndoCommand {
public:
    RemoveItemCommand(QGraphicsScene* scene, QGraphicsItem* item, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_scene(scene), m_item(item), m_inScene(true) {
        setText("Delete Item");
    }
    ~RemoveItemCommand() {
        if (!m_inScene && m_item) {
            delete m_item;
        }
    }
    void undo() override {
        if (!m_inScene && m_item) {
            m_scene->addItem(m_item);
            m_inScene = true;
        }
    }
    void redo() override {
        if (m_inScene && m_item) {
            m_scene->removeItem(m_item);
            m_inScene = false;
        }
    }
private:
    QGraphicsScene* m_scene;
    QGraphicsItem* m_item;
    bool m_inScene = true;
};

class ModifyPixmapCommand : public QUndoCommand {
public:
    ModifyPixmapCommand(CanvasScene* scene, const QPixmap& oldPix, const QPixmap& newPix, const QString& text, QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_scene(scene), m_oldPix(oldPix), m_newPix(newPix) {
        setText(text);
    }
    void undo() override {
        m_scene->setBasePixmap(m_oldPix);
    }
    void redo() override {
        m_scene->setBasePixmap(m_newPix);
    }
private:
    CanvasScene* m_scene;
    QPixmap m_oldPix;
    QPixmap m_newPix;
};

class ResizeCanvasCommand : public QUndoCommand {
public:
    ResizeCanvasCommand(CanvasScene* scene, const QPixmap& oldPix, const QPixmap& newPix,
                        const QPointF& itemShift, const QString& text = "Resize Canvas",
                        QUndoCommand* parent = nullptr)
        : QUndoCommand(parent), m_scene(scene), m_oldPix(oldPix), m_newPix(newPix),
          m_shift(itemShift) {
        setText(text);
    }
    void undo() override {
        m_scene->shiftAnnotationItems(-m_shift);
        m_scene->setBasePixmap(m_oldPix);
    }
    void redo() override {
        if (m_executedOnce) {
            m_scene->shiftAnnotationItems(m_shift);
            m_scene->setBasePixmap(m_newPix);
        } else {
            m_executedOnce = true;
        }
    }
private:
    CanvasScene* m_scene;
    QPixmap m_oldPix;
    QPixmap m_newPix;
    QPointF m_shift;
    bool m_executedOnce = false;
};

class CanvasFrameItem : public QGraphicsItem {
public:
    explicit CanvasFrameItem(CanvasScene* scene, QGraphicsItem* parent = nullptr)
        : QGraphicsItem(parent), m_scene(scene) {
        setZValue(9500);
        setAcceptedMouseButtons(Qt::NoButton);
    }

    void notifyGeometryChange() {
        prepareGeometryChange();
    }

    QRectF boundingRect() const override {
        if (!m_scene) return QRectF();
        QRectF r = m_scene->sceneRect();
        return r.adjusted(-12, -12, 12, 12);
    }

    QPainterPath shape() const override {
        // Empty shape so frame doesn't block underlying annotation items from itemAt() queries
        return QPainterPath();
    }

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* /*option*/, QWidget* /*widget*/) override {
        if (!m_scene) return;
        QRectF r = m_scene->sceneRect();
        if (r.isEmpty()) return;

        painter->setRenderHint(QPainter::Antialiasing, true);

        // 1. Subtle dashed border outlining current canvas
        QPen borderPen(QColor(120, 120, 120, 180), 1, Qt::DashLine);
        painter->setPen(borderPen);
        painter->setBrush(Qt::NoBrush);
        painter->drawRect(r);

        // 2. Draw 8 edge/corner resize handles
        CanvasScene::CanvasHandle activeH = m_scene->activeHandle();
        CanvasScene::CanvasHandle hoveredH = m_scene->hoveredHandle();

        const CanvasScene::CanvasHandle allHandles[] = {
            CanvasScene::CanvasHandle::TopLeft, CanvasScene::CanvasHandle::Top,
            CanvasScene::CanvasHandle::TopRight, CanvasScene::CanvasHandle::Right,
            CanvasScene::CanvasHandle::BottomRight, CanvasScene::CanvasHandle::Bottom,
            CanvasScene::CanvasHandle::BottomLeft, CanvasScene::CanvasHandle::Left
        };

        for (CanvasScene::CanvasHandle h : allHandles) {
            QRectF hr = m_scene->handleRect(h);
            bool isHot = (h == activeH || (activeH == CanvasScene::CanvasHandle::None && h == hoveredH));

            QColor fillCol = isHot ? QColor(48, 229, 0) : QColor(255, 255, 255);
            QColor strokeCol = isHot ? QColor(20, 120, 0) : QColor(50, 50, 50);

            painter->setPen(QPen(strokeCol, 1.0));
            painter->setBrush(fillCol);
            painter->drawRect(hr);
        }
    }

private:
    CanvasScene* m_scene = nullptr;
};

// --- CanvasScene ---

CanvasScene::CanvasScene(QObject* parent)
    : QGraphicsScene(parent)
{
    m_basePixmapItem = new QGraphicsPixmapItem();
    m_basePixmapItem->setZValue(-1000);
    addItem(m_basePixmapItem);

    m_canvasFrameItem = new CanvasFrameItem(this);
    addItem(m_canvasFrameItem);

    m_canvasResizeGuideItem = new QGraphicsRectItem();
    m_canvasResizeGuideItem->setZValue(9600);
    m_canvasResizeGuideItem->setPen(QPen(QColor(48, 229, 0), 2, Qt::DashLine));
    m_canvasResizeGuideItem->setBrush(QColor(48, 229, 0, 25));
    m_canvasResizeGuideItem->setVisible(false);
    addItem(m_canvasResizeGuideItem);

    m_areaSelectionItem = new AreaSelectionItem();
    m_areaSelectionItem->setVisible(false);
    addItem(m_areaSelectionItem);
}

bool CanvasScene::isSystemItem(QGraphicsItem* item) const {
    return item == m_basePixmapItem || item == m_areaSelectionItem ||
           item == m_canvasFrameItem || item == m_canvasResizeGuideItem;
}

void CanvasScene::setBasePixmap(const QPixmap& pixmap) {
    QPixmap normPix = pixmap;
    normPix.setDevicePixelRatio(1.0);
    m_basePixmapItem->setPixmap(normPix);
    setSceneRect(normPix.rect());
    if (m_areaSelectionItem) {
        m_areaSelectionItem->setCanvasRect(normPix.rect());
    }
    clearAreaSelection();
    if (m_canvasFrameItem) {
        static_cast<CanvasFrameItem*>(m_canvasFrameItem)->notifyGeometryChange();
        m_canvasFrameItem->update();
    }
    for (auto* item : items()) {
        if (auto* blur = dynamic_cast<BlurItem*>(item)) {
            blur->updateEffect(normPix);
        }
    }
    update();
    for (auto* view : views()) {
        view->viewport()->update();
    }
    emit sceneModified();
}

QPixmap CanvasScene::basePixmap() const {
    return m_basePixmapItem->pixmap();
}

QPixmap CanvasScene::renderToPixmap() const {
    QRectF sr = sceneRect();
    if (sr.isEmpty()) return QPixmap();

    QPixmap result(sr.toRect().size());
    result.fill(Qt::transparent);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

    // Hide selection markers, canvas frame, and guides during export
    if (m_areaSelectionItem) m_areaSelectionItem->setVisible(false);
    if (m_canvasFrameItem) m_canvasFrameItem->setVisible(false);
    if (m_canvasResizeGuideItem) m_canvasResizeGuideItem->setVisible(false);

    // Finish editing on any active text item during export
    for (auto* item : items()) {
        if (auto* txt = dynamic_cast<TextItem*>(item)) {
            if (txt->isEditing()) {
                txt->finishEditing();
            }
        }
    }

    // Deselect items temporarily for clean render
    bool oldBlock = const_cast<CanvasScene*>(this)->blockSignals(true);
    QList<QGraphicsItem*> selected = selectedItems();
    for (auto* item : selected) item->setSelected(false);

    const_cast<CanvasScene*>(this)->render(&painter, sr, sr);

    // Restore selection and frame
    for (auto* item : selected) item->setSelected(true);
    if (hasAreaSelection() && m_areaSelectionItem) {
        m_areaSelectionItem->setVisible(true);
    }
    if (m_canvasFrameItem) m_canvasFrameItem->setVisible(true);
    const_cast<CanvasScene*>(this)->blockSignals(oldBlock);

    result.setDevicePixelRatio(1.0);
    return result;
}

QColor CanvasScene::colorAt(const QPointF& pos) const {
    QRectF sr = sceneRect();
    if (!sr.contains(pos)) {
        if (!views().isEmpty()) {
            return views().first()->backgroundBrush().color();
        }
        return QColor(36, 36, 36);
    }

    int px = qRound(pos.x());
    int py = qRound(pos.y());

    // 1. If pos is within base pixmap and has no annotations on top, read directly
    if (m_basePixmapItem && !m_basePixmapItem->pixmap().isNull()) {
        QRect imgRect = m_basePixmapItem->pixmap().rect();
        if (imgRect.contains(px, py)) {
            QList<QGraphicsItem*> itemsAtPos = items(pos);
            bool hasAnnotations = false;
            for (auto* it : itemsAtPos) {
                if (!isSystemItem(it) && it != m_basePixmapItem) {
                    hasAnnotations = true;
                    break;
                }
            }
            if (!hasAnnotations) {
                return m_basePixmapItem->pixmap().toImage().pixelColor(px, py);
            }
        }
    }

    // 2. Render 1x1 pixel evaluating all items/annotations
    QImage pixelImg(1, 1, QImage::Format_ARGB32_Premultiplied);
    pixelImg.fill(Qt::transparent);
    QPainter p(&pixelImg);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    const_cast<CanvasScene*>(this)->render(&p, QRectF(0, 0, 1, 1), QRectF(pos.x(), pos.y(), 1, 1));
    p.end();

    QColor c = pixelImg.pixelColor(0, 0);
    if (c.alpha() == 0 && m_basePixmapItem && !m_basePixmapItem->pixmap().isNull()) {
        QRect imgRect = m_basePixmapItem->pixmap().rect();
        if (imgRect.contains(px, py)) {
            return m_basePixmapItem->pixmap().toImage().pixelColor(px, py);
        }
    }
    return c;
}

QImage CanvasScene::imagePatch(const QPointF& centerPos, int span) const {
    span = qMax(3, span);
    QImage patch(span, span, QImage::Format_ARGB32_Premultiplied);
    QColor bg = (!views().isEmpty()) ? views().first()->backgroundBrush().color() : QColor(36, 36, 36);
    patch.fill(bg);
    QPainter p(&patch);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    int half = span / 2;
    QRectF target(0, 0, span, span);
    QRectF source(centerPos.x() - half, centerPos.y() - half, span, span);
    const_cast<CanvasScene*>(this)->render(&p, target, source);
    p.end();
    return patch;
}

void CanvasScene::setCurrentTool(ToolType tool) {
    m_currentTool = tool;
    clearAreaSelection();
    if (m_areaSelectionItem) {
        m_areaSelectionItem->setCanvasRect(sceneRect());
        m_areaSelectionItem->setCropMode(tool == ToolType::Crop);
    }

    // Enable/disable movable flags depending on tool
    auto allItems = items();
    for (auto* item : allItems) {
        if (!isSystemItem(item)) {
            item->setFlag(QGraphicsItem::ItemIsSelectable, tool == ToolType::Select);
            item->setFlag(QGraphicsItem::ItemIsMovable, tool == ToolType::Select);
            if (auto* txt = dynamic_cast<TextItem*>(item)) {
                if (tool != ToolType::Text && txt->isEditing()) {
                    txt->finishEditing();
                }
            }
        }
    }

    for (auto* view : views()) {
        if (auto* cv = qobject_cast<CanvasView*>(view)) {
            cv->updateToolCursor();
        }
    }
}

void CanvasScene::setSelectedArea(const QRectF& rect) {
    m_selectedArea = rect.normalized();
    if (m_areaSelectionItem) {
        m_areaSelectionItem->setCanvasRect(sceneRect());
        m_areaSelectionItem->setCropMode(m_currentTool == ToolType::Crop);
        m_areaSelectionItem->setRect(m_selectedArea);
        m_areaSelectionItem->setVisible(!rect.isNull() && rect.width() > 2 && rect.height() > 2);
    }
    emit areaSelectionChanged(m_selectedArea, hasAreaSelection());
}

void CanvasScene::clearAreaSelection() {
    m_selectedArea = QRectF();
    m_cropActiveHandle = AreaSelectionItem::Handle::None;
    if (m_areaSelectionItem) {
        m_areaSelectionItem->setVisible(false);
        m_areaSelectionItem->setRect(QRectF());
    }
    emit areaSelectionChanged(QRectF(), false);
}

void CanvasScene::copyActiveImage() {
    if (hasAreaSelection()) {
        copySelectedArea();
        return;
    }
    QPixmap fullPix = renderToPixmap();
    if (!fullPix.isNull()) {
        fullPix.setDevicePixelRatio(1.0);
        if (ClipboardHelper::copyImage(fullPix)) {
            emit statusMessageRequested(tr("Image copied to clipboard!"));
        }
    }
}

void CanvasScene::copySelectedArea() {
    if (!hasAreaSelection()) return;
    QPixmap fullPix = renderToPixmap();
    fullPix.setDevicePixelRatio(1.0);
    QRect cropRect = m_selectedArea.toRect().intersected(fullPix.rect());
    if (!cropRect.isEmpty() && cropRect.width() >= 2 && cropRect.height() >= 2) {
        QPixmap sub = fullPix.copy(cropRect);
        sub.setDevicePixelRatio(1.0);
        if (ClipboardHelper::copyImage(sub)) {
            emit statusMessageRequested(tr("Selected area copied to clipboard!"));
        }
    }
}

void CanvasScene::cutSelectedArea() {
    if (!hasAreaSelection()) return;
    copySelectedArea();
    deleteSelectedArea();
}

void CanvasScene::moveSelectedArea() {
    if (!hasAreaSelection()) return;
    QRect cropRect = m_selectedArea.toRect().intersected(m_basePixmapItem->pixmap().rect());
    if (cropRect.width() < 2 || cropRect.height() < 2) return;

    QPixmap patch = m_basePixmapItem->pixmap().copy(cropRect);

    // Clear the selected area from base pixmap with clean white background
    QPixmap oldPix = m_basePixmapItem->pixmap();
    QPixmap newPix = oldPix;
    QPainter p(&newPix);
    p.fillRect(cropRect, Qt::white);
    p.end();

    PixmapItem* floatingPatch = new PixmapItem(patch);
    floatingPatch->setPos(cropRect.topLeft());

    m_undoStack.beginMacro("Move Area");
    m_undoStack.push(new ModifyPixmapCommand(this, oldPix, newPix, "Clear Moved Area"));
    m_undoStack.push(new AddItemCommand(this, floatingPatch));
    m_undoStack.endMacro();

    clearSelection();
    floatingPatch->setSelected(true);
    clearAreaSelection();
    emit sceneModified();
}

void CanvasScene::deleteSelectedArea() {
    if (!hasAreaSelection()) return;
    QPixmap oldPix = m_basePixmapItem->pixmap();
    QPixmap newPix = oldPix;

    QPainter p(&newPix);
    p.fillRect(m_selectedArea.toRect().intersected(oldPix.rect()), Qt::white);
    p.end();

    m_undoStack.push(new ModifyPixmapCommand(this, oldPix, newPix, "Delete Area"));
    clearAreaSelection();
}

void CanvasScene::cropToSelectedArea() {
    if (!hasAreaSelection()) return;
    cropToArea(m_selectedArea);
}

void CanvasScene::cropToArea(const QRectF& rect) {
    QRect cropRect = rect.toRect().intersected(m_basePixmapItem->pixmap().rect());
    if (cropRect.width() < 4 || cropRect.height() < 4) return;

    QPixmap oldPix = m_basePixmapItem->pixmap();
    QPixmap newPix = oldPix.copy(cropRect);

    // Shift vector items to account for crop origin
    QPointF offset(-cropRect.x(), -cropRect.y());
    for (auto* item : items()) {
        if (!isSystemItem(item)) {
            item->setPos(item->pos() + offset);
        }
    }

    m_undoStack.push(new ModifyPixmapCommand(this, oldPix, newPix, "Crop Image"));
    clearAreaSelection();
}

void CanvasScene::pasteImage(const QPixmap& pix, const QPointF& pos) {
    if (pix.isNull()) return;

    clearSelection();

    QPointF pastePos;
    if (!pos.isNull()) {
        pastePos = pos;
    } else if (hasAreaSelection()) {
        pastePos = m_selectedArea.topLeft();
    } else if (!views().isEmpty() && views().first()->viewport()) {
        QGraphicsView* v = views().first();
        QPoint viewportCenter = v->viewport()->rect().center();
        QPointF sceneCenter = v->mapToScene(viewportCenter);
        pastePos = sceneCenter - QPointF(pix.width() / 2.0, pix.height() / 2.0);
    } else {
        pastePos = QPointF(
            qMax(0.0, (sceneRect().width() - pix.width()) / 2.0),
            qMax(0.0, (sceneRect().height() - pix.height()) / 2.0)
        );
    }

    // Expand canvas if pasted image extends outside the current sceneRect
    qreal newW = qMax(sceneRect().width(), pastePos.x() + pix.width());
    qreal newH = qMax(sceneRect().height(), pastePos.y() + pix.height());
    if (newW > sceneRect().width() || newH > sceneRect().height()) {
        resizeCanvas(QRectF(0, 0, newW, newH), tr("Expand Canvas for Pasted Image"));
    }

    if (pastePos.x() < 0) pastePos.setX(0);
    if (pastePos.y() < 0) pastePos.setY(0);

    clearAreaSelection();

    PixmapItem* item = new PixmapItem(pix);
    item->setPos(pastePos);
    item->setSelected(true);

    m_undoStack.push(new AddItemCommand(this, item, false));

    setCurrentTool(ToolType::Select);
    emit sceneModified();
}

QRectF CanvasScene::handleRect(CanvasHandle h) const {
    QRectF r = sceneRect();
    if (r.isEmpty() && m_basePixmapItem) {
        r = m_basePixmapItem->pixmap().rect();
    }
    qreal s = 5.0;
    qreal half = s / 2.0;
    qreal w = r.width();
    qreal hgt = r.height();
    qreal x = r.x();
    qreal y = r.y();

    switch (h) {
    case CanvasHandle::TopLeft:     return QRectF(x - half, y - half, s, s);
    case CanvasHandle::Top:         return QRectF(x + w / 2.0 - half, y - half, s, s);
    case CanvasHandle::TopRight:    return QRectF(x + w - half, y - half, s, s);
    case CanvasHandle::Right:       return QRectF(x + w - half, y + hgt / 2.0 - half, s, s);
    case CanvasHandle::BottomRight: return QRectF(x + w - half, y + hgt - half, s, s);
    case CanvasHandle::Bottom:      return QRectF(x + w / 2.0 - half, y + hgt - half, s, s);
    case CanvasHandle::BottomLeft:  return QRectF(x - half, y + hgt - half, s, s);
    case CanvasHandle::Left:        return QRectF(x - half, y + hgt / 2.0 - half, s, s);
    default:                        return QRectF();
    }
}

CanvasScene::CanvasHandle CanvasScene::handleAt(const QPointF& pos) const {
    const CanvasHandle allHandles[] = {
        CanvasHandle::TopLeft, CanvasHandle::TopRight,
        CanvasHandle::BottomLeft, CanvasHandle::BottomRight,
        CanvasHandle::Top, CanvasHandle::Bottom,
        CanvasHandle::Left, CanvasHandle::Right
    };

    const qreal hitRadius = 5.0;
    for (CanvasHandle h : allHandles) {
        QRectF hr = handleRect(h);
        QRectF hitZone = hr.adjusted(-hitRadius, -hitRadius, hitRadius, hitRadius);
        if (hitZone.contains(pos)) {
            return h;
        }
    }
    return CanvasHandle::None;
}

Qt::CursorShape CanvasScene::cursorForHandle(CanvasHandle h) {
    switch (h) {
    case CanvasHandle::Top:
    case CanvasHandle::Bottom:
        return Qt::SizeVerCursor;
    case CanvasHandle::Left:
    case CanvasHandle::Right:
        return Qt::SizeHorCursor;
    case CanvasHandle::TopLeft:
    case CanvasHandle::BottomRight:
        return Qt::SizeFDiagCursor;
    case CanvasHandle::TopRight:
    case CanvasHandle::BottomLeft:
        return Qt::SizeBDiagCursor;
    default:
        return Qt::ArrowCursor;
    }
}

void CanvasScene::resizeCanvas(const QRectF& newBounds, const QString& undoText) {
    if (!m_basePixmapItem) return;
    QPixmap oldPix = m_basePixmapItem->pixmap();
    if (oldPix.isNull()) return;

    int newW = qMax(20, qRound(newBounds.width()));
    int newH = qMax(20, qRound(newBounds.height()));
    int shiftX = qRound(-newBounds.x());
    int shiftY = qRound(-newBounds.y());

    if (newW == oldPix.width() && newH == oldPix.height() && shiftX == 0 && shiftY == 0) {
        return;
    }

    QPixmap newPix(newW, newH);
    newPix.fill(Qt::white);

    QPainter p(&newPix);
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawPixmap(shiftX, shiftY, oldPix);
    p.end();

    QPointF itemOffset(shiftX, shiftY);
    shiftAnnotationItems(itemOffset);
    setBasePixmap(newPix);

    m_undoStack.push(new ResizeCanvasCommand(this, oldPix, newPix, itemOffset, undoText));
}

void CanvasScene::shiftAnnotationItems(const QPointF& offset) {
    if (offset.isNull()) return;
    for (auto* item : items()) {
        if (!isSystemItem(item)) {
            item->setPos(item->pos() + offset);
            if (auto* blur = dynamic_cast<BlurItem*>(item)) {
                blur->updateEffect(m_basePixmapItem->pixmap());
            }
        }
    }
}

void CanvasScene::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (m_currentTool == ToolType::ColorPicker) {
        if (event->button() == Qt::LeftButton || event->button() == Qt::RightButton) {
            m_startPoint = event->scenePos();
            QColor col = colorAt(m_startPoint);
            if (col.isValid()) {
                bool isFill = (event->button() == Qt::RightButton);
                emit colorPicked(col, isFill);
            }
            event->accept();
            return;
        }
    }

    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    m_startPoint = event->scenePos();

    // 0. Boundary restriction: User should only be able to write or add things within the available image canvas
    if (m_currentTool != ToolType::Select && m_currentTool != ToolType::Pan && m_currentTool != ToolType::Crop) {
        if (!sceneRect().contains(m_startPoint)) {
            event->accept();
            return;
        }
    }

    // Check if clicking a handle or inside an existing crop/area selection
    if (hasAreaSelection() && (m_currentTool == ToolType::Crop || m_currentTool == ToolType::Select)) {
        if (m_areaSelectionItem) {
            AreaSelectionItem::Handle h = m_areaSelectionItem->handleAt(m_startPoint);
            if (h != AreaSelectionItem::Handle::None) {
                m_cropActiveHandle = h;
                m_cropPressScenePos = m_startPoint;
                m_cropPressRect = m_selectedArea;
                event->accept();
                return;
            }
        }
    }

    // 1. Check if an edge resize handle was clicked (only active in Select and Pan tools)
    if (m_currentTool == ToolType::Select || m_currentTool == ToolType::Pan) {
        CanvasHandle h = handleAt(m_startPoint);
        if (h != CanvasHandle::None) {
            m_activeHandle = h;
            m_isResizingCanvas = true;
            m_resizeOriginalRect = m_basePixmapItem->pixmap().rect();
            m_canvasResizeGuideItem->setRect(m_resizeOriginalRect);
            m_canvasResizeGuideItem->setVisible(true);
            if (m_canvasFrameItem) m_canvasFrameItem->update();
            event->accept();
            return;
        }
    }

    // 2. Pan tool: CanvasView handles viewport panning
    if (m_currentTool == ToolType::Pan) {
        event->accept();
        return;
    }

    if (m_currentTool == ToolType::Select) {
        // Check if an annotation item was clicked
        QGraphicsItem* clicked = itemAt(m_startPoint, QTransform());
        if (clicked && !isSystemItem(clicked)) {
            clearAreaSelection();
            QGraphicsScene::mousePressEvent(event);
            return;
        }

        // Otherwise, clear selection and start area selection rectangle
        clearSelection();
        m_isSelectingArea = true;
        m_startPoint.setX(qBound(sceneRect().left(), m_startPoint.x(), sceneRect().right()));
        m_startPoint.setY(qBound(sceneRect().top(), m_startPoint.y(), sceneRect().bottom()));
        m_selectedArea = QRectF(m_startPoint, m_startPoint);
        if (m_areaSelectionItem) {
            m_areaSelectionItem->setCanvasRect(sceneRect());
            m_areaSelectionItem->setCropMode(false);
            m_areaSelectionItem->setRect(m_selectedArea);
            m_areaSelectionItem->setVisible(true);
        }
        emit areaSelectionChanged(m_selectedArea, true);
        return;
    }

    if (m_currentTool == ToolType::BucketFill) {
        QColor fillCol = (m_fillColor.isValid() && m_fillColor != Qt::transparent && m_fillColor.alpha() > 0)
                             ? m_fillColor
                             : m_strokeColor;

        // 1. If clicked an existing vector shape or text item, fill it
        QGraphicsItem* clicked = itemAt(m_startPoint, QTransform());
        if (clicked && !isSystemItem(clicked)) {
            if (auto* shape = dynamic_cast<ShapeItem*>(clicked)) {
                shape->setFillColor(fillCol);
                emit sceneModified();
                return;
            }
            if (auto* text = dynamic_cast<TextItem*>(clicked)) {
                text->setFillColor(fillCol);
                emit sceneModified();
                return;
            }
        }

        // 2. Otherwise flood fill the base pixmap at pos
        if (!m_basePixmapItem || m_basePixmapItem->pixmap().isNull()) {
            return;
        }

        QPoint pt = m_startPoint.toPoint();
        QPixmap oldPix = m_basePixmapItem->pixmap();
        if (!oldPix.rect().contains(pt)) {
            return;
        }

        QImage img = oldPix.toImage().convertToFormat(QImage::Format_ARGB32);
        int w = img.width();
        int h = img.height();

        uint32_t targetVal = reinterpret_cast<const uint32_t*>(img.constScanLine(pt.y()))[pt.x()];
        uint32_t fillVal = fillCol.rgba();

        if (targetVal == fillVal) {
            return;
        }

        std::vector<uint8_t> visited(w * h, 0);
        std::vector<int> queue;
        queue.reserve(4096);

        int startIdx = pt.y() * w + pt.x();
        visited[startIdx] = 1;
        reinterpret_cast<uint32_t*>(img.scanLine(pt.y()))[pt.x()] = fillVal;
        queue.push_back(startIdx);

        size_t head = 0;
        while (head < queue.size()) {
            int idx = queue[head++];
            int cx = idx % w;
            int cy = idx / w;

            // Left
            if (cx > 0) {
                int nIdx = idx - 1;
                if (!visited[nIdx]) {
                    uint32_t* row = reinterpret_cast<uint32_t*>(img.scanLine(cy));
                    if (row[cx - 1] == targetVal) {
                        visited[nIdx] = 1;
                        row[cx - 1] = fillVal;
                        queue.push_back(nIdx);
                    }
                }
            }
            // Right
            if (cx + 1 < w) {
                int nIdx = idx + 1;
                if (!visited[nIdx]) {
                    uint32_t* row = reinterpret_cast<uint32_t*>(img.scanLine(cy));
                    if (row[cx + 1] == targetVal) {
                        visited[nIdx] = 1;
                        row[cx + 1] = fillVal;
                        queue.push_back(nIdx);
                    }
                }
            }
            // Up
            if (cy > 0) {
                int nIdx = idx - w;
                if (!visited[nIdx]) {
                    uint32_t* row = reinterpret_cast<uint32_t*>(img.scanLine(cy - 1));
                    if (row[cx] == targetVal) {
                        visited[nIdx] = 1;
                        row[cx] = fillVal;
                        queue.push_back(nIdx);
                    }
                }
            }
            // Down
            if (cy + 1 < h) {
                int nIdx = idx + w;
                if (!visited[nIdx]) {
                    uint32_t* row = reinterpret_cast<uint32_t*>(img.scanLine(cy + 1));
                    if (row[cx] == targetVal) {
                        visited[nIdx] = 1;
                        row[cx] = fillVal;
                        queue.push_back(nIdx);
                    }
                }
            }
        }

        QPixmap newPix = QPixmap::fromImage(img);
        m_undoStack.push(new ModifyPixmapCommand(this, oldPix, newPix, "Flood Fill"));
        emit sceneModified();
        emit toolActionCompleted();
        return;
    }

    if (m_currentTool == ToolType::Crop) {
        m_startPoint.setX(qBound(sceneRect().left(), m_startPoint.x(), sceneRect().right()));
        m_startPoint.setY(qBound(sceneRect().top(), m_startPoint.y(), sceneRect().bottom()));
        m_isSelectingArea = true;
        m_selectedArea = QRectF(m_startPoint, m_startPoint);
        if (m_areaSelectionItem) {
            m_areaSelectionItem->setCanvasRect(sceneRect());
            m_areaSelectionItem->setCropMode(true);
            m_areaSelectionItem->setRect(m_selectedArea);
            m_areaSelectionItem->setVisible(true);
        }
        emit areaSelectionChanged(m_selectedArea, true);
        return;
    }

    if (m_currentTool == ToolType::Text) {
        QGraphicsItem* clicked = itemAt(m_startPoint, QTransform());
        if (clicked && !isSystemItem(clicked)) {
            if (auto* txt = dynamic_cast<TextItem*>(clicked)) {
                clearSelection();
                txt->setSelected(true);
                if (!txt->isEditing()) {
                    txt->startEditing();
                }
                QGraphicsScene::mousePressEvent(event);
                return;
            }
        }
        createNewItem(m_startPoint);
        event->accept();
        return;
    }

    // Creating a new annotation item
    m_isDrawing = true;
    createNewItem(m_startPoint);
    event->accept();
}

void CanvasScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isResizingCanvas) {
        QPointF pt = event->scenePos();
        QRectF orig = m_resizeOriginalRect;
        qreal minX = orig.left();
        qreal maxX = orig.right();
        qreal minY = orig.top();
        qreal maxY = orig.bottom();

        switch (m_activeHandle) {
        case CanvasHandle::Right:
            maxX = qMax(minX + 20.0, pt.x());
            break;
        case CanvasHandle::Bottom:
            maxY = qMax(minY + 20.0, pt.y());
            break;
        case CanvasHandle::BottomRight:
            maxX = qMax(minX + 20.0, pt.x());
            maxY = qMax(minY + 20.0, pt.y());
            break;
        case CanvasHandle::Left:
            minX = qMin(maxX - 20.0, pt.x());
            break;
        case CanvasHandle::Top:
            minY = qMin(maxY - 20.0, pt.y());
            break;
        case CanvasHandle::TopLeft:
            minX = qMin(maxX - 20.0, pt.x());
            minY = qMin(maxY - 20.0, pt.y());
            break;
        case CanvasHandle::TopRight:
            minY = qMin(maxY - 20.0, pt.y());
            maxX = qMax(minX + 20.0, pt.x());
            break;
        case CanvasHandle::BottomLeft:
            minX = qMin(maxX - 20.0, pt.x());
            maxY = qMax(minY + 20.0, pt.y());
            break;
        default:
            break;
        }

        QRectF guideRect(minX, minY, maxX - minX, maxY - minY);
        m_canvasResizeGuideItem->setRect(guideRect);
        event->accept();
        return;
    }

    // 1. Crop / Area selection handle dragging or moving
    if (m_cropActiveHandle != AreaSelectionItem::Handle::None) {
        QPointF delta = event->scenePos() - m_cropPressScenePos;
        QRectF newRect = m_cropPressRect;
        QRectF sr = sceneRect();
        qreal minSize = 4.0;

        switch (m_cropActiveHandle) {
        case AreaSelectionItem::Handle::Right:
            newRect.setRight(qBound(m_cropPressRect.left() + minSize, m_cropPressRect.right() + delta.x(), sr.right()));
            break;
        case AreaSelectionItem::Handle::Bottom:
            newRect.setBottom(qBound(m_cropPressRect.top() + minSize, m_cropPressRect.bottom() + delta.y(), sr.bottom()));
            break;
        case AreaSelectionItem::Handle::BottomRight:
            newRect.setRight(qBound(m_cropPressRect.left() + minSize, m_cropPressRect.right() + delta.x(), sr.right()));
            newRect.setBottom(qBound(m_cropPressRect.top() + minSize, m_cropPressRect.bottom() + delta.y(), sr.bottom()));
            break;
        case AreaSelectionItem::Handle::Left:
            newRect.setLeft(qBound(sr.left(), m_cropPressRect.left() + delta.x(), m_cropPressRect.right() - minSize));
            break;
        case AreaSelectionItem::Handle::Top:
            newRect.setTop(qBound(sr.top(), m_cropPressRect.top() + delta.y(), m_cropPressRect.bottom() - minSize));
            break;
        case AreaSelectionItem::Handle::TopLeft:
            newRect.setLeft(qBound(sr.left(), m_cropPressRect.left() + delta.x(), m_cropPressRect.right() - minSize));
            newRect.setTop(qBound(sr.top(), m_cropPressRect.top() + delta.y(), m_cropPressRect.bottom() - minSize));
            break;
        case AreaSelectionItem::Handle::TopRight:
            newRect.setRight(qBound(m_cropPressRect.left() + minSize, m_cropPressRect.right() + delta.x(), sr.right()));
            newRect.setTop(qBound(sr.top(), m_cropPressRect.top() + delta.y(), m_cropPressRect.bottom() - minSize));
            break;
        case AreaSelectionItem::Handle::BottomLeft:
            newRect.setLeft(qBound(sr.left(), m_cropPressRect.left() + delta.x(), m_cropPressRect.right() - minSize));
            newRect.setBottom(qBound(m_cropPressRect.top() + minSize, m_cropPressRect.bottom() + delta.y(), sr.bottom()));
            break;
        case AreaSelectionItem::Handle::Inside: {
            qreal w = m_cropPressRect.width();
            qreal h = m_cropPressRect.height();
            qreal newX = qBound(sr.left(), m_cropPressRect.left() + delta.x(), sr.right() - w);
            qreal newY = qBound(sr.top(), m_cropPressRect.top() + delta.y(), sr.bottom() - h);
            newRect = QRectF(newX, newY, w, h);
            break;
        }
        default:
            break;
        }

        m_selectedArea = newRect.normalized();
        if (m_areaSelectionItem) {
            m_areaSelectionItem->setRect(m_selectedArea);
        }
        emit areaSelectionChanged(m_selectedArea, true);
        event->accept();
        return;
    }

    // 2. Hover tracking for crop / selection handles
    if (!m_isDrawing && !m_isSelectingArea && hasAreaSelection() && (m_currentTool == ToolType::Crop || m_currentTool == ToolType::Select)) {
        if (m_areaSelectionItem && m_areaSelectionItem->isVisible()) {
            AreaSelectionItem::Handle h = m_areaSelectionItem->handleAt(event->scenePos());
            if (h != AreaSelectionItem::Handle::None) {
                for (auto* view : views()) {
                    view->setCursor(AreaSelectionItem::cursorForHandle(h));
                }
                event->accept();
                return;
            } else {
                for (auto* view : views()) {
                    if (auto* cv = qobject_cast<CanvasView*>(view)) {
                        cv->updateToolCursor();
                    }
                }
            }
        }
    }

    // 3. Handle hover tracking for canvas resize handles (only for Select or Pan tools)
    if (!m_isDrawing && !m_isSelectingArea && (m_currentTool == ToolType::Select || m_currentTool == ToolType::Pan)) {
        CanvasHandle h = handleAt(event->scenePos());
        if (h != m_hoveredHandle) {
            m_hoveredHandle = h;
            if (m_canvasFrameItem) m_canvasFrameItem->update();
        }
        if (h != CanvasHandle::None) {
            for (auto* view : views()) {
                view->setCursor(cursorForHandle(h));
            }
        } else if (!hasAreaSelection()) {
            for (auto* view : views()) {
                if (auto* cv = qobject_cast<CanvasView*>(view)) {
                    cv->updateToolCursor();
                }
            }
        }
    }

    if (m_currentTool == ToolType::Pan || m_currentTool == ToolType::ColorPicker) {
        event->accept();
        return;
    }

    if (m_isSelectingArea) {
        QPointF curPt = event->scenePos();
        QRectF sr = sceneRect();
        curPt.setX(qBound(sr.left(), curPt.x(), sr.right()));
        curPt.setY(qBound(sr.top(), curPt.y(), sr.bottom()));
        m_selectedArea = QRectF(m_startPoint, curPt).normalized();
        if (m_areaSelectionItem) {
            m_areaSelectionItem->setCanvasRect(sr);
            m_areaSelectionItem->setCropMode(m_currentTool == ToolType::Crop);
            m_areaSelectionItem->setRect(m_selectedArea);
        }
        emit areaSelectionChanged(m_selectedArea, true);
        return;
    }

    if (m_isDrawing && m_activeItem) {
        QPointF pt = event->scenePos();
        QRectF sr = sceneRect();
        pt.setX(qBound(sr.left(), pt.x(), sr.right()));
        pt.setY(qBound(sr.top(), pt.y(), sr.bottom()));
        updateActiveItem(pt);
        return;
    }

    QGraphicsScene::mouseMoveEvent(event);
}

void CanvasScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_currentTool == ToolType::ColorPicker) {
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (m_isResizingCanvas) {
            m_isResizingCanvas = false;
            QRectF guideRect = m_canvasResizeGuideItem->rect();
            m_canvasResizeGuideItem->setVisible(false);
            m_canvasResizeGuideItem->setRect(QRectF());
            m_activeHandle = CanvasHandle::None;
            if (m_canvasFrameItem) m_canvasFrameItem->update();

            if (guideRect.isValid() && (guideRect != m_resizeOriginalRect)) {
                resizeCanvas(guideRect);
            }

            for (auto* view : views()) {
                if (auto* cv = qobject_cast<CanvasView*>(view)) {
                    cv->updateToolCursor();
                }
            }
            event->accept();
            return;
        }

        if (m_cropActiveHandle != AreaSelectionItem::Handle::None) {
            m_cropActiveHandle = AreaSelectionItem::Handle::None;
            for (auto* view : views()) {
                if (auto* cv = qobject_cast<CanvasView*>(view)) {
                    cv->updateToolCursor();
                }
            }
            event->accept();
            return;
        }

        if (m_currentTool == ToolType::Pan) {
            event->accept();
            return;
        }

        if (m_isSelectingArea) {
            m_isSelectingArea = false;
            QPointF curPt = event->scenePos();
            QRectF sr = sceneRect();
            curPt.setX(qBound(sr.left(), curPt.x(), sr.right()));
            curPt.setY(qBound(sr.top(), curPt.y(), sr.bottom()));
            m_selectedArea = QRectF(m_startPoint, curPt).normalized();
            if (m_selectedArea.width() < 5 || m_selectedArea.height() < 5) {
                clearAreaSelection();
            } else {
                if (m_areaSelectionItem) {
                    m_areaSelectionItem->setCanvasRect(sr);
                    m_areaSelectionItem->setCropMode(m_currentTool == ToolType::Crop);
                    m_areaSelectionItem->setRect(m_selectedArea);
                }
                emit areaSelectionChanged(m_selectedArea, true);
            }
            return;
        }

        if (m_isDrawing) {
            m_isDrawing = false;
            finishActiveItem();
            emit toolActionCompleted();
            return;
        }
    }

    QGraphicsScene::mouseReleaseEvent(event);
}

void CanvasScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) {
    if (m_currentTool == ToolType::Crop && hasAreaSelection()) {
        if (m_selectedArea.contains(event->scenePos())) {
            cropToSelectedArea();
            event->accept();
            return;
        }
    }
    QGraphicsScene::mouseDoubleClickEvent(event);
}

void CanvasScene::keyPressEvent(QKeyEvent* event) {
    // 1. If any TextItem is currently active and editing text, forward all keyboard events directly to it
    for (auto* item : items()) {
        if (auto* txt = dynamic_cast<TextItem*>(item)) {
            if (txt->isEditing()) {
                if (focusItem() != txt) {
                    txt->setFocus();
                }
                QGraphicsScene::keyPressEvent(event);
                return;
            }
        }
    }

    if (m_currentTool == ToolType::Select && hasAreaSelection()) {
        if (event->matches(QKeySequence::Copy)) {
            copySelectedArea();
            event->accept();
            return;
        }
        if (event->matches(QKeySequence::Cut)) {
            cutSelectedArea();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
            deleteSelectedArea();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            cropToSelectedArea();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Escape) {
            clearAreaSelection();
            event->accept();
            return;
        }
    }

    if (m_currentTool == ToolType::Crop && hasAreaSelection()) {
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
            cropToSelectedArea();
            event->accept();
            return;
        }
        if (event->key() == Qt::Key_Escape) {
            clearAreaSelection();
            event->accept();
            return;
        }
    }

    // Paste image from clipboard
    if (event->matches(QKeySequence::Paste)) {
        QPixmap clipPix = ClipboardHelper::getClipboardImage();
        if (!clipPix.isNull()) {
            pasteImage(clipPix);
            event->accept();
            return;
        }
    }

    // Copy single selected PixmapItem
    if (event->matches(QKeySequence::Copy)) {
        auto sel = selectedItems();
        if (sel.count() == 1) {
            if (auto* pm = dynamic_cast<PixmapItem*>(sel.first())) {
                ClipboardHelper::copyImage(pm->pixmap());
                event->accept();
                return;
            }
        }
    }

    // Precision nudge selected annotation / pixmap items with Arrow keys
    if (!selectedItems().isEmpty() && !hasAreaSelection()) {
        qreal step = (event->modifiers() & Qt::ShiftModifier) ? 10.0 : 1.0;
        QPointF delta;
        if (event->key() == Qt::Key_Left) delta = QPointF(-step, 0);
        else if (event->key() == Qt::Key_Right) delta = QPointF(step, 0);
        else if (event->key() == Qt::Key_Up) delta = QPointF(0, -step);
        else if (event->key() == Qt::Key_Down) delta = QPointF(0, step);

        if (!delta.isNull()) {
            for (auto* item : selectedItems()) {
                if (!isSystemItem(item)) {
                    item->setPos(item->pos() + delta);
                }
            }
            emit sceneModified();
            event->accept();
            return;
        }
    }

    // Delete selected annotation items with Undo support
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        auto selItems = selectedItems();
        if (!selItems.isEmpty()) {
            m_undoStack.beginMacro("Delete Items");
            for (auto* item : selItems) {
                if (!isSystemItem(item)) {
                    m_undoStack.push(new RemoveItemCommand(this, item));
                }
            }
            m_undoStack.endMacro();
            emit sceneModified();
            event->accept();
            return;
        }
    }

    QGraphicsScene::keyPressEvent(event);
}

void CanvasScene::createNewItem(const QPointF& pos) {
    switch (m_currentTool) {
    case ToolType::Pen: {
        PenItem* pen = new PenItem(false);
        pen->setStrokeColor(m_strokeColor);
        pen->setStrokeWidth(m_strokeWidth);
        pen->addPoint(pos);
        m_activeItem = pen;
        break;
    }
    case ToolType::Highlighter: {
        PenItem* hl = new PenItem(true);
        QColor hlColor = m_strokeColor;
        if (hlColor.alpha() == 255) {
            if (hlColor == QColor(255, 30, 30)) {
                hlColor = QColor(255, 235, 59, 140);
            } else {
                hlColor.setAlpha(140);
            }
        }
        hl->setStrokeColor(hlColor);
        int w = m_strokeWidth < 10 ? 18 : m_strokeWidth;
        hl->setStrokeWidth(w);
        hl->addPoint(pos);
        m_activeItem = hl;
        break;
    }
    case ToolType::Line: {
        ArrowItem* line = new ArrowItem(ArrowMode::LineOnly);
        line->setStrokeColor(m_strokeColor);
        line->setStrokeWidth(m_strokeWidth);
        line->setEndpoints(pos, pos);
        m_activeItem = line;
        break;
    }
    case ToolType::Arrow: {
        ArrowItem* arrow = new ArrowItem(ArrowMode::SingleArrow);
        arrow->setStrokeColor(m_strokeColor);
        arrow->setStrokeWidth(m_strokeWidth);
        arrow->setEndpoints(pos, pos);
        m_activeItem = arrow;
        break;
    }
    case ToolType::DoubleArrow: {
        ArrowItem* darrow = new ArrowItem(ArrowMode::DoubleArrow);
        darrow->setStrokeColor(m_strokeColor);
        darrow->setStrokeWidth(m_strokeWidth);
        darrow->setEndpoints(pos, pos);
        m_activeItem = darrow;
        break;
    }
    case ToolType::Rectangle: {
        ShapeItem* rect = new ShapeItem(false);
        rect->setStrokeColor(m_strokeColor);
        rect->setFillColor(m_fillColor);
        rect->setStrokeWidth(m_strokeWidth);
        rect->setRect(QRectF(pos, pos));
        m_activeItem = rect;
        break;
    }
    case ToolType::Ellipse: {
        ShapeItem* ell = new ShapeItem(true);
        ell->setStrokeColor(m_strokeColor);
        ell->setFillColor(m_fillColor);
        ell->setStrokeWidth(m_strokeWidth);
        ell->setRect(QRectF(pos, pos));
        m_activeItem = ell;
        break;
    }
    case ToolType::Badge: {
        int num = m_badgeCounter++;
        BadgeItem* badge = new BadgeItem(num, pos);
        badge->setStrokeColor(m_strokeColor);
        badge->setFillColor(m_strokeColor);
        m_undoStack.push(new AddItemCommand(this, badge));
        emit badgeCounterChanged(m_badgeCounter);
        emit sceneModified();
        m_activeItem = nullptr;
        m_isDrawing = false;
        return;
    }
    case ToolType::Text: {
        TextItem* txt = new TextItem("", pos);
        txt->setStrokeColor(m_strokeColor);
        txt->setFillColor(m_fillColor);
        txt->setFont(m_font);
        txt->setInitialCreation(true);
        addItem(txt);
        connect(txt, &TextItem::initialCreationFinished, this, [this, txt](bool hasText) {
            if (!hasText) {
                if (txt->scene() == this) {
                    removeItem(txt);
                }
                txt->deleteLater();
            } else {
                m_undoStack.push(new AddItemCommand(this, txt, true));
                emit sceneModified();
            }
        });
        txt->startEditing();
        m_activeItem = nullptr;
        m_isDrawing = false;
        return;
    }
    case ToolType::Blur: {
        BlurItem* blur = new BlurItem(QRectF(pos, pos), m_basePixmapItem->pixmap(), m_blurLevel);
        m_activeItem = blur;
        break;
    }
    default:
        m_activeItem = nullptr;
        break;
    }

    if (m_activeItem) {
        addItem(m_activeItem);
    }
}

void CanvasScene::updateActiveItem(const QPointF& pos) {
    if (!m_activeItem) return;

    switch (m_currentTool) {
    case ToolType::Pen:
    case ToolType::Highlighter: {
        PenItem* pen = static_cast<PenItem*>(m_activeItem);
        pen->addPoint(pos);
        break;
    }
    case ToolType::Line:
    case ToolType::Arrow:
    case ToolType::DoubleArrow: {
        ArrowItem* arrow = static_cast<ArrowItem*>(m_activeItem);
        arrow->setEndpoints(m_startPoint, pos);
        break;
    }
    case ToolType::Rectangle:
    case ToolType::Ellipse: {
        ShapeItem* shape = static_cast<ShapeItem*>(m_activeItem);
        shape->setRect(QRectF(m_startPoint, pos).normalized());
        break;
    }
    case ToolType::Blur: {
        BlurItem* blur = static_cast<BlurItem*>(m_activeItem);
        blur->setRect(QRectF(m_startPoint, pos).normalized());
        blur->updateEffect(m_basePixmapItem->pixmap());
        break;
    }
    default:
        break;
    }
}

void CanvasScene::finishActiveItem() {
    if (!m_activeItem) return;

    if (auto* blur = dynamic_cast<BlurItem*>(m_activeItem)) {
        if (blur->rect().width() < 3 || blur->rect().height() < 3) {
            removeItem(m_activeItem);
            delete m_activeItem;
            m_activeItem = nullptr;
            return;
        }
    }

    // Add to undo stack
    removeItem(m_activeItem); // AddItemCommand will re-add and manage lifecycle
    m_undoStack.push(new AddItemCommand(this, m_activeItem));
    m_activeItem = nullptr;
    emit sceneModified();
}

void CanvasScene::setBadgeCounter(int n) {
    m_badgeCounter = qMax(1, n);
    emit badgeCounterChanged(m_badgeCounter);
}

void CanvasScene::resetBadgeCounter() {
    m_badgeCounter = 1;
    emit badgeCounterChanged(1);
}

void CanvasScene::modifyBadgeNumber(BadgeItem* badge, int newNumber) {
    if (!badge || badge->number() == newNumber) return;
    m_undoStack.push(new SetBadgeNumberCommand(this, badge, badge->number(), newNumber));
}

void CanvasScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* event) {
    if (m_currentTool == ToolType::ColorPicker) {
        event->accept();
        return;
    }

    QGraphicsScene::contextMenuEvent(event);
    if (event->isAccepted()) {
        return;
    }

    QPixmap clipPix = ClipboardHelper::getClipboardImage();
    QMenu menu;

    // Check if an existing annotation/pixmap item was clicked
    QGraphicsItem* clicked = itemAt(event->scenePos(), QTransform());
    bool clickedAnnotation = (clicked && !isSystemItem(clicked));

    // 1. Copy Actions: Right-clicking anywhere on the image allows copying!
    QAction* actCopy = nullptr;
    QAction* actCopyFull = nullptr;
    if (hasAreaSelection()) {
        actCopy = menu.addAction(IconManager::getIcon("copy"), tr("Copy Selected Area"));
        actCopyFull = menu.addAction(IconManager::getIcon("copy"), tr("Copy Full Image"));
    } else {
        actCopy = menu.addAction(IconManager::getIcon("copy"), tr("Copy Image"));
    }

    // 2. Paste Action (enabled if clipboard has an image)
    QAction* actPaste = nullptr;
    if (!clipPix.isNull()) {
        actPaste = menu.addAction(IconManager::getIcon("paste"), tr("Paste Image Here"));
    }

    // 3. Area selection actions (if active area selection)
    QAction* actCut = nullptr;
    QAction* actCrop = nullptr;
    QAction* actDelArea = nullptr;
    if (hasAreaSelection()) {
        menu.addSeparator();
        actCut = menu.addAction(tr("Cut Selected Area"));
        actCrop = menu.addAction(tr("Crop to Selected Area"));
        actDelArea = menu.addAction(tr("Delete Area"));
    }

    // 4. Item specific actions (if clicked an annotation item)
    QAction* actCopyItem = nullptr;
    QAction* actDelItem = nullptr;
    QAction* actFront = nullptr;
    QAction* actBack = nullptr;
    if (clickedAnnotation) {
        menu.addSeparator();
        if (auto* pm = dynamic_cast<PixmapItem*>(clicked)) {
            actCopyItem = menu.addAction(IconManager::getIcon("copy"), tr("Copy Selected Item"));
        }
        actDelItem = menu.addAction(tr("Delete Item"));
        actFront = menu.addAction(tr("Bring to Front"));
        actBack = menu.addAction(tr("Send to Back"));
    }

    // 5. Badge tool specific actions
    QAction* actResetBadge = nullptr;
    QAction* actSetBadge = nullptr;
    if (m_currentTool == ToolType::Badge) {
        menu.addSeparator();
        actResetBadge = menu.addAction(tr("Reset Badge Numbering to 1"));
        actSetBadge = menu.addAction(tr("Set Next Badge Number..."));
    }

    QAction* chosen = menu.exec(event->screenPos());
    if (!chosen) {
        event->accept();
        return;
    }

    if (chosen == actCopy) {
        if (hasAreaSelection()) {
            copySelectedArea();
        } else {
            copyActiveImage();
        }
        event->accept();
    } else if (actCopyFull && chosen == actCopyFull) {
        copyActiveImage();
        event->accept();
    } else if (actCopyItem && chosen == actCopyItem) {
        if (auto* pm = dynamic_cast<PixmapItem*>(clicked)) {
            QPixmap p = pm->pixmap();
            p.setDevicePixelRatio(1.0);
            if (ClipboardHelper::copyImage(p)) {
                emit statusMessageRequested(tr("Copied item to clipboard!"));
            }
            event->accept();
        }
    } else if (actPaste && chosen == actPaste) {
        QPointF pastePos = hasAreaSelection() ? m_selectedArea.topLeft() : event->scenePos();
        pasteImage(clipPix, pastePos);
        event->accept();
    } else if (actCut && chosen == actCut) {
        cutSelectedArea();
        event->accept();
    } else if (actCrop && chosen == actCrop) {
        cropToSelectedArea();
        event->accept();
    } else if (actDelArea && chosen == actDelArea) {
        deleteSelectedArea();
        event->accept();
    } else if (actDelItem && chosen == actDelItem) {
        m_undoStack.push(new RemoveItemCommand(this, clicked));
        emit sceneModified();
        event->accept();
    } else if (actFront && chosen == actFront) {
        qreal maxZ = 2.0;
        for (auto* it : items()) {
            if (!isSystemItem(it) && it->zValue() > maxZ) maxZ = it->zValue();
        }
        clicked->setZValue(maxZ + 1.0);
        emit sceneModified();
        event->accept();
    } else if (actBack && chosen == actBack) {
        clicked->setZValue(1.0);
        emit sceneModified();
        event->accept();
    } else if (actResetBadge && chosen == actResetBadge) {
        resetBadgeCounter();
        event->accept();
    } else if (actSetBadge && chosen == actSetBadge) {
        bool ok = false;
        int n = QInputDialog::getInt(nullptr, tr("Set Next Badge Number"), tr("Next Badge Number:"), m_badgeCounter, 1, 9999, 1, &ok);
        if (ok) {
            setBadgeCounter(n);
        }
        event->accept();
    }
}
