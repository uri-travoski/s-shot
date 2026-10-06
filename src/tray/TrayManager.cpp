#include "TrayManager.h"
#include "../editor/MainWindow.h"
#include "../capture/CaptureManager.h"
#include "../dialogs/SettingsDialog.h"
#include "../dialogs/AboutDialog.h"
#include <QApplication>
#include <QIcon>
#include <QStyle>

TrayManager::TrayManager(std::function<MainWindow*()> getMainWindowFunc, QObject* parent)
    : QObject(parent)
    , m_getMainWindow(getMainWindowFunc)
{
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(QIcon(":/icons/s-shot.svg"));
    m_trayIcon->setToolTip("S-Shot - Screenshot & Annotation Tool");

    createTrayMenu();

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayActivated);
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
    m_trayIcon->show();
}

void TrayManager::showMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon, int timeout) {
    m_trayIcon->showMessage(title, message, icon, timeout);
}

void TrayManager::createTrayMenu() {
    m_trayMenu = new QMenu();
    m_trayMenu->setStyleSheet(
        "QMenu { background-color: #2b2b2b; color: #ffffff; border: 1px solid #444; padding: 4px; }"
        "QMenu::item { padding: 6px 24px 6px 24px; border-radius: 3px; }"
        "QMenu::item:selected { background-color: #2e7d32; color: #ffffff; }"
        "QMenu::separator { height: 1px; background-color: #444; margin: 4px 8px; }"
    );

    QAction* actOpen = m_trayMenu->addAction(QIcon(":/icons/open.svg"), tr("Open"), this, &TrayManager::onOpenImage);
    actOpen->setToolTip(tr("Open an existing image in annotation editor"));

    QAction* actEditor = m_trayMenu->addAction(QIcon(":/icons/new.svg"), tr("Editor"), this, &TrayManager::onOpenEditor);
    actEditor->setToolTip(tr("Open annotation editor"));

    m_trayMenu->addSeparator();

    m_trayMenu->addAction(tr("Capture Fullscreen"), this, &TrayManager::onCaptureFullscreen);
    m_trayMenu->addAction(tr("Capture Selected Region"), this, &TrayManager::onCaptureRegion);
    m_trayMenu->addAction(tr("Capture Scrolling Window"), this, &TrayManager::onCaptureScrolling);
    m_trayMenu->addAction(tr("Colour Picker"), this, &TrayManager::onColorPicker);

    m_trayMenu->addSeparator();

    m_trayMenu->addAction(tr("Settings"), this, &TrayManager::onOpenSettings);
    m_trayMenu->addAction(tr("About"), this, &TrayManager::onOpenAbout);

    m_trayMenu->addSeparator();

    m_trayMenu->addAction(tr("Quit"), this, &TrayManager::onQuit);

    m_trayIcon->setContextMenu(m_trayMenu);
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

void TrayManager::onCaptureScrolling() {
    CaptureManager::instance().captureScrollingWindow();
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
