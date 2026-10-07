#include "RegionSnippingOverlay.h"
#include "../core/SettingsManager.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainterPath>
#include <cmath>
#include <malloc.h>

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

    setCursor(Qt::CrossCursor);

    showFullScreen();
    raise();
    activateWindow();
}

void RegionSnippingOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    // Draw original grabbed screen
    p.drawPixmap(rect(), m_screenGrab);

    // Dim the entire screen
    p.fillRect(rect(), QColor(0, 0, 0, 110));

    const qreal scaleX = (width() > 0 && m_screenGrab.width() > 0) ? (static_cast<qreal>(m_screenGrab.width()) / static_cast<qreal>(width())) : 1.0;
    const qreal scaleY = (height() > 0 && m_screenGrab.height() > 0) ? (static_cast<qreal>(m_screenGrab.height()) / static_cast<qreal>(height())) : 1.0;

    // If there is an active selection, redraw that region clearly
    if (!m_selectedRect.isNull() && m_selectedRect.isValid()) {
        QRect selPhys(
            qBound(0, static_cast<int>(std::round(m_selectedRect.x() * scaleX)), m_screenGrab.width()),
            qBound(0, static_cast<int>(std::round(m_selectedRect.y() * scaleY)), m_screenGrab.height()),
            qBound(0, static_cast<int>(std::round(m_selectedRect.width() * scaleX)), m_screenGrab.width()),
            qBound(0, static_cast<int>(std::round(m_selectedRect.height() * scaleY)), m_screenGrab.height())
        );
        p.drawPixmap(m_selectedRect, m_screenGrab, selPhys);

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
    const int K = 9;
    const int srcSpan = 2 * K + 1; // 19 pixels (exact center pixel at index 9)
    const int zoomFactor = 8;
    const int diameter = srcSpan * zoomFactor; // 152 pixels
    const int radius = diameter / 2; // 76 pixels

    const qreal scaleX = (width() > 0 && m_screenGrab.width() > 0) ? (static_cast<qreal>(m_screenGrab.width()) / static_cast<qreal>(width())) : 1.0;
    const qreal scaleY = (height() > 0 && m_screenGrab.height() > 0) ? (static_cast<qreal>(m_screenGrab.height()) / static_cast<qreal>(height())) : 1.0;

    int physCenterX = qBound(0, static_cast<int>(std::round(pos.x() * scaleX)), m_screenGrab.width() - 1);
    int physCenterY = qBound(0, static_cast<int>(std::round(pos.y() * scaleY)), m_screenGrab.height() - 1);

    // Position loupe offset from the cross (cursor pos) so the cross is outside the circle
    int offset = 28;
    int loupeX = pos.x() + offset;
    int loupeY = pos.y() + offset;

    // Flip horizontally if near right screen edge
    if (loupeX + diameter > width() - 10) {
        loupeX = pos.x() - offset - diameter;
    }
    // Flip vertically if near bottom screen edge (leaving space for badge)
    if (loupeY + diameter + 35 > height() - 10) {
        loupeY = pos.y() - offset - diameter;
    }

    if (loupeX < 8) loupeX = 8;
    if (loupeY < 8) loupeY = 8;

    QRect loupeRect(loupeX, loupeY, diameter, diameter);

    // Extract 19x19 source pixel region centered exactly on the physical pixel under the cross
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
    QPixmap zoomPix = QPixmap::fromImage(srcImg).scaled(diameter, diameter, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    // 1. Draw circular magnified view
    p.save();
    QPainterPath circlePath;
    circlePath.addEllipse(loupeRect);
    p.setClipPath(circlePath);

    p.drawPixmap(loupeRect.topLeft(), zoomPix);

    // 2. Pixel grid
    p.setPen(QPen(QColor(255, 255, 255, 30), 1));
    for (int i = 0; i <= diameter; i += zoomFactor) {
        p.drawLine(loupeRect.left() + i, loupeRect.top(), loupeRect.left() + i, loupeRect.bottom());
        p.drawLine(loupeRect.left(), loupeRect.top() + i, loupeRect.right(), loupeRect.top() + i);
    }

    // 3. Highlight the center target pixel (exact pixel directly at the cross)
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

    // 5. Smart Readout Badge (Color, Coordinates & Dimensions)
    QColor curColor = m_screenGrab.toImage().pixelColor(physCenterX, physCenterY);
    QString hexText = curColor.name(QColor::HexRgb).toUpper();

    QString infoText;
    if (m_isSelecting && !m_selectedRect.isNull()) {
        infoText = QString("%1  (%2, %3)  [%4 × %5]").arg(hexText).arg(pos.x()).arg(pos.y()).arg(m_selectedRect.width()).arg(m_selectedRect.height());
    } else {
        infoText = QString("%1  (%2, %3)").arg(hexText).arg(pos.x()).arg(pos.y());
    }

    QFont f = p.font();
    f.setPixelSize(11);
    f.setBold(true);
    p.setFont(f);
    QFontMetrics fm(f);
    int badgeW = fm.horizontalAdvance(infoText) + 30;
    int badgeH = 22;

    int badgeX = loupeRect.center().x() - badgeW / 2;
    if (badgeX < 6) badgeX = 6;
    if (badgeX + badgeW > width() - 6) badgeX = width() - badgeW - 6;

    int badgeY = loupeRect.bottom() + 8;
    if (badgeY + badgeH > height() - 8) {
        badgeY = loupeRect.top() - badgeH - 8;
    }

    QRect badgeRect(badgeX, badgeY, badgeW, badgeH);
    p.setBrush(QColor(20, 20, 20, 230));
    p.setPen(QPen(QColor(48, 229, 0), 1));
    p.drawRoundedRect(badgeRect, 4, 4);

    // Swatch inside badge
    QRect swatchRect(badgeRect.left() + 5, badgeRect.top() + 4, 14, 14);
    p.fillRect(swatchRect, curColor);
    p.setPen(QColor(180, 180, 180));
    p.drawRect(swatchRect);

    // Text
    p.setPen(Qt::white);
    p.drawText(badgeRect.adjusted(24, 0, -4, 0), Qt::AlignVCenter | Qt::AlignLeft, infoText);
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
        setCursor(Qt::ArrowCursor);
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
            setCursor(Qt::ArrowCursor);
            hide();
            const qreal scaleX = (width() > 0 && m_screenGrab.width() > 0) ? (static_cast<qreal>(m_screenGrab.width()) / static_cast<qreal>(width())) : 1.0;
            const qreal scaleY = (height() > 0 && m_screenGrab.height() > 0) ? (static_cast<qreal>(m_screenGrab.height()) / static_cast<qreal>(height())) : 1.0;
            QRect cropPhys(
                qBound(0, static_cast<int>(std::round(m_selectedRect.x() * scaleX)), m_screenGrab.width() - 1),
                qBound(0, static_cast<int>(std::round(m_selectedRect.y() * scaleY)), m_screenGrab.height() - 1),
                qBound(1, static_cast<int>(std::round(m_selectedRect.width() * scaleX)), m_screenGrab.width()),
                qBound(1, static_cast<int>(std::round(m_selectedRect.height() * scaleY)), m_screenGrab.height())
            );
            QPixmap cropped = m_screenGrab.copy(cropPhys.intersected(m_screenGrab.rect()));
            emit regionCaptured(cropped);
        } else {
            m_selectedRect = QRect();
            update();
        }
    }
}

void RegionSnippingOverlay::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        setCursor(Qt::ArrowCursor);
        hide();
        emit snippingCancelled();
    } else if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !m_selectedRect.isNull()) {
        setCursor(Qt::ArrowCursor);
        hide();
        QPixmap cropped = m_screenGrab.copy(m_selectedRect);
        emit regionCaptured(cropped);
    }
}

void RegionSnippingOverlay::hideEvent(QHideEvent* event) {
    QWidget::hideEvent(event);
    m_screenGrab = QPixmap();
    malloc_trim(0);
}
