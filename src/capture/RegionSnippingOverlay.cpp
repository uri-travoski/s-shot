#include "RegionSnippingOverlay.h"
#include "../core/SettingsManager.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainterPath>
#include <cmath>

RegionSnippingOverlay::RegionSnippingOverlay(QWidget* parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::BypassWindowManagerHint)
{
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void RegionSnippingOverlay::startSnipping() {
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect virtualGeo = screen->virtualGeometry();
    setGeometry(virtualGeo);

    // Grab complete virtual desktop
    WId rootWin = 0;
    m_screenGrab = screen->grabWindow(rootWin, virtualGeo.x(), virtualGeo.y(), virtualGeo.width(), virtualGeo.height());

    m_isSelecting = false;
    m_selectionDone = false;
    m_selectedRect = QRect();
    m_startPos = QPoint();
    m_currentPos = QCursor::pos() - virtualGeo.topLeft();

    showFullScreen();
    raise();
    activateWindow();
}

void RegionSnippingOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw original grabbed screen
    p.drawPixmap(0, 0, m_screenGrab);

    // Dim the entire screen
    p.fillRect(rect(), QColor(0, 0, 0, 110));

    // If there is an active selection, redraw that region clearly
    if (!m_selectedRect.isNull() && m_selectedRect.isValid()) {
        p.drawPixmap(m_selectedRect, m_screenGrab, m_selectedRect);

        // Selection border
        QPen borderPen(QColor(48, 229, 0), 2, Qt::SolidLine);
        p.setPen(borderPen);
        p.drawRect(m_selectedRect);

        // Dimensions badge
        QString dimText = QString("%1 × %2 px").arg(m_selectedRect.width()).arg(m_selectedRect.height());
        QFont font = p.font();
        font.setPixelSize(12);
        font.setBold(true);
        p.setFont(font);

        QFontMetrics fm(font);
        int textWidth = fm.horizontalAdvance(dimText) + 14;
        int textHeight = fm.height() + 8;

        int badgeX = m_selectedRect.left();
        int badgeY = m_selectedRect.top() - textHeight - 4;
        if (badgeY < 4) badgeY = m_selectedRect.bottom() + 4;
        if (badgeX + textWidth > width()) badgeX = width() - textWidth - 4;

        QRect badgeRect(badgeX, badgeY, textWidth, textHeight);
        p.setBrush(QColor(20, 20, 20, 210));
        p.setPen(QColor(48, 229, 0));
        p.drawRoundedRect(badgeRect, 4, 4);

        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, dimText);
    }

    // Draw magnifier loupe when selecting or hovering if enabled
    if (SettingsManager::instance().magnifierEnabled() && !m_selectionDone) {
        drawMagnifier(p, m_currentPos);
    }
}

void RegionSnippingOverlay::drawMagnifier(QPainter& p, const QPoint& pos) {
    int loupeSize = 130;
    int zoomFactor = 6;
    int srcSize = loupeSize / zoomFactor; // ~21 pixels

    // Position loupe offset from cursor
    int offsetX = 25;
    int offsetY = 25;
    if (pos.x() + offsetX + loupeSize > width()) {
        offsetX = -loupeSize - 25;
    }
    if (pos.y() + offsetY + loupeSize > height()) {
        offsetY = -loupeSize - 25;
    }

    QRect loupeRect(pos.x() + offsetX, pos.y() + offsetY, loupeSize, loupeSize);

    // Extract source pixel region
    QRect srcRect(pos.x() - srcSize / 2, pos.y() - srcSize / 2, srcSize, srcSize);
    QPixmap zoomPix = m_screenGrab.copy(srcRect).scaled(loupeSize, loupeSize, Qt::KeepAspectRatio, Qt::FastTransformation);

    // Draw loupe background and frame
    p.save();
    QPainterPath path;
    path.addRoundedRect(loupeRect, 8, 8);
    p.setClipPath(path);

    p.drawPixmap(loupeRect.topLeft(), zoomPix);

    // Center crosshair
    QPoint center = loupeRect.center();
    p.setPen(QPen(QColor(48, 229, 0, 200), 1));
    p.drawLine(center.x() - 10, center.y(), center.x() + 10, center.y());
    p.drawLine(center.x(), center.y() - 10, center.x(), center.y() + 10);
    p.restore();

    // Border
    p.setPen(QPen(QColor(48, 229, 0), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(loupeRect, 8, 8);

    // Color readout badge
    QColor curColor = m_screenGrab.toImage().pixelColor(qBound(0, pos.x(), m_screenGrab.width() - 1),
                                                         qBound(0, pos.y(), m_screenGrab.height() - 1));
    QString colText = curColor.name().toUpper();

    QRect badgeRect(loupeRect.left(), loupeRect.bottom() + 3, loupeRect.width(), 20);
    p.setBrush(QColor(15, 15, 15, 230));
    p.setPen(QPen(QColor(80, 80, 80), 1));
    p.drawRoundedRect(badgeRect, 3, 3);

    // Color preview swatch inside badge
    QRect swatch(badgeRect.left() + 4, badgeRect.top() + 4, 12, 12);
    p.fillRect(swatch, curColor);
    p.drawRect(swatch);

    QFont font = p.font();
    font.setPixelSize(10);
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::white);
    p.drawText(badgeRect.adjusted(20, 0, 0, 0), Qt::AlignCenter, QString("%1  (%2,%3)").arg(colText).arg(pos.x()).arg(pos.y()));
}

void RegionSnippingOverlay::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_startPos = event->pos();
        m_currentPos = event->pos();
        m_isSelecting = true;
        m_selectionDone = false;
        m_selectedRect = QRect(m_startPos, m_startPos);
        update();
    } else if (event->button() == Qt::RightButton) {
        // Cancel on right click
        hide();
        emit snippingCancelled();
    }
}

void RegionSnippingOverlay::mouseMoveEvent(QMouseEvent* event) {
    m_currentPos = event->pos();
    if (m_isSelecting) {
        m_selectedRect = QRect(m_startPos, m_currentPos).normalized();
    }
    update();
}

void RegionSnippingOverlay::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton && m_isSelecting) {
        m_isSelecting = false;
        m_currentPos = event->pos();
        m_selectedRect = QRect(m_startPos, m_currentPos).normalized();

        // Minimum threshold of 4x4 px to avoid accidental zero-size clicks
        if (m_selectedRect.width() > 4 && m_selectedRect.height() > 4) {
            m_selectionDone = true;
            hide();
            QPixmap cropped = m_screenGrab.copy(m_selectedRect);
            emit regionCaptured(cropped);
        } else {
            m_selectedRect = QRect();
            update();
        }
    }
}

void RegionSnippingOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        hide();
        emit snippingCancelled();
    } else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !m_selectedRect.isNull()) {
        hide();
        QPixmap cropped = m_screenGrab.copy(m_selectedRect);
        emit regionCaptured(cropped);
    }
}
