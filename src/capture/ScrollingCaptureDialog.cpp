#include "ScrollingCaptureDialog.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <cmath>

#if defined(Q_OS_LINUX)
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#endif

ScrollingCaptureDialog::ScrollingCaptureDialog(QWidget* parent)
    : QDialog(parent, Qt::Window | Qt::WindowStaysOnTopHint)
{
    setWindowTitle(tr("Scrolling Window Capture - S-Shot"));
    resize(460, 540);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_statusLabel = new QLabel(tr("Click 'Auto Scroll & Capture' or scroll and capture slices manually."), this);
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
    m_autoCaptureBtn = new QPushButton(tr("▶ Auto Scroll & Capture"), this);
    m_autoCaptureBtn->setStyleSheet("background-color: #2e7d32; color: white; padding: 8px 12px; font-weight: bold; border-radius: 4px;");
    connect(m_autoCaptureBtn, &QPushButton::clicked, this, [this]() {
        if (m_isAutoCapturing) {
            stopAutoCapture();
            finishCapture();
        } else {
            startAutoCapture();
        }
    });

    m_captureNextBtn = new QPushButton(tr("📸 Next Slice"), this);
    m_captureNextBtn->setStyleSheet("padding: 8px; border-radius: 4px;");
    connect(m_captureNextBtn, &QPushButton::clicked, this, &ScrollingCaptureDialog::captureNextSlice);

    m_finishBtn = new QPushButton(tr("✓ Finish & Edit"), this);
    m_finishBtn->setStyleSheet("background-color: #1976d2; color: white; padding: 8px 12px; font-weight: bold; border-radius: 4px;");
    connect(m_finishBtn, &QPushButton::clicked, this, &ScrollingCaptureDialog::finishCapture);

    m_cancelBtn = new QPushButton(tr("Cancel"), this);
    m_cancelBtn->setStyleSheet("padding: 8px; border-radius: 4px;");
    connect(m_cancelBtn, &QPushButton::clicked, this, &ScrollingCaptureDialog::discardCapture);

    btnLayout->addWidget(m_autoCaptureBtn);
    btnLayout->addWidget(m_captureNextBtn);
    btnLayout->addWidget(m_finishBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    m_autoTimer = new QTimer(this);
    connect(m_autoTimer, &QTimer::timeout, this, &ScrollingCaptureDialog::performAutoScrollStep);
}

void ScrollingCaptureDialog::startWithRegion(const QRect& region, const QPixmap& initialSlice) {
    m_region = region;
    m_slices.clear();
    m_lastSlice = initialSlice.toImage();
    m_slices.append(m_lastSlice);
    m_stitchedImage = m_lastSlice;
    m_isAutoCapturing = false;
    if (m_autoTimer->isActive()) m_autoTimer->stop();

    m_autoCaptureBtn->setText(tr("▶ Auto Scroll & Capture"));
    m_autoCaptureBtn->setStyleSheet("background-color: #2e7d32; color: white; padding: 8px 12px; font-weight: bold; border-radius: 4px;");
    m_statusLabel->setText(tr("Target region selected. Click 'Auto Scroll & Capture' to begin."));
    m_statusLabel->setStyleSheet("font-weight: bold; color: #30e500; font-size: 13px;");

    updatePreview();
    show();
    raise();
    activateWindow();
}

void ScrollingCaptureDialog::injectScrollDown(int steps) {
#if defined(Q_OS_LINUX)
    Display* display = XOpenDisplay(nullptr);
    if (!display) return;

    int cx = m_region.x() + m_region.width() / 2;
    int cy = m_region.y() + m_region.height() / 2;

    XTestFakeMotionEvent(display, -1, cx, cy, CurrentTime);
    XFlush(display);

    for (int i = 0; i < steps; ++i) {
        XTestFakeButtonEvent(display, 5, True, CurrentTime);
        XTestFakeButtonEvent(display, 5, False, CurrentTime);
    }
    XFlush(display);
    XCloseDisplay(display);
#else
    Q_UNUSED(steps);
#endif
}

void ScrollingCaptureDialog::startAutoCapture() {
    if (m_isAutoCapturing) return;
    m_isAutoCapturing = true;
    m_autoCaptureBtn->setText(tr("⏹ Stop & Finish"));
    m_autoCaptureBtn->setStyleSheet("background-color: #d32f2f; color: white; padding: 8px 12px; font-weight: bold; border-radius: 4px;");
    m_statusLabel->setText(tr("Auto-scrolling in progress... (Press Esc or click Stop to finish)"));
    m_statusLabel->setStyleSheet("font-weight: bold; color: #ffeb3b; font-size: 13px;");

    // Hide dialog so it doesn't obstruct target window
    hide();
    QGuiApplication::processEvents();

    m_autoTimer->start(250);
}

void ScrollingCaptureDialog::stopAutoCapture() {
    if (!m_isAutoCapturing) return;
    m_isAutoCapturing = false;
    m_autoTimer->stop();
    m_autoCaptureBtn->setText(tr("▶ Auto Scroll & Capture"));
    m_autoCaptureBtn->setStyleSheet("background-color: #2e7d32; color: white; padding: 8px 12px; font-weight: bold; border-radius: 4px;");
    show();
    updatePreview();
}

void ScrollingCaptureDialog::performAutoScrollStep() {
    if (!m_isAutoCapturing) return;

    injectScrollDown(4);
    QGuiApplication::processEvents();

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) {
        stopAutoCapture();
        finishCapture();
        return;
    }

    QPixmap grab = screen->grabWindow(0, m_region.x(), m_region.y(), m_region.width(), m_region.height());
    QImage newSlice = grab.toImage();

    // Check if bottom of page reached (duplicate slice)
    if (areSlicesIdentical(newSlice, m_lastSlice)) {
        stopAutoCapture();
        finishCapture();
        return;
    }

    m_stitchedImage = stitchImages(m_stitchedImage, newSlice);
    m_slices.append(newSlice);
    m_lastSlice = newSlice;

    if (m_slices.size() >= m_maxSlices) {
        stopAutoCapture();
        finishCapture();
    }
}

void ScrollingCaptureDialog::captureNextSlice() {
    hide();
    QGuiApplication::processEvents();

    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    QPixmap grab = screen->grabWindow(0, m_region.x(), m_region.y(), m_region.width(), m_region.height());
    QImage newSlice = grab.toImage();

    m_stitchedImage = stitchImages(m_stitchedImage, newSlice);
    m_slices.append(newSlice);
    m_lastSlice = newSlice;

    show();
    updatePreview();
}

bool ScrollingCaptureDialog::areSlicesIdentical(const QImage& a, const QImage& b) {
    if (a.isNull() || b.isNull()) return false;
    if (a.size() != b.size()) return false;

    int w = a.width();
    int h = a.height();
    double totalDiff = 0.0;
    int samples = 0;

    for (int y = 0; y < h; y += 8) {
        for (int x = 0; x < w; x += 8) {
            QRgb p1 = a.pixel(x, y);
            QRgb p2 = b.pixel(x, y);
            totalDiff += std::abs(qRed(p1) - qRed(p2)) +
                         std::abs(qGreen(p1) - qGreen(p2)) +
                         std::abs(qBlue(p1) - qBlue(p2));
            samples++;
        }
    }

    if (samples == 0) return true;
    double avgDiff = totalDiff / samples;
    return avgDiff < 3.0;
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
    if (m_isAutoCapturing) {
        stopAutoCapture();
    }
    hide();
    emit scrollingCaptureFinished(QPixmap::fromImage(m_stitchedImage));
}

void ScrollingCaptureDialog::discardCapture() {
    if (m_isAutoCapturing) {
        stopAutoCapture();
    }
    hide();
    m_slices.clear();
}

void ScrollingCaptureDialog::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape || event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
        if (m_isAutoCapturing) {
            stopAutoCapture();
            finishCapture();
        } else {
            finishCapture();
        }
        event->accept();
    } else {
        QDialog::keyPressEvent(event);
    }
}
