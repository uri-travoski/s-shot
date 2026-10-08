#pragma once

#include <QObject>
#include <QString>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

class SettingsManager : public QObject {
    Q_OBJECT

public:
    static SettingsManager& instance();

    void load();
    void save();

    // General
    bool startWithPC() const { return m_startWithPC; }
    void setStartWithPC(bool val);

    QString saveLocation() const { return m_saveLocation; }
    void setSaveLocation(const QString& val) { m_saveLocation = val; }

    QString defaultFormat() const { return m_defaultFormat; }
    void setDefaultFormat(const QString& val) { m_defaultFormat = val; }

    bool autoCopyToClipboard() const { return m_autoCopyToClipboard; }
    void setAutoCopyToClipboard(bool val) { m_autoCopyToClipboard = val; }

    bool openEditorAfterCapture() const { return m_openEditorAfterCapture; }
    void setOpenEditorAfterCapture(bool val) { m_openEditorAfterCapture = val; }

    bool runInTrayOnClose() const { return m_runInTrayOnClose; }
    void setRunInTrayOnClose(bool val) { m_runInTrayOnClose = val; }

    bool autoCheckUpdates() const { return m_autoCheckUpdates; }
    void setAutoCheckUpdates(bool val) { m_autoCheckUpdates = val; }

    // Theme (Dark / Light (Grey UI))
    QString theme() const { return m_theme; }
    void setTheme(const QString& val) { m_theme = val; }

    // Hotkeys
    QString hotkeyFullscreen() const { return m_hotkeyFullscreen; }
    void setHotkeyFullscreen(const QString& val) { m_hotkeyFullscreen = val; }

    QString hotkeyRegion() const { return m_hotkeyRegion; }
    void setHotkeyRegion(const QString& val) { m_hotkeyRegion = val; }

    QString hotkeyColorPicker() const { return m_hotkeyColorPicker; }
    void setHotkeyColorPicker(const QString& val) { m_hotkeyColorPicker = val; }

    QString hotkeyEditor() const { return m_hotkeyEditor; }
    void setHotkeyEditor(const QString& val) { m_hotkeyEditor = val; }

    // Capture
    int captureDelay() const { return m_captureDelay; }
    void setCaptureDelay(int seconds) { m_captureDelay = seconds; }

    bool magnifierEnabled() const { return m_magnifierEnabled; }
    void setMagnifierEnabled(bool val) { m_magnifierEnabled = val; }

signals:
    void settingsChanged();
    void themeChanged(const QString& theme);

private:
    SettingsManager();
    void updateAutostartDesktopFile(bool enable);

    bool m_startWithPC = false;
    QString m_saveLocation;
    QString m_defaultFormat = "PNG";
    bool m_autoCopyToClipboard = true;
    bool m_openEditorAfterCapture = true;
    bool m_runInTrayOnClose = true;
    bool m_autoCheckUpdates = true;
    QString m_theme = "Light";

    QString m_hotkeyFullscreen = "Ctrl+Shift+Print";
    QString m_hotkeyRegion = "Ctrl+Print";
    QString m_hotkeyColorPicker = "Ctrl+Shift+C";
    QString m_hotkeyEditor = "Ctrl+Shift+E";

    int m_captureDelay = 0;
    bool m_magnifierEnabled = true;
};
