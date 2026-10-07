#pragma once

#include <QIcon>
#include <QString>
#include <QSize>

class IconManager {
public:
    // Returns a QIcon for the given icon name (e.g. "select", "crop", "new", "settings")
    // If isLight is true, it attempts to load from ":/icons/light/" if present, falling back to ":/icons/".
    static QIcon getIcon(const QString& name, bool isLight = false, const QSize& baseSize = QSize(24, 24));

    // Returns a QIcon for an explicit resource path (e.g. ":/icons/s-shot.svg")
    static QIcon getIconFromPath(const QString& resourcePath, const QSize& baseSize = QSize(24, 24));

    // Returns the application icon containing multi-resolution raster pixmaps (16, 22, 24, 32, 48, 64, 128, 256)
    static QIcon getAppIcon();

    // Clear any cached icons
    static void clearCache();
};
