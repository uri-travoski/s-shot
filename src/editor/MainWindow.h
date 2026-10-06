#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QToolBar>
#include <QActionGroup>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QCloseEvent>
#include "ToolType.h"
#include "CanvasView.h"
#include "CanvasScene.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    void openImage(const QString& filePath);
    void addImageTab(const QPixmap& pixmap, const QString& title = QString());
    void createBlankTab(int width = 800, int height = 600);
    void applyTheme(const QString& theme);

public slots:
    void openFileDialog();
    void pasteFromClipboard();
    void saveActiveTab();
    void saveActiveTabAs();
    void copyActiveImageToClipboard();
    void openSettingsDialog();
    void openAboutDialog();

    // Capture slots
    void onCaptureFullscreen();
    void onCaptureRegion();
    void onCaptureScrolling();
    void onColorPicker();

    // Tool selection slots
    void onToolTriggered(QAction* action);
    void onSelectStrokeColor();
    void onSelectFillColor();
    void onStrokeWidthChanged(int width);
    void onBlurLevelChanged(int level);
    void onResetBadgeCounter();

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onZoomChanged(double factor);
    void onCursorMoved(const QPoint& pt);
    void onUndo();
    void onRedo();

private:
    void setupMenus();
    void setupToolbars();
    void setupStatusBar();
    void updateToolProperties();
    void updateToolPropertiesVisibility(ToolType tool);

    CanvasView* currentView() const;
    CanvasScene* currentScene() const;

    QTabWidget* m_tabWidget = nullptr;
    QPushButton* m_newTabBtn = nullptr;
    QToolBar* m_mainToolBar = nullptr;
    QToolBar* m_leftToolBar = nullptr;
    QToolBar* m_propToolBar = nullptr;

    // Themed action registry
    struct ThemedAction {
        QAction* action;
        QString iconName;
    };
    QList<ThemedAction> m_themedActions;

    // Tool actions
    QActionGroup* m_toolActionGroup = nullptr;
    QAction* m_actSelect = nullptr;
    QAction* m_actPen = nullptr;
    QAction* m_actHighlighter = nullptr;
    QAction* m_actLine = nullptr;
    QAction* m_actArrow = nullptr;
    QAction* m_actDoubleArrow = nullptr;
    QAction* m_actRect = nullptr;
    QAction* m_actEllipse = nullptr;
    QAction* m_actText = nullptr;
    QAction* m_actBadge = nullptr;
    QAction* m_actBlur = nullptr;
    QAction* m_actCrop = nullptr;

    // Property widgets
    QLabel* m_strokeLbl = nullptr;
    QPushButton* m_strokeColorBtn = nullptr;
    QLabel* m_fillLbl = nullptr;
    QPushButton* m_fillColorBtn = nullptr;
    QLabel* m_widthLbl = nullptr;
    QSpinBox* m_strokeWidthSpin = nullptr;
    QLabel* m_blurRadiusLbl = nullptr;
    QSpinBox* m_blurRadiusSpin = nullptr;
    QPushButton* m_resetBadgeBtn = nullptr;

    // Status bar widgets
    QLabel* m_statusDimensions = nullptr;
    QLabel* m_statusZoom = nullptr;
    QLabel* m_statusCoords = nullptr;

    QColor m_currentStrokeColor = QColor(255, 30, 30);
    QColor m_currentFillColor = Qt::transparent;
    int m_currentStrokeWidth = 3;
    int m_currentBlurLevel = 5;
    bool m_firstCloseNotification = true;
};
