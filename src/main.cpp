#include <QApplication>
#include <QCommandLineParser>
#include <QLocalServer>
#include <QLocalSocket>
#include <QIcon>
#include <QDebug>
#include <QTimer>
#include <memory>
#include <malloc.h>

#include "core/SettingsManager.h"
#include "core/HotkeyManager.h"
#include "core/UpdateManager.h"
#include "core/IconManager.h"
#include "tray/TrayManager.h"
#include "capture/CaptureManager.h"
#include "editor/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("s-shot");
    app.setApplicationDisplayName("S-Shot");
    app.setApplicationVersion("1.24");
    app.setOrganizationName("S-Shot");
    app.setWindowIcon(IconManager::getAppIcon());

    const QString serverName = "s-shot-single-instance-socket";
    QLocalSocket socket;
    socket.connectToServer(serverName);
    if (socket.waitForConnected(500)) {
        QStringList args = app.arguments();
        args.removeFirst();
        QByteArray data = args.join(";").toUtf8();
        socket.write(data);
        socket.waitForBytesWritten(1000);
        return 0;
    }

    QLocalServer server;
    QLocalServer::removeServer(serverName);
    server.listen(serverName);

    QCommandLineParser parser;
    parser.setApplicationDescription("S-Shot: Lightweight Linux Screenshot & Annotation Tool");
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption trayOption("tray", "Start minimized in system tray.");
    parser.addOption(trayOption);

    QCommandLineOption fullscreenOption("fullscreen", "Capture fullscreen immediately.");
    parser.addOption(fullscreenOption);

    QCommandLineOption regionOption("region", "Capture selected region immediately.");
    parser.addOption(regionOption);

    QCommandLineOption editorOption("editor", "Open annotation editor.");
    parser.addOption(editorOption);

    QCommandLineOption colorpickerOption("colorpicker", "Pick color from screen immediately.");
    parser.addOption(colorpickerOption);

    QCommandLineOption settingsOption("settings", "Open settings dialog.");
    parser.addOption(settingsOption);

    parser.addPositionalArgument("file", "Image file to open", "[file]");
    parser.process(app);

    SettingsManager& settings = SettingsManager::instance();
    CaptureManager& captureMgr = CaptureManager::instance();

    std::unique_ptr<MainWindow> mainWindow;
    auto getMainWindow = [&mainWindow]() -> MainWindow* {
        if (!mainWindow) {
            mainWindow = std::make_unique<MainWindow>();
        }
        return mainWindow.get();
    };

    TrayManager trayManager(getMainWindow);
    HotkeyManager& hotkeys = HotkeyManager::instance();
    hotkeys.init();

    trayManager.showTrayIcon();

    // Connect capture signals to editor
    QObject::connect(&captureMgr, &CaptureManager::screenshotReady, [&getMainWindow, &settings](const QPixmap& pix) {
        MainWindow* win = getMainWindow();
        win->addImageTab(pix);
        if (settings.openEditorAfterCapture()) {
            win->show();
            win->raise();
            win->activateWindow();
        }
    });

    QObject::connect(&captureMgr, &CaptureManager::colorPicked, &trayManager, [&trayManager](const QColor&, const QString& hex) {
        trayManager.showMessage("Color Picked", QString("Copied %1 to clipboard!").arg(hex));
    });

    // Connect global hotkeys
    QObject::connect(&hotkeys, &HotkeyManager::hotkeyTriggered, [&captureMgr, &getMainWindow](HotkeyAction action) {
        switch (action) {
        case HotkeyAction::Fullscreen:
            captureMgr.captureFullscreen();
            break;
        case HotkeyAction::Region:
            captureMgr.captureRegion();
            break;
        case HotkeyAction::Scrolling:
            captureMgr.captureScrollingWindow();
            break;
        case HotkeyAction::ColorPicker:
            captureMgr.pickColor();
            break;
        case HotkeyAction::Editor: {
            MainWindow* win = getMainWindow();
            win->show();
            win->raise();
            win->activateWindow();
            break;
        }
        }
    });

    // Handle incoming commands from another instance
    QObject::connect(&server, &QLocalServer::newConnection, [&server, &captureMgr, &getMainWindow]() {
        QLocalSocket* client = server.nextPendingConnection();
        if (!client) return;
        QObject::connect(client, &QLocalSocket::readyRead, [client, &captureMgr, &getMainWindow]() {
            QString command = QString::fromUtf8(client->readAll());
            QStringList tokens = command.split(";", Qt::SkipEmptyParts);
            for (const QString& token : tokens) {
                if (token == "--fullscreen") {
                    captureMgr.captureFullscreen();
                } else if (token == "--region") {
                    captureMgr.captureRegion();
                } else if (token == "--colorpicker") {
                    captureMgr.pickColor();
                } else if (token == "--editor") {
                    MainWindow* win = getMainWindow();
                    win->show();
                    win->raise();
                    win->activateWindow();
                } else if (token == "--settings") {
                    MainWindow* win = getMainWindow();
                    win->show();
                    win->openSettingsDialog();
                } else if (!token.startsWith("--")) {
                    MainWindow* win = getMainWindow();
                    win->openImage(token);
                }
            }
        });
    });

    // Trim heap memory before starting event loop
    malloc_trim(0);

    const QStringList positionalArgs = parser.positionalArguments();
    if (!positionalArgs.isEmpty()) {
        MainWindow* win = getMainWindow();
        win->openImage(positionalArgs.first());
        win->show();
    } else if (parser.isSet(fullscreenOption)) {
        captureMgr.captureFullscreen();
    } else if (parser.isSet(regionOption)) {
        captureMgr.captureRegion();
    } else if (parser.isSet(colorpickerOption)) {
        captureMgr.pickColor();
    } else if (parser.isSet(editorOption)) {
        MainWindow* win = getMainWindow();
        win->show();
    } else if (parser.isSet(settingsOption)) {
        MainWindow* win = getMainWindow();
        win->show();
        win->openSettingsDialog();
    } else if (parser.isSet(trayOption)) {
        // Run purely in tray
    } else {
        // Default interactive start: open editor
        MainWindow* win = getMainWindow();
        win->show();
    }

    if (SettingsManager::instance().autoCheckUpdates()) {
        QTimer::singleShot(3000, []() {
            UpdateManager::instance().checkForUpdates(true /* silentIfUpToDate */);
        });
    }

    return app.exec();
}
