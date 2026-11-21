#ifndef LAUNCHARGUMENTS_H
#define LAUNCHARGUMENTS_H
#include "playerinfo.h"
#include <QList>
#include "download.h"
// launcharguments.h
struct FeatureContext {
    bool hasCustomResolution  = false;
    bool isDemoUser           = false;
    bool hasQuickPlaysSupport = false;
    bool isQuickPlaySingleplayer = false;
    bool isQuickPlayMultiplayer  = false;
    bool isQuickPlayRealms       = false;
};
class launchArguments
{
public:
    static launchArguments Defult(QString now){
        PlayerInfo player;
        launchArguments arg;
        player.isOffline = 1;
        arg.argName = "new Config";
        player.AuthAccessToken = "0";
        player.ClientID = "0";
        player.Name = "TEST_PLAYER";
        player.UUID = "0";
        player.XUID = "0";
        arg.player = player;
        arg.Path = Download::getInstance()->getMainFolder(NULL) + "/versions/"+now+"/"+now+".json";
        arg.version = now;

        arg.quickPlay = false;
        arg.versionType = "release";
        arg.assetIndex = "0";//TODO
        arg.resolutionHeight = 800;
        arg.resolutionWidth = 600;
        arg.gameDir = Download::getInstance()->getMainFolder(NULL) + "/versions/"+now+"";
        arg.libPath = Download::getInstance()->getMainFolder(NULL) + "/libraries";
        return arg;
    }
    launchArguments();
    QString argName;
    QString Path;
    PlayerInfo player;
    QString gameDir;
    QString version;
    QString assetsDir;
    QString assetIndex;
    QString versionType;
    QString libPath;
    int resolutionWidth;
    int resolutionHeight;
    QString get(QString key);
    QString quickPlayPath;
    QString quickPlaySingleplayer;
    QString quickPlayMultiplayer;
    QString quickPlayRealms;
    bool quickPlay = false;
    QString classPath;
    FeatureContext ctx;
};

#endif // LAUNCHARGUMENTS_H
