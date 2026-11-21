#ifndef DOWNLOADTHREAD_H
#define DOWNLOADTHREAD_H
#include "logger.h"
#include <QObject>
#include "library.h"
#include <QNetworkAccessManager>
class DownloadThread : public QObject
{
    Q_OBJECT
public:
    explicit DownloadThread(QObject *parent = nullptr);
public slots:
    void DownloadAssets(const QList<QString> &list,int start,int end,QString Path);
    void DownloadLibraries( QList<Library> list,int start,int end,QString Path);
private:
    Logger *logger = Logger::getInstance();
signals:
    void resultReady(bool error);
    void progress(int percent);

    void LibrariesReady(bool error);
    void LibrariesProgress(int percent);
};

#endif // DOWNLOADTHREAD_H
