#pragma once

#include <QObject>
#include <QString>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QWidget>

class UpdateManager : public QObject {
    Q_OBJECT

public:
    static UpdateManager& instance();

    // Check for updates against GitHub Releases API
    void checkForUpdates(bool silentIfUpToDate = false, QWidget* parent = nullptr);

    // Static helper to compare semantic versions
    static bool isVersionNewer(const QString& remoteVersion, const QString& currentVersion);

    QString currentVersion() const;
    QString latestVersion() const { return m_latestVersion; }
    QString releaseUrl() const { return m_releaseUrl; }
    bool isChecking() const { return m_isChecking; }

signals:
    void checkStarted();
    void checkFinished(bool updateAvailable, const QString& latestVersion, const QString& releaseUrl);
    void checkError(const QString& errorMessage);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    explicit UpdateManager(QObject* parent = nullptr);
    ~UpdateManager() override = default;

    QNetworkAccessManager* m_nam = nullptr;
    bool m_isChecking = false;
    bool m_silentIfUpToDate = false;
    QWidget* m_parentWidget = nullptr;

    QString m_latestVersion;
    QString m_releaseUrl;
};
