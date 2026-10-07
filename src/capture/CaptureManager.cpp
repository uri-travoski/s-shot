#include "CaptureManager.h"
#include "RegionSnippingOverlay.h"
#include "ColorPickerOverlay.h"
#include "ScrollingCaptureDialog.h"
#include "../core/SettingsManager.h"
#include <QGuiApplication>
#include <QScreen>
#include <QClipboard>
#include <malloc.h>

CaptureManager& CaptureManager::instance() {
    static CaptureManager s_instance;
    return s_instance;
}

CaptureManager::CaptureManager() {
}

RegionSnippingOverlay* CaptureManager::regionOverlay() {
    if (!m_regionOverlay) {
        m_regionOverlay = new RegionSnippingOverlay();
        connect(m_regionOverlay, &RegionSnippingOverlay::regionCaptured, this, &CaptureManager::onRegionCaptured);
        connect(m_regionOverlay, &RegionSnippingOverlay::snippingCancelled, this, &CaptureManager::captureCancelled);
    }
    return m_regionOverlay;
}

ColorPickerOverlay* CaptureManager::colorOverlay() {
    if (!m_colorOverlay) {
        m_colorOverlay = new ColorPickerOverlay();
        connect(m_colorOverlay, &ColorPickerOverlay::colorPicked, this, &CaptureManager::onColorPicked);
        connect(m_colorOverlay, &ColorPickerOverlay::pickingCancelled, this, &CaptureManager::captureCancelled);
    }
    return m_colorOverlay;
}

ScrollingCaptureDialog* CaptureManager::scrollingDialog() {
    if (!m_scrollingDialog) {
        m_scrollingDialog = new ScrollingCaptureDialog();
        connect(m_scrollingDialog, &ScrollingCaptureDialog::scrollingCaptureFinished, this, &CaptureManager::onScrollingCaptured);
    }
    return m_scrollingDialog;
}

void CaptureManager::captureFullscreen() {
    int delay = SettingsManager::instance().captureDelay();
    if (delay > 0) {
        QTimer::singleShot(delay * 1000, this, &CaptureManager::doCaptureFullscreen);
    } else {
        doCaptureFullscreen();
    }
}

void CaptureManager::doCaptureFullscreen() {
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect virtualGeo = screen->virtualGeometry();
    QPixmap grab = screen->grabWindow(0, virtualGeo.x(), virtualGeo.y(), virtualGeo.width(), virtualGeo.height());

    if (SettingsManager::instance().autoCopyToClipboard()) {
        QGuiApplication::clipboard()->setPixmap(grab);
    }

    emit screenshotReady(grab);
}

void CaptureManager::captureRegion() {
    m_isSelectingForScrolling = false;
    int delay = SettingsManager::instance().captureDelay();
    if (delay > 0) {
        QTimer::singleShot(delay * 1000, this, [this]() {
            regionOverlay()->startSnipping();
        });
    } else {
        regionOverlay()->startSnipping();
    }
}

void CaptureManager::captureScrollingWindow() {
    m_isSelectingForScrolling = true;
    regionOverlay()->startSnipping();
}

void CaptureManager::pickColor() {
    colorOverlay()->startPicking();
}

void CaptureManager::onRegionCaptured(const QPixmap& pixmap) {
    if (m_isSelectingForScrolling) {
        m_isSelectingForScrolling = false;
        scrollingDialog()->startWithRegion(QRect(50, 50, pixmap.width(), pixmap.height()), pixmap);
        return;
    }

    if (SettingsManager::instance().autoCopyToClipboard()) {
        QGuiApplication::clipboard()->setPixmap(pixmap);
    }

    emit screenshotReady(pixmap);
}

void CaptureManager::onColorPicked(const QColor& color, const QString& hex) {
    emit colorPicked(color, hex);
}

void CaptureManager::onScrollingCaptured(const QPixmap& pixmap) {
    if (SettingsManager::instance().autoCopyToClipboard()) {
        QGuiApplication::clipboard()->setPixmap(pixmap);
    }

    emit screenshotReady(pixmap);
}
