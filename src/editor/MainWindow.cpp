#include "MainWindow.h"
#include "../core/SettingsManager.h"
#include "../core/UpdateManager.h"
#include "../core/IconManager.h"
#include "../core/ClipboardHelper.h"
#include "../capture/CaptureManager.h"
#include "../dialogs/SettingsDialog.h"
#include "../dialogs/AboutDialog.h"
#include "../core/CrashHandler.h"
#include <QMenuBar>
#include <QMenu>
#include <QTextEdit>
#include <QDesktopServices>
#include <QUrl>
#include <QFontDatabase>
#include <QStatusBar>
#include <QFileDialog>
#include <QColorDialog>
#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>
#include <QDateTime>
#include <QDir>
#include <malloc.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("S-Shot");
    setWindowIcon(IconManager::getAppIcon());
    resize(1020, 720);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::onTabCloseRequested);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onCurrentTabChanged);

    m_newTabBtn = new QPushButton("+", this);
    m_newTabBtn->setToolTip(tr("Create new blank image"));
    connect(m_newTabBtn, &QPushButton::clicked, this, [this]() { createBlankTab(); });
    m_tabWidget->setCornerWidget(m_newTabBtn, Qt::TopRightCorner);

    setCentralWidget(m_tabWidget);

    setupMenus();
    setupToolbars();
    setupStatusBar();

    // Listen to theme changes from Settings
    connect(&SettingsManager::instance(), &SettingsManager::themeChanged, this, &MainWindow::applyTheme);

    // Apply currently selected theme
    applyTheme(SettingsManager::instance().theme());

    // Restore window if capture was cancelled
    connect(&CaptureManager::instance(), &CaptureManager::captureCancelled, this, [this]() {
        show();
        raise();
        activateWindow();
    });

    // Default blank tab on first open if no screenshot exists
    createBlankTab(800, 500);
}

MainWindow::~MainWindow() {
    if (m_tabWidget) {
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            if (auto* v = qobject_cast<CanvasView*>(m_tabWidget->widget(i))) {
                if (v->canvasScene()) {
                    v->canvasScene()->disconnect(this);
                    v->canvasScene()->clearSelection();
                }
            }
        }
    }
}

CanvasView* MainWindow::currentView() const {
    return qobject_cast<CanvasView*>(m_tabWidget->currentWidget());
}

CanvasScene* MainWindow::currentScene() const {
    CanvasView* v = currentView();
    return v ? v->canvasScene() : nullptr;
}

void MainWindow::applyTheme(const QString& theme) {
    bool isLight = (theme == "Light");

    if (isLight) {
        // Light Theme: Sleek Grey UI
        setStyleSheet(
            "QMainWindow { background-color: #e2e2e2; color: #222222; }"
            "QMenuBar { background-color: #d6d6d6; color: #222222; border-bottom: 1px solid #bfbfbf; }"
            "QMenuBar::item:selected { background-color: #c4c4c4; }"
            "QMenu { background-color: #eeeeee; color: #222222; border: 1px solid #b5b5b5; }"
            "QMenu::item:selected { background-color: #c4c4c4; color: #111111; }"
            "QMenu::separator { height: 1px; background-color: #cccccc; margin: 4px 8px; }"
            "QToolBar { background-color: #dcdcdc; border-bottom: 1px solid #cccccc; spacing: 4px; padding: 3px; }"
            "QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; color: #222222; }"
            "QToolButton:hover { background-color: #cccccc; border-color: #b0b0b0; }"
            "QToolButton:pressed { background-color: #b0b0b0; border: 1px solid #707070; padding-top: 5px; padding-left: 5px; padding-right: 3px; padding-bottom: 3px; }"
            "QToolButton:checked { background-color: #b8b8b8; border: 1px solid #999999; color: #111111; }"
            "QTabWidget::pane { border: none; background-color: #cccccc; }"
            "QTabBar::tab { background-color: #d0d0d0; color: #555555; padding: 8px 16px; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #ebebeb; color: #111111; font-weight: bold; border-bottom: 2px solid #666666; }"
            "QStatusBar { background-color: #d6d6d6; color: #333333; border-top: 1px solid #bfbfbf; }"
            "QLabel { color: #222222; }"
            "QSpinBox { background-color: #ffffff; color: #222222; border: 1px solid #b0b0b0; border-radius: 3px; padding: 2px 4px; }"
        );
        m_newTabBtn->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; background: transparent; color: #444444; border: none; padding: 4px 10px; } QPushButton:hover { background: #cccccc; border-radius: 4px; }");
        if (m_resetBadgeBtn) {
            m_resetBadgeBtn->setStyleSheet("QPushButton { padding: 2px 8px; border: 1px solid #b0b0b0; border-radius: 3px; font-size: 11px; font-weight: bold; background-color: #f0f0f0; color: #222222; } QPushButton:hover { background-color: #e0e0e0; border-color: #888888; } QPushButton:pressed { background-color: #d0d0d0; }");
        }
        if (m_fontBtn) {
            m_fontBtn->setStyleSheet("QPushButton { padding: 2px 8px; border: 1px solid #b0b0b0; border-radius: 3px; font-size: 11px; background-color: #f0f0f0; color: #222222; } QPushButton:hover { background-color: #e0e0e0; border-color: #888888; }");
        }
    } else {
        // Dark Theme: Neutral Sleek Dark UI
        setStyleSheet(
            "QMainWindow { background-color: #2b2b2b; color: #e0e0e0; }"
            "QMenuBar { background-color: #242424; color: #e0e0e0; border-bottom: 1px solid #3c3c3c; }"
            "QMenuBar::item:selected { background-color: #383838; }"
            "QMenu { background-color: #2c2c2c; color: #ffffff; border: 1px solid #444; }"
            "QMenu::item:selected { background-color: #484848; color: #ffffff; }"
            "QMenu::separator { height: 1px; background-color: #444; margin: 4px 8px; }"
            "QToolBar { background-color: #282828; border: none; spacing: 4px; padding: 3px; }"
            "QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; color: #e0e0e0; }"
            "QToolButton:hover { background-color: #383838; border-color: #555555; }"
            "QToolButton:pressed { background-color: #444444; border: 1px solid #30e500; padding-top: 5px; padding-left: 5px; padding-right: 3px; padding-bottom: 3px; }"
            "QToolButton:checked { background-color: #484848; border: 1px solid #666666; color: #ffffff; }"
            "QTabWidget::pane { border: none; background-color: #202020; }"
            "QTabBar::tab { background-color: #282828; color: #aaaaaa; padding: 8px 16px; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #383838; color: #ffffff; font-weight: bold; border-bottom: 2px solid #888888; }"
            "QStatusBar { background-color: #1e1e1e; color: #999999; border-top: 1px solid #333333; }"
            "QLabel { color: #e0e0e0; }"
            "QSpinBox { background-color: #383838; color: #ffffff; border: 1px solid #555555; border-radius: 3px; padding: 2px 4px; }"
        );
        m_newTabBtn->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; background: transparent; color: #cccccc; border: none; padding: 4px 10px; } QPushButton:hover { background: #383838; border-radius: 4px; }");
        if (m_resetBadgeBtn) {
            m_resetBadgeBtn->setStyleSheet("QPushButton { padding: 2px 8px; border: 1px solid #555555; border-radius: 3px; font-size: 11px; font-weight: bold; background-color: #383838; color: #ffffff; } QPushButton:hover { background-color: #484848; border-color: #777777; } QPushButton:pressed { background-color: #282828; }");
        }
        if (m_fontBtn) {
            m_fontBtn->setStyleSheet("QPushButton { padding: 2px 8px; border: 1px solid #555555; border-radius: 3px; font-size: 11px; background-color: #383838; color: #ffffff; } QPushButton:hover { background-color: #484848; border-color: #777777; }");
        }
    }

    // Update icons for all registered actions
    for (const auto& item : m_themedActions) {
        item.action->setIcon(IconManager::getIcon(item.iconName, isLight));
    }

    // Apply theme to all tabs
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CanvasView* v = qobject_cast<CanvasView*>(m_tabWidget->widget(i));
        if (v) {
            v->applyTheme(isLight);
        }
    }
}

void MainWindow::setupMenus() {
    QMenuBar* mb = menuBar();

    auto registerAct = [this](QAction* act, const QString& iconName) {
        m_themedActions.append({act, iconName});
    };

    // File Menu
    QMenu* fileMenu = mb->addMenu(tr("&File"));
    QAction* actNew = fileMenu->addAction(IconManager::getIcon("new"), tr("&New Tab"), QKeySequence::New, this, [this]() { createBlankTab(); });
    registerAct(actNew, "new");

    QAction* actOpen = fileMenu->addAction(IconManager::getIcon("open"), tr("&Open..."), QKeySequence::Open, this, &MainWindow::openFileDialog);
    registerAct(actOpen, "open");

    QAction* actSave = fileMenu->addAction(IconManager::getIcon("save"), tr("&Save"), QKeySequence::Save, this, &MainWindow::saveActiveTab);
    registerAct(actSave, "save");

    QAction* actSaveAs = fileMenu->addAction(IconManager::getIcon("save"), tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveActiveTabAs);
    registerAct(actSaveAs, "save");

    fileMenu->addSeparator();

    QAction* actCloseTray = fileMenu->addAction(IconManager::getIcon("s-shot"), tr("Close to &Tray"), this, &MainWindow::hide);
    registerAct(actCloseTray, "s-shot");

    fileMenu->addSeparator();

    QAction* actQuit = fileMenu->addAction(IconManager::getIcon("quit"), tr("&Quit S-Shot"), QKeySequence::Quit, qApp, &QCoreApplication::quit);
    registerAct(actQuit, "quit");

    // Edit Menu
    QMenu* editMenu = mb->addMenu(tr("&Edit"));
    QAction* actUndo = editMenu->addAction(IconManager::getIcon("undo"), tr("&Undo"), QKeySequence::Undo, this, &MainWindow::onUndo);
    registerAct(actUndo, "undo");

    QAction* actRedo = editMenu->addAction(IconManager::getIcon("redo"), tr("&Redo"), QKeySequence::Redo, this, &MainWindow::onRedo);
    registerAct(actRedo, "redo");

    editMenu->addSeparator();

    QAction* actCopy = editMenu->addAction(IconManager::getIcon("copy"), tr("&Copy Image"), QKeySequence::Copy, this, &MainWindow::copyActiveImageToClipboard);
    registerAct(actCopy, "copy");

    QAction* actPaste = editMenu->addAction(IconManager::getIcon("paste"), tr("&Paste"), QKeySequence::Paste, this, &MainWindow::pasteFromClipboard);
    registerAct(actPaste, "paste");

    editMenu->addSeparator();

    QAction* actResetBadge = editMenu->addAction(IconManager::getIcon("badge"), tr("Reset &Badge Numbering to 1"), this, &MainWindow::onResetBadgeCounter);
    registerAct(actResetBadge, "badge");

    // Capture Menu
    QMenu* capMenu = mb->addMenu(tr("&Capture"));
    QAction* actCapFull = capMenu->addAction(IconManager::getIcon("fullscreen"), tr("Capture &Fullscreen"), this, &MainWindow::onCaptureFullscreen);
    registerAct(actCapFull, "fullscreen");

    QAction* actCapRegion = capMenu->addAction(IconManager::getIcon("snip"), tr("Capture &Selected Region"), this, &MainWindow::onCaptureRegion);
    registerAct(actCapRegion, "snip");

    QAction* actCapColor = capMenu->addAction(IconManager::getIcon("picker"), tr("Colour &Picker"), this, &MainWindow::onColorPicker);
    registerAct(actCapColor, "picker");

    // View Menu
    QMenu* viewMenu = mb->addMenu(tr("&View"));
    QAction* actZoomIn = viewMenu->addAction(IconManager::getIcon("zoom_in"), tr("Zoom &In"), QKeySequence::ZoomIn, this, [this]() { if (currentView()) currentView()->zoomIn(); });
    registerAct(actZoomIn, "zoom_in");

    QAction* actZoomOut = viewMenu->addAction(IconManager::getIcon("zoom_out"), tr("Zoom &Out"), QKeySequence::ZoomOut, this, [this]() { if (currentView()) currentView()->zoomOut(); });
    registerAct(actZoomOut, "zoom_out");

    QAction* actZoom100 = viewMenu->addAction(IconManager::getIcon("zoom_100"), tr("&Actual Size (100%)"), this, [this]() { if (currentView()) currentView()->zoomActual(); });
    registerAct(actZoom100, "zoom_100");

    QAction* actZoomFit = viewMenu->addAction(IconManager::getIcon("zoom_fit"), tr("&Fit to Window"), this, [this]() { if (currentView()) currentView()->zoomFit(); });
    registerAct(actZoomFit, "zoom_fit");

    // Options Menu
    QMenu* optMenu = mb->addMenu(tr("&Options"));
    QAction* actSettings = optMenu->addAction(IconManager::getIcon("settings"), tr("&Settings..."), this, &MainWindow::openSettingsDialog);
    registerAct(actSettings, "settings");

    // Help Menu
    QMenu* helpMenu = mb->addMenu(tr("&Help"));
    QAction* actCheckUpdates = helpMenu->addAction(tr("Check for &Updates..."), this, [this]() {
        UpdateManager::instance().checkForUpdates(false, this);
    });
    QAction* actLogs = helpMenu->addAction(IconManager::getIcon("open"), tr("View &Logs / Crash Reports..."), this, &MainWindow::openLogViewerDialog);
    registerAct(actLogs, "open");
    QAction* actAbout = helpMenu->addAction(IconManager::getIcon("about"), tr("&About S-Shot"), this, &MainWindow::openAboutDialog);
    registerAct(actAbout, "about");
}

void MainWindow::setupToolbars() {
    auto registerAct = [this](QAction* act, const QString& iconName) {
        m_themedActions.append({act, iconName});
    };

    // 1. Top Main Toolbar
    m_mainToolBar = addToolBar(tr("Main Toolbar"));
    m_mainToolBar->setMovable(false);
    m_mainToolBar->setIconSize(QSize(20, 20));

    connect(m_mainToolBar, &QToolBar::actionTriggered, this, [this](QAction* act) {
        if (!act) return;
        QWidget* w = m_mainToolBar->widgetForAction(act);
        if (w) {
            bool isDark = (SettingsManager::instance().theme() != "Light");
            QString flashStyle = isDark 
                ? "background-color: #2e5c2e; border: 1px solid #30e500; border-radius: 4px;" 
                : "background-color: #cce4f7; border: 1px solid #0078d7; border-radius: 4px;";
            w->setStyleSheet(flashStyle);
            QTimer::singleShot(180, w, [w]() {
                w->setStyleSheet(QString());
            });
        }
    });

    QAction* tbNew = m_mainToolBar->addAction(IconManager::getIcon("new"), tr("New"), this, [this]() {
        createBlankTab();
        statusBar()->showMessage(tr("Created new canvas"), 2000);
    });
    registerAct(tbNew, "new");

    QAction* tbOpen = m_mainToolBar->addAction(IconManager::getIcon("open"), tr("Open"), this, &MainWindow::openFileDialog);
    registerAct(tbOpen, "open");

    QAction* tbSave = m_mainToolBar->addAction(IconManager::getIcon("save"), tr("Save"), this, &MainWindow::saveActiveTab);
    registerAct(tbSave, "save");

    QAction* tbCopy = m_mainToolBar->addAction(IconManager::getIcon("copy"), tr("Copy"), this, &MainWindow::copyActiveImageToClipboard);
    registerAct(tbCopy, "copy");

    m_mainToolBar->addSeparator();

    QAction* tbUndo = m_mainToolBar->addAction(IconManager::getIcon("undo"), tr("Undo"), this, &MainWindow::onUndo);
    registerAct(tbUndo, "undo");

    QAction* tbRedo = m_mainToolBar->addAction(IconManager::getIcon("redo"), tr("Redo"), this, &MainWindow::onRedo);
    registerAct(tbRedo, "redo");

    m_mainToolBar->addSeparator();

    QAction* tbZoomIn = m_mainToolBar->addAction(IconManager::getIcon("zoom_in"), tr("Zoom In"), this, [this]() {
        if (currentView()) {
            currentView()->zoomIn();
            statusBar()->showMessage(QString("Zoom: %1%").arg(qRound(currentView()->zoomFactor() * 100)), 2000);
        }
    });
    registerAct(tbZoomIn, "zoom_in");

    QAction* tbZoomOut = m_mainToolBar->addAction(IconManager::getIcon("zoom_out"), tr("Zoom Out"), this, [this]() {
        if (currentView()) {
            currentView()->zoomOut();
            statusBar()->showMessage(QString("Zoom: %1%").arg(qRound(currentView()->zoomFactor() * 100)), 2000);
        }
    });
    registerAct(tbZoomOut, "zoom_out");

    QAction* tbZoom100 = m_mainToolBar->addAction(IconManager::getIcon("zoom_100"), tr("100%"), this, [this]() {
        if (currentView()) {
            currentView()->zoomActual();
            statusBar()->showMessage(tr("Zoom: 100%"), 2000);
        }
    });
    registerAct(tbZoom100, "zoom_100");

    QAction* tbZoomFit = m_mainToolBar->addAction(IconManager::getIcon("zoom_fit"), tr("Fit"), this, [this]() {
        if (currentView()) {
            currentView()->zoomFit();
            statusBar()->showMessage(tr("Zoom: Fit"), 2000);
        }
    });
    registerAct(tbZoomFit, "zoom_fit");

    // 2. Property Toolbar (below main toolbar)
    addToolBarBreak();
    m_propToolBar = addToolBar(tr("Tool Properties"));
    m_propToolBar->setMovable(false);

    m_strokeLbl = new QLabel(tr(" Stroke: "), this);
    m_actStrokeLbl = m_propToolBar->addWidget(m_strokeLbl);

    m_strokeColorBtn = new QPushButton(this);
    m_strokeColorBtn->setFixedSize(26, 22);
    m_strokeColorBtn->setStyleSheet("background-color: #ff1e1e; border: 1px solid #888; border-radius: 3px;");
    connect(m_strokeColorBtn, &QPushButton::clicked, this, &MainWindow::onSelectStrokeColor);
    m_actStrokeColorBtn = m_propToolBar->addWidget(m_strokeColorBtn);

    m_fillLbl = new QLabel(tr("  Fill: "), this);
    m_actFillLbl = m_propToolBar->addWidget(m_fillLbl);

    m_fillColorBtn = new QPushButton(this);
    m_fillColorBtn->setFixedSize(26, 22);
    m_fillColorBtn->setText("Ø");
    m_fillColorBtn->setStyleSheet("background-color: #888; color: #eee; border: 1px solid #666; border-radius: 3px; font-weight: bold;");
    connect(m_fillColorBtn, &QPushButton::clicked, this, &MainWindow::onSelectFillColor);
    m_actFillColorBtn = m_propToolBar->addWidget(m_fillColorBtn);

    m_fontBtn = new QPushButton(tr("More..."), this);
    m_fontBtn->setStyleSheet("padding: 2px 6px; border: 1px solid #888; border-radius: 3px; font-size: 11px;");
    m_fontBtn->setToolTip(tr("Advanced font options"));
    connect(m_fontBtn, &QPushButton::clicked, this, &MainWindow::onSelectFont);
    m_actFontBtn = m_propToolBar->addWidget(m_fontBtn);

    m_fontFamilyCombo = new QFontComboBox(this);
    m_fontFamilyCombo->setCurrentFont(m_currentFont);
    m_fontFamilyCombo->setFixedWidth(160);
    m_fontFamilyCombo->setToolTip(tr("Font family"));
    connect(m_fontFamilyCombo, &QFontComboBox::currentFontChanged, this, &MainWindow::onFontFamilyChanged);
    m_actFontFamilyCombo = m_propToolBar->addWidget(m_fontFamilyCombo);

    m_fontSizeLbl = new QLabel(tr(" Size: "), this);
    m_actFontSizeLbl = m_propToolBar->addWidget(m_fontSizeLbl);

    m_fontSizeSpin = new QSpinBox(this);
    m_fontSizeSpin->setRange(6, 144);
    m_fontSizeSpin->setValue(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 11);
    m_fontSizeSpin->setFixedWidth(65);
    m_fontSizeSpin->setSuffix(" pt");
    m_fontSizeSpin->setToolTip(tr("Font point size"));
    connect(m_fontSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onFontSizeChanged);
    m_actFontSizeSpin = m_propToolBar->addWidget(m_fontSizeSpin);

    m_widthLbl = new QLabel(tr("  Width: "), this);
    m_actWidthLbl = m_propToolBar->addWidget(m_widthLbl);

    m_strokeWidthSpin = new QSpinBox(this);
    m_strokeWidthSpin->setRange(1, 50);
    m_strokeWidthSpin->setValue(3);
    m_strokeWidthSpin->setFixedWidth(60);
    connect(m_strokeWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onStrokeWidthChanged);
    m_actStrokeWidthSpin = m_propToolBar->addWidget(m_strokeWidthSpin);

    m_blurRadiusLbl = new QLabel(tr("  Blur (1-10): "), this);
    m_actBlurRadiusLbl = m_propToolBar->addWidget(m_blurRadiusLbl);

    m_blurRadiusSpin = new QSpinBox(this);
    m_blurRadiusSpin->setRange(1, 10);
    m_blurRadiusSpin->setValue(4);
    m_blurRadiusSpin->setFixedWidth(55);
    m_blurRadiusSpin->setToolTip(tr("Blur intensity from 1 (light) to 10 (heavy redaction)"));
    connect(m_blurRadiusSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBlurLevelChanged);
    m_actBlurRadiusSpin = m_propToolBar->addWidget(m_blurRadiusSpin);

    m_actBadgeSeparator = m_propToolBar->addSeparator();

    m_badgeNumberLbl = new QLabel(tr("  Next #: "), this);
    m_actBadgeNumberLbl = m_propToolBar->addWidget(m_badgeNumberLbl);

    m_badgeNumberSpin = new QSpinBox(this);
    m_badgeNumberSpin->setRange(1, 9999);
    m_badgeNumberSpin->setValue(1);
    m_badgeNumberSpin->setFixedWidth(60);
    m_badgeNumberSpin->setToolTip(tr("Next badge number counter (or number of selected badge)"));
    connect(m_badgeNumberSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBadgeNumberSpinChanged);
    m_actBadgeNumberSpin = m_propToolBar->addWidget(m_badgeNumberSpin);

    m_resetBadgeBtn = new QPushButton(tr("Reset Numbering (1)"), this);
    m_resetBadgeBtn->setToolTip(tr("Reset badge number counter to 1"));
    m_resetBadgeBtn->setStyleSheet("padding: 2px 8px; border: 1px solid #888; border-radius: 3px; font-size: 11px; font-weight: bold;");
    connect(m_resetBadgeBtn, &QPushButton::clicked, this, &MainWindow::onResetBadgeCounter);
    m_actResetBadgeBtn = m_propToolBar->addWidget(m_resetBadgeBtn);

    // 3. Left Vertical Toolbar (Annotation Tools - ksnip style)
    m_leftToolBar = new QToolBar(tr("Tools"), this);
    m_leftToolBar->setMovable(false);
    m_leftToolBar->setOrientation(Qt::Vertical);
    m_leftToolBar->setIconSize(QSize(24, 24));
    addToolBar(Qt::LeftToolBarArea, m_leftToolBar);

    m_toolActionGroup = new QActionGroup(this);
    m_toolActionGroup->setExclusive(true);
    connect(m_toolActionGroup, &QActionGroup::triggered, this, &MainWindow::onToolTriggered);

    auto addToolAct = [this, registerAct](const QString& iconName, const QString& title, ToolType t, bool check = false) -> QAction* {
        QAction* act = new QAction(IconManager::getIcon(iconName), title, this);
        act->setCheckable(true);
        act->setChecked(check);
        act->setData(static_cast<int>(t));
        m_toolActionGroup->addAction(act);
        m_leftToolBar->addAction(act);
        registerAct(act, iconName);
        return act;
    };

    m_actPan = addToolAct("hand", tr("Pan / Move Canvas (Hand Tool)"), ToolType::Pan, true);
    m_actSelect = addToolAct("select", tr("Select / Area Tool (Copy/Cut/Move/Delete/Crop)"), ToolType::Select);
    m_actText = addToolAct("text", tr("Text"), ToolType::Text);
    m_actPen = addToolAct("pen", tr("Pen (Freehand Drawing)"), ToolType::Pen);
    m_actHighlighter = addToolAct("highlighter", tr("Highlighter"), ToolType::Highlighter);
    m_actArrow = addToolAct("arrow", tr("Arrow"), ToolType::Arrow);
    m_actLine = addToolAct("line", tr("Line"), ToolType::Line);
    m_actDoubleArrow = addToolAct("double_arrow", tr("Double Arrow"), ToolType::DoubleArrow);
    m_actRect = addToolAct("rect", tr("Rectangle"), ToolType::Rectangle);
    m_actEllipse = addToolAct("ellipse", tr("Ellipse"), ToolType::Ellipse);
    m_actBadge = addToolAct("badge", tr("Number / Stepper Badge (1, 2, 3...)"), ToolType::Badge);
    m_actBlur = addToolAct("blur", tr("Blur / Pixelate Redaction"), ToolType::Blur);
    m_actBucket = addToolAct("bucket", tr("Fill Colour Bucket Tool"), ToolType::BucketFill);
    m_actCrop = addToolAct("crop", tr("Crop Tool"), ToolType::Crop);

    updateToolPropertiesVisibility(ToolType::Pan);
}

void MainWindow::setupStatusBar() {
    QStatusBar* sb = statusBar();
    m_statusDimensions = new QLabel(tr("0 × 0 px"), this);
    m_statusZoom = new QLabel(tr("100%"), this);
    m_statusCoords = new QLabel(tr("X: 0, Y: 0"), this);

    sb->addWidget(m_statusDimensions, 1);
    sb->addPermanentWidget(m_statusZoom);
    sb->addPermanentWidget(m_statusCoords);
}

void MainWindow::addImageTab(const QPixmap& pixmap, const QString& title) {
    if (pixmap.isNull() || pixmap.width() <= 0 || pixmap.height() <= 0) {
        return;
    }

    // If only 1 tab exists and it is the untouched initial Blank Canvas, replace it
    if (m_tabWidget->count() == 1 && m_tabWidget->tabText(0) == tr("Blank Canvas")) {
        CanvasView* firstView = qobject_cast<CanvasView*>(m_tabWidget->widget(0));
        if (firstView && firstView->canvasScene() && !firstView->canvasScene()->undoStack()->canUndo()) {
            m_tabWidget->removeTab(0);
            firstView->deleteLater();
        }
    }

    CanvasScene* scene = new CanvasScene(this);
    scene->setBasePixmap(pixmap);
    scene->setStrokeColor(m_currentStrokeColor);
    scene->setFillColor(m_currentFillColor);
    scene->setStrokeWidth(m_currentStrokeWidth);
    scene->setBlurLevel(m_currentBlurLevel);
    scene->setCurrentFont(m_currentFont);
    connect(scene, &QGraphicsScene::selectionChanged, this, &MainWindow::onSceneSelectionChanged);
    connect(scene, &CanvasScene::badgeCounterChanged, this, &MainWindow::onBadgeCounterChanged);
    connect(scene, &CanvasScene::sceneModified, this, [this, scene]() {
        QPixmap p = scene->basePixmap();
        if (m_statusDimensions) {
            m_statusDimensions->setText(QString("%1 × %2 px").arg(p.width()).arg(p.height()));
        }
    });

    QAction* activeAct = m_toolActionGroup->checkedAction();
    if (activeAct) {
        scene->setCurrentTool(static_cast<ToolType>(activeAct->data().toInt()));
    }

    CanvasView* view = new CanvasView(scene, this);
    view->applyTheme(SettingsManager::instance().theme() == "Light");
    connect(view, &CanvasView::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(view, &CanvasView::mouseMovedTo, this, &MainWindow::onCursorMoved);
    view->updateToolCursor();

    QString tabTitle = title.isEmpty() ? QString("Capture %1").arg(m_tabWidget->count() + 1) : title;
    int idx = m_tabWidget->addTab(view, tabTitle);
    m_tabWidget->setCurrentIndex(idx);

    show();
    raise();
    activateWindow();
    QTimer::singleShot(0, view, &CanvasView::zoomFit);
}

void MainWindow::createBlankTab(int width, int height) {
    QPixmap pix(width, height);
    pix.fill(Qt::white);
    addImageTab(pix, tr("Blank Canvas"));
}

void MainWindow::openFileDialog() {
    QString path = QFileDialog::getOpenFileName(this, tr("Open Image"), "", tr("Images (*.png *.jpg *.jpeg *.bmp *.webp);;All Files (*)"));
    if (!path.isEmpty()) {
        openImage(path);
    }
}

void MainWindow::openImage(const QString& filePath) {
    QPixmap pix;
    if (pix.load(filePath)) {
        QFileInfo fi(filePath);
        addImageTab(pix, fi.fileName());
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Could not open image file: %1").arg(filePath));
    }
}

void MainWindow::pasteFromClipboard() {
    const QClipboard* cb = QGuiApplication::clipboard();
    QPixmap pix = cb->pixmap();
    if (!pix.isNull()) {
        addImageTab(pix, tr("Pasted Image"));
    }
}

void MainWindow::saveActiveTab() {
    CanvasScene* scene = currentScene();
    if (!scene) return;

    QString saveDir = SettingsManager::instance().saveLocation();
    QDir().mkpath(saveDir);

    QString ext = SettingsManager::instance().defaultFormat().toLower();
    QString fileName = QString("Screenshot_%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")).arg(ext);
    QString fullPath = saveDir + "/" + fileName;

    QPixmap outPix = scene->renderToPixmap();
    if (outPix.save(fullPath)) {
        statusBar()->showMessage(tr("Saved to %1").arg(fullPath), 3000);
        m_tabWidget->setTabText(m_tabWidget->currentIndex(), fileName);
    } else {
        saveActiveTabAs();
    }
}

void MainWindow::saveActiveTabAs() {
    CanvasScene* scene = currentScene();
    if (!scene) return;

    QString saveDir = SettingsManager::instance().saveLocation();
    QString ext = SettingsManager::instance().defaultFormat().toLower();
    QString filter = QString("%1 (*.%2);;All Files (*)").arg(ext.toUpper()).arg(ext);

    QString path = QFileDialog::getSaveFileName(this, tr("Save Screenshot As"), saveDir, filter);
    if (!path.isEmpty()) {
        QPixmap outPix = scene->renderToPixmap();
        if (outPix.save(path)) {
            QFileInfo fi(path);
            statusBar()->showMessage(tr("Saved to %1").arg(path), 3000);
            m_tabWidget->setTabText(m_tabWidget->currentIndex(), fi.fileName());
        }
    }
}

void MainWindow::copyActiveImageToClipboard() {
    CanvasScene* scene = currentScene();
    if (!scene) return;

    QPixmap outPix;
    if (scene->hasAreaSelection()) {
        QRect cropRect = scene->selectedArea().toRect().intersected(scene->basePixmap().rect());
        if (cropRect.width() >= 2 && cropRect.height() >= 2) {
            QPixmap fullPix = scene->renderToPixmap();
            outPix = fullPix.copy(cropRect);
        } else {
            outPix = scene->renderToPixmap();
        }
    } else {
        outPix = scene->renderToPixmap();
    }

    if (ClipboardHelper::copyImage(outPix)) {
        statusBar()->showMessage(tr("Image copied to clipboard!"), 3000);
    }
}

bool MainWindow::saveTab(int index) {
    QWidget* w = m_tabWidget->widget(index);
    CanvasView* view = qobject_cast<CanvasView*>(w);
    if (!view || !view->canvasScene()) return false;

    CanvasScene* scene = view->canvasScene();
    QString saveDir = SettingsManager::instance().saveLocation();
    QDir().mkpath(saveDir);

    QString ext = SettingsManager::instance().defaultFormat().toLower();
    QString tabTitle = m_tabWidget->tabText(index);
    QString fileName;
    if (tabTitle.endsWith("." + ext, Qt::CaseInsensitive)) {
        fileName = tabTitle;
    } else {
        fileName = QString("Screenshot_%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss")).arg(ext);
    }
    QString fullPath = saveDir + "/" + fileName;

    QPixmap outPix = scene->renderToPixmap();
    if (outPix.save(fullPath)) {
        statusBar()->showMessage(tr("Saved to %1").arg(fullPath), 3000);
        m_tabWidget->setTabText(index, fileName);
        return true;
    } else {
        QString filter = QString("%1 (*.%2);;All Files (*)").arg(ext.toUpper()).arg(ext);
        QString path = QFileDialog::getSaveFileName(this, tr("Save Screenshot As"), fullPath, filter);
        if (path.isEmpty()) {
            return false;
        }
        if (outPix.save(path)) {
            QFileInfo fi(path);
            statusBar()->showMessage(tr("Saved to %1").arg(path), 3000);
            m_tabWidget->setTabText(index, fi.fileName());
            return true;
        }
        return false;
    }
}

bool MainWindow::maybeSaveTab(int index) {
    if (index < 0 || index >= m_tabWidget->count()) return true;

    QString tabTitle = m_tabWidget->tabText(index);
    QMessageBox::StandardButton res = QMessageBox::question(
        this,
        tr("Save Changes"),
        tr("Do you want to save changes to \"%1\" before closing?").arg(tabTitle),
        QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel,
        QMessageBox::Yes
    );

    if (res == QMessageBox::Yes) {
        return saveTab(index);
    } else if (res == QMessageBox::No) {
        return true;
    }
    // Cancel or closed dialog
    return false;
}

void MainWindow::onTabCloseRequested(int index) {
    if (!maybeSaveTab(index)) {
        return;
    }

    QWidget* w = m_tabWidget->widget(index);
    m_tabWidget->removeTab(index);
    if (w) {
        w->deleteLater();
    }

    if (m_tabWidget->count() == 0) {
        createBlankTab();
    }
    malloc_trim(0);
}

void MainWindow::onCurrentTabChanged(int) {
    CanvasView* v = currentView();
    if (v && v->canvasScene()) {
        QPixmap p = v->canvasScene()->basePixmap();
        m_statusDimensions->setText(QString("%1 × %2 px").arg(p.width()).arg(p.height()));
        onZoomChanged(v->zoomFactor());
        onSceneSelectionChanged();
    }
}

void MainWindow::onZoomChanged(double factor) {
    m_statusZoom->setText(QString("%1%").arg(qRound(factor * 100)));
}

void MainWindow::onCursorMoved(const QPoint& pt) {
    m_statusCoords->setText(QString("X: %1, Y: %2").arg(pt.x()).arg(pt.y()));
}

void MainWindow::onUndo() {
    CanvasScene* scene = currentScene();
    if (scene && scene->undoStack()->canUndo()) {
        scene->undoStack()->undo();
        statusBar()->showMessage(tr("Undo performed"), 2000);
    } else {
        statusBar()->showMessage(tr("Nothing to undo"), 2000);
    }
}

void MainWindow::onRedo() {
    CanvasScene* scene = currentScene();
    if (scene && scene->undoStack()->canRedo()) {
        scene->undoStack()->redo();
        statusBar()->showMessage(tr("Redo performed"), 2000);
    } else {
        statusBar()->showMessage(tr("Nothing to redo"), 2000);
    }
}

void MainWindow::selectTool(ToolType tool) {
    if (!m_toolActionGroup) return;
    for (QAction* act : m_toolActionGroup->actions()) {
        if (act->data().toInt() == static_cast<int>(tool)) {
            act->setChecked(true);
            onToolTriggered(act);
            break;
        }
    }
}

void MainWindow::onToolTriggered(QAction* action) {
    if (!action) return;
    ToolType tool = static_cast<ToolType>(action->data().toInt());

    if (tool == ToolType::BucketFill) {
        if (m_currentFillColor == Qt::transparent || !m_currentFillColor.isValid()) {
            m_currentFillColor = m_currentStrokeColor;
            m_fillColorBtn->setText("");
            m_fillColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(m_currentFillColor.name()));
            updateToolProperties();
        }
    }

    updateToolPropertiesVisibility(tool);

    if (tool == ToolType::Highlighter) {
        if (m_strokeWidthSpin->value() < 10) {
            m_strokeWidthSpin->setValue(18);
        }
    } else if (tool == ToolType::Pen) {
        if (m_strokeWidthSpin->value() > 10) {
            m_strokeWidthSpin->setValue(3);
        }
    }

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CanvasView* v = qobject_cast<CanvasView*>(m_tabWidget->widget(i));
        if (v && v->canvasScene()) {
            v->canvasScene()->setCurrentTool(tool);
            v->canvasScene()->setBlurLevel(m_currentBlurLevel);
        }
    }
}

void MainWindow::onSelectStrokeColor() {
    QColor c = QColorDialog::getColor(m_currentStrokeColor, this, tr("Select Color"));
    if (c.isValid()) {
        m_currentStrokeColor = c;
        m_strokeColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(c.name()));
        updateToolProperties();
        if (CanvasScene* scene = currentScene()) {
            for (auto* item : scene->selectedItems()) {
                if (auto* txt = dynamic_cast<TextItem*>(item)) {
                    txt->setStrokeColor(c);
                } else if (auto* pen = dynamic_cast<PenItem*>(item)) {
                    pen->setStrokeColor(c);
                } else if (auto* arrow = dynamic_cast<ArrowItem*>(item)) {
                    arrow->setStrokeColor(c);
                } else if (auto* shape = dynamic_cast<ShapeItem*>(item)) {
                    shape->setStrokeColor(c);
                } else if (auto* badge = dynamic_cast<BadgeItem*>(item)) {
                    badge->setStrokeColor(c);
                    badge->setFillColor(c);
                }
            }
            emit scene->sceneModified();
        }
    }
}

void MainWindow::onSelectFillColor() {
    QColor c = QColorDialog::getColor(m_currentFillColor.isValid() && m_currentFillColor != Qt::transparent ? m_currentFillColor : Qt::white, this, tr("Select Fill Color"), QColorDialog::ShowAlphaChannel);
    if (c.isValid()) {
        m_currentFillColor = c;
        m_fillColorBtn->setText("");
        m_fillColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(c.name()));
    } else {
        m_currentFillColor = Qt::transparent;
        m_fillColorBtn->setText("Ø");
        m_fillColorBtn->setStyleSheet("background-color: #888; color: #eee; border: 1px solid #666; border-radius: 3px; font-weight: bold;");
    }
    updateToolProperties();
    if (CanvasScene* scene = currentScene()) {
        for (auto* item : scene->selectedItems()) {
            if (auto* txt = dynamic_cast<TextItem*>(item)) {
                txt->setFillColor(m_currentFillColor);
            } else if (auto* shape = dynamic_cast<ShapeItem*>(item)) {
                shape->setFillColor(m_currentFillColor);
            }
        }
        emit scene->sceneModified();
    }
}

void MainWindow::onSelectFont() {
    bool ok = false;
    QFont f = QFontDialog::getFont(&ok, m_currentFont, this, tr("Select Font"));
    if (ok) {
        m_currentFont = f;
        if (m_fontFamilyCombo) {
            m_fontFamilyCombo->blockSignals(true);
            m_fontFamilyCombo->setCurrentFont(f);
            m_fontFamilyCombo->blockSignals(false);
        }
        if (m_fontSizeSpin) {
            m_fontSizeSpin->blockSignals(true);
            m_fontSizeSpin->setValue(f.pointSize() > 0 ? f.pointSize() : 11);
            m_fontSizeSpin->blockSignals(false);
        }
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            CanvasView* v = qobject_cast<CanvasView*>(m_tabWidget->widget(i));
            if (v && v->canvasScene()) {
                v->canvasScene()->setCurrentFont(f);
                for (auto* item : v->canvasScene()->selectedItems()) {
                    if (auto* txt = dynamic_cast<TextItem*>(item)) {
                        txt->setFont(f);
                    }
                }
                emit v->canvasScene()->sceneModified();
            }
        }
    }
}

void MainWindow::onFontFamilyChanged(const QFont& font) {
    m_currentFont.setFamily(font.family());
    CanvasScene* scene = currentScene();
    if (!scene) return;
    scene->setCurrentFont(m_currentFont);

    for (auto* item : scene->selectedItems()) {
        if (auto* txt = dynamic_cast<TextItem*>(item)) {
            QFont f = txt->font();
            f.setFamily(font.family());
            txt->setFont(f);
            emit scene->sceneModified();
        }
    }
}

void MainWindow::onFontSizeChanged(int size) {
    m_currentFont.setPointSize(size);
    CanvasScene* scene = currentScene();
    if (!scene) return;
    scene->setCurrentFont(m_currentFont);

    for (auto* item : scene->selectedItems()) {
        if (auto* txt = dynamic_cast<TextItem*>(item)) {
            QFont f = txt->font();
            f.setPointSize(size);
            txt->setFont(f);
            emit scene->sceneModified();
        }
    }
}

void MainWindow::onStrokeWidthChanged(int width) {
    m_currentStrokeWidth = width;
    updateToolProperties();
    if (CanvasScene* scene = currentScene()) {
        for (auto* item : scene->selectedItems()) {
            if (auto* shape = dynamic_cast<ShapeItem*>(item)) {
                shape->setStrokeWidth(width);
            } else if (auto* pen = dynamic_cast<PenItem*>(item)) {
                pen->setStrokeWidth(width);
            } else if (auto* arrow = dynamic_cast<ArrowItem*>(item)) {
                arrow->setStrokeWidth(width);
            }
        }
        emit scene->sceneModified();
    }
}

void MainWindow::onBlurLevelChanged(int level) {
    m_currentBlurLevel = level;
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CanvasView* v = qobject_cast<CanvasView*>(m_tabWidget->widget(i));
        if (v && v->canvasScene()) {
            v->canvasScene()->setBlurLevel(level);
            for (auto* item : v->canvasScene()->selectedItems()) {
                if (auto* bi = dynamic_cast<BlurItem*>(item)) {
                    bi->setBlurLevel(level);
                    bi->updateEffect(v->canvasScene()->basePixmap());
                }
            }
            emit v->canvasScene()->sceneModified();
        }
    }
}

void MainWindow::onResetBadgeCounter() {
    CanvasScene* scene = currentScene();
    if (scene) {
        auto sel = scene->selectedItems();
        if (!sel.isEmpty()) {
            for (auto* item : sel) {
                if (auto* badge = dynamic_cast<BadgeItem*>(item)) {
                    scene->modifyBadgeNumber(badge, 1);
                }
            }
        }
        scene->resetBadgeCounter();
        if (m_badgeNumberSpin) {
            m_badgeNumberSpin->blockSignals(true);
            m_badgeNumberSpin->setValue(1);
            m_badgeNumberSpin->blockSignals(false);
        }
        statusBar()->showMessage(tr("Badge counter reset to 1"), 2000);
    }
}

void MainWindow::onBadgeNumberSpinChanged(int value) {
    CanvasScene* scene = currentScene();
    if (!scene) return;

    auto sel = scene->selectedItems();
    if (!sel.isEmpty()) {
        for (auto* item : sel) {
            if (auto* badge = dynamic_cast<BadgeItem*>(item)) {
                scene->modifyBadgeNumber(badge, value);
            }
        }
    } else {
        scene->setBadgeCounter(value);
    }
}

void MainWindow::onBadgeCounterChanged(int nextNumber) {
    if (m_badgeNumberSpin && (!currentScene() || currentScene()->selectedItems().isEmpty())) {
        m_badgeNumberSpin->blockSignals(true);
        m_badgeNumberSpin->setValue(nextNumber);
        m_badgeNumberSpin->blockSignals(false);
    }
}

void MainWindow::onSceneSelectionChanged() {
    CanvasScene* scene = currentScene();
    if (!scene) return;

    auto sel = scene->selectedItems();
    if (sel.isEmpty()) {
        // No items selected: restore properties toolbar to match active tool
        QAction* activeAct = m_toolActionGroup->checkedAction();
        ToolType tool = activeAct ? static_cast<ToolType>(activeAct->data().toInt()) : ToolType::Pan;
        updateToolPropertiesVisibility(tool);

        // Restore toolbar widget values to application defaults
        m_strokeColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(m_currentStrokeColor.name()));
        if (m_currentFillColor.isValid() && m_currentFillColor != Qt::transparent && m_currentFillColor.alpha() > 0) {
            m_fillColorBtn->setText("");
            m_fillColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(m_currentFillColor.name()));
        } else {
            m_fillColorBtn->setText("Ø");
            m_fillColorBtn->setStyleSheet("background-color: #888; color: #eee; border: 1px solid #666; border-radius: 3px; font-weight: bold;");
        }
        m_strokeWidthSpin->blockSignals(true);
        m_strokeWidthSpin->setValue(m_currentStrokeWidth);
        m_strokeWidthSpin->blockSignals(false);

        m_blurRadiusSpin->blockSignals(true);
        m_blurRadiusSpin->setValue(m_currentBlurLevel);
        m_blurRadiusSpin->blockSignals(false);

        if (m_badgeNumberSpin && scene) {
            m_badgeNumberSpin->blockSignals(true);
            m_badgeNumberSpin->setValue(scene->badgeCounter());
            m_badgeNumberSpin->blockSignals(false);
        }
        return;
    }

    // Inspect the primary selected item
    QGraphicsItem* item = sel.last();

    if (auto* blur = dynamic_cast<BlurItem*>(item)) {
        if (m_actStrokeLbl) m_actStrokeLbl->setVisible(false);
        if (m_actStrokeColorBtn) m_actStrokeColorBtn->setVisible(false);
        if (m_actFillLbl) m_actFillLbl->setVisible(false);
        if (m_actFillColorBtn) m_actFillColorBtn->setVisible(false);
        if (m_actFontBtn) m_actFontBtn->setVisible(false);
        if (m_actFontFamilyCombo) m_actFontFamilyCombo->setVisible(false);
        if (m_actFontSizeLbl) m_actFontSizeLbl->setVisible(false);
        if (m_actFontSizeSpin) m_actFontSizeSpin->setVisible(false);
        if (m_fontBtn) m_fontBtn->setVisible(false);
        if (m_fontFamilyCombo) m_fontFamilyCombo->setVisible(false);
        if (m_fontSizeLbl) m_fontSizeLbl->setVisible(false);
        if (m_fontSizeSpin) m_fontSizeSpin->setVisible(false);
        if (m_actWidthLbl) m_actWidthLbl->setVisible(false);
        if (m_actStrokeWidthSpin) m_actStrokeWidthSpin->setVisible(false);
        if (m_actBadgeSeparator) m_actBadgeSeparator->setVisible(false);
        if (m_actBadgeNumberLbl) m_actBadgeNumberLbl->setVisible(false);
        if (m_actBadgeNumberSpin) m_actBadgeNumberSpin->setVisible(false);
        if (m_actResetBadgeBtn) m_actResetBadgeBtn->setVisible(false);
        if (m_actBlurRadiusLbl) m_actBlurRadiusLbl->setVisible(true);
        if (m_actBlurRadiusSpin) m_actBlurRadiusSpin->setVisible(true);

        m_blurRadiusSpin->blockSignals(true);
        m_blurRadiusSpin->setValue(blur->blurLevel());
        m_blurRadiusSpin->blockSignals(false);
    } else if (auto* txt = dynamic_cast<TextItem*>(item)) {
        if (m_actStrokeLbl) m_actStrokeLbl->setVisible(true);
        if (m_actStrokeColorBtn) m_actStrokeColorBtn->setVisible(true);
        if (m_actFillLbl) m_actFillLbl->setVisible(true);
        if (m_actFillColorBtn) m_actFillColorBtn->setVisible(true);
        if (m_actFontBtn) m_actFontBtn->setVisible(true);
        if (m_actFontFamilyCombo) m_actFontFamilyCombo->setVisible(true);
        if (m_actFontSizeLbl) m_actFontSizeLbl->setVisible(true);
        if (m_actFontSizeSpin) m_actFontSizeSpin->setVisible(true);
        if (m_fontBtn) m_fontBtn->setVisible(true);
        if (m_fontFamilyCombo) m_fontFamilyCombo->setVisible(true);
        if (m_fontSizeLbl) m_fontSizeLbl->setVisible(true);
        if (m_fontSizeSpin) m_fontSizeSpin->setVisible(true);
        if (m_actWidthLbl) m_actWidthLbl->setVisible(false);
        if (m_actStrokeWidthSpin) m_actStrokeWidthSpin->setVisible(false);
        if (m_actBadgeSeparator) m_actBadgeSeparator->setVisible(false);
        if (m_actBadgeNumberLbl) m_actBadgeNumberLbl->setVisible(false);
        if (m_actBadgeNumberSpin) m_actBadgeNumberSpin->setVisible(false);
        if (m_actResetBadgeBtn) m_actResetBadgeBtn->setVisible(false);
        if (m_actBlurRadiusLbl) m_actBlurRadiusLbl->setVisible(false);
        if (m_actBlurRadiusSpin) m_actBlurRadiusSpin->setVisible(false);

        if (m_fontFamilyCombo) {
            m_fontFamilyCombo->blockSignals(true);
            m_fontFamilyCombo->setCurrentFont(txt->font());
            m_fontFamilyCombo->blockSignals(false);
        }
        if (m_fontSizeSpin) {
            m_fontSizeSpin->blockSignals(true);
            m_fontSizeSpin->setValue(txt->font().pointSize() > 0 ? txt->font().pointSize() : 11);
            m_fontSizeSpin->blockSignals(false);
        }

        if (m_strokeLbl) m_strokeLbl->setText(tr(" Text: "));
        if (m_strokeColorBtn) {
            m_strokeColorBtn->setToolTip(tr("Text Color"));
            m_strokeColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(txt->strokeColor().name()));
        }
        if (m_fillLbl) m_fillLbl->setText(tr("  Background: "));
        if (m_fillColorBtn) {
            m_fillColorBtn->setToolTip(tr("Textbox Background Color (Ø for Transparent)"));
            if (txt->fillColor().isValid() && txt->fillColor() != Qt::transparent && txt->fillColor().alpha() > 0) {
                m_fillColorBtn->setText("");
                m_fillColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(txt->fillColor().name()));
            } else {
                m_fillColorBtn->setText("Ø");
                m_fillColorBtn->setStyleSheet("background-color: #888; color: #eee; border: 1px solid #666; border-radius: 3px; font-weight: bold;");
            }
        }
    } else if (auto* shape = dynamic_cast<ShapeItem*>(item)) {
        if (m_actStrokeLbl) m_actStrokeLbl->setVisible(true);
        if (m_actStrokeColorBtn) m_actStrokeColorBtn->setVisible(true);
        if (m_actFillLbl) m_actFillLbl->setVisible(true);
        if (m_actFillColorBtn) m_actFillColorBtn->setVisible(true);
        if (m_actFontBtn) m_actFontBtn->setVisible(false);
        if (m_actFontFamilyCombo) m_actFontFamilyCombo->setVisible(false);
        if (m_actFontSizeLbl) m_actFontSizeLbl->setVisible(false);
        if (m_actFontSizeSpin) m_actFontSizeSpin->setVisible(false);
        if (m_fontBtn) m_fontBtn->setVisible(false);
        if (m_fontFamilyCombo) m_fontFamilyCombo->setVisible(false);
        if (m_fontSizeLbl) m_fontSizeLbl->setVisible(false);
        if (m_fontSizeSpin) m_fontSizeSpin->setVisible(false);
        if (m_actWidthLbl) m_actWidthLbl->setVisible(true);
        if (m_actStrokeWidthSpin) m_actStrokeWidthSpin->setVisible(true);
        if (m_actBadgeSeparator) m_actBadgeSeparator->setVisible(false);
        if (m_actBadgeNumberLbl) m_actBadgeNumberLbl->setVisible(false);
        if (m_actBadgeNumberSpin) m_actBadgeNumberSpin->setVisible(false);
        if (m_actResetBadgeBtn) m_actResetBadgeBtn->setVisible(false);
        if (m_actBlurRadiusLbl) m_actBlurRadiusLbl->setVisible(false);
        if (m_actBlurRadiusSpin) m_actBlurRadiusSpin->setVisible(false);

        if (m_strokeLbl) m_strokeLbl->setText(tr(" Stroke: "));
        if (m_strokeColorBtn) {
            m_strokeColorBtn->setToolTip(tr("Stroke Color"));
            m_strokeColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(shape->strokeColor().name()));
        }
        if (m_fillLbl) m_fillLbl->setText(tr("  Fill: "));
        if (m_fillColorBtn) {
            m_fillColorBtn->setToolTip(tr("Fill Color (Ø for Transparent)"));
            if (shape->fillColor().isValid() && shape->fillColor() != Qt::transparent && shape->fillColor().alpha() > 0) {
                m_fillColorBtn->setText("");
                m_fillColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(shape->fillColor().name()));
            } else {
                m_fillColorBtn->setText("Ø");
                m_fillColorBtn->setStyleSheet("background-color: #888; color: #eee; border: 1px solid #666; border-radius: 3px; font-weight: bold;");
            }
        }
        m_strokeWidthSpin->blockSignals(true);
        m_strokeWidthSpin->setValue(shape->strokeWidth());
        m_strokeWidthSpin->blockSignals(false);
    } else if (auto* badge = dynamic_cast<BadgeItem*>(item)) {
        if (m_actStrokeLbl) {
            m_strokeLbl->setText(tr(" Color: "));
            m_actStrokeLbl->setVisible(true);
        }
        if (m_actStrokeColorBtn) {
            m_strokeColorBtn->setToolTip(tr("Badge Color"));
            m_strokeColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(badge->fillColor().name()));
            m_actStrokeColorBtn->setVisible(true);
        }
        if (m_actFillLbl) m_actFillLbl->setVisible(false);
        if (m_actFillColorBtn) m_actFillColorBtn->setVisible(false);
        if (m_actFontBtn) m_actFontBtn->setVisible(false);
        if (m_actFontFamilyCombo) m_actFontFamilyCombo->setVisible(false);
        if (m_actFontSizeLbl) m_actFontSizeLbl->setVisible(false);
        if (m_actFontSizeSpin) m_actFontSizeSpin->setVisible(false);
        if (m_fontBtn) m_fontBtn->setVisible(false);
        if (m_fontFamilyCombo) m_fontFamilyCombo->setVisible(false);
        if (m_fontSizeLbl) m_fontSizeLbl->setVisible(false);
        if (m_fontSizeSpin) m_fontSizeSpin->setVisible(false);
        if (m_actWidthLbl) m_actWidthLbl->setVisible(false);
        if (m_actStrokeWidthSpin) m_actStrokeWidthSpin->setVisible(false);
        if (m_actBlurRadiusLbl) m_actBlurRadiusLbl->setVisible(false);
        if (m_actBlurRadiusSpin) m_actBlurRadiusSpin->setVisible(false);

        if (m_actBadgeSeparator) m_actBadgeSeparator->setVisible(true);
        if (m_badgeNumberLbl) {
            m_badgeNumberLbl->setText(tr("  Badge #: "));
            m_badgeNumberLbl->setVisible(true);
        }
        if (m_actBadgeNumberLbl) m_actBadgeNumberLbl->setVisible(true);
        if (m_badgeNumberSpin) {
            m_badgeNumberSpin->blockSignals(true);
            m_badgeNumberSpin->setValue(badge->number());
            m_badgeNumberSpin->blockSignals(false);
            m_badgeNumberSpin->setVisible(true);
        }
        if (m_actBadgeNumberSpin) m_actBadgeNumberSpin->setVisible(true);
        if (m_resetBadgeBtn) {
            m_resetBadgeBtn->setText(tr("Reset to 1"));
            m_resetBadgeBtn->setVisible(true);
        }
        if (m_actResetBadgeBtn) m_actResetBadgeBtn->setVisible(true);
    } else if (auto* base = dynamic_cast<BaseAnnotationItem*>(item)) {
        if (m_actStrokeLbl) m_actStrokeLbl->setVisible(true);
        if (m_actStrokeColorBtn) m_actStrokeColorBtn->setVisible(true);
        if (m_actFillLbl) m_actFillLbl->setVisible(false);
        if (m_actFillColorBtn) m_actFillColorBtn->setVisible(false);
        if (m_actFontBtn) m_actFontBtn->setVisible(false);
        if (m_actFontFamilyCombo) m_actFontFamilyCombo->setVisible(false);
        if (m_actFontSizeLbl) m_actFontSizeLbl->setVisible(false);
        if (m_actFontSizeSpin) m_actFontSizeSpin->setVisible(false);
        if (m_fontBtn) m_fontBtn->setVisible(false);
        if (m_fontFamilyCombo) m_fontFamilyCombo->setVisible(false);
        if (m_fontSizeLbl) m_fontSizeLbl->setVisible(false);
        if (m_fontSizeSpin) m_fontSizeSpin->setVisible(false);
        if (m_actWidthLbl) m_actWidthLbl->setVisible(true);
        if (m_actStrokeWidthSpin) m_actStrokeWidthSpin->setVisible(true);
        if (m_actBadgeSeparator) m_actBadgeSeparator->setVisible(false);
        if (m_actBadgeNumberLbl) m_actBadgeNumberLbl->setVisible(false);
        if (m_actBadgeNumberSpin) m_actBadgeNumberSpin->setVisible(false);
        if (m_actResetBadgeBtn) m_actResetBadgeBtn->setVisible(false);
        if (m_actBlurRadiusLbl) m_actBlurRadiusLbl->setVisible(false);
        if (m_actBlurRadiusSpin) m_actBlurRadiusSpin->setVisible(false);

        if (m_strokeLbl) m_strokeLbl->setText(tr(" Stroke: "));
        if (m_strokeColorBtn) {
            m_strokeColorBtn->setToolTip(tr("Stroke Color"));
            m_strokeColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(base->strokeColor().name()));
        }
        m_strokeWidthSpin->blockSignals(true);
        m_strokeWidthSpin->setValue(base->strokeWidth());
        m_strokeWidthSpin->blockSignals(false);
    }
}

void MainWindow::updateToolProperties() {
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        CanvasView* v = qobject_cast<CanvasView*>(m_tabWidget->widget(i));
        if (v && v->canvasScene()) {
            v->canvasScene()->setStrokeColor(m_currentStrokeColor);
            v->canvasScene()->setFillColor(m_currentFillColor);
            v->canvasScene()->setStrokeWidth(m_currentStrokeWidth);
            v->canvasScene()->setBlurLevel(m_currentBlurLevel);
            v->canvasScene()->setCurrentFont(m_currentFont);
        }
    }
}

void MainWindow::updateToolPropertiesVisibility(ToolType tool) {
    bool hasStroke = (tool == ToolType::Pen || tool == ToolType::Highlighter ||
                      tool == ToolType::Line || tool == ToolType::Arrow ||
                      tool == ToolType::DoubleArrow || tool == ToolType::Rectangle ||
                      tool == ToolType::Ellipse || tool == ToolType::Badge ||
                      tool == ToolType::Text || tool == ToolType::BucketFill);
    bool hasFill = (tool == ToolType::Rectangle || tool == ToolType::Ellipse ||
                    tool == ToolType::Text || tool == ToolType::BucketFill);
    bool hasWidth = (tool == ToolType::Pen || tool == ToolType::Highlighter ||
                     tool == ToolType::Line || tool == ToolType::Arrow ||
                     tool == ToolType::DoubleArrow || tool == ToolType::Rectangle ||
                     tool == ToolType::Ellipse);
    bool isBadge = (tool == ToolType::Badge);
    bool isBlur = (tool == ToolType::Blur);
    bool isText = (tool == ToolType::Text);

    if (m_actStrokeLbl) {
        m_strokeLbl->setText(isText ? tr(" Text: ") : (isBadge ? tr(" Color: ") : tr(" Stroke: ")));
        m_actStrokeLbl->setVisible(hasStroke);
    }
    if (m_actStrokeColorBtn) {
        m_strokeColorBtn->setToolTip(isText ? tr("Text Color") : (isBadge ? tr("Badge Color") : tr("Stroke Color")));
        m_actStrokeColorBtn->setVisible(hasStroke);
    }
    if (m_actFillLbl) m_actFillLbl->setVisible(hasFill);
    if (m_actFillColorBtn) m_actFillColorBtn->setVisible(hasFill);
    if (m_actFontBtn) m_actFontBtn->setVisible(isText);
    if (m_actFontFamilyCombo) m_actFontFamilyCombo->setVisible(isText);
    if (m_actFontSizeLbl) m_actFontSizeLbl->setVisible(isText);
    if (m_actFontSizeSpin) m_actFontSizeSpin->setVisible(isText);
    if (m_fontBtn) m_fontBtn->setVisible(isText);
    if (m_fontFamilyCombo) m_fontFamilyCombo->setVisible(isText);
    if (m_fontSizeLbl) m_fontSizeLbl->setVisible(isText);
    if (m_fontSizeSpin) m_fontSizeSpin->setVisible(isText);

    if (isText) {
        if (m_fontFamilyCombo) {
            m_fontFamilyCombo->blockSignals(true);
            m_fontFamilyCombo->setCurrentFont(m_currentFont);
            m_fontFamilyCombo->blockSignals(false);
        }
        if (m_fontSizeSpin) {
            m_fontSizeSpin->blockSignals(true);
            m_fontSizeSpin->setValue(m_currentFont.pointSize() > 0 ? m_currentFont.pointSize() : 11);
            m_fontSizeSpin->blockSignals(false);
        }
    }

    if (m_actWidthLbl) m_actWidthLbl->setVisible(hasWidth);
    if (m_actStrokeWidthSpin) m_actStrokeWidthSpin->setVisible(hasWidth);
    if (m_actBadgeSeparator) m_actBadgeSeparator->setVisible(isBadge);
    if (m_badgeNumberLbl) {
        m_badgeNumberLbl->setText(tr("  Next #: "));
        m_badgeNumberLbl->setVisible(isBadge);
    }
    if (m_actBadgeNumberLbl) m_actBadgeNumberLbl->setVisible(isBadge);
    if (m_badgeNumberSpin) {
        if (isBadge) {
            CanvasScene* scene = currentScene();
            m_badgeNumberSpin->blockSignals(true);
            m_badgeNumberSpin->setValue(scene ? scene->badgeCounter() : 1);
            m_badgeNumberSpin->blockSignals(false);
        }
        m_badgeNumberSpin->setVisible(isBadge);
    }
    if (m_actBadgeNumberSpin) m_actBadgeNumberSpin->setVisible(isBadge);
    if (m_resetBadgeBtn) {
        m_resetBadgeBtn->setText(tr("Reset Numbering (1)"));
        m_resetBadgeBtn->setVisible(isBadge);
    }
    if (m_actResetBadgeBtn) m_actResetBadgeBtn->setVisible(isBadge);
    if (m_actBlurRadiusLbl) m_actBlurRadiusLbl->setVisible(isBlur);
    if (m_actBlurRadiusSpin) m_actBlurRadiusSpin->setVisible(isBlur);

    if (m_fillLbl) {
        m_fillLbl->setText(isText ? tr("  Background: ") : tr("  Fill: "));
    }
    if (m_fillColorBtn) {
        m_fillColorBtn->setToolTip(isText ? tr("Textbox Background Color (Ø for Transparent)") : tr("Fill Color (Ø for Transparent)"));
    }
}

void MainWindow::onCaptureFullscreen() {
    hide();
    QTimer::singleShot(250, this, []() {
        CaptureManager::instance().captureFullscreen();
    });
}

void MainWindow::onCaptureRegion() {
    hide();
    QTimer::singleShot(250, this, []() {
        CaptureManager::instance().captureRegion();
    });
}

void MainWindow::onColorPicker() {
    hide();
    QTimer::singleShot(250, this, []() {
        CaptureManager::instance().pickColor();
    });
}

void MainWindow::openSettingsDialog() {
    SettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::openAboutDialog() {
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::openLogViewerDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle(tr("S-Shot Logs & Diagnostics"));
    dlg.resize(650, 480);
    dlg.setStyleSheet("QDialog { background-color: #242424; color: #ffffff; }");

    QVBoxLayout* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(10);
    layout->setContentsMargins(16, 16, 16, 16);

    QTabWidget* tabWidget = new QTabWidget(&dlg);
    tabWidget->setStyleSheet("QTabWidget::pane { border: 1px solid #333333; } "
                             "QTabBar::tab { background: #2a2a2a; color: #bbb; padding: 6px 12px; } "
                             "QTabBar::tab:selected { background: #383838; color: #fff; }");

    auto createViewer = [](const QString& filePath, const QString& emptyMsg) -> QTextEdit* {
        QTextEdit* edit = new QTextEdit();
        edit->setReadOnly(true);
        edit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        edit->setStyleSheet("background-color: #1a1a1a; color: #e0e0e0; border: none; font-family: monospace; font-size: 11px;");
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QString content = QString::fromUtf8(file.readAll());
            edit->setPlainText(content.isEmpty() ? emptyMsg : content);
        } else {
            edit->setPlainText(emptyMsg);
        }
        return edit;
    };

    QTextEdit* appLogEdit = createViewer(CrashHandler::logPath(), tr("No application logs recorded yet."));
    QTextEdit* crashLogEdit = createViewer(CrashHandler::crashLogPath(), tr("No crash reports found. The application is running normally."));

    tabWidget->addTab(appLogEdit, tr("Application Log (s-shot.log)"));
    tabWidget->addTab(crashLogEdit, tr("Crash Log (crash.log)"));
    layout->addWidget(tabWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* openDirBtn = new QPushButton(tr("Open Logs Folder"), &dlg);
    openDirBtn->setStyleSheet("QPushButton { background-color: #383838; color: #fff; padding: 6px 12px; border-radius: 4px; }");
    connect(openDirBtn, &QPushButton::clicked, []() {
        QDesktopServices::openUrl(QUrl::fromLocalFile(CrashHandler::logDir()));
    });
    btnLayout->addWidget(openDirBtn);
    btnLayout->addStretch();

    QPushButton* closeBtn = new QPushButton(tr("Close"), &dlg);
    closeBtn->setStyleSheet("QPushButton { background-color: #383838; color: #fff; padding: 6px 14px; border-radius: 4px; }");
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    btnLayout->addWidget(closeBtn);

    layout->addLayout(btnLayout);
    dlg.exec();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (SettingsManager::instance().runInTrayOnClose()) {
        event->ignore();
        hide();
        malloc_trim(0);
    } else {
        for (int i = m_tabWidget->count() - 1; i >= 0; --i) {
            if (!maybeSaveTab(i)) {
                event->ignore();
                return;
            }
        }
        event->accept();
    }
}
