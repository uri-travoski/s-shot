#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QTabWidget>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

private slots:
    void browseSaveLocation();
    void saveSettings();

private:
    void loadSettings();
    void applyDialogTheme(const QString& theme);

    // General tab
    QComboBox* m_themeCombo = nullptr;
    QCheckBox* m_startWithPCCheck = nullptr;
    QCheckBox* m_runInTrayCheck = nullptr;
    QLineEdit* m_saveLocationEdit = nullptr;
    QPushButton* m_browseLocationBtn = nullptr;
    QComboBox* m_formatCombo = nullptr;
    QCheckBox* m_autoCopyCheck = nullptr;
    QCheckBox* m_openEditorCheck = nullptr;

    // Hotkeys tab
    QLineEdit* m_hotkeyFullscreenEdit = nullptr;
    QLineEdit* m_hotkeyRegionEdit = nullptr;
    QLineEdit* m_hotkeyScrollingEdit = nullptr;
    QLineEdit* m_hotkeyColorPickerEdit = nullptr;
    QLineEdit* m_hotkeyEditorEdit = nullptr;

    // Capture tab
    QSpinBox* m_delaySpin = nullptr;
    QCheckBox* m_magnifierCheck = nullptr;
};
