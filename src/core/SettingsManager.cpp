#include "SettingsManager.h"
#include <QCoreApplication>

SettingsManager& SettingsManager::instance() {
    static SettingsManager s_instance;
    return s_instance;
}

SettingsManager::SettingsManager() {
    QString picDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (picDir.isEmpty()) {
        picDir = QDir::homePath() + "/Pictures";
    }
    m_saveLocation = picDir;
    load();
}

void SettingsManager::load() {
    QString picDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    if (picDir.isEmpty()) {
        picDir = QDir::homePath() + "/Pictures";
    }

    QSettings s("s-shot", "s-shot");
    m_startWithPC = s.value("startWithPC", false).toBool();
    m_saveLocation = s.value("saveLocation", m_saveLocation).toString();
    if (m_saveLocation == picDir + "/Screenshots") {
        m_saveLocation = picDir;
    }
    m_defaultFormat = s.value("defaultFormat", "PNG").toString();
    m_autoCopyToClipboard = s.value("autoCopyToClipboard", true).toBool();
    m_openEditorAfterCapture = s.value("openEditorAfterCapture", true).toBool();
    m_runInTrayOnClose = s.value("runInTrayOnClose", true).toBool();
    m_theme = s.value("theme", "Light").toString();

    m_hotkeyFullscreen = s.value("hotkeyFullscreen", "Ctrl+Shift+Print").toString();
    m_hotkeyRegion = s.value("hotkeyRegion", "Ctrl+Print").toString();
    m_hotkeyScrolling = s.value("hotkeyScrolling", "Ctrl+Shift+S").toString();
    m_hotkeyColorPicker = s.value("hotkeyColorPicker", "Ctrl+Shift+C").toString();
    m_hotkeyEditor = s.value("hotkeyEditor", "Ctrl+Shift+E").toString();

    m_captureDelay = s.value("captureDelay", 0).toInt();
    m_magnifierEnabled = s.value("magnifierEnabled", true).toBool();
}

void SettingsManager::save() {
    QSettings s("s-shot", "s-shot");
    s.setValue("startWithPC", m_startWithPC);
    s.setValue("saveLocation", m_saveLocation);
    s.setValue("defaultFormat", m_defaultFormat);
    s.setValue("autoCopyToClipboard", m_autoCopyToClipboard);
    s.setValue("openEditorAfterCapture", m_openEditorAfterCapture);
    s.setValue("runInTrayOnClose", m_runInTrayOnClose);
    s.setValue("theme", m_theme);

    s.setValue("hotkeyFullscreen", m_hotkeyFullscreen);
    s.setValue("hotkeyRegion", m_hotkeyRegion);
    s.setValue("hotkeyScrolling", m_hotkeyScrolling);
    s.setValue("hotkeyColorPicker", m_hotkeyColorPicker);
    s.setValue("hotkeyEditor", m_hotkeyEditor);

    s.setValue("captureDelay", m_captureDelay);
    s.setValue("magnifierEnabled", m_magnifierEnabled);

    updateAutostartDesktopFile(m_startWithPC);
    emit settingsChanged();
    emit themeChanged(m_theme);
}

void SettingsManager::setStartWithPC(bool val) {
    m_startWithPC = val;
    updateAutostartDesktopFile(val);
}

void SettingsManager::updateAutostartDesktopFile(bool enable) {
    QString autostartDir = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart";
    QDir().mkpath(autostartDir);
    QString desktopFilePath = autostartDir + "/s-shot.desktop";

    if (enable) {
        QFile file(desktopFilePath);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "[Desktop Entry]\n";
            out << "Type=Application\n";
            out << "Name=S-Shot\n";
            out << "Comment=Lightweight Screenshot & Annotation Tool\n";
            out << "Exec=s-shot --tray\n";
            out << "Icon=s-shot\n";
            out << "Terminal=false\n";
            out << "Categories=Graphics;Utility;\n";
            out << "X-GNOME-Autostart-enabled=true\n";
        }
    } else {
        if (QFile::exists(desktopFilePath)) {
            QFile::remove(desktopFilePath);
        }
    }
}
