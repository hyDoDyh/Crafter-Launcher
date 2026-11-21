#ifndef MINECRAFTVERSION_H
#define MINECRAFTVERSION_H
#include <QString>
class MinecraftVersion
{
public:
    MinecraftVersion(QString id,QString type,QString url,QString time,QString releaseTime);
    MinecraftVersion();
    QString getId(){return id;};
    QString getType(){return type;};
    QString getUrl(){return url;};
    QString getTime(){return time;};
    QString getReleaseTime(){return releaseTime;};
    void setId(QString val){id = val;};
    void setType(QString val){type = val;};
    void setUrl(QString val){url = val;};
    void setTime(QString val){time = val;};
    void setReleaseTime(QString val){releaseTime = val;};
private:
    QString id;
    QString type;
    QString url;
    QString time;
    QString releaseTime;
};

#endif // MINECRAFTVERSION_H
