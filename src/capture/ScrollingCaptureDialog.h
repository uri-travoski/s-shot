#pragma once

#include <QDialog>
#include <QPixmap>
#include <QImage>
#include <QList>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRect>

#include <QTimer>
#include <QKeyEvent>

class ScrollingCaptureDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScrollingCaptureDialog(QWidget* parent = nullptr);
    void startWithRegion(const QRect& region, const QPixmap& initialSlice);

    static QImage stitchImages(const QImage& base, const QImage& next);
    static bool areSlicesIdentical(const QImage& a, const QImage& b);

signals:
    void scrollingCaptureFinished(const QPixmap& result);

public slots:
    void startAutoCapture();
    void stopAutoCapture();
    void captureNextSlice();
    void finishCapture();
    void discardCapture();

protected:
    void keyPressEvent(QKeyEvent* event) override;

private slots:
    void performAutoScrollStep();

private:
    void updatePreview();
    void injectScrollDown(int steps = 3);

    QRect m_region;
    QList<QImage> m_slices;
    QImage m_lastSlice;
    QImage m_stitchedImage;

    bool m_isAutoCapturing = false;
    QTimer* m_autoTimer = nullptr;
    int m_maxSlices = 40;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_previewLabel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QPushButton* m_autoCaptureBtn = nullptr;
    QPushButton* m_captureNextBtn = nullptr;
    QPushButton* m_finishBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};
