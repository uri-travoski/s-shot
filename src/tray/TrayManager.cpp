#include "TrayManager.h"
#include "../editor/MainWindow.h"
#include "../capture/CaptureManager.h"
#include "../dialogs/SettingsDialog.h"
#include "../dialogs/AboutDialog.h"
#include "../core/SettingsManager.h"
#include "../core/UpdateManager.h"
#include "../core/IconManager.h"
#include <QApplication>
#include <QIcon>
#include <QStyle>

TrayManager::TrayManager(std::function<MainWindow*()> getMainWindowFunc, QObject* parent)
    : QObject(parent)
    , m_getMainWindow(getMainWindowFunc)
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(IconManager::getAppIcon());
    m_trayIcon->setToolTip("S-Shot - Screenshot & Annotation Tool");

    createTrayMenu();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayActivated);
    connect(&SettingsManager::instance(), &SettingsManager::themeChanged, this, &TrayManager::updateMenuTheme);
}

TrayManager::~TrayManager() {
    if (m_trayIcon) {
        m_trayIcon->hide();
    }
}

MainWindow* TrayManager::mainWindow() {
    return m_getMainWindow();
}

void TrayManager::showTrayIcon() {
    m_trayIcon->setIcon(IconManager::getAppIcon());
    m_trayIcon->show();
}

void TrayManager::showMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon, int timeout) {
    m_trayIcon->showMessage(title, message, icon, timeout);
}

void TrayManager::createTrayMenu() {
    m_trayMenu = new QMenu();

    // Group 1: Open & Editor (No icons in tray menu per user requirement)
    m_actOpen = m_trayMenu->addAction(tr("Open"), this, &TrayManager::onOpenImage);
    m_actOpen->setToolTip(tr("Open an existing image in annotation editor"));

    m_actEditor = m_trayMenu->addAction(tr("Editor"), this, &TrayManager::onOpenEditor);
    m_actEditor->setToolTip(tr("Open annotation editor"));

    // Divider 1
    m_trayMenu->addSeparator();

    // Group 2: Capture Actions
    m_actFullscreen = m_trayMenu->addAction(tr("Capture Fullscreen"), this, &TrayManager::onCaptureFullscreen);
    m_actRegion = m_trayMenu->addAction(tr("Capture Selected Region"), this, &TrayManager::onCaptureRegion);
    m_actColorPicker = m_trayMenu->addAction(tr("Colour Picker"), this, &TrayManager::onColorPicker);

    // Divider 2
    m_trayMenu->addSeparator();

    // Group 3: Settings & About
    m_actSettings = m_trayMenu->addAction(tr("Settings"), this, &TrayManager::onOpenSettings);
    m_trayMenu->addAction(tr("Check for Updates..."), this, []() {
        UpdateManager::instance().checkForUpdates(false);
    });
    m_actAbout = m_trayMenu->addAction(tr("About"), this, &TrayManager::onOpenAbout);

    // Divider 3
    m_trayMenu->addSeparator();

    // Group 4: Quit
    m_actQuit = m_trayMenu->addAction(tr("Quit"), this, &TrayManager::onQuit);

    m_trayIcon->setContextMenu(m_trayMenu);

    updateMenuTheme(SettingsManager::instance().theme());
}

void TrayManager::updateMenuTheme(const QString& theme) {
    bool isLight = (theme == "Light");

    if (m_trayMenu) {
        if (isLight) {
            m_trayMenu->setStyleSheet(
                "QMenu { background-color: #e2e2e2; color: #222222; border: 1px solid #b5b5b5; padding: 4px; }"
                "QMenu::item { padding: 6px 24px 6px 10px; border-radius: 4px; }"
                "QMenu::item:selected { background-color: #2e7d32; color: #ffffff; }"
                "QMenu::separator { height: 1px; background-color: #cccccc; margin: 4px 8px; }"
            );
        } else {
            m_trayMenu->setStyleSheet(
                "QMenu { background-color: #2b2b2b; color: #ffffff; border: 1px solid #444444; padding: 4px; }"
                "QMenu::item { padding: 6px 24px 6px 10px; border-radius: 4px; }"
                "QMenu::item:selected { background-color: #2e7d32; color: #ffffff; }"
                "QMenu::separator { height: 1px; background-color: #444444; margin: 4px 8px; }"
            );
        }
    }
}

void TrayManager::onTrayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        onOpenEditor();
    }
}

void TrayManager::onOpenImage() {
    MainWindow* w = mainWindow();
    w->show();
    w->raise();
    w->activateWindow();
    w->openFileDialog();
}

void TrayManager::onOpenEditor() {
    MainWindow* w = mainWindow();
    w->show();
    w->raise();
    w->activateWindow();
}

void TrayManager::onCaptureFullscreen() {
    CaptureManager::instance().captureFullscreen();
}

void TrayManager::onCaptureRegion() {
    CaptureManager::instance().captureRegion();
}

void TrayManager::onColorPicker() {
    CaptureManager::instance().pickColor();
}

void TrayManager::onOpenSettings() {
    mainWindow()->openSettingsDialog();
}

void TrayManager::onOpenAbout() {
    mainWindow()->openAboutDialog();
}

void TrayManager::onQuit() {
    qApp->quit();
}
