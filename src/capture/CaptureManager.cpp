#include "CaptureManager.h"
#include "RegionSnippingOverlay.h"
#include "ColorPickerOverlay.h"
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
    QRect virtualGeometry = screen->virtualGeometry();
    QPixmap grab = screen->grabWindow(0, virtualGeometry.x(), virtualGeometry.y(), virtualGeometry.width(), virtualGeometry.height());

    if (SettingsManager::instance().autoCopyToClipboard()) {
        QGuiApplication::clipboard()->setPixmap(grab);
    }

    emit screenshotReady(grab);
}

void CaptureManager::captureRegion() {
    int delay = SettingsManager::instance().captureDelay();
    if (delay > 0) {
        QTimer::singleShot(delay * 1000, this, [this]() {
            regionOverlay()->startSnipping();
        });
    } else {
        regionOverlay()->startSnipping();
    }
}

void CaptureManager::pickColor() {
    colorOverlay()->startPicking();
}

void CaptureManager::onRegionCaptured(const QPixmap& pixmap) {
    if (SettingsManager::instance().autoCopyToClipboard()) {
        QGuiApplication::clipboard()->setPixmap(pixmap);
    }

    emit screenshotReady(pixmap);
}

void CaptureManager::onColorPicked(const QColor& color, const QString& hex) {
    emit colorPicked(color, hex);
}
