#pragma once

#include <QMainWindow>
#include <QTabWidget>
#include <QToolBar>
#include <QActionGroup>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QFontComboBox>
#include <QCloseEvent>
#include "ToolType.h"
#include "CanvasView.h"
#include "CanvasScene.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void openImage(const QString& filePath);
    void addImageTab(const QPixmap& pixmap, const QString& title = QString());
    void createBlankTab(int width = 800, int height = 600);
    void applyTheme(const QString& theme);
    void selectTool(ToolType tool);
    CanvasView* currentView() const;
    CanvasScene* currentScene() const;

public slots:
    void openFileDialog();
    void pasteFromClipboard();
    void pasteAsNewImage();
    void saveActiveTab();
    void saveActiveTabAs();
    void copyActiveImageToClipboard();
    void openSettingsDialog();
    void openAboutDialog();
    void openLogViewerDialog();

    // Capture slots
    void onCaptureFullscreen();
    void onCaptureRegion();
    void onColorPicker();

    // Tool selection slots
    void onToolTriggered(QAction* action);
    void onSelectStrokeColor();
    void onSelectFillColor();
    void onStrokeWidthChanged(int width);
    void onBlurLevelChanged(int level);
    void onResetBadgeCounter();
    void onBadgeNumberSpinChanged(int value);
    void onBadgeCounterChanged(int nextNumber);
    void onSelectFont();
    void onFontFamilyChanged(const QFont& font);
    void onFontSizeChanged(int size);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onTabCloseRequested(int index);
    void onCurrentTabChanged(int index);
    void onZoomChanged(double factor);
    void onCursorMoved(const QPoint& pt);
    void onUndo();
    void onRedo();
    void onSceneSelectionChanged();

private:
    bool maybeSaveTab(int index);
    bool saveTab(int index);

    void setupMenus();
    void setupToolbars();
    void setupStatusBar();
    void updateToolProperties();
    void updateToolPropertiesVisibility(ToolType tool);

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
    QAction* m_actPan = nullptr;
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
    QAction* m_actBucket = nullptr;
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
    QLabel* m_badgeNumberLbl = nullptr;
    QSpinBox* m_badgeNumberSpin = nullptr;
    QPushButton* m_resetBadgeBtn = nullptr;
    QPushButton* m_fontBtn = nullptr;
    QFontComboBox* m_fontFamilyCombo = nullptr;
    QLabel* m_fontSizeLbl = nullptr;
    QSpinBox* m_fontSizeSpin = nullptr;

    QAction* m_actStrokeLbl = nullptr;
    QAction* m_actStrokeColorBtn = nullptr;
    QAction* m_actFillLbl = nullptr;
    QAction* m_actFillColorBtn = nullptr;
    QAction* m_actFontBtn = nullptr;
    QAction* m_actFontFamilyCombo = nullptr;
    QAction* m_actFontSizeLbl = nullptr;
    QAction* m_actFontSizeSpin = nullptr;
    QAction* m_actWidthLbl = nullptr;
    QAction* m_actStrokeWidthSpin = nullptr;
    QAction* m_actBlurRadiusLbl = nullptr;
    QAction* m_actBlurRadiusSpin = nullptr;
    QAction* m_actBadgeSeparator = nullptr;
    QAction* m_actBadgeNumberLbl = nullptr;
    QAction* m_actBadgeNumberSpin = nullptr;
    QAction* m_actResetBadgeBtn = nullptr;

    // Status bar widgets
    QLabel* m_statusDimensions = nullptr;
    QLabel* m_statusZoom = nullptr;
    QLabel* m_statusCoords = nullptr;

    QColor m_currentStrokeColor = QColor(255, 30, 30);
    QColor m_currentFillColor = Qt::transparent;
    QFont m_currentFont = QFont("Sans", 11, QFont::Bold);
    int m_currentStrokeWidth = 3;
    int m_currentBlurLevel = 4;
    bool m_firstCloseNotification = true;
};
