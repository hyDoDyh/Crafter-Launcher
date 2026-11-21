#ifndef DOWNLOAD_H
#define DOWNLOAD_H
#include "logger.h"
#include "minecraftversion.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include "library.h"
class Download : public QObject
{
    Q_OBJECT
public:
    void getAllVersions(QString url = "https://launchermeta.mojang.com/mc/game/version_manifest.json" );
    static Download* getInstance(){
        if(instance != nullptr)
            return instance;
        else
            return instance = new Download();
    }
    void setMainFolder(QWidget* wid);
    QString getMainFolder(QWidget* wid);
    void InstallMinecraft(QString url,QString name);// download.h
    static bool SaveOrCreateFile(const QString &filePath, const QByteArray &content);
    QByteArray DownloadFile(QString url);
private:
    static Download* instance;
    static int Progess;
    static int LibrariesProgess;
    static int All;
    static int AllLibraries ;
    Download();
    QNetworkAccessManager *networkManager = new QNetworkAccessManager(this);
    Logger *logger = Logger::getInstance();
    void DownloadAssets(QList<QString> list,int start,int end,QString Path);
    void DownloadLibraries(QList<Library>list,int start,int end,QString path);
signals:
    void versionsReady(const QList<MinecraftVersion> &versions);
    void errorOccurred(const QString &error);
    void downloadFinished(const QString &url, const QByteArray &data, bool success);
    void downloadProgress(const QString &url, qint64 received, qint64 total);

};
#endif // DOWNLOAD_H
