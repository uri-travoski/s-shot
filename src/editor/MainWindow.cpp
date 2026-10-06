#include "MainWindow.h"
#include "../core/SettingsManager.h"
#include "../capture/CaptureManager.h"
#include "../dialogs/SettingsDialog.h"
#include "../dialogs/AboutDialog.h"
#include <QMenuBar>
#include <QMenu>
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
    setWindowIcon(QIcon(":/icons/s-shot.svg"));
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

    // Default blank tab on first open if no screenshot exists
    createBlankTab(800, 500);
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
            "QMenu::item:selected { background-color: #2e7d32; color: #ffffff; }"
            "QMenu::separator { height: 1px; background-color: #cccccc; margin: 4px 8px; }"
            "QToolBar { background-color: #dcdcdc; border-bottom: 1px solid #cccccc; spacing: 4px; padding: 3px; }"
            "QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; color: #222222; }"
            "QToolButton:hover { background-color: #cccccc; border-color: #b0b0b0; }"
            "QToolButton:checked { background-color: #2e7d32; border-color: #30e500; color: #ffffff; }"
            "QTabWidget::pane { border: none; background-color: #cccccc; }"
            "QTabBar::tab { background-color: #d0d0d0; color: #555555; padding: 8px 16px; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #ebebeb; color: #2e7d32; font-weight: bold; border-bottom: 2px solid #2e7d32; }"
            "QStatusBar { background-color: #d6d6d6; color: #333333; border-top: 1px solid #bfbfbf; }"
            "QLabel { color: #222222; }"
            "QSpinBox { background-color: #ffffff; color: #222222; border: 1px solid #b0b0b0; border-radius: 3px; padding: 2px 4px; }"
        );
        m_newTabBtn->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; background: transparent; color: #2e7d32; border: none; padding: 4px 10px; } QPushButton:hover { background: #cccccc; border-radius: 4px; }");
    } else {
        // Dark Theme
        setStyleSheet(
            "QMainWindow { background-color: #2b2b2b; color: #e0e0e0; }"
            "QMenuBar { background-color: #242424; color: #e0e0e0; border-bottom: 1px solid #3c3c3c; }"
            "QMenuBar::item:selected { background-color: #383838; }"
            "QMenu { background-color: #2c2c2c; color: #ffffff; border: 1px solid #444; }"
            "QMenu::item:selected { background-color: #388e3c; color: #ffffff; }"
            "QMenu::separator { height: 1px; background-color: #444; margin: 4px 8px; }"
            "QToolBar { background-color: #282828; border: none; spacing: 4px; padding: 3px; }"
            "QToolButton { background-color: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; color: #e0e0e0; }"
            "QToolButton:hover { background-color: #3d3d3d; border-color: #555555; }"
            "QToolButton:checked { background-color: #2e7d32; border-color: #30e500; color: #ffffff; }"
            "QTabWidget::pane { border: none; background-color: #202020; }"
            "QTabBar::tab { background-color: #282828; color: #aaaaaa; padding: 8px 16px; border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }"
            "QTabBar::tab:selected { background-color: #383838; color: #30e500; font-weight: bold; border-bottom: 2px solid #30e500; }"
            "QStatusBar { background-color: #1e1e1e; color: #999999; border-top: 1px solid #333333; }"
            "QLabel { color: #e0e0e0; }"
            "QSpinBox { background-color: #383838; color: #ffffff; border: 1px solid #555555; border-radius: 3px; padding: 2px 4px; }"
        );
        m_newTabBtn->setStyleSheet("QPushButton { font-weight: bold; font-size: 14px; background: transparent; color: #30e500; border: none; padding: 4px 10px; } QPushButton:hover { background: #383838; border-radius: 4px; }");
    }

    // Update icons for all registered actions
    for (const auto& item : m_themedActions) {
        QString iconPath = isLight ? QString(":/icons/light/%1.svg").arg(item.iconName)
                                   : QString(":/icons/%1.svg").arg(item.iconName);
        if (!QFile::exists(iconPath)) {
            iconPath = QString(":/icons/%1.svg").arg(item.iconName);
        }
        item.action->setIcon(QIcon(iconPath));
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
    QAction* actNew = fileMenu->addAction(QIcon(":/icons/new.svg"), tr("&New Tab"), QKeySequence::New, this, [this]() { createBlankTab(); });
    registerAct(actNew, "new");

    QAction* actOpen = fileMenu->addAction(QIcon(":/icons/open.svg"), tr("&Open..."), QKeySequence::Open, this, &MainWindow::openFileDialog);
    registerAct(actOpen, "open");

    QAction* actSave = fileMenu->addAction(QIcon(":/icons/save.svg"), tr("&Save"), QKeySequence::Save, this, &MainWindow::saveActiveTab);
    registerAct(actSave, "save");

    QAction* actSaveAs = fileMenu->addAction(QIcon(":/icons/save.svg"), tr("Save &As..."), QKeySequence::SaveAs, this, &MainWindow::saveActiveTabAs);
    registerAct(actSaveAs, "save");

    fileMenu->addSeparator();

    QAction* actCloseTray = fileMenu->addAction(QIcon(":/icons/s-shot.svg"), tr("Close to &Tray"), this, &MainWindow::hide);
    registerAct(actCloseTray, "s-shot");

    fileMenu->addSeparator();

    QAction* actQuit = fileMenu->addAction(QIcon(":/icons/quit.svg"), tr("&Quit S-Shot"), QKeySequence::Quit, qApp, &QCoreApplication::quit);
    registerAct(actQuit, "quit");

    // Edit Menu
    QMenu* editMenu = mb->addMenu(tr("&Edit"));
    QAction* actUndo = editMenu->addAction(QIcon(":/icons/undo.svg"), tr("&Undo"), QKeySequence::Undo, this, &MainWindow::onUndo);
    registerAct(actUndo, "undo");

    QAction* actRedo = editMenu->addAction(QIcon(":/icons/redo.svg"), tr("&Redo"), QKeySequence::Redo, this, &MainWindow::onRedo);
    registerAct(actRedo, "redo");

    editMenu->addSeparator();

    QAction* actCopy = editMenu->addAction(QIcon(":/icons/copy.svg"), tr("&Copy Image"), QKeySequence::Copy, this, &MainWindow::copyActiveImageToClipboard);
    registerAct(actCopy, "copy");

    QAction* actPaste = editMenu->addAction(QIcon(":/icons/paste.svg"), tr("&Paste"), QKeySequence::Paste, this, &MainWindow::pasteFromClipboard);
    registerAct(actPaste, "paste");

    // Capture Menu
    QMenu* capMenu = mb->addMenu(tr("&Capture"));
    QAction* actCapFull = capMenu->addAction(QIcon(":/icons/fullscreen.svg"), tr("Capture &Fullscreen"), this, &MainWindow::onCaptureFullscreen);
    registerAct(actCapFull, "fullscreen");

    QAction* actCapRegion = capMenu->addAction(QIcon(":/icons/snip.svg"), tr("Capture &Selected Region"), this, &MainWindow::onCaptureRegion);
    registerAct(actCapRegion, "snip");

    QAction* actCapScroll = capMenu->addAction(QIcon(":/icons/scroll.svg"), tr("Capture &Scrolling Window"), this, &MainWindow::onCaptureScrolling);
    registerAct(actCapScroll, "scroll");

    QAction* actCapColor = capMenu->addAction(QIcon(":/icons/picker.svg"), tr("Colour &Picker"), this, &MainWindow::onColorPicker);
    registerAct(actCapColor, "picker");

    // View Menu
    QMenu* viewMenu = mb->addMenu(tr("&View"));
    QAction* actZoomIn = viewMenu->addAction(QIcon(":/icons/zoom_in.svg"), tr("Zoom &In"), QKeySequence::ZoomIn, this, [this]() { if (currentView()) currentView()->zoomIn(); });
    registerAct(actZoomIn, "zoom_in");

    QAction* actZoomOut = viewMenu->addAction(QIcon(":/icons/zoom_out.svg"), tr("Zoom &Out"), QKeySequence::ZoomOut, this, [this]() { if (currentView()) currentView()->zoomOut(); });
    registerAct(actZoomOut, "zoom_out");

    QAction* actZoom100 = viewMenu->addAction(QIcon(":/icons/zoom_100.svg"), tr("&Actual Size (100%)"), this, [this]() { if (currentView()) currentView()->zoomActual(); });
    registerAct(actZoom100, "zoom_100");

    QAction* actZoomFit = viewMenu->addAction(QIcon(":/icons/zoom_fit.svg"), tr("&Fit to Window"), this, [this]() { if (currentView()) currentView()->zoomFit(); });
    registerAct(actZoomFit, "zoom_fit");

    // Options Menu
    QMenu* optMenu = mb->addMenu(tr("&Options"));
    QAction* actSettings = optMenu->addAction(QIcon(":/icons/settings.svg"), tr("&Settings..."), this, &MainWindow::openSettingsDialog);
    registerAct(actSettings, "settings");

    // Help Menu
    QMenu* helpMenu = mb->addMenu(tr("&Help"));
    QAction* actAbout = helpMenu->addAction(QIcon(":/icons/about.svg"), tr("&About S-Shot"), this, &MainWindow::openAboutDialog);
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

    QAction* tbNew = m_mainToolBar->addAction(QIcon(":/icons/new.svg"), tr("New"), this, [this]() { createBlankTab(); });
    registerAct(tbNew, "new");

    QAction* tbOpen = m_mainToolBar->addAction(QIcon(":/icons/open.svg"), tr("Open"), this, &MainWindow::openFileDialog);
    registerAct(tbOpen, "open");

    QAction* tbSave = m_mainToolBar->addAction(QIcon(":/icons/save.svg"), tr("Save"), this, &MainWindow::saveActiveTab);
    registerAct(tbSave, "save");

    QAction* tbCopy = m_mainToolBar->addAction(QIcon(":/icons/copy.svg"), tr("Copy"), this, &MainWindow::copyActiveImageToClipboard);
    registerAct(tbCopy, "copy");

    m_mainToolBar->addSeparator();

    QAction* tbUndo = m_mainToolBar->addAction(QIcon(":/icons/undo.svg"), tr("Undo"), this, &MainWindow::onUndo);
    registerAct(tbUndo, "undo");

    QAction* tbRedo = m_mainToolBar->addAction(QIcon(":/icons/redo.svg"), tr("Redo"), this, &MainWindow::onRedo);
    registerAct(tbRedo, "redo");

    m_mainToolBar->addSeparator();

    QAction* tbZoomIn = m_mainToolBar->addAction(QIcon(":/icons/zoom_in.svg"), tr("Zoom In"), this, [this]() { if (currentView()) currentView()->zoomIn(); });
    registerAct(tbZoomIn, "zoom_in");

    QAction* tbZoomOut = m_mainToolBar->addAction(QIcon(":/icons/zoom_out.svg"), tr("Zoom Out"), this, [this]() { if (currentView()) currentView()->zoomOut(); });
    registerAct(tbZoomOut, "zoom_out");

    QAction* tbZoom100 = m_mainToolBar->addAction(QIcon(":/icons/zoom_100.svg"), tr("100%"), this, [this]() { if (currentView()) currentView()->zoomActual(); });
    registerAct(tbZoom100, "zoom_100");

    QAction* tbZoomFit = m_mainToolBar->addAction(QIcon(":/icons/zoom_fit.svg"), tr("Fit"), this, [this]() { if (currentView()) currentView()->zoomFit(); });
    registerAct(tbZoomFit, "zoom_fit");

    // 2. Property Toolbar (below main toolbar)
    addToolBarBreak();
    m_propToolBar = addToolBar(tr("Tool Properties"));
    m_propToolBar->setMovable(false);

    m_strokeLbl = new QLabel(tr(" Stroke: "), this);
    m_propToolBar->addWidget(m_strokeLbl);

    m_strokeColorBtn = new QPushButton(this);
    m_strokeColorBtn->setFixedSize(26, 22);
    m_strokeColorBtn->setStyleSheet("background-color: #ff1e1e; border: 1px solid #888; border-radius: 3px;");
    connect(m_strokeColorBtn, &QPushButton::clicked, this, &MainWindow::onSelectStrokeColor);
    m_propToolBar->addWidget(m_strokeColorBtn);

    m_fillLbl = new QLabel(tr("  Fill: "), this);
    m_propToolBar->addWidget(m_fillLbl);

    m_fillColorBtn = new QPushButton(this);
    m_fillColorBtn->setFixedSize(26, 22);
    m_fillColorBtn->setText("Ø");
    m_fillColorBtn->setStyleSheet("background-color: #888; color: #eee; border: 1px solid #666; border-radius: 3px; font-weight: bold;");
    connect(m_fillColorBtn, &QPushButton::clicked, this, &MainWindow::onSelectFillColor);
    m_propToolBar->addWidget(m_fillColorBtn);

    m_widthLbl = new QLabel(tr("  Width: "), this);
    m_propToolBar->addWidget(m_widthLbl);

    m_strokeWidthSpin = new QSpinBox(this);
    m_strokeWidthSpin->setRange(1, 50);
    m_strokeWidthSpin->setValue(3);
    m_strokeWidthSpin->setFixedWidth(60);
    connect(m_strokeWidthSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onStrokeWidthChanged);
    m_propToolBar->addWidget(m_strokeWidthSpin);

    m_blurRadiusLbl = new QLabel(tr("  Blur (1-10): "), this);
    m_propToolBar->addWidget(m_blurRadiusLbl);

    m_blurRadiusSpin = new QSpinBox(this);
    m_blurRadiusSpin->setRange(1, 10);
    m_blurRadiusSpin->setValue(5);
    m_blurRadiusSpin->setFixedWidth(55);
    m_blurRadiusSpin->setToolTip(tr("Blur intensity from 1 (light) to 10 (heavy redaction)"));
    connect(m_blurRadiusSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBlurLevelChanged);
    m_propToolBar->addWidget(m_blurRadiusSpin);

    m_propToolBar->addSeparator();

    m_resetBadgeBtn = new QPushButton(tr("Reset Stepper (1)"), this);
    m_resetBadgeBtn->setStyleSheet("padding: 2px 6px; border: 1px solid #888; border-radius: 3px; font-size: 11px;");
    connect(m_resetBadgeBtn, &QPushButton::clicked, this, &MainWindow::onResetBadgeCounter);
    m_propToolBar->addWidget(m_resetBadgeBtn);

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
        QAction* act = new QAction(QIcon(QString(":/icons/%1.svg").arg(iconName)), title, this);
        act->setCheckable(true);
        act->setChecked(check);
        act->setData(static_cast<int>(t));
        m_toolActionGroup->addAction(act);
        m_leftToolBar->addAction(act);
        registerAct(act, iconName);
        return act;
    };

    m_actSelect = addToolAct("select", tr("Select / Area Tool (Copy/Cut/Move/Delete/Crop)"), ToolType::Select, true);
    m_actPen = addToolAct("pen", tr("Pen (Freehand Drawing)"), ToolType::Pen);
    m_actHighlighter = addToolAct("highlighter", tr("Highlighter"), ToolType::Highlighter);
    m_actLine = addToolAct("line", tr("Line"), ToolType::Line);
    m_actArrow = addToolAct("arrow", tr("Arrow"), ToolType::Arrow);
    m_actDoubleArrow = addToolAct("double_arrow", tr("Double Arrow"), ToolType::DoubleArrow);
    m_actRect = addToolAct("rect", tr("Rectangle"), ToolType::Rectangle);
    m_actEllipse = addToolAct("ellipse", tr("Ellipse"), ToolType::Ellipse);
    m_actText = addToolAct("text", tr("Text"), ToolType::Text);
    m_actBadge = addToolAct("badge", tr("Number / Stepper Badge (1, 2, 3...)"), ToolType::Badge);
    m_actBlur = addToolAct("blur", tr("Blur / Pixelate Redaction"), ToolType::Blur);
    m_actCrop = addToolAct("crop", tr("Crop Tool"), ToolType::Crop);

    updateToolPropertiesVisibility(ToolType::Select);
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
    CanvasScene* scene = new CanvasScene(this);
    scene->setBasePixmap(pixmap);
    scene->setStrokeColor(m_currentStrokeColor);
    scene->setFillColor(m_currentFillColor);
    scene->setStrokeWidth(m_currentStrokeWidth);
    scene->setBlurLevel(m_currentBlurLevel);

    QAction* activeAct = m_toolActionGroup->checkedAction();
    if (activeAct) {
        scene->setCurrentTool(static_cast<ToolType>(activeAct->data().toInt()));
    }

    CanvasView* view = new CanvasView(scene, this);
    view->applyTheme(SettingsManager::instance().theme() == "Light");
    connect(view, &CanvasView::zoomChanged, this, &MainWindow::onZoomChanged);
    connect(view, &CanvasView::mouseMovedTo, this, &MainWindow::onCursorMoved);

    QString tabTitle = title.isEmpty() ? QString("Capture %1").arg(m_tabWidget->count() + 1) : title;
    int idx = m_tabWidget->addTab(view, tabTitle);
    m_tabWidget->setCurrentIndex(idx);

    view->zoomFit();
    show();
    raise();
    activateWindow();
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

    QPixmap outPix = scene->renderToPixmap();
    QGuiApplication::clipboard()->setPixmap(outPix);
    statusBar()->showMessage(tr("Image copied to clipboard!"), 3000);
}

void MainWindow::onTabCloseRequested(int index) {
    if (m_tabWidget->count() <= 1) {
        m_tabWidget->removeTab(index);
        createBlankTab();
    } else {
        m_tabWidget->removeTab(index);
    }
}

void MainWindow::onCurrentTabChanged(int) {
    CanvasView* v = currentView();
    if (v && v->canvasScene()) {
        QPixmap p = v->canvasScene()->basePixmap();
        m_statusDimensions->setText(QString("%1 × %2 px").arg(p.width()).arg(p.height()));
        onZoomChanged(v->zoomFactor());
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
    }
}

void MainWindow::onRedo() {
    CanvasScene* scene = currentScene();
    if (scene && scene->undoStack()->canRedo()) {
        scene->undoStack()->redo();
    }
}

void MainWindow::onToolTriggered(QAction* action) {
    if (!action) return;
    ToolType tool = static_cast<ToolType>(action->data().toInt());

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
    QColor c = QColorDialog::getColor(m_currentStrokeColor, this, tr("Select Stroke Color"));
    if (c.isValid()) {
        m_currentStrokeColor = c;
        m_strokeColorBtn->setStyleSheet(QString("background-color: %1; border: 1px solid #888; border-radius: 3px;").arg(c.name()));
        updateToolProperties();
    }
}

void MainWindow::onSelectFillColor() {
    QColor c = QColorDialog::getColor(m_currentFillColor.isValid() ? m_currentFillColor : Qt::white, this, tr("Select Fill Color"), QColorDialog::ShowAlphaChannel);
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
}

void MainWindow::onStrokeWidthChanged(int width) {
    m_currentStrokeWidth = width;
    updateToolProperties();
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
        }
    }
}

void MainWindow::onResetBadgeCounter() {
    CanvasScene* scene = currentScene();
    if (scene) {
        scene->resetBadgeCounter();
        statusBar()->showMessage(tr("Badge counter reset to 1"), 2000);
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
        }
    }
}

void MainWindow::updateToolPropertiesVisibility(ToolType tool) {
    bool hasStroke = (tool == ToolType::Pen || tool == ToolType::Highlighter ||
                      tool == ToolType::Line || tool == ToolType::Arrow ||
                      tool == ToolType::DoubleArrow || tool == ToolType::Rectangle ||
                      tool == ToolType::Ellipse || tool == ToolType::Badge ||
                      tool == ToolType::Text);
    bool hasFill = (tool == ToolType::Rectangle || tool == ToolType::Ellipse || tool == ToolType::Text);
    bool hasWidth = (tool == ToolType::Pen || tool == ToolType::Highlighter ||
                     tool == ToolType::Line || tool == ToolType::Arrow ||
                     tool == ToolType::DoubleArrow || tool == ToolType::Rectangle ||
                     tool == ToolType::Ellipse);
    bool isBadge = (tool == ToolType::Badge);
    bool isBlur = (tool == ToolType::Blur);

    if (m_strokeLbl) m_strokeLbl->setVisible(hasStroke);
    if (m_strokeColorBtn) m_strokeColorBtn->setVisible(hasStroke);
    if (m_fillLbl) m_fillLbl->setVisible(hasFill);
    if (m_fillColorBtn) m_fillColorBtn->setVisible(hasFill);
    if (m_widthLbl) m_widthLbl->setVisible(hasWidth);
    if (m_strokeWidthSpin) m_strokeWidthSpin->setVisible(hasWidth);
    if (m_resetBadgeBtn) m_resetBadgeBtn->setVisible(isBadge);

    if (m_blurRadiusLbl) m_blurRadiusLbl->setVisible(isBlur);
    if (m_blurRadiusSpin) m_blurRadiusSpin->setVisible(isBlur);
}

void MainWindow::onCaptureFullscreen() {
    CaptureManager::instance().captureFullscreen();
}

void MainWindow::onCaptureRegion() {
    CaptureManager::instance().captureRegion();
}

void MainWindow::onCaptureScrolling() {
    CaptureManager::instance().captureScrollingWindow();
}

void MainWindow::onColorPicker() {
    CaptureManager::instance().pickColor();
}

void MainWindow::openSettingsDialog() {
    SettingsDialog dlg(this);
    dlg.exec();
}

void MainWindow::openAboutDialog() {
    AboutDialog dlg(this);
    dlg.exec();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (SettingsManager::instance().runInTrayOnClose()) {
        event->ignore();
        hide();
        malloc_trim(0);
    } else {
        event->accept();
    }
}
