#include "ColorPickerOverlay.h"
#include <QGuiApplication>
#include <QScreen>
#include <QClipboard>
#include <QPainterPath>
#include <malloc.h>

ColorPickerOverlay::ColorPickerOverlay(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint)
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void ColorPickerOverlay::startPicking() {
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect virtualGeo = screen->virtualGeometry();
    setGeometry(virtualGeo);

    m_screenGrab = screen->grabWindow(0, virtualGeo.x(), virtualGeo.y(), virtualGeo.width(), virtualGeo.height());
    m_currentPos = QCursor::pos() - virtualGeo.topLeft();

    setCursor(Qt::CrossCursor);
    showFullScreen();
    raise();
    activateWindow();
}

void ColorPickerOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Render underlying screen
    p.drawPixmap(rect(), m_screenGrab);

    const int K = 9;
    const int srcSpan = 2 * K + 1; // 19 pixels (exact center pixel at index 9)
    const int zoomFactor = 8;
    const int diameter = srcSpan * zoomFactor; // 152 pixels
    const int radius = diameter / 2; // 76 pixels

    const qreal scaleX = (width() > 0 && m_screenGrab.width() > 0) ? (static_cast<qreal>(m_screenGrab.width()) / static_cast<qreal>(width())) : 1.0;
    const qreal scaleY = (height() > 0 && m_screenGrab.height() > 0) ? (static_cast<qreal>(m_screenGrab.height()) / static_cast<qreal>(height())) : 1.0;

    int physCenterX = qBound(0, static_cast<int>(std::round(m_currentPos.x() * scaleX)), m_screenGrab.width() - 1);
    int physCenterY = qBound(0, static_cast<int>(std::round(m_currentPos.y() * scaleY)), m_screenGrab.height() - 1);

    // Position loupe offset from the cross (cursor pos) - a few cm away next to it
    int offset = 32;
    int loupeX = m_currentPos.x() + offset;
    int loupeY = m_currentPos.y() + offset;

    int cardH = 46;
    int totalH = diameter + cardH + 20;

    // Flip horizontally if near right screen edge
    if (loupeX + diameter > width() - 10) {
        loupeX = m_currentPos.x() - offset - diameter;
    }
    // Flip vertically if near bottom screen edge
    if (loupeY + totalH > height() - 10) {
        loupeY = m_currentPos.y() - offset - diameter;
    }

    if (loupeX < 8) loupeX = 8;
    if (loupeY < 8) loupeY = 8;

    QRect loupeRect(loupeX, loupeY, diameter, diameter);

    // Extract 19x19 source pixel region centered directly at the physical pixel under the cross
    QRect srcRect(physCenterX - K, physCenterY - K, srcSpan, srcSpan);
    QRect validScreenRect = srcRect.intersected(m_screenGrab.rect());

    QImage srcImg(srcSpan, srcSpan, QImage::Format_ARGB32_Premultiplied);
    srcImg.fill(Qt::black);
    if (!validScreenRect.isEmpty()) {
        QImage grabbedSub = m_screenGrab.copy(validScreenRect).toImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);
        int destX = validScreenRect.left() - srcRect.left();
        int destY = validScreenRect.top() - srcRect.top();
        QPainter pImg(&srcImg);
        pImg.drawImage(destX, destY, grabbedSub);
    }
    QPixmap zoomed = QPixmap::fromImage(srcImg).scaled(diameter, diameter, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    // 1. Draw circular magnified view
    p.save();
    QPainterPath circlePath;
    circlePath.addEllipse(loupeRect);
    p.setClipPath(circlePath);

    p.drawPixmap(loupeRect.topLeft(), zoomed);

    // 2. Pixel grid
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    for (int i = 0; i <= diameter; i += zoomFactor) {
        p.drawLine(loupeRect.left() + i, loupeRect.top(), loupeRect.left() + i, loupeRect.bottom());
        p.drawLine(loupeRect.left(), loupeRect.top() + i, loupeRect.right(), loupeRect.top() + i);
    }

    // 3. Highlight center target pixel (exact pixel directly at the cross)
    int centerLoupeX = loupeRect.left() + radius;
    int centerLoupeY = loupeRect.top() + radius;
    QRect centerPixelRect(centerLoupeX - zoomFactor / 2, centerLoupeY - zoomFactor / 2, zoomFactor, zoomFactor);

    p.setPen(QPen(QColor(48, 229, 0, 220), 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRect(centerPixelRect);

    p.restore();

    // 4. Circular Bezel Ring
    p.setPen(QPen(QColor(0, 0, 0, 180), 3.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(loupeRect.adjusted(-1, -1, 1, 1));

    p.setPen(QPen(QColor(48, 229, 0), 2.5));
    p.drawEllipse(loupeRect);

    // 5. Read pixel color at the cross
    QColor color = m_screenGrab.toImage().pixelColor(physCenterX, physCenterY);
    QString hex = color.name(QColor::HexRgb).toUpper();
    QString rgbStr = QString("RGB(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());

    // 6. Smart Info Card (positioned below loupe)
    int cardW = 160;
    int cardX = loupeRect.center().x() - cardW / 2;
    if (cardX < 6) cardX = 6;
    if (cardX + cardW > width() - 6) cardX = width() - cardW - 6;

    int cardY = loupeRect.bottom() + 8;
    if (cardY + cardH > height() - 8) {
        cardY = loupeRect.top() - cardH - 8;
    }

    QRect infoRect(cardX, cardY, cardW, cardH);
    p.setBrush(QColor(20, 20, 20, 230));
    p.setPen(QPen(QColor(48, 229, 0), 1));
    p.drawRoundedRect(infoRect, 6, 6);

    // Color swatch
    QRect swatch(infoRect.left() + 8, infoRect.top() + 7, 32, 32);
    p.fillRect(swatch, color);
    p.setPen(QColor(200, 200, 200));
    p.drawRect(swatch);

    // Text labels
    QFont f = p.font();
    f.setPixelSize(11);
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(infoRect.left() + 48, infoRect.top() + 19, hex);

    f.setPixelSize(9);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(180, 180, 180));
    p.drawText(infoRect.left() + 48, infoRect.top() + 34, rgbStr);
}

void ColorPickerOverlay::mousePressEvent(QMouseEvent* event) {
    setCursor(Qt::ArrowCursor);
    if (event->button() == Qt::LeftButton) {
        const qreal scaleX = (width() > 0 && m_screenGrab.width() > 0) ? (static_cast<qreal>(m_screenGrab.width()) / static_cast<qreal>(width())) : 1.0;
        const qreal scaleY = (height() > 0 && m_screenGrab.height() > 0) ? (static_cast<qreal>(m_screenGrab.height()) / static_cast<qreal>(height())) : 1.0;

        int physCenterX = qBound(0, static_cast<int>(std::round(m_currentPos.x() * scaleX)), m_screenGrab.width() - 1);
        int physCenterY = qBound(0, static_cast<int>(std::round(m_currentPos.y() * scaleY)), m_screenGrab.height() - 1);

        QColor color = m_screenGrab.toImage().pixelColor(physCenterX, physCenterY);
        QString hex = color.name(QColor::HexRgb).toUpper();

        QClipboard* cb = QGuiApplication::clipboard();
        cb->setText(hex);

        hide();
        emit colorPicked(color, hex);
    } else {
        hide();
        emit pickingCancelled();
    }
}

void ColorPickerOverlay::mouseMoveEvent(QMouseEvent* event) {
    m_currentPos = event->pos();
    update();
}

void ColorPickerOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        setCursor(Qt::ArrowCursor);
        hide();
        emit pickingCancelled();
    }
}

void ColorPickerOverlay::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    m_screenGrab = QPixmap();
    malloc_trim(0);
}
