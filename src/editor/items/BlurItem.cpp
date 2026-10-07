#include "BlurItem.h"
#include <QPainter>
#include <QPainterPath>
#include <QImage>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>

static void fastBoxBlur(QImage& img, int radius) {
    if (radius <= 0 || img.width() <= 2 || img.height() <= 2) return;

    int w = img.width();
    int h = img.height();
    int div = 2 * radius + 1;

    QImage temp = img;

    // Horizontal pass
    for (int y = 0; y < h; ++y) {
        const QRgb* srcLine = reinterpret_cast<const QRgb*>(img.constScanLine(y));
        QRgb* dstLine = reinterpret_cast<QRgb*>(temp.scanLine(y));

        int sumA = 0, sumR = 0, sumG = 0, sumB = 0;
        for (int i = -radius; i <= radius; ++i) {
            QRgb p = srcLine[qBound(0, i, w - 1)];
            sumA += qAlpha(p);
            sumR += qRed(p);
            sumG += qGreen(p);
            sumB += qBlue(p);
        }

        for (int x = 0; x < w; ++x) {
            dstLine[x] = qRgba(sumR / div, sumG / div, sumB / div, sumA / div);
            QRgb pOut = srcLine[qBound(0, x - radius, w - 1)];
            QRgb pIn  = srcLine[qBound(0, x + radius + 1, w - 1)];
            sumA += qAlpha(pIn) - qAlpha(pOut);
            sumR += qRed(pIn) - qRed(pOut);
            sumG += qGreen(pIn) - qGreen(pOut);
            sumB += qBlue(pIn) - qBlue(pOut);
        }
    }

    // Vertical pass
    for (int x = 0; x < w; ++x) {
        int sumA = 0, sumR = 0, sumG = 0, sumB = 0;
        for (int i = -radius; i <= radius; ++i) {
            int py = qBound(0, i, h - 1);
            QRgb p = *reinterpret_cast<const QRgb*>(temp.constScanLine(py) + x * 4);
            sumA += qAlpha(p);
            sumR += qRed(p);
            sumG += qGreen(p);
            sumB += qBlue(p);
        }

        for (int y = 0; y < h; ++y) {
            QRgb* dstPix = reinterpret_cast<QRgb*>(img.scanLine(y) + x * 4);
            *dstPix = qRgba(sumR / div, sumG / div, sumB / div, sumA / div);
            int yOut = qBound(0, y - radius, h - 1);
            int yIn  = qBound(0, y + radius + 1, h - 1);
            QRgb pOut = *reinterpret_cast<const QRgb*>(temp.constScanLine(yOut) + x * 4);
            QRgb pIn  = *reinterpret_cast<const QRgb*>(temp.constScanLine(yIn) + x * 4);
            sumA += qAlpha(pIn) - qAlpha(pOut);
            sumR += qRed(pIn) - qRed(pOut);
            sumG += qGreen(pIn) - qGreen(pOut);
            sumB += qBlue(pIn) - qBlue(pOut);
        }
    }
}

BlurItem::BlurItem(const QRectF& rect, const QPixmap& sourcePixmap, int blurLevel)
    : m_rect(rect)
    , m_blurLevel(qBound(1, blurLevel, 10))
{
    setFlag(ItemSendsGeometryChanges, true);
    m_strokeColor = QColor(48, 229, 0, 160);
    m_strokeWidth = 1;
    if (!sourcePixmap.isNull() && !m_rect.isNull()) {
        applyBlur(sourcePixmap);
    }
}

void BlurItem::setRect(const QRectF& r) {
    prepareGeometryChange();
    m_rect = r;
    update();
}

QPixmap BlurItem::getSourcePixmap() const {
    if (scene()) {
        const auto itemsList = scene()->items();
        for (QGraphicsItem* it : itemsList) {
            if (auto* pixItem = dynamic_cast<QGraphicsPixmapItem*>(it)) {
                if (pixItem->zValue() == -1000) {
                    return pixItem->pixmap();
                }
            }
        }
    }
    return m_sourceCache;
}

void BlurItem::setBlurLevel(int level) {
    m_blurLevel = qBound(1, level, 10);
    QPixmap src = getSourcePixmap();
    if (!src.isNull()) {
        applyBlur(src);
    }
    update();
}

void BlurItem::updateEffect(const QPixmap& sourcePixmap) {
    applyBlur(sourcePixmap);
    update();
}

QVariant BlurItem::itemChange(GraphicsItemChange change, const QVariant& value) {
    if (change == ItemPositionHasChanged) {
        QPixmap src = getSourcePixmap();
        if (!src.isNull()) {
            applyBlur(src);
        }
    } else if (change == ItemSceneHasChanged) {
        if (scene()) {
            m_sourceCache = QPixmap();
        }
    }
    return BaseAnnotationItem::itemChange(change, value);
}

void BlurItem::applyBlur(const QPixmap& sourcePixmap) {
    if (!scene()) {
        m_sourceCache = sourcePixmap;
    } else {
        m_sourceCache = QPixmap();
    }

    if (sourcePixmap.isNull()) {
        m_blurredPixmap = QPixmap();
        return;
    }

    QRectF sceneRectF = mapToScene(m_rect.normalized()).boundingRect();
    QRect sceneRect = sceneRectF.toRect();
    QRect clampedRect = sceneRect.intersected(sourcePixmap.rect());

    if (clampedRect.width() <= 1 || clampedRect.height() <= 1) {
        m_blurredPixmap = QPixmap();
        return;
    }

    QImage subImg = sourcePixmap.copy(clampedRect).toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
    int w = subImg.width();
    int h = subImg.height();

    int level = qBound(1, m_blurLevel, 10);
    int scaleFactor = (level >= 7) ? 4 : ((level >= 4) ? 2 : 1);

    QImage procImg;
    if (scaleFactor > 1 && w > scaleFactor * 2 && h > scaleFactor * 2) {
        procImg = subImg.scaled(w / scaleFactor, h / scaleFactor, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    } else {
        procImg = subImg;
    }

    int radius = level + 1;
    fastBoxBlur(procImg, radius);
    fastBoxBlur(procImg, radius);
    fastBoxBlur(procImg, radius);

    if (procImg.size() != subImg.size()) {
        procImg = procImg.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    m_blurredPixmap = QPixmap::fromImage(procImg);
}

QRectF BlurItem::boundingRect() const {
    return m_rect.normalized().adjusted(-2, -2, 2, 2);
}

QPainterPath BlurItem::shape() const {
    QPainterPath p;
    p.addRect(m_rect.normalized());
    return p;
}

void BlurItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*) {
    painter->save();
    QRectF r = m_rect.normalized();

    if (!m_blurredPixmap.isNull()) {
        painter->drawPixmap(r.topLeft(), m_blurredPixmap);
    } else {
        painter->fillRect(r, QColor(140, 140, 140, 120));
    }

    paintSelectionBorder(painter, r);
    painter->restore();
}
