#include "ClipboardHelper.h"
#include <QGuiApplication>
#include <QClipboard>
#include <QMimeData>
#include <QBuffer>
#include <QProcess>
#include <QStandardPaths>

bool ClipboardHelper::copyImage(const QPixmap& pixmap) {
    if (pixmap.isNull()) return false;

    QPixmap norm = pixmap;
    norm.setDevicePixelRatio(1.0);
    QImage img = norm.toImage();

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

    // Prepare JPEG bytes
    QByteArray jpegData;
    QBuffer jpegBuf(&jpegData);
    jpegBuf.open(QIODevice::WriteOnly);
    img.save(&jpegBuf, "JPEG");

    auto createMimeData = [&]() -> QMimeData* {
        QMimeData* mime = new QMimeData();
        mime->setImageData(img);
        mime->setData("image/png", pngData);
        mime->setData("image/x-png", pngData);
        mime->setData("image/bmp", bmpData);
        mime->setData("image/x-bmp", bmpData);
        mime->setData("image/x-MS-bmp", bmpData);
        mime->setData("image/jpeg", jpegData);
        mime->setData("image/jpg", jpegData);
        return mime;
    };

    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setMimeData(createMimeData(), QClipboard::Clipboard);
        if (clipboard->supportsSelection()) {
            clipboard->setMimeData(createMimeData(), QClipboard::Selection);
        }
    }

    // Wayland: pipe to wl-copy for rock-solid clipboard persistence across windows
    const QString wlCopy = QStandardPaths::findExecutable("wl-copy");
    if (!wlCopy.isEmpty()) {
        QProcess* proc = new QProcess();
        proc->start(wlCopy, QStringList() << "-t" << "image/png");
        if (proc->waitForStarted(500)) {
            proc->write(pngData);
            proc->closeWriteChannel();
            QObject::connect(proc, &QProcess::finished, proc, &QObject::deleteLater);
        } else {
            delete proc;
        }

        QProcess* procPrim = new QProcess();
        procPrim->start(wlCopy, QStringList() << "--primary" << "-t" << "image/png");
        if (procPrim->waitForStarted(500)) {
            procPrim->write(pngData);
            procPrim->closeWriteChannel();
            QObject::connect(procPrim, &QProcess::finished, procPrim, &QObject::deleteLater);
        } else {
            delete procPrim;
        }
    }

    // X11: pipe to xclip for robust persistence across clipboard managers
    const QString xclip = QStandardPaths::findExecutable("xclip");
    if (!xclip.isEmpty()) {
        QProcess* proc = new QProcess();
        proc->start(xclip, QStringList() << "-selection" << "clipboard" << "-t" << "image/png");
        if (proc->waitForStarted(500)) {
            proc->write(pngData);
            proc->closeWriteChannel();
            QObject::connect(proc, &QProcess::finished, proc, &QObject::deleteLater);
        } else {
            delete proc;
        }

        QProcess* procPrim = new QProcess();
        procPrim->start(xclip, QStringList() << "-selection" << "primary" << "-t" << "image/png");
        if (procPrim->waitForStarted(500)) {
            procPrim->write(pngData);
            procPrim->closeWriteChannel();
            QObject::connect(procPrim, &QProcess::finished, procPrim, &QObject::deleteLater);
        } else {
            delete procPrim;
        }
    }

    return true;
}

QPixmap ClipboardHelper::getClipboardImage() {
    const QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        // 1. Direct QPixmap check (Clipboard buffer)
        QPixmap pix = clipboard->pixmap(QClipboard::Clipboard);
        if (!pix.isNull()) {
            pix.setDevicePixelRatio(1.0);
            return pix;
        }

        // 2. Direct QImage check (Clipboard buffer)
        QImage img = clipboard->image(QClipboard::Clipboard);
        if (!img.isNull()) {
            QPixmap p = QPixmap::fromImage(img);
            p.setDevicePixelRatio(1.0);
            return p;
        }

        // 3. MIME data extraction from Clipboard buffer
        const QMimeData* mime = clipboard->mimeData(QClipboard::Clipboard);
        if (mime && mime->hasImage()) {
            QVariant data = mime->imageData();
            if (data.canConvert<QPixmap>()) {
                QPixmap p = qvariant_cast<QPixmap>(data);
                if (!p.isNull()) {
                    p.setDevicePixelRatio(1.0);
                    return p;
                }
            }
            if (data.canConvert<QImage>()) {
                QImage im = qvariant_cast<QImage>(data);
                if (!im.isNull()) {
                    QPixmap p = QPixmap::fromImage(im);
                    p.setDevicePixelRatio(1.0);
                    return p;
                }
            }
        }

        // 4. Fallback check for Selection buffer on X11 / Wayland
        if (clipboard->supportsSelection()) {
            QPixmap selPix = clipboard->pixmap(QClipboard::Selection);
            if (!selPix.isNull()) {
                selPix.setDevicePixelRatio(1.0);
                return selPix;
            }

            QImage selImg = clipboard->image(QClipboard::Selection);
            if (!selImg.isNull()) {
                QPixmap p = QPixmap::fromImage(selImg);
                p.setDevicePixelRatio(1.0);
                return p;
            }
        }
    }

    // 5. Wayland fallback via wl-paste
    const QString wlPaste = QStandardPaths::findExecutable("wl-paste");
    if (!wlPaste.isEmpty()) {
        QProcess proc;
        proc.start(wlPaste, QStringList() << "-t" << "image/png");
        if (proc.waitForFinished(1000)) {
            QByteArray data = proc.readAllStandardOutput();
            if (!data.isEmpty()) {
                QPixmap p;
                if (p.loadFromData(data, "PNG")) {
                    p.setDevicePixelRatio(1.0);
                    return p;
                }
            }
        }
    }

    // 6. X11 fallback via xclip
    const QString xclip = QStandardPaths::findExecutable("xclip");
    if (!xclip.isEmpty()) {
        QProcess proc;
        proc.start(xclip, QStringList() << "-selection" << "clipboard" << "-t" << "image/png" << "-o");
        if (proc.waitForFinished(1000)) {
            QByteArray data = proc.readAllStandardOutput();
            if (!data.isEmpty()) {
                QPixmap p;
                if (p.loadFromData(data, "PNG")) {
                    p.setDevicePixelRatio(1.0);
                    return p;
                }
            }
        }
    }

    return QPixmap();
}
