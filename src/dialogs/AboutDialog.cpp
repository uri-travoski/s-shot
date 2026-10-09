#include "AboutDialog.h"
#include "../core/IconManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QFile>
#include <QTextStream>
#include <QDialogButtonBox>
#include <QCoreApplication>
#include <unistd.h>

AboutDialog::AboutDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("About S-Shot"));
    resize(420, 360);
    setStyleSheet("QDialog { background-color: #242424; color: #ffffff; }");

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(14);
    layout->setContentsMargins(24, 24, 24, 24);

    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setPixmap(IconManager::getAppIcon().pixmap(64, 64));
    headerLayout->addWidget(iconLabel);

    QVBoxLayout* titleLayout = new QVBoxLayout();
    QLabel* nameLabel = new QLabel("S-Shot", this);
    nameLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #30e500;");
    QString appVer = QCoreApplication::applicationVersion();
    if (appVer.isEmpty()) appVer = "1.31";
    QLabel* verLabel = new QLabel(tr("Version %1 (Linux x86_64)").arg(appVer), this);
    verLabel->setStyleSheet("font-size: 12px; color: #aaaaaa;");
    titleLayout->addWidget(nameLabel);
    titleLayout->addWidget(verLabel);
    headerLayout->addLayout(titleLayout);
    headerLayout->addStretch();
    layout->addLayout(headerLayout);

    QLabel* descLabel = new QLabel(
        tr("<b>S-Shot</b> is a blazing fast, ultra-lightweight screenshot capture "
           "and annotation tool for Linux with ksnip-inspired workflows.<br><br>"
           "• Multi-tab image & annotation editing<br>"
           "• Full annotation tools (Pen, Arrow, Shapes, Text, Badges, Blur)<br>"
           "• Select tool with Copy, Cut, Delete, and Crop raster actions<br>"
           "• Fullscreen, Region, & Color Picker<br>"
           "• Global shortcuts & System Tray integration"), this);
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("font-size: 12px; line-height: 1.4; color: #dddddd;");
    layout->addWidget(descLabel);

    // Live Memory Footprint
    QLabel* ramLabel = new QLabel(QString(tr("⚡ <b>RAM Footprint:</b> %1")).arg(getMemoryUsageString()), this);
    ramLabel->setStyleSheet("background-color: #1a1a1a; border: 1px solid #333333; border-radius: 4px; padding: 6px; color: #30e500;");
    layout->addWidget(ramLabel);

    layout->addStretch();

    QDialogButtonBox* bbox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    bbox->setStyleSheet("QPushButton { background-color: #383838; color: #fff; padding: 6px 14px; border-radius: 4px; }");
    connect(bbox, &QDialogButtonBox::rejected, this, &QDialog::accept);
    layout->addWidget(bbox);
}

QString AboutDialog::getMemoryUsageString() {
    QFile file("/proc/self/statm");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        long size, resident, share, text, lib, data, dt;
        stream >> size >> resident >> share >> text >> lib >> data >> dt;
        long pageSizeKb = sysconf(_SC_PAGESIZE) / 1024;
        double rssMb = (resident * pageSizeKb) / 1024.0;
        return QString("%1 MB (Target < 60 MB)").arg(QString::number(rssMb, 'f', 1));
    }
    return "< 25 MB";
}
