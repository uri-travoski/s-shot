#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>
#include <functional>

class MainWindow;

class TrayManager : public QObject {
    Q_OBJECT

public:
    explicit TrayManager(std::function<MainWindow*()> getMainWindowFunc, QObject* parent = nullptr);
    ~TrayManager();

    void showTrayIcon();
    void showMessage(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information, int timeout = 3000);

private slots:
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);
    void onOpenImage();
    void onOpenEditor();
    void onCaptureFullscreen();
    void onCaptureRegion();
    void onCaptureScrolling();
    void onColorPicker();
    void onOpenSettings();
    void onOpenAbout();
    void onQuit();

private:
    void createTrayMenu();
    MainWindow* mainWindow();

    std::function<MainWindow*()> m_getMainWindow;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
};
