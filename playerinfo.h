#ifndef PLAYERINFO_H
#define PLAYERINFO_H
#include <QString>
class PlayerInfo
{
public:
    PlayerInfo();
    QString Name;
    QString UUID;
    QString XUID;
    QString AuthAccessToken;
    QString ClientID;
    bool isOffline = true;

};

#endif // PLAYERINFO_H
