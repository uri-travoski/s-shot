#include "IconManager.h"
#include <QPainter>
#include <QFile>
#include <QHash>
#include <QVector>
#include <QSvgRenderer>

static QHash<QString, QIcon> s_iconCache;

QIcon IconManager::getIcon(const QString& name, bool isLight, const QSize& baseSize) {
    QString path = isLight ? QStringLiteral(":/icons/light/%1.svg").arg(name)
                           : QStringLiteral(":/icons/%1.svg").arg(name);
    if (!QFile::exists(path)) {
        path = QStringLiteral(":/icons/%1.svg").arg(name);
    }
    return getIconFromPath(path, baseSize);
}

QIcon IconManager::getIconFromPath(const QString& resourcePath, const QSize& baseSize) {
    if (s_iconCache.contains(resourcePath)) {
        return s_iconCache.value(resourcePath);
    }

    QIcon icon;
    // QSvgRenderer renders SVG XML directly to QPixmap without needing qt6-svg-plugins (libqsvg.so)
    if (resourcePath.endsWith(QLatin1String(".svg"), Qt::CaseInsensitive)) {
        QSvgRenderer renderer(resourcePath);
        if (renderer.isValid()) {
            const QVector<int> sizes = {16, 20, 22, 24, 32, 48, 64};
            for (int sz : sizes) {
                QPixmap pix(sz, sz);
                pix.fill(Qt::transparent);
                QPainter painter(&pix);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setRenderHint(QPainter::SmoothPixmapTransform);
                renderer.render(&painter);
                painter.end();
                icon.addPixmap(pix);
            }
        }
    }

    if (icon.isNull()) {
        icon = QIcon(resourcePath);
    }

    s_iconCache.insert(resourcePath, icon);
    return icon;
}

QIcon IconManager::getAppIcon() {
    static QIcon s_appIcon;
    if (!s_appIcon.isNull()) {
        return s_appIcon;
    }

    const QVector<int> sizes = {16, 22, 24, 32, 48, 64, 128, 256};
    for (int sz : sizes) {
        QString pngPath = QStringLiteral(":/icons/hicolor/%1x%1/apps/s-shot.png").arg(sz);
        if (QFile::exists(pngPath)) {
            s_appIcon.addFile(pngPath, QSize(sz, sz));
        }
    }

    QSvgRenderer renderer(QStringLiteral(":/icons/s-shot.svg"));
    if (renderer.isValid()) {
        for (int sz : sizes) {
            QPixmap pix(sz, sz);
            pix.fill(Qt::transparent);
            QPainter painter(&pix);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);
            renderer.render(&painter);
            painter.end();
            s_appIcon.addPixmap(pix);
        }
    }

    return s_appIcon;
}

void IconManager::clearCache() {
    s_iconCache.clear();
}
