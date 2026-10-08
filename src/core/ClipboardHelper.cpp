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

QPixmap ClipboardHelper::getClipboardImage() {
    const QClipboard* clipboard = QGuiApplication::clipboard();
    if (!clipboard) return QPixmap();

    // 1. Direct QPixmap check (Clipboard buffer)
    QPixmap pix = clipboard->pixmap(QClipboard::Clipboard);
    if (!pix.isNull()) return pix;

    // 2. Direct QImage check (Clipboard buffer)
    QImage img = clipboard->image(QClipboard::Clipboard);
    if (!img.isNull()) return QPixmap::fromImage(img);

    // 3. MIME data extraction from Clipboard buffer
    const QMimeData* mime = clipboard->mimeData(QClipboard::Clipboard);
    if (mime && mime->hasImage()) {
        QVariant data = mime->imageData();
        if (data.canConvert<QPixmap>()) {
            QPixmap p = qvariant_cast<QPixmap>(data);
            if (!p.isNull()) return p;
        }
        if (data.canConvert<QImage>()) {
            QImage im = qvariant_cast<QImage>(data);
            if (!im.isNull()) return QPixmap::fromImage(im);
        }
    }

    // 4. Fallback check for Selection buffer on X11 / Wayland
    if (clipboard->supportsSelection()) {
        QPixmap selPix = clipboard->pixmap(QClipboard::Selection);
        if (!selPix.isNull()) return selPix;

        QImage selImg = clipboard->image(QClipboard::Selection);
        if (!selImg.isNull()) return QPixmap::fromImage(selImg);
    }

    return QPixmap();
}
