#include "BlurItem.h"
#include <QPainter>
#include <QPainterPath>
#include <QImage>

BlurItem::BlurItem(const QRectF& rect, const QPixmap& sourcePixmap)
    : m_rect(rect)
{
    m_strokeColor = QColor(48, 229, 0, 160);
    m_strokeWidth = 1;
    if (!sourcePixmap.isNull() && !m_rect.isNull()) {
        updateEffect(sourcePixmap);
    }
}

void BlurItem::setRect(const QRectF& r) {
    prepareGeometryChange();
    m_rect = r;
    update();
}

void BlurItem::updateEffect(const QPixmap& sourcePixmap) {
    generatePixelatedPixmap(sourcePixmap);
    update();
}

void BlurItem::generatePixelatedPixmap(const QPixmap& sourcePixmap) {
    QRect r = m_rect.toRect().normalized();
    if (r.width() <= 0 || r.height() <= 0 || sourcePixmap.isNull()) {
        m_pixelatedPixmap = QPixmap();
        return;
    }

    // Intersect with source bounds
    QRect srcBounds(0, 0, sourcePixmap.width(), sourcePixmap.height());
    QRect clampedRect = r.intersected(srcBounds);
    if (clampedRect.isEmpty()) return;

    QImage subImg = sourcePixmap.copy(clampedRect).toImage().convertToFormat(QImage::Format_ARGB32);
    int w = subImg.width();
    int h = subImg.height();

    int bs = qMax(4, m_blockSize);

    // Pixelate block by block
    for (int y = 0; y < h; y += bs) {
        for (int x = 0; x < w; x += bs) {
            int blockW = qMin(bs, w - x);
            int blockH = qMin(bs, h - y);

            // Compute average color in block
            quint64 totalR = 0, totalG = 0, totalB = 0, count = 0;
            for (int by = 0; by < blockH; by += 2) {
                for (int bx = 0; bx < blockW; bx += 2) {
                    QRgb p = subImg.pixel(x + bx, y + by);
                    totalR += qRed(p);
                    totalG += qGreen(p);
                    totalB += qBlue(p);
                    count++;
                }
            }

            if (count > 0) {
                QRgb avgCol = qRgb(totalR / count, totalG / count, totalB / count);
                for (int by = 0; by < blockH; ++by) {
                    for (int bx = 0; bx < blockW; ++bx) {
                        subImg.setPixel(x + bx, y + by, avgCol);
                    }
                }
            }
        }
    }

    m_pixelatedPixmap = QPixmap::fromImage(subImg);
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

    if (!m_pixelatedPixmap.isNull()) {
        painter->drawPixmap(r.topLeft(), m_pixelatedPixmap);
    } else {
        painter->fillRect(r, QColor(100, 100, 100, 180));
    }

    // Border
    painter->setPen(QPen(m_strokeColor, 1.0, Qt::DashLine));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(r);

    paintSelectionBorder(painter, r);
    painter->restore();
}
