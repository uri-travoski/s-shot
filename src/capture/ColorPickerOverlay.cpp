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

    showFullScreen();
    raise();
    activateWindow();
}

void ColorPickerOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Render underlying screen
    p.drawPixmap(0, 0, m_screenGrab);

    // Loupe size and settings
    int loupeSize = 140;
    int zoomFactor = 8;
    int srcSize = loupeSize / zoomFactor; // ~17px

    int offsetX = 25;
    int offsetY = 25;
    if (m_currentPos.x() + offsetX + loupeSize > width()) {
        offsetX = -loupeSize - 25;
    }
    if (m_currentPos.y() + offsetY + loupeSize + 40 > height()) {
        offsetY = -loupeSize - 65;
    }

    QRect loupeRect(m_currentPos.x() + offsetX, m_currentPos.y() + offsetY, loupeSize, loupeSize);

    // Copy source pixels around cursor
    QRect srcRect(m_currentPos.x() - srcSize / 2, m_currentPos.y() - srcSize / 2, srcSize, srcSize);
    QPixmap zoomed = m_screenGrab.copy(srcRect).scaled(loupeSize, loupeSize, Qt::KeepAspectRatio, Qt::FastTransformation);

    // Draw circular loupe
    p.save();
    QPainterPath circlePath;
    circlePath.addEllipse(loupeRect);
    p.setClipPath(circlePath);
    p.drawPixmap(loupeRect.topLeft(), zoomed);

    // Draw crosshair at center
    QPoint center = loupeRect.center();
    p.setPen(QPen(QColor(48, 229, 0), 1));
    p.drawRect(center.x() - zoomFactor / 2, center.y() - zoomFactor / 2, zoomFactor, zoomFactor);
    p.restore();

    // Loupe border
    p.setPen(QPen(QColor(48, 229, 0), 3));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(loupeRect);

    // Read pixel color
    int px = qBound(0, m_currentPos.x(), m_screenGrab.width() - 1);
    int py = qBound(0, m_currentPos.y(), m_screenGrab.height() - 1);
    QColor color = m_screenGrab.toImage().pixelColor(px, py);
    QString hex = color.name(QColor::HexRgb).toUpper();
    QString rgbStr = QString("RGB(%1, %2, %3)").arg(color.red()).arg(color.green()).arg(color.blue());

    // Info card below loupe
    QRect infoRect(loupeRect.left() - 10, loupeRect.bottom() + 8, loupeSize + 20, 48);
    p.setBrush(QColor(20, 20, 20, 230));
    p.setPen(QPen(QColor(70, 70, 70), 1));
    p.drawRoundedRect(infoRect, 6, 6);

    // Color swatch
    QRect swatch(infoRect.left() + 8, infoRect.top() + 8, 32, 32);
    p.fillRect(swatch, color);
    p.setPen(QColor(200, 200, 200));
    p.drawRect(swatch);

    // Text labels
    QFont f = p.font();
    f.setPixelSize(11);
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::white);
    p.drawText(infoRect.left() + 48, infoRect.top() + 20, hex);

    f.setPixelSize(9);
    f.setBold(false);
    p.setFont(f);
    p.setPen(QColor(180, 180, 180));
    p.drawText(infoRect.left() + 48, infoRect.top() + 36, rgbStr);
}

void ColorPickerOverlay::mousePressEvent(QMouseEvent* event) {
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
        hide();
        emit pickingCancelled();
    }
}
