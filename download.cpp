#include "download.h"
#include "downloadthread.h"
#include <QSettings>
#include <QDir>
#include <QFileDialog>
#include <QCoreApplication>
#include <QThread>
#include <QtConcurrent>
Download* Download::instance = nullptr;

Download::Download() {
}
int Download::Progess = 0;
int Download::LibrariesProgess = 0;
int Download::All = 0;
int Download::AllLibraries = 0;
void Download::getAllVersions(QString url){
    QNetworkRequest request;
    request.setUrl(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,"application/json");
    QNetworkReply *reply = networkManager->get(request);
    connect(reply,&QNetworkReply::finished,this,[=](){
        //获取完成
        if(reply->error() == QNetworkReply::NoError){
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if(doc.isNull()){
                logger->error("JSON解析失败!");
                return;
            }
            QJsonObject obj = doc.object();
            QJsonArray arr = obj["versions"].toArray();
            QList<MinecraftVersion> all = {};
            for(QJsonValue obj : arr){
                MinecraftVersion version = MinecraftVersion();
                version.setId(obj["id"].toString());
                version.setType(obj["type"].toString());
                version.setUrl(obj["url"].toString());
                version.setTime(obj["time"].toString());
                version.setReleaseTime(obj["releaseTime"].toString());
                all.append(version);
            }
            emit versionsReady(all);
        }else{
            logger->error("获取版本列表失败!");
        }
    });

}

void Download::setMainFolder(QWidget* wid){
    QSettings settings;
    QString dir = QFileDialog::getExistingDirectory(
        wid,
        "选择 Minecraft 游戏目录",
         QDir::homePath() ,

        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
        );
    qDebug() <<dir;

    if (!dir.isEmpty()) {
        settings.setValue("game/paths/main", dir);

        settings.sync();  // 强制写盘
        logger->info("已选择目录: " + dir);
    }
}
QString Download::getMainFolder(QWidget* wid){

    QSettings settings;
    QString last = settings.value("game/paths/main").toString();
    if(last.isEmpty()){
        QString dir = QFileDialog::getExistingDirectory(
            wid,
            "选择 Minecraft 游戏目录",
            last.isEmpty() ? QDir::homePath() : last,

            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
            );
        qDebug() <<dir;

        if (!dir.isEmpty()) {
            settings.setValue("game/paths/main", dir);

            settings.sync();  // 强制写盘
            logger->info("已选择目录: " + dir);
        }
        return dir;
    }else{

        return last;
    }
}
QByteArray Download::DownloadFile(QString url){
    QNetworkRequest request;
    QString folderPath = getMainFolder(0);
    request.setUrl(QUrl(url));
    QNetworkReply *reply = networkManager->get(request);

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    //同步等待(堵塞)
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    timer.start(10000);

    loop.exec();

    if(reply->error() != QNetworkReply::NoError){
        logger->error("下载文件"+url+"失败");
        return NULL;
    }
    QByteArray data = reply->readAll();
    reply->deleteLater();
    return data;
}
void Download::InstallMinecraft(QString url,QString Name){
    QString mainFolder = getMainFolder(NULL);

    QString folderPath = getMainFolder(0);
    QByteArray data = DownloadFile(url);
    QString path = QString("%1/versions/%2/%2.json").arg(folderPath).arg(Name);
    if(!SaveOrCreateFile(path,data)){
        logger->info("保存文件失败!");
    }
    QJsonDocument json = QJsonDocument::fromJson(data);

    QJsonObject obj = json.object();
    QJsonObject assetsIndex = obj["assetIndex"].toObject();
    QString AssetsUrl = assetsIndex["url"].toString();
    QString AssetsId = assetsIndex["id"].toString();
    logger->info("获取到assetIndex Url: "+AssetsUrl);
    //下载资源文件




    QByteArray AssetsData = DownloadFile(AssetsUrl);
    SaveOrCreateFile(mainFolder + "/assets/indexes/"+AssetsId+".json",AssetsData);
    QJsonDocument doc = QJsonDocument::fromJson(AssetsData);
    QList<QString> AssetsList;
    QJsonObject root = doc.object()["objects"].toObject();
    //分Count个线程进行下载
    int Count = 10;
    int cnt = 0;
    int lst = 0;
    int pr = root.size() / Count + 1;
    All = root.size();
    for (auto it = root.begin(); it != root.end(); ++it) {
        cnt++;
        QString key = it.key();
        QJsonValue value = it.value();
        if (value.isObject()) {
            QJsonObject obj = value.toObject();
            QString hash = obj["hash"].toString();
            AssetsList.append(hash);
        }
        if((cnt - lst )> pr){
            DownloadAssets(AssetsList,lst,AssetsList.size()-1,mainFolder+ "/assets/objects");
            lst = cnt;
        }
    }
    if(cnt != lst){

        DownloadAssets(AssetsList,lst,AssetsList.size()-1,mainFolder+ "/assets/objects");
    }
    QList<Library> list;
    QJsonArray libraries = obj["libraries"].toArray();
    qDebug() << libraries.size() << ",";
    lst = 0;
    cnt = 0;
    pr = libraries.size() / Count + 1;
    AllLibraries = libraries.size();
    //下载库文件
    for(QJsonValue n : libraries){
        cnt++;
        QJsonObject obj = n.toObject()["downloads"].toObject()["artifact"].toObject();
        QString path = obj["path"].toString();
        QString url = obj["url"].toString();
        QString name =  n.toObject()["downloads"].toObject()["name"].toString();
        list.append(Library(path,url,name));
        if((cnt - lst) > pr){
            DownloadLibraries(list,lst,list.size()-1,mainFolder+"/libraries");
            lst = cnt;
        }
    }
    if(cnt != lst){
        DownloadLibraries(list,lst,list.size()-1,mainFolder+"/libraries");
    }
    //下载游戏本体
    QJsonObject downloads = obj["downloads"].toObject();
    QJsonObject client = downloads["client"].toObject();
    QString url2 = client["url"].toString();
    logger->info("获取到主文件地址:"+url2);
    QString path2 = QString("%1/versions/%2/%2.jar").arg(folderPath).arg(Name);
    QByteArray reply = DownloadFile(url2);
    SaveOrCreateFile(path2,reply);
}
bool Download::SaveOrCreateFile(const QString &filePath, const QByteArray &content)
{
    QFileInfo info(filePath);
    QDir().mkpath(info.absolutePath());

    QSaveFile file(filePath);                    // 原子写入：先写临时文件，commit 才生效
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(content) != content.size()) {
        file.cancelWriting();                    // 丢弃临时文件，不污染磁盘
        return false;
    }
    return file.commit();                        // 真正 rename
}

void Download::DownloadLibraries(QList<Library>list,int start,int end,QString path){
    QThread *thread = new QThread(this);
    DownloadThread *worker = new DownloadThread();

    worker->moveToThread(thread);

    connect(thread,&QThread::started,worker,[worker,list,start,end,path](){
        worker->DownloadLibraries(list,start,end,path);
    });
    connect(worker,&DownloadThread::LibrariesReady,this,[this,start,end](bool error){
        if(error)
            this->logger->error("下载"+QString::number(start)+"到"+QString::number(end)+"的库文件失败！");
        else
            this->logger->info("下载"+QString::number(start)+"到"+QString::number(end)+"的库文件下载成功！");
    });
    connect(worker,&DownloadThread::LibrariesProgress,this,[this,start,end](int pr){
        LibrariesProgess += pr;
        logger->info(QString::number(LibrariesProgess)+"/"+QString::number(AllLibraries));
    });

    connect(worker,&DownloadThread::LibrariesReady,thread,&QThread::quit);
    connect(thread,&QThread::finished,worker,&QObject::deleteLater);
    connect(thread,&QThread::finished,worker,&QObject::deleteLater);
    thread->start();

}
void Download::DownloadAssets(QList<QString> list,int start,int end,QString Path){

    // 创建线程和 Worker
    QThread *thread = new QThread(this);
    DownloadThread *worker = new DownloadThread();

    // 将 Worker 移动到子线程
    worker->moveToThread(thread);

    // 连接信号
    connect(thread, &QThread::started, worker, [worker,list,start,end,Path](){
        worker->DownloadAssets(list,start,end,Path);
    });
    connect(worker, &DownloadThread::resultReady, this, [this,start,end](bool error){
        if(error)
            this->logger->error("下载"+QString::number(start)+"到"+QString::number(end)+"的资源文件失败！");
        else
            this->logger->info("下载"+QString::number(start)+"到"+QString::number(end)+"的资源文件下载成功！");
    });
    connect(worker, &DownloadThread::progress, this,[=](int pr){
        Progess += pr;
        logger->info(QString::number(Progess)+"/"+QString::number(All));
    });

    // 清理
    connect(worker, &DownloadThread::resultReady, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    // 启动线程
    thread->start();
}
