#include "ColorPickerOverlay.h"
#include <QGuiApplication>
#include <QScreen>
#include <QClipboard>
#include <QPainterPath>

ColorPickerOverlay::ColorPickerOverlay(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint)
{
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

    setCursor(Qt::BlankCursor);
    showFullScreen();
    raise();
    activateWindow();
}

void ColorPickerOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Render underlying screen
    p.drawPixmap(0, 0, m_screenGrab);

    const int K = 9;
    const int srcSpan = 2 * K + 1; // 19 pixels (exact center pixel at index 9)
    const int zoomFactor = 8;
    const int diameter = srcSpan * zoomFactor; // 152 pixels
    const int radius = diameter / 2; // 76 pixels

    // Center loupe directly on current cursor position
    QRect loupeRect(m_currentPos.x() - radius, m_currentPos.y() - radius, diameter, diameter);

    // Extract 19x19 source pixel region safely with boundary clipping
    QRect srcRect(m_currentPos.x() - K, m_currentPos.y() - K, srcSpan, srcSpan);
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
    p.setPen(QPen(QColor(255, 255, 255, 35), 1));
    for (int i = 0; i <= diameter; i += zoomFactor) {
        p.drawLine(loupeRect.left() + i, loupeRect.top(), loupeRect.left() + i, loupeRect.bottom());
        p.drawLine(loupeRect.left(), loupeRect.top() + i, loupeRect.right(), loupeRect.top() + i);
    }

    // 3. Precision reticle & center pixel highlight
    QRect centerPixelRect(m_currentPos.x() - zoomFactor / 2, m_currentPos.y() - zoomFactor / 2, zoomFactor, zoomFactor);

    p.setPen(QPen(QColor(48, 229, 0, 220), 1.5));
    // Horizontal crosshair lines
    p.drawLine(loupeRect.left(), m_currentPos.y(), centerPixelRect.left(), m_currentPos.y());
    p.drawLine(centerPixelRect.right() + 1, m_currentPos.y(), loupeRect.right(), m_currentPos.y());
    // Vertical crosshair lines
    p.drawLine(m_currentPos.x(), loupeRect.top(), m_currentPos.x(), centerPixelRect.top());
    p.drawLine(m_currentPos.x(), centerPixelRect.bottom() + 1, m_currentPos.x(), loupeRect.bottom());

    // Highlight center target pixel box
    p.setPen(QPen(QColor(48, 229, 0), 1.5));
    p.setBrush(Qt::NoBrush);
    p.drawRect(centerPixelRect);

    p.restore();

    // 4. Circular Bezel Ring
    p.setPen(QPen(QColor(0, 0, 0, 180), 3.5));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(loupeRect.adjusted(-1, -1, 1, 1));

    p.setPen(QPen(QColor(48, 229, 0), 2.5));
    p.drawEllipse(loupeRect);

    // 5. Read pixel color
    int px = qBound(0, m_currentPos.x(), m_screenGrab.width() - 1);
    int py = qBound(0, m_currentPos.y(), m_screenGrab.height() - 1);
    QColor color = m_screenGrab.toImage().pixelColor(px, py);
    QString hex = color.name(QColor::HexRgb).toUpper();
    QString rgbStr = QString("RGB(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());

    // 6. Smart Info Card
    int cardW = 160;
    int cardH = 46;
    int cardX = m_currentPos.x() - cardW / 2;
    if (cardX < 6) cardX = 6;
    if (cardX + cardW > width() - 6) cardX = width() - cardW - 6;

    int cardY = m_currentPos.y() + radius + 10;
    if (cardY + cardH > height() - 8) {
        cardY = m_currentPos.y() - radius - cardH - 10;
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
        int px = qBound(0, m_currentPos.x(), m_screenGrab.width() - 1);
        int py = qBound(0, m_currentPos.y(), m_screenGrab.height() - 1);
        QColor color = m_screenGrab.toImage().pixelColor(px, py);
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
