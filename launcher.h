#ifndef LAUNCHER_H
#define LAUNCHER_H
#include "minecraft.h"
#include "launcharguments.h"
#include "download.h"
class Launcher : public QObject
{
    Q_OBJECT
public:
    bool launch(QString mc,launchArguments &arg);
    QList<QString> getVersions();
    static Launcher* getInstance(){
        if(instance == NULL){
            return instance = new Launcher();
        }
        return instance;
    }
    bool matchRule(const QJsonObject &obj,const FeatureContext& ctx);
    QList<launchArguments> getAllConfiguration(QString now);
    void CreateDefultConfiguration(QString now,QString config);
    launchArguments LoadConfig(QString ver,QString config);
    void SaveConfig(QString ver,launchArguments config);
private:
    Launcher();
    static Launcher *instance;
    Logger *logger;
    Download *downloader;
signals:
    void onInfo(QString log);
    void onError(QString log);
    void onStart(QString log);
    void onExit(int exitCode);
    void onFail(QString log);
};

#endif // LAUNCHER_H
