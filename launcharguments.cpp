#include "launcharguments.h"
#include <QDir>
launchArguments::launchArguments() {

}

QString launchArguments::get(QString key){
    QString keys[] = {
        "${auth_player_name}",
        "${version_name}",
        "${game_directory}",
        "${assets_root}",
        "${assets_index_name}",
        "${auth_uuid}",
        "${auth_access_token}",
        "${clientid}",
        "${auth_xuid}",
        "${version_type}",
        "${resolution_width}",
        "${resolution_height}",
        "${quickPlayPath}",
        "${quickPlaySingleplayer}",
        "${quickPlayMultiplayer}",
        "${quickPlayRealms}",
        "${natives_directory}",
        "${launcher_name}",
        "${launcher_version}",
        "${classpath}",
        "${library_directory}",
        "${classpath_separator}",
        "${game_assets}",
        "${auth_session}",
        "${user_properties}",
        "${user_type}",
    };

    QString values[] = {
        player.Name,
        version,
        gameDir,
        assetsDir,
        assetIndex,
        player.UUID,
        player.AuthAccessToken,
        player.ClientID,
        player.XUID,
        versionType,
        QString::number(resolutionWidth),
        QString::number(resolutionHeight),
        quickPlayPath,
        quickPlaySingleplayer,
        quickPlayMultiplayer,
        quickPlayRealms,
        gameDir + "/" + version + "-natives/",
        "Crafter Launcher",
        "1.0.0",
        classPath,
        libPath,
        QString(QDir::listSeparator()),
        assetsDir + "/virtual/legacy",
        "token:" + player.AuthAccessToken + ":" + player.UUID,
        "{}",
        player.isOffline ? "legacy" : "msa"
    };
    int i = 0;
    for(QString str : keys){
        key.replace(str,values[i]);
        i++;
    }
    return key;
}

