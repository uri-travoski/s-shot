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

class ScrollingCaptureDialog : public QDialog {
    Q_OBJECT

public:
    explicit ScrollingCaptureDialog(QWidget* parent = nullptr);
    void startWithRegion(const QRect& region, const QPixmap& initialSlice);

signals:
    void scrollingCaptureFinished(const QPixmap& result);

private slots:
    void captureNextSlice();
    void finishCapture();
    void discardCapture();

private:
    void updatePreview();
    QImage stitchImages(const QImage& base, const QImage& next);

    QRect m_region;
    QList<QImage> m_slices;
    QImage m_stitchedImage;

    QLabel* m_statusLabel = nullptr;
    QLabel* m_previewLabel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QPushButton* m_captureNextBtn = nullptr;
    QPushButton* m_finishBtn = nullptr;
    QPushButton* m_cancelBtn = nullptr;
};
