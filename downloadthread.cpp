#include "downloadthread.h"
#include "download.h"
#include <QThread>
#include <algorithm>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QFile>
#include <QEventLoop>
#include <QFileInfo>
#include <QTimer>
DownloadThread::DownloadThread(QObject *parent)
    : QObject{parent}
{}
void DownloadThread::DownloadAssets(const QList<QString> &list,int start,int end,QString Path){
    if(start > end) {
        emit resultReady(true);
        return;
    }
    if(start >= list.count()){
        emit resultReady(true);
        return;
    }
    QNetworkAccessManager *networkManager = new QNetworkAccessManager();
    QString threadIdHex = "0x" + QString::number(
                              reinterpret_cast<quintptr>(QThread::currentThreadId()),
                              16
                              );
    logger->info("启动下载线程:"+threadIdHex);
    int sum = 0;
    for(int i = start;i <= end;i++){
        QNetworkRequest request;
        QString prefix = list[i].mid(0,2);
        request.setUrl(QUrl("https://resources.download.minecraft.net/"+prefix+"/"+list[i]));
        // 正确：使用事件循环等待下载完成
        QNetworkReply *reply = networkManager->get(request);

        QEventLoop loop;
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);

        // 设置超时
        QTimer timer;
        timer.setSingleShot(true);
        connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(30000);  // 30秒超时

        loop.exec();  // 等待下载完成

        QString path = QString("%1/"+prefix+"/"+list[i]).arg(Path);
        Download::SaveOrCreateFile(path,reply->readAll());

        reply->deleteLater();
        sum++;
        if(sum % 50 == 0) {
            logger->info(threadIdHex+"的线程，进度："+QString::number(i - start)+"/"+QString::number(end - start));
            emit progress(50);
        }
    }
    emit progress(sum % 50);
    emit resultReady(false);
}
void DownloadThread::DownloadLibraries(QList<Library> list, int start, int end, QString Path)
{
    if (start > end || start >= list.count()) {
        emit LibrariesReady(true);
        return;
    }
    QNetworkAccessManager networkManager;   // 栈上，随函数销毁

    QString threadIdHex = "0x" + QString::number(
                              reinterpret_cast<quintptr>(QThread::currentThreadId()), 16);
    logger->info("启动库下载线程:" + threadIdHex);

    int sum = 0;
    int total = end - start + 1;

    for (int i = start; i <= end && i < list.count(); ++i) {
        qDebug()<<list[i].getUrl();
        const QString url  = list[i].getUrl();
        const QString path = Path + "/" + list[i].getPath();

        if (url.isEmpty()) {
            logger->warning("空 URL 跳过: " + path);
            continue;
        }

        // 已存在且非空则跳过（可选）
        if (QFileInfo(path).exists() && QFileInfo(path).size() > 0) {
            sum++;
            continue;
        }

        bool ok = false;
        for (int retry = 0; retry < 3 && !ok; ++retry) {
            QNetworkRequest request{QUrl(url)};
            QNetworkReply *reply = networkManager.get(request);

            QEventLoop loop;
            QTimer timer;
            timer.setSingleShot(true);
            bool timedOut = false;

            connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            connect(&timer, &QTimer::timeout, &loop, [&]{
                timedOut = true;
                reply->abort();              // ★ 让 finished 立刻触发
            });
            timer.start(60000);

            loop.exec();


            if (!timedOut && reply->error() == QNetworkReply::NoError) {
                int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
                if (status == 200) {
                    QByteArray data = reply->readAll();          // ★ 先 readAll
                    QVariant lenVar = reply->header(QNetworkRequest::ContentLengthHeader);
                    qint64 expected = lenVar.isValid() ? lenVar.toLongLong() : -1;

                    if (expected < 0 || data.size() == expected) {
                        Download::SaveOrCreateFile(path, data);
                        ok = true;
                    } else {
                        logger->warning(QString("长度不符: 期望 %1 实际 %2，重试 %3/3")
                                            .arg(expected).arg(data.size()).arg(retry + 1));
                    }
                } else {
                    logger->error("HTTP " + QString::number(status) + " " + url);
                }
            } else if (timedOut) {
                logger->warning("超时，重试 " + QString::number(retry + 1) + "/3: " + url);
            } else {
                logger->error("下载错误: " + reply->errorString() + " " + url);
            }
            reply->deleteLater();            // ★ 最后 deleteLater
        }

        if (!ok) {
            logger->error("库下载失败(已重试3次): " + url);
        }

        sum++;
        if (sum % 50 == 0 || sum == total) {
            logger->info(threadIdHex + " 进度: " + QString::number(sum) + "/" + QString::number(total));
            emit LibrariesProgress(sum % 50 == 0 ? 50 : (sum % 50));
        }
    }
    emit LibrariesReady(false);
}

