#pragma once

#include <QPixmap>
#include <QImage>

class ClipboardHelper {
public:
    // Copies pixmap to clipboard with full MIME type support (image/png, image/x-png, image/bmp)
    // for both standard Clipboard and middle-click Selection on X11 / Wayland / desktop.
    static bool copyImage(const QPixmap& pixmap);
};
