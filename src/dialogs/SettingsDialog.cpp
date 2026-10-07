#include "SettingsDialog.h"
#include "../core/SettingsManager.h"
#include "../core/HotkeyManager.h"
#include "../core/UpdateManager.h"
#include <QCoreApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFileDialog>
#include <QDialogButtonBox>
#include <QGroupBox>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings - S-Shot"));
    resize(500, 460);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    QTabWidget* tabs = new QTabWidget(this);

    // --- Tab 1: General ---
    QWidget* generalTab = new QWidget();
    QVBoxLayout* genLayout = new QVBoxLayout(generalTab);

    // Appearance Group
    QGroupBox* appGroup = new QGroupBox(tr("Appearance && Theme"), generalTab);
    QFormLayout* appForm = new QFormLayout(appGroup);
    m_themeCombo = new QComboBox(appGroup);
    m_themeCombo->addItem(tr("Dark"), "Dark");
    m_themeCombo->addItem(tr("Light (Grey UI)"), "Light");
    appForm->addRow(tr("Theme:"), m_themeCombo);
    genLayout->addWidget(appGroup);

    // Startup & Tray Group
    QGroupBox* startupGroup = new QGroupBox(tr("Startup && Tray"), generalTab);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    m_startWithPCCheck = new QCheckBox(tr("Start with PC (Launch automatically in system tray)"), startupGroup);
    m_runInTrayCheck = new QCheckBox(tr("Close editor to system tray instead of exiting"), startupGroup);
    startupLayout->addWidget(m_startWithPCCheck);
    startupLayout->addWidget(m_runInTrayCheck);
    genLayout->addWidget(startupGroup);

    // Save & Storage Group
    QGroupBox* saveGroup = new QGroupBox(tr("Save && Storage"), generalTab);
    QFormLayout* saveForm = new QFormLayout(saveGroup);
    QHBoxLayout* pathLayout = new QHBoxLayout();
    m_saveLocationEdit = new QLineEdit(saveGroup);
    m_browseLocationBtn = new QPushButton(tr("Browse..."), saveGroup);
    connect(m_browseLocationBtn, &QPushButton::clicked, this, &SettingsDialog::browseSaveLocation);
    pathLayout->addWidget(m_saveLocationEdit);
    pathLayout->addWidget(m_browseLocationBtn);
    saveForm->addRow(tr("Save Location:"), pathLayout);

    m_formatCombo = new QComboBox(saveGroup);
    m_formatCombo->addItems({"PNG", "JPG", "BMP", "WEBP"});
    saveForm->addRow(tr("Default Format:"), m_formatCombo);

    m_autoCopyCheck = new QCheckBox(tr("Automatically copy screenshot to clipboard"), saveGroup);
    saveForm->addRow("", m_autoCopyCheck);

    m_openEditorCheck = new QCheckBox(tr("Open editor automatically after screenshot capture"), saveGroup);
    saveForm->addRow("", m_openEditorCheck);

    genLayout->addWidget(saveGroup);

    // Software Updates Group
    QGroupBox* updateGroup = new QGroupBox(tr("Software Updates"), generalTab);
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);

    m_autoCheckUpdatesCheck = new QCheckBox(tr("Automatically check for updates on startup"), updateGroup);
    updateLayout->addWidget(m_autoCheckUpdatesCheck);

    QHBoxLayout* updateBtnsLayout = new QHBoxLayout();
    m_checkForUpdatesBtn = new QPushButton(tr("Check for Updates"), updateGroup);
    m_autoUpdateCheckBtn = new QPushButton(tr("Auto-Update Check"), updateGroup);
    updateBtnsLayout->addWidget(m_checkForUpdatesBtn);
    updateBtnsLayout->addWidget(m_autoUpdateCheckBtn);
    updateLayout->addLayout(updateBtnsLayout);

    m_updateStatusLabel = new QLabel(tr("Current version: %1").arg(QCoreApplication::applicationVersion()), updateGroup);
    m_updateStatusLabel->setStyleSheet("color: #888888; font-size: 11px;");
    updateLayout->addWidget(m_updateStatusLabel);

    connect(m_checkForUpdatesBtn, &QPushButton::clicked, this, &SettingsDialog::onCheckForUpdatesClicked);
    connect(m_autoUpdateCheckBtn, &QPushButton::clicked, this, &SettingsDialog::onAutoUpdateCheckClicked);

    connect(&UpdateManager::instance(), &UpdateManager::checkStarted, this, [this]() {
        m_updateStatusLabel->setText(tr("Checking for updates on GitHub..."));
        m_checkForUpdatesBtn->setEnabled(false);
        m_autoUpdateCheckBtn->setEnabled(false);
    });

    connect(&UpdateManager::instance(), &UpdateManager::checkFinished, this, [this](bool updateAvailable, const QString& latestVer, const QString&) {
        m_checkForUpdatesBtn->setEnabled(true);
        m_autoUpdateCheckBtn->setEnabled(true);
        if (updateAvailable) {
            m_updateStatusLabel->setText(tr("New version %1 available! (Current: %2)").arg(latestVer, QCoreApplication::applicationVersion()));
            m_updateStatusLabel->setStyleSheet("color: #30e500; font-size: 11px; font-weight: bold;");
        } else {
            m_updateStatusLabel->setText(tr("S-Shot is up to date (v%1).").arg(QCoreApplication::applicationVersion()));
            m_updateStatusLabel->setStyleSheet("color: #888888; font-size: 11px;");
        }
    });

    connect(&UpdateManager::instance(), &UpdateManager::checkError, this, [this](const QString& err) {
        m_checkForUpdatesBtn->setEnabled(true);
        m_autoUpdateCheckBtn->setEnabled(true);
        m_updateStatusLabel->setText(tr("Check failed: %1").arg(err));
        m_updateStatusLabel->setStyleSheet("color: #ff5555; font-size: 11px;");
    });

    genLayout->addWidget(updateGroup);
    genLayout->addStretch();
    tabs->addTab(generalTab, tr("General"));

    // --- Tab 2: Hotkeys ---
    QWidget* hotkeysTab = new QWidget();
    QFormLayout* hkForm = new QFormLayout(hotkeysTab);
    hkForm->setContentsMargins(15, 15, 15, 15);

    m_hotkeyFullscreenEdit = new QLineEdit(hotkeysTab);
    m_hotkeyRegionEdit = new QLineEdit(hotkeysTab);
    m_hotkeyScrollingEdit = new QLineEdit(hotkeysTab);
    m_hotkeyColorPickerEdit = new QLineEdit(hotkeysTab);
    m_hotkeyEditorEdit = new QLineEdit(hotkeysTab);

    hkForm->addRow(tr("Capture Fullscreen:"), m_hotkeyFullscreenEdit);
    hkForm->addRow(tr("Capture Selected Region:"), m_hotkeyRegionEdit);
    hkForm->addRow(tr("Capture Scrolling Window:"), m_hotkeyScrollingEdit);
    hkForm->addRow(tr("Colour Picker:"), m_hotkeyColorPickerEdit);
    hkForm->addRow(tr("Open Editor:"), m_hotkeyEditorEdit);

    tabs->addTab(hotkeysTab, tr("Hotkeys"));

    // --- Tab 3: Capture ---
    QWidget* captureTab = new QWidget();
    QFormLayout* capForm = new QFormLayout(captureTab);
    capForm->setContentsMargins(15, 15, 15, 15);

    m_delaySpin = new QSpinBox(captureTab);
    m_delaySpin->setRange(0, 30);
    m_delaySpin->setSuffix(tr(" sec"));
    capForm->addRow(tr("Capture Delay:"), m_delaySpin);

    m_magnifierCheck = new QCheckBox(tr("Show magnifier loupe during region capture"), captureTab);
    capForm->addRow("", m_magnifierCheck);

    tabs->addTab(captureTab, tr("Capture"));

    mainLayout->addWidget(tabs);

    // Button box
    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(bbox, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(bbox);

    loadSettings();
    applyDialogTheme(SettingsManager::instance().theme());
}

void SettingsDialog::applyDialogTheme(const QString& theme) {
    if (theme == "Light") {
        setStyleSheet(
            "QDialog { background-color: #ebebeb; color: #222222; }"
            "QGroupBox { font-weight: bold; border: 1px solid #cccccc; border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #222; }"
            "QLineEdit, QComboBox, QSpinBox { background-color: #ffffff; color: #222222; border: 1px solid #c0c0c0; border-radius: 4px; padding: 4px; }"
            "QPushButton { background-color: #dfdfdf; color: #222222; border: 1px solid #bfbfbf; border-radius: 4px; padding: 5px 12px; }"
            "QPushButton:hover { background-color: #d0d0d0; border-color: #30e500; }"
            "QTabBar::tab { background-color: #dddddd; color: #555555; padding: 6px 14px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
            "QTabBar::tab:selected { background-color: #ebebeb; color: #222; font-weight: bold; border-bottom: 2px solid #30e500; }"
        );
    } else {
        setStyleSheet(
            "QDialog { background-color: #2b2b2b; color: #ffffff; }"
            "QGroupBox { font-weight: bold; border: 1px solid #444444; border-radius: 5px; margin-top: 10px; padding-top: 10px; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 4px; color: #eee; }"
            "QLineEdit, QComboBox, QSpinBox { background-color: #383838; color: #ffffff; border: 1px solid #555555; border-radius: 4px; padding: 4px; }"
            "QPushButton { background-color: #3d3d3d; color: #ffffff; border: 1px solid #555555; border-radius: 4px; padding: 5px 12px; }"
            "QPushButton:hover { background-color: #4a4a4a; border-color: #30e500; }"
            "QTabBar::tab { background-color: #252525; color: #aaaaaa; padding: 6px 14px; border-top-left-radius: 4px; border-top-right-radius: 4px; }"
            "QTabBar::tab:selected { background-color: #383838; color: #30e500; font-weight: bold; border-bottom: 2px solid #30e500; }"
        );
    }
}

void SettingsDialog::loadSettings() {
    auto& s = SettingsManager::instance();
    int themeIdx = m_themeCombo->findData(s.theme());
    if (themeIdx >= 0) m_themeCombo->setCurrentIndex(themeIdx);

    m_startWithPCCheck->setChecked(s.startWithPC());
    m_runInTrayCheck->setChecked(s.runInTrayOnClose());
    m_saveLocationEdit->setText(s.saveLocation());
    m_formatCombo->setCurrentText(s.defaultFormat());
    m_autoCopyCheck->setChecked(s.autoCopyToClipboard());
    m_openEditorCheck->setChecked(s.openEditorAfterCapture());
    m_autoCheckUpdatesCheck->setChecked(s.autoCheckUpdates());

    m_hotkeyFullscreenEdit->setText(s.hotkeyFullscreen());
    m_hotkeyRegionEdit->setText(s.hotkeyRegion());
    m_hotkeyScrollingEdit->setText(s.hotkeyScrolling());
    m_hotkeyColorPickerEdit->setText(s.hotkeyColorPicker());
    m_hotkeyEditorEdit->setText(s.hotkeyEditor());

    m_delaySpin->setValue(s.captureDelay());
    m_magnifierCheck->setChecked(s.magnifierEnabled());
}

void SettingsDialog::browseSaveLocation() {
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Save Directory"), m_saveLocationEdit->text());
    if (!dir.isEmpty()) {
        m_saveLocationEdit->setText(dir);
    }
}

void SettingsDialog::saveSettings() {
    auto& s = SettingsManager::instance();
    s.setTheme(m_themeCombo->currentData().toString());
    s.setStartWithPC(m_startWithPCCheck->isChecked());
    s.setRunInTrayOnClose(m_runInTrayCheck->isChecked());
    s.setSaveLocation(m_saveLocationEdit->text());
    s.setDefaultFormat(m_formatCombo->currentText());
    s.setAutoCopyToClipboard(m_autoCopyCheck->isChecked());
    s.setOpenEditorAfterCapture(m_openEditorCheck->isChecked());
    s.setAutoCheckUpdates(m_autoCheckUpdatesCheck->isChecked());

    s.setHotkeyFullscreen(m_hotkeyFullscreenEdit->text());
    s.setHotkeyRegion(m_hotkeyRegionEdit->text());
    s.setHotkeyScrolling(m_hotkeyScrollingEdit->text());
    s.setHotkeyColorPicker(m_hotkeyColorPickerEdit->text());
    s.setHotkeyEditor(m_hotkeyEditorEdit->text());

    s.setCaptureDelay(m_delaySpin->value());
    s.setMagnifierEnabled(m_magnifierCheck->isChecked());

    s.save();
    HotkeyManager::instance().updateHotkeys();

    accept();
}

void SettingsDialog::onCheckForUpdatesClicked() {
    UpdateManager::instance().checkForUpdates(false, this);
}

void SettingsDialog::onAutoUpdateCheckClicked() {
    UpdateManager::instance().checkForUpdates(false, this);
}
