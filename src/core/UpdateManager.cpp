#include "UpdateManager.h"
#include <QCoreApplication>
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPushButton>
#include <QDesktopServices>
#include <vector>

UpdateManager& UpdateManager::instance() {
    static UpdateManager s_instance;
    return s_instance;
}

UpdateManager::UpdateManager(QObject* parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
{
}

QString UpdateManager::currentVersion() const {
    QString ver = QCoreApplication::applicationVersion();
    if (ver.isEmpty()) {
        ver = "1.26";
    }
    return ver;
}

bool UpdateManager::isVersionNewer(const QString& remoteVersion, const QString& currentVersion) {
    auto parse = [](QString v) -> std::vector<int> {
        v = v.trimmed();
        if (v.startsWith('v', Qt::CaseInsensitive)) {
            v.remove(0, 1);
        }
        QStringList parts = v.split('.', Qt::SkipEmptyParts);
        std::vector<int> nums;
        for (const QString& part : parts) {
            QString digits;
            for (QChar c : part) {
                if (c.isDigit()) digits.append(c);
                else break;
            }
            nums.push_back(digits.isEmpty() ? 0 : digits.toInt());
        }
        while (nums.size() < 3) {
            nums.push_back(0);
        }
        return nums;
    };

    std::vector<int> r = parse(remoteVersion);
    std::vector<int> c = parse(currentVersion);

    for (size_t i = 0; i < std::max(r.size(), c.size()); ++i) {
        int rv = (i < r.size()) ? r[i] : 0;
        int cv = (i < c.size()) ? c[i] : 0;
        if (rv > cv) return true;
        if (rv < cv) return false;
    }
    return false;
}

void UpdateManager::checkForUpdates(bool silentIfUpToDate, QWidget* parent) {
    if (m_isChecking) return;
    m_isChecking = true;
    m_silentIfUpToDate = silentIfUpToDate;
    m_parentWidget = parent;

    emit checkStarted();

    QUrl url("https://api.github.com/repos/uri-travoski/s-shot/releases/latest");
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::UserAgentHeader, "S-Shot/" + currentVersion());
    req.setRawHeader("Accept", "application/vnd.github+json");

    QNetworkReply* reply = m_nam->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void UpdateManager::onReplyFinished(QNetworkReply* reply) {
    reply->deleteLater();
    m_isChecking = false;

    QWidget* parent = m_parentWidget;
    if (!parent) {
        parent = QApplication::activeWindow();
    }

    if (reply->error() != QNetworkReply::NoError) {
        QString err = reply->errorString();
        emit checkError(err);
        if (!m_silentIfUpToDate) {
            QMessageBox::warning(
                parent,
                tr("Update Check Failed"),
                tr("Could not check for updates:\n%1").arg(err)
            );
        }
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        QString err = tr("Invalid response received from GitHub.");
        emit checkError(err);
        if (!m_silentIfUpToDate) {
            QMessageBox::warning(parent, tr("Update Check Failed"), err);
        }
        return;
    }

    QJsonObject obj = doc.object();
    QString tagName = obj.value("tag_name").toString();
    QString htmlUrl = obj.value("html_url").toString();
    if (htmlUrl.isEmpty()) {
        htmlUrl = "https://github.com/uri-travoski/s-shot/releases";
    }

    m_latestVersion = tagName;
    m_releaseUrl = htmlUrl;

    bool newer = isVersionNewer(tagName, currentVersion());
    emit checkFinished(newer, tagName, htmlUrl);

    if (newer) {
        QMessageBox box(parent);
        box.setWindowTitle(tr("Update Available - S-Shot"));
        box.setIcon(QMessageBox::Information);
        box.setText(tr("A new version of S-Shot is available!"));
        box.setInformativeText(
            tr("Current version: %1\nLatest version: %2\n\nWould you like to open the GitHub release page to update?")
            .arg(currentVersion(), tagName)
        );
        QPushButton* updateBtn = box.addButton(tr("Yes, Update"), QMessageBox::AcceptRole);
        QPushButton* laterBtn = box.addButton(tr("No, Later"), QMessageBox::RejectRole);
        box.setDefaultButton(updateBtn);
        box.exec();

        if (box.clickedButton() == updateBtn) {
            QDesktopServices::openUrl(QUrl(htmlUrl));
        }
    } else {
        if (!m_silentIfUpToDate) {
            QMessageBox::information(
                parent,
                tr("S-Shot is Up to Date"),
                tr("You are running the latest version of S-Shot (v%1).").arg(currentVersion())
            );
        }
    }
}
