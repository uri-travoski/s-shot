#include "ClipboardHelper.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>

bool ClipboardHelper::copyImage(const QPixmap& pixmap) {
    if (pixmap.isNull()) return false;

    QImage img = pixmap.toImage();

    // Prepare PNG bytes
    QByteArray pngData;
    QBuffer buf(&pngData);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");

    // Prepare BMP bytes for legacy X11 / desktop applications
    QByteArray bmpData;
    QBuffer bmpBuf(&bmpData);
    bmpBuf.open(QIODevice::WriteOnly);
    img.save(&bmpBuf, "BMP");

    auto createMimeData = [&]() -> QMimeData* {
        QMimeData* mime = new QMimeData();
        mime->setImageData(img);
        mime->setData("image/png", pngData);
        mime->setData("image/x-png", pngData);
        mime->setData("image/bmp", bmpData);
        return mime;
    };

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (!clipboard) return false;

    clipboard->setMimeData(createMimeData(), QClipboard::Clipboard);
    if (clipboard->supportsSelection()) {
        clipboard->setMimeData(createMimeData(), QClipboard::Selection);
    }

    return true;
}
