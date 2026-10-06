#include "ScrollingCaptureDialog.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <cmath>

ScrollingCaptureDialog::ScrollingCaptureDialog(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::WindowStaysOnTopHint)
{
    setWindowTitle(tr("Scrolling Window Capture - S-Shot"));
    resize(420, 520);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_statusLabel = new QLabel(tr("Scroll your window down, then click 'Capture Next Slice'."), this);
    m_statusLabel->setStyleSheet("font-weight: bold; color: #30e500; font-size: 13px;");
    mainLayout->addWidget(m_statusLabel);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("background-color: #222;");
    m_scrollArea->setWidget(m_previewLabel);
    mainLayout->addWidget(m_scrollArea, 1);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    m_captureNextBtn = new QPushButton(tr("📸 Capture Next Slice"), this);
    m_captureNextBtn->setStyleSheet("background-color: #2e7d32; color: white; padding: 8px; font-weight: bold; border-radius: 4px;");
    connect(m_captureNextBtn, &QPushButton::clicked, this, &ScrollingCaptureDialog::captureNextSlice);

    m_finishBtn = new QPushButton(tr("✓ Finish & Edit"), this);
    m_finishBtn->setStyleSheet("background-color: #1976d2; color: white; padding: 8px; font-weight: bold; border-radius: 4px;");
    connect(m_finishBtn, &QPushButton::clicked, this, &ScrollingCaptureDialog::finishCapture);

    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    m_cancelBtn->setStyleSheet("padding: 8px; border-radius: 4px;");
    connect(m_cancelBtn, &QPushButton::clicked, this, &ScrollingCaptureDialog::discardCapture);

    btnLayout->addWidget(m_captureNextBtn);
    btnLayout->addWidget(m_finishBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);
}

void ScrollingCaptureDialog::startWithRegion(const QRect& region, const QPixmap& initialSlice) {
    m_region = region;
    m_slices.clear();
    m_slices.append(initialSlice.toImage());
    m_stitchedImage = initialSlice.toImage();
    updatePreview();
    show();
    raise();
    activateWindow();
}

void ScrollingCaptureDialog::captureNextSlice() {
    hide();
    // Allow window to repaint underneath
    QGuiApplication::processEvents();

    QScreen* screen = QGuiApplication::primaryScreen();
    QPixmap grab = screen->grabWindow(0, m_region.x(), m_region.y(), m_region.width(), m_region.height());
    QImage newSlice = grab.toImage();

    m_stitchedImage = stitchImages(m_stitchedImage, newSlice);
    m_slices.append(newSlice);

    show();
    updatePreview();
}

QImage ScrollingCaptureDialog::stitchImages(const QImage& base, const QImage& next) {
    if (base.isNull()) return next;
    if (next.isNull()) return base;

    int width = base.width();
    int baseH = base.height();
    int nextH = next.height();

    // Check overlap: search for best overlap between bottom rows of base and top rows of next
    int maxOverlap = qMin(baseH, nextH) - 10;
    int bestOverlap = 0;
    double bestDiff = 1e9;

    // Scan overlap range
    for (int overlap = 20; overlap <= maxOverlap; overlap += 2) {
        double currentDiff = 0.0;
        int sampleStep = 5;
        int samples = 0;

        for (int y = 0; y < overlap; y += 4) {
            int baseY = baseH - overlap + y;
            int nextY = y;
            for (int x = 0; x < width; x += sampleStep) {
                QRgb p1 = base.pixel(x, baseY);
                QRgb p2 = next.pixel(x, nextY);
                int dr = qRed(p1) - qRed(p2);
                int dg = qGreen(p1) - qGreen(p2);
                int db = qBlue(p1) - qBlue(p2);
                currentDiff += (std::abs(dr) + std::abs(dg) + std::abs(db));
                samples++;
            }
        }

        if (samples > 0) {
            double avgDiff = currentDiff / samples;
            if (avgDiff < bestDiff) {
                bestDiff = avgDiff;
                bestOverlap = overlap;
            }
        }
    }

    // If a good overlap is found (threshold diff < 25), stitch using overlap
    int appendY = (bestDiff < 25.0 && bestOverlap > 10) ? bestOverlap : 0;
    int newHeight = baseH + (nextH - appendY);

    QImage result(width, newHeight, QImage::Format_ARGB32_Premultiplied);
    result.fill(Qt::transparent);

    QPainter p(&result);
    p.drawImage(0, 0, base);
    QRect nextSourceRect(0, appendY, width, nextH - appendY);
    p.drawImage(0, baseH, next, 0, appendY, width, nextH - appendY);
    p.end();

    return result;
}

void ScrollingCaptureDialog::updatePreview() {
    int count = m_slices.size();
    m_statusLabel->setText(tr("Slices captured: %1 | Height: %2 px").arg(count).arg(m_stitchedImage.height()));
    QPixmap previewPix = QPixmap::fromImage(m_stitchedImage);
    if (previewPix.width() > 360) {
        previewPix = previewPix.scaledToWidth(360, Qt::SmoothTransformation);
    }
    m_previewLabel->setPixmap(previewPix);
}

void ScrollingCaptureDialog::finishCapture() {
    hide();
    emit scrollingCaptureFinished(QPixmap::fromImage(m_stitchedImage));
}

void ScrollingCaptureDialog::discardCapture() {
    hide();
    m_slices.clear();
}
