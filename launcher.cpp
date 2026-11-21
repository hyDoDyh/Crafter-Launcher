#include "launcher.h"
#include "winversion.h"
#include <QDir>
#include <QProcess>
#include <QSettings>
#include <QuaZip-Qt6-1.7.2/quazip/JlCompress.h>
Launcher::Launcher() {
    logger = Logger::getInstance();
    downloader = Download::getInstance();
}
bool Launcher::matchRule(const QJsonObject &obj,const FeatureContext& ctx) {
    bool allowed = false;
    for (const QJsonValue &v : obj["rules"].toArray()) {
        QJsonObject rule = v.toObject();
        bool match = true;
        if (rule.contains("os")) {
            QJsonObject os = rule["os"].toObject();
            QString name = os["name"].toString();
            QString cur;
#if defined(Q_OS_WIN)
            cur = "windows";
#elif defined(Q_OS_LINUX)
            cur = "linux";
#elif defined(Q_OS_MAC)
            cur = "osx";
#endif
            if (!name.isEmpty() && name != cur) match = false;

#if defined(Q_OS_WIN)
            if (match && os.contains("versionRange")) {
                QJsonObject vr = os["versionRange"].toObject();
                QVersionNumber sys =QOperatingSystemVersion::current().version();
                if (vr.contains("min") &&
                    sys < QVersionNumber::fromString(vr["min"].toString()))
                    match = false;
                if (vr.contains("max") &&
                    sys > QVersionNumber::fromString(vr["max"].toString()))
                    match = false;
            }
#endif
        }
        if (rule.contains("arch")) {
            QString a = QSysInfo::currentCpuArchitecture();
            QString mapped = (a == "x86_64") ? "x64"
                             : (a == "i386")   ? "x86"
                                             : a;              // arm64 原样
            if (mapped != rule["arch"].toString()) match = false;
        }
        if (rule.contains("features")) {
            QJsonObject feat = rule["features"].toObject();
            for (const QString &key : feat.keys()) {
                bool ruleVal = feat[key].toBool();
                bool ctxVal  = false;
                if      (key == "has_custom_resolution")    ctxVal = ctx.hasCustomResolution;
                else if (key == "is_demo_user")             ctxVal = ctx.isDemoUser;
                else if (key == "has_quick_plays_support")  ctxVal = ctx.hasQuickPlaysSupport;
                else if (key == "is_quick_play_singleplayer") ctxVal = ctx.isQuickPlaySingleplayer;
                else if (key == "is_quick_play_multiplayer")  ctxVal = ctx.isQuickPlayMultiplayer;
                else if (key == "is_quick_play_realms")       ctxVal = ctx.isQuickPlayRealms;
                else ctxVal = false;   // 未知 feature 一律视为不支持

                if (ruleVal != ctxVal) { match = false; break; }
            }
        }
        if (match)
            allowed = (rule["action"].toString() != "disallow");
    }
    return allowed;
}
bool Launcher::launch(QString mc,launchArguments &arg){
    FeatureContext ctx = arg.ctx;
    QList<QString> args;
    QFile file(arg.Path);
    // 读取文件
    QByteArray data;
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        data = file.readAll();
        file.close();
    }else{
        logger->error("无法打开游戏json文件");
        return false;
    }
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data,&err);
    if (doc.isNull()) {
        logger->error("JSON 解析失败: " + err.errorString());
        return false;
    }
    QJsonObject obj = doc.object();
    QJsonObject arguments = obj["arguments"].toObject();

    QProcess *process = new QProcess(this);
    for(QJsonValue value : arguments["default-user-jvm"].toArray()){
        if(value.isString()){
            args.append(value.toString());
        }
        else{
            QJsonObject valobj = value.toObject();
            if(valobj.contains("rules")){
                if(matchRule(valobj,ctx)){
                    for(QJsonValue val : valobj["value"].toArray()){
                        args.append(val.toString());
                    }
                }
            }else{
                for(QJsonValue val : valobj["value"].toArray()){
                    args.append(val.toString());
                }
            }
        }
    }
    for(QJsonValue value : arguments["jvm"].toArray()){
        if(value.isString()){
            args.append(value.toString());
        }
        else{
            QJsonObject valobj = value.toObject();
            if(valobj.contains("rules")){
                if(matchRule(valobj,ctx)){
                    for(QJsonValue val : valobj["value"].toArray()){
                        args.append(val.toString());
                    }
                }
            }else{
                for(QJsonValue val : valobj["value"].toArray()){
                    args.append(val.toString());
                }
            }
        }
    }
    QString mainClass = obj["mainClass"].toString();
    if (mainClass.isEmpty()) {
        logger->error("JSON 缺少 mainClass 字段");
        process->deleteLater();
        return false;
    }
    args.append(mainClass);

    for (QJsonValue value : arguments["game"].toArray()) {
        if (value.isString()) {
            args.append(value.toString());
        } else {
            QJsonObject o = value.toObject();
            if (!o.contains("rules") || matchRule(o,ctx)) {
                for (QJsonValue val : o["value"].toArray())
                    args.append(val.toString());
            }
        }
    }
    QString platformKey;
#if defined(Q_OS_WIN)
    platformKey = "natives-windows";
#elif defined(Q_OS_MAC)
    platformKey = (QSysInfo::currentCpuArchitecture() == "arm64")
                      ? "natives-macos-arm64" : "natives-macos";
#elif defined(Q_OS_LINUX)
    platformKey = "natives-linux";
#endif
    //获取ClassPath
    QJsonArray libraries = obj["libraries"].toArray();
    QString cp = "";
    QString natives = arg.get("${natives_directory}");
    for(QJsonValue library : libraries){
        QJsonObject libObj = library.toObject();
        if (libObj.contains("rules") && !matchRule(libObj,ctx)) continue;

        QString rel = libObj["downloads"].toObject()
                          ["artifact"].toObject()["path"].toString();
        if (rel.isEmpty()) continue;

        QString libPath = arg.libPath+"/"+library.toObject()["downloads"].toObject()["artifact"].toObject()["path"].toString();
        const QString sep = QString(QDir::listSeparator());
        cp += libPath + sep;
        if (rel.contains(platformKey)) {
            QDir nd(natives);
            nd.mkpath(".");
            // 用标记文件避免重复解压（比 entryList 判空更稳）
            QString mark = nd.filePath("." + QFileInfo(libPath).baseName() + ".ok");
            if (!QFile::exists(mark)) {
                QStringList extracted = JlCompress::extractDir(libPath, natives);
                if (extracted.isEmpty()) {
                    logger->error("natives 解压失败: " + libPath + " → " + natives);
                    process->deleteLater();
                    return false;
                }
                QFile(mark).open(QIODevice::WriteOnly);  // 创建标记
            }
        }
    }
    cp += arg.gameDir+"/"+arg.version+".jar";
    arg.classPath = cp;
    for(int i = 0;i < args.size();i++){
        args[i] = arg.get(args[i]);
    }

    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QByteArray data = process->readAllStandardOutput();
        emit onInfo( QString::fromLocal8Bit(data));

    });

    connect(process, &QProcess::readyReadStandardError, this, [=]() {
        QByteArray data = process->readAllStandardError();
        emit onError( QString::fromLocal8Bit(data));
    });

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, [=](int exitCode, QProcess::ExitStatus exitStatus) {

                emit onExit(exitCode);
            });

    connect(process, &QProcess::started, this, [=]{
        logger->info("Java 已启动，PID=" + QString::number(process->processId()));
        emit onStart(QString::number(process->processId()));
    });
    connect(process, &QProcess::errorOccurred, this, [=](QProcess::ProcessError){
        logger->error("Java 启动失败: " + process->errorString());
        emit onFail(process->errorString());
    });
    process->start("java", args);
    return true;
}
Launcher *Launcher::instance;
launchArguments Launcher::LoadConfig(QString ver,QString config){
    QSettings settings;
    launchArguments arg = launchArguments();

    auto cfg = [=](const QString &x) {
        return QStringLiteral("Launcher/Configuration/") + ver + "/" + config + "/" + x;
    };
    arg.argName     = settings.value(cfg("ConfigName")).toString();
    // ── 路径与版本信息 ─────────────────────────
    arg.Path        = settings.value(cfg("Path")).toString();
    arg.gameDir     = settings.value(cfg("GameDir")).toString();
    arg.version     = settings.value(cfg("Version")).toString();
    arg.assetsDir   = settings.value(cfg("AssetsDir")).toString();
    arg.assetIndex  = settings.value(cfg("AssetIndex")).toString();
    arg.versionType = settings.value(cfg("VersionType"), "release").toString();
    arg.libPath     = settings.value(cfg("LibPath")).toString();

    // ── 窗口分辨率 ─────────────────────────────
    arg.resolutionWidth  = settings.value(cfg("ResolutionWidth"),  854).toInt();
    arg.resolutionHeight = settings.value(cfg("ResolutionHeight"), 480).toInt();

    // ── QuickPlay 目标 ─────────────────────────
    arg.quickPlayPath         = settings.value(cfg("QuickPlayPath")).toString();
    arg.quickPlaySingleplayer = settings.value(cfg("QuickPlaySingleplayer")).toString();
    arg.quickPlayMultiplayer  = settings.value(cfg("QuickPlayMultiplayer")).toString();
    arg.quickPlayRealms       = settings.value(cfg("QuickPlayRealms")).toString();

    arg.quickPlay = !arg.quickPlaySingleplayer.isEmpty()
                    || !arg.quickPlayMultiplayer.isEmpty()
                    || !arg.quickPlayRealms.isEmpty();

    // ── 玩家账户信息 ────────────────────────────
    arg.player.Name            = settings.value(cfg("Account/Name")).toString();
    arg.player.UUID            = settings.value(cfg("Account/UUID")).toString();
    arg.player.XUID            = settings.value(cfg("Account/XUID")).toString();
    arg.player.AuthAccessToken = settings.value(cfg("Account/AuthAccessToken")).toString();
    arg.player.ClientID        = settings.value(cfg("Account/ClientID")).toString();
    arg.player.isOffline       = settings.value(cfg("Account/IsOffline"), true).toBool();

    // ── FeatureContext ─────────────────────────
    arg.ctx.hasCustomResolution = (arg.resolutionWidth > 0 && arg.resolutionHeight > 0);
    arg.ctx.isDemoUser          = false;
    arg.ctx.hasQuickPlaysSupport = settings.value(cfg("FeatureContext/HasQuickPlaysSupport"), true).toBool();

    arg.ctx.isQuickPlaySingleplayer = !arg.quickPlaySingleplayer.isEmpty();
    arg.ctx.isQuickPlayMultiplayer  = !arg.quickPlayMultiplayer.isEmpty();
    arg.ctx.isQuickPlayRealms       = !arg.quickPlayRealms.isEmpty();

    // arg.classPath 不在这里赋值：launch() 内部拼完 libraries 后回写
    return arg;
}
void Launcher::CreateDefultConfiguration(QString now,QString Config){
    QSettings settings;
    auto cfg = [=](const QString &x) {
        return QStringLiteral("Launcher/Configuration/") + now + "/" + Config + "/"+ x;
    };

    const QString main = downloader->getMainFolder(NULL);
    // 假设 now 已经在上文定义
    settings.setValue(cfg("ConfigName"),Config);
    // ── 账户（离线测试账户）────────────────────
    settings.setValue(cfg("Account/Name"),            "TEST_PLAYER");
    settings.setValue(cfg("Account/UUID"),            "0");
    settings.setValue(cfg("Account/XUID"),            "0");
    settings.setValue(cfg("Account/AuthAccessToken"), "0");
    settings.setValue(cfg("Account/ClientID"),        "0");
    settings.setValue(cfg("Account/IsOffline"),       true);

    // ── 路径 / 版本 ───────────────────────────
    settings.setValue(cfg("Path"),        main + "/versions/" + now + "/" + now + ".json");
    settings.setValue(cfg("Version"),     now);
    settings.setValue(cfg("VersionType"), "release");
    settings.setValue(cfg("GameDir"),     main + "/versions/" + now);
    settings.setValue(cfg("LibPath"),     main + "/libraries");
    settings.setValue(cfg("AssetsDir"),   main + "/assets");   // 上轮读取有这项，测试也得给
    settings.setValue(cfg("AssetIndex"),  "0");                 // TODO

    // ── 分辨率 ────────────────────────────────
    settings.setValue(cfg("ResolutionWidth"),  600);
    settings.setValue(cfg("ResolutionHeight"), 800);

    // ── QuickPlay（测试阶段全部关闭）─────────
    // arg.quickPlay = false 等价于下面四个都是空：
    settings.setValue(cfg("QuickPlayPath"),         "");
    settings.setValue(cfg("QuickPlaySingleplayer"), "");
    settings.setValue(cfg("QuickPlayMultiplayer"),  "");
    settings.setValue(cfg("QuickPlayRealms"),       "");

    // ── FeatureContext ────────────────────────
    // 测试阶段关掉 --quickPlayPath 的注入，避免参数里出现 --quickPlayPath ""
    settings.setValue(cfg("FeatureContext/HasQuickPlaysSupport"), false);

    settings.sync();   // 确保立即落盘，后续 launch() 读得到
}
QList<QString> Launcher::getVersions(){

    qDebug()<<downloader->getMainFolder(NULL)+"/versions/";
    QDir dir(downloader->getMainFolder(NULL)+"/versions/");
    QStringList subDirs = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    QList<QString> result;
    for(const QString &name : subDirs){
        result.append(name);
    }
    return result;
}
QList<launchArguments> Launcher::getAllConfiguration(QString now){
    QList<launchArguments> ans = QList<launchArguments>();
    QSettings settings;

    auto cfgs = QStringLiteral("Launcher/Configuration/") + now + "/ConfigurationList";
    if(!settings.contains(cfgs)){
        settings.setValue(cfgs,QStringList("new Config"));

        ans.append(launchArguments::Defult(now));

        CreateDefultConfiguration(now,"new Config");
        return ans;
    }
    QStringList lst = settings.value(cfgs).toString().split(";");
    for(QString str : lst){
        launchArguments arg = LoadConfig(now,str);
        ans.append(arg);
        // arg.classPath 不在这里赋值：launch() 内部拼完 libraries 后会回写
    }
    return ans;
}
void Launcher::SaveConfig(QString now,launchArguments Config){

    QSettings settings;
    auto cfg = [=](const QString &x) {
        return QStringLiteral("Launcher/Configuration/") + now + "/" + Config.argName + "/"+ x;
    };

    const QString main = downloader->getMainFolder(NULL);
    // 假设 now 已经在上文定义
    settings.setValue(cfg("ConfigName"),Config.argName);
    // ── 账户（离线测试账户）────────────────────
    settings.setValue(cfg("Account/Name"),            Config.player.Name);
    settings.setValue(cfg("Account/UUID"),            Config.player.UUID);
    settings.setValue(cfg("Account/XUID"),            Config.player.XUID);
    settings.setValue(cfg("Account/AuthAccessToken"), Config.player.AuthAccessToken);
    settings.setValue(cfg("Account/ClientID"),        Config.player.ClientID);
    settings.setValue(cfg("Account/IsOffline"),       Config.player.isOffline);

    // ── 路径 / 版本 ───────────────────────────
    settings.setValue(cfg("Path"),        main + "/versions/" + now + "/" + now + ".json");
    settings.setValue(cfg("Version"),     now);
    settings.setValue(cfg("VersionType"), "release");
    settings.setValue(cfg("GameDir"),     main + "/versions/" + now);
    settings.setValue(cfg("LibPath"),     main + "/libraries");
    settings.setValue(cfg("AssetsDir"),   main + "/assets");   // 上轮读取有这项，测试也得给
    settings.setValue(cfg("AssetIndex"),  "0");                 // TODO

    // ── 分辨率 ────────────────────────────────
    settings.setValue(cfg("ResolutionWidth"),  600);
    settings.setValue(cfg("ResolutionHeight"), 800);

    // ── QuickPlay（测试阶段全部关闭）─────────
    // arg.quickPlay = false 等价于下面四个都是空：
    settings.setValue(cfg("QuickPlayPath"),         "");
    settings.setValue(cfg("QuickPlaySingleplayer"), "");
    settings.setValue(cfg("QuickPlayMultiplayer"),  "");
    settings.setValue(cfg("QuickPlayRealms"),       "");

    // ── FeatureContext ────────────────────────
    // 测试阶段关掉 --quickPlayPath 的注入，避免参数里出现 --quickPlayPath ""
    settings.setValue(cfg("FeatureContext/HasQuickPlaysSupport"), false);

    settings.sync();   // 确保立即落盘，后续 launch() 读得到
}
